# @HEADER
# ************************************************************************
#
#            TriBITS: Tribial Build, Integrate, and Test System
#                    Copyright 2013 Sandia Corporation
#
# Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
# the U.S. Government retains certain rights in this software.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
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
# ************************************************************************
# @HEADER


INCLUDE(${CMAKE_CURRENT_LIST_DIR}/TribitsAdjustPackageEnablesHelpers.cmake)


#####################################################################
#
# Unit tests for code in TribitsAdjustPackageEnables.cmake
#
#####################################################################


#
# A) Test basic package processing and reading dependencies 
#


FUNCTION(UNITTEST_READ_PACKAGES_LIST_WITH_EXTRA_REPO)

  MESSAGE("\n***")
  MESSAGE("*** Testing the reading of packages list with extra repo")
  MESSAGE("***\n")

  # Debugging
  #SET(TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS ON)
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${PROJECT_NAME} ".")
  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${EXTRA_REPO_NAME} ${EXTRA_REPO_DIR})

  UNITTEST_COMPARE_CONST( ${PROJECT_NAME}_PACKAGES
    "Teuchos;RTOp;Ex2Package1;Ex2Package2")
  UNITTEST_COMPARE_CONST( ${PROJECT_NAME}_PACKAGE_DIRS
    "packages/teuchos;packages/rtop;extraRepoTwoPackages/package1;extraRepoTwoPackages/package2")
  UNITTEST_COMPARE_CONST( ${PROJECT_NAME}_NUM_PACKAGES 4 )

ENDFUNCTION()


FUNCTION(UNITTEST_READ_TPLS_LISTS_WTIH_DUPLICATE_TPLS)

  MESSAGE("\n***")
  MESSAGE("*** Testing the reading of TPL lists with duplicate TPLs ")
  MESSAGE("***\n")

  # Debugging
  #SET(TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS ON)
  SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)
  
  SET(${EXTRA_REPO_NAME}_TPLS_FINDMODS_CLASSIFICATIONS
    EXTPL2       tpls/    EX
    Boost        tpls/    PS
  )

  TRIBITS_PROCESS_TPLS_LISTS(${PROJECT_NAME} ".")
  TRIBITS_PROCESS_TPLS_LISTS(${EXTRA_REPO_NAME} ${EXTRA_REPO_DIR})

  # The TPL is not added again
  UNITTEST_COMPARE_CONST( ${PROJECT_NAME}_TPLS "MPI;BLAS;LAPACK;Boost;EXTPL2")
  UNITTEST_COMPARE_CONST( ${PROJECT_NAME}_NUM_TPLS "5" )
  UNITTEST_COMPARE_CONST( ${PROJECT_NAME}_REVERSE_TPLS "EXTPL2;Boost;LAPACK;BLAS;MPI" )
  UNITTEST_COMPARE_CONST( MPI_FINDMOD "cmake/TPLs/FindTPLMPI.cmake" )
  UNITTEST_COMPARE_CONST( MPI_CLASSIFICATION "PS" )
  UNITTEST_COMPARE_CONST( BLAS_FINDMOD "cmake/TPLs/FindTPLBLAS.cmake" )
  UNITTEST_COMPARE_CONST( BLAS_CLASSIFICATION "PS" )

  # The find module is overridden in extra repo
  UNITTEST_COMPARE_CONST( Boost_FINDMOD "${EXTRA_REPO_NAME}/tpls/FindTPLBoost.cmake" )

  # The classification is not overridden in extra repo
  UNITTEST_COMPARE_CONST( Boost_CLASSIFICATION "SS" )

ENDFUNCTION()


