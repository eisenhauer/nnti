SET(SUBPACKAGES_DIRS_CLASSIFICATIONS_OPTREQS
  Util       stk_util        SS  OPTIONAL
  Topology   stk_topology    SS  OPTIONAL
  Mesh       stk_mesh        SS  OPTIONAL
  IO         stk_io          SS  OPTIONAL
  Search     stk_search      SS  OPTIONAL
  SearchUtil stk_search_util SS  OPTIONAL
  Transfer   stk_transfer    SS  OPTIONAL
#  Sddm       stk_sddm        SS  OPTIONAL
)

SET(LIB_REQUIRED_DEP_PACKAGES)
SET(LIB_OPTIONAL_DEP_PACKAGES)
SET(TEST_REQUIRED_DEP_PACKAGES)
SET(TEST_OPTIONAL_DEP_PACKAGES)
SET(LIB_REQUIRED_DEP_TPLS)
SET(LIB_OPTIONAL_DEP_TPLS MPI)
SET(TEST_REQUIRED_DEP_TPLS)
SET(TEST_OPTIONAL_DEP_TPLS)
