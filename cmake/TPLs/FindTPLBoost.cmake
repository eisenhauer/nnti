INCLUDE(TPLDeclareLibraries)

TPL_DECLARE_LIBRARIES( Boost
  REQUIRED_HEADERS boost/version.hpp boost/mpl/at.hpp
  REQUIRED_LIBS_NAMES "program_options"
  )
