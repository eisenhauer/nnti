
INCLUDE(Trilinos_Add_Option)
INCLUDE(TrilinosHelpers)

#
# 2008/10/06: rabartl:
#
# Below, we change the value of user cache values like
# Trilinos_ENABLE_${PACKAGE_NAME}, ${PACKAGE_NAME}_ENABLE_TESTS, and
# ${PACKAGE_NAME}_ENABLE_EXAMPLES by just setting them to regular variables
# instead of putting them back on the cache.  That means that they are used as
# global varibles but we don't want to disturb the cache since that would
# change the behavior for future invocations of cmake.  Because of this, these
# macros must all be called from the top-level Trilinos CMakeLists.txt file
# and macros must call macros as not to change the variable scope.
#
# I had to do it this way in order to be able to get the right behavior which
# is:
#
# 1) Override the value of these variables in all CMake processing
#
# 2) Avoid changing the user cache values because that would be confusing and
# would make it hard to change package enables/disable later without blowing
# away the cache
# 

#
# Macro that gets the current list of enables packages
#
# Accesses the global varaibles:
#
#   Trilinos_PACKAGES
#   Trilinos_ENABLE_${PACKAGE}
#
# where ${PACKAGE} is every package in TRILINOS_PACKAGES_IN
#

MACRO(TRILINOS_GET_ENABLED_PACKAGES_LIST TRILINOS_ENABLED_PACKAGES_OUT NUM_ENABLED_OUT)
  SET(${TRILINOS_ENABLED_PACKAGES_OUT} "")
  SET(${NUM_ENABLED_OUT} 0)
  FOREACH(PACKAGE ${Trilinos_PACKAGES})
    ASSERT_DEFINED(Trilinos_ENABLE_${PACKAGE})
    IF (Trilinos_ENABLE_${PACKAGE})
      SET(${TRILINOS_ENABLED_PACKAGES_OUT} "${${TRILINOS_ENABLED_PACKAGES_OUT}} ${PACKAGE}")
      MATH(EXPR ${NUM_ENABLED_OUT} "${${NUM_ENABLED_OUT}}+1")
    ENDIF()
  ENDFOREACH()
ENDMACRO()


#
# Function that prints the current set of packages
#
FUNCTION(TRILINOS_PRINT_ENABLED_PACKAGE_LIST DOCSTRING)
  TRILINOS_GET_ENABLED_PACKAGES_LIST(Trilinos_ENABLED_PACKAGES NUM_ENABLED)
  MESSAGE("${DOCSTRING}: ${Trilinos_ENABLED_PACKAGES} ${NUM_ENABLED}")
ENDFUNCTION()


#
# Function that sets a varaible to DECLARED-UNDEFINED
#

FUNCTION(DECLARE_UNDEFINED VAR)
  SET(${VAR} DECLARED-UNDEFINED PARENT_SCOPE)
ENDFUNCTION()


#
# Function that asserts that a package dependency variable is defined
# correctly
#

FUNCTION(ASSERT_DEFINED_PACKAGE_VAR PACKAGE_VAR PACKAGE_NAME)
  IF (${PACKAGE_VAR} STREQUAL DECLARED-UNDEFINED)
    MESSAGE(FATAL_ERROR
      "Error, the package variable ${PACKAGE_VAR} was not defined correctly for package ${PACKAGE_NAME}!"
      )
  ENDIF()
ENDFUNCTION()


#
# Macro that reads in package dependencies for a package and sets forward
# dependencies for packages already read in.
#
# Modifies the global varibles:
#
#   ${PACKAGE_NAME}_LIB_REQUIRED_DEP_PACKAGES
#   ${PACKAGE_NAME}_LIB_OPTIONAL_DEP_PACKAGES
#   ${PACKAGE_NAME}_TEST_REQUIRED_DEP_PACKAGES
#   ${PACKAGE_NAME}_TEST_OPTIONAL_DEP_PACKAGES
#   ${PACKAGE_NAME}_FORWARD_LIB_REQUIRED_DEP_PACKAGES
#   ${PACKAGE_NAME}_FORWARD_LIB_OPTIONAL_DEP_PACKAGES
#   ${PACKAGE_NAME}_FORWARD_TEST_REQUIRED_DEP_PACKAGES
#   ${PACKAGE_NAME}_FORWARD_TEST_OPTIONAL_DEP_PACKAGES
#

