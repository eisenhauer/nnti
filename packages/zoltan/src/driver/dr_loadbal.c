/*****************************************************************************
 * Zoltan Library for Parallel Applications                                  *
 * Copyright (c) 2000,2001,2002, Sandia National Laboratories.               *
 * For more info, see the README file in the top-level Zoltan directory.     * 
 *****************************************************************************/
/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    $Revision$
 ****************************************************************************/

#include <mpi.h>
#ifdef TIMER_CALLBACKS
/* Code that times how much time is spent in the callback functions.
 * By default, this code is OFF.
 */
double Timer_Callback_Time, Timer_Global_Callback_Time;
#define START_CALLBACK_TIMER  double stime = MPI_Wtime()
#define STOP_CALLBACK_TIMER   Timer_Callback_Time += MPI_Wtime() - stime
#else
#define START_CALLBACK_TIMER
#define STOP_CALLBACK_TIMER 
#endif /* TIMER_CALLBACKS */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "dr_const.h"
#include "dr_err_const.h"
#include "dr_loadbal_const.h"
#include "dr_eval_const.h"
#include "dr_util_const.h"
#include "ch_init_dist_const.h"
#include "dr_param_file.h"

#ifdef __cplusplus
/* if C++, define the rest of this header file as extern C */
extern "C" {
#endif

static int Num_Global_Parts;
static int Num_GID = 1, Num_LID = 1;
static int Export_Lists_Special = 0;
static int Matrix_Partition_Approach = 0;
static void test_drops(int, MESH_INFO_PTR, PARIO_INFO_PTR,
   struct Zoltan_Struct *);
static int sorted_unique_list(unsigned int *l, int len, 
   unsigned int **newl, int *newLen);

extern int Zoltan_Matrix_Partition(struct Zoltan_Struct *zz);

/*--------------------------------------------------------------------------*/
/* Purpose: Call Zoltan to determine a new load balance.                    */
/*          Contains all of the callback functions that Zoltan needs        */
/*          for the load balancing.                                         */
/*--------------------------------------------------------------------------*/

/*
 *  PROTOTYPES for load-balancer interface functions.
 */

ZOLTAN_NUM_OBJ_FN get_num_elements;

ZOLTAN_OBJ_LIST_FN get_elements;
ZOLTAN_FIRST_OBJ_FN get_first_element;
ZOLTAN_NEXT_OBJ_FN get_next_element;

ZOLTAN_NUM_GEOM_FN get_num_geom;
ZOLTAN_GEOM_MULTI_FN get_geom_multi;
ZOLTAN_GEOM_FN get_geom;

ZOLTAN_NUM_EDGES_FN get_num_edges;
ZOLTAN_NUM_EDGES_MULTI_FN get_num_edges_multi;
ZOLTAN_EDGE_LIST_FN get_edge_list;
ZOLTAN_EDGE_LIST_MULTI_FN get_edge_list_multi;

ZOLTAN_NUM_CHILD_FN get_num_child;
ZOLTAN_CHILD_LIST_FN get_child_elements;
ZOLTAN_FIRST_COARSE_OBJ_FN get_first_coarse_element;
ZOLTAN_NEXT_COARSE_OBJ_FN get_next_coarse_element;

ZOLTAN_PARTITION_MULTI_FN get_partition_multi;
ZOLTAN_PARTITION_FN get_partition;

ZOLTAN_HG_SIZE_CS_FN get_hg_size_compressed_pin_storage;
ZOLTAN_HG_SIZE_EDGE_WTS_FN get_hg_size_edge_weights;
ZOLTAN_HG_CS_FN get_hg_compressed_pin_storage;
ZOLTAN_HG_EDGE_WTS_FN get_hg_edge_weights;

ZOLTAN_NUM_FIXED_OBJ_FN get_num_fixed_obj;
ZOLTAN_FIXED_OBJ_LIST_FN get_fixed_obj_list;

ZOLTAN_CSR_SIZE_FN get_sparse_matrix_size;
ZOLTAN_CSC_SIZE_FN get_sparse_matrix_size;
ZOLTAN_CSR_FN get_sparse_matrix;
ZOLTAN_CSC_FN get_sparse_matrix;

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int setup_zoltan(struct Zoltan_Struct *zz, int Proc, PROB_INFO_PTR prob,
                 MESH_INFO_PTR mesh, PARIO_INFO_PTR pio_info)
{
/* Local declarations. */
  char *yo = "setup_zoltan";

  float *psize;                  /* Partition size */
  int *partid;                   /* Partition numbers */
  int *idx;                      /* Index numbers */
  int nprocs;                    /* Number of processors. */
  int i;                         /* Loop index */
  int ierr;                      /* Error code */
  char errmsg[128];              /* Error message */

  DEBUG_TRACE_START(Proc, yo);

  /* Allocate space for arrays. */
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  Num_Global_Parts = nprocs;
  psize = (float *) malloc(nprocs*sizeof(float)); 
  partid = (int *) malloc(2*nprocs*sizeof(int)); 
  idx = partid + nprocs;
  Export_Lists_Special = 0;

  if (Test.Vtx_Inc) {
    Test.Multi_Callbacks = 1;  /* vertex increment implemented only in
                                  multi callbacks */
  }

  /* Set the user-specified parameters */
  for (i = 0; i < prob->num_params; i++) {
    if (prob->params[i].Index>=0)
      ierr = Zoltan_Set_Param_Vec(zz, prob->params[i].Name, prob->params[i].Val,
             prob->params[i].Index);
    else {
      ierr = Zoltan_Set_Param(zz, prob->params[i].Name, prob->params[i].Val);
      if (strcasecmp(prob->params[i].Name, "NUM_GLOBAL_PARTITIONS") == 0)
        Num_Global_Parts = atoi(prob->params[i].Val);
    }
    if (ierr == ZOLTAN_FATAL) {
      sprintf(errmsg,
              "fatal: error in Zoltan_Set_Param when setting parameter %s\n",
              prob->params[i].Name);
      Gen_Error(0, errmsg);
      return 0;
    }
    if (strcasecmp(prob->params[i].Name, "NUM_GID_ENTRIES") == 0) 
      Num_GID = atoi(prob->params[i].Val);
    else if (strcasecmp(prob->params[i].Name, "NUM_LID_ENTRIES") == 0) 
      Num_LID = atoi(prob->params[i].Val);
    else if (strcasecmp(prob->params[i].Name, "RETURN_LISTS") == 0) 
      Export_Lists_Special = (strstr(prob->params[i].Val,"partition") != NULL);
    else if (strcasecmp(prob->params[i].Name, "MATRIX_APPROACH") == 0) {
      if ((strstr(prob->params[i].Val,"rows") != NULL) ||
          (strstr(prob->params[i].Val,"row") != NULL)){
        Matrix_Partition_Approach = MP_ROWS;
      }
      else if ((strstr(prob->params[i].Val,"columns") != NULL) ||
               (strstr(prob->params[i].Val,"cols") != NULL)    ||
               (strstr(prob->params[i].Val,"col") != NULL)) {
        Matrix_Partition_Approach = MP_COLS;
      }
      else{
        /* Zoltan_Matrix_Partition defaults to using phg on s.m. rows */
        Matrix_Partition_Approach = MP_ROWS;
      }
    }
  }

  /* Set the load-balance method */
  if (Zoltan_Set_Param(zz, "LB_METHOD", prob->method) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Param(LB_METHOD)\n");
    return 0;
  }

  /* if there is a paramfile specified, read it
     note: contents of this file may override the parameters set above */
  if (strcmp(prob->zoltanParams_file, "")) {
    zoltanParams_read_file(zz, prob->zoltanParams_file, MPI_COMM_WORLD);
  }

  if (Test.Fixed_Objects) {
    /* Register fixed object callback functions */
    if (Zoltan_Set_Fn(zz, ZOLTAN_NUM_FIXED_OBJ_FN_TYPE,
                      (void (*)()) get_num_fixed_obj,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }

    if (Zoltan_Set_Fn(zz, ZOLTAN_FIXED_OBJ_LIST_FN_TYPE,
                      (void (*)()) get_fixed_obj_list,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
  }

  if (Test.Local_Partitions == 1) {
    /* Compute Proc partitions for each processor */
    char s[8];
    sprintf(s, "%d", Proc);
    if (Zoltan_Set_Param(zz, "NUM_LOCAL_PARTITIONS", s) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Param()\n");
      return 0;
    }
  }
  else if (Test.Local_Partitions == 2) {
    /* Compute Proc partitions for odd-ranked processors; let remaining
     * partitions be in even-ranked processors. */
    if (Proc%2) {
      char s[8];
      sprintf(s, "%d", Proc);
      if (Zoltan_Set_Param(zz, "NUM_LOCAL_PARTITIONS", s) == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from Zoltan_Set_Param()\n");
        return 0;
      }
    }
  }
  else if (Test.Local_Partitions == 3 || Test.Local_Partitions == 5) {
    /* Variable partition sizes, but one partition per proc */
    /* Test.Local_Partitions == 5 is same as 3, but with sizes increased by 1 */
    /* to avoid zero-sized partitions (for ParMETIS tests). */
    i = 0;
    psize[0] = (float) (Proc + (Test.Local_Partitions == 5)); 
    /* Set partition sizes using global numbers. */
    Zoltan_LB_Set_Part_Sizes(zz, 1, 1, &Proc, &i, psize);
    /* Reset partition sizes for upper half of procs. */
    if (Proc >= nprocs/2){
      psize[0] = 0.5 + (Proc%2) + (Test.Local_Partitions == 5);
      Zoltan_LB_Set_Part_Sizes(zz, 1, 1, &Proc, &i, psize);
    }
  }
  else if (Test.Local_Partitions == 4) {
    /* Variable number of partitions per proc and variable sizes. */
    /* Request Proc partitions for each processor, of size 1/Proc.  */
    char s[8];
    sprintf(s, "%d", Proc);
    if (Zoltan_Set_Param(zz, "NUM_LOCAL_PARTITIONS", s) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Param()\n");
      return 0;
    }
    /* Each partition size is inverse to the no. of partitions on a proc. */ 
    for (i=0; i<Proc; i++){
      partid[i] = i;                    /* Local partition number */
      idx[i] = 0;
      psize[i] = 1.0/Proc; 
    }
    Zoltan_LB_Set_Part_Sizes(zz, 0, Proc, partid, idx, psize);
  }
  else if (Test.Local_Partitions == 6) {
    /* Variable partition sizes, but one partition per proc */
    /* When nprocs >= 6, zero-sized partitions on processors >= 2. */
    i = 0;
    psize[0] = (float) Proc;
    /* Set partition sizes using global numbers. */
    Zoltan_LB_Set_Part_Sizes(zz, 1, 1, &Proc, &i, psize);
    /* Reset partition sizes for procs near end. */
    if (nprocs >= 6 && Proc >= nprocs-4){
      psize[0] = 0.;
      Zoltan_LB_Set_Part_Sizes(zz, 1, 1, &Proc, &i, psize);
    }
    else if (nprocs < 6) {
      Gen_Error(0, "warning:  Test Local Partitions = 6 should be run "
                    "on six or more processors.\n");
      error_report(Proc);
    }
  }
  else if (Test.Local_Partitions == 7) {
    /* Variable partition sizes, but one partition per proc */
    /* When nprocs >= 6, zero-sized partitions on processors 0, 1, 2, and 3. */
    i = 0;
    psize[0] = (float) Proc;
    /* Set partition sizes using global numbers. */
    Zoltan_LB_Set_Part_Sizes(zz, 1, 1, &Proc, &i, psize);
    /* Reset partition sizes for procs near beginning. */
    if (nprocs >= 6 && Proc < 4){
      psize[0] = 0.;
      Zoltan_LB_Set_Part_Sizes(zz, 1, 1, &Proc, &i, psize);
    }
    else if (nprocs < 6) {
      Gen_Error(0, "warning:  Test Local Partitions = 7 should be run "
                    "on six or more processors.\n");
      error_report(Proc);
    }
  }
  else if (Test.Local_Partitions == 8) {
    int nparts=100;
    /* Variable partition sizes. Assume at most 100 global partitions. */
    /* Realloc arrays. */
    safe_free((void **) &psize);
    safe_free((void **) &partid);
    psize = (float *) malloc(nparts*sizeof(float));
    partid = (int *) malloc(nparts*sizeof(int));
    for (i=0; i<nparts; i++){
      partid[i] = i;
      psize[i] = (float) i;
    }
    /* Set partition sizes using global numbers. */
    Zoltan_LB_Set_Part_Sizes(zz, 1, nparts, partid, NULL, psize);
  }

  /* Free temporary arrays for partition sizes. */
  safe_free((void **) &psize);
  safe_free((void **) &partid);

  /*
   * Set the callback functions
   */

  if (Zoltan_Set_Fn(zz, ZOLTAN_NUM_OBJ_FN_TYPE, (void (*)()) get_num_elements,
                    (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
    return 0;
  }

  if (Test.Multi_Callbacks) {
    if (Zoltan_Set_Fn(zz, ZOLTAN_OBJ_LIST_FN_TYPE,
                      (void (*)()) get_elements,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
  }
  else {
    if (Zoltan_Set_Fn(zz, ZOLTAN_FIRST_OBJ_FN_TYPE,
                      (void (*)()) get_first_element,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }

    if (Zoltan_Set_Fn(zz, ZOLTAN_NEXT_OBJ_FN_TYPE,
                      (void (*)()) get_next_element,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
  }

  /* Functions for geometry based algorithms */
  if (Zoltan_Set_Fn(zz, ZOLTAN_NUM_GEOM_FN_TYPE, (void (*)()) get_num_geom,
                    (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
    return 0;
  }

  if (Test.Multi_Callbacks) {
    if (Zoltan_Set_Fn(zz, ZOLTAN_GEOM_MULTI_FN_TYPE,
                      (void (*)()) get_geom_multi,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
  }
  else {
    if (Zoltan_Set_Fn(zz, ZOLTAN_GEOM_FN_TYPE, (void (*)()) get_geom,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
  }

  /* Functions for graph based algorithms */
  if (Test.Graph_Callbacks) {
    if (Test.Multi_Callbacks) {
      if (Zoltan_Set_Fn(zz, ZOLTAN_NUM_EDGES_MULTI_FN_TYPE,
                        (void (*)()) get_num_edges_multi,
                        (void *) mesh) == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
        return 0;
      }
      if (Zoltan_Set_Fn(zz, ZOLTAN_EDGE_LIST_MULTI_FN_TYPE,
                        (void (*)()) get_edge_list_multi,
                        (void *) mesh) == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
        return 0;
      }
      /* Sizes used in repartitioning to reduce migration cost */
      if (Zoltan_Set_Fn(zz, ZOLTAN_OBJ_SIZE_MULTI_FN_TYPE,
                        (void (*)()) migrate_elem_size_multi,
                        (void *) mesh) == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
        return 0;
      }
    }
    else {
      if (Zoltan_Set_Fn(zz, ZOLTAN_NUM_EDGES_FN_TYPE, (void (*)()) get_num_edges,
                        (void *) mesh) == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
        return 0;
      }
      if (Zoltan_Set_Fn(zz, ZOLTAN_EDGE_LIST_FN_TYPE, (void (*)()) get_edge_list,
                        (void *) mesh)== ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
        return 0;
      }
      if (Zoltan_Set_Fn(zz, ZOLTAN_OBJ_SIZE_FN_TYPE,
                        (void (*)()) migrate_elem_size,
                        (void *) mesh) == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
        return 0;
      }
    }
  }


  /* Functions for tree-based algorithms */
  if (Zoltan_Set_Fn(zz, ZOLTAN_NUM_COARSE_OBJ_FN_TYPE,
                    (void (*)()) get_num_elements,
                    (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
    return 0;
  }

  if (Zoltan_Set_Fn(zz, ZOLTAN_FIRST_COARSE_OBJ_FN_TYPE,
                    (void (*)()) get_first_coarse_element,
                    (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
    return 0;
  }

  if (Zoltan_Set_Fn(zz, ZOLTAN_NEXT_COARSE_OBJ_FN_TYPE,
                    (void (*)()) get_next_coarse_element,
                    (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
    return 0;
  }

  if (Zoltan_Set_Fn(zz, ZOLTAN_NUM_CHILD_FN_TYPE,
                    (void (*)()) get_num_child,
                    (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
    return 0;
  }

  if (Zoltan_Set_Fn(zz, ZOLTAN_CHILD_LIST_FN_TYPE,
                    (void (*)()) get_child_elements,
                    (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
    return 0;
  }

  /* Hypergraph-based callbacks */
  if ((mesh->data_type == HYPERGRAPH) && Test.Hypergraph_Callbacks) {

    if (Zoltan_Set_Fn(zz, ZOLTAN_HG_SIZE_CS_FN_TYPE,
                      (void (*)()) get_hg_size_compressed_pin_storage,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }

    if (Zoltan_Set_Fn(zz, ZOLTAN_HG_SIZE_EDGE_WTS_FN_TYPE,
                      (void (*)()) get_hg_size_edge_weights,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }

    if (Zoltan_Set_Fn(zz, ZOLTAN_HG_CS_FN_TYPE,
                      (void (*)()) get_hg_compressed_pin_storage,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }

    if (Zoltan_Set_Fn(zz, ZOLTAN_HG_EDGE_WTS_FN_TYPE,
                      (void (*)()) get_hg_edge_weights,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }

    if (Test.Multi_Callbacks) {
      if (Zoltan_Set_Fn(zz, ZOLTAN_OBJ_SIZE_MULTI_FN_TYPE,
                        (void (*)()) migrate_elem_size_multi,
                        (void *) mesh) == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
        return 0;
      }
    }
    else {
      if (Zoltan_Set_Fn(zz, ZOLTAN_OBJ_SIZE_FN_TYPE,
                        (void (*)()) migrate_elem_size,
                        (void *) mesh) == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
        return 0;
      }
    }
  }

  /* Functions for partitions */
  if (Test.Multi_Callbacks) {
    if (Zoltan_Set_Fn(zz, ZOLTAN_PARTITION_MULTI_FN_TYPE,
                      (void (*)()) get_partition_multi,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
  }
  else {
    if (Zoltan_Set_Fn(zz, ZOLTAN_PARTITION_FN_TYPE,
                      (void (*)()) get_partition,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
  }

  /* Functions for partitioning sparse matrices */

  if (pio_info->init_dist_pins == INITIAL_ROW){
    if (Zoltan_Set_Fn(zz, ZOLTAN_CSR_SIZE_FN_TYPE,
                      (void (*)()) get_sparse_matrix_size,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
    if (Zoltan_Set_Fn(zz, ZOLTAN_CSR_FN_TYPE,
                      (void (*)()) get_sparse_matrix,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
  }
  else{
    if (Zoltan_Set_Fn(zz, ZOLTAN_CSC_SIZE_FN_TYPE,
                      (void (*)()) get_sparse_matrix_size,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
    if (Zoltan_Set_Fn(zz, ZOLTAN_CSC_FN_TYPE,
                      (void (*)()) get_sparse_matrix,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Fn()\n");
      return 0;
    }
  }

  DEBUG_TRACE_END(Proc, yo);
  return 1;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int run_zoltan(struct Zoltan_Struct *zz, int Proc, PROB_INFO_PTR prob,
               MESH_INFO_PTR mesh, PARIO_INFO_PTR pio_info)
{
/* Local declarations. */
  char *yo = "run_zoltan";
  struct Zoltan_Struct *zz_copy;

  /* Variables returned by Zoltan */
  ZOLTAN_ID_PTR import_gids = NULL;  /* Global nums of objs to be imported   */
  ZOLTAN_ID_PTR import_lids = NULL;  /* Local indices to objs to be imported */
  int   *import_procs = NULL;        /* Proc IDs of procs owning objs to be
                                        imported.                            */
  int   *import_to_part = NULL;      /* Partition #s to which imported objs 
                                        should be assigned.                  */
  ZOLTAN_ID_PTR export_gids = NULL;  /* Global nums of objs to be exported   */
  ZOLTAN_ID_PTR export_lids = NULL;  /* local indices to objs to be exported */
  int   *export_procs = NULL;        /* Proc IDs of destination procs for objs
                                        to be exported.                      */
  int   *export_to_part = NULL;      /* Partition #s for objs to be exported.*/
  int num_imported;              /* Number of objs to be imported.          */
  int num_exported;              /* Number of objs to be exported.          */
  int new_decomp;                /* Flag indicating whether the decomposition
                                    has changed                              */
  int num_gid_entries;           /* Number of array entries in a global ID.  */
  int num_lid_entries;           /* Number of array entries in a local ID.   */

  int i;                      /* Loop index                               */
  int errcnt, gerrcnt;
  int ierr=ZOLTAN_OK;
  double stime = 0.0, mytime = 0.0, maxtime = 0.0;
  char fname[128];
  ELEM_INFO *current_elem = NULL;

/***************************** BEGIN EXECUTION ******************************/

  DEBUG_TRACE_START(Proc, yo);

  if (Driver_Action & 1){

    /* Load balancing part */
  
    /* Evaluate the old balance */
    if (Debug_Driver > 0) {
      if (Proc == 0) printf("\nBEFORE load balancing\n");
      driver_eval(mesh);
      i = Zoltan_LB_Eval(zz, 1, NULL, NULL, NULL, NULL, NULL, NULL);
      if (i) printf("Warning: Zoltan_LB_Eval returned code %d\n", i);
    }
    if (Test.Gen_Files) {
      /* Write output files. */
      strcpy(fname, pio_info->pexo_fname);
      strcat(fname, ".before");
      Zoltan_Generate_Files(zz, fname, 1, 1, 1, 1);
    }

    if (Test.Fixed_Objects) 
      setup_fixed_obj(mesh, Num_Global_Parts);
  
    /*
     * Call Zoltan
     */
#ifdef TIMER_CALLBACKS
    Timer_Callback_Time = 0.0;
#endif /* TIMER_CALLBACKS */

    MPI_Barrier(MPI_COMM_WORLD);   /* For timings only */
    stime = MPI_Wtime();

    ierr = Zoltan_LB_Partition(zz, &new_decomp, 
                 &num_gid_entries, &num_lid_entries,
                 &num_imported, &import_gids,
                 &import_lids, &import_procs, &import_to_part,
                 &num_exported, &export_gids,
                 &export_lids, &export_procs, &export_to_part);

    if ((ierr != ZOLTAN_OK) && (ierr != ZOLTAN_WARN)){
      Gen_Error(0, "fatal:  error returned from Zoltan_LB_Partition()\n");
      return 0;
    }

    mytime = MPI_Wtime() - stime;
    MPI_Allreduce(&mytime, &maxtime, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    if (Proc == 0)
      printf("DRIVER:  Zoltan_LB_Partition time = %g\n", maxtime);
    Total_Partition_Time += maxtime;

#ifdef TIMER_CALLBACKS
    MPI_Allreduce(&Timer_Callback_Time, &Timer_Global_Callback_Time, 
                   1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    if (Proc == 0)
      printf("DRIVER:  Callback time = %g\n", Timer_Global_Callback_Time);
#endif /* TIMER_CALLBACKS */


    {int mine[2], gmax[2], gmin[2];
    mine[0] = num_imported;
    mine[1] = num_exported;
    MPI_Allreduce(mine, gmax, 2, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(mine, gmin, 2, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    if (Proc == 0) {
      printf("DRIVER:  new_decomp %d Min/Max Import: %d %d\n", new_decomp, gmin[0], gmax[0]);
      printf("DRIVER:  new_decomp %d Min/Max Export: %d %d\n", new_decomp, gmin[1], gmax[1]);
    }
    }

#ifdef ZOLTAN_NEMESIS
    if (pio_info->file_type == NEMESIS_FILE && Output.Nemesis) {
      i = write_elem_vars(Proc, mesh, pio_info, num_exported, export_gids,
                          export_procs, export_to_part);
    }
#endif

    /*
     * Call another routine to perform the migration
     */
    MPI_Barrier(MPI_COMM_WORLD);   /* For timings only */
    stime = MPI_Wtime();
    if (new_decomp && (num_exported != -1 || num_imported != -1) &&
         !Export_Lists_Special){

      /* Migrate if new decomposition and RETURN_LISTS != NONE and
         RETURN_LISTS != PARTITIONS */

      if (!migrate_elements(Proc, mesh, zz, num_gid_entries, num_lid_entries,
          num_imported, import_gids, import_lids, import_procs, import_to_part,
          num_exported, export_gids, export_lids, export_procs, export_to_part)){
        Gen_Error(0, "fatal:  error returned from migrate_elements()\n");
        return 0;
      }
    }
    mytime = MPI_Wtime() - stime;
    MPI_Allreduce(&mytime, &maxtime, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    if (Proc == 0)
      printf("DRIVER:  Total migration time = %g\n", maxtime);

    /* Test whether Fixed Object assignments are correct */
    if (Test.Fixed_Objects) {
      for (errcnt=0, i = 0; i < mesh->num_elems; i++) {
        if (mesh->blank_count && (mesh->blank[i] == 1)) continue;
        current_elem = &(mesh->elements[i]);
        if (current_elem->fixed_part != -1 &&
            current_elem->fixed_part != current_elem->my_part) {
          errcnt++;
          printf("%d:  Object %d fixed to %d but assigned to %d\n",
                 Proc, current_elem->globalID, current_elem->fixed_part,
                 current_elem->my_part);
        }
      }
      MPI_Allreduce(&errcnt, &gerrcnt, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);
      if (gerrcnt) {
        Zoltan_LB_Free_Part(&import_gids, &import_lids,
                            &import_procs, &import_to_part);
        Zoltan_LB_Free_Part(&export_gids, &export_lids,
                            &export_procs, &export_to_part);
        Gen_Error(0, "Fatal:  Fixed objects' assignments incorrect.");
        return 0;
      }
      else if (Proc == 0)
        printf("%d:  All fixed objects are correct.\n", Proc);
    }

    /*
     * Test copy function
     */
    zz_copy = Zoltan_Copy(zz);
    if (zz_copy == NULL){
        Gen_Error(0, "fatal:  Zoltan_Copy failure\n");
        return 0;
    }
    if (Zoltan_Copy_To(zz, zz_copy)){
        Gen_Error(0, "fatal:  Zoltan_Copy_To failure\n");
        return 0;
    }

    Zoltan_Destroy(&zz_copy);
  
    /* Evaluate the new balance */
    if (Debug_Driver > 0) {
      if (!Test.Dynamic_Graph){
        if (Proc == 0) printf("\nAFTER load balancing\n");
        driver_eval(mesh);
        i = Zoltan_LB_Eval(zz, 1, NULL, NULL, NULL, NULL, NULL, NULL);
        if (i) printf("Warning: Zoltan_LB_Eval returned code %d\n", i);
      }
      else{
        if (Proc == 0){
          printf("\nAFTER load balancing\n");
          printf("  Omitting load balance evaluation because vertex blanking\n");
          printf("  may cause result to be incorrect.\n\n");
        }
        
      }
    }
    if (Test.Gen_Files) {
      /* Write output files. */
      strcpy(fname, pio_info->pexo_fname);
      strcat(fname, ".after");
      Zoltan_Generate_Files(zz, fname, 1, 1, 1, 1);
    }
  
    if (Test.Drops && !Test.No_Global_Objects)
      test_drops(Proc, mesh, pio_info, zz);

    if (Test.RCB_Box && !(strcasecmp(prob->method, "RCB")) && !Test.No_Global_Objects) {
      double xmin, ymin, zmin;
      double xmax, ymax, zmax;
      int ndim;
      ierr = Zoltan_RCB_Box(zz, Proc, &ndim, &xmin, &ymin, &zmin,
                            &xmax, &ymax, &zmax);
      if (!ierr) 
        printf("DRIVER %d DIM: %d BOX: (%e,%e,%e) -- (%e,%e,%e)\n",
               Proc, ndim, xmin, ymin, zmin, xmax, ymax, zmax);
    }
    
    /* Clean up */
    Zoltan_LB_Free_Part(&import_gids, &import_lids,
                        &import_procs, &import_to_part);
    Zoltan_LB_Free_Part(&export_gids, &export_lids,
                        &export_procs, &export_to_part);
  }

  if (Driver_Action & 2){
      /* Only do ordering if this was specified in the driver input file */
      /* NOTE: This part of the code is not modified to recognize
               blanked vertices. */
      int *order = NULL;		/* Ordering vector(s) */
      ZOLTAN_ID_PTR order_gids = NULL;  /* List of all gids for ordering */
      ZOLTAN_ID_PTR order_lids = NULL;  /* List of all lids for ordering */

      if (Test.Dynamic_Graph && !Proc){
        printf("ORDERING DOES NOT WITH WITH DYNAMIC GRAPHS.\n");
        printf("Turn off \"test dynamic graph\".\n");
      }
      
      order = (int *) malloc (2*(mesh->num_elems) * sizeof(int));
      order_gids = (ZOLTAN_ID_PTR) malloc(mesh->num_elems * sizeof(int));
      order_lids = (ZOLTAN_ID_PTR) malloc(mesh->num_elems * sizeof(int));

      if (!order || !order_gids || !order_lids) {
          /* Free order data */
          safe_free((void **) &order);
          safe_free((void **) &order_gids);
          safe_free((void **) &order_lids);
          Gen_Error(0, "memory alloc failed for Zoltan_Order\n");
          return 0;
      }
          
          
    /* Evaluate the old ordering */
    if (Debug_Driver > 0) {
      if (Proc == 0) printf("\nBEFORE ordering\n");
      /* Not yet impl. */
    }

    if (Zoltan_Order(zz, &num_gid_entries, &num_lid_entries,
        mesh->num_elems, order_gids, order_lids,
        order, &order[mesh->num_elems], NULL) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Order()\n");
      return 0;
    }

    /* Evaluate the new ordering */
    if (Debug_Driver > 0) {
      if (Proc == 0) printf("\nAFTER ordering\n");
      /* Not yet impl. */
    }

    /* Copy ordering permutation into mesh structure */
    for (i = 0; i < mesh->num_elems; i++){
      int lid = order_lids[num_lid_entries * i + (num_lid_entries - 1)];
      mesh->elements[lid].perm_value = order[i];
      mesh->elements[lid].invperm_value = order[(mesh->num_elems)+i];
    }

    /* Free order data */
    safe_free((void **) &order);
    safe_free((void **) &order_gids);
    safe_free((void **) &order_lids);
  }


  if (Driver_Action & 4) {
      int *color = NULL;          /* Color vector */
      ZOLTAN_ID_PTR gids = NULL;  /* List of all gids for ordering */
      ZOLTAN_ID_PTR lids = NULL;  /* List of all lids for ordering */

      if (Test.Dynamic_Graph && !Proc){
        printf("COLORING DOES NOT WITH WITH DYNAMIC GRAPHS.\n");
        printf("Turn off \"test dynamic graph\".\n");
      }

      color = (int *) malloc (mesh->num_elems * sizeof(int));
      gids = (ZOLTAN_ID_PTR) malloc(mesh->num_elems * sizeof(int));
      lids = (ZOLTAN_ID_PTR) malloc(mesh->num_elems * sizeof(int));

      if (!color || !gids || !lids) {
          safe_free((void **) &color);
          safe_free((void **) &gids);
          safe_free((void **) &lids); 
          Gen_Error(0, "memory alloc failed for Zoltan_Color\n");
          return 0;
      }
      
      /* Only do coloring if it is specified in the driver input file */
      /* Do coloring after load balancing */        
      if (Zoltan_Color(zz, &num_gid_entries, &num_lid_entries,
                       mesh->num_elems, gids, lids, color) == ZOLTAN_FATAL) {
          Gen_Error(0, "fatal:  error returned from Zoltan_Color()\n");
          safe_free((void **) &color);
          safe_free((void **) &gids);
          safe_free((void **) &lids);
          return 0;
      }

      /* Verify coloring */
      if (Debug_Driver > 0) {
          if (Proc == 0)
              printf("\nVerifying coloring result\n");
          if (Zoltan_Color_Test(zz, &num_gid_entries, &num_lid_entries,
                                mesh->num_elems, gids, lids, color) == ZOLTAN_FATAL) {
              Gen_Error(0, "fatal:  error returned from Zoltan_Color_Test()\n");
              safe_free((void **) &color);
              safe_free((void **) &gids);
              safe_free((void **) &lids);
              return 0;
          }
      }

      /* Copy color info as "perm" into mesh structure */
      for (i = 0; i < mesh->num_elems; i++){
          int lid = lids[num_lid_entries * i + (num_lid_entries - 1)];
          mesh->elements[lid].perm_value = color[i];
      }
      
      /* Free color data */
      safe_free((void **) &color);
      safe_free((void **) &gids);
      safe_free((void **) &lids);
  }

  
  DEBUG_TRACE_END(Proc, yo);
  return 1;
}
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int run_zoltan_sparse_matrix(struct Zoltan_Struct *zz, 
               int Proc, PROB_INFO_PTR prob,
               MESH_INFO_PTR mesh, PARIO_INFO_PTR pio_info)
{
/* Local declarations. */
  char *yo = "run_zoltan_sparse_matrix";
  int ierr=ZOLTAN_OK;
  double stime = 0.0, mytime = 0.0, maxtime = 0.0;
  int i, j, numRows=0, numCols=0, numNZs=0, numGIDs=0, numIDs=0;
  int Num_Proc;
  unsigned int *gidList=NULL;
  int *procList=NULL, *partList=NULL; 
  unsigned int *idx1=NULL, *idx2=NULL;
  unsigned int *rowIDs=NULL, *colIDs=NULL, *idList=NULL;
  char rowcol[8];
  struct Zoltan_Struct *zz_copy=NULL;

  MPI_Comm_size(MPI_COMM_WORLD, &Num_Proc);

/***************************** BEGIN EXECUTION ******************************/

  DEBUG_TRACE_START(Proc, yo);

  if (Driver_Action & 1){

    /* Load balancing part */
  
    /*
     * Call Zoltan
     */
#ifdef TIMER_CALLBACKS
    Timer_Callback_Time = 0.0;
#endif /* TIMER_CALLBACKS */

    MPI_Barrier(MPI_COMM_WORLD);   /* For timings only */
    stime = MPI_Wtime();

    ierr = Zoltan_Matrix_Partition(zz);

    if ((ierr != ZOLTAN_OK) && (ierr != ZOLTAN_WARN)){
      Gen_Error(0, "fatal:  error returned from Zoltan_Matrix_Partition()\n");
      return 0;
    }

    mytime = MPI_Wtime() - stime;
    MPI_Allreduce(&mytime, &maxtime, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    if (Proc == 0)
      printf("DRIVER:  Zoltan_LB_Partition time = %g\n", maxtime);
    Total_Partition_Time += maxtime;


#ifdef TIMER_CALLBACKS
    MPI_Allreduce(&Timer_Callback_Time, &Timer_Global_Callback_Time, 
                   1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    if (Proc == 0)
      printf("DRIVER:  Callback time = %g\n", Timer_Global_Callback_Time);
#endif /* TIMER_CALLBACKS */

    /* Test Zoltan_MP_Copy_Structure and Zoltan_MP_Free_Structure */
    zz_copy = Zoltan_Copy(zz);
    if (zz_copy == NULL){
        Gen_Error(0, "fatal:  Zoltan_Copy failure\n");
        return 0;
    }
    if (Zoltan_Copy_To(zz, zz_copy)){
        Gen_Error(0, "fatal:  Zoltan_Copy_To failure\n");
        return 0;
    }

    Zoltan_Destroy(&zz_copy);

    if (Debug_Driver > 2){
      /* 
       * We will call the sparse matrix partitioning queries.  Really
       * we should call Zoltan_Matrix_Partition_Eval(), but it isn't
       * written yet.  Eval() should call these queries and compute
       * a balance.
       *
       * zdrive parameter:
       * "init_dist_pins=INITIAL_COL"     we gave Zoltan CSC
       * "init_dist_pins=INITIAL_ROW or anything else"   CSR
       *
       * Zoltan parameter:
       * "MATRIX_APPROACH=MP_ROWS"  we asked Zoltan to partition rows
       * "MATRIX_APPROACH=MP_COLS"  we asked Zoltan to partition columns 
       *
       */
  
      numNZs = mesh->hindex[mesh->nhedges];
  
      ierr = sorted_unique_list((unsigned int *)mesh->hvertex, 
                                 numNZs, &gidList, &numGIDs);
  
      if (ierr != ZOLTAN_OK){
        Gen_Error(0, "fatal:  error returned from sorted_unique_list()\n");
        return 0;
      }
  
      if (pio_info->init_dist_pins != INITIAL_COL){ 
        numRows = mesh->nhedges;
        numCols = numGIDs;
        rowIDs = (unsigned int *)mesh->hgid;
        colIDs = gidList;
      }
      else{
        numRows = numGIDs;
        numCols = mesh->nhedges;
        rowIDs = gidList;
        colIDs = (unsigned int *)mesh->hgid;
      }
  
      if (Matrix_Partition_Approach == MP_ROWS){
        /* 
         * Get partitioning assignment of sparse matrix rows
         */
      
        procList = (int *)ZOLTAN_MALLOC(sizeof(int) * numRows);
        partList = (int *)ZOLTAN_MALLOC(sizeof(int) * numRows);
  
        ierr = Zoltan_MP_Get_Row_Assignment(zz, numRows, rowIDs,
                        procList, partList);
  
        if (ierr != ZOLTAN_OK){
          Gen_Error(0, 
          "fatal:  error returned from Zoltan_MP_Get_Row_Assignment()\n");
          return 0;
        }
        strcpy(rowcol, "Row");
        idList = rowIDs;
        numIDs = numRows;
      }
      else if (Matrix_Partition_Approach == MP_COLS){
        /* 
         * Get partitioning assignment of sparse matrix columns
         */
        procList = (int *)ZOLTAN_MALLOC(sizeof(int) * numCols);
        partList = (int *)ZOLTAN_MALLOC(sizeof(int) * numCols);
  
        ierr = Zoltan_MP_Get_Column_Assignment(zz, numCols, colIDs,
                        procList, partList);
  
        if (ierr != ZOLTAN_OK){
          Gen_Error(0, "fatal:  error returned from Zoltan_MP_Get_Column_Assignment()\n");
          return 0;
        }
        strcpy(rowcol, "Column");
        idList = colIDs;
        numIDs = numCols;
      }
  
      for (i=0; i<Num_Proc; i++){
        if (i == Proc){
          printf("(Process %d) %s assignments (process/partition)\n",i,rowcol);
          for (j=0; j<numIDs; j++){
            printf("%d (%d / %d)\n",idList[j],procList[j],partList[j]);
          }
          printf("\n");
          fflush(stdout);
        }
      }
      ZOLTAN_FREE(&procList);
      ZOLTAN_FREE(&partList);
      ZOLTAN_FREE(&gidList);
  
      /*
       * Get the assignment of my non-zeroes
       */
  
      procList = (int *)ZOLTAN_MALLOC(sizeof(int) * numNZs);
      partList = (int *)ZOLTAN_MALLOC(sizeof(int) * numNZs);
      idx1 = (unsigned int *)ZOLTAN_MALLOC(sizeof(unsigned int) * numNZs);
      idx2 = (unsigned int *)ZOLTAN_MALLOC(sizeof(unsigned int) * numNZs);
  
      if (numNZs && (!procList || !partList || !idx1 || !idx2)){
        Gen_Error(0, "fatal: memory error in run_zoltan_sparse_matrix\n");
        return 0;
      }
  
      for (i=0, numNZs=0; i<mesh->nhedges; i++){
        for (j=mesh->hindex[i]; j<mesh->hindex[i+1]; j++){
          idx1[numNZs] = (unsigned int)mesh->hgid[i];
          idx2[numNZs] = (unsigned int)mesh->hvertex[j];
          numNZs++;
        }
      }
  
      if (pio_info->init_dist_pins != INITIAL_COL){ 
        ierr = Zoltan_MP_Get_NonZero_Assignment(zz, numNZs,
                       idx1, idx2, procList, partList);
      }else{
        ierr = Zoltan_MP_Get_NonZero_Assignment(zz, numNZs,
                       idx2, idx1, procList, partList);
      }
      if (ierr != ZOLTAN_OK){
        Gen_Error(0, 
        "fatal:  error returned from Zoltan_MP_Get_NonZero_Assignment()\n");
        return 0;
      }
  
      fflush(stdout);
      for (i=0; i<Num_Proc; i++){
        if (i == Proc){
          printf("(Process %d) Nonzero (row, column) assignments (process / partition)\n",i);
          for (j=0; j<numNZs; j++){
            printf("(%d, %d) %d / %d\n",
              ((pio_info->init_dist_pins != INITIAL_COL) ? idx1[j] : idx2[j]),
              ((pio_info->init_dist_pins != INITIAL_COL) ? idx2[j] : idx1[j]),
               procList[j],partList[j]);
          }
          printf("\n");
          fflush(stdout);
        }
      }
      MPI_Barrier(MPI_COMM_WORLD);
      ZOLTAN_FREE(&procList);
      ZOLTAN_FREE(&partList);
      ZOLTAN_FREE(&idx1);
      ZOLTAN_FREE(&idx2);
    }
  }

  /* TODO - evaluate balance - can't use Zoltan_LB_Eval because we
   * don't require sparse_matrix app to define those queries.
   */
  
  DEBUG_TRACE_END(Proc, yo);
  return 1;
}

static int increasingOrder(const void *p1, const void *p2)
{
  unsigned int *a = (unsigned int *)p1;
  unsigned int *b = (unsigned int *)p2;

  if (*a < *b) return -1;
  else if (*a == *b) return 0;
  else return 1;
}
static int sorted_unique_list(unsigned int *l, int len, 
                              unsigned int **newl, int *newLen)
{
  int i, j, len2;
  unsigned int *l2;

  if (len < 1) return ZOLTAN_OK;

  l2 = *newl = (unsigned int *)ZOLTAN_MALLOC(sizeof(unsigned int) * len);
  if (!l2){
    return ZOLTAN_MEMERR;
  }
  memcpy(l2, l, sizeof(unsigned int) * len);

  qsort((void *)l2, len, sizeof(unsigned int), increasingOrder);

  j = 0;
  for (i=1; i<len; i++){
    if (l2[i] == l2[j]) continue;
    if (i > j)
       l2[j+1] = l2[i];
    j++;
  }
  len2 = *newLen = j+1;

  if (len2 < len){
    l2 = *newl =
    (unsigned int *)ZOLTAN_REALLOC(l2, sizeof(unsigned int) * len2);
    if (!l2){
      return ZOLTAN_MEMERR;
    }
  }

  return ZOLTAN_OK;
}
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_num_elements(void *data, int *ierr)
{
MESH_INFO_PTR mesh;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return 0;
  }

  *ierr = ZOLTAN_OK; /* set error code */

  /* testing boundary case where there are no objects at all */
  if (Test.No_Global_Objects) return 0;

  mesh = (MESH_INFO_PTR) data;

  STOP_CALLBACK_TIMER;

  if ((mesh->data_type == HYPERGRAPH) && mesh->visible_nvtx) {
    int i, cnt = 0;
    for (i = 0; i < mesh->num_elems; i++) 
      if (mesh->elements[i].globalID < mesh->visible_nvtx) cnt++;
    return(cnt);
  }
  else
    return(mesh->num_elems - mesh->blank_count);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_elements(void *data, int num_gid_entries, int num_lid_entries,
                  ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id,
                  int wdim, float *wgt, int *ierr)
{
  MESH_INFO_PTR mesh;
  ELEM_INFO *elem;
  ELEM_INFO *current_elem;
  int i, j, idx;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;

  START_CALLBACK_TIMER;


  *ierr = ZOLTAN_OK; 

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }
  
  mesh = (MESH_INFO_PTR) data;
  elem = mesh->elements;
  for (i = 0, idx = 0; i < mesh->num_elems; i++) {

    if (mesh->blank && mesh->blank[i]) continue;

    current_elem = &elem[i];
    if ((mesh->data_type == HYPERGRAPH) && mesh->visible_nvtx &&
        (current_elem->globalID >= mesh->visible_nvtx)) continue;

    for (j = 0; j < gid; j++) global_id[idx*num_gid_entries+j]=0;
    global_id[idx*num_gid_entries+gid] = (ZOLTAN_ID_TYPE) current_elem->globalID;
    if (num_lid_entries) {
      for (j = 0; j < lid; j++) local_id[idx*num_lid_entries+j]=0;
      local_id[idx*num_lid_entries+lid] = i;
    }

    if (wdim>0) {
      for (j=0; j<wdim; j++) {
        wgt[idx*wdim+j] = current_elem->cpu_wgt[j];
      }
    }
    idx++;
  }

  STOP_CALLBACK_TIMER;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_first_element(void *data, int num_gid_entries, int num_lid_entries,
                      ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id,
                      int wdim, float *wgt, int *ierr)
{
  MESH_INFO_PTR mesh;
  ELEM_INFO *elem;
  ELEM_INFO *current_elem;
  int i, j, first, allblank;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;

  START_CALLBACK_TIMER;

 *ierr = ZOLTAN_OK; 

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return 0;
  }
  
  mesh = (MESH_INFO_PTR) data;

  if (mesh->num_elems <= 0) {
    printf("%d: no elements in mesh\n",mesh->proc);
    *ierr = ZOLTAN_WARN;
    return 0;
  }

  if (mesh->num_elems == mesh->blank_count) {
    printf("%d: all elements are blanked, A\n",mesh->proc);
    *ierr = ZOLTAN_FATAL;
    return 0;
  }

  elem = mesh->elements;
  first = 0;

  if (mesh->blank_count){
    allblank = 1;
    for (i=0; i<mesh->num_elems; i++){
      if (mesh->blank[i] == 0){
        allblank = 0;
        first = i;
        break;
      }
    }
    if (allblank){
      printf("%d: all elements are blanked, B\n",mesh->proc);
      *ierr = ZOLTAN_FATAL;
      return 0;
    }
  }

  current_elem = &elem[first];
  if (num_lid_entries) {
    for (j = 0; j < lid; j++) local_id[j]=0;
    local_id[lid] = first;
  }
  for (j = 0; j < gid; j++) global_id[j]=0;
  global_id[gid] = (ZOLTAN_ID_TYPE) current_elem->globalID;

  if (wdim>0){
    for (i=0; i<wdim; i++){
      *wgt++ = current_elem->cpu_wgt[i];
      /* printf("Debug: In query function, object = %d, weight no. %1d = %f\n",
             global_id[gid], i, current_elem->cpu_wgt[i]); */
    }
  }

  STOP_CALLBACK_TIMER;

  return 1;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_next_element(void *data, int num_gid_entries, int num_lid_entries,
                     ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id,
                     ZOLTAN_ID_PTR next_global_id, ZOLTAN_ID_PTR next_local_id, 
                     int wdim, float *next_wgt, int *ierr)
{
  int found = 0;
  ELEM_INFO *elem;
  ELEM_INFO *next_elem;
  MESH_INFO_PTR mesh;
  int i, j, idx, next;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return 0;
  }
  
  mesh = (MESH_INFO_PTR) data;
  elem = mesh->elements;

  if (num_lid_entries) {
    idx = local_id[lid];
  }
  else {
    /* testing zero-length local IDs; search by global ID for current elem */
    (void) search_by_global_id(mesh, global_id[gid], &idx);
  }

  if (mesh->blank_count > 0){
    for (next=idx+1; next<mesh->num_elems; next++){
      if (mesh->blank[next] == 0){
        break;    
      }
    }
  }
  else{
    next = idx+1;
  }

  if (next < mesh->num_elems) { 
    found = 1;
    if (num_lid_entries) {
      for (j = 0; j < lid; j++) next_local_id[j]=0;
      next_local_id[lid] = next;
    }
    next_elem = &elem[next];
    for (j = 0; j < gid; j++) next_global_id[j]=0;
    next_global_id[gid] = next_elem->globalID;

    if (wdim>0){
      for (i=0; i<wdim; i++){
        *next_wgt++ = next_elem->cpu_wgt[i];
      }
    }

    *ierr = ZOLTAN_OK; 
  }

  STOP_CALLBACK_TIMER;

  return(found);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_num_geom(void *data, int *ierr)
{
  MESH_INFO_PTR mesh;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return 0;
  }
  mesh = (MESH_INFO_PTR) data;

  *ierr = ZOLTAN_OK; /* set error flag */

  STOP_CALLBACK_TIMER;

  return(mesh->num_dims);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_geom(void *data, int num_gid_entries, int num_lid_entries,
              ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id,
              double *coor, int *ierr)
{
  ELEM_INFO *elem;
  ELEM_INFO *current_elem;
  int i, idx;
  MESH_INFO_PTR mesh;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }
  mesh = (MESH_INFO_PTR) data;
  elem = mesh->elements;
  current_elem = (num_lid_entries 
                    ? &elem[local_id[lid]] 
                    : search_by_global_id(mesh, global_id[gid], &idx));

  if (mesh->eb_nnodes[current_elem->elem_blk] == 0) {
    /* No geometry info was read. */
    *ierr = ZOLTAN_FATAL;
    return;
  }
  
  /*
   * calculate the geometry of the element by averaging
   * the coordinates of the nodes in its connect table
   */
  for (i = 0; i < mesh->num_dims; i++) {
    coor[i] = current_elem->avg_coord[i];
  }

  *ierr = ZOLTAN_OK;

  STOP_CALLBACK_TIMER;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_geom_multi(void *data, int num_gid_entries, int num_lid_entries,
              int num_obj, ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id,
              int num_dim, double *coor, int *ierr)
{
ELEM_INFO *elem;
ELEM_INFO *current_elem;
MESH_INFO_PTR mesh;
int i, k, idx;
int gid = num_gid_entries - 1;
int lid = num_lid_entries - 1;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }
  mesh = (MESH_INFO_PTR) data;
  elem = mesh->elements;

  for (i = 0; i < num_obj; i++) {
    current_elem = (num_lid_entries
                    ? &elem[local_id[i*num_lid_entries+lid]]
                    : search_by_global_id(mesh,
                             global_id[i*num_gid_entries+gid], &idx));

    if (mesh->eb_nnodes[current_elem->elem_blk] == 0) {
      /* No geometry info was read. */
      *ierr = ZOLTAN_FATAL;
    }

    /*
     * calculate the geometry of the element by averaging
     * the coordinates of the nodes in its connect table
     */
    for (k = 0; k < mesh->num_dims; k++) {
      coor[i*num_dim+k] = current_elem->avg_coord[k];
    }
    if (*ierr != ZOLTAN_OK) break;
  }

  STOP_CALLBACK_TIMER;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_num_edges(void *data, int num_gid_entries, int num_lid_entries,
                  ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id, int *ierr)
{
  MESH_INFO_PTR mesh;
  ELEM_INFO *elem, *current_elem;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;
  int i, idx, nedges, ncount;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return 0;
  }
  mesh = (MESH_INFO_PTR) data;
  elem = mesh->elements;

  *ierr = ZOLTAN_OK;

  current_elem = (num_lid_entries 
                    ? &elem[local_id[lid]] 
                    : search_by_global_id(mesh, global_id[gid], &idx));

  nedges = current_elem->nadj;

  if (mesh->blank_count || current_elem->adj_blank){
    ncount = 0;
    for (i=0; i<nedges; i++){
      if (current_elem->adj_proc[i] == mesh->proc){
        if (!mesh->blank || !mesh->blank[current_elem->adj[i]]) ncount++;
      }
      else{
        if (!current_elem->adj_blank || !current_elem->adj_blank[i]) ncount++;
      }
    }
    nedges = ncount;
  }

  STOP_CALLBACK_TIMER;

  return(nedges);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_num_edges_multi(
  void *data, int num_gid_entries, int num_lid_entries, int num_obj,
  ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id, int *edges_per_obj, 
  int *ierr)
{
  MESH_INFO_PTR mesh;
  ELEM_INFO *elem, *current_elem;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;
  int i, j, idx, nedges, ncount;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }
  mesh = (MESH_INFO_PTR) data;
  elem = mesh->elements;

  *ierr = ZOLTAN_OK;

  for (i = 0; i < num_obj; i++) {
    current_elem = (num_lid_entries 
                    ? &elem[local_id[i*num_lid_entries + lid]] 
                    : search_by_global_id(mesh, 
                                          global_id[i*num_gid_entries + gid],
                                          &idx));
    nedges = current_elem->nadj;

    if (mesh->blank_count || current_elem->adj_blank){
      ncount = 0;
      for (j=0; j<nedges; j++){
        if (current_elem->adj_proc[j] == mesh->proc){
          /* Did I blank this vertex of mine */
          if (!mesh->blank || !mesh->blank[current_elem->adj[j]]) ncount++;
        }
        else{
          if (!current_elem->adj_blank || !current_elem->adj_blank[j]) ncount++;
        }
      }
      nedges = ncount;
    }

    if (mesh->visible_nvtx) {
      ncount = 0;
      for (j = 0; j < nedges; j++)  {
        if (current_elem->adj_proc[j] == mesh->proc) {
          if (mesh->elements[current_elem->adj[j]].globalID<mesh->visible_nvtx) 
            ncount++;
        }
        else {
          if (current_elem->adj[j] < mesh->visible_nvtx)
            ncount++;
        }
      }
      nedges = ncount;
    }
    edges_per_obj[i] = nedges;
  }
  STOP_CALLBACK_TIMER;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_edge_list_multi (void *data, int num_gid_entries, int num_lid_entries, 
                   int num_obj, ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id,
                   int *edge_per_obj, ZOLTAN_ID_PTR nbor_global_id, 
                   int *nbor_procs, int get_ewgts, float *nbor_ewgts, int *ierr)
{
  MESH_INFO_PTR mesh;
  ELEM_INFO *elem;
  ELEM_INFO *current_elem;
  int i, j, k, cnt, local_elem, idx;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;

  START_CALLBACK_TIMER;

  if (get_ewgts > 1) {
    Gen_Error(0, "Multiple edge weights not supported.");
    *ierr = ZOLTAN_FATAL;
    return;
  }

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }

  mesh = (MESH_INFO_PTR) data;
  elem = mesh->elements;

  j = 0;
  for (cnt = 0; cnt < num_obj; cnt++) {
    current_elem = (num_lid_entries
                     ? &elem[local_id[cnt * num_lid_entries + lid]] 
                     : search_by_global_id(mesh,
                                 global_id[cnt * num_gid_entries + gid], &idx));

    for (i = 0; i < current_elem->adj_len; i++) {

      /* Skip NULL adjacencies (sides that are not adjacent to another elem). */
      if (current_elem->adj[i] == -1) continue;

      if (current_elem->adj_proc[i] == mesh->proc) {
        local_elem = current_elem->adj[i];
      }
      else{
        local_elem = -1;
      }

      /* Skip blanked vertices */
      if (local_elem >= 0){
        /* Did I blank this vertex of mine */
        if (mesh->blank && mesh->blank[local_elem]) continue;
        if (mesh->visible_nvtx && 
            mesh->elements[local_elem].globalID >= mesh->visible_nvtx) continue;
      }
      else {
        if (current_elem->adj_blank && current_elem->adj_blank[i]) continue;
        if (mesh->visible_nvtx && 
            current_elem->adj[i] >= mesh->visible_nvtx) continue;
      }

      for (k = 0; k < gid; k++) nbor_global_id[k+j*num_gid_entries] = 0;
      if (local_elem >= 0){
        nbor_global_id[gid+j*num_gid_entries] = elem[local_elem].globalID;
      }
      else { /* adjacent element on another processor */
        nbor_global_id[gid+j*num_gid_entries] = current_elem->adj[i];
      }
      nbor_procs[j] = current_elem->adj_proc[i];

      if (get_ewgts) {
        if (current_elem->edge_wgt == NULL)
          nbor_ewgts[j] = 1.0; /* uniform weights is default */
        else
          nbor_ewgts[j] = current_elem->edge_wgt[i];
      }
      j++;
    }
  }

  *ierr = ZOLTAN_OK;
  STOP_CALLBACK_TIMER;
}
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_edge_list (void *data, int num_gid_entries, int num_lid_entries, 
                   ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id,
                   ZOLTAN_ID_PTR nbor_global_id, int *nbor_procs,
                   int get_ewgts, float *nbor_ewgts, int *ierr)
{
  MESH_INFO_PTR mesh;
  ELEM_INFO *elem;
  ELEM_INFO *current_elem;
  int i, j, k, local_elem, idx;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }

  mesh = (MESH_INFO_PTR) data;
  elem = mesh->elements;
  current_elem = (num_lid_entries
                   ? &elem[local_id[lid]] 
                   : search_by_global_id(mesh, global_id[gid], &idx));

  j = 0;
  for (i = 0; i < current_elem->adj_len; i++) {

    /* Skip NULL adjacencies (sides that are not adjacent to another elem). */
    if (current_elem->adj[i] == -1) continue;

    if (current_elem->adj_proc[i] == mesh->proc) {
      local_elem = current_elem->adj[i];
    }
    else{
      local_elem = -1;
    }

    /* Skip blanked vertices */

    if (local_elem >= 0){
      /* Did I blank this vertex of mine */
      if (mesh->blank && mesh->blank[local_elem]) continue;
    }
    else{
      if (current_elem->adj_blank && current_elem->adj_blank[i]) continue;
    }

    for (k = 0; k < gid; k++) nbor_global_id[k+j*num_gid_entries] = 0;
    if (local_elem >= 0){
      nbor_global_id[gid+j*num_gid_entries] = elem[local_elem].globalID;
    }
    else { /* adjacent element on another processor */
      nbor_global_id[gid+j*num_gid_entries] = current_elem->adj[i];
    }
    nbor_procs[j] = current_elem->adj_proc[i];

    if (get_ewgts) {
      if (current_elem->edge_wgt == NULL)
        nbor_ewgts[j] = 1.0; /* uniform weights is default */
      else
        nbor_ewgts[j] = current_elem->edge_wgt[i];
    }
    j++;
  }

  *ierr = ZOLTAN_OK;
  STOP_CALLBACK_TIMER;
}
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int get_first_coarse_element(void *data, int num_gid_entries, 
                      int num_lid_entries,
                      ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id,
                      int *assigned, int *num_vert, ZOLTAN_ID_PTR vertices,
                      int *in_order, ZOLTAN_ID_PTR in_vertex, 
                      ZOLTAN_ID_PTR out_vertex, int *ierr)
{

MESH_INFO_PTR mesh;
ELEM_INFO *elem;
ELEM_INFO *current_elem;
int gid = num_gid_entries-1;
int lid = num_lid_entries-1;
int idx, i, j;
int ok;

  START_CALLBACK_TIMER;

  *ierr = ZOLTAN_OK;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return 0;
  }


  /* 
   * Assumption:  data is same for get_first_coarse_element and 
   * get_first_element 
   */
  ok = get_first_element(data, num_gid_entries, num_lid_entries, 
                         global_id, local_id, 0, NULL, ierr);

  if (ok) {
    mesh = (MESH_INFO_PTR) data;
    elem = mesh->elements;
    current_elem = (num_lid_entries
                     ? &elem[local_id[lid]]
                     : search_by_global_id(mesh, global_id[gid], &idx));

    *assigned = 1;
    *in_order = 0;
    *num_vert = mesh->eb_nnodes[current_elem->elem_blk];
    for (i = 0; i < *num_vert; i++) {
      for (j = 0; j < gid; j++) vertices[i*num_gid_entries + j] = 0;
      vertices[i*num_gid_entries + gid] = current_elem->connect[i];
    }
  }

  STOP_CALLBACK_TIMER;

  return ok;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int get_next_coarse_element(void *data, int num_gid_entries, 
                      int num_lid_entries,
                      ZOLTAN_ID_PTR prev_global_id, ZOLTAN_ID_PTR prev_local_id,
                      ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id,
                      int *assigned, int *num_vert, ZOLTAN_ID_PTR vertices,
                      ZOLTAN_ID_PTR in_vertex, ZOLTAN_ID_PTR out_vertex,
                      int *ierr)
{

MESH_INFO_PTR mesh;
ELEM_INFO *elem;
ELEM_INFO *current_elem;
int gid = num_gid_entries-1;
int lid = num_lid_entries-1;
int idx, i, j;
int ok;

  START_CALLBACK_TIMER;

  *ierr = ZOLTAN_OK;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return 0;
  }


  /* 
   * Assumption:  data is same for get_first_coarse_element and 
   * get_first_element 
   */
  ok = get_next_element(data, num_gid_entries, num_lid_entries, 
                        prev_global_id, prev_local_id,
                        global_id, local_id,
                        0, NULL, ierr);

  if (ok) {
    mesh = (MESH_INFO_PTR) data;
    elem = mesh->elements;
    current_elem = (num_lid_entries
                     ? &elem[local_id[lid]]
                     : search_by_global_id(mesh, global_id[gid], &idx));

    *assigned = 1;
    *num_vert = mesh->eb_nnodes[current_elem->elem_blk];
    for (i = 0; i < *num_vert; i++) {
      for (j = 0; j < gid; j++) vertices[i*num_gid_entries + j] = 0;
      vertices[i*num_gid_entries + gid] = current_elem->connect[i];
    }
  }

  STOP_CALLBACK_TIMER;

  return ok;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_num_child(void *data, int num_gid_entries, int num_lid_entries, 
                  ZOLTAN_ID_PTR global_id,
                  ZOLTAN_ID_PTR local_id, int *ierr)
{
  START_CALLBACK_TIMER;
  *ierr = ZOLTAN_OK;
  STOP_CALLBACK_TIMER;
  return 0;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_child_elements(void *data, int num_gid_entries, int num_lid_entries,
                   ZOLTAN_ID_PTR parent_gid, ZOLTAN_ID_PTR parent_lid, 
                   ZOLTAN_ID_PTR child_gids, ZOLTAN_ID_PTR child_lids, 
                   int *assigned, int *num_vert, ZOLTAN_ID_PTR vertices, 
                   ZOLTAN_REF_TYPE *ref_type,
                   ZOLTAN_ID_PTR in_vertex, ZOLTAN_ID_PTR out_vertex, int *ierr)
{
  START_CALLBACK_TIMER;

  *ierr = ZOLTAN_OK;

  STOP_CALLBACK_TIMER;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_partition_multi(void *data, int num_gid_entries, int num_lid_entries,
  int num_obj, ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id, int *parts,
  int *ierr)
{
  ELEM_INFO *elem;
  ELEM_INFO *current_elem;
  MESH_INFO_PTR mesh;
  int idx, i;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }

  mesh = (MESH_INFO_PTR) data;
  elem = mesh->elements;
  for (i = 0; i < num_obj; i++) {
    current_elem = (num_lid_entries 
                    ? &elem[local_id[i*num_lid_entries + lid]] 
                    : search_by_global_id(mesh,
                                          global_id[i*num_gid_entries + gid],
                                          &idx));
    parts[i] = current_elem->my_part;
  }

  *ierr = ZOLTAN_OK;
  STOP_CALLBACK_TIMER;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_partition(void *data, int num_gid_entries, int num_lid_entries,
                  ZOLTAN_ID_PTR global_id, ZOLTAN_ID_PTR local_id, int *ierr)
{
  ELEM_INFO *elem;
  ELEM_INFO *current_elem;
  MESH_INFO_PTR mesh;
  int idx;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return -1;
  }

  mesh = (MESH_INFO_PTR) data;
  elem = mesh->elements;
  current_elem = (num_lid_entries 
                    ? &elem[local_id[lid]] 
                    : search_by_global_id(mesh, global_id[gid], &idx));


  *ierr = ZOLTAN_OK;

  STOP_CALLBACK_TIMER;

  return current_elem->my_part;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_hg_size_compressed_pin_storage(
  void *data,
  int *num_lists,
  int *num_pins,
  int *format, int *ierr)
{
  MESH_INFO_PTR mesh;
  int i;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
     return;
  }

  *ierr = ZOLTAN_OK;

  mesh = (MESH_INFO_PTR) data;

  /* testing boundary case where there are no objects at all */
  if (Test.No_Global_Objects) {
    *num_lists = *num_pins = 0;
    *format = mesh->format;
    return;
  }

  *num_lists = mesh->nhedges;
  *format = mesh->format;
  *num_pins = mesh->hindex[mesh->nhedges];
 
  if (mesh->visible_nvtx){
    /* Count #pins in "visible" part */
    *num_pins = 0;
    for (i=0; i<mesh->hindex[mesh->nhedges]; i++){
      if (mesh->hvertex[i] < mesh->visible_nvtx){
          ++(*num_pins);
      }
    }
  }
/*
{int KDD; MPI_Comm_rank(MPI_COMM_WORLD, &KDD);printf("%d KDDPINS %d  LISTS %d\n", KDD, *num_pins, *num_lists);}
*/

  STOP_CALLBACK_TIMER;
}
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_hg_size_edge_weights(
  void *data,
  int *num_edge, int *ierr)
{
  MESH_INFO_PTR mesh;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }

  mesh = (MESH_INFO_PTR) data;

  *num_edge = mesh->heNumWgts;

  STOP_CALLBACK_TIMER;

  *ierr = ZOLTAN_OK;
}
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_sparse_matrix_size(
  void *data,
  unsigned int *numrc,
  unsigned int *numnz,
  int *ierr)
{
  MESH_INFO_PTR mesh;

  START_CALLBACK_TIMER;
  *ierr = ZOLTAN_OK;

  mesh = (MESH_INFO_PTR) data;
  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    goto End;
  }

  *numrc = mesh->nhedges;
  *numnz = mesh->hindex[mesh->nhedges];
 
End:

  STOP_CALLBACK_TIMER;
} 
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_sparse_matrix(
  void *data,
  unsigned int numrc,
  unsigned int numnz,
  unsigned int *rc_gids,
  unsigned int *cr_index,
  unsigned int *cr_gids,
  int *ierr)
{
  MESH_INFO_PTR mesh;
  int i, numIDs;

  START_CALLBACK_TIMER;
  *ierr = ZOLTAN_OK;

  mesh = (MESH_INFO_PTR) data;
  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    goto End;
  }

  numIDs = mesh->nhedges;

  if ((numrc != numIDs) || (numnz != mesh->hindex[numIDs])){
    *ierr = ZOLTAN_FATAL;
    goto End;
  }

  /* we could use memcpy's , but maybe someday these won't
   * be the same objects.
   */
  for (i=0; i<numIDs; i++){
    rc_gids[i] = mesh->hgid[i];
    cr_index[i] = mesh->hindex[i];
  }
  for (i=0; i<numnz; i++){
    cr_gids[i] = mesh->hvertex[i];
  }
    
End:

  STOP_CALLBACK_TIMER;
} 
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_hg_compressed_pin_storage(
  void *data,
  int num_gid_entries,
  int nrowcol,
  int npins,
  int format,
  ZOLTAN_ID_PTR rowcol_GID,
  int *rowcol_ptr,
  ZOLTAN_ID_PTR pin_GID, int *ierr)
{
  MESH_INFO_PTR mesh;
  ZOLTAN_ID_PTR edg_GID, vtx_GID;
  int *row_ptr;
  int nedges, pins;
  int gid = num_gid_entries - 1;
  int i, j, k;

  START_CALLBACK_TIMER;
  *ierr = ZOLTAN_OK;

  mesh = (MESH_INFO_PTR) data;
  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    goto End;
  }

  if (format != mesh->format){
    *ierr = ZOLTAN_FATAL;
    goto End;
  }

  edg_GID = rowcol_GID;
  vtx_GID = pin_GID;
  row_ptr = rowcol_ptr;
  nedges = nrowcol;

  /* copy hyperedge (row) GIDs */
  for (i=0; i<nedges; i++){
    for (k=0; k<gid; k++){
      *edg_GID++ = 0;
    }
    *edg_GID++ = mesh->hgid[i];
  }

  /* copy row (hyperedge) pointers */
  memcpy(row_ptr, mesh->hindex, sizeof(int) * nedges);

  /* copy pin (vertex) GIDs */
  for (i=0; i<mesh->nhedges; i++){
    pins = 0;
    for (j=mesh->hindex[i]; j<mesh->hindex[i+1]; j++){
      if (mesh->visible_nvtx && (mesh->hvertex[j] >= mesh->visible_nvtx))
        ; /* skip pin */
      else{
        for (k=0; k<gid; k++){
          *vtx_GID++ = 0;
        }
        *vtx_GID++ = mesh->hvertex[j];
        pins++;
      }
    }
    if (mesh->visible_nvtx)
      /* overwrite row_ptr to account for only visible part */ 
      row_ptr[i+1] = row_ptr[i]+pins;
  }

End:

  STOP_CALLBACK_TIMER;
}
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
  
void get_hg_edge_weights(
  void *data, 
  int num_gid_entries,
  int num_lid_entries,
  int nedges,
  int ewgt_dim, 
  ZOLTAN_ID_PTR edge_gids,
  ZOLTAN_ID_PTR edge_lids,
  float *edge_weights, int *ierr
)
{
  MESH_INFO_PTR mesh;
  int *id=NULL;
  int i, k; 
  int gid = num_gid_entries-1;
  *ierr = ZOLTAN_OK;

  START_CALLBACK_TIMER;

  if (data == NULL){
    *ierr = ZOLTAN_FATAL;
    goto End;
  }

  mesh = (MESH_INFO_PTR) data;

  if (nedges > mesh->heNumWgts){
    *ierr = ZOLTAN_FATAL;
    goto End;
  }

  if (mesh->heWgtId){
    id = mesh->heWgtId;
  }
  else{
    id = mesh->hgid;
  }
  
  for (i=0; i<nedges; i++){
    for (k=0; k<gid; k++){
      *edge_gids++ = 0;
    }
    *edge_gids++ = id[i];
  }
    
  memcpy(edge_weights, mesh->hewgts, sizeof(float) * ewgt_dim * nedges);

  if (num_lid_entries > 0){
    for (i=0; i<nedges; i++){
      for (k=0; k<gid; k++){
        *edge_lids++ = 0;
      }
      *edge_lids++ = i;
    }
  }

End:

  STOP_CALLBACK_TIMER;
  return; 
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_num_fixed_obj(void *data, int *ierr)
{
MESH_INFO_PTR mesh;
int i, cnt;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return 0;
  }
  mesh = (MESH_INFO_PTR) data;

  *ierr = ZOLTAN_OK; 

  if (Test.No_Global_Objects) {
    return 0;
  }

  for (cnt=0, i = 0; i < mesh->num_elems; i++){
    if (mesh->blank && mesh->blank[i]) continue;
    if (mesh->elements[i].fixed_part != -1) 
      cnt++;
  }

  STOP_CALLBACK_TIMER;

  return(cnt);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void get_fixed_obj_list(void *data, int num_fixed_obj, 
  int num_gid_entries, ZOLTAN_ID_PTR fixed_gids, int *fixed_part, int *ierr)
{
MESH_INFO_PTR mesh;
int i, cnt;
int ngid = num_gid_entries-1;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    printf("Bad data field in get_fixed_obj_list\n");
    return;
  }
  mesh = (MESH_INFO_PTR) data;

  cnt = 0;
  for (i = 0; i < mesh->num_elems; i++){
    if (mesh->blank && mesh->blank[i]) continue;
    if (mesh->elements[i].fixed_part != -1) {
      fixed_gids[cnt*num_gid_entries+ngid] =
                 (ZOLTAN_ID_TYPE) mesh->elements[i].globalID;
      fixed_part[cnt] = mesh->elements[i].fixed_part;
      cnt++;
    }
  }

  if (cnt == num_fixed_obj)
    *ierr = ZOLTAN_OK; /* set error code */
  else {
    printf("Count mismatch in get_fixed_obj_list %d %d \n", cnt, num_fixed_obj);
    *ierr = ZOLTAN_FATAL;
  }

  STOP_CALLBACK_TIMER;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
ELEM_INFO *search_by_global_id(MESH_INFO *mesh, int global_id, int *idx)
{
/*
 * Function that searchs for an element based upon its global ID.
 * This function does not provide the most efficient implementation of
 * the query functions; more efficient implementation uses local IDs
 * to directly access element info.  However, this function is useful
 * for testing Zoltan when the number of entries in a local ID 
 * (NUM_LID_ENTRIES) is zero.
 */

int i;
ELEM_INFO *elem, *found_elem = NULL;


  elem = mesh->elements;

  for (i = 0; i < mesh->elem_array_len; i++)
    if (elem[i].globalID == global_id) {
      found_elem = &elem[i];
      *idx = i;
      break;
    }
  
  return(found_elem);
}

/*****************************************************************************/
/*****************************************************************************/

static void test_point_drops(FILE *, double *, struct Zoltan_Struct *,
  int, int *, int, int *, int, int);
static void test_box_drops(FILE *, double *, double *, struct Zoltan_Struct *, 
  int, int, int, int);

static void test_drops(
  int Proc,
  MESH_INFO_PTR mesh, 
  PARIO_INFO_PTR pio_info,
  struct Zoltan_Struct *zz
)
{
FILE *fp;
double xlo[3], xhi[3];
char par_out_fname[FILENAME_MAX+1], ctemp[FILENAME_MAX+1];
int i;
int Num_Proc;
int max_part, gmax_part;
int test_both;  /* If true, test both Zoltan_*_Assign and Zoltan_*_PP_Assign. */
                /* If false, test only Zoltan_*_PP_Assign.                    */
                /* True if # partitions == # processors.                      */

  /* Find maximum partition number across all processors. */
  MPI_Comm_size(MPI_COMM_WORLD, &Num_Proc);
  max_part = gmax_part = -1;
  for (i = 0; i < mesh->num_elems; i++)
    if (mesh->elements[i].my_part > max_part)
      max_part = mesh->elements[i].my_part;
  MPI_Allreduce(&max_part, &gmax_part, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  test_both = ((gmax_part == (Num_Proc-1)) && (Test.Local_Partitions == 0));

  /* generate the parallel filename for this processor */
  strcpy(ctemp, pio_info->pexo_fname);
  strcat(ctemp, ".drops");
  gen_par_filename(ctemp, par_out_fname, pio_info, Proc, Num_Proc);
  fp = fopen(par_out_fname, "w");

  /* Test unit box */
  xlo[0] = xlo[1] = xlo[2] = 0.0L;
  xhi[0] = xhi[1] = xhi[2] = 1.0L;
  test_box_drops(fp, xlo, xhi, zz, Proc, -1, -1, test_both);

  /* Test box based on this processor */
  if (mesh->num_elems > 0) {
    double x[3] = {0., 0., 0.};
    int iierr = 0;
    ELEM_INFO_PTR current_elem = &(mesh->elements[0]);
    unsigned int lid = 0;
    unsigned int gid = (unsigned) (current_elem->globalID);

    if (mesh->eb_nnodes[current_elem->elem_blk] == 1) {
      x[0] = current_elem->coord[0][0];
      if (mesh->num_dims > 1)
        x[1] = current_elem->coord[0][1];
      if (mesh->num_dims > 2)
        x[2] = current_elem->coord[0][2];
    }
    else 
      get_geom((void *) mesh, 1, 1, &gid, &lid, x, &iierr);

    xlo[0] = x[0];
    xlo[1] = x[1];
    xlo[2] = x[2];
    xhi[0] = x[0] + 1.0;
    xhi[1] = x[1] + 2.0;
    xhi[2] = x[2] + 3.0;
    test_box_drops(fp, xlo, xhi, zz, Proc, Proc, current_elem->my_part,
                   test_both);
  }

  /* Test box that (most likely) includes the entire domain. */
  /* All partitions and processors with partitions should be in the output.  */
  xlo[0] = -1000000.;
  xlo[1] = -1000000.;
  xlo[2] = -1000000.;
  xhi[0] = 1000000.;
  xhi[1] = 1000000.;
  xhi[2] = 1000000.;
  test_box_drops(fp, xlo, xhi, zz, Proc, 
                ((max_part >= 0) ? Proc : -1), /* do not test for proc if
                                                  proc has no partitions */
                -1, test_both);

  fclose(fp);
}

/*****************************************************************************/
static void test_point_drops(
  FILE *fp, 
  double *x, 
  struct Zoltan_Struct *zz, 
  int Proc,
  int *procs,
  int proccnt,
  int *parts,
  int partcnt,
  int test_both
)
{
int status;
int one_part, one_proc;
int i;

  if (test_both) {
    status = Zoltan_LB_Point_Assign(zz, x, &one_proc);
    if (status != ZOLTAN_OK) 
      fprintf(fp, "error returned from Zoltan_LB_Point_Assign()\n");
    else  {
      fprintf(fp, "%d Zoltan_LB_Point_Assign    (%e %e %e) on proc %d\n",
              Proc, x[0], x[1], x[2], one_proc);
      for (i = 0; i < proccnt; i++) 
        if (one_proc == procs[i]) 
          break;
      if (i == proccnt) 
        fprintf(fp, "%d Error:  processor %d (from Zoltan_LB_Point_Assign) "
                    "not in proc list from Zoltan_LB_Box_Assign\n", 
                    Proc, one_proc);
    }
  }
  else fprintf(fp, "%d Zoltan_LB_Point_Assign not tested.\n", Proc);

  status = Zoltan_LB_Point_PP_Assign(zz, x, &one_proc, &one_part);
  if (status != ZOLTAN_OK) 
    fprintf(fp, "error returned from Zoltan_LB_Point_PP_Assign()\n");
  else {
    fprintf(fp, "%d Zoltan_LB_Point_PP_Assign (%e %e %e) on proc %d part %d\n",
            Proc, x[0], x[1], x[2], one_proc, one_part);

    for (i = 0; i < proccnt; i++) 
      if (one_proc == procs[i]) 
        break;
    if (i == proccnt) 
      fprintf(fp, "%d Error:  processor %d (from Zoltan_LB_Point_PP_Assign) "
                  "not in proc list from Zoltan_LB_Box_PP_Assign\n", 
                  Proc, one_proc);

    if (parts != NULL) {
      for (i = 0; i < partcnt; i++) 
        if (one_part == parts[i]) 
          break;
      if (i == partcnt) 
        fprintf(fp, "%d Error:  partition %d (from Zoltan_LB_Point_PP_Assign) "
                    "not in part list from Zoltan_LB_Box_PP_Assign\n", 
                    Proc, one_part);
    }
  }
}

/*****************************************************************************/
static void test_box_drops(
  FILE *fp, 
  double *xlo, 
  double *xhi, 
  struct Zoltan_Struct *zz, 
  int Proc, 
  int answer_proc,   /* If >= 0, an expected answer for proc. */
  int answer_part,   /* If >= 0, an expected answer for part. */
  int test_both
)
{
int status, procfound, partfound;
int proccnt, partcnt;
int procs[1000], parts[1000];
double x[3];
int i;

  fprintf(fp, "\n-------------------------------------------------------\n");
  if (test_both) {
    status = Zoltan_LB_Box_Assign(zz, xlo[0], xlo[1], xlo[2], 
                                      xhi[0], xhi[1], xhi[2], 
                                      procs, &proccnt);
    if (status != ZOLTAN_OK) 
      fprintf(fp, "error returned from Zoltan_LB_Box_Assign()\n");
    else {
      fprintf(fp, "%d Zoltan_LB_Box_Assign    LO: (%e %e %e)\n"
                  "%d                         HI: (%e %e %e)\n", 
                  Proc, xlo[0], xlo[1], xlo[2], Proc, xhi[0], xhi[1], xhi[2]);
  
      procfound = 0;
      fprintf(fp, "       On %d Procs: ", proccnt);
      for (i = 0; i < proccnt; i++) {
        fprintf(fp, "%d ", procs[i]);
        if (procs[i] == answer_proc) procfound = 1;
      }
      fprintf(fp, "\n");
      if (answer_proc >= 0 && !procfound)
        fprintf(fp, "%d Zoltan_LB_Box_Assign error:  "
                     "expected proc %d not in output proc list\n",
                      Proc, answer_proc);
    }
  }
  else fprintf(fp, "%d Zoltan_LB_Box_Assign not tested.\n", Proc);


  status = Zoltan_LB_Box_PP_Assign(zz, xlo[0], xlo[1], xlo[2], 
                                       xhi[0], xhi[1], xhi[2], 
                                       procs, &proccnt, 
                                       parts, &partcnt);
  if (status != ZOLTAN_OK) 
    fprintf(fp, "error returned from Zoltan_LB_Box_PP_Assign()\n");
  else {
    fprintf(fp, "%d Zoltan_LB_Box_PP_Assign LO: (%e %e %e)\n"
                "%d                         HI: (%e %e %e)\n", 
                Proc, xlo[0], xlo[1], xlo[2], Proc, xhi[0], xhi[1], xhi[2]);

    procfound = 0;
    fprintf(fp, "       On %d Procs: ", proccnt);
    for (i = 0; i < proccnt; i++) {
      fprintf(fp, "%d ", procs[i]);
      if (procs[i] == answer_proc) procfound = 1;
    }
    fprintf(fp, "\n");

    partfound = 0;
    fprintf(fp, "       In %d Parts: ", partcnt);
    for (i = 0; i < partcnt; i++) {
      fprintf(fp, "%d ", parts[i]);
      if (parts[i] == answer_part) partfound = 1;
    }
    fprintf(fp, "\n");
    if (answer_proc >= 0 && !procfound)
      fprintf(fp, "%d Zoltan_LB_Box_PP_Assign error:  "
                   "expected proc %d not in output proc list\n",
                    Proc, answer_proc);
    if (answer_part >= 0 && !partfound)
      fprintf(fp, "%d Zoltan_LB_Box_PP_Assign error:  "
                  "expected part %d not in output part list\n",
                  Proc, answer_part);

    /* Test point assign */
    test_point_drops(fp, xlo, zz, Proc, procs, proccnt, parts, partcnt, 
                     test_both);
    test_point_drops(fp, xhi, zz, Proc, procs, proccnt, parts, partcnt, 
                     test_both);
    x[0] = 0.5 * (xlo[0] + xhi[0]);
    x[1] = 0.5 * (xlo[1] + xhi[1]);
    x[2] = 0.5 * (xlo[2] + xhi[2]);
    test_point_drops(fp, x, zz, Proc, procs, proccnt, parts, partcnt, 
                     test_both);
  }
}

#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif
