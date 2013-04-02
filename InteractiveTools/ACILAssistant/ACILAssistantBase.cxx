#include "ACILAssistantBase.h"
#include "cipConventions.h"
#include "cipLabelMapToLungLobeLabelMapImageFilter.h"
#include <fstream>
#include <time.h>
#include "vtkPoints.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkStructuredGrid.h"
#include "vtkArrowSource.h"
#include "vtkGlyph3D.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkFlRenderWindowInteractor.h"
#include <FL/Fl_Window.H>
#include "vtkImageData.h"
//#include "cipChestRegionChestTypeLocations.h"
#include "cipChestRegionChestTypeLocationsIO.h"


ACILAssistantBase::ACILAssistantBase()
{
  this->LabelMap       = LabelMapType::New();
  this->GrayscaleImage = GrayscaleImageType::New();
  this->HeadFirst      = true;
  this->FeetFirst      = false;
  this->Supine         = true;
  this->Prone          = false;
}

ACILAssistantBase::~ACILAssistantBase(){}


void ACILAssistantBase::SetScanIsHeadFirst()
{
  this->HeadFirst = true;
  this->FeetFirst = false;
}


void ACILAssistantBase::SetScanIsFeetFirst()
{
  this->HeadFirst = false;
  this->FeetFirst = true;
}


void ACILAssistantBase::SetScanIsProne()
{
  this->Prone  = true;
  this->Supine = false;
}


void ACILAssistantBase::SetScanIsSupine()
{
  this->Prone  = false;
  this->Supine = true;
}


void ACILAssistantBase::SetLabelMapImage( LabelMapType::Pointer image )
{
  LabelMapType::SizeType     size    = image->GetBufferedRegion().GetSize();
  LabelMapType::SpacingType  spacing = image->GetSpacing();
  LabelMapType::PointType    origin  = image->GetOrigin();

  this->InitializeLabelMapImage( size, spacing, origin );

  LabelMapIteratorType iIt( image, image->GetBufferedRegion() );
  LabelMapIteratorType mIt( this->LabelMap, this->LabelMap->GetBufferedRegion() );

  iIt.GoToBegin();
  mIt.GoToBegin();
  while ( !mIt.IsAtEnd() )
    {
    mIt.Set( iIt.Get() );

    ++iIt;
    ++mIt;
    }
}


void ACILAssistantBase::SetGrayscaleImage( GrayscaleImageType::Pointer image )
{
  GrayscaleImageType::SizeType     size    = image->GetBufferedRegion().GetSize();
  GrayscaleImageType::SpacingType  spacing = image->GetSpacing();
  GrayscaleImageType::PointType    origin  = image->GetOrigin();

  this->GrayscaleImage->SetRegions( size );
  this->GrayscaleImage->Allocate();
  this->GrayscaleImage->FillBuffer( 0 );
  this->GrayscaleImage->SetSpacing( spacing );  
  this->GrayscaleImage->SetOrigin( origin );

  GrayscaleIteratorType iIt( image, image->GetBufferedRegion() );
  GrayscaleIteratorType mIt( this->GrayscaleImage, this->GrayscaleImage->GetBufferedRegion() );

  iIt.GoToBegin();
  mIt.GoToBegin();
  while ( !mIt.IsAtEnd() )
    {
    mIt.Set( iIt.Get() );

    ++iIt;
    ++mIt;
    }

  //
  // We want to set up a Nrrd pointer for probing the volume, should
  // the user want to do that. First export the ITK image to "VTK
  // Land" 
  //
  ExportType::Pointer refExporter = ExportType::New();
    refExporter->SetInput( this->GrayscaleImage );

  vtkImageImport* refImporter = vtkImageImport::New();

  ConnectPipelines( refExporter, refImporter );

  //
  // Now create the nrrd pointer
  //
//   vtkNRRDExport* nrrdExport = vtkNRRDExport::New();
//     nrrdExport->SetInput( refImporter->GetOutput() );

//  this->NrrdPointer = nrrdExport->GetNRRDPointer();
}


