INCLUDE(TribitsAddExecutable)

FUNCTION(FORTRILINOS_ADD_UNIT_TEST TESTING_MODULE_NAME)

######################################################################
######################################################################

INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../UNITTEST_COMMON
  ${CMAKE_CURRENT_BINARY_DIR}
  )

SET(TESTING_MODULE_NAME_OO ${TESTING_MODULE_NAME}_oo)

######################################################################

SET(FORTRAN_DRIVER_TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/../UNITTEST_COMMON/unittest.F90)
SET(CONFIG_FILE_PATH ${CMAKE_CURRENT_BINARY_DIR}/../../../src)
SET(UNITTEST_COMMON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../UNITTEST_COMMON)
SET(RUNTIME_MACROS_FILE ${UNITTEST_COMMON_PATH}/runtime_macros.h)

######################################################################

SET(UNITTEST_MACROS_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../UNITTEST_MACROS)
SET(MACROEXP_PERL_SCRIPT ${UNITTEST_MACROS_PATH}/expand_macros.pl)
SET(MACROEXP_PERL_MODS ${UNITTEST_MACROS_PATH}/macroexp.pm
                       ${UNITTEST_MACROS_PATH}/misc.pm
                       ${UNITTEST_MACROS_PATH}/parser.pm)

######################################################################

SET(PREPROCESSING_INPUT ${CMAKE_CURRENT_SOURCE_DIR}/${TESTING_MODULE_NAME}_tests.un)
SET(FORTRAN_DRIVER_STEM ${TESTING_MODULE_NAME}_tests)
SET(FORTRAN_CALLS_STEM ${TESTING_MODULE_NAME}_test_calls)
SET(FORTRAN_IMPLS_STEM ${TESTING_MODULE_NAME}_test_impls)

######################################################################

SET(PREPROCESSING_INPUT_OO ${CMAKE_CURRENT_SOURCE_DIR}/${TESTING_MODULE_NAME_OO}_tests.un)
SET(FORTRAN_DRIVER_STEM_OO ${TESTING_MODULE_NAME_OO}_tests)
SET(FORTRAN_CALLS_STEM_OO ${TESTING_MODULE_NAME_OO}_test_calls)
SET(FORTRAN_IMPLS_STEM_OO ${TESTING_MODULE_NAME_OO}_test_impls)
SET(UNITTEST_UTILS_FILE_OO ${UNITTEST_COMMON_PATH}/ForTrilinos_oo_unittest_utils.F90)

######################################################################
######################################################################

ADD_CUSTOM_COMMAND(
  OUTPUT ${FORTRAN_IMPLS_STEM}-tmp.F90 ${FORTRAN_CALLS_STEM}.F90 ${FORTRAN_DRIVER_STEM}.tests ${FORTRAN_DRIVER_STEM}.mpitests
  COMMAND ForTrilinos_unittest_parser ${TESTING_MODULE_NAME} ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${PREPROCESSING_INPUT} ${RUNTIME_MACROS_FILE} ForTrilinos_unittest_parser
  )

ADD_CUSTOM_COMMAND(
  OUTPUT ${FORTRAN_IMPLS_STEM}.F90
  COMMAND perl -I ${UNITTEST_MACROS_PATH} ${MACROEXP_PERL_SCRIPT} < ${FORTRAN_IMPLS_STEM}-tmp.F90 > ${FORTRAN_IMPLS_STEM}.F90
  DEPENDS ${FORTRAN_IMPLS_STEM}-tmp.F90 ${PREPROCESSING_INPUT} ${RUNTIME_MACROS_FILE} ${MACROEXP_PERL_SCRIPT} ${MACROEXP_PERL_MODS}
  )

######################################################################

TRIBITS_ADD_EXECUTABLE(${FORTRAN_DRIVER_STEM}
  SOURCES
  ${FORTRAN_IMPLS_STEM}.F90
  ${FORTRAN_CALLS_STEM}.F90
  ${FORTRAN_DRIVER_TEMPLATE}
  ${FORTRAN_DRIVER_STEM}.tests
  ${FORTRAN_DRIVER_STEM}.mpitests
  ${RUNTIME_MACROS_FILE}
  ${PREPROCESSING_INPUT}
  NOEXEPREFIX
  )

SET_PROPERTY(
  TARGET ${FORTRAN_DRIVER_STEM}
  APPEND PROPERTY COMPILE_DEFINITIONS TEST_CALLS_FILE=${TESTING_MODULE_NAME}_test_calls TEST_IMPLS_FILE_STR="${TESTING_MODULE_NAME}_test_impls"
  )

