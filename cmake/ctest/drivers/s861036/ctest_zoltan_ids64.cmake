
INCLUDE("${CTEST_SCRIPT_DIRECTORY}/../../TrilinosCTestDriverCore.cmake")

#
# Set the options specific to this build case
#   Build Zoltan, Isorropia, Epetra and EpetraExt
#   Specify 64-bit global IDs for Zoltan and Isorropia
#  "-D Trilinos_ENABLE_Isorropia:BOOL=ON"
#  "-D Trilinos_ENABLE_Epetra:BOOL=ON"
#  "-D Trilinos_ENABLE_EpetraExt:BOOL=ON"
#

SET(COMM_TYPE MPI)
SET(BUILD_TYPE RELEASE)
SET(BUILD_DIR_NAME MPI_ZOLTAN_64)

SET(Trilinos_ENABLE_SECONDARY_STABLE_CODE OFF)

SET( EXTRA_CONFIGURE_OPTIONS
  "-DDART_TESTING_TIMEOUT:STRING=300"
  "-DTrilinos_ENABLE_EXPLICIT_INSTANTIATION:BOOL=ON"
  "-DTrilinos_DATA_DIR:STRING=$ENV{TRILINOSDATADIRECTORY}"
  "-DTPL_ENABLE_MPI:BOOL=ON"
  "-D CMAKE_C_FLAGS:STRING=-std=c99 -DZOLTAN_ID_TYPE_LONG"
  "-D CMAKE_CXX_FLAGS:STRING=-DZOLTAN_ID_TYPE_LONG"
  "-D MPI_EXEC_MAX_NUMPROCS:STRING=11"
  "-D Trilinos_ENABLE_ALL_PACKAGES:BOOL=OFF"
  "-D Trilinos_ENABLE_EXAMPLES:BOOL=ON"
  "-D Trilinos_ENABLE_TESTS:BOOL=ON"
  "-D Trilinos_ENABLE_Zoltan:BOOL=ON"
  "-D Zoltan_ENABLE_ParMETIS:BOOL=ON"
  "-D Zoltan_ENABLE_Scotch:BOOL=ON"
  "-D ParMETIS_INCLUDE_DIRS:FILEPATH=/home/lafisk/system/parmetis/ParMetis-3.1"
  "-D ParMETIS_LIBRARY_DIRS:FILEPATH=/home/lafisk/system/parmetis/ParMetis-3.1"
  "-D Scotch_INCLUDE_DIRS:FILEPATH=/home/lriesen/system/scotch_5.1.10a-32/include"
  "-D Scotch_LIBRARY_DIRS:FILEPATH=/home/lriesen/system/scotch_5.1.10a-32/lib"
  )

SET( CTEST_DASHBOARD_ROOT "${TRILINOS_CMAKE_DIR}/../../${BUILD_DIR_NAME}" )
SET( CTEST_NOTES_FILES "${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}" )
SET( CTEST_BUILD_FLAGS "-j8 -i" )
SET( CTEST_COVERAGE_COMMAND /usr/bin/gcov )
SET( CTEST_MEMORYCHECK_COMMAND /usr/local/bin/valgrind )

TRILINOS_CTEST_DRIVER()
