/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    $Revision$
 ****************************************************************************/

#include <stdio.h>
#include "lb_const.h"
#include "reftree_const.h"
#include "params_const.h"

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

/* Prototypes for functions internal to this file */

static void LB_Reftree_Free_Subtree(LB_REFTREE *subroot);
static int order_tri_bisect(LB *lb, int *vert1, int *order, int *vertices,
                     int *in_vertex, int *out_vertex, LB_REFTREE *subroot);
static int order_quad_quad(LB *lb, int *vert1, int *order, int *vertices,
                     int *in_vertex, int *out_vertex, LB_REFTREE *subroot);
static int order_other_ref(LB *lb, LB_REFTREE *parent, int num_child, 
                    int *num_vert,
                    int *vert1, int *vertices, int *order, int *in_vertex,
                    int *out_vertex);
static void order_other_ref_recur(int new_entry, int level, int *order, 
                          int *on_path,
                          int num_child, int *has_out, int **share_vert,
                          int max_share, int *solved);
static int find_inout(int level, int num_child, int *num_vert, int *vert1,
               int *vertices, int *in_vertex, int *out_vertex, int *order);
static int LB_Reftree_Reinit_Coarse(LB *lb);
static int LB_Reftree_Build_Recursive(LB *lb,LB_REFTREE *subroot);

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
/*  Parameters structure for reftree methods */
static PARAM_VARS REFTREE_params[] = {
        { "REFTREE_HASH_SIZE", NULL, "INT" },
        { NULL, NULL, NULL } };

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int LB_Set_Reftree_Param(
char *name,                     /* name of variable */
char *val)                      /* value of variable */
{
    int status;
    PARAM_UTYPE result;         /* value returned from Check_Param */
    int index;                  /* index returned from Check_Param */

    status = LB_Check_Param(name, val, REFTREE_params, &result, &index);

    return(status);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int LB_Reftree_Init(LB *lb)

{
/*
 *  Function to initialize a refinement tree.  This creates the root and
 *  the first level of the tree, which corresponds to the initial coarse grid
 */
char *yo = "LB_Reftree_Init";
char msg[256];
struct LB_reftree_data_struct *reftree_data; /* data pointed to by lb */
LB_REFTREE *root;          /* Root of the refinement tree */
struct LB_reftree_hash_node **hashtab; /* hash table */
int nproc;                 /* number of processors */
LB_ID_PTR local_gids;      /* coarse element Global IDs from user */
LB_ID_PTR local_lids;      /* coarse element Local IDs from user */
LB_ID_PTR lid, prev_lid;   /* temporary coarse element Local ID; used to pass
                              NULL to query functions when NUM_LID_ENTRIES=0 */
LB_ID_PTR all_gids;        /* coarse element Global IDs from all procs */
int *assigned;             /* 1 if the element is assigned to this proc */
int *num_vert;             /* number of vertices for each coarse element */
int *vertices;             /* vertices for the coarse elements */
int *in_vertex;            /* "in" vertex for each coarse element */
int *out_vertex;           /* "out" vertex for each coarse element */
int in_order;              /* 1 if user is supplying order of the elements */
int num_obj;               /* number of coarse objects known to this proc */
int *num_obj_all;          /* num_obj from each processor */
int *displs;               /* running sum of num_obj_all */
int sum_num_obj;           /* full sum of num_obj_all */
int total_num_obj;         /* number of objects in the whole coarse grid */
int ierr;                  /* error flag from calls */
int final_ierr;            /* error flag returned by this routine */
int wdim;                  /* dimension of object weights */
int count;                 /* counter for number of objects */
int sum_vert;              /* summation of number of vertices of objects */
int *order;                /* permutation array for ordering coarse elements */
int found;                 /* flag for terminating first/next query loop */
int hashsize;              /* size of the hash table */
int i, j;                  /* loop counters */
int num_gid_entries = lb->Num_GID;  /* number of array entries in a global ID */
int num_lid_entries = lb->Num_LID;  /* number of array entries in a local ID */

  LB_TRACE_ENTER(lb, yo);

  final_ierr = LB_OK;

  if (lb->Obj_Weight_Dim == 0) {
    wdim = 1;
  } else {
    wdim = lb->Obj_Weight_Dim;
  }

  nproc = lb->Num_Proc;

  /*
   * Allocate the root of the refinement tree for this load balancing structure.
   * If a tree already exists, destroy it first.
   */

  if (lb->Data_Structure != NULL) LB_Reftree_Free_Structure(lb);

  root = (LB_REFTREE *) LB_MALLOC(sizeof(LB_REFTREE));
  if (root == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_TRACE_EXIT(lb, yo);
    return(LB_MEMERR);
  }

  /*
   * Initialize the root
   */

  root->global_id = LB_MALLOC_GID(lb);
  root->local_id  = LB_MALLOC_LID(lb);
  if (root->global_id == NULL || (num_lid_entries && root->local_id == NULL)) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(LB_MEMERR);
  }

  root->children       = (LB_REFTREE *) NULL;
  root->num_child      = 0;
  root->num_vertex     = 0;
  root->vertices       = (int *) NULL;
  root->in_vertex      = (int) NULL;
  root->out_vertex     = (int) NULL;
  root->assigned_to_me = 0;
  root->partition      = 0;

  root->weight = (float *) LB_MALLOC(wdim*sizeof(float));
  root->summed_weight = (float *) LB_MALLOC(wdim*sizeof(float));
  root->my_sum_weight = (float *) LB_MALLOC(wdim*sizeof(float));
  if (root->weight == NULL || root->summed_weight == NULL ||
      root->my_sum_weight == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(LB_MEMERR);
  }
  for (i=0; i<wdim; i++) {
    root->weight[i] = 0.0;
    root->summed_weight[i] = 0.0;
    root->my_sum_weight[i] = 0.0;
  }

  /*
   * Allocate and initialize the hash table.
   */

  LB_Bind_Param(REFTREE_params, "REFTREE_HASH_SIZE", (void *) &hashsize);
  hashsize = DEFAULT_HASH_TABLE_SIZE;
  LB_Assign_Param_Vals(lb->Params, REFTREE_params, lb->Debug_Level, lb->Proc,
                       lb->Debug_Proc);

  hashtab = (struct LB_reftree_hash_node **)
            LB_MALLOC(sizeof(struct LB_reftree_hash_node *)*hashsize);
  if (hashtab == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(LB_MEMERR);
  }
  for (i=0; i<hashsize; i++)
    hashtab[i] = (struct LB_reftree_hash_node *)NULL;

  /*
   * set the lb pointer for later access to the refinement tree and hash table
   */

  reftree_data = (struct LB_reftree_data_struct *)
                 LB_MALLOC(sizeof(struct LB_reftree_data_struct));
  reftree_data->reftree_root = root;
  reftree_data->hash_table = hashtab;
  reftree_data->hash_table_size = hashsize;
  lb->Data_Structure = (void *) reftree_data;

  /*
   * Get the list of initial elements known to this processor
   */


  /*
   * Get the number of objects
   */

  if (lb->Get_Num_Coarse_Obj == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Must register LB_NUM_COARSE_OBJ_FN.");
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(LB_FATAL);
  }

  num_obj = lb->Get_Num_Coarse_Obj(lb->Get_Num_Coarse_Obj_Data, &ierr);
  if (ierr) {
    LB_PRINT_ERROR(lb->Proc, yo, 
                   "Error returned from user function Get_Num_Coarse_Obj.");
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(ierr);
  }

  /*
   * Get the objects, if the number is not 0
   */

  if (num_obj > 0) {

    num_obj += 1; /* allocate one extra spot for the last call to NEXT_OBJ */
    local_gids = LB_MALLOC_GID_ARRAY(lb, num_obj);
    local_lids = LB_MALLOC_LID_ARRAY(lb, num_obj);
    assigned   = (int *) LB_MALLOC(num_obj*sizeof(int));
    num_vert   = (int *) LB_MALLOC(num_obj*sizeof(int));
    vertices   = (int *) LB_MALLOC(MAXVERT*num_obj*sizeof(int));
    in_vertex  = (int *) LB_MALLOC(num_obj*sizeof(int));
    out_vertex = (int *) LB_MALLOC(num_obj*sizeof(int));
    num_obj -= 1;

    if (local_gids == NULL || (num_lid_entries > 0 && local_lids == NULL) || 
        assigned   == NULL ||
        num_vert   == NULL || vertices   == NULL || in_vertex == NULL ||
        out_vertex == NULL) {
      LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
      LB_FREE(&local_gids);
      LB_FREE(&local_lids);
      LB_FREE(&assigned);
      LB_FREE(&num_vert);
      LB_FREE(&vertices);
      LB_FREE(&in_vertex);
      LB_FREE(&out_vertex);
      LB_Reftree_Free_Structure(lb);
      LB_TRACE_EXIT(lb, yo);
      return(LB_MEMERR);
    }

    if (lb->Get_Coarse_Obj_List != NULL) {

  /*
   * Get objects via list
   */

      lb->Get_Coarse_Obj_List(lb->Get_Coarse_Obj_List_Data, 
                              num_gid_entries, num_lid_entries,
                              local_gids, local_lids, 
                              assigned, num_vert, vertices,
                              &in_order, in_vertex, out_vertex, &ierr);
      if (ierr) {
        LB_PRINT_ERROR(lb->Proc, yo, 
                      "Error returned from user function Get_Coarse_Obj_List.");
        LB_FREE(&local_gids);
        LB_FREE(&local_lids);
        LB_FREE(&assigned);
        LB_FREE(&num_vert);
        LB_FREE(&vertices);
        LB_FREE(&in_vertex);
        LB_FREE(&out_vertex);
        LB_Reftree_Free_Structure(lb);
        LB_TRACE_EXIT(lb, yo);
        return(ierr);
      }

    }

    else if (lb->Get_First_Coarse_Obj != NULL &&
             lb->Get_Next_Coarse_Obj  != NULL) {

  /*
   * Get objects via first/next
   */

      sum_vert = 0;
      count = 0;
      lid = (num_lid_entries ? &(local_lids[count*num_lid_entries]) : NULL);
      found = lb->Get_First_Coarse_Obj(lb->Get_First_Coarse_Obj_Data,
                                       num_gid_entries, num_lid_entries,
                                       &(local_gids[count*num_gid_entries]), 
                                       lid,
                                       &assigned[count],
                                       &num_vert[count], &vertices[sum_vert],
                                       &in_order,
                                       &in_vertex[count], &out_vertex[count],
                                       &ierr);
      if (ierr) {
        LB_PRINT_ERROR(lb->Proc, yo, 
                     "Error returned from user function Get_First_Coarse_Obj.");
        LB_FREE(&local_gids);
        LB_FREE(&local_lids);
        LB_FREE(&assigned);
        LB_FREE(&num_vert);
        LB_FREE(&vertices);
        LB_FREE(&in_vertex);
        LB_FREE(&out_vertex);
        LB_Reftree_Free_Structure(lb);
        LB_TRACE_EXIT(lb, yo);
        return(ierr);
      }

      while (found && count <= num_obj) {
        sum_vert += num_vert[count];
        count += 1;
        prev_lid = (num_lid_entries ? &(local_lids[(count-1)*num_lid_entries]) 
                                    : NULL);
        lid = (num_lid_entries ? &(local_lids[count*num_lid_entries]) : NULL);
        found = lb->Get_Next_Coarse_Obj(lb->Get_Next_Coarse_Obj_Data,
                                      num_gid_entries, num_lid_entries,
                                      &(local_gids[(count-1)*num_gid_entries]), 
                                      prev_lid,
                                      &(local_gids[count*num_gid_entries]), 
                                      lid,
                                      &assigned[count],
                                      &num_vert[count], &vertices[sum_vert],
                                      &in_vertex[count], &out_vertex[count],
                                      &ierr);
        if (ierr) {
          LB_PRINT_ERROR(lb->Proc, yo, 
                      "Error returned from user function Get_Next_Coarse_Obj.");
          LB_FREE(&local_gids);
          LB_FREE(&local_lids);
          LB_FREE(&assigned);
          LB_FREE(&num_vert);
          LB_FREE(&vertices);
          LB_FREE(&in_vertex);
          LB_FREE(&out_vertex);
          LB_Reftree_Free_Structure(lb);
          LB_TRACE_EXIT(lb, yo);
          return(ierr);
        }
      }
      if (count != num_obj) {
        sprintf(msg, "Number of objects returned by "
                     "First/Next_Coarse_Obj = %d is not equal to the "
                     "number returned by Num_Coarse_Obj = %d\n",
                     count, num_obj);
        LB_PRINT_WARN(lb->Proc, yo, msg);
        final_ierr = LB_WARN;
      }
    }

    else {
      LB_PRINT_ERROR(lb->Proc, yo, "Must define and register either "
                      "LB_COARSE_OBJ_LIST_FN or "
                      "LB_FIRST_COARSE_OBJ_FN/LB_NEXT_COARSE_OBJ_FN pair.");
      LB_FREE(&local_gids);
      LB_FREE(&local_lids);
      LB_FREE(&assigned);
      LB_FREE(&num_vert);
      LB_FREE(&vertices);
      LB_FREE(&in_vertex);
      LB_FREE(&out_vertex);
      LB_Reftree_Free_Structure(lb);
      LB_TRACE_EXIT(lb, yo);
      return(LB_FATAL);
    }
  } /* endif (num_obj > 0) */

  /*
   * Communicate to get coarse grid objects unknown to this processor.
   */

  /*
   * First determine how many coarse objects are on each processor
   */

  num_obj_all = (int *)LB_MALLOC(nproc*sizeof(int));
  displs = (int *)LB_MALLOC(nproc*sizeof(int));
  if (num_obj_all == NULL || displs == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_FREE(&local_gids);
    LB_FREE(&local_lids);
    LB_FREE(&assigned);
    LB_FREE(&num_vert);
    LB_FREE(&vertices);
    LB_FREE(&in_vertex);
    LB_FREE(&out_vertex);
    LB_FREE(&num_obj_all);
    LB_FREE(&displs);
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(LB_MEMERR);
  }

  MPI_Allgather((void *)&num_obj,1,MPI_INT,(void *)num_obj_all,1,MPI_INT,
                lb->Communicator);
  displs[0] = 0;
  for (i=1; i<nproc; i++) displs[i] = displs[i-1]+num_obj_all[i-1];
  sum_num_obj = displs[nproc-1] + num_obj_all[nproc-1];

  /*
   * Then get the coarse objects from all processors
   */

  all_gids = LB_MALLOC_GID_ARRAY(lb, sum_num_obj);
  if (all_gids == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_FREE(&local_gids);
    LB_FREE(&local_lids);
    LB_FREE(&assigned);
    LB_FREE(&num_vert);
    LB_FREE(&vertices);
    LB_FREE(&in_vertex);
    LB_FREE(&out_vertex);
    LB_FREE(&num_obj_all);
    LB_FREE(&displs);
    LB_FREE(&all_gids);
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(LB_MEMERR);
  }

  /* KDDKDD Changed MPI_BYTE to LB_ID_MPI_TYPE  */

  /* Account for number of array entries in an ID. */
  for (i=0; i<nproc; i++) {
    num_obj_all[i] = num_obj_all[i]*num_gid_entries;
    displs[i] = displs[i]*num_gid_entries;
  }

  MPI_Allgatherv((void *)local_gids,num_obj*num_gid_entries,LB_ID_MPI_TYPE,
                 (void *)all_gids,num_obj_all,displs,LB_ID_MPI_TYPE,
                 lb->Communicator);

  LB_FREE(&displs);
  LB_FREE(&num_obj_all);

  /*
   * Finally, build a list with each coarse grid element, beginning with
   * the ones this processor knows.  Also set the default order of the
   * elements as given by the user, with processor rank resolving duplicates
   */

  local_gids = LB_REALLOC_GID_ARRAY(lb, local_gids, sum_num_obj);
  order = (int *) LB_MALLOC(sum_num_obj*sizeof(int));
  if (local_gids == NULL || order == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_FREE(&local_gids);
    LB_FREE(&local_lids);
    LB_FREE(&assigned);
    LB_FREE(&num_vert);
    LB_FREE(&vertices);
    LB_FREE(&in_vertex);
    LB_FREE(&out_vertex);
    LB_FREE(&all_gids);
    LB_FREE(&order);
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(LB_MEMERR);
  }

/* TEMP this is terribly inefficient.  Probably better to sort all_gids to
        identify the duplicates.  Of course, it's not bad if the
        initial grid is really coarse */

  total_num_obj = num_obj;
  count = 0;
  for (i=0; i<sum_num_obj; i++) order[i] = -1;

  for (i=0; i<sum_num_obj; i++) {
    found = 0;
    for (j=0; j<total_num_obj && !found; j++) {
      if (LB_EQ_GID(lb, &(all_gids[i*num_gid_entries]),
                    &(local_gids[j*num_gid_entries]))) 
        found = 1;
    }
    if (found) {
      if (order[j-1] == -1) {
        order[j-1] = count;
        count += 1;
      }
    }
    else {
      LB_SET_GID(lb, &(local_gids[total_num_obj*num_gid_entries]), 
                     &(all_gids[i*num_gid_entries]));
      order[total_num_obj] = count;
      count += 1;
      total_num_obj += 1;
    }
  }

  if (count != total_num_obj) {
    sprintf(msg, "Number of objects counted while "
                 "setting default order = %d is not equal to the "
                 "number counted while getting objects from other procs "
                 "= %d.", count, total_num_obj);
    LB_PRINT_WARN(lb->Proc, yo, msg);
    final_ierr = LB_WARN;
  }

  LB_FREE(&all_gids);

  num_vert = (int *) LB_REALLOC(num_vert,total_num_obj*sizeof(int));
  if (num_vert == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_FREE(&local_gids);
    LB_FREE(&local_lids);
    LB_FREE(&assigned);
    LB_FREE(&num_vert);
    LB_FREE(&vertices);
    LB_FREE(&in_vertex);
    LB_FREE(&out_vertex);
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(LB_MEMERR);
  }

  for (i=num_obj; i<total_num_obj; i++) num_vert[i] = -1;

  /*
   * Determine the order of the coarse grid elements.
   * If the user supplies the order, it was set above.
   */

  if (!in_order) {

  /*
   * TEMP For now, require that the user provide the order.
   */

    LB_PRINT_WARN(lb->Proc, yo, "Currently not supporting automatic "
                    "determination of the order of the coarse grid objects.  "
                    "Using the order in which they were provided.");
    final_ierr = LB_WARN;

  }

  /*
   * Copy the elements into the child list of the root
   */

  /*
   * Allocate the children of the root
   */

  root->children = (LB_REFTREE *) LB_MALLOC(total_num_obj*sizeof(LB_REFTREE));
  if (root->children == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_FREE(&local_gids);
    LB_FREE(&local_lids);
    LB_FREE(&assigned);
    LB_FREE(&num_vert);
    LB_FREE(&vertices);
    LB_FREE(&in_vertex);
    LB_FREE(&out_vertex);
    LB_FREE(&order);
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(LB_MEMERR);
  }
  root->num_child = total_num_obj;

  /*
   * Make sure the weights have been provided, if needed
   */

  if (lb->Obj_Weight_Dim != 0 && lb->Get_Child_Weight == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Must register LB_CHILD_WEIGHT_FN.");
    LB_FREE(&local_gids);
    LB_FREE(&local_lids);
    LB_FREE(&assigned);
    LB_FREE(&num_vert);
    LB_FREE(&vertices);
    LB_FREE(&in_vertex);
    LB_FREE(&out_vertex);
    LB_FREE(&order);
    LB_Reftree_Free_Structure(lb);
    LB_TRACE_EXIT(lb, yo);
    return(LB_FATAL);
  }

  /*
   * For each coarse grid object ...
   */

  sum_vert = 0;
  for (i=0; i<total_num_obj; i++) {

  /*
   * allocate memory within the tree node
   */

    root->children[order[i]].global_id = LB_MALLOC_GID(lb);
    root->children[order[i]].local_id  = LB_MALLOC_LID(lb);
    root->children[order[i]].weight = (float *) LB_MALLOC(wdim*sizeof(float));
    root->children[order[i]].summed_weight = (float *) LB_MALLOC(wdim*sizeof(float));
    root->children[order[i]].my_sum_weight = (float *) LB_MALLOC(wdim*sizeof(float));
    if (num_vert[i] <= 0)
      root->children[order[i]].vertices = (int *) LB_MALLOC(sizeof(int));
    else
      root->children[order[i]].vertices = (int *) LB_MALLOC(num_vert[i]*sizeof(int));
    if (root->children[order[i]].global_id == NULL     ||
        (num_lid_entries && root->children[order[i]].local_id == NULL) ||
        root->children[order[i]].weight        == NULL ||
        root->children[order[i]].summed_weight == NULL ||
        root->children[order[i]].my_sum_weight == NULL ||
        root->children[order[i]].vertices      == NULL) {
      LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
      LB_FREE(&local_gids);
      LB_FREE(&local_lids);
      LB_FREE(&assigned);
      LB_FREE(&num_vert);
      LB_FREE(&vertices);
      LB_FREE(&in_vertex);
      LB_FREE(&out_vertex);
      LB_FREE(&order);
      LB_Reftree_Free_Structure(lb);
      LB_TRACE_EXIT(lb, yo);
      return(LB_MEMERR);
    }

  /*
   * Get the weight
   */

    if (lb->Obj_Weight_Dim == 0) {
  /* if an initial element is a leaf, the weight gets set to 1 later */
       *(root->children[order[i]].weight) = 0.0;
    }
    else if (num_vert[i] == -1) {
  /* if the element is not known to this processor, the weight is 0 */
       *(root->children[order[i]].weight) = 0.0;
    }
    else {
      lid = (num_lid_entries ? &(local_lids[i*num_lid_entries]) : NULL);
      lb->Get_Child_Weight(lb->Get_Child_Weight_Data,
                           num_gid_entries, num_lid_entries,
                           &(local_gids[i*num_gid_entries]),
                           lid, lb->Obj_Weight_Dim, 
                           root->children[order[i]].weight, &ierr);
    }
    for (j=0; j<wdim; j++) {
      root->children[order[i]].summed_weight[j] = 0.0;
      root->children[order[i]].my_sum_weight[j] = 0.0;
    }

  /*
   * Copy the vertices
   */

    for (j=0; j<num_vert[i]; j++) 
      root->children[order[i]].vertices[j] = vertices[sum_vert+j];
    if (num_vert[i] > 0) sum_vert += num_vert[i];

  /*
   * Copy from temporary arrays and set empty defaults
   */

    if (num_vert[i] == -1) {
  /* elements not known to this processor have more empty entries */
      LB_SET_GID(lb, root->children[order[i]].global_id,
                 &(local_gids[i*num_gid_entries]));
      LB_INIT_LID(lb, root->children[order[i]].local_id);
      root->children[order[i]].children       = (LB_REFTREE *) NULL;
      root->children[order[i]].num_child      = 0;
      root->children[order[i]].num_vertex     = num_vert[i];
      root->children[order[i]].in_vertex      = 0;
      root->children[order[i]].out_vertex     = 0;
      root->children[order[i]].assigned_to_me = 0;
      root->children[order[i]].partition      = 0;
    }
    else {
      LB_SET_GID(lb, root->children[order[i]].global_id,
                 &(local_gids[i*num_gid_entries]));
      LB_SET_LID(lb, root->children[order[i]].local_id,
                 &(local_lids[i*num_lid_entries]));
      root->children[order[i]].children       = (LB_REFTREE *) NULL;
      root->children[order[i]].num_child      = 0;
      root->children[order[i]].num_vertex     = num_vert[i];
      root->children[order[i]].in_vertex      = in_vertex[i];
      root->children[order[i]].out_vertex     = out_vertex[i];
      root->children[order[i]].assigned_to_me = assigned[i];
      root->children[order[i]].partition      = 0;
    }

  /*
   * Add it to the hash table
   */

    LB_Reftree_Hash_Insert(lb, &(root->children[order[i]]),hashtab,hashsize);

  }

  /*
   * clean up and return error code
   */

  LB_FREE(&local_gids);
  LB_FREE(&local_lids);
  LB_FREE(&assigned);
  LB_FREE(&num_vert);
  LB_FREE(&vertices);
  LB_FREE(&in_vertex);
  LB_FREE(&out_vertex);
  LB_FREE(&order);
  LB_TRACE_EXIT(lb, yo);
  return(final_ierr);
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int LB_Reftree_Build(LB *lb)

{
/*
 * Function to build a refinement tree
 */
char *yo = "LB_Reftree_Build";
LB_REFTREE *root;          /* Root of the refinement tree */
int ierr;                  /* Error code returned by called functions */
int i;                     /* loop counter */

  /*
   * Initialize the tree, if not already there, and set the root.  If already
   * there, reinitialize coarse grid.
   */

  if (lb->Data_Structure == NULL) {
    ierr = LB_Reftree_Init(lb);
    if (ierr==LB_FATAL || ierr==LB_MEMERR) {
      LB_PRINT_ERROR(lb->Proc, yo, "Error returned from LB_Reftree_Init.");
      return(ierr);
    }
  }
  else {
    LB_Reftree_Reinit_Coarse(lb);
  }
  root = ((struct LB_reftree_data_struct *)lb->Data_Structure)->reftree_root;

  /*
   * Verify the required child query functions are registered
   */

  if (lb->Get_Num_Child == NULL || lb->Get_Child_List == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Must register LB_NUM_CHILD_FN"
            " and LB_CHILD_LIST_FN.");
    LB_Reftree_Free_Structure(lb);
    return(LB_FATAL);
  }

  /*
   * For each child of the root, build the subtree rooted at that child
   * and, if the weights are not provided, set its weight if it is a leaf.
   * Skip elements not known to this processor.
   */

  for (i=0; i<root->num_child; i++) {
    if ( (root->children[i]).num_vertex != -1 ) {
      ierr = LB_Reftree_Build_Recursive(lb,&(root->children[i]));
      if (ierr==LB_FATAL || ierr==LB_MEMERR) {
        LB_PRINT_ERROR(lb->Proc, yo, 
                       "Error returned from LB_Reftree_Build_Recursive.");
        return(ierr);
      }
    }
  }

  return(LB_OK);
}

