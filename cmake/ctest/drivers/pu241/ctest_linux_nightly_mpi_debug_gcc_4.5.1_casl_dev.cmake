#
# Build all Secondary Stable CASL VRI add-on Trilnos packages with GCC 4.5.1 compiler
#

INCLUDE("${CTEST_SCRIPT_DIRECTORY}/hybrid-build-11.cmake")
INCLUDE("${CTEST_SCRIPT_DIRECTORY}/TribitsCTestDriverCore.pu241.gcc.4.5.1.cmake")
INCLUDE("${CTEST_SCRIPT_DIRECTORY}/SubmitToCaslDev.cmake")
INCLUDE("${CTEST_SCRIPT_DIRECTORY}/casl-vri-packages-coupled.cmake")
INCLUDE("${CTEST_SCRIPT_DIRECTORY}/enable_casl_packages.gcc.mpi.cmake")

SET(COMM_TYPE MPI)
SET(BUILD_TYPE DEBUG)
SET(BUILD_DIR_NAME MPI_DEBUG_HYBRID11_CASLDEV)
#SET(CTEST_TEST_TYPE Experimental)
#SET(CTEST_TEST_TIMEOUT 900)
SET(Trilinos_ENABLE_SECONDARY_STABLE_CODE ON)
SET(EXTRA_CONFIGURE_OPTIONS
  ${EXTRA_CONFIGURE_OPTIONS}
  -DTPL_ENABLE_Netcdf=ON
  -DSTK_ENABLE_BoostLib:BOOL=OFF
  )

TRILINOS_SYSTEM_SPECIFIC_CTEST_DRIVER()
