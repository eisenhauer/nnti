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

#include <stdio.h>
#include <stdlib.h>
#include "par_const.h"


/************ R O U T I N E S   I N   T H I S   F I L E  **********************

       NAME                             TYPE
----------------------------------------------------------------------
	LB_Print_Stats			void

******************************************************************************/

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void LB_Print_Stats (MPI_Comm communicator, int debug_proc, double x, char *msg)
{
/****************************************************************/
/* Print max, sum, and imbalance for a variable over all procs. */
/****************************************************************/
  double sum, max;
  int proc, num_proc;

  MPI_Comm_rank(communicator, &proc);
  MPI_Comm_size(communicator, &num_proc);

  MPI_Reduce((void *)&x, (void *)&sum, 1, MPI_DOUBLE, MPI_SUM, debug_proc, 
             communicator);

  MPI_Reduce((void *)&x, (void *)&max, 1, MPI_DOUBLE, MPI_MAX, debug_proc, 
             communicator);

  if (proc == debug_proc && sum != 0.0)
    printf("%s: Max: %g, Sum: %g, Imbal.: %g\n",
            msg, max, sum, max*(num_proc)/sum);

}