void ACILAssistantBase::InitializeLabelMapImage( LabelMapType::SizeType size, LabelMapType::SpacingType spacing, 
                                                 LabelMapType::PointType origin )
{
  this->LabelMap->SetRegions( size );
  this->LabelMap->Allocate();
  this->LabelMap->FillBuffer( 0 );
  this->LabelMap->SetSpacing( spacing );
  this->LabelMap->SetOrigin( origin );
}


void ACILAssistantBase::PaintLabelMapSlice( LabelMapType::IndexType index, unsigned char cipType, unsigned char cipRegion, unsigned int radius, 
                                            short lowerThreshold, short upperThreshold, unsigned int orientation )
{  
  ChestConventions conventions;

  LabelMapType::IndexType tempIndex;

  // 'M' and 'N' used here as generic indices (not know aprior whether
  // the indices will represent x, y, or z).
  int startM, endM, startN, endN;

  // Sagittal
  if ( orientation == 0 )
    {
    startM = static_cast< int >( index[1] ) - static_cast< int >( radius );
    endM   = static_cast< int >( index[1] ) + static_cast< int >( radius );
    startN = static_cast< int >( index[2] ) - static_cast< int >( radius );
    endN   = static_cast< int >( index[2] ) + static_cast< int >( radius );
    }

  // Coronal
  if ( orientation == 1 )
    {
    startM = static_cast< int >( index[0] ) - static_cast< int >( radius );
    endM   = static_cast< int >( index[0] ) + static_cast< int >( radius );
    startN = static_cast< int >( index[2] ) - static_cast< int >( radius );
    endN   = static_cast< int >( index[2] ) + static_cast< int >( radius );
    }

  // Axial
  if ( orientation == 2 )
    {
    startM = static_cast< int >( index[0] ) - static_cast< int >( radius );
    endM   = static_cast< int >( index[0] ) + static_cast< int >( radius );
    startN = static_cast< int >( index[1] ) - static_cast< int >( radius );
    endN   = static_cast< int >( index[1] ) + static_cast< int >( radius );
    }

  for ( int x = startM; x <= endM; x++ )
    {
    for ( int y = startN; y <= endN; y++ )
      {
      // Sagittal
      if ( orientation == 0 )
        {
        tempIndex[0] = index[0];
        tempIndex[1] = x;
        tempIndex[2] = y;
        }

      // Coronal
      if ( orientation == 1 )
        {
        tempIndex[0] = x;
        tempIndex[1] = index[1];
        tempIndex[2] = y;
        }

      // Axial
      if ( orientation == 2 )
        {
        tempIndex[0] = x;
        tempIndex[1] = y;
        tempIndex[2] = index[2];
        }

      if ( this->LabelMap->GetBufferedRegion().IsInside( tempIndex ) )
        {
        if ( this->GrayscaleImage->GetPixel( tempIndex ) >= lowerThreshold && this->GrayscaleImage->GetPixel( tempIndex ) <= upperThreshold )
          {
          unsigned short newLabel = conventions.GetValueFromChestRegionAndType( cipRegion, cipType );        

          this->LabelMap->SetPixel( tempIndex, newLabel );
          this->PaintedIndices.push_back( tempIndex );
          }
        }
      }
    }
}


