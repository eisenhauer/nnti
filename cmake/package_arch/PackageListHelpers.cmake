
INCLUDE(PackageArchHostType)


#
# This module defines the datastructure for the list of packages
# ${PROJECT_NAME}_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS which has the form:
#
#   Package0_name  Package0_dir Package0_classification
#   Package1_name  Package1_dir Package1_classification
#   ...
#
# There are 3 fields per row all stored in a flat array.
# 


SET(PLH_NUM_FIELDS_PER_PACKAGE 3)
SET(PLH_NUM_PACKAGE_DIR_OFFSET 1)
SET(PLH_NUM_PACKAGE_CLASSIFICATION_OFFSET 2)


MACRO(PACKAGE_SET_PACKAGE_TO_EX  PACKAGE_NAME)
  LIST(FIND ${PROJECT_NAME}_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS
    ${PACKAGE_NAME} PACKAGE_NAME_IDX)
  IF (PACKAGE_NAME_IDX EQUAL -1)
    MESSAGE(
      "\n***"
      "\n*** WARNING: Package ${PACKAGE_NAME} not found in list of packages!"
      "\n***\n"
      )
  ELSE()
    MATH(EXPR PACKAGE_CLASSIFICATION_IDX "${PACKAGE_NAME_IDX}+2")
    LIST(REMOVE_AT ${PROJECT_NAME}_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS
      ${PACKAGE_CLASSIFICATION_IDX} )
    LIST(INSERT ${PROJECT_NAME}_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS
      ${PACKAGE_CLASSIFICATION_IDX} EX )
  ENDIF()

ENDMACRO()


MACRO( PACKAGE_DISABLE_ON_PLATFORMS  PACKAGE_NAME )
  #MESSAGE("PACKAGE_DISABLE_ON_PLATFORMS: ${PACKAGE_NAME}")
  #PRINT_VAR(${PROJECT_NAME}_HOSTTYPE)
  FOREACH(HOSTTYPE ${ARGN})
    #PRINT_VAR(HOSTTYPE)
    IF (${PROJECT_NAME}_HOSTTYPE STREQUAL ${HOSTTYPE})
      #MESSAGE("${${PROJECT_NAME}_HOSTTYPE} == ${HOSTTYPE}")
      PACKAGE_SET_PACKAGE_TO_EX(${PACKAGE_NAME})
      #PRINT_VAR(${PROJECT_NAME}_PACKAGES_AND_DIRS_AND_CLASSIFICATIONS)
      IF (${PROJECT_NAME}_ENABLE_${PACKAGE_NAME})
        MESSAGE(
          "\n***"
          "\n*** WARNING: User has set ${PROJECT_NAME}_ENABLE_${PACKAGE_NAME}=ON but the"
          "\n*** package ${PACKAGE_NAME} is not supported on this platform type '${HOSTTYPE}'!"
          "\n***\n"
          )
      ENDIF()
    ENDIF()
  ENDFOREACH()
ENDMACRO()
