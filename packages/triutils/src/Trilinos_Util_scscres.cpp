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

#include "Trilinos_Util.h"


double Trilinos_Util_scscres (int isym, int m, int n, 
	      double *val, int *indx, int *pntr,
	      double *x, double *b)
{
    int i, j, ibgn, iend;
    double norm_tmp = 0.0, norm_b = 0.0;
    double scaled_res_norm, res_norm, *tmp, max_norm = 0.0;


/*     Computes the residual

                      res = || b - A*x ||

       where x and b are vectors and A is a sparse matrix stored
       in MSR format. */

/*     -------------------------- 
       First executable statement 
       -------------------------- */

    /* Create tmp workspace */
    tmp = (double *) calloc(m,sizeof(double));

/* .....initialize soln */

    for (i = 0; i < m; i++)
	tmp[i] = b[i];

/* .....do a series of SPAXPYs (sparse saxpys) */

    for (j = 0; j < n ; j++) 
      {
	ibgn = pntr[j];
	iend = pntr[j + 1];
	
	for (i = ibgn; i < iend; i++)
	  {
	    tmp[indx[i]] -= val[i] * x[j];
 	    if (indx[i] != j && isym) tmp[j] -= val[i]*x[indx[i]];
	  }
     }
    for (i = 0; i < m; i++)
      {
	max_norm = Trilinos_Util_max(fabs(tmp[i]),max_norm);
	norm_tmp += tmp[i]*tmp[i];
	norm_b += b[i]*b[i];
      }
   
    res_norm = sqrt(norm_tmp);
    printf("\n\nMax norm of residual        = %12.4g\n",max_norm);
    printf(    "Two norm of residual        = %12.4g\n",res_norm);
    if (norm_b > 1.0E-7) 
      {
	   scaled_res_norm = res_norm/sqrt(norm_b);
	   printf(    "Scaled two norm of residual = %12.4g\n",scaled_res_norm);
      }

    free((void *) tmp);

    return(scaled_res_norm);

} /* scscres */

