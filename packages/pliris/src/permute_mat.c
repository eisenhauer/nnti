/*
//@HEADER
// ************************************************************************
//
//               Pliris: Parallel Dense Solver Package
//                 Copyright 2004 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// Export of this program may require a license from the United States
// Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
//@HEADER

Authors:

Brian Driessen
Sandia National Labs
(505)-844-9297
bjdries@sandia.gov

Joseph D. Kotulski
Sandia National Labs
(505)-845-7955
jdkotul@sandia.gov

*/


#include <math.h>
#include <stdio.h>
#   include <mpi.h>

#include "defines.h"
#include "BLAS_prototypes.h"
#include "macros.h"
#include "pcomm.h"


extern int myrow;
extern int mycol;

extern int me;	               /* processor id information */
extern int nprocs_row;         /* num of procs to which a row is assigned */
extern int nprocs_col;         /* num of procs to which a col is assigned */
extern int nrows_matrix;       /* number of rows in the matrix */
extern int ncols_matrix;       /* number of cols in the matrix */
extern int my_rows;            /* num of rows I own */
extern int my_cols;            /* num of cols I own */
extern int my_rhs;



extern int *pivot_vec;         /* ptr to vector storing list of pivot rows */


extern MPI_Comm col_comm;
extern MPI_Comm row_comm;





void
permute_mat(DATA_TYPE *mat, int *permutations)
{ int j,k;                        /* loop counter */

  int col_len,row_len;

  int J;			/* global column index */

  
  MPI_Status msgstatus;
 
  int pivot_row, k_row;
  DATA_TYPE tmpv;

 
  col_len = my_rows;    /* length of column in remaining local matrix */


  row_len = my_cols + my_rhs;  /* length of row in local matrix including 
			          rhs's*/

 
  /* BJD starts here */ 
  for (j=0;j<=my_cols-1;j++)
  {
    J=j*nprocs_row+mycol;
    for (k=J+1;k<=nrows_matrix-1;k++)
    {
	k_row=k%nprocs_col;
	if (myrow==k_row)
  	  pivot_row=permutations[k/nprocs_col];
	MPI_Bcast(&pivot_row,1,MPI_INT,k_row,col_comm);
	if (k != pivot_row)
   	{
	  if (myrow == k_row)
	  {
		MPI_Send((char *)(&mat[(J/nprocs_row)*my_rows+(k/nprocs_col)]),
		  sizeof(DATA_TYPE),MPI_CHAR,pivot_row%nprocs_col,2,col_comm);
	  }
	  if (myrow == pivot_row%nprocs_col)
	  {	
	  	MPI_Send((char *)(&mat[(J/nprocs_row)*my_rows+
			(pivot_row/nprocs_col)]),sizeof(DATA_TYPE),
			MPI_CHAR,k_row,3,col_comm);
	  }
	  if (myrow == k_row)
	  {
		MPI_Recv((char *)(&tmpv),sizeof(DATA_TYPE),MPI_CHAR,
			pivot_row%nprocs_col,3,col_comm,
		  	&msgstatus);
		mat[(J/nprocs_row)*my_rows+(k/nprocs_col)] = tmpv;
	  }
 	  if (myrow == pivot_row%nprocs_col)
  	  {
		MPI_Recv((char *)(&tmpv),sizeof(DATA_TYPE),MPI_CHAR,k_row,
			2,col_comm,&msgstatus);
		mat[(J/nprocs_row)*my_rows+(pivot_row/nprocs_col)] = tmpv;
	  }
	}/* End of if (k != pivot_row) */
     }/* End of for (k=J+1;k<=nrows_matrix-2;k++) */
  }/* End of for (j=0;j<=my_cols-1;j++) */
}/* End of function permute_mat. */