FUNCTION(UNITTEST_STANDARD_PROJECT_DEFAULT_EMAIL_ADDRESS_BASE)

  MESSAGE("\n***")
  MESSAGE("*** Testing the case where the TriBITS project has a default email address base and uses standard package regression email list names")
  MESSAGE("***\n")

  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${PROJECT_NAME} ".")
  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${EXTRA_REPO_NAME} ${EXTRA_REPO_DIR})
  TRIBITS_READ_ALL_PACKAGE_DEPENDENCIES()

  UNITTEST_COMPARE_CONST(Teuchos_REGRESSION_EMAIL_LIST teuchos-regression@repo.site.gov)
  UNITTEST_COMPARE_CONST(RTOp_REGRESSION_EMAIL_LIST thyra-regression@software.sandia.gov)
  UNITTEST_COMPARE_CONST(Ex2Package1_REGRESSION_EMAIL_LIST ex2-package1-override@some.ornl.gov)
  UNITTEST_COMPARE_CONST(Ex2Package2_REGRESSION_EMAIL_LIST ex2package2-regression@project.site.gov)

ENDFUNCTION()


FUNCTION(UNITTEST_SINGLE_REPOSITORY_EMAIL_LIST)

  MESSAGE("\n***")
  MESSAGE("*** Test setting a single regression email address for all the packages in the first repo but defer to hard-coded package email addresses")
  MESSAGE("***\n")

  # Debugging
  #SET(TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS ON)
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_REPOSITORY_MASTER_EMAIL_ADDRESSS "my-repo@some.url.com")
  SET(${PROJECT_NAME}_REPOSITORY_EMAIL_URL_ADDRESSS_BASE OFF) # Will cause to be ignored!

  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${PROJECT_NAME} ".")
  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${EXTRA_REPO_NAME} ${EXTRA_REPO_DIR})
  TRIBITS_READ_ALL_PACKAGE_DEPENDENCIES()

  UNITTEST_COMPARE_CONST(Teuchos_REGRESSION_EMAIL_LIST "my-repo@some.url.com")
  UNITTEST_COMPARE_CONST(RTOp_REGRESSION_EMAIL_LIST thyra-regression@software.sandia.gov)
  UNITTEST_COMPARE_CONST(Ex2Package1_REGRESSION_EMAIL_LIST ex2-package1-override@some.ornl.gov)
  UNITTEST_COMPARE_CONST(Ex2Package2_REGRESSION_EMAIL_LIST ex2package2-regression@project.site.gov)

ENDFUNCTION()


FUNCTION(UNITTEST_SINGLE_REPOSITORY_EMAIL_LIST_OVERRIDE_0)

  MESSAGE("\n***")
  MESSAGE("*** Test setting a single regression email address for all the packages in the first repo with override")
  MESSAGE("***\n")

  # Debugging
  #SET(TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS ON)
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_REPOSITORY_MASTER_EMAIL_ADDRESSS "my-repo@some.url.com")
  SET(${PROJECT_NAME}_REPOSITORY_OVERRIDE_PACKAGE_EMAIL_LIST ON)
  SET(${PROJECT_NAME}_REPOSITORY_EMAIL_URL_ADDRESSS_BASE OFF) # Will cause to be ignored!

  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${PROJECT_NAME} ".")
  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${EXTRA_REPO_NAME} ${EXTRA_REPO_DIR})
  TRIBITS_READ_ALL_PACKAGE_DEPENDENCIES()

  UNITTEST_COMPARE_CONST(Teuchos_REGRESSION_EMAIL_LIST "my-repo@some.url.com")
  UNITTEST_COMPARE_CONST(RTOp_REGRESSION_EMAIL_LIST "my-repo@some.url.com")
  UNITTEST_COMPARE_CONST(Ex2Package1_REGRESSION_EMAIL_LIST ex2-package1-override@some.ornl.gov)
  UNITTEST_COMPARE_CONST(Ex2Package2_REGRESSION_EMAIL_LIST ex2package2-regression@project.site.gov)

ENDFUNCTION()


