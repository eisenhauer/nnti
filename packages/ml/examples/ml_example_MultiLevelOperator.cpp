#ifndef HAVE_CONFIG_H
#define HAVE_CONFIG_H
#endif

#include "ml_include.h"

#if defined(HAVE_ML_EPETRA) && defined(HAVE_ML_TEUCHOS) && defined(HAVE_ML_TRIUTILS) && defined(HAVE_ML_AZTECOO)

#ifdef HAVE_MPI
#include "mpi.h"
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"
#include "Epetra_IntVector.h"
#include "Epetra_SerialDenseVector.h"
#include "Epetra_Vector.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_VbrMatrix.h"
#include "Epetra_LinearProblem.h"
#include "Epetra_Time.h"
#include "AztecOO.h"

// includes required by ML
#include "Trilinos_Util_CommandLineParser.h"
#include "Trilinos_Util_CrsMatrixGallery.h"

#include "ml_epetra_utils.h"
#include "ml_MultiLevelOperator.h"
#include "Teuchos_ParameterList.hpp"

using namespace ML_Epetra;
using namespace Teuchos;
using namespace Trilinos_Util;

#include <iostream>

// MAIN DRIVER -- example of use of ML_Epetra::MultiLevelOperator
//
// This file reads a matrix in Harwell/Boeing format from the
// specified file, and solves the corresponding linear system using
// ML as a preconditioner. 
//
// From the command line, you may try something like that:
// $ mpirun -np 4 ./ml_example_epetra_operator.exe -matrix_name=<matrix>
//
// For more options for Trilinos_Util::CrsMatrixGallery, consult the
// Trilinos 4.0 tutorial

int main(int argc, char *argv[])
{
  
#ifdef EPETRA_MPI
  MPI_Init(&argc,&argv);
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif
  
  Epetra_Time Time(Comm);
  
  CommandLineParser CLP(argc,argv);
  // to read MatrixMarket matrices, simply change "hb" to "matrix_market"
  CrsMatrixGallery Gallery("hb", Comm);

  // initialize MatrixGallery object with options specified in the shell
  Gallery.Set(CLP);
  
  // get pointer to the linear system matrix
  Epetra_CrsMatrix* A = Gallery.GetMatrix();

  // get a pointer to the map
  const Epetra_Map* Map = Gallery.GetMap();

  // get a pointer to the linear system problem
  Epetra_LinearProblem* Problem = Gallery.GetLinearProblem();
  
  // Construct a solver object for this problem
  AztecOO solver(*Problem);
  
  // ================= MultiLevelOperator SECTION ========================

  // this is the "developers' way": each of the ML components
  // has to be properly created and configured. If you are
  // looking for an easier way to do this, try using the
  // ML_Epetra::MultiLevelPreconditioner class.

  int nLevels = 10;            // maximum number of levels
  int maxMgLevels = 6;         // 
  ML_Set_PrintLevel(10);       // print level (0 silent, 10 verbose)
  ML *ml_handle;               // container of all ML' data
  
  ML_Create(&ml_handle, maxMgLevels);

  // convert to epetra matrix, put finest matrix into
  // position maxMgLevels - 1 of the hierarchy
  EpetraMatrix2MLMatrix(ml_handle, maxMgLevels-1, A);
  
  // create an Aggregate object; this will contain information
  // about the aggregation process for each level
  ML_Aggregate *agg_object;
  ML_Aggregate_Create(&agg_object);
  
  // select coarsening scheme. 
  ML_Aggregate_Set_CoarsenScheme_Uncoupled(agg_object);

  // generate the hierarchy. We decided to use decreasing ordering;
  // one can also use ML_INCREASING (in this case, you need to replace
  // maxMgLevels-1 with 0 in call to EpetraMatrix2MLMatrix())
  nLevels = ML_Gen_MGHierarchy_UsingAggregation(ml_handle, maxMgLevels-1,
						ML_DECREASING, agg_object);

  // define the ID of the coarsest level
  int coarsestLevel = maxMgLevels - nLevels;

  // set up some smoothers. Here we suppose a symmetric problem
  int nits = 1;
  for(int level = maxMgLevels-1; level > coarsestLevel; level--)
    ML_Gen_Smoother_MLS(ml_handle, level, ML_BOTH, 30., 3);

  // simple coarse solver. You may want to use Amesos to access
  // to a large variety of direct solvers, serial and parallel
  ML_Gen_Smoother_GaussSeidel(ml_handle, coarsestLevel, ML_BOTH, 
                              nits, ML_DEFAULT);
 
  // generate the solver
  ML_Gen_Solver(ml_handle, ML_MGV, maxMgLevels-1, coarsestLevel);
 
  // create an Epetra_Operrator based on the previously created
  // hierarchy
  MultiLevelOperator MLPrec(ml_handle, Comm, *Map, *Map);

  // ========== End of MultiLevelOperator SECTION ========================
  
  // tell AztecOO to use this preconditioner, then solve
  solver.SetPrecOperator(&MLPrec);

  solver.SetAztecOption(AZ_solver, AZ_gmres);
  // set the AztecOO's output level
  solver.SetAztecOption(AZ_output, 16);

  // solve with AztecOO
  solver.Iterate(500, 1e-5);

  // check the real residual

  double residual, diff, res2;
  Gallery.ComputeResidual(&residual);
  Gallery.ComputeDiffBetweenStartingAndExactSolutions(&diff);
  
  (Gallery.GetExactSolution())->Norm2(&res2);
  if( Comm.MyPID()==0 ) {
    cout << "||b-Ax||_2 = " << residual << endl;
    cout << "||x_exact - x||_2 = " << diff/res2 << endl;
    cout << "Total Time = " << Time.ElapsedTime() << endl;
  }

  if (diff > 1e-5)
    exit(EXIT_FAILURE);

#ifdef EPETRA_MPI
  MPI_Finalize();
#endif

  exit(EXIT_SUCCESS);
  
}

#else

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  puts("Please configure ML with --enable-epetra --enable-teuchos");
  puts("--enable-aztecoo --enable-triutils");
  
  return 0;
}

#endif /* #if defined(ML_WITH_EPETRA) && defined(HAVE_ML_TEUCHOS) && defined(HAVE_ML_TRIUTILS) */