static int LB_Reftree_Build_Recursive(LB *lb,LB_REFTREE *subroot)

{
/*
 * Recursive function to traverse a tree while building it
 */
static int TEMP_first_warning = 1; /* TEMP until ref_type is fully supported */
char *yo = "LB_Reftree_Build_Recursive";
char msg[256];
int ierr;                  /* error code called routines */
int final_ierr;            /* error code returned by this routine */
int num_obj;               /* number of children returned by user */
LB_ID_PTR local_gids;      /* coarse element Global IDs from user */
LB_ID_PTR local_lids;      /* coarse element Local IDs from user */
LB_ID_PTR lid;             /* temporary coarse element Local ID; used to pass
                              NULL to query functions when NUM_LID_ENTRIES=0 */
int *assigned;             /* 1 if the element is assigned to this proc */
int *num_vert;             /* number of vertices for each coarse element */
int *vertices;             /* vertices for the coarse elements */
int *in_vertex;            /* "in" vertex for each coarse element */
int *out_vertex;           /* "out" vertex for each coarse element */
LB_REF_TYPE ref_type;      /* type of refinement that creates children */
int *order;                /* order of the children */
int wdim;                  /* dimension for weights */
int i, j;                  /* loop counters */
int sum_vert;              /* running sum of the number of vertices */
int *vert1;                /* array containing the first vertex for each child*/
struct LB_reftree_hash_node **hashtab; /* hash tree */
int hashsize;              /* size of the hash table */
int num_gid_entries = lb->Num_GID;  /* number of array entries in a global ID */
int num_lid_entries = lb->Num_LID;  /* number of array entries in a local ID */

  final_ierr = LB_OK;
  if (lb->Obj_Weight_Dim == 0) {
    wdim = 1;
  } else {
    wdim = lb->Obj_Weight_Dim;
  }

  /*
   * Print a warning if a nonexistent subroot is passed in
   */

  if (subroot == NULL) {
    LB_PRINT_WARN(lb->Proc, yo, "Called with nonexistent subroot.");
    return(LB_WARN);
  }

  /*
   * Get the number of children of this node
   */

  num_obj = lb->Get_Num_Child(lb->Get_Num_Child_Data, 
                              num_gid_entries, num_lid_entries,
                              subroot->global_id, subroot->local_id, &ierr);
  if (ierr) {
    LB_PRINT_ERROR(lb->Proc, yo, 
                   "Error returned from user function Get_Num_Child.");
    LB_Reftree_Free_Structure(lb);
    return(ierr);
  }

  /*
   * If there are no children, set the weight if it is not user provided,
   * and return.  The default is to use 1.0 for leaves and 0.0 for others.
   */

  if (num_obj == 0) {
    if (lb->Obj_Weight_Dim == 0) *(subroot->weight) = 1.0;
    return(LB_OK);
  }

  /*
   * Get the children
   */

  local_gids = LB_MALLOC_GID_ARRAY(lb, num_obj);
  local_lids = LB_MALLOC_LID_ARRAY(lb, num_obj);
  assigned   = (int *) LB_MALLOC(num_obj*sizeof(int));
  num_vert   = (int *) LB_MALLOC(num_obj*sizeof(int));
  vertices   = (int *) LB_MALLOC(MAXVERT*num_obj*sizeof(int));
  in_vertex  = (int *) LB_MALLOC(num_obj*sizeof(int));
  out_vertex = (int *) LB_MALLOC(num_obj*sizeof(int));
  vert1      = (int *) LB_MALLOC((num_obj+1)*sizeof(int));
  if (local_gids == NULL || (num_lid_entries > 0 && local_lids == NULL) || 
      assigned   == NULL ||
      num_vert   == NULL || vertices   == NULL || in_vertex == NULL ||
      out_vertex == NULL || vert1      == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_FREE(&local_gids);
    LB_FREE(&local_lids);
    LB_FREE(&assigned);
    LB_FREE(&num_vert);
    LB_FREE(&vertices);
    LB_FREE(&in_vertex);
    LB_FREE(&out_vertex);
    LB_FREE(&vert1);
    LB_Reftree_Free_Structure(lb);
    return(LB_MEMERR);
  }
  lb->Get_Child_List(lb->Get_Child_List_Data, 
                     num_gid_entries, num_lid_entries,
                     subroot->global_id, subroot->local_id, 
                     local_gids, local_lids, assigned,
                     num_vert, vertices, &ref_type, in_vertex, out_vertex,
                     &ierr);
  if (ierr) {
    LB_PRINT_ERROR(lb->Proc, yo, 
                   "Error returned from user function Get_Child_List.");
    LB_FREE(&local_gids);
    LB_FREE(&local_lids);
    LB_FREE(&assigned);
    LB_FREE(&num_vert);
    LB_FREE(&vertices);
    LB_FREE(&in_vertex);
    LB_FREE(&out_vertex);
    LB_FREE(&vert1);
    LB_Reftree_Free_Structure(lb);
    return(ierr);
  }

  /*
   * Set the start of the list of vertices for each child
   */

  vert1[0] = 0;
  for (i=0; i<num_obj; i++) vert1[i+1] = vert1[i] + num_vert[i];

  /*
   * Determine the order of the children
   */

  order = (int *) LB_MALLOC(num_obj*sizeof(int));
  if (order == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_FREE(&local_gids);
    LB_FREE(&local_lids);
    LB_FREE(&assigned);
    LB_FREE(&num_vert);
    LB_FREE(&vertices);
    LB_FREE(&in_vertex);
    LB_FREE(&out_vertex);
    LB_FREE(&vert1);
    LB_Reftree_Free_Structure(lb);
    return(LB_MEMERR);
  }

  /*
   * TEMP until code is supplied to support these refinement types
   */

  switch (ref_type) {
  case LB_HEX3D_OCT:
    if (TEMP_first_warning) {
      LB_PRINT_WARN(lb->Proc, yo, "Currently not supporting "
                      "automatic ordering of elements for refinement type "
                      "LB_HEX3D_OCT.  Using LB_OTHER_REF.");
      TEMP_first_warning = 0;
      final_ierr = LB_WARN;
    }
    ref_type = LB_OTHER_REF;
    break;
  default:
    break;
  }

  /* end TEMP */

  /*
   *  Refinement type dependent determination of the order of the children
   *  and the in/out vertices
   */

  switch (ref_type) {

  case LB_IN_ORDER:
    for (i=0; i<num_obj; i++) order[i] = i;
    break;
  case LB_TRI_BISECT:
    ierr = order_tri_bisect(lb,vert1,order,vertices,in_vertex,out_vertex,
                            subroot);
    break;
  case LB_QUAD_QUAD:
    ierr = order_quad_quad(lb,vert1,order,vertices,in_vertex,out_vertex,
                           subroot);
    break;
  case LB_HEX3D_OCT:
    /* TEMP */
    LB_PRINT_WARN(lb->Proc, yo, "Oops, still got into case for HEX3D_OCT.");
    for (i=0; i<num_obj; i++) order[i] = i;
    break;
  case LB_OTHER_REF:
    ierr = order_other_ref(lb, subroot, num_obj, num_vert, vert1, vertices,
                           order, in_vertex, out_vertex);
    break;

  /*
   * Default case if a bad value gets returned; use them in order.
   */
  default:
    sprintf(msg, "Unknown value returned for ref_type"
            " = %d.  Using children in order provided.",ref_type);
    LB_PRINT_WARN(lb->Proc, yo, msg);
    for (i=0; i<num_obj; i++) order[i] = i;
    final_ierr = LB_WARN;
  }

  /*
   * Copy the children into the child list of the subroot
   */

  /*
   * Allocate the children
   */

  if (subroot->children != NULL) {
    LB_PRINT_WARN(lb->Proc, yo, "children already existed; memory"
                    " leak potential.");
    final_ierr = LB_WARN;
  }

  subroot->children = (LB_REFTREE *) LB_MALLOC(num_obj*sizeof(LB_REFTREE));
  if (subroot->children == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_FREE(&local_gids);
    LB_FREE(&local_lids);
    LB_FREE(&assigned);
    LB_FREE(&num_vert);
    LB_FREE(&vertices);
    LB_FREE(&in_vertex);
    LB_FREE(&out_vertex);
    LB_FREE(&order);
    LB_FREE(&vert1);
    LB_Reftree_Free_Structure(lb);
    return(LB_MEMERR);
  }
  subroot->num_child = num_obj;

  hashtab  = ((struct LB_reftree_data_struct *)lb->Data_Structure)->hash_table;
  hashsize = ((struct LB_reftree_data_struct *)lb->Data_Structure)->hash_table_size;

  /*
   * For each child ...
   */

  sum_vert = 0;
  for (i=0; i<num_obj; i++) {

  /*
   * allocate memory within the child
   */

    subroot->children[order[i]].global_id = LB_MALLOC_GID(lb);
    subroot->children[order[i]].local_id  = LB_MALLOC_LID(lb);
    subroot->children[order[i]].weight = (float *) LB_MALLOC(wdim*sizeof(float));
    subroot->children[order[i]].summed_weight = (float *) LB_MALLOC(wdim*sizeof(float));
    subroot->children[order[i]].my_sum_weight = (float *) LB_MALLOC(wdim*sizeof(float));
    if (num_vert[i] <= 0)
      subroot->children[order[i]].vertices = (int *) LB_MALLOC(sizeof(int));
    else
      subroot->children[order[i]].vertices = (int *) LB_MALLOC(num_vert[i]*sizeof(int));
    if (subroot->children[order[i]].global_id     == NULL ||
        (num_lid_entries && subroot->children[order[i]].local_id == NULL) ||
        subroot->children[order[i]].weight        == NULL ||
        subroot->children[order[i]].summed_weight == NULL ||
        subroot->children[order[i]].my_sum_weight == NULL ||
        subroot->children[order[i]].vertices      == NULL) {
      LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
      LB_FREE(&local_gids);
      LB_FREE(&local_lids);
      LB_FREE(&assigned);
      LB_FREE(&num_vert);
      LB_FREE(&vertices);
      LB_FREE(&in_vertex);
      LB_FREE(&out_vertex);
      LB_FREE(&vert1);
      LB_FREE(&order);
      LB_Reftree_Free_Structure(lb);
      return(LB_MEMERR);
    }

  /*
   * Get the weight
   */

    if (lb->Obj_Weight_Dim == 0) {
       *(subroot->children[order[i]].weight) = 0.0;
    }
    else {
      lid = (num_lid_entries ? &(local_lids[i*num_lid_entries]) : NULL);
      lb->Get_Child_Weight(lb->Get_Child_Weight_Data,
                           num_gid_entries, num_lid_entries,
                           &(local_gids[i*num_gid_entries]),
                           lid, 
                           lb->Obj_Weight_Dim,
                           subroot->children[order[i]].weight, &ierr);
    }
    for (j=0; j<wdim; j++) {
      subroot->children[order[i]].summed_weight[j] = 0.0;
      subroot->children[order[i]].my_sum_weight[j] = 0.0;
    }

  /*
   * Copy the vertices
   */

    for (j=0; j<num_vert[i]; j++)
      subroot->children[order[i]].vertices[j] = vertices[sum_vert+j];
    if (num_vert[i] > 0) sum_vert += num_vert[i];

  /*
   * Copy from temporary arrays and set empty defaults
   */

    LB_SET_GID(lb, subroot->children[order[i]].global_id,
               &(local_gids[i*num_gid_entries]));
    LB_SET_LID(lb, subroot->children[order[i]].local_id,
               &(local_lids[i*num_lid_entries]));
    subroot->children[order[i]].children       = (LB_REFTREE *) NULL;
    subroot->children[order[i]].num_child      = 0;
    subroot->children[order[i]].num_vertex     = num_vert[i];
    subroot->children[order[i]].in_vertex      = in_vertex[i];
    subroot->children[order[i]].out_vertex     = out_vertex[i];
    subroot->children[order[i]].assigned_to_me = assigned[i];
    subroot->children[order[i]].partition      = 0;

  /*
   * Add it to the hash table
   */

    LB_Reftree_Hash_Insert(lb, &(subroot->children[order[i]]),hashtab,hashsize);

  }

  /*
   * clean up
   */

  LB_FREE(&local_gids);
  LB_FREE(&local_lids);
  LB_FREE(&assigned);
  LB_FREE(&num_vert);
  LB_FREE(&vertices);
  LB_FREE(&in_vertex);
  LB_FREE(&out_vertex);
  LB_FREE(&vert1);
  LB_FREE(&order);

  /*
   * recursively do the children
   */

  for (i=0; i<subroot->num_child; i++) {
    ierr = LB_Reftree_Build_Recursive(lb,&(subroot->children[i]));
    if (ierr) final_ierr = ierr;
  }

  return(final_ierr);

}