FUNCTION(UNITTEST_SINGLE_REPOSITORY_EMAIL_LIST_OVERRIDE_1)

  MESSAGE("\n***")
  MESSAGE("*** Test setting a single regression email address for all the packages in the second repo with override")
  MESSAGE("***\n")

  # Debugging
  #SET(TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS ON)
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${EXTRA_REPO_NAME}_REPOSITORY_MASTER_EMAIL_ADDRESSS "extra-repo@some.url.com")
  SET(${EXTRA_REPO_NAME}_REPOSITORY_OVERRIDE_PACKAGE_EMAIL_LIST ON)

  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${PROJECT_NAME} ".")
  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${EXTRA_REPO_NAME} ${EXTRA_REPO_DIR})
  TRIBITS_READ_ALL_PACKAGE_DEPENDENCIES()

  UNITTEST_COMPARE_CONST(Teuchos_REGRESSION_EMAIL_LIST teuchos-regression@repo.site.gov)
  UNITTEST_COMPARE_CONST(RTOp_REGRESSION_EMAIL_LIST thyra-regression@software.sandia.gov)
  UNITTEST_COMPARE_CONST(Ex2Package1_REGRESSION_EMAIL_LIST extra-repo@some.url.com)
  UNITTEST_COMPARE_CONST(Ex2Package2_REGRESSION_EMAIL_LIST extra-repo@some.url.com)

ENDFUNCTION()


FUNCTION(UNITTEST_SINGLE_PROJECT_EMAIL_LIST)

  MESSAGE("\n***")
  MESSAGE("*** Test setting a single regression email address for all the packages in a TriBITS Project but defer to hard-coded package email addresses")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_PROJECT_MASTER_EMAIL_ADDRESSS "my-project@some.url.com")
  SET(${PROJECT_NAME}_PROJECT_EMAIL_URL_ADDRESSS_BASE OFF)
  SET(${PROJECT_NAME}_REPOSITORY_EMAIL_URL_ADDRESSS_BASE OFF)

  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${PROJECT_NAME} ".")
  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${EXTRA_REPO_NAME} ${EXTRA_REPO_DIR})
  TRIBITS_READ_ALL_PACKAGE_DEPENDENCIES()

  UNITTEST_COMPARE_CONST(Teuchos_REGRESSION_EMAIL_LIST "my-project@some.url.com")
  UNITTEST_COMPARE_CONST(RTOp_REGRESSION_EMAIL_LIST thyra-regression@software.sandia.gov)
  UNITTEST_COMPARE_CONST(Ex2Package1_REGRESSION_EMAIL_LIST ex2-package1-override@some.ornl.gov)
  UNITTEST_COMPARE_CONST(Ex2Package2_REGRESSION_EMAIL_LIST my-project@some.url.com)

ENDFUNCTION()


FUNCTION(UNITTEST_SINGLE_PROJECT_EMAIL_LIST_OVERRIDE)

  MESSAGE("\n***")
  MESSAGE("*** Test setting a single regression email address for all the packages in a TriBITS Project and overriding hard-coded package email addresses")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_PROJECT_MASTER_EMAIL_ADDRESSS "my-project@some.url.com")
  SET(${PROJECT_NAME}_PROJECT_EMAIL_URL_ADDRESSS_BASE OFF)
  SET(${PROJECT_NAME}_REPOSITORY_EMAIL_URL_ADDRESSS_BASE OFF)
  SET(${PROJECT_NAME}_REPOSITORY_OVERRIDE_PACKAGE_EMAIL_LIST ON)
  SET(${EXTRA_REPO_NAME}_REPOSITORY_OVERRIDE_PACKAGE_EMAIL_LIST ON)

  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${PROJECT_NAME} ".")
  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${EXTRA_REPO_NAME} ${EXTRA_REPO_DIR})
  TRIBITS_READ_ALL_PACKAGE_DEPENDENCIES()

  UNITTEST_COMPARE_CONST(Teuchos_REGRESSION_EMAIL_LIST "my-project@some.url.com")
  UNITTEST_COMPARE_CONST(RTOp_REGRESSION_EMAIL_LIST my-project@some.url.com)
  UNITTEST_COMPARE_CONST(Ex2Package1_REGRESSION_EMAIL_LIST my-project@some.url.com)
  UNITTEST_COMPARE_CONST(Ex2Package2_REGRESSION_EMAIL_LIST my-project@some.url.com)

ENDFUNCTION()


