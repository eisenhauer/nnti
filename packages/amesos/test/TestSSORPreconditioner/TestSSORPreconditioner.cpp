// @HEADER
// ***********************************************************************
// 
//                Amesos: Direct Sparse Solver Package
//                 Copyright (2004) Sandia Corporation
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

// test the Amesos_Preconditioner class
#include "Amesos_ConfigDefs.h"
#include "Amesos.h"
#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_CrsMatrix.h"
#include "Epetra_Vector.h"
#include "Trilinos_Util_CrsMatrixGallery.h"
#include "Teuchos_ParameterList.hpp"
#include "Amesos_SSORPreconditioner.h"
#include "AztecOO.h"

using namespace Trilinos_Util;

int main(int argc, char *argv[])
{

#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
  Epetra_MpiComm Comm( MPI_COMM_WORLD );
#else
  Epetra_SerialComm Comm;
#endif

  // size of the global matrix. 
  const int NumPoints = 900;

  CrsMatrixGallery Gallery("laplace_2d", Comm);
  Gallery.Set("problem_size", NumPoints);
  Gallery.Set("map_type", "linear");

  // The following methods of CrsMatrixGallery are used to get pointers
  // to internally stored Epetra_RowMatrix and Epetra_LinearProblem.

  Epetra_RowMatrix* A = Gallery.GetMatrix();
  Epetra_LinearProblem* Problem = Gallery.GetLinearProblem();
  Epetra_MultiVector* LHS = Problem->GetLHS();
  Epetra_MultiVector* RHS = Problem->GetRHS();

  Epetra_Operator* SSORPrec;
  
  // ========================= //
  // create the AztecOO solver //
  // ========================= //

  AztecOO AztecOOSolver(*Problem);

  AztecOOSolver.SetAztecOption(AZ_solver,AZ_cg_condnum);
  AztecOOSolver.SetAztecOption(AZ_output,16);

  // ============================== //
  // create the SSOR preconditioner //
  // ============================== //

  Teuchos::ParameterList List;
  bool UseTranspose = false;

  List.set("sweeps", 5);
  List.set("omega", 0.67);
  List.set("debug", false);

  SSORPrec = 
    new Amesos_SSORPreconditioner(A, List);

  AztecOOSolver.SetPrecOperator(SSORPrec);

  // solver. The solver should converge in one iteration,
  // or maximum two (numerical errors)
  AztecOOSolver.Iterate(1550,1e-9);

  // some output
  if( Comm.MyPID() == 0 ) {
    cout << "Solver performed " << AztecOOSolver.NumIters()
      << " iterations.\n";
    cout << "Norm of the true residual = " << AztecOOSolver.TrueResidual() << endl;
  }

#ifdef HAVE_MPI
  MPI_Finalize() ; 
#endif

}

