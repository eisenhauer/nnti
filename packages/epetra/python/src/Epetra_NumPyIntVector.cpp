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

#include "Epetra_NumPyIntVector.h"

#define DEBUG 0
#if DEBUG
#include <iostream>
#include <string>
using namespace std;
#endif

// Static variables
const Epetra_SerialComm   Epetra_NumPyIntVector::defaultComm = Epetra_SerialComm();
PyArrayObject           * Epetra_NumPyIntVector::tmp_array   = NULL               ;
Epetra_Map              * Epetra_NumPyIntVector::tmp_map     = NULL               ;

// Static helper functions
// =============================================================================
int * Epetra_NumPyIntVector::getArray(PyObject * pyObject)
{
  // Try to build a contiguous PyArrayObject from the pyObject
  if (!tmp_array) tmp_array = (PyArrayObject *) PyArray_ContiguousFromObject(pyObject,'i',0,0);
  
  // If this fails, build a vector with length zero to prevent a Bus
  // Error
  if (!tmp_array) {
    intp dimensions[ ] = { 0 };
    tmp_array = (PyArrayObject *) PyArray_SimpleNew(1,dimensions,PyArray_INT);
  }

  return (int*)(tmp_array->data);
}

// =============================================================================
int * Epetra_NumPyIntVector::getArray(const Epetra_BlockMap & blockMap,
				      PyObject * pyObject)
{
  // Only build the tmp_array if it does not already exist
  if (!tmp_array) {

    // Default dimensions
    intp defaultDims[ ] = { blockMap.NumMyPoints() };

    // PyObject argument is a bool
    if PyBool_Check(pyObject) {
      tmp_array = (PyArrayObject *) PyArray_SimpleNew(1,defaultDims,PyArray_INT);
      if (tmp_array == NULL) {
	defaultDims[0] = 0;
	tmp_array = (PyArrayObject *) PyArray_SimpleNew(1,defaultDims,PyArray_INT);
      }

    // PyObject argument is not a bool ... try to build a contiguous PyArrayObject from it
    } else {
      tmp_array = (PyArrayObject *) PyArray_ContiguousFromObject(pyObject,'i',0,0);

      // If this fails, build a single vector with zero length
      if (tmp_array == NULL) {
	defaultDims[0] = 0;
	tmp_array = (PyArrayObject *) PyArray_SimpleNew(1,defaultDims,PyArray_INT);

      // If the contiguous PyArrayObject built successfully, make sure
      // it has the correct number of dimensions
      } else {
	int  nd = tmp_array->nd;
	intp arraySize = PyArray_MultiplyList(tmp_array->dimensions,nd);
	if (arraySize != defaultDims[0]) {
	  PyArrayObject * myArray = (PyArrayObject *) PyArray_SimpleNew(1,defaultDims,
									PyArray_INT);
	  int           * myData  = (int *) myArray->data;
	  int           * tmpData = (int *) tmp_array->data;
	  for (int i=0; i<defaultDims[0]; i++) {
	    myData[i] = tmpData[i];
	  }
	  Py_XDECREF(tmp_array);
	  tmp_array = myArray;
	}
      }
    }
  }
  return (int*)(tmp_array->data);
}

// =============================================================================
Epetra_Map & Epetra_NumPyIntVector::getMap(PyObject * pyObject)
{
  if (!tmp_array) getArray(pyObject);
  if (!tmp_map) {
    const int totalLength = PyArray_Size((PyObject *)tmp_array);
    tmp_map = new Epetra_Map(totalLength,0,defaultComm);
  }
  return *tmp_map;
}

// =============================================================================
void Epetra_NumPyIntVector::cleanup()
{
  if (tmp_array) {
    Py_DECREF(tmp_array);
    tmp_array = NULL;
  }
  if (tmp_map) {
    delete tmp_map;
    tmp_map = NULL;
  }
}

// =============================================================================

// Constructors
// =============================================================================
Epetra_NumPyIntVector::Epetra_NumPyIntVector(const Epetra_BlockMap & blockMap, bool zeroOut):
  Epetra_IntVector(blockMap, zeroOut)
{
  // Create the array object
  intp dims[ ] = { blockMap.NumMyPoints() };
  int *v = NULL;
  Epetra_IntVector::ExtractView(&v);
  array = (PyArrayObject *) PyArray_SimpleNewFromData(1,dims,PyArray_INT,
						      (void *)v);

  // Copy the Epetra_BlockMap
  map = new Epetra_BlockMap(blockMap);
}

// =============================================================================
Epetra_NumPyIntVector::Epetra_NumPyIntVector(const Epetra_IntVector & source):
  Epetra_IntVector(source)
{
  map = new Epetra_BlockMap(source.Map());
  intp dims[ ] = { map->NumMyPoints() };
  int *v = NULL;
  Epetra_IntVector::ExtractView(&v);
  array = (PyArrayObject *) PyArray_SimpleNewFromData(1,dims,PyArray_INT,
						      (void *)v);
}

// =============================================================================
Epetra_NumPyIntVector::Epetra_NumPyIntVector(const Epetra_BlockMap & blockMap,
					     PyObject * pyObject):
  Epetra_IntVector(View, blockMap, getArray(blockMap,pyObject))
{
  // Get the pointer to the array from static variable and clear
  assert(NULL != tmp_array);
  array     = tmp_array;
  tmp_array = NULL;

  // Copy the Epetra_BlockMap
  map = new Epetra_BlockMap(blockMap);
}

// =============================================================================
Epetra_NumPyIntVector::Epetra_NumPyIntVector(PyObject * pyObject):
  Epetra_IntVector(View, getMap(pyObject), getArray(pyObject)) 
{
  // Store the pointer to the Epetra_Map
  assert(NULL != tmp_map);
  map     = tmp_map;
  tmp_map = NULL;

  // Store the pointer to the PyArrayObject
  assert(NULL != tmp_array);
  array     = tmp_array;
  tmp_array = NULL;
}

// =============================================================================
// Destructor
Epetra_NumPyIntVector::~Epetra_NumPyIntVector()
{
  Py_XDECREF(array);
  delete map;
}

// =============================================================================
PyObject * Epetra_NumPyIntVector::ExtractCopy() const
{
  return PyArray_Copy(array);
}

// =============================================================================
PyObject * Epetra_NumPyIntVector::ExtractView() const
{
  Py_INCREF(array);
  return PyArray_Return(array);
}

// =============================================================================
PyObject * Epetra_NumPyIntVector::Values() const
{
  Py_INCREF(array);
  return PyArray_Return(array);
}
