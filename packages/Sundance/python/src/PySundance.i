// -*- c++ -*-

%module PySundance

%feature("autodoc");

%exception 
{
  try
    {
      $action
    }
  catch (std::exception& e)
    {
      PyErr_SetString(PyExc_RuntimeError, const_cast<char*>(e.what()));
      return NULL;
    }
}

%{
#include "Sundance.hpp"
#include "SundancePathUtils.hpp"
  %}


%inline %{
  bool passFailTest(double err, double tol)
  {
    return SundanceStdFwk::Sundance::passFailTest(err, tol);
  }


  void skipTimingOutput() {Sundance::skipTimingOutput()=true;}

  %}


%include "std_string.i"
namespace SundanceUtils
{
  std::string searchForFile(const std::string& name);
}


%inline%{
  class PyOut
  {
  public:
    PyOut() {}

    void write(const std::string& s) 
      {
        SundanceUtils::Out::os() << s;
      }
  };
  %}

%include Mesh.i

%include Utils.i

%include Array.i

%include ParameterList.i

%include CellFilter.i

%include TSF.i

%include Quadrature.i

%include Basis.i

%include Spectral.i

%include Symbolics.i

%include CoordinateSystem.i

%include Integral.i

%include LinearProblem.i

%include NonlinearProblem.i

%include LinearEigenproblem.i

%include Functional.i

%include Viz.i

%include Discrete.i

%include AToC.i  