MACRO(TRILINOS_READ_PACKAGE_DEPENDENCIES PACKAGE_NAME PACKAGE_DIR)

  SET(${PACKAGE_NAME}_FORWARD_LIB_REQUIRED_DEP_PACKAGES "")
  SET(${PACKAGE_NAME}_FORWARD_LIB_OPTIONAL_DEP_PACKAGES "")
  SET(${PACKAGE_NAME}_FORWARD_TEST_REQUIRED_DEP_PACKAGES "")
  SET(${PACKAGE_NAME}_FORWARD_TEST_OPTIONAL_DEP_PACKAGES "")

  DECLARE_UNDEFINED(LIB_REQUIRED_DEP_PACKAGES)
  DECLARE_UNDEFINED(LIB_OPTIONAL_DEP_PACKAGES)
  DECLARE_UNDEFINED(TEST_REQUIRED_DEP_PACKAGES)
  DECLARE_UNDEFINED(TEST_OPTIONAL_DEP_PACKAGES)

  INCLUDE(packages/${PACKAGE_DIR}/cmake/Dependencies.cmake)

  ASSERT_DEFINED_PACKAGE_VAR(LIB_REQUIRED_DEP_PACKAGES ${PACKAGE_NAME})
  ASSERT_DEFINED_PACKAGE_VAR(LIB_OPTIONAL_DEP_PACKAGES ${PACKAGE_NAME})
  ASSERT_DEFINED_PACKAGE_VAR(TEST_REQUIRED_DEP_PACKAGES ${PACKAGE_NAME})
  ASSERT_DEFINED_PACKAGE_VAR(TEST_OPTIONAL_DEP_PACKAGES ${PACKAGE_NAME})

  SET(${PACKAGE_NAME}_LIB_REQUIRED_DEP_PACKAGES ${LIB_REQUIRED_DEP_PACKAGES})
  SET(${PACKAGE_NAME}_LIB_OPTIONAL_DEP_PACKAGES ${LIB_OPTIONAL_DEP_PACKAGES})
  SET(${PACKAGE_NAME}_TEST_REQUIRED_DEP_PACKAGES ${TEST_REQUIRED_DEP_PACKAGES})
  SET(${PACKAGE_NAME}_TEST_OPTIONAL_DEP_PACKAGES ${TEST_OPTIONAL_DEP_PACKAGES})

  TRILINOS_APPEND_FORWARD_DEP_PACKAGES(${PACKAGE_NAME} LIB_REQUIRED_DEP_PACKAGES)
  TRILINOS_APPEND_FORWARD_DEP_PACKAGES(${PACKAGE_NAME} LIB_OPTIONAL_DEP_PACKAGES)
  TRILINOS_APPEND_FORWARD_DEP_PACKAGES(${PACKAGE_NAME} TEST_REQUIRED_DEP_PACKAGES)
  TRILINOS_APPEND_FORWARD_DEP_PACKAGES(${PACKAGE_NAME} TEST_OPTIONAL_DEP_PACKAGES)

  # 2008/10/10: rabartl: ToDo: Above, we must put in special logic to make sure
  # that a package defines its linkage varibles correctly in their Dependencies.cmake
  # file!

ENDMACRO()


#
# Macro that prints out dependencies for a package
#
# Does not modify the global state.
#

MACRO(TRILINOS_PRINT_PACKAGE_DEPENDENCIES PACKAGE_NAME)

  PRINT_NONEMPTY_VAR(${PACKAGE_NAME}_LIB_REQUIRED_DEP_PACKAGES)
  PRINT_NONEMPTY_VAR(${PACKAGE_NAME}_LIB_OPTIONAL_DEP_PACKAGES)
  PRINT_NONEMPTY_VAR(${PACKAGE_NAME}_TEST_REQUIRED_DEP_PACKAGES)
  PRINT_NONEMPTY_VAR(${PACKAGE_NAME}_TEST_OPTIONAL_DEP_PACKAGES)

  PRINT_NONEMPTY_VAR(${PACKAGE_NAME}_FORWARD_LIB_REQUIRED_DEP_PACKAGES)
  PRINT_NONEMPTY_VAR(${PACKAGE_NAME}_FORWARD_LIB_OPTIONAL_DEP_PACKAGES)
  PRINT_NONEMPTY_VAR(${PACKAGE_NAME}_FORWARD_TEST_REQUIRED_DEP_PACKAGES)
  PRINT_NONEMPTY_VAR(${PACKAGE_NAME}_FORWARD_TEST_OPTIONAL_DEP_PACKAGES)

