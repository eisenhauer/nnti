INCLUDE(Parse_Variable_Arguments)
INCLUDE(Global_Set)


# These macros are used to help setup up libraries for a Trilinos package.
#
# NOTE: These last two macros communicate through the variables
# PRIVATE_PACKAGE_TARGET_LIBS and PRIVATE_PACKAGE_LIB_DEPS so it is critical
# that the package not try to use these variables for its own use.


MACRO(TRILINOS_PACKAGE_CONFIGURE_FILE PACKAGE_NAME_CONFIG_FILE)

  IF (Trilinos_VERBOSE_CONFIGURE)
    MESSAGE("\nTRILINOS_PACKAGE_CONFIGURE_FILE: ${PACKAGE_NAME_CONFIG_FILE}")
  ENDIF()

  CONFIGURE_FILE(
    ${PACKAGE_SOURCE_DIR}/cmake/${PACKAGE_NAME_CONFIG_FILE}.in 
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME_CONFIG_FILE}
    )

  INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME_CONFIG_FILE}
    DESTINATION ${TRILINOS_INSTALL_LIB_INCLUDE_DIR}
    COMPONENT ${PACKAGE_NAME}
    )

ENDMACRO()


MACRO(TRILINOS_PRIVATE_ADD_DEP_PACKAGE_INCLUDES_LIBS PACKAGE_NAME DEP_PKG)
  INCLUDE_DIRECTORIES(AFTER ${${DEP_PKG}_INCLUDE_DIRS})
  LIST(APPEND EXTERNAL_LIB_DEPS ${${DEP_PKG}_LIBRARIES})
  LINK_DIRECTORIES(${${DEP_PKG}_LIBRARY_DIRS})
ENDMACRO()


MACRO(TRILINOS_PACKAGE_ADD_LIBRARY LIBRARY_NAME)

  IF (Trilinos_VERBOSE_CONFIGURE)
    MESSAGE("\nTRILINOS_PACKAGE_ADD_LIBRARY: ${LIBRARY_NAME}")
  ENDIF()

  PARSE_ARGUMENTS(
    PARSE #prefix
    "HEADERS;NOINSTALLHEADERS;SOURCES;DEPLIBS" # Lists
    "" #Options
    ${ARGN} # Remaining arguments passed in
    )

  # Add the link directory for this library.

  LINK_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

  # NOTE: Above , this link path not really used here for anything.
  # Instead it is just added to the other set link library directories
  # that are already set.  These link directories are then extracted
  # and stored into stored in ${PACKAGE_NAME}_LIBRARY_DIRS.

  # Add whatever include directories have been defined so far

  INCLUDE_DIRECTORIES(AFTER ${${PACKAGE_NAME}_INCLUDE_DIRS})

  # Add dependent libraries in dependent Trilinos packages

  IF (PARSE_DEPLIBS AND Trilinos_VERBOSE_CONFIGURE)
    MESSAGE(STATUS "DEPLIBS = ${PARSE_DEPLIBS}")
  ENDIF()

  SET(EXTERNAL_LIB_DEPS "")

  FOREACH(DEP_PKG ${${PACKAGE_NAME}_LIB_REQUIRED_DEP_PACKAGES})
    TRILINOS_PRIVATE_ADD_DEP_PACKAGE_INCLUDES_LIBS(${PACKAGE_NAME} ${DEP_PKG})
  ENDFOREACH()

  FOREACH(DEP_PKG ${${PACKAGE_NAME}_LIB_OPTIONAL_DEP_PACKAGES})
    IF (${PACKAGE_NAME}_ENABLE_${DEP_PKG})
      TRILINOS_PRIVATE_ADD_DEP_PACKAGE_INCLUDES_LIBS(${PACKAGE_NAME} ${DEP_PKG})
    ENDIF()
  ENDFOREACH()

  # Add the library and all the dependencies

  ADD_LIBRARY(${LIBRARY_NAME} ${PARSE_HEADERS} ${PARSE_NOINSTALLHEADERS}
    ${PARSE_SOURCES})
  LIST(APPEND PRIVATE_PACKAGE_TARGET_LIBS ${LIBRARY_NAME})
  LIST(APPEND PRIVATE_PACKAGE_LIB_DEPS ${PARSE_DEPLIBS})
  TARGET_LINK_LIBRARIES(${LIBRARY_NAME}
    ${PRIVATE_PACKAGE_LIB_DEPS} ${EXTERNAL_LIB_DEPS})

  IF(Trilinos_ENABLE_MPI)
    LIST(APPEND PRIVATE_PACKAGE_LIB_DEPS ${MPI_LIBRARY} ${MPI_EXTRA_LIBRARY})
    INCLUDE_DIRECTORIES(AFTER ${MPI_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${PRIVATE_PACKAGE_LIB_DEPS})
  ENDIF()
  
  INSTALL(
    TARGETS ${LIBRARY_NAME}
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
    COMPONENT ${PACKAGE_NAME}
    )
  
  INSTALL(
    FILES ${PARSE_HEADERS}
    DESTINATION ${TRILINOS_INSTALL_INCLUDE_DIR}
    COMPONENT ${PACKAGE_NAME}
    )

ENDMACRO()


MACRO(TRILINOS_PACKAGE_EXPORT_DEPENDENCY_VARIABLES)
  GET_DIRECTORY_PROPERTY(INCLUDE_DIRS_CURRENT INCLUDE_DIRECTORIES)
  GLOBAL_SET(${PACKAGE_NAME}_INCLUDE_DIRS ${INCLUDE_DIRS_CURRENT}
    ${${PACKAGE_NAME}_INCLUDE_DIRS})
  GET_DIRECTORY_PROPERTY(LIBRARY_DIRS_CURRENT LINK_DIRECTORIES)
  GLOBAL_SET(${PACKAGE_NAME}_LIBRARY_DIRS ${LIBRARY_DIRS_CURRENT}
    ${${PACKAGE_NAME}_LIBRARY_DIRS})
  GLOBAL_SET(${PACKAGE_NAME}_LIBRARIES ${PRIVATE_PACKAGE_TARGET_LIBS}
    ${${PACKAGE_NAME}_LIBRARIES} ${PRIVATE_PACKAGE_LIB_DEPS})
ENDMACRO()
