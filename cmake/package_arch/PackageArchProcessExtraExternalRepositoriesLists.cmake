
INCLUDE(SetCacheOnOffEmpty)
INCLUDE(MultilineSet)
INCLUDE(AdvancedOption)


#
# This module defines the datastructure for the list of packages
# ${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY which has the form:
#
#   repo0_name  repo1_dir  repo1_repotype  repo0_repourl  repo0_packstat  repo0_classification
#   repo1_name  repo1_dir  repo1_repotype  repo1_repourl  repo1_packstat  repo1_classification
#   ...
#
# If repoi_dir is "" then it is the same as repoi_name
# If repo_packstak is "" then it means that the repo has packages and if is NO_PACKAGES IT
#   has no packages
#
# There are 6 fields per row all stored in a flat array.
# 


SET(ERP_REPO_NAME_OFFSET 0)
SET(ERP_REPO_DIR_OFFSET 1)
SET(ERP_REPO_REPOTYPE_OFFSET 2)
SET(ERP_REPO_REPOURL_OFFSET 3)
SET(ERP_REPO_PACKSTAT_OFFSET 4)
SET(ERP_REPO_CLASSIFICATION_OFFSET 5)

SET(ERP_NUM_FIELDS_PER_REPO 6)


#
# Macro that processes the list varaible contents in
# ${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY into sperate arrays:
#   ${PROJECT_NAME}_EXTRA_REPOSITORIES_DEFAULT
#   ${PROJECT_NAME}_EXTRA_REPOSITORIES_DIRS
#   ${PROJECT_NAME}_EXTRA_REPOSITORIES_REPOTYPES
#   ${PROJECT_NAME}_EXTRA_REPOSITORIES_REPOURLS
#   ${PROJECT_NAME}_EXTRA_REPOSITORIES_PACKSTATS
#
# The macro responds to ${PROJECT_NAME}_ENABLE_KNOWN_EXTERNAL_REPOS_TYPE
# to match the categories.
#
#