void ACILAssistantBase::EraseLabelMapSlice( LabelMapType::IndexType index, unsigned char cipRegion, unsigned char cipType, unsigned int radius, 
                                            short lowerThreshold, short upperThreshold, bool eraseSelected, unsigned int orientation )
{
  ChestConventions conventions;

  LabelMapType::IndexType tempIndex;

  // 'M' and 'N' used here as generic indices (not know aprior whether
  // the indices will represent x, y, or z).
  int startM, endM, startN, endN;

  // Sagittal
  if ( orientation == 0 )
    {
    startM = static_cast< int >( index[1] ) - static_cast< int >( radius );
    endM   = static_cast< int >( index[1] ) + static_cast< int >( radius );
    startN = static_cast< int >( index[2] ) - static_cast< int >( radius );
    endN   = static_cast< int >( index[2] ) + static_cast< int >( radius );
    }

  // Coronal
  if ( orientation == 1 )
    {
    startM = static_cast< int >( index[0] ) - static_cast< int >( radius );
    endM   = static_cast< int >( index[0] ) + static_cast< int >( radius );
    startN = static_cast< int >( index[2] ) - static_cast< int >( radius );
    endN   = static_cast< int >( index[2] ) + static_cast< int >( radius );
    }

  // Axial
  if ( orientation == 2 )
    {
    startM = static_cast< int >( index[0] ) - static_cast< int >( radius );
    endM   = static_cast< int >( index[0] ) + static_cast< int >( radius );
    startN = static_cast< int >( index[1] ) - static_cast< int >( radius );
    endN   = static_cast< int >( index[1] ) + static_cast< int >( radius );
    }

  for ( int x = startM; x <= endM; x++ )
    {
    for ( int y = startN; y <= endN; y++ )
      {
      // Sagittal
      if ( orientation == 0 )
        {
        tempIndex[0] = index[0];
        tempIndex[1] = x;
        tempIndex[2] = y;
        }

      // Coronal
      if ( orientation == 1 )
        {
        tempIndex[0] = x;
        tempIndex[1] = index[1];
        tempIndex[2] = y;
        }

      // Axial
      if ( orientation == 2 )
        {
        tempIndex[0] = x;
        tempIndex[1] = y;
        tempIndex[2] = index[2];
        }

      if ( this->LabelMap->GetBufferedRegion().IsInside( tempIndex ) )
        {
        if ( this->GrayscaleImage->GetPixel( tempIndex ) >= lowerThreshold && this->GrayscaleImage->GetPixel( tempIndex ) <= upperThreshold )
          {
          unsigned short currentLabel = this->LabelMap->GetPixel( tempIndex );

          if ( currentLabel != 0 )
            {
            if ( eraseSelected )
              {
              unsigned char currentRegion = conventions.GetChestRegionFromValue( currentLabel );
              unsigned char currentType   = conventions.GetChestTypeFromValue( currentLabel );
            
              unsigned char newRegion = currentRegion;
              unsigned char newType   = currentType;

              if ( currentRegion == cipRegion )
                {
                newRegion = 0;
                }
              if ( currentType == cipType )
                {
                newType = 0;
                }

              this->LabelMap->SetPixel( tempIndex, conventions.GetValueFromChestRegionAndType( newRegion, newType ) );
              }
            else
              {
              this->LabelMap->SetPixel( tempIndex, 0 );
              }
            }
          }
        }
      }
    }       
}


bool ACILAssistantBase::LabelLungThirds()
{
  //
  // First get the number of voxels in the label map
  //
  unsigned int totalVoxelCount = 0;

  LabelMapIteratorType it( this->LabelMap, this->LabelMap->GetBufferedRegion() );
  
  it.GoToBegin();
  while ( !it.IsAtEnd() )
    {
    if ( it.Get() != 0 )
      {
      totalVoxelCount++;
      }
    
    ++it;
    }

  //
  // Now label by thirds
  //
  bool foundLeftLung  = false;
  bool foundRightLung = false;
  unsigned int voxelCount = 0;

  it.GoToBegin();
  while ( !it.IsAtEnd() )
    {
    if ( it.Get() != 0 )
      {
      voxelCount++;

      if ( static_cast< double >( voxelCount ) < static_cast< double >( totalVoxelCount )/3.0 )
        {
        if ( it.Get() == static_cast< unsigned short >( LEFTLUNG ) )
          {
          foundLeftLung = true;
          it.Set( static_cast< unsigned short >( LEFTLOWERTHIRD ) );
          }
        else
          {
          foundRightLung = true;
          it.Set( static_cast< unsigned short >( RIGHTLOWERTHIRD ) );
          }        
        }
      else if ( static_cast< double >( voxelCount ) < 2.0*static_cast< double >( totalVoxelCount )/3.0 )
        {
        if ( it.Get() == static_cast< unsigned short >( LEFTLUNG ) )
          {
          foundLeftLung = true;
          it.Set( static_cast< unsigned short >( LEFTMIDDLETHIRD ) );
          }
        else
          {
          foundRightLung = true;
          it.Set( static_cast< unsigned short >( RIGHTMIDDLETHIRD ) );
          }        
        }
      else
        {
        if ( it.Get() == static_cast< unsigned short >( LEFTLUNG ) )
          {
          foundLeftLung = true;
          it.Set( static_cast< unsigned short >( LEFTUPPERTHIRD ) );
          }
        else
          {
          foundRightLung = true;
          it.Set( static_cast< unsigned short >( RIGHTUPPERTHIRD ) );
          }        
        }
      }

    ++it;
    } 

  if ( foundLeftLung && foundRightLung )
    {
    return true;
    }
  
  return false;
}


