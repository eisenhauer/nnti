# @HEADER
# ************************************************************************
#
#            TriBITS: Tribial Build, Test, and Integrate System
#                 Copyright (2011) Sandia Corporation
#
#
# Copyright (2011) Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this
# work by or on behalf of the U.S. Government.  Export of this program
# may require a license from the United States Government.
#
# 1. Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the Corporation nor the names of the
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# NOTICE:  The United States Government is granted for itself and others
# acting on its behalf a paid-up, nonexclusive, irrevocable worldwide
# license in this data to reproduce, prepare derivative works, and
# perform publicly and display publicly.  Beginning five (5) years from
# July 25, 2001, the United States Government is granted for itself and
# others acting on its behalf a paid-up, nonexclusive, irrevocable
# worldwide license in this data to reproduce, prepare derivative works,
# distribute copies to the public, perform publicly and display
# publicly, and to permit others to do so.
#
# NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT
# OF ENERGY, NOR SANDIA CORPORATION, NOR ANY OF THEIR EMPLOYEES, MAKES
# ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR
# RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY
# INFORMATION, APPARATUS, PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS
# THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.
#
# ************************************************************************
# @HEADER


INCLUDE(TribitsHostType)


#
# This module defines the datastructure for the list of packages
# ${PROJECT_NAME}_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS which has the form:
#
#   Package0_name  Package0_dir Package0_classification
#   Package1_name  Package1_dir Package1_classification
#   ...
#
# There are 3 fields per row all stored in a flat array.
# 


SET(PLH_NUM_FIELDS_PER_PACKAGE 3)
SET(PLH_NUM_PACKAGE_DIR_OFFSET 1)
SET(PLH_NUM_PACKAGE_CLASSIFICATION_OFFSET 2)


MACRO(TRIBITS_SET_PACKAGE_TO_EX  PACKAGE_NAME)
  LIST(FIND ${PROJECT_NAME}_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS
    ${PACKAGE_NAME} PACKAGE_NAME_IDX)
  IF (PACKAGE_NAME_IDX EQUAL -1)
    MESSAGE(
      "\n***"
      "\n*** WARNING: Package ${PACKAGE_NAME} not found in list of packages!"
      "\n***\n"
      )
  ELSE()
    MATH(EXPR PACKAGE_CLASSIFICATION_IDX "${PACKAGE_NAME_IDX}+2")
    LIST(REMOVE_AT ${PROJECT_NAME}_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS
      ${PACKAGE_CLASSIFICATION_IDX} )
    LIST(INSERT ${PROJECT_NAME}_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS
      ${PACKAGE_CLASSIFICATION_IDX} EX )
  ENDIF()

ENDMACRO()


MACRO( PACKAGE_DISABLE_ON_PLATFORMS  PACKAGE_NAME )
  #MESSAGE("PACKAGE_DISABLE_ON_PLATFORMS: ${PACKAGE_NAME}")
  #PRINT_VAR(${PROJECT_NAME}_HOSTTYPE)
  FOREACH(HOSTTYPE ${ARGN})
    #PRINT_VAR(HOSTTYPE)
    IF (${PROJECT_NAME}_HOSTTYPE STREQUAL ${HOSTTYPE})
      #MESSAGE("${${PROJECT_NAME}_HOSTTYPE} == ${HOSTTYPE}")
      TRIBITS_SET_PACKAGE_TO_EX(${PACKAGE_NAME})
      #PRINT_VAR(${PROJECT_NAME}_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS)
      IF (${PROJECT_NAME}_ENABLE_${PACKAGE_NAME})
        MESSAGE(
          "\n***"
          "\n*** WARNING: User has set ${PROJECT_NAME}_ENABLE_${PACKAGE_NAME}=ON but the"
          "\n*** package ${PACKAGE_NAME} is not supported on this platform type '${HOSTTYPE}'!"
          "\n***\n"
          )
      ENDIF()
    ENDIF()
  ENDFOREACH()
ENDMACRO()
