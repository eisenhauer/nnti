// -*- c++ -*-

// @HEADER
// ***********************************************************************
//
//              PyTrilinos: Python Interface to Trilinos
//                 Copyright (2005) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Bill Spotz (wfspotz@sandia.gov)
//
// ***********************************************************************
// @HEADER

%{
// Epetra includes
#include "Epetra_Version.h"
#include "Epetra_CombineMode.h"
#include "Epetra_DataAccess.h"
#include "Epetra_Object.h"
#include "Epetra_SrcDistObject.h"
#include "Epetra_DistObject.h"
#include "Epetra_CompObject.h"
#include "Epetra_BLAS.h"
#include "Epetra_LAPACK.h"
#include "Epetra_Flops.h"
#include "Epetra_Time.h"
#include "Epetra_Util.h"
#include "Epetra_MapColoring.h"

// Teuchos include
#include "Teuchos_FILEstream.hpp"

// Epetra python exception
static PyObject * PyExc_EpetraError = PyErr_NewException("Epetra.Error",NULL,NULL);
%}

// NumPy support
%pythoncode %{
# Much of the Epetra module is compatible with the numpy module
import numpy

# From numpy version 1.0 forward, we want to use the following import syntax for
# user_array.container.  We rename it UserArray for backward compatibility with
# 0.9.x versions of numpy:
try:
    from numpy.lib.user_array import container as UserArray

# If the previous import failed, it is because we are using a numpy version
# prior to 1.0.  So we catch it and try again with different syntax.
except ImportError:

    # There is a bug in UserArray from numpy 0.9.8.  If this is the case, we
    # have our own patched version.
    try:
        from UserArrayFix import UserArray
        
    # If the previous import failed, it is because we are using a version of
    # numpy prior to 0.9.8, such as 0.9.6, which has no bug and therefore has no
    # local patch.  Now we can import using the old syntax:
    except ImportError:
        from numpy.lib.UserArray import UserArray
%}

// General ignore directives
%ignore *::operator=;                // Not overrideable in python
%ignore *::operator[];               // Replaced with __setitem__ method
%ignore *::operator[] const;         // Replaced with __getitem__ method
%ignore *::UpdateFlops(int) const;   // Use long int version
%ignore *::UpdateFlops(float) const; // Use double version

////////////////////////
// Exception handling //
////////////////////////

// Define the EpetraError exception
%constant PyObject * Error = PyExc_EpetraError;  // This steals the reference

// Define macro for handling exceptions thrown by Epetra methods and
// constructors
%define %epetra_exception(className,methodName)
%exception className::methodName {
  try {
    $action
    if (PyErr_Occurred()) SWIG_fail;
  } catch(int errCode) {
    PyErr_Format(PyExc_EpetraError, "Error code = %d\nSee stderr for details", errCode);
    SWIG_fail;
  }
}
%enddef

// Define macro for handling exceptions thrown by Epetra_NumPy*
// constructors
%define %epetra_numpy_ctor_exception(className)
%exception className::className {
  try {
    $action
    if (PyErr_Occurred()) {
      className::cleanup();
      SWIG_fail;
    }
  } catch (int errCode) {
    PyErr_Format(PyExc_EpetraError, "Error code = %d\nSee stderr for details", errCode);
    SWIG_fail;
  }
}
%enddef

//////////////////////////////
// Raw data buffer handling //
//////////////////////////////

// Define a macro for converting a method that returns a pointer to
// a 1D array of ints to returning a NumPy array
%define %epetra_intarray1d_output_method(className,methodName,dimMethod)
%ignore className::methodName() const;
%extend className {
  PyObject * methodName() {
    int * result = self->methodName();
    if (result == NULL) return Py_BuildValue("");
    int * data   = NULL;
    intp dims[ ] = { self->dimMethod() };
    PyArray_Descr * dtype = PyArray_DescrFromType(NPY_INT);
    PyObject * returnObj = PyArray_NewFromDescr(&PyArray_Type, dtype, 1, dims, NULL,
						NULL, FARRAY_FLAGS, NULL);
    if (returnObj == NULL) goto fail;
    data = (int*) array_data(returnObj);
    for (int i=0; i<dims[0]; ++i) data[i] = result[i];
    return PyArray_Return((PyArrayObject*)returnObj);
  fail:
    return NULL;
  }
}
%enddef

// Define a macro for converting a method that returns a pointer to
// a 1D array of doubles to returning a NumPy array
%define %epetra_array1d_output_method(className,methodName,dimMethod)
%ignore className::methodName() const;
%extend className {
  PyObject * methodName() {
    double * result = self->methodName();
    if (result == NULL) return Py_BuildValue("");
    double * data   = NULL;
    intp dims[ ] = { self->dimMethod() };
    PyArray_Descr * dtype = PyArray_DescrFromType(NPY_DOUBLE);
    PyObject * returnObj = PyArray_NewFromDescr(&PyArray_Type, dtype, 1, dims, NULL,
						NULL, FARRAY_FLAGS, NULL);
    if (returnObj == NULL) goto fail;
    data = (double*) array_data(returnObj);
    for (int i=0; i<dims[0]; ++i) data[i] = result[i];
    return PyArray_Return((PyArrayObject*)returnObj);
  fail:
    return NULL;
  }
}
%enddef

