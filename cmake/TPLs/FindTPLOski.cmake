INCLUDE(TPLDeclareLibraries)

TPL_DECLARE_LIBRARIES( Oski
  REQUIRED_HEADERS oski/oski.h 
  REQUIRED_LIBS_NAMES oski oskilt oski_mat_CSR_Tid oski_mat_CSC_Tid oski_util)
