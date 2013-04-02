/** \file
 *  \ingroup commandLineTools 
 *  \details This program is used to classify fissure particles using Fischer's
 *  Linear Discriminant. Left or right lung fissure particles are read in
 *  along with lobe boundary shape models for the left or right lung. For
 *  each particle, its distance and angle with respect to the lobe
 *  boundaries are computed. The weighted sum of these quantities is then
 *  computed and compared to a threshold value, and a classification
 *  decision is made (either fissure or noise). If particles in the right
 *  lung are being considered, a particle is classified according to which
 *  entity it is most like (noise, right horizontal or right oblique). The
 *  classified particles are then written to file.
 * 
 *  USAGE: 
 *
 *  ClassifyFissureParticles  [-t \<string\>] [-a \<string\>] [-d \<string\>]
 *                            [--rhClassified \<string\>] 
 *                            [--roClassified \<string\>] [--loClassified \<string\>]
 *                            [--rhModel \<string\>] [--roModel \<string\>]
 *                            [--loModel \<string\>] -p \<string\> [--]
 *                            [--version] [-h]
 *
 *  Where: 
 *
 *  -t \<string\>,  --thresh \<string\>
 *    Threshold for Fischer discriminant based classification
 *
 *  -a \<string\>,  --angle \<string\>
 *    Angle weight for Fischer discriminant projection
 *
 *  -d \<string\>,  --dist \<string\>
 *    Distance weight for Fischer discriminant projection
 *
 *  --rhClassified \<string\>
 *    Right horizontal classified particles file name
 *
 *  --roClassified \<string\>
 *    Right oblique classified particles file name
 *
 *  --loClassified \<string\>
 *    Left oblique classified particles file name
 *
 *  --rhModel \<string\>
 *    Right horizontal shape model file name. If specified, a right oblique
 *    shape model most also be specified.
 *
 *  --roModel \<string\>
 *    Right oblique shape model file name. If specified, a right horizontal
 *    shape model most also be specified.
 *
 *  --loModel \<string\>
 *    Left oblique shape model file name
 *
 *  -p \<string\>,  --particles \<string\>
 *    (required)  Particles file name
 *
 *  --,  --ignore_rest
 *    Ignores the rest of the labeled arguments following this flag.
 *
 *  --version
 *    Displays version information and exits.
 *
 *  -h,  --help
 *    Displays usage information and exits.
 *
 *  $Date: 2012-09-10 13:37:14 -0400 (Mon, 10 Sep 2012) $
 *  $Revisionw$
 *  $Author: jross $
 *
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <tclap/CmdLine.h>
#include "cipConventions.h"
#include "vtkSmartPointer.h"
#include "vtkPolyDataReader.h"
#include "vtkPolyDataWriter.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "itkNumericTraits.h"
#include "vtkFieldData.h"
#include "vtkPolyData.h"
#include "cipNewtonOptimizer.h"
#include "cipThinPlateSplineSurface.h"
#include "cipNelderMeadSimplexOptimizer.h"
#include "cipThinPlateSplineSurfaceModelToParticlesMetric.h"
#include "cipConventions.h"
#include "cipLobeBoundaryShapeModelIO.h"


struct PARTICLEINFO
{
  std::vector< double > distance;
  std::vector< double > angle;
  unsigned char         cipType;
};


double GetVectorMagnitude( double[3] );
double GetAngleBetweenVectors( double[3], double[3], bool );
void GetParticleDistanceAndAngle( vtkPolyData*, unsigned int, cipThinPlateSplineSurface*, double*, double* );
void TallyParticleInfo( vtkPolyData*, std::vector< cipThinPlateSplineSurface* >, std::map< unsigned int, PARTICLEINFO >* );
void ClassifyParticles( std::map< unsigned int, PARTICLEINFO >*, std::vector< cipThinPlateSplineSurface* >, double, double, double );
void WriteParticlesToFile( vtkSmartPointer< vtkPolyData >, std::map< unsigned int, PARTICLEINFO >, std::string, unsigned char );


int main( int argc, char *argv[] )
{
  //
  // Define arguments
  //
  std::string particlesFileName     = "NA";
  std::string loShapeModelFileName  = "NA";
  std::string roShapeModelFileName  = "NA";
  std::string rhShapeModelFileName  = "NA";
  std::string loClassifiedFileName  = "NA";
  std::string roClassifiedFileName  = "NA";
  std::string rhClassifiedFileName  = "NA";

  double distanceWeight = -0.4677;
  double angleWeight    = -0.8839;
  double threshold      = -30.0;

  //
  // Program and argument descriptions for user help
  //
  std::string programDesc = "This program is used to classify fissure particles using Fischer's Linear \
Discriminant. Left or right lung fissure particles are read in along with lobe boundary shape models \
for the left or right lung. For each particle, its distance and angle with respect to the lobe boundaries \
are computed. The weighted sum of these quantities is then computed and compared to a threshold value, \
and a classification decision is made (either fissure or noise). If particles in the right lung are \
being considered, a particle is classified according to which entity it is most like (noise, right \
horizontal or right oblique). The classified particles are then written to file.";

  std::string particlesFileNameDesc = "Particles file name";
  std::string loShapeModelFileNameDesc = "Left oblique shape model file name";
  std::string roShapeModelFileNameDesc = "Right oblique shape model file name. If specified, a right \
horizontal shape model most also be specified.";
  std::string rhShapeModelFileNameDesc = "Right horizontal shape model file name. If specified, a right \
oblique shape model most also be specified.";
  std::string loClassifiedFileNameDesc = "Left oblique classified particles file name";
  std::string roClassifiedFileNameDesc = "Right oblique classified particles file name";
  std::string rhClassifiedFileNameDesc = "Right horizontal classified particles file name";
  std::string distanceWeightDesc = "Distance weight for Fischer discriminant projection";
  std::string angleWeightDesc = "Angle weight for Fischer discriminant projection";
  std::string thresholdDesc = "Threshold for Fischer discriminant based classification";

  //
  // Parse the input arguments
  //
  try
    {
    TCLAP::CmdLine cl( programDesc, ' ', "$Revision: 257 $" );

    TCLAP::ValueArg<std::string> particlesFileNameArg( "p", "particles", particlesFileNameDesc, true, particlesFileName, "string", cl );
    TCLAP::ValueArg<std::string> loShapeModelFileNameArg( "", "loModel", loShapeModelFileNameDesc, false, loShapeModelFileName, "string", cl );
    TCLAP::ValueArg<std::string> roShapeModelFileNameArg( "", "roModel", roShapeModelFileNameDesc, false, roShapeModelFileName, "string", cl );
    TCLAP::ValueArg<std::string> rhShapeModelFileNameArg( "", "rhModel", rhShapeModelFileNameDesc, false, rhShapeModelFileName, "string", cl );
    TCLAP::ValueArg<std::string> loClassifiedFileNameArg( "", "loClassified", loClassifiedFileNameDesc, false, loClassifiedFileName, "string", cl );
    TCLAP::ValueArg<std::string> roClassifiedFileNameArg( "", "roClassified", roClassifiedFileNameDesc, false, roClassifiedFileName, "string", cl );
    TCLAP::ValueArg<std::string> rhClassifiedFileNameArg( "", "rhClassified", rhClassifiedFileNameDesc, false, rhClassifiedFileName, "string", cl );
    TCLAP::ValueArg<double> distanceWeightArg( "d", "dist", distanceWeightDesc, false, distanceWeight, "string", cl );
    TCLAP::ValueArg<double> angleWeightArg( "a", "angle", angleWeightDesc, false, angleWeight, "string", cl );
    TCLAP::ValueArg<double> thresholdArg( "t", "thresh", thresholdDesc, false, threshold, "string", cl );

    cl.parse( argc, argv );

    particlesFileName    = particlesFileNameArg.getValue();
    loShapeModelFileName = loShapeModelFileNameArg.getValue();
    roShapeModelFileName = roShapeModelFileNameArg.getValue();
    rhShapeModelFileName = rhShapeModelFileNameArg.getValue();
    loClassifiedFileName = loClassifiedFileNameArg.getValue();
    roClassifiedFileName = roClassifiedFileNameArg.getValue();
    rhClassifiedFileName = rhClassifiedFileNameArg.getValue();
    distanceWeight       = distanceWeightArg.getValue();
    angleWeight          = angleWeightArg.getValue();
    threshold            = thresholdArg.getValue();
    }
  catch ( TCLAP::ArgException excp )
    {
    std::cerr << "Error: " << excp.error() << " for argument " << excp.argId() << std::endl;
    return cip::ARGUMENTPARSINGERROR;
    }

  //
  // Read complete particle in lung
  //
  std::cout << "Reading lung particles..." << std::endl;
  vtkSmartPointer< vtkPolyDataReader > particlesReader = vtkSmartPointer< vtkPolyDataReader >::New();
    particlesReader->SetFileName( particlesFileName.c_str() );
    particlesReader->Update();

  //
  // Read shape models
  //
  std::vector< cipThinPlateSplineSurface* > tpsVec;

  if ( (roShapeModelFileName.compare( "NA" ) != 0 && rhShapeModelFileName.compare( "NA" ) == 0) || 
       (roShapeModelFileName.compare( "NA" ) == 0 && rhShapeModelFileName.compare( "NA" ) != 0) )
    {
    std::cerr << "ERROR: If one shape model in the right lung is specified, they both must be. Exiting." << std::endl;
    return cip::ARGUMENTPARSINGERROR;
    }

  if ( roShapeModelFileName.compare( "NA" ) != 0 )
    { 
    std::cout << "Reading right oblique shape model..." << std::endl;
    cipLobeBoundaryShapeModelIO* roShapeModelIO = new cipLobeBoundaryShapeModelIO(); 
      roShapeModelIO->SetFileName( roShapeModelFileName );
      roShapeModelIO->Read();

    cipThinPlateSplineSurface* roTPS = new cipThinPlateSplineSurface();
      roTPS->SetSurfacePoints( roShapeModelIO->GetOutput()->GetWeightedSurfacePoints() );

    //
    // Note ordering is important here. The RO needs to be pushed back
    // before the RH (assumed when we execute 'TallyParticleInfo') 
    //
    tpsVec.push_back( roTPS );
    
    std::cout << "Reading right horizontal shape model..." << std::endl;
    cipLobeBoundaryShapeModelIO* rhShapeModelIO = new cipLobeBoundaryShapeModelIO();
      rhShapeModelIO->SetFileName( rhShapeModelFileName );
      rhShapeModelIO->Read();

    cipThinPlateSplineSurface* rhTPS = new cipThinPlateSplineSurface();
      rhTPS->SetSurfacePoints( rhShapeModelIO->GetOutput()->GetWeightedSurfacePoints() );

    //
    // Note ordering is important here. The RO needs to be pushed back
    // before the RH (assumed when we execute 'TallyParticleInfo') 
    //
    tpsVec.push_back( rhTPS );
    }
  else if ( loShapeModelFileName.compare( "NA" ) != 0 )
    {
    std::cout << "Reading left oblique shape model..." << std::endl;
    cipLobeBoundaryShapeModelIO* loShapeModelIO = new cipLobeBoundaryShapeModelIO();
      loShapeModelIO->SetFileName( loShapeModelFileName );
      loShapeModelIO->Read();

    cipThinPlateSplineSurface* loTPS = new cipThinPlateSplineSurface();
      loTPS->SetSurfacePoints( loShapeModelIO->GetOutput()->GetWeightedSurfacePoints() );

    tpsVec.push_back( loTPS );
    }
  else
    {
    std::cerr << "ERROR: No shape model specified. Exiting." << std::endl;
    return 1;
    }

  //
  // Now we want to tally the component information, computing the
  // mean distance and the mean angle with respect to the fit surface 
  //
  std::map< unsigned int, PARTICLEINFO >  particleToInfoMap;

  TallyParticleInfo( particlesReader->GetOutput(), tpsVec, &particleToInfoMap );

  //
  // Now classify the particles
  //
  std::cout << "Classifying particles..." << std::endl;
  ClassifyParticles( &particleToInfoMap, tpsVec, distanceWeight, angleWeight, threshold );

  //
  // Write the classified (fissure) particles to file
  //
  if ( loClassifiedFileName.compare( "NA" ) != 0 )
    {
    std::cout << "Writing left oblique particles to file..." << std::endl;
    WriteParticlesToFile( particlesReader->GetOutput(), particleToInfoMap, loClassifiedFileName, static_cast< unsigned char >( cip::OBLIQUEFISSURE ) );
    }
  if ( roClassifiedFileName.compare( "NA" ) != 0 )
    {
    std::cout << "Writing right oblique particles to file..." << std::endl;
    WriteParticlesToFile( particlesReader->GetOutput(), particleToInfoMap, roClassifiedFileName, static_cast< unsigned char >( cip::OBLIQUEFISSURE ) );
    }
  if ( rhClassifiedFileName.compare( "NA" ) != 0 )
    {
    std::cout << "Writing right horizontal particles to file..." << std::endl;
    WriteParticlesToFile( particlesReader->GetOutput(), particleToInfoMap, rhClassifiedFileName, static_cast< unsigned char >( cip::HORIZONTALFISSURE ) );
    }
  
  std::cout << "DONE." << std::endl;

  return 0;
}


double GetVectorMagnitude( double vector[3] )
{
  double magnitude = vcl_sqrt( std::pow( vector[0], 2 ) + std::pow( vector[1], 2 ) + std::pow( vector[2], 2 ) );

  return magnitude;
}


double GetAngleBetweenVectors( double vec1[3], double vec2[3], bool returnDegrees )
{
  double vec1Mag = GetVectorMagnitude( vec1 );
  double vec2Mag = GetVectorMagnitude( vec2 );

  double arg = (vec1[0]*vec2[0] + vec1[1]*vec2[1] + vec1[2]*vec2[2])/(vec1Mag*vec2Mag);

  if ( abs( arg ) > 1.0 )
    {
    arg = 1.0;
    }

  double angle = acos( arg );

  if ( !returnDegrees )
    {
    return angle;
    }

  double angleInDegrees = (180.0/3.14159265358979323846)*angle;

  if ( angleInDegrees > 90.0 )
    {
    angleInDegrees = 180.0 - angleInDegrees;
    }

  return angleInDegrees;
}


void GetParticleDistanceAndAngle( vtkPolyData* particles, unsigned int whichParticle, cipThinPlateSplineSurface* tps,
                                  double* distance, double* angle ) 
{
  cipParticleToThinPlateSplineSurfaceMetric* particleToTPSMetric = new cipParticleToThinPlateSplineSurfaceMetric();
    particleToTPSMetric->SetThinPlateSplineSurface( tps );

  cipNewtonOptimizer< 2 >* newtonOptimizer = new cipNewtonOptimizer< 2 >();
    newtonOptimizer->SetMetric( particleToTPSMetric );

  double* position    = new double[3];
  double* normal      = new double[3];
  double* orientation = new double[3];

  cipNewtonOptimizer< 2 >::PointType* domainParams  = new cipNewtonOptimizer< 2 >::PointType( 2, 2 );
  cipNewtonOptimizer< 2 >::PointType* optimalParams = new cipNewtonOptimizer< 2 >::PointType( 2, 2 );

  position[0] = particles->GetPoint(whichParticle)[0];
  position[1] = particles->GetPoint(whichParticle)[1];
  position[2] = particles->GetPoint(whichParticle)[2];
  
  orientation[0] = particles->GetFieldData()->GetArray( "hevec2" )->GetTuple(whichParticle)[0];
  orientation[1] = particles->GetFieldData()->GetArray( "hevec2" )->GetTuple(whichParticle)[1];
  orientation[2] = particles->GetFieldData()->GetArray( "hevec2" )->GetTuple(whichParticle)[2];

  particleToTPSMetric->SetParticle( position );

  (*domainParams)[0] = position[0]; 
  (*domainParams)[1] = position[1]; 

  newtonOptimizer->SetInitialParameters( domainParams );
  newtonOptimizer->Update();
  newtonOptimizer->GetOptimalParameters( optimalParams );

  *distance = vcl_sqrt( newtonOptimizer->GetOptimalValue() );
  
  tps->GetSurfaceNormal( (*optimalParams)[0], (*optimalParams)[1], normal );

  *angle = GetAngleBetweenVectors( normal, orientation, true );
}


//
// 'tpsVec' has either one (left) or two (right) elements. The
// convention is that if the 'tpsVec' contains surfaces for the right
// lung, the first element of the vector corresponds to the oblique,
// and the second element corresponds to the horizontal.
//
void TallyParticleInfo( vtkPolyData* particles, std::vector< cipThinPlateSplineSurface* > tpsVec, 
			std::map< unsigned int, PARTICLEINFO >* particleToInfoMap )
{
  if ( tpsVec.size() == 1 )
    {
    //
    // Dealing with the left lung
    //
    for ( unsigned int i=0; i<particles->GetNumberOfPoints(); i++ )
      {
      PARTICLEINFO pInfo;

      double distance, angle;
      GetParticleDistanceAndAngle( particles, i, tpsVec[0], &distance, &angle );
      
      pInfo.distance.push_back( distance );
      pInfo.angle.push_back( angle );

      (*particleToInfoMap)[i] = pInfo;
      }
    }
  else
    {
    //
    // Dealing with the right lung
    //
    double roSurfaceHeight, rhSurfaceHeight;
    for ( unsigned int i=0; i<particles->GetNumberOfPoints(); i++ )
      {
      PARTICLEINFO pInfo;

      roSurfaceHeight = tpsVec[0]->GetSurfaceHeight( particles->GetPoint(i)[0], particles->GetPoint(i)[1] );
      rhSurfaceHeight = tpsVec[1]->GetSurfaceHeight( particles->GetPoint(i)[0], particles->GetPoint(i)[1] ); 
     
      if ( roSurfaceHeight > rhSurfaceHeight )
        {
        double distance, angle;
        GetParticleDistanceAndAngle( particles, i, tpsVec[0], &distance, &angle );
      
        pInfo.distance.push_back( distance );
        pInfo.angle.push_back( angle );
        
        (*particleToInfoMap)[i] = pInfo;
        }
      else
        {
        double roDistance, roAngle;
        GetParticleDistanceAndAngle( particles, i, tpsVec[0], &roDistance, &roAngle );

        pInfo.distance.push_back( roDistance );
        pInfo.angle.push_back( roAngle );

        double rhDistance, rhAngle;
        GetParticleDistanceAndAngle( particles, i, tpsVec[1], &rhDistance, &rhAngle );

        pInfo.distance.push_back( rhDistance );
        pInfo.angle.push_back( rhAngle );
          
        (*particleToInfoMap)[i] = pInfo;
        }
      }
    }
}

  
void ClassifyParticles( std::map< unsigned int, PARTICLEINFO >* particleToInfoMap, std::vector< cipThinPlateSplineSurface* > tpsVec, 
                        double distanceWeight, double angleWeight, double threshold )
{
  std::map< unsigned int, PARTICLEINFO >::iterator it = (*particleToInfoMap).begin();

  while ( it != (*particleToInfoMap).end() )
    {
    std::vector< double > projection;

    for ( unsigned int i=0; i<(*it).second.distance.size(); i++)
      {
      projection.push_back( distanceWeight*(*it).second.distance[i] + angleWeight*(*it).second.angle[i] ); 
      }

    if ( projection.size() == 1 )
      {
      if ( projection[0] > threshold )
        {
	  (*it).second.cipType = static_cast< unsigned char >( cip::OBLIQUEFISSURE );
        }
      else
        {
	  (*it).second.cipType = static_cast< unsigned char >( cip::UNDEFINEDTYPE );
        }
      }
    else
      {
      //
      // If here, we're necessarily talking about the right lung
      //
      if ( projection[0] >= projection[1] )
        {
        if ( projection[0] > threshold )
          {
	    (*it).second.cipType = static_cast< unsigned char >( cip::OBLIQUEFISSURE );
          }
        else
          {
	    (*it).second.cipType = static_cast< unsigned char >( cip::UNDEFINEDTYPE );
          }
        }
      else
        {
        if ( projection[1] > threshold )
          {
	    (*it).second.cipType = static_cast< unsigned char >( cip::HORIZONTALFISSURE );
          }
        else
          {
	    (*it).second.cipType = static_cast< unsigned char >( cip::UNDEFINEDTYPE );
          }
        }
      }

    ++it;
    }
}


void WriteParticlesToFile( vtkSmartPointer< vtkPolyData > particles, std::map< unsigned int, PARTICLEINFO > particleToInfoMap, 
                          std::string fileName, unsigned char cipType )
{
  unsigned int numberOfFieldDataArrays = particles->GetFieldData()->GetNumberOfArrays();

  vtkPoints* outputPoints  = vtkPoints::New();
   
  std::vector< vtkFloatArray* > arrayVec;

  for ( unsigned int i=0; i<numberOfFieldDataArrays; i++ )
    {
    vtkFloatArray* array = vtkFloatArray::New();
      array->SetNumberOfComponents( particles->GetFieldData()->GetArray(i)->GetNumberOfComponents() );
      array->SetName( particles->GetFieldData()->GetArray(i)->GetName() );

    arrayVec.push_back( array );
    }

  unsigned int inc = 0;
  for ( unsigned int i=0; i<particles->GetNumberOfPoints(); i++ )
    {
    if ( particleToInfoMap[i].cipType == cipType )
      {
      outputPoints->InsertNextPoint( particles->GetPoint(i) );

      for ( unsigned int k=0; k<numberOfFieldDataArrays; k++ )
        {
        arrayVec[k]->InsertTuple( inc, particles->GetFieldData()->GetArray(k)->GetTuple(i) );
        }        
      inc++;
      }
    }

  vtkSmartPointer< vtkPolyData > outputParticles = vtkSmartPointer< vtkPolyData >::New();

  outputParticles->SetPoints( outputPoints );
  for ( unsigned int j=0; j<numberOfFieldDataArrays; j++ )
    {
    outputParticles->GetFieldData()->AddArray( arrayVec[j] );
    }

  vtkSmartPointer< vtkPolyDataWriter > writer = vtkSmartPointer< vtkPolyDataWriter >::New();
    writer->SetInput( outputParticles );
    writer->SetFileName( fileName.c_str() );
    writer->Write();
}

#endif