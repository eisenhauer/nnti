/*****************************************************************************
 * Zoltan Dynamic Load-Balancing Library for Parallel Applications           *
 * Copyright (c) 2000, Sandia National Laboratories.                         *
 * For more info, see the README file in the top-level Zoltan directory.     *  
 *****************************************************************************/
/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    $Revision$
 ****************************************************************************/

#ifndef __PAR_UTIL_CONST_H
#define __PAR_UTIL_CONST_H

extern int LB_find_median(double *dots, double *wgts, int *dotmark, int dotnum,
  int proc, double fractionlo, MPI_Comm local_comm, double *valuehalf,
  int first_guess, int *counter);

#endif
