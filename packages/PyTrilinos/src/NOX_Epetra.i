// -*- c++ -*-

%module NOX_Epetra

%{
// Epetra includes
#include "Epetra_BLAS.h"
#include "Epetra_Object.h"
#include "Epetra_CompObject.h"
#include "Epetra_SrcDistObject.h"
#include "Epetra_DistObject.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Vector.h"
#include "Epetra_NumPyVector.h"
#include "Epetra_Operator.h"
#include "Epetra_RowMatrix.h"

// NOX includes
#include "NOX_Abstract_Group.H"
#include "NOX_Abstract_Vector.H"
#include "NOX_Epetra_Interface.H"
#include "NOX_Epetra_Group.H"
#include "NOX_Epetra_FiniteDifference.H"
#include "NOX_Epetra_FiniteDifferenceColoring.H"
#include "NOX_Epetra_Vector.H"

// Local includes
#include "Callback.h"
#include "Epetra_VectorHelper.h"
#include "NumPyWrapper.h"
#include "PyInterface.h"

// Namespace flattening
using namespace NOX          ;
using namespace NOX::Abstract;
using namespace NOX::Epetra  ;
%}

// Ignore directives
%ignore *::print(ostream &, int) const;
%ignore Epetra_CompObject::operator=(const Epetra_CompObject &);
%ignore Epetra_MultiVector::operator=(const Epetra_MultiVector &);
%ignore Epetra_MultiVector::operator[](int);
%ignore Epetra_MultiVector::operator[](int) const;
%ignore Epetra_MapColoring::operator[](int);
%ignore Epetra_MapColoring::operator[](int) const;
%ignore NOX::Abstract::Group::operator=(const NOX::Abstract::Group&);
%ignore NOX::Abstract::Vector::operator=(const Epetra_Vector&);
%ignore NOX::Abstract::Vector::operator=(const NOX::Epetra::Vector&);
%ignore NOX::Abstract::Vector::operator=(const NOX::Abstract::Vector&);
%ignore NOX::Abstract::Vector::print() const;
%ignore NOX::Epetra::Group::operator=(const NOX::Epetra::Group&);
%ignore NOX::Epetra::Group::operator=(const NOX::Abstract::Group&);
%ignore NOX::Epetra::Vector(Epetra_Vector&, NOX::CopyType, bool);

// Rename directives
%rename(Group_None) NOX::Epetra::Group::None;

// Epetra, EpetraExt and NOX::Abstract imports
%import "Epetra_BLAS.h"
%import "Epetra_Object.h"
%import "Epetra_CompObject.h"
%import "Epetra_SrcDistObject.h"
%import "Epetra_DistObject.h"
%import "Epetra_MultiVector.h"
%import "Epetra_Vector.h"
%import "Epetra_NumPyVector.h"
%import "Epetra_Operator.h"
%import "Epetra_RowMatrix.h"
%import "NOX_Abstract_Group.H"
%import "NOX_Abstract_Vector.H"
%import "EpetraExt.i"

// NOX interface includes
using namespace std;
%include "NOX_Epetra_Interface.H"
%include "NOX_Epetra_Group.H"
%include "NOX_Epetra_FiniteDifference.H"
%include "NOX_Epetra_FiniteDifferenceColoring.H"
%include "NOX_Epetra_Vector.H"

// Local interface includes
%include "Callback.h"
%include "PyInterface.h"
