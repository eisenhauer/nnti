FUNCTION(WRITE_EXPORT_MAKEFILE Proj EMFName)

  SET(PACK_LIST ${${Proj}_PACKAGES})

  LIST(REVERSE PACK_LIST)

  SET(LIB_LIST)
  SET(TPL_LIB_LIST)
  SET(TPL_INCLUDE_LIST)

  FOREACH(PACK_NAME ${PACK_LIST})

    IF(Trilinos_ENABLE_${PACK_NAME})
  
      LIST(APPEND LIB_LIST ${${PACK_NAME}_LIBRARIES})

      SET(REQ_TPLS ${${PACK_NAME}_LIB_REQUIRED_DEP_TPLS})
      IF(REQ_TPLS)
        LIST(REVERSE REQ_TPLS)
      ENDIF()
      FOREACH(TPL_NAME ${REQ_TPLS})
        LIST(APPEND TPL_LIB_LIST ${TPL_${TPL_NAME}_LIBRARIES})
        LIST(APPEND TPL_INCLUDE_LIST ${TPL_${TPL_NAME}_INCLUDE_DIRS})
      ENDFOREACH()

      SET(OPT_TPLS ${${PACK_NAME}_LIB_OPTIONAL_DEP_TPLS})
      IF(OPT_TPLS)
        LIST(REVERSE OPT_TPLS)
      ENDIF()
      FOREACH(TPL_NAME ${OPT_TPLS})
        IF(TPL_ENABLE_${TPL_NAME})
          LIST(APPEND TPL_LIB_LIST ${TPL_${TPL_NAME}_LIBRARIES})
          LIST(APPEND TPL_INCLUDE_LIST ${TPL_${TPL_NAME}_INCLUDE_DIRS})
        ENDIF()
      ENDFOREACH()
    ENDIF()
  ENDFOREACH()

  SET(LIB_LIST_COPY)
  FOREACH(LIB ${LIB_LIST})
    LIST(APPEND LIB_LIST_COPY ${CMAKE_LINK_LIBRARY_FLAG}${LIB})
  ENDFOREACH()
  SET(LIB_LIST ${LIB_LIST_COPY})

  IF (LIB_LIST)
    LIST(REVERSE LIB_LIST)
    LIST(REMOVE_DUPLICATES LIB_LIST)
    LIST(REVERSE LIB_LIST)      
  ENDIF()

  SET(LIB_STR "")
  FOREACH(LIB ${LIB_LIST})
    SET(LIB_STR "${LIB_STR} ${LIB}")
  ENDFOREACH()


  IF (TPL_LIB_LIST)
    LIST(REVERSE TPL_LIB_LIST)
    LIST(REMOVE_DUPLICATES TPL_LIB_LIST)
    LIST(REVERSE TPL_LIB_LIST)
  ENDIF()

  SET(TPL_LIB_STR "")
  FOREACH(LIB ${TPL_LIB_LIST})
    SET(TPL_LIB_STR "${TPL_LIB_STR} ${LIB}")
  ENDFOREACH()

  SET(TPL_INCLUDE_STR "")
  FOREACH(INC_DIR ${TPL_INCLUDE_LIST})
    SET(TPL_INCLUDE_STR "${TPL_INCLUDE_STR} -I${INC_DIR}")
  ENDFOREACH()

  IF (${Proj}_VERBOSE_CONFIGURE)
    PRINT_VAR(LIB_STR)
    PRINT_VAR(TPL_LIB_STR)
    PRINT_VAR(TPL_INCLUDE_STR)
  ENDIF()



  PACKAGE_CONFIGURE_FILE(${EMFName})

ENDFUNCTION()