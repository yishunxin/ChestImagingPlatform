################################################################################
#
#  Program: 3D Slicer
#
#  Copyright (c) Kitware Inc.
#
#  See COPYRIGHT.txt
#  or http://www.slicer.org/copyright/copyright.txt for details.
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
#  This file was originally developed by Jean-Christophe Fillion-Robin and Zach Mullen, Kitware Inc.
#  and was partially funded by NIH grant 3P41RR013218-12S1
#
################################################################################

if(NOT DEFINED MIDAS_API_DISPLAY_URL)
  set(MIDAS_API_DISPLAY_URL 0)
endif()

include(CMakeParseArguments)

#
# midas_api_login
#
#   MIDAS_REST_URL The url of the MIDAS server
#   MIDAS_USER_EMAIL The email to use to authenticate to the server
#   MIDAS_USER_APIKEY The default api key to use to authenticate to the server
function(midas_api_login)
  set(options)
  set(oneValueArgs MIDAS_REST_URL MIDAS_USER_EMAIL MIDAS_USER_APIKEY RESULT_VARNAME)
  set(multiValueArgs)
  cmake_parse_arguments(MY "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  # Sanity check
  set(expected_nonempty_vars MIDAS_REST_URL MIDAS_USER_EMAIL MIDAS_USER_APIKEY RESULT_VARNAME)
  foreach(var ${expected_nonempty_vars})
    if("${MY_${var}}" STREQUAL "")
      message(FATAL_ERROR "error: ${var} CMake variable is empty !")
    endif()
  endforeach()

  midas_api_escape_for_url(email ${MY_MIDAS_USER_EMAIL})
  midas_api_escape_for_url(apikey ${MY_MIDAS_USER_APIKEY})
  
  set(params "&appname=Default")
  set(params "${params}&email=${email}")
  set(params "${params}&apikey=${apikey}")
  set(url "${MY_MIDAS_REST_URL}/system/login?appname=Default${params}")

  if("${MIDAS_API_DISPLAY_URL}")
    message(STATUS "URL: ${url}")
  endif()

  set(login_token_filepath ${CMAKE_CURRENT_BINARY_DIR}/MIDAStoken.txt)
  file(DOWNLOAD ${url} ${login_token_filepath} INACTIVITY_TIMEOUT 120)
  file(READ ${login_token_filepath} resp)
  file(REMOVE ${login_token_filepath})
  string(REGEX REPLACE ".*token\":\"(.*)\".*" "\\1" token "${resp}")

  string(LENGTH "${token}" tokenlength)
  if(NOT tokenlength EQUAL 40)
    set(token "")
    message(WARNING "Failed to login to MIDAS server\n"
                    "  url: ${MY_MIDAS_REST_URL}\n"
                    "  email: ${email}\n"
                    "  apikey: ${apikey}\n"
                    "  response: ${resp}")
  endif()
  set(${MY_RESULT_VARNAME} "${token}" PARENT_SCOPE)
endfunction()

function(midas_api_escape_for_url var str)
  # Source: http://www.blooberry.com/indexdot/html/topics/urlencoding.htm
  string(REPLACE "%" "%25" _tmp "${str}") # Percent character
  # Reserved characters
  string(REPLACE "$" "%24" _tmp "${_tmp}") # Slash
  string(REPLACE "&" "%26" _tmp "${_tmp}") # Ampersand
  string(REPLACE "+" "%2B" _tmp "${_tmp}") # Plus
  string(REPLACE "," "%2C" _tmp "${_tmp}") # Comma
  string(REPLACE "/" "%2F" _tmp "${_tmp}") # Slash
  string(REPLACE ":" "%3A" _tmp "${_tmp}") # Colon
  string(REPLACE ";" "%3B" _tmp "${_tmp}") # Semi-colon
  string(REPLACE "=" "%3D" _tmp "${_tmp}") # Equals
  string(REPLACE "?" "%3F" _tmp "${_tmp}") # Question mark
  string(REPLACE "@" "%40" _tmp "${_tmp}") # 'At' symbol
  # Unsafe characters
  string(REPLACE " " "%20" _tmp "${_tmp}") # Space
  string(REPLACE "\"" "%22" _tmp "${_tmp}") # Quotation marks
  string(REPLACE "<" "%3C" _tmp "${_tmp}") # 'Less Than' symbol
  string(REPLACE ">" "%3E" _tmp "${_tmp}") # 'Greater Than' symbol
  string(REPLACE "#" "%23" _tmp "${_tmp}") # 'Pound' character
  string(REPLACE "{" "%7B" _tmp "${_tmp}") # Left Curly Brace
  string(REPLACE "}" "%7D" _tmp "${_tmp}") # Right Curly Brace
  string(REPLACE "|" "%7C" _tmp "${_tmp}") # Vertical Bar/Pipe
  string(REPLACE "\\" "%5C" _tmp "${_tmp}") # Backslash
  string(REPLACE "^" "%5E" _tmp "${_tmp}") # Caret
  string(REPLACE "~" "%7E" _tmp "${_tmp}") # Tilde
  string(REPLACE "[" "%5B" _tmp "${_tmp}") # Left Square Bracket
  string(REPLACE "]" "%5D" _tmp "${_tmp}") # Right Square Bracket
  string(REPLACE "`" "%60" _tmp "${_tmp}") # Grave Accent
  set(${var} ${_tmp} PARENT_SCOPE)
endfunction()
