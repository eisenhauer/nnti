SET(FIND_TPL_NAME "FindTPLCUDA.cmake")
SET(NEW_TPL_DIR "core/std_tpls")

MESSAGE(WARNING "WARNING: The file tpls/${FIND_TPL_NAME}"
  " has been moved to ${NEW_TPL_DIR}/${FIND_TPL_NAME}!"
  "Please use the moved copy as this deprecated copy will be removed soon!")
INCLUDE("${CMAKE_CURRENT_LIST_DIR}/../${NEW_TPL_DIR}/${FIND_TPL_NAME}")
