
#include "SimpleCxx_HelloWorld.hpp"
#include "HeaderOnlyTpl_stuff.hpp"


std::string SimpleCxx::deps()
{
  return "no_deps";
}


namespace SimpleCxx {


void HelloWorld::printHelloWorld(std::ostream &out) const
{
  out << "Hello World!\n";
#ifdef HAVE_SIMPLECXX___INT64
  out << "We have __int64!\n";
#endif
#ifdef HAVE_SIMPLECXX_DEBUG
  out << "Debug is enabled!\n";
#else
  out << "Release is enabled!\n";
#endif
  out << "Sqr(3) = " << HeaderOnlyTpl::sqr(3) << "\n";
#ifdef SIMPLECXX_SHOW_DEPRECATED_WARNINGS
  (void)someOldFunc();
  (void)someOldFunc2();
#endif
}


int HelloWorld::someOldFunc() const
{
  return 1;
}


int HelloWorld::someOldFunc2() const
{
  return 2;
}


} // namespace SimpleCxx
