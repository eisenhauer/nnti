// -*- c++ -*-

// @HEADER
// ***********************************************************************
//
//            PyTrilinos.Epetra: Python Interface to Epetra
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
// @HEADER

%{
// Epetra includes
#include "Epetra_MultiVector.h"
#include "Epetra_Vector.h"
#include "Epetra_FEVector.h"
#include "Epetra_IntVector.h"

// Local includes
#include "Epetra_NumPyMultiVector.h"
#include "Epetra_NumPyVector.h"
#include "Epetra_NumPyIntVector.h"
%}

// Ignore directives
%ignore Epetra_MultiVector::operator()(int) const;
%ignore Epetra_MultiVector::ExtractCopy(double *, int   ) const;  // These Extract methods
%ignore Epetra_MultiVector::ExtractCopy(double **       ) const;  // are given functionality
%ignore Epetra_MultiVector::ExtractView(double **, int *) const;  // in the derived class
%ignore Epetra_MultiVector::ExtractView(double ***      ) const;  // Epetra_NumPyMultiVector
%ignore Epetra_MultiVector::Dot(const Epetra_MultiVector&,double*) const;
%ignore Epetra_MultiVector::Norm1(double*) const;
%ignore Epetra_MultiVector::Norm2(double*) const;
%ignore Epetra_MultiVector::NormInf(double*) const;
%ignore Epetra_MultiVector::NormWeighted(const Epetra_MultiVector&,double*) const;
%ignore Epetra_MultiVector::MinValue(double*) const;
%ignore Epetra_MultiVector::MaxValue(double*) const;
%ignore Epetra_MultiVector::MeanValue(double*) const;
%ignore Epetra_MultiVector::ResetView(double **);     // These are expert
%ignore Epetra_MultiVector::Pointers() const;         // methods not supported in python
%ignore Epetra_Vector::ExtractCopy(double * ) const;  // These Extract methods are given functionality
%ignore Epetra_Vector::ExtractView(double **) const;  // in the derived class Epetra_NumPyVector
%ignore Epetra_Vector::ReplaceGlobalValues(int,double*,int*);      // These four Replace methods are overloaded
%ignore Epetra_Vector::ReplaceGlobalValues(int,int,double*,int*);  // in Epetra_NumPyVector with the double*
%ignore Epetra_Vector::ReplaceMyValues(int,double*,int*);          // and int* arguments replaced with PyObject*s
%ignore Epetra_Vector::ReplaceMyValues(int,int,double*,int*);      // and the first int argument made implicit
%ignore Epetra_Vector::SumIntoGlobalValues(int,double*,int*);      // These four SumInto methods are overloaded
%ignore Epetra_Vector::SumIntoGlobalValues(int,int,double*,int*);  // in Epetra_NumPyVector with the double*
%ignore Epetra_Vector::SumIntoMyValues(int,double*,int*);	   // and int* arguments replaced with PyObject*s
%ignore Epetra_Vector::SumIntoMyValues(int,int,double*,int*);	   // and the first int argument made implicit
%ignore Epetra_Vector::ResetView(double *);           // Expert method not supported in python

// Rename directives
%rename(NumPyMultiVector) Epetra_NumPyMultiVector;
%rename(NumPyVector     ) Epetra_NumPyVector;
%rename(NumPyIntVector  ) Epetra_NumPyIntVector;
%rename(FEVector        ) Epetra_FEVector;
%rename(_AuxMultiVector ) MultiVector;
%rename(_AuxVector      ) Vector;
%rename(_AuxIntVector   ) IntVector;

// These are place-holders on the C++ side for the python-defined
// MultiVector, Vector and IntVector classes
%inline {
  struct MultiVector { };
  struct Vector      { };
  struct IntVector   { };
}

// Import directives for Epetra
%include "Epetra_MultiVector.h"
%include "Epetra_Vector.h"
%include "Epetra_FEVector.h"
%include "Epetra_IntVector.h"

// Local interface includes
%include "Epetra_NumPyMultiVector.h"
%include "Epetra_NumPyVector.h"
%include "Epetra_NumPyIntVector.h"

// Python code.  Here we define the Epetra.MultiVector, Epetra.Vector
// and Epetra.IntVector python classes, which multiply inherit from
// the Numeric UserArray class (making these classes Numeric arrays)
// and the Epetra_NumPyMultiVector, Epetra_NumPyVector or
// Epetra_NumPyIntVector class (making these classes also Epetra
// objects).
%pythoncode %{

from UserArray import *

class MultiVector(UserArray,NumPyMultiVector):
    def __init__(self, *args):
        """
        __init__(self, BlockMap map, int numVectors, bool zeroOut=True) -> MultiVector
        __init__(self, MultiVector source) -> MultiVector
        __init__(self, BlockMap map, PyObject array) -> MultiVector
        __init__(self, DataAccess CV, MultiVector source, PyObject range) -> MultiVector
        __init__(self, PyObject array) -> MultiVector
        """
        NumPyMultiVector.__init__(self, *args)
        UserArray.__init__(self,self.ExtractView(),'d',copy=False,savespace=True)
    def __str__(self):
        return str(self.array)
    def __setattr__(self, key, value):
        "Protect the 'array' and 'shape' attributes"
        if key == "array":
            if key in self.__dict__:
                raise AttributeError, "Cannot change Epetra.MultiVector array attribute"
        elif key == "shape":
            value = tuple(value)
            if len(value) < 2:
                raise ValueError, "Epetra.MultiVector shape is " + str(value) + \
		  " but must have minimum of 2 elements"
        UserArray.__setattr__(self, key, value)

class Vector(UserArray,NumPyVector):
    def __init__(self, *args):
        """
        __init__(self, BlockMap map, bool zeroOut=True) -> Vector
        __init__(self, Vector source) -> Vector
        __init__(self, BlockMap map, PyObject array) -> Vector
        __init__(self, DataAccess CV, MultiVector source, PyObject range) -> Vector
        __init__(self, PyObject array) -> Vector
        """
        NumPyVector.__init__(self, *args)
        UserArray.__init__(self,self.ExtractView(),'d',copy=False,savespace=True)
    def __str__(self):
        return str(self.array)
    def __setattr__(self, key, value):
        "Protect the 'array' attribute"
        if key == "array":
            if key in self.__dict__:
                raise AttributeError, "Cannot change Epetra.Vector array attribute"
        UserArray.__setattr__(self, key, value)

class IntVector(UserArray,NumPyIntVector):
    def __init__(self, *args):
        """
        __init__(self, BlockMap map, bool zeroOut=True) -> IntVector
        __init__(self, IntVector source) -> IntVector
        __init__(self, BlockMap map, PyObject array) -> IntVector
        __init__(self, PyObject array) -> IntVector
        """
        NumPyIntVector.__init__(self, *args)
        UserArray.__init__(self,self.ExtractView(),'i',copy=False,savespace=True)
    def __str__(self):
        return str(self.array)
    def __setattr__(self, key, value):
        "Protect the 'array' attribute"
        if key == "array":
            if key in self.__dict__:
                raise AttributeError, "Cannot change Epetra.IntVector array attribute"
        UserArray.__setattr__(self, key, value)

_Epetra._AuxMultiVector_swigregister(MultiVector)
_Epetra._AuxVector_swigregister(          Vector)
_Epetra._AuxIntVector_swigregister(    IntVector)

%}
