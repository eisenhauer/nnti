// @HEADER
// ***********************************************************************
// 
//                IFPACK
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

#include "Ifpack_ConfigDefs.h"
#if defined(HAVE_IFPACK_AZTECOO) && defined(HAVE_IFPACK_TEUCHOS)
#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_CrsMatrix.h"
#include "Epetra_Vector.h"
#include "Epetra_LinearProblem.h"
#include "Epetra_Time.h"
#include "Trilinos_Util_CrsMatrixGallery.h"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_CommandLineProcessor.hpp"
#include "Ifpack_AdditiveSchwarz.h"
#include "AztecOO.h"
#include "Ifpack_Graph_Epetra_RowMatrix.h"
#include "Ifpack_PointRelaxation.h"
#include "Ifpack_IC.h"
#include "Ifpack_ILU.h"
#include "Ifpack_Amesos.h"

bool verbose = false;

using namespace Trilinos_Util;

bool CompareWithAztecOO(Epetra_LinearProblem* Problem, const string what,
                       int Overlap, int ival)
{

  AztecOO AztecOOSolver(*Problem);
  AztecOOSolver.SetAztecOption(AZ_solver,AZ_gmres);
  AztecOOSolver.SetAztecOption(AZ_output,AZ_none);
  AztecOOSolver.SetAztecOption(AZ_overlap,Overlap);
  AztecOOSolver.SetAztecOption(AZ_graph_fill,ival);
  AztecOOSolver.SetAztecOption(AZ_poly_ord, ival);
  AztecOOSolver.SetAztecParam(AZ_drop, 0.0);
  AztecOOSolver.SetAztecParam(AZ_athresh, 0.0);
  AztecOOSolver.SetAztecParam(AZ_rthresh, 0.0);

  Epetra_MultiVector& RHS = *(Problem->GetRHS());
  Epetra_MultiVector& LHS = *(Problem->GetLHS());
  Epetra_RowMatrix* A = Problem->GetMatrix();

  LHS.Random();
  A->Multiply(false,LHS,RHS);

  Teuchos::ParameterList List;
  List.set("fact: level-of-fill", ival);
  List.set("relaxation: sweeps", ival);
  List.set("relaxation: damping factor", 1.0);
  List.set("relaxation: zero starting solution", true);
 
  //default combine mode is as for AztecOO
  List.set("schwarz: combine mode", Zero);

  Epetra_Time Time(A->Comm());

  Ifpack_Preconditioner* Prec = 0;
  
  if (what == "Jacobi") {
    Prec = new Ifpack_PointRelaxation(A);
    List.set("relaxation: type", "Jacobi");
    AztecOOSolver.SetAztecOption(AZ_precond,AZ_Jacobi);
    AztecOOSolver.SetAztecOption(AZ_reorder,0);
  }
  else if (what == "IC no reord") {
    Prec = new Ifpack_AdditiveSchwarz<Ifpack_IC>(A,Overlap);
    AztecOOSolver.SetAztecOption(AZ_precond,AZ_dom_decomp);
    AztecOOSolver.SetAztecOption(AZ_subdomain_solve,AZ_icc);
    AztecOOSolver.SetAztecOption(AZ_reorder,0);
  }
  else if (what == "IC reord") {
    Prec = new Ifpack_AdditiveSchwarz<Ifpack_IC>(A,Overlap);
    List.set("schwarz: use reordering", true);
    AztecOOSolver.SetAztecOption(AZ_precond,AZ_dom_decomp);
    AztecOOSolver.SetAztecOption(AZ_subdomain_solve,AZ_icc);
    AztecOOSolver.SetAztecOption(AZ_reorder,1);
  }
  else if (what == "ILU no reord") {
    Prec = new Ifpack_AdditiveSchwarz<Ifpack_ILU>(A,Overlap);
    AztecOOSolver.SetAztecOption(AZ_precond,AZ_dom_decomp);
    AztecOOSolver.SetAztecOption(AZ_subdomain_solve,AZ_ilu);
    AztecOOSolver.SetAztecOption(AZ_reorder,0);
  }
  else if (what == "ILU reord") {
    Prec = new Ifpack_AdditiveSchwarz<Ifpack_ILU>(A,Overlap);
    List.set("schwarz: use reordering", true);
    AztecOOSolver.SetAztecOption(AZ_precond,AZ_dom_decomp);
    AztecOOSolver.SetAztecOption(AZ_subdomain_solve,AZ_ilu);
    AztecOOSolver.SetAztecOption(AZ_reorder,1);
  }
  else if (what == "LU") {
    Prec = new Ifpack_AdditiveSchwarz<Ifpack_Amesos>(A,Overlap);
    List.set("amesos: solver type", "Klu");
    AztecOOSolver.SetAztecOption(AZ_precond,AZ_dom_decomp);
    AztecOOSolver.SetAztecOption(AZ_subdomain_solve,AZ_lu);
  }
  else {
    cerr << "Option not recognized" << endl;
    exit(EXIT_FAILURE);
  }

  // ==================================== //
  // Solve with AztecOO's preconditioners //
  // ==================================== //

  LHS.PutScalar(0.0);

  Time.ResetStartTime();
  AztecOOSolver.Iterate(150,1e-5);

  if (verbose) {
    cout << endl;
    cout << "==================================================" << endl;
    cout << "Testing `" << what << "', Overlap = "
         << Overlap << ", ival = " << ival << endl;
    cout << endl;
    cout << "[AztecOO] Total time = " << Time.ElapsedTime() << " (s)" << endl;
    cout << "[AztecOO] Residual   = " << AztecOOSolver.TrueResidual() << " (s)" << endl;
    cout << "[AztecOO] Iterations = " << AztecOOSolver.NumIters() << endl;
    cout << endl;
  }

  int AztecOOPrecIters = AztecOOSolver.NumIters();

  // =========================================== //
  // Create the IFPACK preconditioner and solver //
  // =========================================== //
 
  Epetra_Time Time2(A->Comm());
  assert(Prec != 0);
  IFPACK_CHK_ERR(Prec->SetParameters(List));

  Time.ResetStartTime();
  IFPACK_CHK_ERR(Prec->Initialize());
  if (verbose)
    cout << "[IFPACK] Time for Initialize() = "
         << Time.ElapsedTime() << " (s)" << endl;

  Time.ResetStartTime();
  IFPACK_CHK_ERR(Prec->Compute());
  if (verbose)
    cout << "[IFPACK] Time for Compute() = "
         << Time.ElapsedTime() << " (s)" << endl;


  AztecOOSolver.SetPrecOperator(Prec);

  LHS.PutScalar(0.0);

  Time.ResetStartTime();
  AztecOOSolver.Iterate(150,1e-5);

  if (verbose) {
    cout << "[IFPACK] Total time = " << Time2.ElapsedTime() << " (s)" << endl;
    cout << "[IFPACK] Residual   = " << AztecOOSolver.TrueResidual() << " (s)" << endl;
    cout << "[IFPACK] Iterations = " << AztecOOSolver.NumIters() << endl;
    cout << endl;
  }

  int IFPACKPrecIters = AztecOOSolver.NumIters();

  delete Prec;

  if (IFPACK_ABS(AztecOOPrecIters - IFPACKPrecIters) > 3) {
    cerr << "TEST FAILED (" << AztecOOPrecIters << " != " 
         << IFPACKPrecIters << ")" << endl;
    return(false);
  }
  else
    return(true);

}