FUNCTION(UNITTEST_EXTRA_REPO_MISSING_OPTIONAL_PACKAGE)

  MESSAGE("\n***")
  MESSAGE("*** Testing the reading of packages list with extra repo with missing optional upstream package")
  MESSAGE("***\n")

  SET(EXTRA_REPO_INCLUDE_MISSING_OPTIONAL_DEP_PACKAGE ON)
  SET(MESSAGE_WRAPPER_UNIT_TEST_MODE ON)

  # Debugging
  #SET(TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS ON)
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${PROJECT_NAME} ".")
  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${EXTRA_REPO_NAME} ${EXTRA_REPO_DIR})

  GLOBAL_SET(MESSAGE_WRAPPER_INPUT)  							     
  TRIBITS_READ_ALL_PACKAGE_DEPENDENCIES()

  UNITTEST_COMPARE_CONST(MESSAGE_WRAPPER_INPUT
    "WARNING: MissingUpstreamPackage is being ignored since its directory; is missing and MissingUpstreamPackage_ALLOW_MISSING_EXTERNAL_PACKAGE =; TRUE!")
  UNITTEST_COMPARE_CONST( ${PROJECT_NAME}_PACKAGES
    "Teuchos;RTOp;Ex2Package1;Ex2Package2")
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 "")

ENDFUNCTION()


FUNCTION(UNITTEST_EXTRA_REPO_MISSING_REQUIRED_PACKAGE)

  MESSAGE("\n***")
  MESSAGE("*** Testing the reading of packages list with extra repo with missing required upstream package")
  MESSAGE("***\n")

  SET(EXTRA_REPO_INCLUDE_MISSING_REQUIRED_DEP_PACKAGE ON)
  SET(MESSAGE_WRAPPER_UNIT_TEST_MODE ON)

  # Debugging
  #SET(TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS ON)
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${PROJECT_NAME} ".")
  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${EXTRA_REPO_NAME} ${EXTRA_REPO_DIR})

  GLOBAL_SET(MESSAGE_WRAPPER_INPUT)  							     
  TRIBITS_READ_ALL_PACKAGE_DEPENDENCIES()

  UNITTEST_COMPARE_CONST(MESSAGE_WRAPPER_INPUT
    "WARNING: MissingUpstreamPackage is being ignored since its directory; is missing and MissingUpstreamPackage_ALLOW_MISSING_EXTERNAL_PACKAGE =; TRUE!;WARNING: Setting Trilinos_ENABLE_Ex2Package1=OFF because; MissingUpstreamPackage is a required missing package!")
  UNITTEST_COMPARE_CONST( ${PROJECT_NAME}_PACKAGES
    "Teuchos;RTOp;Ex2Package1;Ex2Package2")
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 OFF)

ENDFUNCTION()


FUNCTION(UNITTEST_ELEVATE_SUBPACKAGES_SS_TO_PS)

  MESSAGE("\n***")
  MESSAGE("*** Testing elevating packages and subpackages from SS to PS")
  MESSAGE("***\n")

  # Debugging
  #SET(TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS ON)
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)
  #SET(TRIBITS_INSERT_STANDARD_PACKAGE_OPTIONS_DEBUG ON)

  SET(${PROJECT_NAME}_ELEVATE_SS_TO_PS TRUE)

  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(${PROJECT_NAME} ".")

  INCLUDE(${PROJECT_SOURCE_DIR}/extraRepoOnePackageThreeSubpackages/PackagesList.cmake)
  TRIBITS_PROCESS_PACKAGES_AND_DIRS_LISTS(extraRepoOnePackageThreeSubpackages
    extraRepoOnePackageThreeSubpackages)

  TRIBITS_READ_ALL_PACKAGE_DEPENDENCIES()

  UNITTEST_COMPARE_CONST( ${PROJECT_NAME}_SE_PACKAGES
    "Teuchos;RTOp;extraRepoOnePackageThreeSubpackagesSP1;extraRepoOnePackageThreeSubpackagesSP2;extraRepoOnePackageThreeSubpackagesSP3;extraRepoOnePackageThreeSubpackages")
  UNITTEST_COMPARE_CONST(extraRepoOnePackageThreeSubpackagesSP1_CLASSIFICATION PS)
  UNITTEST_COMPARE_CONST(extraRepoOnePackageThreeSubpackagesSP2_CLASSIFICATION PS)
  UNITTEST_COMPARE_CONST(extraRepoOnePackageThreeSubpackagesSP3_CLASSIFICATION EX)

