
#
# Set the options specific to this build
#

SET(COMM_TYPE MPI)
SET(BUILD_TYPE RELEASE)
SET(BUILD_DIR_NAME MPI_OPT)

SET(CTEST_DUAL_CONFIGURE ON) # Needed because of TPL include header problem

SET(CTEST_DO_COVERAGE_TESTING TRUE)


SET(CTEST_INITIAL_CACHE "

CMAKE_CXX_FLAGS_${BUILD_TYPE}:STRING=-O3 -fprofile-arcs -ftest-coverage
CMAKE_C_FLAGS_${BUILD_TYPE}:STRING=-O3 -fprofile-arcs -ftest-coverage
CMAKE_Fortran_FLAGS_${BUILD_TYPE}:STRING=-O5 -fprofile-arcs -ftest-coverage

DART_TESTING_TIMEOUT:STRING=600
CMAKE_VERBOSE_MAKEFILE:BOOL=TRUE

Trilinos_ENABLE_ALL_PACKAGES:BOOL=ON

Trilinos_ENABLE_DEPENCENCY_UNIT_TESTS:BOOL=OFF

Trilinos_ENABLE_TESTS:BOOL=ON

Trilinos_ENABLE_EXPLICIT_INSTANTIATION:BOOL=ON

TPL_ENABLE_Boost:BOOL=ON

TPL_ENABLE_ParMETIS:BOOL=ON
ParMETIS_LIBRARY_DIRS:PATH=/home/kddevin/code/ParMETIS3_1

TPL_ENABLE_Scotch:BOOL=ON
Scotch_INCLUDE_DIRS:PATH=/home/kddevin/code/scotch_5.1/include
Scotch_LIBRARY_DIRS:PATH=/home/kddevin/code/scotch_5.1/lib

EpetraExt_BUILD_GRAPH_REORDERINGS:BOOL=ON
EpetraExt_BUILD_BDF:BOOL=ON

")


#
# Read in the platform-independent and platform-dependent options
#

INCLUDE("${CTEST_SCRIPT_DIRECTORY}/TrilinosCTestSupport.godel.cmake")
