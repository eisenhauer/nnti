/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    $Revision$
 ****************************************************************************/
/* FILE  ******************       MPI_Scatter.c      ************************/
/****************************************************************************/
/* Author : Lisa Alano July 16 2002                                         */
/* Copyright (c) 2002 University of California Regents                      */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mpi.h"

/*==========================================================================*/
int MPI_Scatter (void *sendbuf, int sendcnt, MPI_Datatype sendtype, void *recvbuf,
    int recvcnt, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  _MPI_COVERAGE();
  return PMPI_Scatter(sendbuf, sendcnt, sendtype, recvbuf, recvcnt, recvtype, root, comm);
}
/*==========================================================================*/