/*****************************************************************************/

static int order_tri_bisect(LB *lb, int *vert1, int *order, int *vertices,
                     int *in_vertex, int *out_vertex, LB_REFTREE *subroot)
{
/*
 * Function to determine the order of the children and in/out vertices
 * when refinement is done by bisecting triangles.  Determine which of
 * the first and second child has the in_vertex and out_vertex, and find the
 * common vertex to go between them.
 */
char *yo = "order_tri_bisect";
int i, j;                  /* loop indices */
int parents_vert[6];       /* cross index between children and parent */
int parent_in;             /* index of the parent in vertex */
int parent_out;            /* index of the parent out vertex */
int parent_third;          /* index of the parent triangle non in/out vert */
int not_parent[2];         /* index of a vertex the parent does not have */
int has_in[2];             /* flag for an element having the parent in vert */
int has_out[2];            /* flag for an element having the parent out vert */
int has_third[2];          /* flag for a triangle having the parent non in/out*/
int bad_case;              /* flag for failing to identify order */

  /* verify that 3 vertices were given for each triangle; if not, punt */
  if (vert1[1] != 3 || vert1[2] != 6) {
    LB_PRINT_WARN(lb->Proc, yo, "Incorrect number of vertices "
                                "given for bisected triangles.");
    order[0] = 0;
    order[1] = 1;
    in_vertex[0] = vertices[vert1[0]];
    out_vertex[0] = vertices[vert1[0]+1];
    in_vertex[1] = vertices[vert1[1]];
    out_vertex[1] = vertices[vert1[1]+1];
    return(LB_WARN);
  }

  /* determine the relationship between the parent's vertices and
     the children's vertices */
  for (i=0; i<6; i++) {
    parents_vert[i] = -1;
    for (j=0; j<3; j++) {
      if (vertices[i] == subroot->vertices[j]) parents_vert[i] = j;
    }
  }

  /* determine the location of the parents in and out vertices */
  parent_in = -1; parent_out = -1; parent_third = -1;
  for (i=0; i<3; i++) {
    if (subroot->vertices[i] == subroot->in_vertex) {
      parent_in = i;
    }
    else if (subroot->vertices[i] == subroot->out_vertex) {
      parent_out = i;
    }
    else {
    parent_third = i;
    }
  }
  if (parent_in == -1 || parent_out == -1 || parent_third == -1) {
    /* failed to locate one of them */
    LB_PRINT_WARN(lb->Proc, yo, "Could not locate in and out "
                                "vertices in the parent.");
    order[0] = 0;
    order[1] = 1;
    in_vertex[0] = vertices[vert1[0]];
    out_vertex[0] = vertices[vert1[0]+1];
    in_vertex[1] = vertices[vert1[1]];
    out_vertex[1] = vertices[vert1[1]+1];
    return(LB_WARN);
  }

  /* find the vertex that the parent doesn't have */
  if (parents_vert[0] == -1) not_parent[0] = 0;
  if (parents_vert[1] == -1) not_parent[0] = 1;
  if (parents_vert[2] == -1) not_parent[0] = 2;
  if (parents_vert[3] == -1) not_parent[1] = 3;
  if (parents_vert[4] == -1) not_parent[1] = 4;
  if (parents_vert[5] == -1) not_parent[1] = 5;

  /* see which children have which special vertices */
  if (parents_vert[0] == parent_in || parents_vert[1] == parent_in ||
      parents_vert[2] == parent_in) has_in[0] = 1;
  else has_in[0] = 0;
  if (parents_vert[0] == parent_out || parents_vert[1] == parent_out ||
      parents_vert[2] == parent_out) has_out[0] = 1;
  else has_out[0] = 0;
  if (parents_vert[0] == parent_third || parents_vert[1] == parent_third ||
      parents_vert[2] == parent_third) has_third[0] = 1;
  else has_third[0] = 0;
  if (parents_vert[3] == parent_in || parents_vert[4] == parent_in ||
      parents_vert[5] == parent_in) has_in[1] = 1;
  else has_in[1] = 0;
  if (parents_vert[3] == parent_out || parents_vert[4] == parent_out ||
      parents_vert[5] == parent_out) has_out[1] = 1;
  else has_out[1] = 0;
  if (parents_vert[3] == parent_third || parents_vert[4] == parent_third ||
      parents_vert[5] == parent_third) has_third[1] = 1;
  else has_third[1] = 0;

  /* look for the case for this refinement */
  bad_case = 0;
  if (has_in[0]) {
    if (has_out[1]) {
      order[0] = 0; order[1] = 1;
      in_vertex[0] = subroot->vertices[parent_in];
      out_vertex[1] = subroot->vertices[parent_out];
      if (has_third[0] && has_third[1]) {
        out_vertex[0] = subroot->vertices[parent_third];
        in_vertex[1] = subroot->vertices[parent_third];
      }else{
        out_vertex[0] = vertices[not_parent[0]];
        in_vertex[1] = vertices[not_parent[1]];
      }
    }
    else if (has_in[1]) {
      if (has_out[0]) {
        order[0] = 1; order[1] = 0;
        in_vertex[1] = subroot->vertices[parent_in];
        out_vertex[0] = subroot->vertices[parent_out];
        if (has_third[0] && has_third[1]) {
          out_vertex[1] = subroot->vertices[parent_third];
          in_vertex[0] = subroot->vertices[parent_third];
        }else{
          out_vertex[1] = vertices[not_parent[1]];
          in_vertex[0] = vertices[not_parent[0]];
        }
      }else{ /* impossible case, no one has the out vertex */
        bad_case = 1;
        order[0] = 0; order[1] = 1;
        in_vertex[0] = subroot->vertices[parent_in];
        out_vertex[0] = subroot->vertices[parent_third];
        in_vertex[1] = subroot->vertices[parent_third];
        out_vertex[1] = subroot->vertices[parent_in];
      }
    }else{ /* impossible case, second child has neither in nor out */
      bad_case = 1;
      order[0] = 0; order[1] = 1;
      in_vertex[0] = subroot->vertices[parent_in];
      out_vertex[0] = subroot->vertices[parent_third];
      in_vertex[1] = vertices[3];
      out_vertex[1] = vertices[4];
    }
  }
  else if (has_out[0]) {
    if (has_in[1]) {
      order[0] = 1; order[1] = 0;
      in_vertex[1] = subroot->vertices[parent_in];
      out_vertex[0] = subroot->vertices[parent_out];
      if (has_third[0] && has_third[1]) {
        out_vertex[1] = subroot->vertices[parent_third];
        in_vertex[0] = subroot->vertices[parent_third];
      }else{
        out_vertex[1] = vertices[not_parent[1]];
        in_vertex[0] = vertices[not_parent[0]];
      }
    }else{ /* impossible case, no one has the in vertex */
      bad_case = 1;
      order[0] = 0; order[1] = 1;
      in_vertex[0] = subroot->vertices[parent_out];
      out_vertex[0] = subroot->vertices[parent_third];
      in_vertex[1] = subroot->vertices[parent_third];
      out_vertex[1] = subroot->vertices[parent_out];
    }
  }else{ /* impossible case, first child has neither in nor out */
    bad_case = 1;
    order[0] = 0; order[1] = 1;
    in_vertex[0] = vertices[0];
    out_vertex[0] = vertices[1];
    in_vertex[1] = vertices[3];
    out_vertex[1] = vertices[4];
  }
  if (bad_case) {
    LB_PRINT_WARN(lb->Proc, yo, "Vertices of children did not "
                    "match the in and out vertices of parent.");
    return(LB_WARN);
  }
  else {
    return(LB_OK);
  }
}

