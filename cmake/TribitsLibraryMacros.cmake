INCLUDE(ParseVariableArguments)
INCLUDE(AppendSet)
include(CreateLibtoolFile)
include(AddLibtoolLibrary)


FUNCTION(TRIBITS_ADD_LIBRARY LIBRARY_NAME)

  PARSE_ARGUMENTS(
    PARSE #prefix
    "HEADERS;NOINSTALLHEADERS;SOURCES;DEPLIBS;IMPORTEDLIBS;DEFINES" # Lists
    "TESTONLY;NO_INSTALL_LIB_OR_HEADERS;CUDALIBRARY" #Options
    ${ARGN} # Remaining arguments passed in
    )

    ADD_LIBTOOL_LIBRARY(NAME ${LIBRARY_NAME} SRC_LIST ${PARSE_SOURCES}  )

ENDFUNCTION()
