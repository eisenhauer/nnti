//@HEADER
/*
************************************************************************

              Epetra: Linear Algebra Services Package 
                Copyright (2001) Sandia Corporation

Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
license for use of this work by or on behalf of the U.S. Government.

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.
 
This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.
 
You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA
Questions? Contact Michael A. Heroux (maherou@sandia.gov) 

************************************************************************
*/
//@HEADER

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "Petra_Comm.h"
#include "Petra_Map.h"
#include "Petra_Time.h"
#include "Petra_RDP_MultiVector.h"
#include "Petra_RDP_Vector.h"
#include "Petra_RDP_CRS_Matrix.h"
#include "Trilinos_LinearProblem.h"
#ifdef PETRA_MPI
#include "mpi.h"
#endif
#ifndef __cplusplus
#define __cplusplus
#endif

// prototype
#include"basis.h"

int main(int argc, char *argv[])
{
  int ierr = 0, i, j;
  bool debug = false;

#ifdef PETRA_MPI

  // Initialize MPI

  MPI_Init(&argc,&argv);
  int size, rank; // Number of MPI processes, My process ID

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

#else

  int size = 1; // Serial case (not using MPI)
  int rank = 0;

#endif


#ifdef PETRA_MPI
  Petra_Comm & Comm = *new Petra_Comm( MPI_COMM_WORLD );
#else
  Petra_Comm & Comm = *new Petra_Comm();
#endif

  int MyPID = Comm.MyPID();
  int NumProc = Comm.NumProc();

  // Get the number of local equations from the command line
  if (argc!=2) { 
    cout << "Usage: " << argv[0] << " number_of_elements" << endl;
    exit(1);
  }
  int NumGlobalElements = atoi(argv[1])+1;
  int IndexBase = 0;

  if (NumGlobalElements < NumProc) {
    cout << "numGlobalBlocks = " << NumGlobalElements 
	 << " cannot be < number of processors = " << NumProc << endl;
    exit(1);
  }

  // Construct a Source Map that puts approximately the same 
  // Number of equations on each processor in uniform global ordering

  Petra_Map& StandardMap = *new Petra_Map(NumGlobalElements, 0, Comm);
  int NumMyElements = StandardMap.NumMyElements();
  int * StandardMyGlobalElements = new int[NumMyElements];
  StandardMap.MyGlobalElements(StandardMyGlobalElements);

  // Create a standard Petra_CRS_Graph
  //Petra_CRS_Graph& StandardGraph = *new Petra_CRS_Graph(Copy, StandardMap, 3);
  //assert(!StandardGraph.IndicesAreGlobal());
  //assert(!StandardGraph.IndicesAreLocal());
  
  // Construct an Overlapped Map of StandardMap that include 
  // the endpoints from two neighboring processors.

  int OverlapNumMyElements;
  int OverlapMinMyGID;

  OverlapNumMyElements = NumMyElements + 2;
  if ((MyPID==0)||(MyPID==NumProc-1)) OverlapNumMyElements--;

  if (MyPID==0) OverlapMinMyGID = StandardMap.MinMyGID();
  else OverlapMinMyGID = StandardMap.MinMyGID()-1;

  int * OverlapMyGlobalElements = new int[OverlapNumMyElements];

  for (i=0; i< OverlapNumMyElements; i++) 
                       OverlapMyGlobalElements[i] = OverlapMinMyGID + i;

  Petra_Map& OverlapMap = *new Petra_Map(-1, OverlapNumMyElements, 
					 OverlapMyGlobalElements, 0, Comm);
  
  int pS=3;
  int pO=3;
  // Create Linear Objects for Solve
  Petra_RDP_Vector& du = *new Petra_RDP_Vector(StandardMap);
  Petra_RDP_Vector& rhs = *new Petra_RDP_Vector(StandardMap);
  Petra_RDP_Vector& soln = *new Petra_RDP_Vector(StandardMap);
  Petra_RDP_CRS_Matrix& A = *new Petra_RDP_CRS_Matrix(Copy, StandardMap, pS);

  // Create Linear Objects for Fill (Solution Vector)
  Petra_RDP_Vector& rhs1 = *new Petra_RDP_Vector(OverlapMap);
  Petra_RDP_Vector& u = *new Petra_RDP_Vector(OverlapMap);
  Petra_RDP_Vector& x = *new Petra_RDP_Vector(OverlapMap);
  Petra_RDP_CRS_Matrix& A1 = *new Petra_RDP_CRS_Matrix(Copy, OverlapMap, pO);

  // Initialize Solution
  i=u.PutScalar(1.0);
  i=soln.PutScalar(1.0);

  int row;
  double eta;
  double *xx = new double[2];
  double *uu = new double[2];
  double jac;
  int column;
  int *indicies = new int[3];
  double *RowValues = new double[OverlapMap.NumGlobalElements()];
  Basis basis;
  double residual, difference;
  double relTol=1.0e-4;
  double absTol=1.0e-9;

  // Create the nodal position variables
  double Length=1.0;
  double dx=Length/((double) NumGlobalElements-1);
  for (i=0; i < OverlapNumMyElements; i++) {
    x[i]=dx*((double) OverlapMinMyGID+i);
  }
  
  // Begin Nonlinear solver LOOP ************************************
  
  for (int NLS=0; NLS<2; NLS++) {


  i=A1.PutScalar(0.0);
  i=A.PutScalar(0.0);
  i=rhs1.PutScalar(0.0);
  i=rhs.PutScalar(0.0);

  // Loop Over # of Finite Elements on Processor
  for (int ne=0; ne < OverlapNumMyElements-1; ne++) {
    
    // Loop Over Gauss Points (5th order GQ)
    for(int gp=0; gp < 3; gp++) {
      xx[0]=x[ne];
      xx[1]=x[ne+1];
      uu[0]=u[ne];
      uu[1]=u[ne+1];
      basis.getBasis(gp, xx, uu);
	            
      // Loop over Nodes in Element
      for (i=0; i< 2; i++) {
	  rhs1[ne+i]+=basis.wt*basis.dx
	    *((1.0/(basis.dx*basis.dx))*basis.duu*
			   basis.dphide[i]+basis.uu*basis.uu*basis.phi[i]);
	  //  printf("Proc=%d, GID=%d, rhs=%e owned%d\n",MyPID ,
	  //OverlapMap.GID(i),rhs[i],StandardMap.MyGID(OverlapMap.GID(i)));

	// Loop over Trial Functions
	for(j=0;j < 2; j++) {
	  jac=basis.wt*basis.dx*((1.0/(basis.dx*basis.dx))*basis.dphide[j]*
	       basis.dphide[i]+2.0*basis.uu*basis.phi[j]*basis.phi[i]);
	  row=OverlapMap.GID(ne+i);
	  column=OverlapMap.GID(ne+j);
	  ierr=A1.SumIntoGlobalValues(row, 1, &jac, &column);
	  if (ierr!=0) {	    
	    // printf("SumInto failed at (%d,%d)!!\n",row,column);
	    ierr=A1.InsertGlobalValues(row, 1, &jac, &column);
	    // if (ierr==0) printf("Insert SUCCEEDED at (%d,%d)!!\n",row,column);
	  } //else if (ierr==0) 
	    // printf("SumInto SUCCEEDED at (%d,%d)!!\n",row,column);
	  
	}
      }
    }
  }

  Comm.Barrier();

  Petra_Import & Importer = *new Petra_Import(StandardMap, OverlapMap);
  assert(rhs.Import(rhs1, Importer, Insert)==0);
  assert(A.Import(A1, Importer, Insert)==0);
  delete &Importer;

  // Insert Boundary Conditions
  // U(0)=1.0
  if (MyPID==0) {
    u[0]=1.0;
    rhs[0]=0.0;
    column=0;
    jac=1.0;
    A.ReplaceGlobalValues(0, 1, &jac, &column);
    column=1;
    jac=0.0;
    A.ReplaceGlobalValues(0, 1, &jac, &column);
  }

  Comm.Barrier();

  assert(A.TransformToLocal()==0);

  /*
  // Print Matrix
  int StandardNumMyRows = A.NumMyRows();
  int * StandardIndices; 
  int StandardNumEntries; 
  double * StandardValues;
  for (i=0; i< StandardNumMyRows; i++) {
    A.ExtractMyRowView(i, StandardNumEntries, 
				    StandardValues, StandardIndices);
    for (j=0; j < StandardNumEntries; j++) {
      printf("MyPID=%d, J[%d,%d]=%e\n",MyPID,i,j,StandardValues[j]);
    }
  }
  */

  // check if Converged   
  ierr=rhs.Norm2(&residual);
  ierr=du.Norm2(&difference);
  if (MyPID==0) printf("\n***********************************************\n");
  if (MyPID==0) printf("Iteration %d  Residual L2=%e   Update L2=%e\n"
			 ,NLS,residual,difference);
  if (MyPID==0) printf("***********************************************\n");  
  if ((residual < absTol)&&(difference < relTol)) {
    if (MyPID==0) printf("\n\nConvergence Achieved!!!!\n");
    return 0;
  }    
  
  Trilinos_LinearProblem *Problem = new  Trilinos_LinearProblem(&A,&du,&rhs);
  Problem->SetPDL(hard);
  Problem->Iterate(400, 1.0e-8);
  delete Problem;

  // Update Solution    
  for (i=0;i<NumMyElements;i++) soln[i] -= du[i];
	 
  Petra_Import & Importer2 = *new Petra_Import(OverlapMap, StandardMap);
  assert(u.Import(soln, Importer2, Insert)==0);
  delete &Importer2;
  
  for (i=0;i<OverlapNumMyElements;i++) 
    printf("Proc=%d GID=%d u=%e soln=%e\n",MyPID,
	   OverlapMap.GID(i),u[i],soln[i]);

  } // End NLS Loop *****************************************************
 


  delete &OverlapMap;
  delete [] OverlapMyGlobalElements;

  //delete &StandardGraph;
  delete &StandardMap;
  delete [] StandardMyGlobalElements;

  delete &Comm;

#ifdef PETRA_MPI
  MPI_Finalize() ;
#endif

/* end main
*/
return ierr ;
}
