#-------------------------------------------------------------------------------
# Nightly testing on linux platform thumper.mp.sandia.gov
# Debug with Coverage and MemoryCheck
#-------------------------------------------------------------------------------

SET (CTEST_SOURCE_NAME Trilinos)
SET (TEST_TYPE nightly)
SET (BUILD_TYPE debug)

SET (CTEST_DASHBOARD_ROOT /scratch/rabartl/dashboards)
SET (CTEST_CMAKE_COMMAND "\"${CMAKE_EXECUTABLE_NAME}\"")

# Options for Nightly builds

SET (CTEST_START_WITH_EMPTY_BINARY_DIRECTORY TRUE)

SET (CTEST_CVS_CHECKOUT
  "cvs -q -d :ext:@software.sandia.gov:/space/CVS co ${CTEST_SOURCE_NAME}"
)

SET (CTEST_CVS_COMMAND
  "cvs -q -d :ext:software.sandia.gov:/space/CVS co ${CTEST_SOURCE_NAME}"
)

SET (CTEST_BINARY_NAME ${CTEST_SOURCE_NAME}-Build)
SET (CTEST_SOURCE_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_SOURCE_NAME}")
SET (CTEST_BINARY_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_BINARY_NAME}")

SET (CTEST_COMMAND 
  "\"${CTEST_EXECUTABLE_NAME}\" -D NightlyStart"
  "\"${CTEST_EXECUTABLE_NAME}\" -D NightlyUpdate"
  "\"${CTEST_EXECUTABLE_NAME}\" -D NightlyConfigure"
  "\"${CTEST_EXECUTABLE_NAME}\" -D NightlyBuild"
  "\"${CTEST_EXECUTABLE_NAME}\" -D NightlySubmit"
  "\"${CTEST_EXECUTABLE_NAME}\" -D NightlyTest"
  #"\"${CTEST_EXECUTABLE_NAME}\" -D NightlySubmit"
  #"\"${CTEST_EXECUTABLE_NAME}\" -D NightlyCoverage"
  #"\"${CTEST_EXECUTABLE_NAME}\" -D NightlySubmit"
  #"\"${CTEST_EXECUTABLE_NAME}\" -D NightlyMemCheck"
  "\"${CTEST_EXECUTABLE_NAME}\" -D NightlySubmit -A \"${CTEST_BINARY_DIRECTORY}/CMakeCache.txt\;${CTEST_DASHBOARD_ROOT}/../scripts/ctest_linux_nightly_serial_debug_thumper.cmake\""
)

SET (CTEST_INITIAL_CACHE "

Trilinos_ENABLE_DEPENCENCY_UNIT_TESTS:BOOL=OFF

MEMORYCHECK_COMMAND:FILEPATH=/usr/local/bin/valgrind
DART_TESTING_TIMEOUT:STRING=600
CMAKE_VERBOSE_MAKEFILE:BOOL=TRUE

Trilinos_ENABLE_ALL_PACKAGES:BOOL=ON
Trilinos_ENABLE_TESTS:BOOL=ON
Trilinos_ENABLE_FORTRAN:BOOL=ON
Teuchos_ENABLE_DEBUG:BOOL=ON
Teuchos_ENABLE_COMPLEX:BOOL=ON
Teuchos_ENABLE_EXTENDED:BOOL=ON
Teuchos_ENABLE_EXPLICIT_INSTANTIATION:BOOL=ON
EpetraExt_BUILD_GRAPH_REORDERINGS:BOOL=ON
EpetraExt_BUILD_BDF:BOOL=ON

BUILDNAME:STRING=$ENV{HOSTTYPE}-${TEST_TYPE}-${BUILD_TYPE}

CMAKE_BUILD_TYPE:STRING=${BUILD_TYPE}

CMAKE_CXX_FLAGS:STRING=-g -O0 -ansi -pedantic -Wall -Wshadow -Wunused-variable -Wunused-function -Wno-system-headers -Wno-deprecated -Woverloaded-virtual -Wwrite-strings -fprofile-arcs -ftest-coverage -fexceptions

CMAKE_C_FLAGS:STRING=-g -O0 -Wall -fprofile-arcs -ftest-coverage -fexceptions

CMAKE_EXE_LINKER_FLAGS:STRING=-fprofile-arcs -ftest-coverage -lg2c

MAKECOMMAND:STRING=gmake -j1 -i

")
