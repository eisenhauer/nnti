// -*- c++ -*-

%module(package="PyTrilinos") EpetraExt

%{
// System includes
#include <vector>

// Epetra includes
#include "Epetra_Object.h"
#include "Epetra_SrcDistObject.h"
#include "Epetra_DistObject.h"
#include "Epetra_CrsGraph.h"
#include "Epetra_IntVector.h"
#include "Epetra_MapColoring.h"

// EpetraExt includes
#include "EpetraExt_MapColoring.h"
#include "EpetraExt_MapColoringIndex.h"
%}

// Ignore directives
%ignore Epetra_CrsGraph::operator[](int);
%ignore Epetra_CrsGraph::operator[](int) const;
%ignore Epetra_CrsGraph::operator=(const Epetra_CrsGraph &);
%ignore Epetra_IntVector::operator=(const Epetra_IntVector &);
%ignore Epetra_IntVector::operator[](int);
%ignore Epetra_IntVector::operator[](int) const;
%ignore Epetra_MapColoring::operator[](int);
%ignore Epetra_MapColoring::operator[](int) const;

// // Typemap directives
// %typemap(out) (std::vector<Epetra_IntVector>)
// {
//   int numIV = $1.size();
//   $result = PyTuple_New(numIV);
//   for (i=0; ++i; i<numIV) {
//     int numInt = $1[i].MyLength();
//     PyObject * item = PyTuple_New(numInt);
//     for (j=0; ++j; j<numInt) {
//       PyTuple_SetItem(item, j, PyInt_FromLong($1[i][j]));
//     }
//     PyTuple_SetItem($result, i, item);
//     Py_DECREF(item);   // We are done with local reference to item
//   }
//   return $result;
// }

// C++ STL support
%include "std_vector.i"

// Epetra interface import
%import "Epetra_Object.h"
%import "Epetra_SrcDistObject.h"
%import "Epetra_DistObject.h"
%import "Epetra_CrsGraph.h"
%import "Epetra_IntVector.h"
%import "Epetra_MapColoring.h"

// EpetraExt interface includes
%include "EpetraExt_Transform.h"
%template (Transform_CrsGraph_MapColoring)
  EpetraExt::Transform<Epetra_CrsGraph, Epetra_MapColoring>;
%template (Transform_CrsGraph_MapColoringIndex)
  EpetraExt::Transform<Epetra_CrsGraph, std::vector<Epetra_IntVector> >;
%template (StructuralTransform_CrsGraph_MapColoring)
  EpetraExt::StructuralTransform<Epetra_CrsGraph, Epetra_MapColoring>;
%template (StructuralTransform_CrsGraph_MapColoringIndex)
  EpetraExt::StructuralTransform<Epetra_CrsGraph, std::vector<Epetra_IntVector> >;

%include "EpetraExt_MapColoring.h"
%include "EpetraExt_MapColoringIndex.h"