// Define a macro for converting a method that returns a pointer to
// a 2D array of doubles to returning a NumPy array
%define %epetra_array2d_output_method(className,methodName,dimMethod1,dimMethod2)
%ignore className::methodName() const;
%extend className {
  PyObject * methodName() {
    double * result = self->methodName();
    if (result == NULL) return Py_BuildValue("");
    double * data   = NULL;
    intp dims[ ] = { self->dimMethod1(), self->dimMethod2() };
    PyArray_Descr * dtype = PyArray_DescrFromType(NPY_DOUBLE);
    PyObject * returnObj = PyArray_NewFromDescr(&PyArray_Type, dtype, 2, dims, NULL,
						NULL, FARRAY_FLAGS, NULL);
    if (returnObj == NULL) goto fail;
    data = (double*) array_data(returnObj);
    for (int i=0; i<dims[0]*dims[1]; ++i) data[i] = result[i];
    return PyArray_Return((PyArrayObject*)returnObj);
  fail:
    return NULL;
  }
}
%enddef

// Define macro for a typemap that converts return arguments from
// Epetra_*Matrix or Epetra_*Vector to the corresponding
// Epetra_NumPy*Matrix or Epetra_NumPy*Vector.  There is additional
// magic in the python code to convert the Epetra_NumPy*Matrix or
// Epetra_NumPy*Vector to to an Epetra.*Matrix or Epetra.*Vector.
%define %epetra_array_output_typemaps(array,numPyArray)
%typemap(out) array * {
  if ($1 == NULL) $result = Py_BuildValue("");
  else {
    numPyArray * npa = new numPyArray(*$1);
    $result = SWIG_NewPointerObj(npa, $descriptor(numPyArray*), 1);
  }
}
%apply (array *) {array &}
%enddef

// Define macro for a typemap that converts a reference to a pointer
// to an object, into a return argument (which might be placed into a
// tuple, if there are more than one).
%define %epetra_argout_typemaps(ClassName)
%typemap(in,numinputs=0) ClassName *& (ClassName * _object) {
  $1 = &_object;
}
%typemap(argout) ClassName *& {
  PyObject * obj = SWIG_NewPointerObj((void*)(*$1), $descriptor(ClassName*), 1);
  $result = SWIG_Python_AppendOutput($result,obj);
}
%enddef

