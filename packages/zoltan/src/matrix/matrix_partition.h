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

#ifndef __MATRIX_PARTITION_H
#define __MATRIX_PARTITION_H

#ifdef __cplusplus
/* if C++, define the rest of this header file as extern C */
extern "C" {
#endif

#include "params_const.h" /* for MAX_PARAM_STRING_LEN */

/********************************************************************
 * Data structures used when converting an input sparse matrix
 * to a PHG problem, running the PHG problem and retaining results
 * for user query.  
 *
 * For performance reasons, we'll use ints for matrix indices and
 * sizes, but for really large matrices these may someday be longs.
 * So typedef the row and column indices and matrix dimensions.
 *
 * If IJTYPE is changed, change the parameters of the
 * CSC and CSR query functions in zoltan.h and the User's Guide.
 *
 * This should be moved out of phg to somewhere more general.  The
 * Zoltan algorithm used for the sparse matrix need not be PHG.
 ********************************************************************/

#include <zoltan.h>

enum ObjectType {ROW_TYPE = 1, COL_TYPE = 2, NZ_TYPE = 3};

enum PartitionType {MP_ROW_TYPE=1, MP_COLUMN_TYPE=2, MP_NZ_TYPE=3};

struct obj_node{
  IJTYPE i;
  IJTYPE j;
  IJTYPE objLID;
  struct obj_node *next;
};
typedef struct _obj_lookup{
  struct obj_node *htTop;
  struct obj_node **ht;
  IJTYPE table_size;   /* Length of ht array */
  IJTYPE num_objs;     /* Length of htTop array */
  int    key_size;
} obj_lookup;

struct Zoltan_MP_Data_Struct{
  struct Zoltan_Struct *zzLib;    /* Problem created by Zoltan_Matrix_Partition() */

  /* Parameters */
  int approach;       /* a PartitionType, the LB_APPROACH parameter */
  char method[MAX_PARAM_STRING_LEN]; /* string, partitioning method */

  /* The local portion of sparse matrix returned by the query function */
  int input_type;    /* a ObjectType, how they supply the matrix (CSC or CSR) */
  IJTYPE numRC;      /* number of rows or columns */
  IJTYPE *rcGID;     /* row or column GIDs   */
  IJTYPE *nzIndex;  /* index into nzGIDs array, last is num nz */
  IJTYPE *nzGID;    /* non-zeros column or row GIDs */

  /* Mirror specification of sparse matrix: if input was CSR, create CSC, 
   * or in input was CSC, create CSR */

  IJTYPE numCR;  
  IJTYPE *crGID;
  IJTYPE *mirrorNzIndex;
  IJTYPE *mirrorNzGID; 

  /* Global values filled out by process_matrix_input().                  */
  IJTYPE rowBaseID;   /* most likely zero or one */
  IJTYPE colBaseID;   /* most likely zero or one */
  IJTYPE nRows;
  IJTYPE nCols;
  IJTYPE nNonZeros;

  /* "Local" objects. For 1d partitioning, we need know which rows are ours.  */
  IJTYPE nMyVtx;    /* 1d model: my number of vertices */
  IJTYPE *vtxGID;   /* 1d model: local vertex GIDs */

  /* Results, to supply data to query functions */
  /* The results are stored in flattened arrays and the lookup functions */
  /* will return index to these arrays. */
  int *rowproc;
  int *rowpart;
  obj_lookup *row_lookup;
  int *colproc;
  int *colpart;
  obj_lookup *col_lookup;
  int *nzproc;
  int *nzpart;
  obj_lookup *nz_lookup;
};

typedef struct Zoltan_MP_Data_Struct ZOLTAN_MP_DATA;

int Zoltan_Lookup_Obj(obj_lookup *lu, IJTYPE I, IJTYPE J);

void Zoltan_MP_Debug_Partitioning(struct Zoltan_Struct *zz);

#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif

#endif   /* __MATRIX_PARTITION_H */