/*****************************************************************************/

static int order_quad_quad(LB *lb, int *vert1, int *order, int *vertices,
                     int *in_vertex, int *out_vertex, LB_REFTREE *subroot)
{
/*
 * Function to determine the order of the children and in/out vertices
 * when refinement is done by quadrasecting quadrilaterals.
 */

int i,j,k,found,shared[3],ord[4];
char *yo = "order_quad_quad";

  /* verify that 4 vertices were given for each quadrilateral; if not, punt */
  if (vert1[1] != 4 || vert1[2] != 8 || vert1[3] != 12) {
    LB_PRINT_WARN(lb->Proc, yo, "Incorrect number of vertices "
                                "given for quadrasected quadrilaterals.");
    for (i=0; i<4; i++) {
      order[i] = i;
      in_vertex[i] = vertices[vert1[i]];
      out_vertex[i] = vertices[vert1[i]+3];
    }
    return(LB_WARN);
  }

  /* find the child that contains the in_vertex and make it first */

  found = 0;
  for (i=0; i<4 && !found; i++) {
    for (j=0; j<4 && !found; j++) {
      if (vertices[4*i+j] == subroot->in_vertex) {
         ord[0] = i;
         found = 1;
      }
    }
  }
  if (!found) {
    LB_PRINT_WARN(lb->Proc, yo, "Couldn't find in_vertex in children");
    for (i=0; i<4; i++) {
      order[i] = i;
      in_vertex[i] = vertices[vert1[i]];
      out_vertex[i] = vertices[vert1[i]+3];
    }
    return(LB_WARN);
  }

  /* find the child that contains the out_vertex and make it last */

  found = 0;
  for (i=0; i<4 && !found; i++) {
    for (j=0; j<4 && !found; j++) {
      if (vertices[4*i+j] == subroot->out_vertex) {
         ord[3] = i;
         found = 1;
      }
    }
    if (found) exit;
  }
  if (!found) {
    LB_PRINT_WARN(lb->Proc, yo, "Couldn't find out_vertex in children");
    for (i=0; i<4; i++) {
      order[i] = i;
      in_vertex[i] = vertices[vert1[i]];
      out_vertex[i] = vertices[vert1[i]+3];
    }
    return(LB_WARN);
  }

  /* find a child that shares two vertices with the first child and is
     not the last child, and make it second */

  found = 0;
  for (k=0; k<4 && found!=2; k++) {
    if (k != ord[0] && k != ord[3]) {
      found = 0;
      for (j=0; j<4 && found!=2; j++) {
        for (i=0; i<4 && found!=2; i++) {
          if (vertices[4*k+j] == vertices[4*ord[0]+i]) {
            shared[found] = vertices[4*k+j];
            found = found + 1;
          }
        }
      }
    }
    if (found == 2) {
      ord[1] = k;
    }
  }
  if (found != 2) {
    LB_PRINT_WARN(lb->Proc, yo, "Couldn't find second child of quadrasection");
    for (i=0; i<4; i++) {
      order[i] = i;
      in_vertex[i] = vertices[vert1[i]];
      out_vertex[i] = vertices[vert1[i]+3];
    }
    return(LB_WARN);
  }

  /* the remaining child is given by six minus the sum of the others */

  ord[2] = 6 - ord[0] - ord[1] - ord[3];

  /* determine which vertex shared by the first and second children is also
     shared by the last child, and hence is the middle of the parent, and
     place that one in shared[0] */

  found = 0;
  for (j=0; j<4 && !found; j++) {
    if (shared[0] == vertices[4*ord[3]+j]) {
      found = 1;
    }
    if (shared[1] == vertices[4*ord[3]+j]) {
      shared[1] = shared[0];
      shared[0] = vertices[4*ord[3]+j];
      found = 1;
    }
  }
  if (!found) {
    LB_PRINT_WARN(lb->Proc, yo, "Couldn't find central node of quadrasection");
    for (i=0; i<4; i++) {
      order[i] = i;
      in_vertex[i] = vertices[vert1[i]];
      out_vertex[i] = vertices[vert1[i]+3];
    }
    return(LB_WARN);
  }

  /* find the other vertex shared by the third and fourth children */

  found = 0;
  for (j=0; j<4 && !found; j++) {
    if (vertices[4*ord[2]+j] != shared[0]) {
      for (i=0; i<4 && !found; i++) {
        if (vertices[4*ord[2]+j] == vertices[4*ord[3]+i]) {
          shared[2] = vertices[4*ord[2]+j];
          found = 1;
        }
      }
    }
  }
  if (!found) {
    LB_PRINT_WARN(lb->Proc, yo, "Couldn't find shared vertex of 3rd and 4th child");
    for (i=0; i<4; i++) {
      order[i] = i;
      in_vertex[i] = vertices[vert1[i]];
      out_vertex[i] = vertices[vert1[i]+3];
    }
    return(LB_WARN);
  }

  /* invert the permutation matrix */

  for (i=0; i<4; i++) {
    order[ord[i]] = i;
  }

  /* set the in/out vertices */

   in_vertex[ord[0]] = subroot->in_vertex;
  out_vertex[ord[0]] = shared[1];
   in_vertex[ord[1]] = shared[1];
  out_vertex[ord[1]] = shared[0];
   in_vertex[ord[2]] = shared[0];
  out_vertex[ord[2]] = shared[2];
   in_vertex[ord[3]] = shared[2];
  out_vertex[ord[3]] = subroot->out_vertex;

  return(LB_OK);
}