bool ACILAssistantBase::LabelLeftLungRightLung()
{
  ChestConventions conventions;

  //
  // First set all types to 'UNDEFINEDTYPE'. This is necessary in the
  // case that, e.g., airways are present, connecting the left and
  // right lungs
  //
  LabelMapIteratorType mIt( this->LabelMap, this->LabelMap->GetBufferedRegion() );

  unsigned char cipRegion;

  mIt.GoToBegin();
  while ( !mIt.IsAtEnd() )
    {
    if ( mIt.Get() != 0 )
      {
      cipRegion = conventions.GetChestRegionFromValue( mIt.Get() );
      mIt.Set( conventions.GetValueFromChestRegionAndType( cipRegion, static_cast< unsigned char >( cip::UNDEFINEDTYPE ) ) );
      }

    ++mIt;
    }

  ConnectedComponentType::Pointer connectedComponent = ConnectedComponentType::New();
    connectedComponent->SetInput( this->LabelMap );
    connectedComponent->Update();

  RelabelComponentType::Pointer relabeler = RelabelComponentType::New();
    relabeler->SetInput( connectedComponent->GetOutput() );
  try
    {
    relabeler->Update();
    }
  catch ( itk::ExceptionObject &excp )
    {
    std::cerr << "Exception caught relabeling:";
    std::cerr << excp << std::endl;
    }

  if ( relabeler->GetNumberOfObjects() < 2 )
    {
    return false;
    }

  unsigned int total = 0;
  for ( unsigned int i=0; i<relabeler->GetNumberOfObjects(); i++ )
    {
    total += relabeler->GetSizeOfObjectsInPixels()[i];
    }

  //
  // If the second largest object doesn't comprise at least 30%
  // (arbitrary) of the foreground region, assume the lungs are
  // connected 
  //
  if ( static_cast< double >( relabeler->GetSizeOfObjectsInPixels()[1] )/static_cast< double >( total ) < 0.3 )
    {
    return false;
    }

  //
  // If we're here, we assume that the left and right have been
  // separated, so label them. First, we need to get the relabel
  // component corresponding to the left and the right. We assume that
  // the relabel component value = 1 corresponds to one of the two
  // lungs and a value of 2 corresponds to the other. Find the
  // left-most and right-most component value. Assuming the scan is
  // supine, head-first, the component value corresponding to the
  // smallest x-index will be the left lung and the other major
  // component will be the right lung.
  //
  unsigned int minX = relabeler->GetOutput()->GetBufferedRegion().GetSize()[0];
  unsigned int maxX = 0;

  unsigned int smallIndexComponentLabel, largeIndexComponentLabel;

  LabelMapIteratorType rIt( relabeler->GetOutput(), relabeler->GetOutput()->GetBufferedRegion() );

  rIt.GoToBegin();
  while ( !rIt.IsAtEnd() )
    {
    if ( rIt.Get() == 1 || rIt.Get() == 2 )
      {
      if ( rIt.GetIndex()[0] < minX )
        {
        smallIndexComponentLabel = rIt.Get();
        minX = rIt.GetIndex()[0];
        }
      if ( rIt.GetIndex()[0] > maxX )
        {
        largeIndexComponentLabel = rIt.Get();
        maxX = rIt.GetIndex()[0];
        }
      }

    ++rIt;
    }

  unsigned int leftLungComponentLabel, rightLungComponentLabel;
  if ( (this->HeadFirst && this->Supine) || (this->FeetFirst && this->Prone) )
    {
    leftLungComponentLabel  = largeIndexComponentLabel;
    rightLungComponentLabel = smallIndexComponentLabel;
    }
  else
    {
    leftLungComponentLabel  = smallIndexComponentLabel;
    rightLungComponentLabel = largeIndexComponentLabel;
    }

  mIt.GoToBegin();
  rIt.GoToBegin();
  while ( !mIt.IsAtEnd() )
    {
    if ( rIt.Get() == leftLungComponentLabel )
      {
      mIt.Set( static_cast< unsigned short >( LEFTLUNG ) );
      }
    if ( rIt.Get() == rightLungComponentLabel )
      {
      mIt.Set( static_cast< unsigned short >( RIGHTLUNG ) );
      }

    ++rIt;
    ++mIt;
    }

  return true;
}