// ====================================================================== 
int main(int argc, char *argv[])
{

#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif

  if (Comm.MyPID() == 0) {
    for (int i = 1 ; i < argc ; ++i) {
      if (strcmp(argv[i], "-v") == 0)
        verbose = true;
    }
  }

  // process the command line
  Teuchos::CommandLineProcessor CLP;
  // matrix name
  string MatrixName = "laplace_3d";
  // global size
  int NumPoints = 27000;
  CLP.setOption("matrix", &MatrixName, "Matrix name for Gallery (ex: laplace_2d)");
  CLP.setOption("size", &NumPoints, "Size of the problem. Note: this may need to be a square/cube depending on specified matrix name");

  CLP.throwExceptions(false);
  CLP.parse(argc,argv);
  
  CrsMatrixGallery Gallery(MatrixName,Comm);
  Gallery.Set("problem_size", NumPoints);
  Gallery.Set("map_type", "linear");

  Epetra_LinearProblem* Problem = Gallery.GetLinearProblem();

  int TestPassed = true;

  // Jacobi as in AztecOO (no overlap)
  for (int ival = 1 ; ival < 10 ; ival += 3) {
    TestPassed = TestPassed && 
      CompareWithAztecOO(Problem,"Jacobi",0,ival);
  }

  // AztecOO with IC and overlap complains, also with
  // large fill-ins (in parallel)
  TestPassed = TestPassed && 
    CompareWithAztecOO(Problem,"IC no reord",0,0);
  TestPassed = TestPassed && 
    CompareWithAztecOO(Problem,"IC reord",0,0);

  vector<string> Tests;
  // now test solvers that accept overlap
  //Tests.push_back("ILU no reord");
  //Tests.push_back("ILU reord");
  // following requires --enable-aztecoo-azlu
  //Tests.push_back("LU");

  for (unsigned int i = 0 ; i < Tests.size() ; ++i) {
    for (int overlap = 0 ; overlap < 1 ; overlap += 2) {
      for (int ival = 0 ; ival < 10 ; ival += 4)
        TestPassed = TestPassed && 
          CompareWithAztecOO(Problem,Tests[i],overlap,ival);
    }
  }

  if (!TestPassed) {
    cerr << "TEST FAILED!!!!!" << endl;
    exit(EXIT_FAILURE);
  }

#ifdef HAVE_MPI
  MPI_Finalize() ; 
#endif
  cout << "TEST PASSED" << endl;

  exit(EXIT_SUCCESS);
}

#else

#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif

int main(int argc, char *argv[])
{

#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
  Epetra_MpiComm Comm( MPI_COMM_WORLD );
#else
  Epetra_SerialComm Comm;
#endif

  puts("please configure IFPACK with --eanble-aztecoo --enable-teuchos");
  puts("to run this test");

#ifdef HAVE_MPI
  MPI_Finalize() ;
#endif
  return(EXIT_SUCCESS);
}

#endif