/*****************************************************************************/

static int order_other_ref(LB *lb, LB_REFTREE *parent, int num_child, 
                    int *num_vert,
                    int *vert1, int *vertices, int *order, int *in_vertex,
                    int *out_vertex)
{
/*
 * Function to determine the order of the children for an undetermined
 * refinement scheme.  This is expensive as it performs a tree search
 * to solve this NP hard problem, but it should work for any refinement.
 */

char *yo = "order_other_ref";
int i, j, vi, vj;   /* loop counters */
int *has_in;        /* flag for children having in vertex */
int *has_out;       /* flag for children having out vertex */
int **share_vert;   /* number of vertices shared by two elements */
int max_share;      /* maximum number of vertices shared by two elements */
int solved;         /* flag for having found the solution */
int final_ierr;     /* error code returned */
int *on_path;       /* flag for already placed element on path */

  final_ierr = LB_OK;

  /*
   * Determine which elements contain the in and out vertices of the parent
   */

  has_in = (int *) LB_MALLOC(num_child*sizeof(int));
  has_out = (int *) LB_MALLOC(num_child*sizeof(int));
  if (has_in == NULL || has_out == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_FREE(&has_in);
    LB_FREE(&has_out);
    return(LB_MEMERR);
  }

  for (i=0; i<num_child; i++) {
    has_in[i] = 0;
    has_out[i] = 0;
    for (j=0; j<num_vert[i] && !has_in[i]; j++)
      if (vertices[vert1[i]+j] == parent->in_vertex) has_in[i] = 1;
    for (j=0; j<num_vert[i] && !has_out[i]; j++)
      if (vertices[vert1[i]+j] == parent->out_vertex) has_out[i] = 1;
  }

  /*
   * Determine which elements share vertices other than the in/out vertices
   */

  share_vert = (int **) LB_MALLOC(num_child*sizeof(int *));
  if (share_vert == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    LB_FREE(&share_vert);
    LB_FREE(&has_in);
    LB_FREE(&has_out);
    return(LB_MEMERR);
  }
  for (i=0; i<num_child; i++) {
    share_vert[i] = (int *) LB_MALLOC(num_child*sizeof(int));
    if (share_vert[i] == NULL) {
      LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
      for (j=0; j<=i; j++) LB_FREE(&(share_vert[j]));
      LB_FREE(&share_vert);
      LB_FREE(&has_in);
      LB_FREE(&has_out);
      return(LB_MEMERR);
    }
  }

  max_share = 0;
  for (i=0; i<num_child; i++) {
    share_vert[i][i] = 1;
    for (j=i+1; j<num_child; j++) {
      share_vert[i][j] = 0;
      share_vert[j][i] = 0;
      for (vi=0; vi<num_vert[i]; vi++) {
        for (vj=0; vj<num_vert[j]; vj++) {
          if (vertices[vert1[i]+vi] == vertices[vert1[j]+vj]) {
            if (vertices[vert1[i]+vi] != parent->in_vertex &&
                vertices[vert1[i]+vi] != parent->out_vertex) {
              share_vert[i][j] = share_vert[i][j] + 1;
              share_vert[j][i] = share_vert[i][j];
            }
          }
        }
      }
      if (share_vert[i][j] > max_share) {
        max_share = share_vert[i][j];
      }
    }
  }

  /*
   * Perform tree search to find solution
   */

  solved = 0;
  on_path = (int *) LB_MALLOC(num_child*sizeof(int));
  if (on_path == NULL) {
    LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
    for (j=0; j<=i; j++) LB_FREE(&(share_vert[j]));
    LB_FREE(&on_path);
    LB_FREE(&share_vert);
    LB_FREE(&has_in);
    LB_FREE(&has_out);
    return(LB_MEMERR);
  }
  for (i=0; i<num_child; i++) on_path[i]=0;

  /*
   * Try each element with the in vertex to start the path
   */

  for (i=0; i<num_child && !solved; i++) {
    if (has_in[i]) {
      order_other_ref_recur(i,0,order,on_path,num_child,
                            has_out,share_vert,max_share,&solved);
    }
  }

  /*
   * This should have found a solution, but if not then use given order
   */

  if (!solved) {
    LB_PRINT_WARN(lb->Proc, yo, "Couldn't find path through children."
                                "  Using given order.");
    for (i=0; i<num_child; i++) order[i] = i;
    final_ierr = LB_WARN;
  }

  LB_FREE(&on_path);
  LB_FREE(&share_vert);
  LB_FREE(&has_out);

  /*
   * Finally, determine the in and out vertices of each child
   */

  in_vertex[order[0]] = parent->in_vertex;
  out_vertex[order[num_child-1]] = parent->out_vertex;
  solved = find_inout(0, num_child, num_vert, vert1, vertices, in_vertex,
                      out_vertex, order);
  if (!solved) {
    LB_PRINT_WARN(lb->Proc, yo, "Couldn't find good set of in/out"
                    " vertices.  Using first and second.\n");
    for (i=0; i<num_child; i++) {
      in_vertex[i]  = vertices[vert1[i]];
      out_vertex[i] = vertices[vert1[i]+1];
    }
    final_ierr = LB_WARN;
  }

  /*
   * Invert the permutation matrix (order) to agree with it's usage in
   * LB_Reftree_Build_Recursive, using has_in as workspace
   */

  for (i=0; i<num_child; i++) {
    has_in[order[i]] = i;
  }
  for (i=0; i<num_child; i++) {
    order[i] = has_in[i];
  }
  LB_FREE(&has_in);

  return(final_ierr);
}