bool ACILAssistantBase::CloseLeftLungRightLung()
{
  this->CloseLabelMap( this->LabelMap, static_cast< unsigned short >( LEFTLUNG ) );
  this->CloseLabelMap( this->LabelMap, static_cast< unsigned short >( RIGHTLUNG ) );

  return true;
}


void ACILAssistantBase::CloseLabelMap( LabelMapType::Pointer labelMap, unsigned short closeLabel )
{
  LabelMapType::SpacingType spacing = labelMap->GetSpacing();

  double closingRadius = 5.0;

  unsigned long closingNeighborhood[3];
    closingNeighborhood[0] = static_cast< unsigned long >( vnl_math_rnd( closingRadius/spacing[0] ) );
    closingNeighborhood[1] = static_cast< unsigned long >( vnl_math_rnd( closingRadius/spacing[1] ) );
    closingNeighborhood[2] = static_cast< unsigned long >( vnl_math_rnd( closingRadius/spacing[2] ) );

  closingNeighborhood[0] = closingNeighborhood[0]>0 ? closingNeighborhood[0] : 1;
  closingNeighborhood[1] = closingNeighborhood[1]>0 ? closingNeighborhood[1] : 1;
  closingNeighborhood[2] = closingNeighborhood[2]>0 ? closingNeighborhood[2] : 1;

  //
  // Perform morphological closing on the mask by dilating and then
  // eroding.  We assume that at this point in the pipeline, the
  // output image only has WHOLELUNG as a foreground value.  (The
  // airways and vessels should be stored in the index vec member
  // variables). 
  //
  ElementType structuringElement;
    structuringElement.SetRadius( closingNeighborhood );
    structuringElement.CreateStructuringElement();

  std::cout << "---Dilating..." << std::endl;
  DilateType::Pointer dilater = DilateType::New();
    dilater->SetInput( labelMap );
    dilater->SetKernel( structuringElement );
    dilater->SetDilateValue( closeLabel );
  try
    {
    dilater->Update();
    }
  catch ( itk::ExceptionObject &excp )
    {
    std::cerr << "Exception caught dilating:";
    std::cerr << excp << std::endl;
    }

  //
  // Occasionally, dilation will extend the mask to the end slices. If
  // this occurs, the erosion step below won't be able to hit these
  // regions. To deal with this, extract the end slices from the
  // dilater and current output image.  Then set the dilater end
  // slices to be zero (provided that the output image is also zero at
  // those locations).
  //
  std::cout << "---Zeroing end slices..." << std::endl;
  LabelMapType::IndexType index;
  LabelMapType::SizeType  size = labelMap->GetBufferedRegion().GetSize();

  for ( unsigned int x=0; x<size[0]; x++ )
    {
    index[0] = x;

    for ( unsigned int y=0; y<size[1]; y++ )
      {
      index[1] = y;
      
      index[2] = 0;
      if ( labelMap->GetPixel( index ) == 0 )
        {
        dilater->GetOutput()->SetPixel( index, 0 );
        }

      index[2] = size[2]-1;
      if ( labelMap->GetPixel( index ) == 0 )
        {
        dilater->GetOutput()->SetPixel( index, 0 );
        }
      }
    }
  
  //
  // Now erode
  //
  std::cout << "---Eroding..." << std::endl;
  ErodeType::Pointer eroder = ErodeType::New();
    eroder->SetInput( dilater->GetOutput() );
    eroder->SetKernel( structuringElement );
    eroder->SetErodeValue( closeLabel );
  try
    {
    eroder->Update();
    }
  catch ( itk::ExceptionObject &excp )
    {
    std::cerr << "Exception caught eroding:";
    std::cerr << excp << std::endl;
    }


  std::cout << "---Copying..." << std::endl;
  LabelMapIteratorType eIt( eroder->GetOutput(), eroder->GetOutput()->GetBufferedRegion() );
  LabelMapIteratorType mIt( labelMap, labelMap->GetBufferedRegion() );

  eIt.GoToBegin();
  mIt.GoToBegin();
  while ( !mIt.IsAtEnd() )
    {
    if ( eIt.Get() != 0 )
      {
      mIt.Set( eIt.Get() );
      }

    ++eIt;
    ++mIt;
    }
}