ENDFUNCTION()


#
# B) Test enabled/disable logic
#


FUNCTION(UNITTEST_ENABLE_NO_PACKAGES)

  MESSAGE("\n***")
  MESSAGE("*** Test enabling no packages (the default)")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_PACKAGES 4)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_TPLS 4)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_MPI "")
  UNITTEST_COMPARE_CONST(TPL_ENABLE_BLAS "")
  UNITTEST_COMPARE_CONST(TPL_ENABLE_LAPACK "")
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos "")
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp "")
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 "")
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 "")

ENDFUNCTION()


FUNCTION(UNITTEST_ENABLE_ALL_PACKAGES)

  MESSAGE("\n***")
  MESSAGE("*** Test enabling all packages")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_ALL_PACKAGES ON)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_PACKAGES 4)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_TPLS 4)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_MPI "")
  UNITTEST_COMPARE_CONST(TPL_ENABLE_BLAS ON)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_LAPACK ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 "")

ENDFUNCTION()


FUNCTION(UNITTEST_ENABLE_ALL_PACKAGES_SS)

  MESSAGE("\n***")
  MESSAGE("*** Test enabling all secondary stable packages")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_ALL_PACKAGES ON)
  SET(${PROJECT_NAME}_ENABLE_SECONDARY_STABLE_CODE ON)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_PACKAGES 4)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_TPLS 4)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_MPI "")
  UNITTEST_COMPARE_CONST(TPL_ENABLE_BLAS ON)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_LAPACK ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)

ENDFUNCTION()


FUNCTION(UNITTEST_ENABLE_ALL_PACKAGES_SS_NO_IMPLICIT_ENABLE_EXTRA_REPO)

  MESSAGE("\n***")
  MESSAGE("*** Test enabling all secondary stable packages except for extra repo")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_ALL_PACKAGES ON)
  SET(${PROJECT_NAME}_ENABLE_SECONDARY_STABLE_CODE ON)
  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE ON)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_PACKAGES 4)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_TPLS 4)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_MPI "")
  UNITTEST_COMPARE_CONST(TPL_ENABLE_BLAS ON)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_LAPACK ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 "")
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 "")

ENDFUNCTION()


FUNCTION(UNITTEST_ENABLE_ALL_PACKAGES_SS_NO_IMPLICIT_ENABLE_EXTRA_REPO_EXCEPT_PACKAGE2)

  MESSAGE("\n***")
  MESSAGE("*** Test enabling all secondary stable packages except for extra repo except for Ex2Package2")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_ALL_PACKAGES ON)
  SET(${PROJECT_NAME}_ENABLE_SECONDARY_STABLE_CODE ON)
  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE ON)
  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE_EXCEPT Ex2Package2)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_PACKAGES 4)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_TPLS 4)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_MPI "")
  UNITTEST_COMPARE_CONST(TPL_ENABLE_BLAS ON)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_LAPACK ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 "")
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)

ENDFUNCTION()


FUNCTION(UNITTEST_ENABLE_ALL_PACKAGES_SS_NO_IMPLICIT_ENABLE_EXTRA_REPO_EXCEPT_PACKAGE1)

  MESSAGE("\n***")
  MESSAGE("*** Test enabling all secondary stable packages except for extra repo except for Ex2Package1")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_ALL_PACKAGES ON)
  SET(${PROJECT_NAME}_ENABLE_SECONDARY_STABLE_CODE ON)
  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE ON)
  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE_EXCEPT Ex2Package1)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_PACKAGES 4)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_TPLS 4)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_MPI "")
  UNITTEST_COMPARE_CONST(TPL_ENABLE_BLAS ON)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_LAPACK ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 "")

ENDFUNCTION()


