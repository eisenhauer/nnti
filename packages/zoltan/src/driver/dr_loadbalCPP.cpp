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

#include <mpi.h>   // must appear before stdio or iostream

#ifdef TFLOP
#include <iostream.h>
#else
#include <iostream>
using namespace std;
#endif

#include "dr_const.h"
#include "dr_err_const.h"
#include "dr_loadbal_const.h"
#include "dr_eval_const.h"
#include "dr_util_const.h"
#include "ch_init_dist_const.h"

#include "zoltan_cpp.h"

static void test_drops(int, MESH_INFO_PTR, PARIO_INFO_PTR, Zoltan_Object &);

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

ZOLTAN_HG_EDGE_LIST_FN get_hg_edge_list;
ZOLTAN_HG_EDGE_INFO_FN get_hg_edge_info;
ZOLTAN_NUM_HG_EDGES_FN get_num_hg_edges;

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int setup_zoltan(Zoltan_Object &zz, int Proc, PROB_INFO_PTR prob,
                 MESH_INFO_PTR mesh)
{
/* Local declarations. */
  const char *yo = "setup_zoltan";
  int ierr;                      /* Error code */
  char errmsg[128];              /* Error message */

  DEBUG_TRACE_START(Proc, yo);

  /* Allocate space for arrays. */
  int nprocs = 0;
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  
  float *psize = new float [nprocs];
  int *partid = new int [2*nprocs];
  int *idx = partid + nprocs;

  /* Set the user-specified parameters */
  for (int i = 0; i < prob->num_params; i++) {
    if (prob->params[i].Index>=0)
      ierr = zz.Set_Param_Vec(prob->params[i].Name, prob->params[i].Val,
             prob->params[i].Index);
    else
      ierr = zz.Set_Param(prob->params[i].Name, prob->params[i].Val);
    if (ierr == ZOLTAN_FATAL) {
      sprintf(errmsg,
              "fatal: error in Zoltan_Set_Param when setting parameter %s\n",
              prob->params[i].Name);
      Gen_Error(0, errmsg);
      delete [] psize;
      delete [] partid;
      return 0;
    }
  }

  /* Set the load-balance method */
  if (zz.Set_Param("LB_METHOD", prob->method) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Param(LB_METHOD)\n");
    delete [] psize;
    delete [] partid;
    return 0;
  }

  if (Test.Local_Partitions == 1) {
    /* Compute Proc partitions for each processor */
    char s[8];
    sprintf(s, "%d", Proc);
    if (zz.Set_Param("NUM_LOCAL_PARTITIONS", s) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Param()\n");
      delete [] psize;
      delete [] partid;
      return 0;
    }
  }
  else if (Test.Local_Partitions == 2) {
    /* Compute Proc partitions for odd-ranked processors; let remaining
     * partitions be in even-ranked processors. */
    if (Proc%2) {
      char s[8];
      sprintf(s, "%d", Proc);
      if (zz.Set_Param("NUM_LOCAL_PARTITIONS", s) == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from Zoltan_Set_Param()\n");
        delete [] psize;
        delete [] partid;
        return 0;
      }
    }
  }
  else if (Test.Local_Partitions == 3 || Test.Local_Partitions == 5) {
    /* Variable partition sizes, but one partition per proc */
    /* Test.Local_Partitions == 5 is same as 3, but with sizes increased by 1 */
    /* to avoid zero-sized partitions (for ParMETIS tests). */
    int i = 0;
    psize[0] = (float) (Proc + (Test.Local_Partitions == 5)); 
    /* Set partition sizes using global numbers. */
    zz.LB_Set_Part_Sizes(1, 1, &Proc, &i, psize);
    /* Reset partition sizes for upper half of procs. */
    if (Proc >= nprocs/2){
      psize[0] = 0.5 + (Proc%2) + (Test.Local_Partitions == 5);
      zz.LB_Set_Part_Sizes(1, 1, &Proc, &i, psize);
    }
  }
  else if (Test.Local_Partitions == 4) {
    /* Variable number of partitions per proc and variable sizes. */
    /* Request Proc partitions for each processor, of size 1/Proc.  */
    char s[8];
    sprintf(s, "%d", Proc);
    if (zz.Set_Param("NUM_LOCAL_PARTITIONS", s) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Param()\n");
      delete [] psize;
      delete [] partid;
      return 0;
    }
    /* Each partition size is inverse to the no. of partitions on a proc. */ 
    for (int i=0; i<Proc; i++){
      partid[i] = i;                    /* Local partition number */
      idx[i] = 0;
      psize[i] = 1.0/Proc; 
    }
    zz.LB_Set_Part_Sizes(0, Proc, partid, idx, psize);
  }
  else if (Test.Local_Partitions == 6) {
    /* Variable partition sizes, but one partition per proc */
    /* When nprocs >= 6, zero-sized partitions on processors >= 2. */
    int i = 0;
    psize[0] = (float) Proc;
    /* Set partition sizes using global numbers. */
    zz.LB_Set_Part_Sizes(1, 1, &Proc, &i, psize);
    /* Reset partition sizes for procs near end. */
    if (nprocs >= 6 && Proc >= nprocs-4){
      psize[0] = 0.;
      zz.LB_Set_Part_Sizes(1, 1, &Proc, &i, psize);
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
    int i = 0;
    psize[0] = (float) Proc;
    /* Set partition sizes using global numbers. */
    zz.LB_Set_Part_Sizes(1, 1, &Proc, &i, psize);
    /* Reset partition sizes for procs near beginning. */
    if (nprocs >= 6 && Proc < 4){
      psize[0] = 0.;
      zz.LB_Set_Part_Sizes(1, 1, &Proc, &i, psize);
    }
    else if (nprocs < 6) {
      Gen_Error(0, "warning:  Test Local Partitions = 7 should be run "
                    "on six or more processors.\n");
      error_report(Proc);
    }
  }

  /* Free temporary arrays for partition sizes. */

  delete [] psize;
  delete [] partid;

  /*
   * Set the callback functions
   */

  if (zz.Set_Num_Obj_Fn(get_num_elements, (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Num_Obj_Fn()\n");
    return 0;
  }

  if (Test.Multi_Callbacks) {
    if (zz.Set_Obj_List_Fn(get_elements, (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Obj_List_Fn()\n");
      return 0;
    }
  }
  else {
    if (zz.Set_First_Obj_Fn(get_first_element, (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_First_Obj_Fn()\n");
      return 0;
    }

    if (zz.Set_Next_Obj_Fn(get_next_element, (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Next_Obj_Fn()\n");
      return 0;
    }
  }

  /* Functions for geometry based algorithms */
  if (zz.Set_Num_Geom_Fn(get_num_geom, (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Num_Geom_Fn()\n");
    return 0;
  }

  if (Test.Multi_Callbacks) {
    if (zz.Set_Geom_Multi_Fn(get_geom_multi, (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Geom_Multi_Fn()\n");
      return 0;
    }
  }
  else {
    if (zz.Set_Geom_Fn(get_geom, (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Set_Geom_Fn()\n");
      return 0;
    }
  }

  /* Functions for graph based algorithms */
  if (Test.Multi_Callbacks) {
    if (zz.Set_Num_Edges_Multi_Fn(get_num_edges_multi, 
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Num_Edges_Multi_Fn()\n");
      return 0;
    }
    if (zz.Set_Edge_List_Multi_Fn(get_edge_list_multi,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Edge_List_Multi_Fn()\n");
      return 0;
    }
#ifdef PARMETIS_V3_1_MEMORY_ERROR_FIXED
    /* Used in ParMETIS to reduce data movement */
    if (zz.Set_Obj_Size_Multi_Fn(migrate_elem_size_multi,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Obj_Size_Multi_Fn()\n");
      return 0;
    }
#endif /* PARMETIS_V3_1_MEMORY_ERROR_FIXED */
  }
  else {
    if (zz.Set_Num_Edges_Fn(get_num_edges, (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Num_Edges_Fn()\n");
      return 0;
    }
    if (zz.Set_Edge_List_Fn(get_edge_list, (void *) mesh)== ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Edge_List_Fn()\n");
      return 0;
    }
#ifdef PARMETIS_V3_1_MEMORY_ERROR_FIXED
    /* Used in ParMETIS to reduce data movement */
    if (zz.Set_Obj_Size_Fn(migrate_elem_size, (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Obj_Size_Fn()\n");
      return 0;
    }
#endif /* PARMETIS_V3_1_MEMORY_ERROR_FIXED */
  }


  /* Functions for tree-based algorithms */
  if (zz.Set_Num_Coarse_Obj_Fn(get_num_elements, (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Num_Coarse_Obj_Fn()\n");
    return 0;
  }

  if (zz.Set_First_Coarse_Obj_Fn(get_first_coarse_element,
                    (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_First_Coarse_Obj_Fn()\n");
    return 0;
  }

  if (zz.Set_Next_Coarse_Obj_Fn(get_next_coarse_element,
                    (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Next_Coarse_Obj_Fn()\n");
    return 0;
  }

  if (zz.Set_Num_Child_Fn(get_num_child, (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, "fatal:  error returned from Zoltan_Set_Num_Child_Fn()\n");
    return 0;
  }

  if (zz.Set_Child_List_Fn(get_child_elements, (void *) mesh) == ZOLTAN_FATAL) {
    Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Child_List_Fn()\n");
    return 0;
  }

  if (mesh->data_type == HYPERGRAPH) {
    if (zz.Set_Num_HG_Edges_Fn(get_num_hg_edges, 
         (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Num_HG_Edges_Fn()\n");
      return 0;
    }

    if (zz.Set_HG_Edge_Info_Fn( get_hg_edge_info, 
                         (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_HG_Edge_Info_Fn()\n");
      return 0;
    }

    if (zz.Set_HG_Edge_List_Fn( get_hg_edge_list, 
                         (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_HG_Edge_List_Fn()\n");
      return 0;
    }
  }

  /* Functions for partitions */
  if (Test.Multi_Callbacks) {
    if (zz.Set_Partition_Multi_Fn(get_partition_multi, 
                  (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Partition_Multi_Fn()\n");
      return 0;
    }
  }
  else {
    if (zz.Set_Partition_Fn(get_partition, (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, 
        "fatal:  error returned from Zoltan_Set_Partition_Fn()\n");
      return 0;
    }
  }

  DEBUG_TRACE_END(Proc, yo);
  return 1;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int run_zoltan(Zoltan_Object &zz, int Proc, PROB_INFO_PTR prob,
               MESH_INFO_PTR mesh, PARIO_INFO_PTR pio_info)
{
/* Local declarations. */
  const char *yo = "run_zoltan";

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
  int dummy;                     /* unused parameters */

/***************************** BEGIN EXECUTION ******************************/

  DEBUG_TRACE_START(Proc, yo);

  if (Driver_Action & 1){

    /* Load balancing part */
  
    /* Evaluate the old balance */
    if (Debug_Driver > 0) {
      if (Proc == 0) cout << "\nBEFORE load balancing" << endl;
      driver_eval(mesh);
      int i = zz.LB_Eval(1, dummy, NULL, dummy, NULL, dummy, dummy);
      if (i) cout << "Warning: Zoltan_LB_Eval returned code " << i << endl;
    }
    if (Test.Gen_Files) {
      /* Write output files. */
      string beforeName(pio_info->pexo_fname);
      beforeName.append(".before");
      zz.Generate_Files(beforeName, 1, 1, 1, 0);
    }
  
    /*
     * Call Zoltan
     */
#ifdef TIMER_CALLBACKS
    Timer_Callback_Time = 0.0;
#endif /* TIMER_CALLBACKS */

    MPI_Barrier(MPI_COMM_WORLD);   /* For timings only */

    double stime = MPI_Wtime();

    if (zz.LB_Partition(new_decomp, num_gid_entries, num_lid_entries,
                 num_imported, import_gids,
                 import_lids, import_procs, import_to_part,
                 num_exported, export_gids,
                 export_lids, export_procs, export_to_part) == ZOLTAN_FATAL){
      Gen_Error(0, "fatal:  error returned from Zoltan_LB_Partition()\n");
      return 0;
    }
    double mytime = MPI_Wtime() - stime;
    double maxtime = 0.0;
    MPI_Allreduce(&mytime, &maxtime, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

    if (Proc == 0)
      cout << "DRIVER:  Zoltan_LB_Partition time = " << maxtime << endl;
    Total_Partition_Time += maxtime;

#ifdef TIMER_CALLBACKS
    MPI_Allreduce(&Timer_Callback_Time, &Timer_Global_Callback_Time, 
                   1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    if (Proc == 0)
      cout << "DRIVER:  Callback time = " << Timer_Global_Callback_Time << endl;
#endif /* TIMER_CALLBACKS */


    {int mine[2], gmax[2], gmin[2];
    mine[0] = num_imported;
    mine[1] = num_exported;
    MPI_Allreduce(mine, gmax, 2, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    MPI_Allreduce(mine, gmin, 2, MPI_INT, MPI_MIN, MPI_COMM_WORLD);

    if (Proc == 0) {
      cout << "DRIVER:  Min/Max Import: " << gmin[0] << " " <<  gmax[0] << endl;
      cout << "DRIVER:  Min/Max Export: " << gmin[1] << " " <<  gmax[1] << endl;
    }
    }

#ifdef ZOLTAN_NEMESIS
    if (pio_info->file_type == NEMESIS_FILE && Output.Nemesis) {
      int i = write_elem_vars(Proc, mesh, pio_info, num_exported, export_gids,
                          export_procs, export_to_part);

      if (i ==0)
        {
        Gen_Error(0, "fatal:  error returned from write_elem_vars()\n");
        return 0;
        }
    }
#endif
  
    /*
     * Call another routine to perform the migration
     */
    MPI_Barrier(MPI_COMM_WORLD);   /* For timings only */
    stime = MPI_Wtime();
    if (new_decomp && num_exported != -1 && num_imported != -1) {
      /* Migrate if new decomposition and RETURN_LISTS != NONE */
      if (!migrate_elements(Proc, mesh, zz, num_gid_entries, num_lid_entries,
          num_imported, import_gids, import_lids, import_procs, import_to_part,
          num_exported, export_gids, export_lids, export_procs, export_to_part))
      {
        Gen_Error(0, "fatal:  error returned from migrate_elements()\n");
        return 0;
      }
    }
    mytime = MPI_Wtime() - stime;
    MPI_Allreduce(&mytime, &maxtime, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    if (Proc == 0)
      cout << "DRIVER:  Total migration time = " << maxtime << endl;
  
    /* Evaluate the new balance */
    if (Debug_Driver > 0) {
      if (Proc == 0) cout << "\nAFTER load balancing\n" << endl;
      driver_eval(mesh);
      int i = zz.LB_Eval(1, dummy, NULL, dummy, NULL, dummy, dummy);
      if (i) cout << "Warning: Zoltan_LB_Eval returned code " << i << endl;
    }
    if (Test.Gen_Files) {
      /* Write output files. */
      string afterName(pio_info->pexo_fname);
      afterName.append(".after");
      zz.Generate_Files(afterName, 1, 1, 1, 0);
    }
  
    if (Test.Drops)
      test_drops(Proc, mesh, pio_info, zz);

    if (!(strcasecmp(prob->method, "RCB"))) {
      double xmin, ymin, zmin;
      double xmax, ymax, zmax;
      int ndim;
      int ierr = zz.RCB_Box(Proc, ndim, xmin, ymin, zmin,
                            xmax, ymax, zmax);
      if (!ierr) {
        cout << "DRIVER " << Proc << " DIM: " << ndim;
        cout << " BOX: (" << xmin << "," << ymin << "," << zmin << ") -- ";
        cout << "(" << xmax << "," << ymax << "," << zmax << ")" << endl;
      }
    }

    /* Clean up */
    zz.LB_Free_Part(import_gids, import_lids, import_procs, import_to_part);
    zz.LB_Free_Part(export_gids, export_lids, export_procs, export_to_part);
  }

  if (Driver_Action & 2){
    /* Only do ordering if this was specified in the driver input file */

    int *order = new int [2*(mesh->num_elems)];
    ZOLTAN_ID_PTR order_gids = new ZOLTAN_ID_TYPE [mesh->num_elems];
    ZOLTAN_ID_PTR order_lids = new ZOLTAN_ID_TYPE [mesh->num_elems];

    /* Evaluate the old ordering */
    if (Debug_Driver > 0) {
      if (Proc == 0) cout << "\nBEFORE ordering" << endl;
      /* Not yet impl. */
    }

    if (zz.Order(num_gid_entries, num_lid_entries,
        mesh->num_elems, order_gids, order_lids,
        order, &order[mesh->num_elems]) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from Zoltan_Order()\n");
      delete [] order;
      delete [] order_gids;
      delete [] order_lids;
      return 0;
    }

    /* Evaluate the new ordering */
    if (Debug_Driver > 0) {
      if (Proc == 0) cout << "\nAFTER ordering" << endl;
      /* Not yet impl. */
    }

    /* Copy ordering permutation into mesh structure */
    for (int i = 0; i < mesh->num_elems; i++){
      int lid = order_lids[num_lid_entries * i + (num_lid_entries - 1)];
      mesh->elements[lid].perm_value = order[i];
      mesh->elements[lid].invperm_value = order[(mesh->num_elems)+i];
    }

    /* Free order data */
    delete [] order;
    delete [] order_gids;
    delete [] order_lids;
  }

  DEBUG_TRACE_END(Proc, yo);
  return 1;
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
  mesh = (MESH_INFO_PTR) data;

  *ierr = ZOLTAN_OK; /* set error code */

  STOP_CALLBACK_TIMER;

  return(mesh->num_elems);
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
  int i, j;
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
  for (i = 0; i < mesh->num_elems; i++) {
    current_elem = &elem[i];
    global_id[i*num_gid_entries+gid] = (ZOLTAN_ID_TYPE) current_elem->globalID;
    if (num_lid_entries) 
      local_id[i*num_lid_entries+lid] = i;

    if (wdim>0) {
      for (j=0; j<wdim; j++) {
        wgt[i*wdim+j] = current_elem->cpu_wgt[j];
      }
    }
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
  int i;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;

  START_CALLBACK_TIMER;

 *ierr = ZOLTAN_OK; 

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return 0;
  }
  
  mesh = (MESH_INFO_PTR) data;
  if (mesh->num_elems == 0) {
    /* No elements on this processor */
    return 0;
  }

  elem = mesh->elements;
  current_elem = &elem[0];
  if (num_lid_entries) local_id[lid] = 0;
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
  int i, idx;
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

  if (idx+1 < mesh->num_elems) { 
    found = 1;
    if (num_lid_entries) next_local_id[lid] = idx + 1;
    next_elem = &elem[idx+1];
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
  int idx;

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
  STOP_CALLBACK_TIMER;

  return(current_elem->nadj);
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
  int i, idx;

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
    edges_per_obj[i] = current_elem->nadj;
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
  int i, j, cnt, local_elem, idx;
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

  /* get the processor number */

  int proc = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &proc);

  j = 0;
  for (cnt = 0; cnt < num_obj; cnt++) {
    current_elem = (num_lid_entries
                     ? &elem[local_id[cnt * num_lid_entries + lid]] 
                     : search_by_global_id(mesh,
                                 global_id[cnt * num_gid_entries + gid], &idx));

    for (i = 0; i < current_elem->adj_len; i++) {

      /* Skip NULL adjacencies (sides that are not adjacent to another elem). */
      if (current_elem->adj[i] == -1) continue;

      if (current_elem->adj_proc[i] == proc) {
        local_elem = current_elem->adj[i];
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
  int i, j, local_elem, idx;
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

  /* get the processor number */
  int proc = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &proc);

  j = 0;
  for (i = 0; i < current_elem->adj_len; i++) {

    /* Skip NULL adjacencies (sides that are not adjacent to another elem). */
    if (current_elem->adj[i] == -1) continue;

    if (current_elem->adj_proc[i] == proc) {
      local_elem = current_elem->adj[i];
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
int idx, i;
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
    for (i = 0; i < *num_vert; i++)
      vertices[i*num_gid_entries + gid] = current_elem->connect[i];
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
int idx, i;
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
    for (i = 0; i < *num_vert; i++)
      vertices[i*num_gid_entries + gid] = current_elem->connect[i];
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
int get_num_hg_edges(
  void *data, 
  int *ierr)
{
  MESH_INFO_PTR mesh;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return -1;
  }

  mesh = (MESH_INFO_PTR) data;
  *ierr = ZOLTAN_OK;

  STOP_CALLBACK_TIMER;

  return mesh->nhedges;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_num_hg_pins(
  void *data, 
  int *ierr)
{
  MESH_INFO_PTR mesh;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return -1;
  }

  mesh = (MESH_INFO_PTR) data;

  *ierr = ZOLTAN_OK;

  STOP_CALLBACK_TIMER;

  return mesh->hindex[mesh->nhedges];
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_hg_edge_info(
  void *data,
  int num_gid_entries,
  int num_lid_entries,
  int nedges,
  int ewgt_dim,
  ZOLTAN_ID_PTR edge_gids,
  ZOLTAN_ID_PTR edge_lids,
  int *edge_sizes,
  float *edge_weights
)
{
  MESH_INFO_PTR mesh;
  int i, j;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;
  int *hindex;
  int ierr = ZOLTAN_OK;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    ierr = ZOLTAN_FATAL;
    goto End;
  }
   
  mesh = (MESH_INFO_PTR) data;
  hindex = mesh->hindex;
  if (nedges != mesh->nhedges) {
    ierr = ZOLTAN_FATAL;
    goto End;
  }
   
  for (i = 0; i < mesh->nhedges; i++) {
    edge_gids[i*num_gid_entries+gid] = mesh->hgid[i];
    if (num_lid_entries) edge_lids[i*num_lid_entries+lid] = i;
    edge_sizes[i] = hindex[i+1] - hindex[i];
    for (j = 0; j < ewgt_dim; j++) {
      if (mesh->hewgt_dim > j)
        edge_weights[j + i*ewgt_dim] = mesh->hewgts[j + i*mesh->hewgt_dim];
      else
        edge_weights[j + i*ewgt_dim] = 1.0;
    }
  }  

End:
    
  STOP_CALLBACK_TIMER;
  return ierr;
}
 
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int get_hg_edge_list(
  void *data,
  int num_gid_entries,
  int num_lid_entries,
  int nedges,
  ZOLTAN_ID_PTR edge_gids,
  ZOLTAN_ID_PTR edge_lids,
  int *edge_sizes,
  ZOLTAN_ID_PTR edge_verts,
  int *edge_procs
)
{
  MESH_INFO_PTR mesh;
  int i, j, k, local_id;
  int pcnt;
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;
  int *hindex;
  int ierr = ZOLTAN_OK;

  START_CALLBACK_TIMER;

  if (data == NULL) {
    ierr = ZOLTAN_FATAL;
    goto End;
  }
   
  mesh = (MESH_INFO_PTR) data;
  hindex = mesh->hindex;

  pcnt = 0;
  for (i = 0; i < nedges; i++) {
    local_id = (num_lid_entries ? edge_lids[i*num_lid_entries+lid]
                                : in_list(edge_gids[i*num_gid_entries+gid],
                                          mesh->nhedges, mesh->hgid));     
    for (j = hindex[local_id]; j < hindex[local_id+1]; j++) {
      edge_procs[pcnt] = mesh->hvertex_proc[j];
      for (k = 0; k < gid; k++) edge_verts[k+pcnt*num_gid_entries] = 0;
      edge_verts[gid+pcnt*num_gid_entries] = mesh->hvertex[j];
      pcnt++;
    }
  }  

End:
    
  STOP_CALLBACK_TIMER;
  return ierr;
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

static void test_point_drops(FILE *, double *, Zoltan_Object &,
  int, int *, int, int *, int, int);
static void test_box_drops(FILE *, double *, double *, Zoltan_Object &,
  int, int, int, int);

static void test_drops(
  int Proc,
  MESH_INFO_PTR mesh, 
  PARIO_INFO_PTR pio_info,
  Zoltan_Object &zz
)
{
FILE *fp;
double xlo[3], xhi[3];
char par_out_fname[FILENAME_MAX+1], ctemp[FILENAME_MAX+1];
int i;
int max_part, gmax_part;
int test_both;  /* If true, test both Zoltan_*_Assign and Zoltan_*_PP_Assign. */
                /* If false, test only Zoltan_*_PP_Assign.                    */
                /* True if # partitions == # processors.                      */

  /* Find maximum partition number across all processors. */
  int Num_Proc = 0;
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
  Zoltan_Object &zz,
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
    status = zz.LB_Point_Assign(x, one_proc);
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

  status = zz.LB_Point_PP_Assign(x, one_proc, one_part);
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
  Zoltan_Object &zz,
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
    status = zz.LB_Box_Assign(xlo[0], xlo[1], xlo[2], 
                                      xhi[0], xhi[1], xhi[2], 
                                      procs, proccnt);
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


  status = zz.LB_Box_PP_Assign(xlo[0], xlo[1], xlo[2], 
                                       xhi[0], xhi[1], xhi[2], 
                                       procs, proccnt, 
                                       parts, partcnt);
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