ENDMACRO()


#
# Macro that sets up standard user options for each package
#

MACRO(TRILINOS_INSERT_STANDARD_PACKAGE_OPTIONS PACKAGE_NAME)

  #MESSAGE("TRILINOS_INSERT_STANDARD_PACKAGE_OPTIONS: ${PACKAGE_NAME}")

  SET( Trilinos_ENABLE_${PACKAGE_NAME} "" CACHE STRING
    "Enable the ${PACKAGE_NAME} package.  Set to 'ON', 'OFF', or leave empty to allow for other logic to decide.")
  SET( ${PACKAGE_NAME}_ENABLE_TESTS "" CACHE STRING
    "Build ${PACKAGE_NAME} tests.  Set to 'ON', 'OFF', or leave empty to allow for other logic to decide." )
  SET( ${PACKAGE_NAME}_ENABLE_EXAMPLES "" CACHE STRING
    "Build ${PACKAGE} examples.  Set to 'ON', 'OFF', or leave empty to allow for other logic to decide." )

ENDMACRO()


#
# Macro that helps to set up forward package dependency lists
#

FUNCTION(TRILINOS_APPEND_FORWARD_DEP_PACKAGES PACKAGE_NAME LIST_TYPE)

  SET(DEP_PKG_LIST_NAME "${PACKAGE_NAME}_${LIST_TYPE}")

  #MESSAGE("DEP_PKG_LIST_NAME = ${DEP_PKG_LIST_NAME}")
  #MESSAGE("${DEP_PKG_LIST_NAME} = ${${DEP_PKG_LIST_NAME}}")

  FOREACH(DEP_PKG ${${DEP_PKG_LIST_NAME}})
    #MESSAGE("DEP_PKG = ${DEP_PKG}")
    SET(FWD_DEP_PKG_LIST_NAME "${DEP_PKG}_FORWARD_${LIST_TYPE}")
    #MESSAGE("FWD_DEP_PKG_LIST_NAME = ${FWD_DEP_PKG_LIST_NAME}")
    IF (NOT DEFINED ${FWD_DEP_PKG_LIST_NAME})
      MESSAGE(FATAL_ERROR "Error, the package '${DEP_PKG}' is listed as a dependency of the package '${PACKAGE_NAME}' in the list '${DEP_PKG_LIST_NAME}' but the package '${DEP_PKG}' is listed later in the package order.  We do not allow circular package dependencies!")
    ENDIF()
    SET(${FWD_DEP_PKG_LIST_NAME} ${${FWD_DEP_PKG_LIST_NAME}} ${PACKAGE_NAME} PARENT_SCOPE)
  ENDFOREACH()

ENDFUNCTION()


#
# Private helper macro
#

MACRO(TRILINOS_PRIVATE_ADD_OPTIONAL_PACKAGE_ENABLE PACKAGE_NAME OPTIONAL_DEP_PACKAGE)

  SET( ${PACKAGE_NAME}_ENABLE_${OPTIONAL_DEP_PACKAGE} "" CACHE STRING
    "Enable optional support for ${OPTIONAL_DEP_PACKAGE} in ${PACKAGE_NAME}.  Set to 'ON', 'OFF', or leave empty to allow for other logic to decide" )

ENDMACRO()


#
# Macro that enables optional package interdependencies dependancies
#

