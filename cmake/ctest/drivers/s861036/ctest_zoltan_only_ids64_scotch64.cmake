
INCLUDE("${CTEST_SCRIPT_DIRECTORY}/../../TrilinosCTestDriverCore.cmake")

#
# Build Zoltan and its examples and tests.
#  Zoltan's global IDs are 64 bits on 64 bit machines.
#  Scotch uses 64 bit IDs
#

SET(COMM_TYPE MPI)
SET(BUILD_TYPE RELEASE)
SET(BUILD_DIR_NAME ZOLTAN_ONLY_64_SCOTCH_64)
SET(Trilinos_PACKAGES Zoltan)

SET(Trilinos_ENABLE_SECONDARY_STABLE_CODE OFF)

SET( EXTRA_CONFIGURE_OPTIONS
  "-DDART_TESTING_TIMEOUT:STRING=1800"
  "-DTrilinos_ENABLE_EXPLICIT_INSTANTIATION:BOOL=ON"
  "-DTrilinos_DATA_DIR:STRING=$ENV{TRILINOSDATADIRECTORY}"
  "-DTPL_ENABLE_MPI:BOOL=ON"
  "-DCMAKE_C_FLAGS:STRING=-std=c99 -DZOLTAN_ID_TYPE_LONG"
  "-DCMAKE_CXX_FLAGS:STRING=-DZOLTAN_ID_TYPE_LONG"
  "-DMPI_EXEC_MAX_NUMPROCS:STRING=11"
  "-DTrilinos_ENABLE_ALL_PACKAGES:BOOL=OFF"
  "-DTrilinos_ENABLE_EXAMPLES:BOOL=ON"
  "-DTrilinos_ENABLE_TESTS:BOOL=ON"
  "-DTrilinos_ENABLE_Zoltan:BOOL=ON"
  "-DZoltan_ENABLE_Scotch:BOOL=ON"
  "-DScotch_INCLUDE_DIRS:FILEPATH=/home/lriesen/system/scotch_5.1.10a/include"
  "-DScotch_LIBRARY_DIRS:FILEPATH=/home/lriesen/system/scotch_5.1.10a/lib"
  )

SET( CTEST_DASHBOARD_ROOT "${TRILINOS_CMAKE_DIR}/../../${BUILD_DIR_NAME}" )
SET( CTEST_NOTES_FILES "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}" )
SET( CTEST_BUILD_FLAGS "-j4 -i" )
SET( CTEST_COVERAGE_COMMAND /usr/bin/gcov )
SET( CTEST_MEMORYCHECK_COMMAND /usr/local/bin/valgrind )

TRILINOS_CTEST_DRIVER()
