IF (FIND_ONE_IN_SET_OF_HEADERS_1)
  SET(REQUIRED_HEADERS
    "MissingHeader1.hpp HeadersAndLibsTpl_header1.hpp"
    "MissingHeader2.hpp HeadersAndLibsTpl_header2.hpp"
    )
ELSE()
  SET(REQUIRED_HEADERS HeadersAndLibsTpl_header1.hpp HeadersAndLibsTpl_header2.hpp)
ENDIF()

IF (FIND_ONE_IN_SET_OF_LIBS_1)
  SET(REQUIRED_LIBS_NAMES
    "missinglib1 haltpl1"
    "missinglib2 haltpl2"
    )
ELSE()
  SET(REQUIRED_LIBS_NAMES  haltpl1  haltpl2)
ENDIF()

IF (NOT NOT_MUST_FIND_ALL_HEADERS)
  SET(MUST_FIND_ALL_HEADERS_ARG  MUST_FIND_ALL_HEADERS)
ENDIF()

IF (NOT NOT_MUST_FIND_ALL_LIBS)
  SET(MUST_FIND_ALL_LIBS_ARG  MUST_FIND_ALL_LIBS)
ENDIF()

TRIBITS_TPL_FIND_INCLUDE_DIRS_AND_LIBRARIES( HeadersAndLibsTpl
  REQUIRED_HEADERS  ${REQUIRED_HEADERS}
  ${MUST_FIND_ALL_HEADERS_ARG}
  REQUIRED_LIBS_NAMES  ${REQUIRED_LIBS_NAMES}
  ${MUST_FIND_ALL_LIBS_ARG}
  )