MACRO(TRILINOS_ADD_OPTIONAL_PACKAGE_ENABLES PACKAGE_NAME)

  #MESSAGE("\nTRILINOS_ADD_OPTIONAL_PACKAGE_ENABLES: ${PACKAGE_NAME}")

  FOREACH(OPTIONAL_DEP_PACKAGE ${${PACKAGE_NAME}_LIB_OPTIONAL_DEP_PACKAGES})
    TRILINOS_PRIVATE_ADD_OPTIONAL_PACKAGE_ENABLE(
      ${PACKAGE_NAME} ${OPTIONAL_DEP_PACKAGE} )
  ENDFOREACH()

  FOREACH(OPTIONAL_DEP_PACKAGE ${${PACKAGE_NAME}_TEST_OPTIONAL_DEP_PACKAGES})
    TRILINOS_PRIVATE_ADD_OPTIONAL_PACKAGE_ENABLE(
      ${PACKAGE_NAME} ${OPTIONAL_DEP_PACKAGE} )
  ENDFOREACH()

ENDMACRO()


#
# Private helper macro
#

MACRO(TRILINOS_PRIVATE_DISABLE_REQUIRED_PACKAGE_ENABLES FORWARD_DEP_PACKAGE_NAME PACKAGE_NAME LIBRARY_DEP)

  #MESSAGE("TRILINOS_PRIVATE_DISABLE_REQUIRED_PACKAGE_ENABLES ${FORWARD_DEP_PACKAGE_NAME} ${LIBRARY_DEP}")  

  IF ("${LIBRARY_DEP}" STREQUAL "TRUE")
    SET(DEP_TYPE_STR "library")
    ASSERT_DEFINED(Trilinos_ENABLE_${FORWARD_DEP_PACKAGE_NAME})
    IF (Trilinos_ENABLE_${FORWARD_DEP_PACKAGE_NAME} OR Trilinos_ENABLE_${FORWARD_DEP_PACKAGE_NAME} STREQUAL "")
      MESSAGE(STATUS
        "Setting Trilinos_ENABLE_${FORWARD_DEP_PACKAGE_NAME}=OFF because ${FORWARD_DEP_PACKAGE_NAME} has a required ${DEP_TYPE_STR} dependence on disabled package ${PACKAGE_NAME}")
      SET(Trilinos_ENABLE_${FORWARD_DEP_PACKAGE_NAME} OFF)
    ENDIF()
  ELSE()
    SET(DEP_TYPE_STR "test/example")
  ENDIF()

  ASSERT_DEFINED(${FORWARD_DEP_PACKAGE_NAME}_ENABLE_TESTS)
  IF (${FORWARD_DEP_PACKAGE_NAME}_ENABLE_TESTS OR ${FORWARD_DEP_PACKAGE_NAME}_ENABLE_TESTS STREQUAL "")
    MESSAGE(STATUS
      "Setting ${FORWARD_DEP_PACKAGE_NAME}_ENABLE_TESTS=OFF because ${FORWARD_DEP_PACKAGE_NAME} has a required ${DEP_TYPE_STR} dependence on disabled package ${PACKAGE_NAME}")
    SET(${FORWARD_DEP_PACKAGE_NAME}_ENABLE_TESTS OFF)
  ENDIF()

  ASSERT_DEFINED(${FORWARD_DEP_PACKAGE_NAME}_ENABLE_EXAMPLES)
  IF (${FORWARD_DEP_PACKAGE_NAME}_ENABLE_EXAMPLES OR ${FORWARD_DEP_PACKAGE_NAME}_ENABLE_EXAMPLES STREQUAL "")
    MESSAGE(STATUS
      "Setting ${FORWARD_DEP_PACKAGE_NAME}_ENABLE_EXAMPLES=OFF because ${FORWARD_DEP_PACKAGE_NAME} has a required ${DEP_TYPE_STR} dependence on disabled package ${PACKAGE_NAME}")
    SET(${FORWARD_DEP_PACKAGE_NAME}_ENABLE_EXAMPLES OFF)
  ENDIF()

ENDMACRO()


#
# Private helper macro
#