// Define macro for a typemap that converts a reference to a pointer
// to an Epetra array object, into a return argument (which might be
// placed into a tuple, if there are more than one).
%define %epetra_array_argout_typemaps(ClassName)
%typemap(in,numinputs=0) Epetra_ ## ClassName *& (Epetra_ ## ClassName * _object) {
  $1 = &_object;
}
%typemap(argout) Epetra_ ## ClassName *& {
  PyObject * obj;
  Epetra_NumPy ## ClassName * npa = new Epetra_NumPy ## ClassName(**$1);
  obj = SWIG_NewPointerObj((void*)npa, $descriptor(Epetra_NumPy ## ClassName*), 1);
  $result = SWIG_Python_AppendOutput($result,obj);
}
%enddef

// Define a macro for a directorin typemap that converts a C++
// Epetra_[Multi]Vector to a python Epetra.[Multi]Vector.
%define %epetra_array_director_typemaps(ClassName)
%typemap(directorin) Epetra_ ## ClassName & %{
  Epetra_NumPy ## ClassName npa$argnum = Epetra_NumPy ## ClassName(View,$1_name);
  $input = SWIG_NewPointerObj(&npa$argnum, $descriptor(Epetra_NumPy ## ClassName*), 0);
%}
%enddef

////////////////////////////
// Epetra_Version support //
////////////////////////////
%rename(Version) Epetra_Version;
%include "Epetra_Version.h"
%pythoncode %{
__version__ = Version().split()[2]
%}

////////////////////////////////
// Epetra_CombineMode support //
////////////////////////////////
%include "Epetra_CombineMode.h"

///////////////////////////////
// Epetra_DataAccess support //
///////////////////////////////
%include "Epetra_DataAccess.h"

///////////////////////////
// Epetra_Object support //
///////////////////////////
%rename(Object) Epetra_Object;
%epetra_exception(Epetra_Object,Print)
%extend Epetra_Object {
  // The __str__() method is used by the python str() operator on any
  // object given to the python print command.
  std::string __str__() {
    std::stringstream os;
    self->Print(os);             // Put the output in os
    std::string s = os.str();         // Extract the string from os
    int last = s.length();       // Get the last index
    if (s.substr(last) == "\n")
      last-=1;                   // Ignore any trailing newline
    return s.substr(0,last);     // Return the string
  }
  // The Epetra_Object::Print(ostream) method will be ignored and
  // replaced by a Print() method here that takes a python file object
  // as its optional argument.  If no argument is given, then output
  // is to standard out.
  void Print(PyObject*pf=NULL) const {
    if (pf == NULL) {
      self->Print(std::cout);
    } else {
      if (!PyFile_Check(pf)) {
	PyErr_SetString(PyExc_IOError, "Print() method expects file object");
      } else {
	std::FILE * f = PyFile_AsFile(pf);
	Teuchos::FILEstream buffer(f);
	std::ostream os(&buffer);
	self->Print(os);
      }
    }
  }
}
%ignore *::Print;  // Only the above Print() method will be implemented
%ignore operator<<(ostream &, const Epetra_Object &); // From python, use __str__
%include "Epetra_Object.h"

//////////////////////////////////
// Epetra_SrcDistObject support //
//////////////////////////////////
%rename(SrcDistObject) Epetra_SrcDistObject;
%include "Epetra_SrcDistObject.h"

///////////////////////////////
// Epetra_DistObject support //
///////////////////////////////
%rename(DistObject) Epetra_DistObject;
%include "Epetra_DistObject.h"

///////////////////////////////
// Epetra_CompObject support //
///////////////////////////////
%rename(CompObject) Epetra_CompObject;
%include "Epetra_CompObject.h"

/////////////////////////
// Epetra_BLAS support //
/////////////////////////
// I do not want to expose this functionality to python
%import "Epetra_BLAS.h"

///////////////////////////
// Epetra_LAPACK support //
///////////////////////////
// I do not want to expose this functionality to python
%import "Epetra_LAPACK.h"

//////////////////////////
// Epetra_Flops support //
//////////////////////////
%rename(FLOPS) Epetra_Flops;
%include "Epetra_Flops.h"

/////////////////////////
// Epetra_Time support //
/////////////////////////
%rename(Time) Epetra_Time;
%include "Epetra_Time.h"

/////////////////////////
// Epetra_Util support //
/////////////////////////
%ignore Epetra_Util::Sort;
%rename(Util) Epetra_Util;
%include "Epetra_Util.h"

////////////////////////////////
// Epetra_MapColoring support //
////////////////////////////////
%rename(MapColoring) Epetra_MapColoring;
%ignore Epetra_MapColoring::Epetra_MapColoring(const Epetra_BlockMap &, int*, const int);
%ignore Epetra_MapColoring::operator()(int) const;
%ignore Epetra_MapColoring::ListOfColors() const;
%ignore Epetra_MapColoring::ColorLIDList(int) const;
%ignore Epetra_MapColoring::ElementColors() const;
%apply (int DIM1, int* IN_ARRAY1) {(int numColors, int* elementColors)};
%extend Epetra_MapColoring {

  Epetra_MapColoring(const Epetra_BlockMap & map,
		     int numColors, int* elementColors,
		     const int defaultColor=0) {
    Epetra_MapColoring * mapColoring;
    if (numColors != map.NumMyElements()) {
      PyErr_Format(PyExc_ValueError,
		   "Epetra.BlockMap has %d elements, while elementColors has %d",
		   map.NumMyElements(), numColors);
      goto fail;
    }
    mapColoring = new Epetra_MapColoring(map, elementColors, defaultColor);
    return mapColoring;
  fail:
    return NULL;
  }

  int __getitem__(int i) {
    return self->operator[](i);
  }

  void __setitem__(int i, int color) {
    self->operator[](i) = color;
  }

  PyObject * ListOfColors() {
    int      * list    = self->ListOfColors();
    intp       dims[ ] = { self->NumColors() };
    int      * data;
    PyObject * retObj  = PyArray_SimpleNew(1,dims,NPY_INT);
    if (retObj == NULL) goto fail;
    data = (int*) array_data(retObj);
    for (int i = 0; i<dims[0]; i++) data[i] = list[i];
    return PyArray_Return((PyArrayObject*)retObj);
  fail:
    Py_XDECREF(retObj);
    return NULL;
  }

  PyObject * ColorLIDList(int color) {
    int      * list    = self->ColorLIDList(color);
    intp       dims[ ] = { self->NumElementsWithColor(color) };
    int      * data;
    PyObject * retObj  = PyArray_SimpleNew(1,dims,NPY_INT);
    if (retObj == NULL) goto fail;
    data = (int*) array_data(retObj);
    for (int i = 0; i<dims[0]; i++) data[i] = list[i];
    return PyArray_Return((PyArrayObject*)retObj);
  fail:
    Py_XDECREF(retObj);
    return NULL;
  }

  PyObject * ElementColors() {
    int      * list    = self->ElementColors();
    intp       dims[ ] = { self->Map().NumMyElements() };
    int      * data;
    PyObject * retObj  = PyArray_SimpleNew(1,dims,NPY_INT);
    if (retObj == NULL) goto fail;
    data = (int*) array_data(retObj);
    for (int i = 0; i<dims[0]; i++) data[i] = list[i];
    return PyArray_Return((PyArrayObject*)retObj);
  fail:
    Py_XDECREF(retObj);
    return NULL;
  }
}
%include "Epetra_MapColoring.h"
%clear (int numColors, int* elementColors);