bool ACILAssistantBase::SegmentLungLobes()
{
  return false;

//   ChestConventions conventions;

//   //
//   // First we'll populate the fissure index vecs. We'll also
//   // perform a check to see if there are multiple range values (z
//   // indices) mapped to the same domain values (x and y indices). This
//   // would violate the function assumption and muck up the TPS
//   // computations. If we find that there are multiple range values,
//   // we simply won't include any beyond the first (we'll arbitrarily
//   // keep the first range value for a given location in the domain).
//   //
//   std::vector< LabelMapType::IndexType > leftObliqueIndicesVec;
//   std::vector< LabelMapType::IndexType > rightObliqueIndicesVec;
//   std::vector< LabelMapType::IndexType > rightHorizontalIndicesVec;

//   LabelMapType::IndexType index;

//   for ( unsigned int i=0; i<(this->PaintedTypeIndices[OBLIQUEFISSURE]).size(); i++ )
//     {
//     index = (this->PaintedTypeIndices[OBLIQUEFISSURE])[i];

//     unsigned short labelValue = this->LabelMap->GetPixel( index );

//     unsigned char currentRegion = conventions.GetChestRegionFromValue( labelValue );

//     if ( currentRegion == static_cast< unsigned char >( LEFTLUNG ) || currentRegion == static_cast< unsigned char >( LEFTSUPERIORLOBE ) || 
//          currentRegion == static_cast< unsigned char >( LEFTINFERIORLOBE ) || currentRegion == static_cast< unsigned char >( LEFTUPPERTHIRD ) ||
//          currentRegion == static_cast< unsigned char >( LEFTMIDDLETHIRD ) || currentRegion == static_cast< unsigned char >( LEFTLOWERTHIRD ) )
//       {
//       bool domainLocationAlreadyExists = false;

//       for ( unsigned int i=0; i<leftObliqueIndicesVec.size(); i++ )
//         {
//         if ( index[0] == (leftObliqueIndicesVec[i])[0] && index[1] == (leftObliqueIndicesVec[i])[1] )
//           {
//           domainLocationAlreadyExists = true;

//           break;
//           }
//         }

//       if ( !domainLocationAlreadyExists )
//         {
//         leftObliqueIndicesVec.push_back( index );
//         }
//       }
//     if ( currentRegion == static_cast< unsigned char >( RIGHTLUNG ) || currentRegion == static_cast< unsigned char >( RIGHTSUPERIORLOBE ) ||
//          currentRegion == static_cast< unsigned char >( RIGHTMIDDLELOBE ) || currentRegion == static_cast< unsigned char >( RIGHTINFERIORLOBE ) ||
//          currentRegion == static_cast< unsigned char >( RIGHTUPPERTHIRD ) || currentRegion == static_cast< unsigned char >( RIGHTMIDDLETHIRD ) ||
//          currentRegion == static_cast< unsigned char >( RIGHTLOWERTHIRD ) )
//       {
//       bool domainLocationAlreadyExists = false;

//       for ( unsigned int i=0; i<rightObliqueIndicesVec.size(); i++ )
//         {
//         if ( index[0] == (rightObliqueIndicesVec[i])[0] && index[1] == (rightObliqueIndicesVec[i])[1] )
//           {
//           domainLocationAlreadyExists = true;

//           break;
//           }
//         }

//       if ( !domainLocationAlreadyExists )
//         {
//         rightObliqueIndicesVec.push_back( index );
//         }
//       }
//     }

//   for ( unsigned int i=0; i<(this->PaintedTypeIndices[HORIZONTALFISSURE]).size(); i++ )
//     {
//     index = (this->PaintedTypeIndices[HORIZONTALFISSURE])[i];

//     unsigned short labelValue = this->LabelMap->GetPixel( index );

//     unsigned char currentRegion = conventions.GetChestRegionFromValue( labelValue );

//     if ( currentRegion == static_cast< unsigned char >( RIGHTLUNG ) || currentRegion == static_cast< unsigned char >( RIGHTSUPERIORLOBE ) ||
//          currentRegion == static_cast< unsigned char >( RIGHTMIDDLELOBE ) || currentRegion == static_cast< unsigned char >( RIGHTINFERIORLOBE ) ||
//          currentRegion == static_cast< unsigned char >( RIGHTUPPERTHIRD ) || currentRegion == static_cast< unsigned char >( RIGHTMIDDLETHIRD ) ||
//          currentRegion == static_cast< unsigned char >( RIGHTLOWERTHIRD ) )
//       {
//       bool domainLocationAlreadyExists = false;

//       for ( unsigned int i=0; i<rightHorizontalIndicesVec.size(); i++ )
//         {
//         if ( index[0] == (rightHorizontalIndicesVec[i])[0] && index[1] == (rightHorizontalIndicesVec[i])[1] )
//           {
//           domainLocationAlreadyExists = true;

//           break;
//           }
//         }

//       if ( !domainLocationAlreadyExists )
//         {
//         rightHorizontalIndicesVec.push_back( index );
//         }
//       }
//     }

//   if ( rightHorizontalIndicesVec.size() == 0 && rightObliqueIndicesVec.size() == 0 && leftObliqueIndicesVec.size() == 0 )
//     {
//     return false;
//     }
//   if ( (rightHorizontalIndicesVec.size() == 0 && rightObliqueIndicesVec.size() != 0) ||
//        (rightHorizontalIndicesVec.size() != 0 && rightObliqueIndicesVec.size() == 0) )
//     {
//     return false;
//     }

//   //
//   // Perform a test to see if the current label map is
//   // "leftLungRightLung" or not
//   //
//   bool leftLungRightLung = true;

//   unsigned short leftLungLabel  = conventions.GetValueFromChestRegionAndType( LEFTLUNG, UNDEFINEDTYPE );
//   unsigned short rightLungLabel = conventions.GetValueFromChestRegionAndType( RIGHTLUNG, UNDEFINEDTYPE );

//   LabelMapIteratorType lIt( this->LabelMap, this->LabelMap->GetBufferedRegion() );

//   lIt.GoToBegin();
//   while ( !lIt.IsAtEnd() )
//     {
//     if ( lIt.Get() > 0 )
//       {
//       if ( lIt.Get() != leftLungLabel || lIt.Get() != rightLungLabel )
//         {
//         unsigned char currentRegion = conventions.GetChestRegionFromValue( lIt.Get() );

//         if ( currentRegion != static_cast< unsigned char >( LEFTLUNG ) && currentRegion != static_cast< unsigned char >( RIGHTLUNG ) )
//           {
//           leftLungRightLung = false;

//           break;
//           }
//         }
//       }

//     ++lIt;
//     }

//   typedef cipLabelMapToLungLobeLabelMapImageFilter LobeSegmentationType;

//   LobeSegmentationType::Pointer lobeSegmenter = LobeSegmentationType::New();
//     lobeSegmenter->SetInput( this->LabelMap );
//   if ( rightHorizontalIndicesVec.size() > 0 && rightObliqueIndicesVec.size() > 0 )
//     {
//     lobeSegmenter->SetRightHorizontalFissureIndices( rightHorizontalIndicesVec );
//     lobeSegmenter->SetRightObliqueFissureIndices( rightObliqueIndicesVec );
//     }
//   if ( leftObliqueIndicesVec.size() > 0 )
//     {
//     lobeSegmenter->SetLeftObliqueFissureIndices( leftObliqueIndicesVec );
//     }
//     lobeSegmenter->Update();

//   LabelMapIteratorType sIt( lobeSegmenter->GetOutput(), lobeSegmenter->GetOutput()->GetBufferedRegion() );

//   lIt.GoToBegin();
//   sIt.GoToBegin();
//   while ( !lIt.IsAtEnd() )
//     {
//     lIt.Set( sIt.Get() );

//     ++lIt;
//     ++sIt;
//     }

//   return true;
}