MACRO(TRILINOS_PRIVATE_DISABLE_OPTIONAL_PACKAGE_ENABLES FORWARD_DEP_PACKAGE_NAME PACKAGE_NAME)

  #MESSAGE("TRILINOS_PRIVATE_DISABLE_OPTIONAL_PACKAGE_ENABLES ${FORWARD_DEP_PACKAGE_NAME} ${PACKAGE_NAME}")  

  ASSERT_DEFINED(${FORWARD_DEP_PACKAGE_NAME}_ENABLE_${PACKAGE_NAME})
  IF (${FORWARD_DEP_PACKAGE_NAME}_ENABLE_${PACKAGE_NAME} OR ${FORWARD_DEP_PACKAGE_NAME}_ENABLE_${PACKAGE_NAME} STREQUAL "")
    MESSAGE(STATUS
      "Setting ${FORWARD_DEP_PACKAGE_NAME}_ENABLE_${PACKAGE_NAME}=OFF because ${FORWARD_DEP_PACKAGE_NAME} has an optional library dependence on disabled package ${PACKAGE_NAME}")
    SET(${FORWARD_DEP_PACKAGE_NAME}_ENABLE_${PACKAGE_NAME} OFF)
  ENDIF()

ENDMACRO()


#
# Function that disables all forward packages recurrsively
#

MACRO(TRILINOS_DISABLE_FORWARD_REQUIRED_DEP_PACKAGES PACKAGE_NAME)

  #MESSAGE("TRILINOS_DISABLE_FORWARD_REQUIRED_DEP_PACKAGES: ${PACKAGE_NAME}")

  IF ("${Trilinos_ENABLE_${PACKAGE}}" STREQUAL "OFF")

    FOREACH(FWD_DEP_PKG ${${PACKAGE_NAME}_FORWARD_LIB_REQUIRED_DEP_PACKAGES})
      TRILINOS_PRIVATE_DISABLE_REQUIRED_PACKAGE_ENABLES(${FWD_DEP_PKG} ${PACKAGE_NAME} TRUE)
    ENDFOREACH()

    FOREACH(FWD_DEP_PKG ${${PACKAGE_NAME}_FORWARD_LIB_OPTIONAL_DEP_PACKAGES})
      TRILINOS_PRIVATE_DISABLE_OPTIONAL_PACKAGE_ENABLES(${FWD_DEP_PKG} ${PACKAGE_NAME})
    ENDFOREACH()

    FOREACH(FWD_DEP_PKG ${${PACKAGE_NAME}_FORWARD_TEST_REQUIRED_DEP_PACKAGES})
      TRILINOS_PRIVATE_DISABLE_REQUIRED_PACKAGE_ENABLES(${FWD_DEP_PKG} ${PACKAGE_NAME} FALSE)
    ENDFOREACH()

  ENDIF()

ENDMACRO()


#
# Private helper macro
#

MACRO(TRILINOS_PRIVATE_POSTPROCESS_OPTIONAL_PACKAGE_ENABLE PACKAGE_NAME OPTIONAL_DEP_PACKAGE)

  #MESSAGE("TRILINOS_PRIVATE_POSTPROCESS_OPTIONAL_PACKAGE_ENABLE: ${PACKAGE_NAME} ${OPTIONAL_DEP_PACKAGE}")

  IF("${${PACKAGE_NAME}_ENABLE_${OPTIONAL_DEP_PACKAGE}}" STREQUAL "")
    IF(Trilinos_ENABLE_${PACKAGE_NAME} AND Trilinos_ENABLE_${OPTIONAL_DEP_PACKAGE})
      MESSAGE(STATUS "Setting ${PACKAGE_NAME}_ENABLE_${OPTIONAL_DEP_PACKAGE}=ON since Trilinos_ENABLE_${PACKAGE_NAME}=ON AND Trilinos_ENABLE_${OPTIONAL_DEP_PACKAGE}=ON")
      SET(${PACKAGE_NAME}_ENABLE_${OPTIONAL_DEP_PACKAGE} ON)
    ENDIF()
  ENDIF()

  STRING(TOUPPER ${PACKAGE_NAME} PACKAGE_NAME_UPPER)
  STRING(TOUPPER ${OPTIONAL_DEP_PACKAGE} OPTIONAL_DEP_PACKAGE_UPPER)
  SET(MACRO_DEFINE_NAME HAVE_${PACKAGE_NAME_UPPER}_${OPTIONAL_DEP_PACKAGE_UPPER})

  IF(${PACKAGE_NAME}_ENABLE_${OPTIONAL_DEP_PACKAGE})
    SET(${MACRO_DEFINE_NAME} ON)
  ELSE()
    SET(${MACRO_DEFINE_NAME} OFF)
  ENDIF()

ENDMACRO()


#
# Macro that post-processes optional dependancies after all other
# dependencies have been worked out
#

