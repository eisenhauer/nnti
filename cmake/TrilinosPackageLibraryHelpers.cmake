INCLUDE(Parse_Variable_Arguments)


# These macros are used to help setup up libraries for a Trilinos package.
#
# NOTE: These last two macros communicate through the variables
# PRIVATE_PACKAGE_TARGET_LIBS and PRIVATE_PACKAGE_LIB_DEPS so it is critical
# that the package not try to use these variables for its own use.


MACRO(TRILINOS_PACKAGE_CONFIGURE_FILE PACKAGE_NAME_CONFIG_FILE)
  CONFIGURE_FILE(
    ${${PROJECT_NAME}_SOURCE_DIR}/cmake/${PACKAGE_NAME_CONFIG_FILE}.in 
    ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME_CONFIG_FILE}
    )
  INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PACKAGE_NAME_CONFIG_FILE}
    DESTINATION ${TRILINOS_INSTALL_LIB_INCLUDE_DIR}
    COMPONENT ${PROJECT_NAME}
    )
ENDMACRO()


MACRO(TRILINOS_PACKAGE_ADD_LIBRARY LIBRARY_NAME)

  PARSE_ARGUMENTS(
    PARSE #prefix
    "HEADERS;SOURCES;DEPLIBS" # Lists
    "" #Options
    ${ARGN} # Remaining arguments passed in
    )

  ADD_LIBRARY(${LIBRARY_NAME} ${PARSE_HEADERS} ${PARSE_SOURCES})
  LIST(APPEND PRIVATE_PACKAGE_TARGET_LIBS ${LIBRARY_NAME})
  LIST(APPEND PRIVATE_PACKAGE_LIB_DEPS ${PARSE_DEPLIBS})
  TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${PRIVATE_PACKAGE_LIB_DEPS})

  IF(TRILINOS_ENABLE_MPI)
    LIST(APPEND PRIVATE_PACKAGE_LIB_DEPS ${MPI_LIBRARY} ${MPI_EXTRA_LIBRARY})
    INCLUDE_DIRECTORIES(${MPI_INCLUDE_DIRS})
    TARGET_LINK_LIBRARIES(${LIBRARY_NAME} ${PRIVATE_PACKAGE_LIB_DEPS})
  ENDIF()
  
  INSTALL(
    TARGETS ${LIBRARY_NAME}
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
    COMPONENT ${PROJECT_NAME}
    )
  
  INSTALL(
    FILES ${PARSE_HEADERS}
    DESTINATION ${TRILINOS_INSTALL_INCLUDE_DIR}
    COMPONENT ${PROJECT_NAME}
    )

ENDMACRO()


MACRO(TRILINOS_PACKAGE_EXPORT_DEPENDENCY_VARIABLES)
  GET_DIRECTORY_PROPERTY(INCLUDE_DIRS_CURRENT INCLUDE_DIRECTORIES)
  SET(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS}
    ${INCLUDE_DIRS_CURRENT} CACHE INTERNAL "")
  GET_DIRECTORY_PROPERTY(LIBRARY_DIRS_CURRENT LINK_DIRECTORIES)
  SET(${PROJECT_NAME}_LIBRARY_DIRS ${${PROJECT_NAME}_LIBRARY_DIRS}
    ${LIBRARY_DIRS_CURRENT} CACHE INTERNAL "")
  SET(${PROJECT_NAME}_LIBRARIES ${${PROJECT_NAME}_LIBRARIES} ${PRIVATE_PACKAGE_TARGET_LIBS}
    ${PRIVATE_PACKAGE_LIB_DEPS} CACHE INTERNAL "")
ENDMACRO()
