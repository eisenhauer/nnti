
INCLUDE(Trilinos_Add_Option)


MACRO(TRILINOS_PRIVATE_ADD_OPTIONAL_PACKAGE_ENABLE PACKAGE_NAME OPTIONAL_DEP_PACKAGE)

  STRING(TOUPPER ${PACKAGE_NAME} PACKAGE_NAME_UPPER)
  STRING(TOUPPER ${OPTIONAL_DEP_PACAKGE} OPTIONAL_DEP_PACAKGE_UPPER)

  TRILINOS_ADD_OPTION(
    ${PACKAGE_NAME}_ENABLE_${OPTIONAL_DEP_PACAKGE}
    HAVE_${PACKAGE_NAME_UPPER}_${OPTIONAL_DEP_PACAKGE_UPPER}
    "Enable optional support in ${PACKAGE_NAME} for ${OPTIONAL_DEP_PACAKGE}"
    ""
    )

ENDMACRO()


#
# Macro that sets up standard user options for each package
#

MACRO(TRILINOS_INSERT_STANDARD_PACKAGE_OPTIONS PACKAGE_NAME)

  #MESSAGE("TRILINOS_INSERT_STANDARD_PACKAGE_OPTIONS: ${PACKAGE_NAME}")

  SET( Trilinos_ENABLE_${PACKAGE_NAME} "" CACHE BOOL
    "Enable the ${PACKAGE_NAME} package.")
  SET( ${PACKAGE_NAME}_ENABLE_TESTS "" CACHE BOOL
    "Build ${PACKAGE_NAME} tests." )
  SET( ${PACKAGE_NAME}_ENABLE_EXAMPLES "" CACHE BOOL
    "Build ${PACKAGE} examples." )

  FOREACH(OPTIONAL_DEP_PACAKGE ${${PACKAGE_NAME}_LIB_OPTIONAL_DEP_PACAKGES})
    TRILINOS_PRIVATE_ADD_OPTIONAL_PACKAGE_ENABLE(${PACKAGE_NAME}
      ${OPTIONAL_DEP_PACAKGE} )
  ENDFOREACH()

  FOREACH(OPTIONAL_DEP_PACAKGE ${${PACKAGE_NAME}_TEST_OPTIONAL_DEP_PACAKGES})
    TRILINOS_PRIVATE_ADD_OPTIONAL_PACKAGE_ENABLE(${PACKAGE_NAME}
      ${OPTIONAL_DEP_PACAKGE} )
  ENDFOREACH()

ENDMACRO()


MACRO(TRILINOS_PRIVATE_SET_OPTIONAL_PACKAGE_ENABLE PACKAGE_NAME OPTIONAL_DEP_PACKAGE)

  IF(${PACKAGE_NAME}_ENABLE_${OPTIONAL_DEP_PACKAGE} STREQUAL "")
    IF(Trilinos_ENABLE_${PACKAGE_NAME} AND Trilinos_ENABLE_${OPTIONAL_DEP_PACKAGE})
      MESSAGE(STATUS "Setting ${PACKAGE_NAME}_ENABLE_${OPTIONAL_DEP_PACKAGE}=ON since Trilinos_ENABLE_${PACKAGE_NAME}=ON AND Trilinos_ENABLE_${OPTIONAL_DEP_PACKAGE}=ON")
      SET(${PACKAGE_NAME}_ENABLE_${OPTIONAL_DEP_PACKAGE} ON)
    ENDIF()
  ENDIF()

ENDMACRO()


#
# Macro that enables optional dependancies
#

MACRO(TRILINOS_SET_OPTIONAL_PACKAGE_ENABLES PACKAGE_NAME)

  FOREACH(OPTIONAL_DEP_PACAKGE ${${PACKAGE_NAME}_LIB_OPTIONAL_DEP_PACAKGES})
    TRILINOS_PRIVATE_SET_OPTIONAL_PACKAGE_ENABLE(${PACKAGE_NAME}
      ${OPTIONAL_DEP_PACAKGE} )
  ENDFOREACH()

  FOREACH(OPTIONAL_DEP_PACAKGE ${${PACKAGE_NAME}_TEST_OPTIONAL_DEP_PACAKGES})
    TRILINOS_PRIVATE_SET_OPTIONAL_PACKAGE_ENABLE(${PACKAGE_NAME}
      ${OPTIONAL_DEP_PACAKGE} )
  ENDFOREACH()

ENDMACRO()


#
# Set an individual pacakge variable based on the global value
#

MACRO(TRILINOS_POSTPROCESS_STANDARD_PACAKGE_VARIABLE TRILINOS_VAR PACKAGE_VAR)

  IF (VERBOSE_CONFIGURE)
    MESSAGE("")
    MESSAGE("TRILINOS_POSTPROCESS_STANDARD_PACAKGE_VARIABLE:")
    MESSAGE(STATUS "${PACKAGE_VAR} = ${${PACKAGE_VAR}}")
    MESSAGE(STATUS "${TRILINOS_VAR} = ${${TRILINOS_VAR}}")
  ENDIF()

  IF (${PACKAGE_VAR} STREQUAL "")
    IF (${TRILINOS_VAR} STREQUAL "ON")
      MESSAGE(STATUS "Setting ${PACKAGE_VAR}=ON")
      SET(${PACKAGE_VAR} ON)
    ELSEIF (TRILINOS_VAR STREQUAL "OFF")
      MESSAGE(STATUS "Setting ${PACKAGE_VAR}=OFF")
      SET(${PACKAGE_VAR} OFF)
    ELSE()
      #MESSAGE(STATUS "ELSE")
      # Otherwise, we will leave it up the the individual package
      # to decide?
    ENDIF()
  ELSE()
    #MESSAGE(STATUS "PACAKGE_VAR NOT DEFAULT")
  ENDIF()

  IF (VERBOSE_CONFIGURE)
    MESSAGE(STATUS "${PACKAGE_VAR} = ${${PACKAGE_VAR}}")
  ENDIF()

ENDMACRO()


MACRO(TRILINOS_APPLY_ALL_PACKAGE_ENABLES PACAKGE)

  TRILINOS_POSTPROCESS_STANDARD_PACAKGE_VARIABLE(
    Trilinos_ENABLE_ALL_PACKAGES Trilinos_ENABLE_${PACKAGE} )

ENDMACRO()


MACRO(TRILINOS_APPLY_TEST_EXAMPLE_EANBLES PACAKGE)

  TRILINOS_POSTPROCESS_STANDARD_PACAKGE_VARIABLE(
    Trilinos_ENABLE_ALL_PACKAGES Trilinos_ENABLE_${PACKAGE} )

  IF (Trilinos_ENABLE_${PACKAGE})

    TRILINOS_POSTPROCESS_STANDARD_PACAKGE_VARIABLE(
      Trilinos_ENABLE_TESTS ${PACKAGE}_ENABLE_TESTS )

    TRILINOS_POSTPROCESS_STANDARD_PACAKGE_VARIABLE(
      Trilinos_ENABLE_EXAMPLES ${PACKAGE}_ENABLE_EXAMPLES )

  ENDIF()

ENDMACRO()
