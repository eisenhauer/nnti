INCLUDE(CMakeBuildTypesList)

INCLUDE(AdvancedSet)
INCLUDE(AssertDefined)
INCLUDE(MultilineSet)
INCLUDE(DualScopeSet)


#
# Macro that just defines the basic flags
#

MACRO(PACKAGE_ARCH_DEFINE_STANDARD_COMPILE_FLAGS_VARS)

  #
  # Setup and general flags
  #

  SET(GENERAL_DEBUG_FLAGS "-g -O0")

  ADVANCED_SET( ${PROJECT_NAME}_ENABLE_CHECKED_STL OFF
    CACHE BOOL "Turn on checked STL checking (e.g. -D_GLIBCXX_DEBUG) or not." )
 
  IF (${PROJECT_NAME}_ENABLE_CHECKED_STL)
    SET(GENERAL_DEBUG_FLAGS "${GENERAL_DEBUG_FLAGS} -D_GLIBCXX_DEBUG")
  ENDIF()
 
  SET(GENERAL_RELEASE_FLAGS "-O3")

  MULTILINE_SET(C_STRONG_COMPILE_WARNING_FLAGS
    " -ansi" # Check for C89 or C++98 standard code
    " -pedantic" # Adds more strick checking to remove non-ANSI GNU extensions
    " -Wall " # Enable a bunch of default warnings
    " -Wno-long-long" # Allow long long int since it is used by MPI, SWIG, etc.
    )
  
  MULTILINE_SET(CXX_STRONG_COMPILE_WARNING_FLAGS
    ${C_STRONG_COMPILE_WARNING_FLAGS}
    " -Wwrite-strings" # Checks for non-const char * copy of string constants
    )

  ADVANCED_SET( ${PROJECT_NAME}_ENABLE_SHADOW_WARNINGS ON
    CACHE BOOL "Turn on shadowing warnings or not" )
  
  IF (${PROJECT_NAME}_ENABLE_SHADOW_WARNINGS)

    MULTILINE_SET(CXX_STRONG_COMPILE_WARNING_FLAGS
      ${CXX_STRONG_COMPILE_WARNING_FLAGS}
      " -Wshadow" # Warn about general shadowing issues
      " -Woverloaded-virtual" # Warn about hiding virtual functions
      )

  ENDIF()

  ADVANCED_SET( ${PROJECT_NAME}_WARNINGS_AS_ERRORS_FLAGS "-Werror"
    CACHE STRING "Flags for treating warnings as errors.  To turn off set to ''")

ENDMACRO()