MACRO(TRILINOS_POSTPROCESS_OPTIONAL_PACKAGE_ENABLES PACKAGE_NAME)

  #MESSAGE("\nTRILINOS_ADD_OPTIONAL_PACKAGE_ENABLES: ${PACKAGE_NAME}")

  FOREACH(OPTIONAL_DEP_PACKAGE ${${PACKAGE_NAME}_LIB_OPTIONAL_DEP_PACKAGES})
    TRILINOS_PRIVATE_POSTPROCESS_OPTIONAL_PACKAGE_ENABLE(
      ${PACKAGE_NAME} ${OPTIONAL_DEP_PACKAGE} )
  ENDFOREACH()

  FOREACH(OPTIONAL_DEP_PACKAGE ${${PACKAGE_NAME}_TEST_OPTIONAL_DEP_PACKAGES})
    TRILINOS_PRIVATE_POSTPROCESS_OPTIONAL_PACKAGE_ENABLE(
      ${PACKAGE_NAME} ${OPTIONAL_DEP_PACKAGE} )
  ENDFOREACH()

ENDMACRO()


#
# Set an individual pacakge variable based on the global value
#

MACRO(TRILINOS_POSTPROCESS_STANDARD_PACKAGE_VARIABLE TRILINOS_VAR PACKAGE_VAR)

  IF (Trilinos_VERBOSE_CONFIGURE)
    MESSAGE("")
    MESSAGE("TRILINOS_POSTPROCESS_STANDARD_PACKAGE_VARIABLE:")
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
    #MESSAGE(STATUS "PACKAGE_VAR NOT DEFAULT")
  ENDIF()

  IF (Trilinos_VERBOSE_CONFIGURE)
    MESSAGE(STATUS "${PACKAGE_VAR} = ${${PACKAGE_VAR}}")
  ENDIF()

ENDMACRO()


#
# Macro used to set Trilinos_ENABLE_${PACKAGE_NAME} based on
# Trilinos_ENABLE_ALL_PACKAGES
#

MACRO(TRILINOS_APPLY_ALL_PACKAGE_ENABLES PACKAGE_NAME)

  TRILINOS_POSTPROCESS_STANDARD_PACKAGE_VARIABLE(
    Trilinos_ENABLE_ALL_PACKAGES Trilinos_ENABLE_${PACKAGE_NAME} )

ENDMACRO()


#
# Macro used to set ${PACKAGE)_ENABLE_TESTS and ${PACKAGE)_ENABLE_EXAMPLES
# based on Trilinos_ENABLE_ALL_PACKAGES
#

MACRO(TRILINOS_APPLY_TEST_EXAMPLE_ENABLES PACKAGE_NAME)

  IF (Trilinos_ENABLE_${PACKAGE_NAME})

    TRILINOS_POSTPROCESS_STANDARD_PACKAGE_VARIABLE(
      Trilinos_ENABLE_TESTS ${PACKAGE_NAME}_ENABLE_TESTS )

    TRILINOS_POSTPROCESS_STANDARD_PACKAGE_VARIABLE(
      Trilinos_ENABLE_EXAMPLES ${PACKAGE_NAME}_ENABLE_EXAMPLES )

  ENDIF()

ENDMACRO()


#
# Private helper macro
#

MACRO(TRILINOS_PRIVATE_ENABLE_FORWARD_PACKAGE FORWARD_DEP_PACKAGE_NAME PACKAGE_NAME)
  # Enable the forward package if it is not already set to ON or OFF
  ASSERT_DEFINED(Trilinos_ENABLE_${FORWARD_DEP_PACKAGE_NAME})
  IF(Trilinos_ENABLE_${FORWARD_DEP_PACKAGE_NAME} STREQUAL "")
    MESSAGE(STATUS "Setting Trilinos_ENABLE_${FORWARD_DEP_PACKAGE_NAME}=ON because Trilinos_ENABLE_${PACKAGE_NAME}=ON")
    ASSERT_DEFINED(Trilinos_ENABLE_${FORWARD_DEP_PACKAGE_NAME})
    SET(Trilinos_ENABLE_${FORWARD_DEP_PACKAGE_NAME} ON)
  ENDIF()
ENDMACRO()


