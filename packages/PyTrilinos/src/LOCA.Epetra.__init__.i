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

%define %loca_epetra_docstring
"
PyTrilinos.LOCA.Epetra is the python interface to namespace Epetra for
the Trilinos package LOCA:

    http://trilinos.sandia.gov/packages/nox

The purpose of LOCA.Epetra is to provide a concrete interface beteen
LOCA and Epetra.

"
%enddef
%module(package      = "PyTrilinos.LOCA.Epetra",
	directors    = "1",
	autodoc      = "1",
	implicitconv = "1",
	docstring    = %loca_epetra_docstring) __init__

%{
// System includes
#include <vector>

// Teuchos includes
#include "Teuchos_PythonParameter.h"


// Local includes
#include "NumPyImporter.h"
#include "Epetra_NumPyIntVector.h"
#include "Epetra_NumPyMultiVector.h"
#include "Epetra_NumPyVector.h"
#include "Epetra_NumPyFEVector.h"

// NOX includes
#include "NOX_StatusTest_Generic.H"
#include "NOX_StatusTest_NormWRMS.H"
#include "NOX_Solver_LineSearchBased.H"
#include "NOX_Solver_TrustRegionBased.H"
#include "NOX_Solver_InexactTrustRegionBased.H"
#include "NOX_Solver_TensorBased.H"

//#include "NOX_Abstract_Group.H"
#include "NOX_Epetra_Group.H"
//#include "NOX_Epetra_Interface_Preconditioner.H"
//#include "NOX_Epetra_FiniteDifference.H"
//#include "NOX_Epetra_FiniteDifferenceColoring.H"
//#include "NOX_Epetra_LinearSystem_AztecOO.H"
//#include "NOX_Epetra_MatrixFree.H"
//#include "LOCA_MultiContinuation_AbstractGroup.H"
//#include "LOCA_MultiContinuation_FiniteDifferenceGroup.H"
//#include "LOCA_Homotopy_AbstractGroup.H"
//#include "LOCA_TurningPoint_MooreSpence_AbstractGroup.H"
//#include "LOCA_TurningPoint_MooreSpence_FiniteDifferenceGroup.H"
//#include "LOCA_TurningPoint_MinimallyAugmented_AbstractGroup.H"
//#include "LOCA_TurningPoint_MinimallyAugmented_FiniteDifferenceGroup.H"
//#include "LOCA_Pitchfork_MooreSpence_AbstractGroup.H"
//#include "LOCA_Pitchfork_MinimallyAugmented_AbstractGroup.H"
//#include "LOCA_TimeDependent_AbstractGroup.H"
//#include "LOCA_Hopf_MooreSpence_AbstractGroup.H"
//#include "LOCA_Hopf_MooreSpence_FiniteDifferenceGroup.H"
//#include "LOCA_Hopf_MinimallyAugmented_AbstractGroup.H"
//#include "LOCA_Hopf_MinimallyAugmented_FiniteDifferenceGroup.H"
//#include "LOCA_Abstract_Group.H"
//#include "LOCA_Abstract_TransposeSolveGroup.H"
#include "LOCA_Epetra.H"
#include "LOCA_Epetra_Group.H"

// Namespace flattening
using Teuchos::RCP;
using Teuchos::rcp;
%}

%ignore *::operator=;

// SWIG library includes
%include "stl.i"

// Trilinos interface support
%import "Teuchos.i"

// add the parent directory to the search path.
%pythoncode
{
import os.path, sys
currentDir,dummy = os.path.split(__file__)
sys.path.append(os.path.normpath(os.path.join(currentDir,"..")))
}

%teuchos_rcp_typemaps(LOCA::Epetra::Group)

//%import "NOX.Abstract.i"
%import "NOX.Epetra.__init__.i"
%import "LOCA.__init__.i"
//%import "LOCA.Abstract.i"
%import "LOCA_Abstract_Group.H"
%import "LOCA_Abstract_TransposeSolveGroup.H"

%pythoncode
%{
from NOX.Epetra import Group
%}
//%import "LOCA.MultiContinuation.i"
%import "LOCA.Epetra.Interface.i"

//////////////////////////////
// LOCA.Epetra.Group support //
//////////////////////////////

// temporarily ignore conflict-causing constructor.  TODO: fix this issue
%ignore LOCA::Epetra::Group::Group(Teuchos::RCP< LOCA::GlobalData > const &,Teuchos::ParameterList &,Teuchos::RCP<LOCA::Epetra::Interface::TimeDependentMatrixFree > const &,NOX::Epetra::Vector &,Teuchos::RCP< NOX::Epetra::LinearSystem > const &,Teuchos::RCP< NOX::Epetra::LinearSystem > const &,LOCA::ParameterVector const &);
%include "LOCA_Epetra.H"
%include "LOCA_Epetra_Group.H"
