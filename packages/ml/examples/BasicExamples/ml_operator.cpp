//@HEADER
// ************************************************************************
// 
//               ML: A Multilevel Preconditioner Package
//                 Copyright (2002) Sandia Corporation
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
// ************************************************************************
//@HEADER

// Goal of this example is to present the usage of the
// ML_Epetra::MultiLevelOperator class. This class should be used if the
// user wants to build all the ML components by him/herself (starting
// from an Epetra_RowMatrix), then use
// the resulting ML preconditioner within AztecOO.
//
// This file creates a matrix from the Triutils Gallery, 
// then solves the corresponding linear system using ML as a preconditioner. 
//
// From the command line, you may try something like that:
// $ mpirun -np 4 ./ml_operator.exe
//
// For more options for Trilinos_Util::CrsMatrixGallery, consult the
// Trilinos 4.0 tutorial
//
// \author Marzio Sala, SNL 9214
//
// \date Last modified on 14-Jun-05

#include "ml_include.h"

#if defined(HAVE_ML_EPETRA) && defined(HAVE_ML_TRIUTILS) && defined(HAVE_ML_AZTECOO)

#ifdef HAVE_MPI
#include "mpi.h"
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"
#include "Epetra_Vector.h"
#include "Epetra_LinearProblem.h"
#include "Epetra_Time.h"
#include "AztecOO.h"
#include "Trilinos_Util_CrsMatrixGallery.h"
#include "ml_epetra_utils.h"
#include "ml_MultiLevelOperator.h"

using namespace ML_Epetra;
using namespace Trilinos_Util;

// =========== //
// main driver //
// =========== //

int main(int argc, char *argv[])
{
  
#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif
  
  Epetra_Time Time(Comm);
  
  // Creates a matrix corresponding to a 2D Laplacian on a
  // Cartesian grid, with nx * nx rows.
  CrsMatrixGallery Gallery("laplace_2d", Comm);
  int nx = 10; 
  Gallery.Set("problem_size", nx * nx);

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
  ML* ml_handle;               // container of all ML' data
  
  ML_Create(&ml_handle, maxMgLevels);

  // convert to epetra matrix, put finest matrix into
  // position maxMgLevels - 1 of the hierarchy. NOTE: the matrix
  // is only wrapped (that is, a suitable getrow() function is used),
  // so data in the linear system matrix are NOT replicated.
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
  for (int level = maxMgLevels-1; level > coarsestLevel; level--)
    ML_Gen_Smoother_MLS(ml_handle, level, ML_BOTH, 30., 3);

  // simple coarse solver. You may want to use Amesos to access
  // to a large variety of direct solvers, serial and parallel
  ML_Gen_Smoother_GaussSeidel(ml_handle, coarsestLevel, ML_BOTH, 
                              nits, ML_DEFAULT);
 
  // generate the solver
  ML_Gen_Solver(ml_handle, ML_MGV, maxMgLevels-1, coarsestLevel);
 
  // create an Epetra_Operator based on the previously created
  // hierarchy
  MultiLevelOperator MLPrec(ml_handle, Comm, *Map, *Map);

  // ========== End of MultiLevelOperator SECTION ========================
  
  // tell AztecOO to use ML as preconditioner with GMRES, output
  // every 16 iterations, then solve with 500 maximum iterations and
  // tolerance of 1e-5.
  
  solver.SetPrecOperator(&MLPrec);
  solver.SetAztecOption(AZ_solver, AZ_gmres);
  solver.SetAztecOption(AZ_output, 16);
  solver.Iterate(500, 1e-5);

  // The following is a check to verify that the real residual is small,
  // using methods of the Gallery.

  double residual, diff, res2;
  Gallery.ComputeResidual(&residual);
  Gallery.ComputeDiffBetweenStartingAndExactSolutions(&diff);
  
  (Gallery.GetExactSolution())->Norm2(&res2);
  if (Comm.MyPID() == 0) 
  {
    cout << "||b-Ax||_2 = " << residual << endl;
    cout << "||x_exact - x||_2 = " << diff/res2 << endl;
    cout << "Total Time = " << Time.ElapsedTime() << endl;
  }

  ML_Aggregate_Destroy(&agg_object);
  ML_Destroy(&ml_handle);

  // for testing purposes only
  if (diff > 1e-5)
    exit(EXIT_FAILURE);

#ifdef HAVE_MPI
  MPI_Finalize();
#endif

  exit(EXIT_SUCCESS);
  
}

#else

#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_MPI
#include "mpi.h"
#endif

int main(int argc, char *argv[])
{
#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
#endif

  puts("Please configure ML with:");
  puts("--enable-epetra");
  puts("--enable-aztecoo");
  puts("--enable-triutils");

#ifdef HAVE_MPI
  MPI_Finalize();
#endif
  
  exit(EXIT_SUCCESS);
}

#endif /* #if defined(HAVE_ML_EPETRA) && defined(HAVE_ML_TRIUTILS) && defined(HAVE_ML_AZTECOO) */