/*****************************************************************************/

static void order_other_ref_recur(int new_entry, int level, int *order, 
                          int *on_path,
                          int num_child, int *has_out, int **share_vert,
                          int max_share, int *solved)
{
/*
 * Recursive routine to search the solution space tree
 */
int i, nshare;

  if (level == num_child-1) {

  /*
   * End of a path, success if this element has the out vertex
   */
    if (has_out[new_entry]) {
      order[level] = new_entry;
      *solved = 1;
    }
    else {
      *solved = 0;
    }
  }

  else {

  /*
   * Add this element to the proposed path
   */

    order[level] = new_entry;
    on_path[new_entry] = 1;

  /*
   * Try each element that is not already on the path and shares a vertex
   * with the current new entry, starting first with those that share the
   * most vertices to give a preference to going through faces
   */

    for (nshare = max_share; nshare>0 && !(*solved); nshare--) {
      for (i=0; i<num_child && !(*solved); i++) {
        if (!on_path[i] && share_vert[new_entry][i]==nshare) {
          order_other_ref_recur(i, level+1, order, on_path, num_child, has_out,
                                share_vert, max_share, solved);
        }
      }
    }

    on_path[new_entry] = 0;
  }
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

static int find_inout(int level, int num_child, int *num_vert, int *vert1,
               int *vertices, int *in_vertex, int *out_vertex, int *order)
{
/*
 * Function to find in and out vertices.
 * On first call, the first in_vertex and last out_vertex should already be set.
 * level should be 0 in the first call.
 */
int i, j;                       /* loop counters */
int solved;                     /* found a solution */

  if (level == num_child-1) {

  /*
   * Last element.  Success if the in vertex is not the last out
   */

    if (in_vertex[order[level]] == out_vertex[order[level]])
      solved = 0;
    else
      solved = 1;

  }
  else {

  /*
   * Not last element.
   * Try each vertex that is not the in vertex, and if the next element in
   * the path shares that vertex, move on to the next element
   */

    solved = 0;
    for (i=0; i<num_vert[order[level]] && !solved; i++) {
      if (vertices[vert1[order[level]]+i] != in_vertex[order[level]]) {
        for (j=0; j<num_vert[order[level+1]] && !solved; j++) {
          if (vertices[vert1[order[level+1]]+j] == vertices[vert1[order[level]]+i]) {
            out_vertex[order[level]]  = vertices[vert1[order[level]]+i];
            in_vertex[order[level+1]] = vertices[vert1[order[level]]+i];
            solved = find_inout(level+1, num_child, num_vert, vert1, vertices,
                                in_vertex, out_vertex, order);
          }
        }
      }
    }

  }

  return(solved);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void LB_Reftree_Free_Structure(LB *lb)

{
/*
 *  Function to free all the memory of a refinement tree
 */
struct LB_reftree_data_struct *reftree_data; /* data structure from lb */
LB_REFTREE *root;                            /* Root of the refinement tree */
struct LB_reftree_hash_node **hashtab;       /* hash table */
int hashsize;                                /* dimension of hash table */
int i;                                       /* loop counter */

  reftree_data = (struct LB_reftree_data_struct *)lb->Data_Structure;

  root = reftree_data->reftree_root;

  if (root != NULL) {

  /*
   * Eliminate all the children, recursively
   */

    if (root->children != NULL) {
      for (i=0; i<root->num_child; i++)
        LB_Reftree_Free_Subtree(&(root->children[i]));

      LB_FREE(&(root->children));
    }

  /*
   * Free the memory in root, and root itself
   */

    LB_FREE(&(root->global_id));
    LB_FREE(&(root->local_id));
    LB_FREE(&(root->weight));
    LB_FREE(&(root->summed_weight));
    LB_FREE(&(root->my_sum_weight));
    LB_FREE(&root);
  }

  /*
   * Free the memory in the hash table
   */

  hashtab  = reftree_data->hash_table;
  hashsize = reftree_data->hash_table_size;

  if (hashtab != NULL) {
    LB_Reftree_Clear_Hash_Table(hashtab,hashsize);
    LB_FREE(&hashtab);
  }

  LB_FREE(&(lb->Data_Structure));

}

static void LB_Reftree_Free_Subtree(LB_REFTREE *subroot)

{
/*
 *  Function to free the memory of a subtree
 */
int i;   /* loop counter */

  if (subroot != NULL) {

  /*
   * Eliminate all the children, recursively
   */

    if (subroot->children != NULL) {
      for (i=0; i<subroot->num_child; i++) 
        LB_Reftree_Free_Subtree(&(subroot->children[i]));
      
      LB_FREE(&(subroot->children));
    }

  /*
   * Free the memory in subroot
   */

    LB_FREE(&(subroot->global_id));
    LB_FREE(&(subroot->local_id));
    LB_FREE(&(subroot->weight));
    LB_FREE(&(subroot->summed_weight));
    LB_FREE(&(subroot->my_sum_weight));
    LB_FREE(&(subroot->vertices));
  }
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void LB_Reftree_Reinitialize(LB *lb)

{
/*
 *  Function to set a refinement tree back to the initial grid (1 tree level)
 */
struct LB_reftree_data_struct *reftree_data; /* data structure from lb */
LB_REFTREE *root;                            /* Root of the refinement tree */
struct LB_reftree_hash_node **hashtab;       /* hash table */
int hashsize;                                /* dimension of hash table */
LB_REFTREE *child;                           /* A child of the root */
int i, j;                                    /* loop counters */


  reftree_data = (struct LB_reftree_data_struct *)lb->Data_Structure;

  root = reftree_data->reftree_root;
  hashtab  = reftree_data->hash_table;
  hashsize = reftree_data->hash_table_size;

  if (root != NULL) {

  /*
   * Clear out the hash table
   */

    LB_Reftree_Clear_Hash_Table(hashtab,hashsize);

  /*
   * Go through the children of the root (initial elements)
   */

    if (root->children != NULL) {
      for (i=0; i<root->num_child; i++) {
        child = &(root->children[i]);

  /*
   * Eliminate each child of this object, and update this object, and
   * add it back to the hash table
   */
  
        if (child->children != NULL) {
          for (j=0; j<child->num_child; j++)
            LB_Reftree_Free_Subtree(&(child->children[j]));

          LB_FREE(&(child->children));
          child->children = (LB_REFTREE *)NULL;
        }
        child->num_child = 0;
        LB_Reftree_Hash_Insert(lb, child,hashtab,hashsize);
      }
    }
  }
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

static int LB_Reftree_Reinit_Coarse(LB *lb)

{
/*
 *  Function to reestablish which coarse grid elements are known to this proc
 */
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

char *yo = "LB_Reftree_Reinit_Coarse";
LB_REFTREE *root;     /* Root of the refinement tree */
struct LB_reftree_hash_node **hashtab; /* hash table */
int hashsize;         /* dimension of hash table */
int i, j;             /* loop counter */
LB_ID_PTR local_gids; /* coarse element Global IDs from user */
LB_ID_PTR local_lids; /* coarse element Local IDs from user */
LB_ID_PTR lid;        /* temporary coarse element Local ID; used to pass
                         NULL to query functions when NUM_LID_ENTRIES=0 */
int *assigned;        /* 1 if the element is assigned to this proc */
int *num_vert;        /* number of vertices for each coarse element */
int *vertices;        /* vertices for the coarse elements */
int *in_vertex;       /* "in" vertex for each coarse element */
int *out_vertex;      /* "out" vertex for each coarse element */
LB_ID_PTR slocal_gids;/* coarse element Global IDs from user */
LB_ID_PTR slocal_lids;/* coarse element Local IDs from user */
LB_ID_PTR plocal_gids;/* previous coarse element Global IDs from user */
LB_ID_PTR plocal_lids;/* previous coarse element Local IDs from user */
int sassigned;        /* 1 if the element is assigned to this proc */
int snum_vert;        /* number of vertices for a coarse element */
int sin_vertex;       /* "in" vertex for a coarse element */
int sout_vertex;      /* "out" vertex for a coarse element */
int in_order;         /* 1 if user is supplying order of the elements */
int num_obj;          /* number of coarse objects known to this proc */
int ierr;             /* error flag */
LB_REFTREE *tree_node;/* pointer to an initial grid element in the tree */
int final_ierr;       /* error code returned */
int sum_vert;         /* running total of number of vertices */
int found;            /* flag for another coarse grid element */
int num_gid_entries = lb->Num_GID;  /* number of array entries in a global ID */
int num_lid_entries = lb->Num_LID;  /* number of array entries in a local ID */

  root = ((struct LB_reftree_data_struct *)lb->Data_Structure)->reftree_root;
  hashtab  = ((struct LB_reftree_data_struct *)lb->Data_Structure)->hash_table;
  hashsize = ((struct LB_reftree_data_struct *)lb->Data_Structure)->hash_table_size;
  final_ierr = LB_OK;

  /*
   * Mark all coarse elements as unknown
   */

  for (i=0; i<root->num_child; i++) {
    ((root->children)[i]).num_vertex = -1;
  }

  /*
   * Get the coarse grid objects and update the vertices, whether the element
   * is assigned to this processor, weight and, if not already set, in/out vert
   */

  if (lb->Get_Coarse_Obj_List != NULL) {

  /*
   * Get objects via list
   */

    num_obj = lb->Get_Num_Coarse_Obj(lb->Get_Num_Coarse_Obj_Data, &ierr);
    if (ierr) {
      LB_PRINT_ERROR(lb->Proc, yo, 
                     "Error returned from user function Get_Num_Coarse_Obj.");
      return(ierr);
    }

    if (num_obj > 0) {
      local_gids = LB_MALLOC_GID_ARRAY(lb, num_obj);
      local_lids = LB_MALLOC_LID_ARRAY(lb, num_obj);
      assigned   = (int *) LB_MALLOC(num_obj*sizeof(int));
      num_vert   = (int *) LB_MALLOC(num_obj*sizeof(int));
      vertices   = (int *) LB_MALLOC(MAXVERT*num_obj*sizeof(int));
      in_vertex  = (int *) LB_MALLOC(num_obj*sizeof(int));
      out_vertex = (int *) LB_MALLOC(num_obj*sizeof(int));

      if (local_gids == NULL || (num_lid_entries > 0 && local_lids == NULL) ||
          assigned   == NULL ||
          num_vert   == NULL || vertices   == NULL || in_vertex == NULL ||
          out_vertex == NULL) {
        LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
        LB_FREE(&local_gids);
        LB_FREE(&local_lids);
        LB_FREE(&assigned);
        LB_FREE(&num_vert);
        LB_FREE(&vertices);
        LB_FREE(&in_vertex);
        LB_FREE(&out_vertex);
        return(LB_MEMERR);
      }

      lb->Get_Coarse_Obj_List(lb->Get_Coarse_Obj_List_Data, 
                              num_gid_entries, num_lid_entries,
                              local_gids, local_lids, 
                              assigned, num_vert, vertices,
                              &in_order, in_vertex, out_vertex, &ierr);
      if (ierr) {
        LB_PRINT_ERROR(lb->Proc, yo, 
                      "Error returned from user function Get_Coarse_Obj_List.");
        LB_FREE(&local_gids);
        LB_FREE(&local_lids);
        LB_FREE(&assigned);
        LB_FREE(&num_vert);
        LB_FREE(&vertices);
        LB_FREE(&in_vertex);
        LB_FREE(&out_vertex);
        return(ierr);
      }

      sum_vert = 0;
      for (i=0; i<num_obj; i++) {

        tree_node = LB_Reftree_hash_lookup(lb, hashtab,
                                           &(local_gids[i*num_gid_entries]),
                                           hashsize);
        if (tree_node == NULL) {
          LB_PRINT_WARN(lb->Proc, yo, "coarse grid element not"
                                      " previously seen.");
          final_ierr = LB_WARN;
        }
        else {
          tree_node->num_vertex = num_vert[i];
          LB_FREE(&(tree_node->vertices));
          if (num_vert[i] <= 0)
            tree_node->vertices = (int *) LB_MALLOC(sizeof(int));
          else
            tree_node->vertices = (int *) LB_MALLOC(num_vert[i]*sizeof(int));
          if (tree_node->vertices == NULL) {
            LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
            LB_FREE(&local_gids);
            LB_FREE(&local_lids);
            LB_FREE(&assigned);
            LB_FREE(&num_vert);
            LB_FREE(&vertices);
            LB_FREE(&in_vertex);
            LB_FREE(&out_vertex);
            return(LB_MEMERR);
          }

          for (j=0; j<num_vert[i]; j++)
            tree_node->vertices[j] = vertices[sum_vert+j];
          if (num_vert[i] > 0) sum_vert += num_vert[i];

          tree_node->assigned_to_me = assigned[i];
/* TEMP if not provided in_order, then in/out are not returned and must be
        determined */
          if (tree_node->in_vertex == 0) tree_node->in_vertex = in_vertex[i];
          if (tree_node->out_vertex == 0) tree_node->out_vertex = out_vertex[i];
          lid = (num_lid_entries ? &(local_lids[i*num_lid_entries]) : NULL);
          lb->Get_Child_Weight(lb->Get_Child_Weight_Data, 
                               num_gid_entries, num_lid_entries,
                               &(local_gids[i*num_gid_entries]),
                               lid,
                               lb->Obj_Weight_Dim,
                               tree_node->weight, &ierr);
        }
      }
      LB_FREE(&local_gids);
      LB_FREE(&local_lids);
      LB_FREE(&assigned);
      LB_FREE(&num_vert);
      LB_FREE(&vertices);
      LB_FREE(&in_vertex);
      LB_FREE(&out_vertex);
    }

  }
  else {

  /*
   * Get objects via first/next
   */

    slocal_gids = LB_MALLOC_GID(lb);
    slocal_lids = LB_MALLOC_LID(lb);
    plocal_gids = LB_MALLOC_GID(lb);
    plocal_lids = LB_MALLOC_LID(lb);
    vertices = (int *) LB_MALLOC(MAXVERT*sizeof(int));
    if (slocal_gids == NULL || (num_lid_entries > 0 && slocal_lids == NULL) || 
        plocal_gids == NULL || (num_lid_entries > 0 && plocal_lids == NULL) || 
        vertices == NULL) {
      LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
      LB_FREE(&slocal_gids);
      LB_FREE(&slocal_lids);
      LB_FREE(&vertices);
      return(LB_MEMERR);
    }

    found = lb->Get_First_Coarse_Obj(lb->Get_First_Coarse_Obj_Data,
                                     num_gid_entries, num_lid_entries,
                                     slocal_gids, slocal_lids, &sassigned,
                                     &snum_vert, vertices, &in_order,
                                     &sin_vertex, &sout_vertex, &ierr);
    if (ierr) {
      LB_PRINT_ERROR(lb->Proc, yo, 
                     "Error returned from user function Get_First_Coarse_Obj.");
      LB_FREE(&slocal_gids);
      LB_FREE(&slocal_lids);
      LB_FREE(&vertices);
      return(ierr);
    }
    while (found) {
      tree_node = LB_Reftree_hash_lookup(lb, hashtab,slocal_gids,hashsize);
      if (tree_node == NULL) {
        LB_PRINT_WARN(lb->Proc, yo, "coarse grid element not"
                                    " previously seen.");
        final_ierr = LB_WARN;
      }
      else {
        tree_node->num_vertex = snum_vert;
        LB_FREE(&(tree_node->vertices));
        if (snum_vert <= 0)
          tree_node->vertices = (int *) LB_MALLOC(sizeof(int));
        else
          tree_node->vertices = (int *) LB_MALLOC(snum_vert*sizeof(int));
        if (tree_node->vertices == NULL) {
          LB_PRINT_ERROR(lb->Proc, yo, "Insufficient memory.");
          return(LB_MEMERR);
        }
  
        for (j=0; j<snum_vert; j++)
          tree_node->vertices[j] = vertices[j];
  
        tree_node->assigned_to_me = sassigned;
/* TEMP if not provided in_order, then in/out are not returned and must be
        determined */
        if (tree_node->in_vertex == 0) tree_node->in_vertex = sin_vertex;
        if (tree_node->out_vertex == 0) tree_node->out_vertex = sout_vertex;
        lb->Get_Child_Weight(lb->Get_Child_Weight_Data, 
                             num_gid_entries, num_lid_entries,
                             slocal_gids, slocal_lids, lb->Obj_Weight_Dim,
                             tree_node->weight, &ierr);
      }

      LB_SET_GID(lb, plocal_gids, slocal_gids);
      LB_SET_LID(lb, plocal_lids, slocal_lids);
      found = lb->Get_Next_Coarse_Obj(lb->Get_Next_Coarse_Obj_Data,
                                      num_gid_entries, num_lid_entries,
                                      plocal_gids, plocal_lids,
                                      slocal_gids, slocal_lids, &sassigned,
                                      &snum_vert, vertices,
                                      &sin_vertex, &sout_vertex, &ierr);
    }
    LB_FREE(&slocal_gids);
    LB_FREE(&slocal_lids);
    LB_FREE(&plocal_gids);
    LB_FREE(&plocal_lids);
  }
  return(final_ierr);
}

void LB_Reftree_Print(LB *lb, LB_REFTREE *subroot, int level)
{
/*
 * Print the refinement tree, for debugging
 */

  int i, me;

  if (subroot == NULL) return;

  me = lb->Proc;
  printf("\n");
  printf("[%d] refinement tree node with local id ", me);
  LB_PRINT_LID(lb, subroot->local_id);
  printf(" on level %d\n", level);
  printf("[%d]   Global ID ",me);
  LB_PRINT_GID(lb, subroot->global_id);
  printf("\n");
  printf("[%d]   first weight %f\n",me,subroot->weight[0]);
  printf("[%d]   first summed weight %f\n",me,subroot->summed_weight[0]);
  printf("[%d]   first my_sum weight %f\n",me,subroot->my_sum_weight[0]);
  printf("[%d]   number of vertices %d\n",me,subroot->num_vertex);
  printf("[%d]   vertices ",me);
  for (i=0; i<subroot->num_vertex; i++) printf("%d ",subroot->vertices[i]);
  printf("\n");
  printf("[%d]   in and out vertices %d %d\n",me,subroot->in_vertex,subroot->out_vertex);
  printf("[%d]   assigned_to_me %d\n",me,subroot->assigned_to_me);
  printf("[%d]   partition %d\n",me,subroot->partition);
  printf("[%d]   number of children %d \n",me,subroot->num_child);
  printf("[%d]   children follow.\n",me);
  for (i=0; i<subroot->num_child; i++)
    LB_Reftree_Print(lb,&(subroot->children[i]),level+1);
}

/* TEMP child_order */

static void get_child_order_recur(LB *lb, LB_REFTREE *subroot, int *isub, int *order)
{

  /*
   * adds the children to order and recursively continues down the tree
   */

int i;

  /*
   * if no children, done with this branch
   */

  if (subroot->num_child == 0) {
    return;
  }

  /*
   * add the subroot and children to order
   */

  order[*isub] = subroot->local_id[0];
  for (i=0; i<subroot->num_child; i++) {
    order[*isub+i+1] = (subroot->children[i]).local_id[0];
  }
  *isub = *isub + subroot->num_child + 1;

  /*
   * traverse the children
   */

  for (i=0; i<subroot->num_child; i++) {
    get_child_order_recur(lb, &(subroot->children[i]), isub, order);
  }
}

void LB_Get_Child_Order(LB *lb, int *order, int *ierr)
{
/*
 * Return the order of the children in the refinement tree.
 * Upon return, order contains LIDs assumed to be an integer.  It contains
 * sets of entries consisting of the LID of an element followed by the
 * LIDs of the children in the order determined by the reftree code.
 * order should be allocated to the correct size by the caller.
 * This is a hack, will be removed in the future, and should not be publicized.
 */

char *yo = "LB_Get_Child_Order";
int isub;
LB_REFTREE *root;

  *ierr = LB_OK;

  /*
   * initialize the tree, if not already done
   */

  if (lb->Data_Structure == NULL) {
    *ierr = LB_Reftree_Init(lb);
    if (*ierr==LB_FATAL || *ierr==LB_MEMERR) {
      LB_PRINT_ERROR(lb->Proc, yo,
                     "Error returned by LB_Reftree_Init.");
      return;
    }
  }

  /*
   * build the refinement tree
   */

  *ierr = LB_Reftree_Build(lb);
  if (*ierr==LB_FATAL || *ierr==LB_MEMERR) {
    LB_PRINT_ERROR(lb->Proc, yo,
                   "Error returned by LB_Reftree_Build.");
    return;
  }

  /*
   * traverse the tree to find the child order
   */

  root = ((struct LB_reftree_data_struct *)lb->Data_Structure)->reftree_root;
  isub = 0;
  get_child_order_recur(lb,root,&isub,order);

  /*
   * delete the tree, except for the first level (initial coarse grid)
   */

  LB_Reftree_Reinitialize(lb);

}

/* end TEMP child_order */
