
// @HEADER
// ***********************************************************************
// 
//            Trilinos: An Object-Oriented Solver Framework
//                 Copyright (2001) Sandia Corporation
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

#include "Ifpack_ConfigDefs.h"
#if defined(HAVE_IFPACK_AZTECOO) && defined(HAVE_IFPACK_TEUCHOS) && defined(HAVE_IFPACK_TRIUTILS)

#include "Epetra_ConfigDefs.h"
#ifdef HAVE_MPI
#include "mpi.h"
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Comm.h"
#include "Epetra_Map.h"
#include "Epetra_Time.h"
#include "Epetra_BlockMap.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Vector.h"
#include "Epetra_Export.h"
#include "AztecOO.h"
#include "Trilinos_Util.h"
#include "Trilinos_Util_CrsMatrixGallery.h"
#include "Ifpack_CrsIct.h"
#include "Ifpack.h"

// function for fancy output

string toString(const int& x) {
  char s[100];
  sprintf(s, "%d", x);
  return string(s);
}

string toString(const double& x) {
  char s[100];
  sprintf(s, "%g", x);
  return string(s);
}

using namespace Trilinos_Util;

// main driver

int main(int argc, char *argv[]) {

#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
  Epetra_MpiComm Comm (MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif

  int MyPID = Comm.MyPID();
  bool verbose = false; 
  if (MyPID==0) verbose = true;

  // size of the global matrix.
  const int NumPoints = 900;

  CrsMatrixGallery Gallery("laplace_2d", Comm);
  Gallery.Set("problem_size", NumPoints);
  Epetra_CrsMatrix* A = dynamic_cast<Epetra_CrsMatrix*>(Gallery.GetMatrix());
  Epetra_MultiVector* LHS = Gallery.GetStartingSolution();
  Epetra_MultiVector* RHS = Gallery.GetRHS();

  // ============================ //
  // Construct ILU preconditioner //
  // ---------------------------- //

  // I wanna test funky values to be sure that they have the same
  // influence on the algorithms, both old and new
  int    LevelFill = 2;
  double DropTol = 0.3333;
  double Condest;
  
  Ifpack_CrsIct * ICT = NULL;
  ICT = new Ifpack_CrsIct(*A,DropTol,LevelFill);
  ICT->SetAbsoluteThreshold(0.00123);
  ICT->SetRelativeThreshold(0.9876);
  // Init values from A
  ICT->InitValues(*A);
  // compute the factors
  ICT->Factor();
  // and now estimate the condition number
  ICT->Condest(false,Condest);
  
  if( Comm.MyPID() == 0 ) {
    cout << "Condition number estimate (level-of-fill = "
	 << LevelFill <<  ") = " << Condest << endl;
  }

  // Define label for printing out during the solve phase
  string label = "Ifpack_CrsIct Preconditioner: LevelFill = " + toString(LevelFill) + 
                                                 " Overlap = 0"; 
  ICT->SetLabel(label.c_str());
  
  // Here we create an AztecOO object
  LHS->PutScalar(0.0);

  int Niters = 1200;

  AztecOO solver;
  solver.SetUserMatrix(A);
  solver.SetLHS(LHS);
  solver.SetRHS(RHS);
  solver.SetAztecOption(AZ_solver,AZ_cg);
  solver.SetPrecOperator(ICT);
  solver.SetAztecOption(AZ_output, 16); 
  solver.Iterate(Niters, 5.0e-5);

  int OldIters = solver.NumIters();


  if (ICT!=0) delete ICT;
				       
  // now rebuild the same preconditioner using ICT, we expect the same
  // number of iterations

  Ifpack Factory;
  Ifpack_Preconditioner* Prec = Factory.Create("IC", A);

  Teuchos::ParameterList List;
  List.get("fact: level-of-fill", 2);
  List.get("fact: drop tolerance", 0.3333);
  List.get("fact: absolute threshold", 0.00123);
  List.get("fact: relative threshold", 0.9876);
  List.get("fact: relaxation value", 0.0);

  IFPACK_CHK_ERR(Prec->SetParameters(List));
  IFPACK_CHK_ERR(Prec->Compute());

  // Here we create an AztecOO object
  LHS->PutScalar(0.0);

  solver.SetUserMatrix(A);
  solver.SetLHS(LHS);
  solver.SetRHS(RHS);
  solver.SetAztecOption(AZ_solver,AZ_cg);
  solver.SetPrecOperator(Prec);
  solver.SetAztecOption(AZ_output, 16); 
  solver.Iterate(Niters, 5.0e-5);

  int NewIters = solver.NumIters();

  assert (OldIters == NewIters);

  if (Prec!=0) delete Prec;

#ifdef HAVE_MPI
  MPI_Finalize() ;
#endif

return 0 ;
}

#else

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  puts("Please configure Didasko with:\n"
       "--enable-epetra\n"
       "--enable-teuchos\n"
       "--enable-amesos");

  return 0;
}
#endif
