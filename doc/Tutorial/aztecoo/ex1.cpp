
//@HEADER
// ************************************************************************
// 
//          Trilinos: An Object-Oriented Solver Framework
//              Copyright (2001) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//   
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//   
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
// 
// ************************************************************************
//@HEADER

// Trilinos Tutorial
// -----------------
// Solve a 2D Laplacian problem
//
// (output reported at the end of the file)
//
// Marzio Sala, SNL, 9214, 20-Nov-2003

#include "Epetra_config.h"
#ifdef HAVE_MPI
#include "mpi.h"
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"
#include "Epetra_Vector.h"
#include "Epetra_CrsMatrix.h"
#include "AztecOO.h"

// external function
void  get_neighbours( const int i, const int nx, const int ny,
		      int & left, int & right, 
		      int & lower, int & upper);

// =========== //
// main driver //
// =========== //

int main(int argc, char *argv[])
{

#ifdef HAVE_MPI
  MPI_Init(&argc, &argv);
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif

  // number of nodes along the x- and y-axis
  int nx = 5;
  int ny = 6;
  int NumGlobalElements = nx * ny;

  // create a linear map
  Epetra_Map Map(NumGlobalElements,0,Comm);
  
  // local number of rows
  int NumMyElements = Map.NumMyElements();
  // get update list
  int * MyGlobalElements = new int [NumMyElements];
  Map.MyGlobalElements( MyGlobalElements );

  // Create a Epetra_Matrix with 5 nonzero per rows
  
  Epetra_CrsMatrix A(Copy,Map,5);

  // Add  rows one-at-a-time
  // Need some vectors to help

  double Values[4];
  int Indices[4];
  int NumEntries;
  int left, right, lower, upper;
  double diag = 4.0;
  
  for( int i=0 ; i<NumMyElements; ++i ) {
    int NumEntries=0;
    get_neighbours(  MyGlobalElements[i], nx, ny, 
		     left, right, lower, upper);
    if( left != -1 ) {
	Indices[NumEntries] = left;
	Values[NumEntries] = -1.0;
	++NumEntries;
    }
    if( right != -1 ) {
      Indices[NumEntries] = right;
      Values[NumEntries] = -1.0;
      ++NumEntries;
    }
    if( lower != -1 ) {
      Indices[NumEntries] = lower;
      Values[NumEntries] = -1.0;
      ++NumEntries;
    }
    if( upper != -1 ) {
      Indices[NumEntries] = upper;
      Values[NumEntries] = -1.0;
      ++NumEntries;
    }
    // put the off-diagonal entries
    assert(A.InsertGlobalValues(MyGlobalElements[i], NumEntries, 
				Values, Indices)==0);
    // Put in the diagonal entry
    assert(A.InsertGlobalValues(MyGlobalElements[i], 1, 
				&diag, MyGlobalElements+i)==0);
  }
  
  // Finish up
  assert(A.TransformToLocal()==0);

  // create x and b vectors
  Epetra_Vector x(Map);
  Epetra_Vector b(Map);
  b.PutScalar(1.0);

  // ==================== AZTECOO INTERFACE ======================
  
  // create linear problem
  Epetra_LinearProblem Problem(&A,&x,&b);
  // create AztecOO instance
  AztecOO Solver(Problem);

  Solver.SetAztecOption( AZ_precond, AZ_Jacobi );
  
  Solver.Iterate(1000,1E-9);

  // ==================== END OF AZTECOO INTERFACE ================
  
  if( Comm.MyPID() == 0 ) {
    cout << "Solver performed " << Solver.NumIters() 
	 << "iterations.\n";
    cout << "Norm of the true residual = " << Solver.TrueResidual() << endl;
  }
  
#ifdef HAVE_MPI
  MPI_Finalize();
#endif

  return( EXIT_SUCCESS );

}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

void  get_neighbours( const int i, const int nx, const int ny,
		      int & left, int & right, 
		      int & lower, int & upper) 
{

  int ix, iy;
  ix = i%nx;
  iy = (i - ix)/nx;

  if( ix == 0 ) 
    left = -1;
  else 
    left = i-1;
  if( ix == nx-1 ) 
    right = -1;
  else
    right = i+1;
  if( iy == 0 ) 
    lower = -1;
  else
    lower = i-nx;
  if( iy == ny-1 ) 
    upper = -1;
  else
    upper = i+nx;

  return;

}

/*

Output of this program (NOTE: the output produced by our code can be
slightly different)

[msala:aztecoo]> mpirun -np 2 ./ex1.exe

                *******************************************************
                ***** Preconditioned GMRES solution
                ***** 1 step block Jacobi
                ***** No scaling
                *******************************************************

                iter:    0           residual = 1.000000e+00
                iter:    1           residual = 6.798693e-01
                iter:    2           residual = 4.028772e-01
                iter:    3           residual = 1.824286e-01
                iter:    4           residual = 5.684696e-02
                iter:    5           residual = 2.070778e-02
                iter:    6           residual = 4.119793e-03
                iter:    7           residual = 1.386616e-04
                iter:    8           residual = 1.272363e-05
                iter:    9           residual = 4.683774e-37


                Solution time: 0.413255 (sec.)
                total iterations: 9
Solver performed 9iterations.
Norm of the true residual = 7.91613e-15

*/