void ACILAssistantBase::Clear()
{
  this->PaintedIndices.clear();
}

short ACILAssistantBase::GetGrayscaleImageIntensity( GrayscaleImageType::IndexType index )
{
  return this->GrayscaleImage->GetPixel( index );
}


void ACILAssistantBase::WritePaintedRegionTypePoints( std::string fileName )
{
  ChestConventions conventions;

  unsigned char cipRegion;
  unsigned char cipType;

  cipChestRegionChestTypeLocationsIO regionTypeLocationsIO;
    regionTypeLocationsIO.SetFileName( fileName );
    regionTypeLocationsIO.Read();

  LabelMapType::IndexType index;
  LabelMapType::IndexType tmpIndex;
  LabelMapType::PointType point;

  for ( unsigned int i=0; i<this->PaintedIndices.size(); i++ )
    {    
    index = this->PaintedIndices[i];

    //
    // Test to see if the index has already been writting
    //
    bool alreadyWritten = false;
    for ( unsigned int j=0; j<i; j++ )
      {
      tmpIndex = this->PaintedIndices[j];

      if ( tmpIndex[0] == index[0] && tmpIndex[1] == index[1] && tmpIndex[2] == index[2] )
        {
        alreadyWritten = true;
        break;
        }
      }

    if ( !alreadyWritten )
      {
      unsigned short labelMapValue = this->LabelMap->GetPixel( index );
      
      this->LabelMap->TransformIndexToPhysicalPoint( index, point );

      cipRegion = conventions.GetChestRegionFromValue( labelMapValue );
      cipType   = conventions.GetChestTypeFromValue( labelMapValue );

      double *location = new double[3];
        location[0] = point[0];
        location[1] = point[1];
        location[2] = point[2];

      if ( cipRegion != static_cast< unsigned char >( cip::UNDEFINEDREGION ) || 
           cipType != static_cast< unsigned char >( cip::UNDEFINEDTYPE ) )
        {
        regionTypeLocationsIO.GetOutput()->SetChestRegionChestTypeLocation( cipRegion, cipType, location );
        }

      delete[] location;
      }
    }

  regionTypeLocationsIO.Write();
}


void ACILAssistantBase::ConnectPipelines( ExportType::Pointer exporter, vtkImageImport* importer )
{
  importer->SetUpdateInformationCallback(exporter->GetUpdateInformationCallback());
  importer->SetPipelineModifiedCallback(exporter->GetPipelineModifiedCallback());
  importer->SetWholeExtentCallback(exporter->GetWholeExtentCallback());
  importer->SetSpacingCallback(exporter->GetSpacingCallback());
  importer->SetOriginCallback(exporter->GetOriginCallback());
  importer->SetScalarTypeCallback(exporter->GetScalarTypeCallback());
  importer->SetNumberOfComponentsCallback(exporter->GetNumberOfComponentsCallback());
  importer->SetPropagateUpdateExtentCallback(exporter->GetPropagateUpdateExtentCallback());
  importer->SetUpdateDataCallback(exporter->GetUpdateDataCallback());
  importer->SetDataExtentCallback(exporter->GetDataExtentCallback());
  importer->SetBufferPointerCallback(exporter->GetBufferPointerCallback());
  importer->SetCallbackUserData(exporter->GetCallbackUserData());
}