FUNCTION(UNITTEST_ENABLE_ALL_OPTIONAL_SS_NO_IMPLICIT_ENABLE_EXTRA_REPO_EXCEPT_PACKAGE2_ENABLE_PACKAGE2)

  MESSAGE("\n***")
  MESSAGE("*** Test enabling all secondary stable packages except for extra repo except for Ex2Package1 with optional packages turned on")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_SECONDARY_STABLE_CODE ON)
  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE ON)
  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE_EXCEPT Ex2Package2)
  SET(${PROJECT_NAME}_ENABLE_ALL_OPTIONAL_PACKAGES ON)
  SET(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_PACKAGES 4)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_NUM_TPLS 4)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_MPI "")
  UNITTEST_COMPARE_CONST(TPL_ENABLE_BLAS ON)
  UNITTEST_COMPARE_CONST(TPL_ENABLE_LAPACK ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp "")
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 "")
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)

ENDFUNCTION()


#
# C) Testing TRIBITS_APPLY_REPOSITORY_NO_IMPLICIT_PACKAGE_ENABLE_DISABLE()
#


FUNCTION(UNITTEST_TARNIPED_ENABLE_ALL)

  MESSAGE("\n***")
  MESSAGE("*** Testing TARNIPE() enable all packages")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_SECONDARY_STABLE_CODE ON)
  SET(${PROJECT_NAME}_ENABLE_ALL_PACKAGES ON)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)

  MESSAGE("Unit test: Disabling repository implicitly excluded packages.")

  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE ON)

  TRIBITS_APPLY_REPOSITORY_NO_IMPLICIT_PACKAGE_ENABLE_DISABLE()  

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 OFF)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 OFF)

ENDFUNCTION()


FUNCTION(UNITTEST_TARNIPED_ENABLE_ALL_TESTS)

  MESSAGE("\n***")
  MESSAGE("*** Testing TARNIPED() with all packages and tests")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_SECONDARY_STABLE_CODE ON)
  SET(${PROJECT_NAME}_ENABLE_ALL_PACKAGES ON)
  SET(${PROJECT_NAME}_ENABLE_TESTS ON)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(Ex2Package1_ENABLE_TESTS ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)
  UNITTEST_COMPARE_CONST(Ex2Package2_ENABLE_TESTS ON)

  MESSAGE("Unit test: Disabling repository implicitly excluded packages.")

  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE ON)

  TRIBITS_APPLY_REPOSITORY_NO_IMPLICIT_PACKAGE_ENABLE_DISABLE()  

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)

ENDFUNCTION()


FUNCTION(UNITTEST_TARNIPED_ENABLE_ALL_Ex2Package1_ENABLE_TESTS)

  MESSAGE("\n***")
  MESSAGE("*** Testing TARNIPED() with all packages and tests for only Ex2Package1")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_SECONDARY_STABLE_CODE ON)
  SET(${PROJECT_NAME}_ENABLE_ALL_PACKAGES ON)
  SET(Ex2Package1_ENABLE_TESTS ON)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(Ex2Package1_ENABLE_TESTS ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)
  UNITTEST_COMPARE_CONST(Ex2Package2_ENABLE_TESTS "")

  MESSAGE("Unit test: Disabling repository implicitly excluded packages.")

  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE ON)

  TRIBITS_APPLY_REPOSITORY_NO_IMPLICIT_PACKAGE_ENABLE_DISABLE()  

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 OFF)

ENDFUNCTION()


FUNCTION(UNITTEST_TARNIPED_ALLOW_Ex2Package1_ENABLE_ALL)

  MESSAGE("\n***")
  MESSAGE("*** Testing TARNIPED() with all packages allowing ExPackage1")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_SECONDARY_STABLE_CODE ON)
  SET(${PROJECT_NAME}_ENABLE_ALL_PACKAGES ON)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)

  MESSAGE("Unit test: Disabling repository implicitly excluded packages.")

  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE ON)
  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE_EXCEPT Ex2Package1)

  TRIBITS_APPLY_REPOSITORY_NO_IMPLICIT_PACKAGE_ENABLE_DISABLE()  

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 OFF)

ENDFUNCTION()


