
SET(CTEST_SOURCE_NAME Trilinos)
SET(CTEST_BINARY_NAME BUILD)
SET(TEST_TYPE nightly)
SET(BUILD_TYPE DEBUG)
SET(EXTRA_BUILD_TYPE serial)
SET(HOSTTYPE Linux) # Have to set this manually on this machine for some reason?
SET(CTEST_DASHBOARD_ROOT /home/rabartl/PROJECTS/dashboards/Trilinos/SERIAL_DEBUG)
SET(CTEST_CMAKE_COMMAND /usr/local/bin/cmake)

# Options for Nightly builds

SET(CTEST_START_WITH_EMPTY_BINARY_DIRECTORY TRUE)
#SET(CTEST_START_WITH_EMPTY_BINARY_DIRECTORY FALSE)

SET(CTEST_CVS_CHECKOUT
  "cvs -q -d :ext:software.sandia.gov:/space/CVS co ${CTEST_SOURCE_NAME}"
  )
SET (CTEST_CVS_COMMAND
  "cvs -q"
  )

SET(CTEST_SOURCE_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_SOURCE_NAME}")
SET(CTEST_BINARY_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_BINARY_NAME}")

SET(TEST_TYPE $ENV{CTEST_TEST_TYPE})
IF (NOT TEST_TYPE)
  SET(TEST_TYPE Nightly)
ENDIF()

SET(CTEST_COMMAND 
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}Start"
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}Update"
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}Configure"
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}Build"
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}Submit"
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}Test"
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}Submit"
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}Coverage"
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}Submit"
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}MemCheck"
  "\"${CTEST_EXECUTABLE_NAME}\" -D ${TEST_TYPE}Submit -A \"${CTEST_BINARY_DIRECTORY}/CMakeCache.txt\;${CTEST_SCRIPT_DIRECTORY}/${CTEST_SCRIPT_NAME}\""
)

SET(CTEST_INITIAL_CACHE "

CMAKE_BUILD_TYPE:STRING=${BUILD_TYPE}

BUILDNAME:STRING=${HOSTTYPE}-${TEST_TYPE}-${EXTRA_BUILD_TYPE}-${BUILD_TYPE}

CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/g++
CMAKE_C_COMPILER:FILEPATH=/usr/bin/gcc
CMAKE_Fortran_COMPILER:FILEPATH=/usr/bin/gfortran

CMAKE_CXX_FLAGS_${BUILD_TYPE}:STRING=-g -O0 -fprofile-arcs -ftest-coverage
CMAKE_C_FLAGS)${BUILD_TYPE}:STRING=-g -O0 -fprofile-arcs -ftest-coverage
CMAKE_Fortran_FLAGS_${BUILD_TYPE}:STRING=-g -O0 -fprofile-arcs -ftest-coverage

CMAKE_EXE_LINKER_FLAGS:STRING=-fprofile-arcs -ftest-coverage -lm

MEMORYCHECK_COMMAND:FILEPATH=/usr/bin/valgrind

MAKECOMMAND:STRING=make -j8 -i

DART_TESTING_TIMEOUT:STRING=600
CMAKE_VERBOSE_MAKEFILE:BOOL=TRUE

Trilinos_ENABLE_DEPENCENCY_UNIT_TESTS:BOOL=OFF

Trilinos_ENABLE_ALL_PACKAGES:BOOL=ON
Trilinos_ENABLE_TESTS:BOOL=ON
Trilinos_ENABLE_DEBUG:BOOL=ON_
Trilinos_ENABLE_EXPLICIT_INSTANTIATION:BOOL=ON

TPL_ENABLE_Boost:BOOL=ON

EpetraExt_BUILD_GRAPH_REORDERINGS:BOOL=ON
EpetraExt_BUILD_BDF:BOOL=ON

")