TARGET_LINK_LIBRARIES(${FORTRAN_DRIVER_STEM} fortrilinos)

######################################################################

TRIBITS_ADD_TEST(
  ../UNITTEST_LAUNCHER/ForTrilinos_unittest_launcher_serial
  NOEXEPREFIX
  NAME ${FORTRAN_DRIVER_STEM}
  ARGS ${FORTRAN_DRIVER_STEM}.tests
  COMM serial
  PASS_REGULAR_EXPRESSION "END RESULT: ALL TESTS PASSED"
  )

TRIBITS_ADD_TEST(
  ${FORTRAN_DRIVER_STEM}
  NOEXEPREFIX
  NAME ${FORTRAN_DRIVER_STEM}
  ARGS "-f ${FORTRAN_DRIVER_STEM}.mpitests"
  COMM mpi
  NUM_MPI_PROCS 3
  PASS_REGULAR_EXPRESSION "END RESULT: ALL TESTS PASSED"
  )

######################################################################
######################################################################

IF(ForTrilinos_ENABLE_OBJECT_ORIENTED)

ADD_CUSTOM_COMMAND(
  OUTPUT ${FORTRAN_IMPLS_STEM_OO}-tmp.F90 ${FORTRAN_CALLS_STEM_OO}.F90 ${FORTRAN_DRIVER_STEM_OO}.tests ${FORTRAN_DRIVER_STEM_OO}.mpitests
  COMMAND ForTrilinos_unittest_parser ${TESTING_MODULE_NAME_OO} ${CMAKE_CURRENT_SOURCE_DIR}
  DEPENDS ${PREPROCESSING_INPUT_OO} ${RUNTIME_MACROS_FILE} ForTrilinos_unittest_parser
  )

ADD_CUSTOM_COMMAND(
  OUTPUT ${FORTRAN_IMPLS_STEM_OO}.F90
  COMMAND perl -I ${UNITTEST_MACROS_PATH} ${MACROEXP_PERL_SCRIPT} < ${FORTRAN_IMPLS_STEM_OO}-tmp.F90 > ${FORTRAN_IMPLS_STEM_OO}.F90
  DEPENDS ${FORTRAN_IMPLS_STEM_OO}-tmp.F90 ${PREPROCESSING_INPUT} ${RUNTIME_MACROS_FILE} ${MACROEXP_PERL_SCRIPT} ${MACROEXP_PERL_MODS}
  )

######################################################################

TRIBITS_ADD_EXECUTABLE(${FORTRAN_DRIVER_STEM_OO}
  SOURCES
  ${UNITTEST_UTILS_FILE_OO}
  ${FORTRAN_IMPLS_STEM_OO}.F90
  ${FORTRAN_CALLS_STEM_OO}.F90
  ${FORTRAN_DRIVER_TEMPLATE}
  ${FORTRAN_DRIVER_STEM_OO}.tests
  ${FORTRAN_DRIVER_STEM_OO}.mpitests
  ${RUNTIME_MACROS_FILE}
  ${PREPROCESSING_INPUT_OO}
  NOEXEPREFIX
  )

SET_PROPERTY(
  TARGET ${FORTRAN_DRIVER_STEM_OO}
  APPEND PROPERTY COMPILE_DEFINITIONS TEST_CALLS_FILE=${TESTING_MODULE_NAME_OO}_test_calls TEST_IMPLS_FILE_STR="${TESTING_MODULE_NAME_OO}_test_impls"
  )

TARGET_LINK_LIBRARIES(${FORTRAN_DRIVER_STEM_OO} fortrilinos)

######################################################################

TRIBITS_ADD_TEST(
  ../UNITTEST_LAUNCHER/ForTrilinos_unittest_launcher_serial
  NOEXEPREFIX
  NAME ${FORTRAN_DRIVER_STEM_OO}
  ARGS ${FORTRAN_DRIVER_STEM_OO}.tests
  COMM serial
  PASS_REGULAR_EXPRESSION "END RESULT: ALL TESTS PASSED"
  )

TRIBITS_ADD_TEST(
  ${FORTRAN_DRIVER_STEM_OO}
  NOEXEPREFIX
  NAME ${FORTRAN_DRIVER_STEM_OO}
  ARGS "-f ${FORTRAN_DRIVER_STEM_OO}.mpitests"
  COMM mpi
  NUM_MPI_PROCS 3
  PASS_REGULAR_EXPRESSION "END RESULT: ALL TESTS PASSED"
  )

ENDIF()

######################################################################
######################################################################

ENDFUNCTION()