#
# Macro used to set Trilinos_ENABLE_${FWD_PACKAGE_NAME)=ON for all optional
# and required forward dependencies of the package ${PACKAGE_NAME}
#

MACRO(TRILINOS_ENABLE_FORWARD_PACKAGE_ENABLES PACKAGE_NAME)

  #MESSAGE("\nTRILINOS_ENABLE_FORWARD_PACKAGE_ENABLES ${PACKAGE_NAME}")
  #MESSAGE(STATUS "Trilinos_ENABLE_${PACKAGE_NAME}=${Trilinos_ENABLE_${PACKAGE_NAME}}")

  # Enable the forward packages if this package is enabled
  ASSERT_DEFINED(Trilinos_ENABLE_${PACKAGE_NAME})
  IF (Trilinos_ENABLE_${PACKAGE_NAME})

    FOREACH(FWD_DEP_PKG ${${PACKAGE_NAME}_FORWARD_LIB_REQUIRED_DEP_PACKAGES})
      TRILINOS_PRIVATE_ENABLE_FORWARD_PACKAGE(${FWD_DEP_PKG} ${PACKAGE_NAME})
    ENDFOREACH()

    FOREACH(FWD_DEP_PKG ${${PACKAGE_NAME}_FORWARD_LIB_OPTIONAL_DEP_PACKAGES})
      TRILINOS_PRIVATE_ENABLE_FORWARD_PACKAGE(${FWD_DEP_PKG} ${PACKAGE_NAME})
    ENDFOREACH()

  ENDIF()

ENDMACRO()


#
# Private helper macro
#

MACRO(TRILINOS_PRIVATE_ENABLE_DEP_PACKAGE PACKAGE_NAME DEP_PACKAGE_NAME)
  ASSERT_DEFINED(Trilinos_ENABLE_${DEP_PACKAGE_NAME})
  IF(Trilinos_ENABLE_${DEP_PACKAGE_NAME} STREQUAL "")
    MESSAGE(STATUS "Setting Trilinos_ENABLE_${DEP_PACKAGE_NAME}=ON because Trilinos_ENABLE_${PACKAGE_NAME}=ON")
    ASSERT_DEFINED(Trilinos_ENABLE_${DEP_PACKAGE_NAME})
    SET(Trilinos_ENABLE_${DEP_PACKAGE_NAME} ON)
  ENDIF()
ENDMACRO()

#
# Macro that sets the optional packages for given package
#

MACRO(TRILINOS_ENABLE_OPTIONAL_PACKAGES PACKAGE_NAME)

  #MESSAGE("TRILINOS_ENABLE_OPTIONAL_PACKAGE_ENABLES: ${PACKAGE_NAME}")
  #MESSAGE(STATUS "Trilinos_ENABLE_${PACKAGE_NAME}=${Trilinos_ENABLE_${PACKAGE_NAME}}")

  ASSERT_DEFINED(Trilinos_ENABLE_${PACKAGE_NAME})

  IF (Trilinos_ENABLE_${PACKAGE_NAME})

    FOREACH(DEP_PKG ${${PACKAGE_NAME}_LIB_OPTIONAL_DEP_PACKAGES})
      TRILINOS_PRIVATE_ENABLE_DEP_PACKAGE(${PACKAGE_NAME} ${DEP_PKG})
    ENDFOREACH()

    FOREACH(DEP_PKG ${${PACKAGE_NAME}_TEST_OPTIONAL_DEP_PACKAGES})
      TRILINOS_PRIVATE_ENABLE_DEP_PACKAGE(${PACKAGE_NAME} ${DEP_PKG})
    ENDFOREACH()

  ENDIF()

ENDMACRO()


#
# Macro that sets the required packages for given package
#