MACRO(PACKAGE_ARCH_PROCESS_EXTRAREPOS_LISTS)

  # A) Get the total number of extrarepos defined  

  #PRINT_VAR(${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY)
  ASSERT_DEFINED(${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY)
  LIST(LENGTH ${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY
    ${PROJECT_NAME}_NUM_EXTRAREPOS_AND_FIELDS )
  MATH(EXPR ${PROJECT_NAME}_NUM_EXTRAREPOS
    "${${PROJECT_NAME}_NUM_EXTRAREPOS_AND_FIELDS}/${ERP_NUM_FIELDS_PER_REPO}")
  #PRINT_VAR(${PROJECT_NAME}_NUM_EXTRAREPOS)
  MATH(EXPR ${PROJECT_NAME}_LAST_EXTRAREPO_IDX "${${PROJECT_NAME}_NUM_EXTRAREPOS}-1")
  #PRINT_VAR(${PROJECT_NAME}_LAST_EXTRAREPO_IDX)

  # B) Process the list of extra repos

  SET(${PROJECT_NAME}_EXTRA_REPOSITORIES_DEFAULT)
  SET(${PROJECT_NAME}_EXTRA_REPOSITORIES_DIRS)
  SET(${PROJECT_NAME}_EXTRA_REPOSITORIES_REPOTYPES)
  SET(${PROJECT_NAME}_EXTRA_REPOSITORIES_REPOURLS)
  SET(${PROJECT_NAME}_EXTRA_REPOSITORIES_PACKSTATS)

  FOREACH(EXTRAREPO_IDX RANGE ${${PROJECT_NAME}_LAST_EXTRAREPO_IDX})

    # B.1) Extract the fields for the current extrarepo row

    # NAME
    MATH(EXPR EXTRAREPO_NAME_IDX
      "${EXTRAREPO_IDX}*${ERP_NUM_FIELDS_PER_REPO}+${ERP_REPO_NAME_OFFSET}")
    #PRINT_VAR(EXTRAREPO_NAME_IDX)
    LIST(GET ${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY
      ${EXTRAREPO_NAME_IDX} EXTRAREPO_NAME )
    #PRINT_VAR(EXTRAREPO_NAME)

    # DIR
    MATH(EXPR EXTRAREPO_DIR_IDX
      "${EXTRAREPO_IDX}*${ERP_NUM_FIELDS_PER_REPO}+${ERP_REPO_DIR_OFFSET}")
    #PRINT_VAR(EXTRAREPO_DIR_IDX)
    LIST(GET ${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY
      ${EXTRAREPO_DIR_IDX} EXTRAREPO_DIR )
    #PRINT_VAR(EXTRAREPO_DIR)
    IF (EXTRAREPO_DIR STREQUAL "")
      SET(EXTRAREPO_DIR ${EXTRAREPO_NAME})
    ENDIF()
    #PRINT_VAR(EXTRAREPO_DIR)

    # REPOTYPE
    MATH(EXPR EXTRAREPO_REPOTYPE_IDX
      "${EXTRAREPO_IDX}*${ERP_NUM_FIELDS_PER_REPO}+${ERP_REPO_REPOTYPE_OFFSET}")
    #PRINT_VAR(EXTRAREPO_REPOTYPE_IDX)
    LIST(GET ${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY
      ${EXTRAREPO_REPOTYPE_IDX} EXTRAREPO_REPOTYPE )
    IF (EXTRAREPO_REPOTYPE STREQUAL GIT
      OR EXTRAREPO_REPOTYPE STREQUAL SVN
      )
      # Okay
    ELSE()
      MESSAGE(SEND_ERROR "Error, the repo type of '${EXTRAREPO_REPOTYPE}' for"
        " extra repo ${EXTRAREPO_NAME} is *not* valid.  Valid choices are 'GIT' and 'SVN'!")
    ENDIF()
    #PRINT_VAR(EXTRAREPO_REPOTYPE)

    # REPOURL
    MATH(EXPR EXTRAREPO_REPOURL_IDX
      "${EXTRAREPO_IDX}*${ERP_NUM_FIELDS_PER_REPO}+${ERP_REPO_REPOURL_OFFSET}")
    #PRINT_VAR(EXTRAREPO_REPOURL_IDX)
    LIST(GET ${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY
      ${EXTRAREPO_REPOURL_IDX} EXTRAREPO_REPOURL )
    #PRINT_VAR(EXTRAREPO_REPOURL)

    # PACKSTAT
    MATH(EXPR EXTRAREPO_PACKSTAT_IDX
      "${EXTRAREPO_IDX}*${ERP_NUM_FIELDS_PER_REPO}+${ERP_REPO_PACKSTAT_OFFSET}")
    LIST(GET ${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY
      ${EXTRAREPO_PACKSTAT_IDX} EXTRAREPO_PACKSTAT )
    #PRINT_VAR(EXTRAREPO_PACKSTAT)
    IF (EXTRAREPO_PACKSTAT STREQUAL "")
      SET(EXTRAREPO_PACKSTAT HASPACKAGES)
    ELSEIF(EXTRAREPO_PACKSTAT STREQUAL NOPACKAGES)
      # Okay
    ELSE()
      MESSAGE(SEND_ERROR "Error, the PACKSTAT of '${EXTRAREPO_PACKSTAT}' for"
        " extra repo ${EXTRAREPO_NAME} is *not* valid.  Valid choices are '' and 'NOPACKAGES'!")
    ENDIF()
    #PRINT_VAR(EXTRAREPO_PACKSTAT)

    # CLASSIFICATION
    MATH(EXPR EXTRAREPO_CLASSIFICATION_IDX
      "${EXTRAREPO_IDX}*${ERP_NUM_FIELDS_PER_REPO}+${ERP_REPO_CLASSIFICATION_OFFSET}")
    LIST(GET ${PROJECT_NAME}_EXTRAREPOS_DIR_REPOTYPE_REPOURL_PACKSTAT_CATEGORY
      ${EXTRAREPO_CLASSIFICATION_IDX} EXTRAREPO_CLASSIFICATION )
    #PRINT_VAR(EXTRAREPO_CLASSIFICATION)

    # B.2) Determine the match of the classification

    SET(ADD_EXTRAREPO FALSE)
    #ASSERT_DEFINED(${PROJECT_NAME}_ENABLE_KNOWN_EXTERNAL_REPOS_TYPE)
    #PRINT_VAR(${PROJECT_NAME}_ENABLE_KNOWN_EXTERNAL_REPOS_TYPE)
    IF (${PROJECT_NAME}_ENABLE_KNOWN_EXTERNAL_REPOS_TYPE STREQUAL "Continuous" AND
        EXTRAREPO_CLASSIFICATION STREQUAL "Continuous"
      )
      SET(ADD_EXTRAREPO TRUE)
    ELSEIF (${PROJECT_NAME}_ENABLE_KNOWN_EXTERNAL_REPOS_TYPE STREQUAL "Nightly" AND
        (EXTRAREPO_CLASSIFICATION STREQUAL "Continuous" OR EXTRAREPO_CLASSIFICATION STREQUAL "Nightly")
      )
      SET(ADD_EXTRAREPO TRUE)
    ENDIF()
    #PRINT_VAR(ADD_EXTRAREPO)


    # B.3) Add the extrarepo to the list if the classification matches

    IF (ADD_EXTRAREPO)
      MESSAGE("-- " "Adding extra ${EXTRAREPO_CLASSIFICATION} repository ${EXTRAREPO_NAME} ...")
      LIST(APPEND ${PROJECT_NAME}_EXTRA_REPOSITORIES_DEFAULT ${EXTRAREPO_NAME})
      LIST(APPEND ${PROJECT_NAME}_EXTRA_REPOSITORIES_DIRS ${EXTRAREPO_DIR})
      LIST(APPEND ${PROJECT_NAME}_EXTRA_REPOSITORIES_REPOTYPES ${EXTRAREPO_REPOTYPE})
      LIST(APPEND ${PROJECT_NAME}_EXTRA_REPOSITORIES_REPOURLS ${EXTRAREPO_REPOURL})
      LIST(APPEND ${PROJECT_NAME}_EXTRA_REPOSITORIES_PACKSTATS ${EXTRAREPO_PACKSTAT})
    ENDIF()

  ENDFOREACH()

  # C) Get the actual number of active extra repos

  LIST(LENGTH ${PROJECT_NAME}_EXTRA_REPOSITORIES_DEFAULT ${PROJECT_NAME}_NUM_EXTRAREPOS )
  #PRINT_VAR(${PROJECT_NAME}_NUM_EXTRAREPOS)
  MATH(EXPR ${PROJECT_NAME}_LAST_EXTRAREPO_IDX "${${PROJECT_NAME}_NUM_EXTRAREPOS}-1")

  # D) Print the final set of extrarepos in verbose mode
  
  IF (${PROJECT_NAME}_VERBOSE_CONFIGURE)
    PRINT_VAR(${PROJECT_NAME}_EXTRA_REPOSITORIES_DEFAULT)
    PRINT_VAR(${PROJECT_NAME}_EXTRA_REPOSITORIES_DIRS)
    PRINT_VAR(${PROJECT_NAME}_EXTRA_REPOSITORIES_REPOTYPES)
    PRINT_VAR(${PROJECT_NAME}_EXTRA_REPOSITORIES_REPOURLS)
    PRINT_VAR(${PROJECT_NAME}_EXTRA_REPOSITORIES_PACKSTAT)
  ENDIF()

ENDMACRO()


#
# Extract the final name of the extra repo
#

FUNCTION(GET_EXTRAREPO_BASE_NAME  EXTRAREPO_NAME EXTRAREPO_NAME_OUT)
  GET_FILENAME_COMPONENT(EXTRAREPO_NAME "${EXTRAREPO_NAME}" NAME)
  SET(${EXTRAREPO_NAME_OUT} "${EXTRAREPO_NAME}" PARENT_SCOPE)
ENDFUNCTION()
