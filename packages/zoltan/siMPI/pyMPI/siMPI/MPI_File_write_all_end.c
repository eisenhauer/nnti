/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    $Revision$
 ****************************************************************************/
/****************************************************************************/
/* FILE  ****************  MPI_File_write_all_end.c  ************************/
/****************************************************************************/
/* Author : Lisa Alano July 23 2002                                         */
/* Copyright (c) 2002 University of California Regents                      */
/****************************************************************************/

#include "mpi.h"

int MPI_File_write_all_end(MPI_File fh, void *buf, MPI_Status *status)
{
  _MPI_COVERAGE();
  return PMPI_File_write_all_end (fh, buf, status);
}