MACRO(TRILINOS_ENABLE_REQUIRED_PACKAGES PACKAGE_NAME)

  #MESSAGE("TRILINOS_ENABLE_REQUIRED_PACKAGE_ENABLES: ${PACKAGE_NAME}")
  #MESSAGE(STATUS "Trilinos_ENABLE_${PACKAGE_NAME}=${Trilinos_ENABLE_${PACKAGE_NAME}}")

  ASSERT_DEFINED(Trilinos_ENABLE_${PACKAGE_NAME})

  IF (Trilinos_ENABLE_${PACKAGE_NAME})

    FOREACH(DEP_PKG ${${PACKAGE_NAME}_LIB_REQUIRED_DEP_PACKAGES})
      TRILINOS_PRIVATE_ENABLE_DEP_PACKAGE(${PACKAGE_NAME} ${DEP_PKG})
    ENDFOREACH()

    FOREACH(DEP_PKG ${${PACKAGE_NAME}_TEST_REQUIRED_DEP_PACKAGES})
      TRILINOS_PRIVATE_ENABLE_DEP_PACKAGE(${PACKAGE_NAME} ${DEP_PKG})
    ENDFOREACH()

  ENDIF()

ENDMACRO()


#
# Macro that adjusts all of the package enables from what the user input
# to the final set that will be used to enable packages
#

MACRO(TRILNOS_ADJUST_PACKAGE_ENABLES)

  MESSAGE("")
  MESSAGE("Disabling forward pacakges that have a required dependancy on explicitly disabled packages ...")
  MESSAGE("")
  FOREACH(PACKAGE ${Trilinos_PACKAGES})
    TRILINOS_DISABLE_FORWARD_REQUIRED_DEP_PACKAGES(${PACKAGE})
  ENDFOREACH()
  
  IF (Trilinos_ENABLE_ALL_PACKAGES)
    MESSAGE("")
    MESSAGE("Enabling all packages that are not currently disabled ...")
    MESSAGE("")
    FOREACH(PACKAGE ${Trilinos_PACKAGES})
      TRILINOS_APPLY_ALL_PACKAGE_ENABLES(${PACKAGE})
    ENDFOREACH()
  ENDIF()
  
  IF (Trilinos_ENABLE_ALL_FORWARD_DEP_PACKAGES)
    MESSAGE("")
    MESSAGE("Enabling all forward dependent packages ...")
    MESSAGE("")
    FOREACH(PACKAGE ${Trilinos_PACKAGES})
      TRILINOS_ENABLE_FORWARD_PACKAGE_ENABLES(${PACKAGE})
    ENDFOREACH()
    SET(Trilinos_ENABLE_ALL_OPTIONAL_PACKAGES ON)
  ENDIF()
  
  IF (Trilinos_ENABLE_TESTS OR Trilinos_ENABLE_EXAMPLES)
    MESSAGE("")
    MESSAGE("Enabling all tests and examples that can be enabled ...")
    MESSAGE("")
    FOREACH(PACKAGE ${Trilinos_PACKAGES})
      TRILINOS_APPLY_TEST_EXAMPLE_ENABLES(${PACKAGE})
    ENDFOREACH()
  ENDIF()
  # NOTE: Above, we enable tests and examples here, before the remaining required
  # packages so that we don't enable tests that don't need to be enabled based
  # on the use of the option Trilinos_ENABLE_ALL_FORWARD_DEP_PACKAGES.
  
  IF (Trilinos_ENABLE_ALL_OPTIONAL_PACKAGES)
    MESSAGE("")
    MESSAGE("Enabling all optional packages for current set of enabled packages ...")
    MESSAGE("")
    FOREACH(PACKAGE ${Trilinos_REVERSE_PACKAGES})
      TRILINOS_ENABLE_OPTIONAL_PACKAGES(${PACKAGE})
    ENDFOREACH()
  ENDIF()
  # NOTE: Above, we have to loop through the packages backward to enable all the
  # pacakges that feed into these packages.
  # NOTE Above, we don't have to enable the required packages because that will
  # come next
  
  MESSAGE("")
  MESSAGE("Enabling all remaining required packages for the current set of enabled packages ...")
  MESSAGE("")
  FOREACH(PACKAGE ${Trilinos_REVERSE_PACKAGES})
    TRILINOS_ENABLE_REQUIRED_PACKAGES(${PACKAGE})
  ENDFOREACH()
  
  MESSAGE("")
  MESSAGE("Enabling all optional intra-package enables that can be if both sets of packages are enabled ...")
  MESSAGE("")
  FOREACH(PACKAGE ${Trilinos_PACKAGES})
    TRILINOS_POSTPROCESS_OPTIONAL_PACKAGE_ENABLES(${PACKAGE})
  ENDFOREACH()

ENDMACRO()