FUNCTION(UNITTEST_TARNIPED_ALLOW_Ex2Package2_ENABLE_ALL)

  MESSAGE("\n***")
  MESSAGE("*** Testing TARNIPED() with all packages allowing ExPackage2")
  MESSAGE("***\n")

  # Debugging
  #SET(${PROJECT_NAME}_VERBOSE_CONFIGURE ON)

  SET(${PROJECT_NAME}_ENABLE_SECONDARY_STABLE_CODE ON)
  SET(${PROJECT_NAME}_ENABLE_ALL_PACKAGES ON)

  UNITTEST_HELPER_READ_AND_PROESS_PACKAGES()

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)

  MESSAGE("Unit test: Disabling repository implicitly excluded packages.")

  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE ON)
  SET(${EXTRA_REPO_NAME}_NO_IMPLICIT_PACKAGE_ENABLE_EXCEPT Ex2Package2)

  TRIBITS_APPLY_REPOSITORY_NO_IMPLICIT_PACKAGE_ENABLE_DISABLE()  

  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Teuchos ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_RTOp ON)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package1 OFF)
  UNITTEST_COMPARE_CONST(${PROJECT_NAME}_ENABLE_Ex2Package2 ON)

ENDFUNCTION()



#####################################################################
#
# Execute the unit tests
#
#####################################################################

# Assume that all unit tests will pass by default
GLOBAL_SET(UNITTEST_OVERALL_PASS TRUE)
GLOBAL_SET(UNITTEST_OVERALL_NUMPASSED 0)
GLOBAL_SET(UNITTEST_OVERALL_NUMRUN 0)

#
# Run the unit tests
#

# A) Test basic package processing and reading dependencies 
UNITTEST_READ_PACKAGES_LIST_WITH_EXTRA_REPO()
UNITTEST_READ_TPLS_LISTS_WTIH_DUPLICATE_TPLS()
UNITTEST_STANDARD_PROJECT_DEFAULT_EMAIL_ADDRESS_BASE()
UNITTEST_SINGLE_REPOSITORY_EMAIL_LIST()
UNITTEST_SINGLE_REPOSITORY_EMAIL_LIST_OVERRIDE_0()
UNITTEST_SINGLE_REPOSITORY_EMAIL_LIST_OVERRIDE_1()
UNITTEST_SINGLE_PROJECT_EMAIL_LIST()
UNITTEST_SINGLE_PROJECT_EMAIL_LIST_OVERRIDE()
UNITTEST_EXTRA_REPO_MISSING_OPTIONAL_PACKAGE()
UNITTEST_EXTRA_REPO_MISSING_REQUIRED_PACKAGE()
UNITTEST_ELEVATE_SUBPACKAGES_SS_TO_PS()

# B) Test enabled/disable logic
UNITTEST_ENABLE_NO_PACKAGES()
UNITTEST_ENABLE_ALL_PACKAGES()
UNITTEST_ENABLE_ALL_PACKAGES_SS()
UNITTEST_ENABLE_ALL_PACKAGES_SS_NO_IMPLICIT_ENABLE_EXTRA_REPO()
UNITTEST_ENABLE_ALL_PACKAGES_SS_NO_IMPLICIT_ENABLE_EXTRA_REPO_EXCEPT_PACKAGE2()
UNITTEST_ENABLE_ALL_PACKAGES_SS_NO_IMPLICIT_ENABLE_EXTRA_REPO_EXCEPT_PACKAGE1()
UNITTEST_ENABLE_ALL_OPTIONAL_SS_NO_IMPLICIT_ENABLE_EXTRA_REPO_EXCEPT_PACKAGE2_ENABLE_PACKAGE2()

# C) Testing TRIBITS_APPLY_REPOSITORY_NO_IMPLICIT_PACKAGE_ENABLE_DISABLE()
UNITTEST_TARNIPED_ENABLE_ALL()
UNITTEST_TARNIPED_ENABLE_ALL_TESTS()
UNITTEST_TARNIPED_ENABLE_ALL_Ex2Package1_ENABLE_TESTS()
UNITTEST_TARNIPED_ALLOW_Ex2Package1_ENABLE_ALL()
UNITTEST_TARNIPED_ALLOW_Ex2Package2_ENABLE_ALL()

# Pass in the number of expected tests that must pass!
UNITTEST_FINAL_RESULT(143)
