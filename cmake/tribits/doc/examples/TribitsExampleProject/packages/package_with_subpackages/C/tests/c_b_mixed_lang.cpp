#include "c_b_mixed_lang.hpp"

#include "C.hpp"
#include "b_mixed_lang.hpp"

std::string PackageWithSubpackages::c_b_mixed_lang()
{
  return (depsC()+" "+b_mixed_lang());
}
