/*====================================================================
 * ------------------------
 * | CVS File Information |
 * ------------------------
 *
 * $RCSfile$
 *
 * $Author$
 *
 * $Date$
 *
 * $Revision$
 *
 *====================================================================*/

#include "lb_const.h"
#include "lb_util_const.h"
#include "all_allo_const.h"
#include "comm_const.h"
#include "parmetis_jostle_const.h"
#include "params_const.h"

/*********************************************************************/
/* Verify ParMetis graph structure.                                  */
/*                                                                   */
/* Fatal error :                                                     */
/*   - non-symmetric graph                                           */
/*   - incorrect vertex number                                       */
/*   - negative vertex or edge weight                                */
/* Warning :                                                         */
/*   - zero vertex or edge weight                                    */
/*   - self-edge                                                     */
/*   - multiple edges between a pair of vertices                     */
/*                                                                   */
/* Input parameter check_graph specifies the level of verification:  */
/*   0 - perform no checks at all                                    */
/*   1 - verify on-processor part of graph                           */
/*   2 - verify complete graph (requires communication)              */
/*********************************************************************/

int LB_verify_graph(MPI_Comm comm, idxtype *vtxdist, idxtype *xadj, 
       idxtype *adjncy, idxtype *vwgt, idxtype *adjwgt, 
       int vwgt_dim, int ewgt_dim, int check_graph)
{
  int i, j, ii, jj, k, kk, num_obj, nedges, ierr;
  int global_i, global_j, flag, cross_edges, mesg_size;
  int nprocs, proc, *proclist;
  idxtype *ptr1, *ptr2;
  char *sendbuf, *recvbuf;
  struct Comm_Obj *comm_plan;
  static char *yo = "LB_verify_graph";

  ierr = LB_OK;
  if (check_graph == 0) /* perform no error checking at all */
     return ierr;

  /* Get number of procs and my rank */
  MPI_Comm_size(comm, &nprocs);
  MPI_Comm_rank(comm, &proc);

  num_obj = vtxdist[proc+1] - vtxdist[proc];

  /* Verify that vertex weights are positive */
  if (vwgt_dim){
    for (i=0; i< vwgt_dim*num_obj; i++){
       if (vwgt[i] < 0) {
          fprintf(stderr, "Zoltan error: Negative object weight of %g in %s\n", 
                  vwgt[i], yo);
          ierr = LB_FATAL;
          return ierr;
       }
       else if (vwgt[i] == 0){
          fprintf(stderr, "Zoltan warning: Zero object weight in %s (after scaling)\n", 
                  yo);
          ierr = LB_WARN;
       }
    }
  }

  /* Verify that edge weights are positive */
  nedges = xadj[num_obj];
  if (ewgt_dim){
    for (j=0; j<ewgt_dim*nedges; j++){
       if (adjwgt[j] < 0) {
          fprintf(stderr, "Zoltan error: Negative communication weight of %g in %s\n", 
          vwgt[j], yo);
          ierr = LB_FATAL;
          return ierr;
       }
       else if (adjwgt[j] == 0){
          fprintf(stderr, "Zoltan warning: Zero communication weight in %s\n", yo);
          ierr = LB_WARN;
       }
    }
  }

  /* Verify that the graph is symmetric (edge weights, too) */
  /* Also check for self-edges and multi-edges */

  /* First pass: Check on-processor edges and count # off-proc edges */
  cross_edges = 0;
  for (i=0; i<num_obj; i++){
    global_i = vtxdist[proc]+i;
    for (ii=xadj[i]; ii<xadj[i+1]; ii++){
      global_j = adjncy[ii];
      /* Reasonable vertex value? */
      if ((global_j < vtxdist[0]) || (global_j >= vtxdist[nprocs])){
        fprintf(stderr, "Zoltan error: Edge to invalid vertex %d detected in %s.\n", 
                global_j, yo);
        ierr = LB_FATAL;
        return ierr;
      }
      /* Self edge? */
      if (global_j == global_i){
        fprintf(stderr, "Zoltan warning: Self edge for vertex %d detected in %s.\n", 
                global_i, yo);
        ierr = LB_WARN;
      }
      /* Duplicate edge? */
      for (kk=xadj[i]; kk<xadj[i+1]; kk++){
        if ((kk != ii) && (adjncy[kk] == adjncy[ii])){
          fprintf(stderr, "Zoltan warning: Duplicate edge (%d,%d) detected in %s.\n", 
                  global_i, global_j, yo);
          ierr = LB_WARN;
        }
      }
      /* Is global_j a local vertex? */
      if ((global_j >= vtxdist[proc]) && (global_j < vtxdist[proc+1])){
        /* Check if (global_j, global_i) is an edge */
        j = global_j - vtxdist[proc];
        flag = 0;
        for (jj=xadj[j]; jj<xadj[j+1]; jj++){
          if (adjncy[jj] == global_i) {
            flag = 1;
            if (ewgt_dim){
              /* Compare weights */
              for (k=0; k<ewgt_dim; k++){
                if (adjwgt[jj*ewgt_dim+k] != adjwgt[ii*ewgt_dim+k])
                   ierr = LB_FATAL;
              }
            }
            break;
          }
        }
        if (!flag) {
          fprintf(stderr, "Zoltan error: Graph in not symmetric in %s. "
                  "Edge (%d,%d) exists, but no edge (%d,%d).\n", 
                  yo, global_i, global_j, global_j, global_i);
          ierr = LB_FATAL;
          return ierr;
        }
      }
      else {
        cross_edges++;
      }
    }
  }

  if ((check_graph >= 2) && cross_edges) {
    /* Allocate space for off-proc data */
    mesg_size = (2+ewgt_dim)*sizeof(int);
    sendbuf = (char *) LB_MALLOC(cross_edges*mesg_size);
    recvbuf = (char *) LB_MALLOC(cross_edges*mesg_size);
    proclist = (int *) LB_MALLOC(cross_edges*sizeof(int));

    if (!(sendbuf && recvbuf && proclist)){
       fprintf(stderr, "Zoltan error: Out of memory in %s\n", yo);
       ierr = LB_MEMERR;
       goto error;
    }

    /* Second pass: Copy data to send buffer */
    kk = 0;
    ptr1 = (idxtype *) sendbuf;
    for (i=0; i<num_obj; i++){
      global_i = vtxdist[proc]+i;
      for (ii=xadj[i]; ii<xadj[i+1]; ii++){
        global_j = adjncy[ii];
        /* Is global_j off-proc? */
        if ((global_j < vtxdist[proc]) || (global_j >= vtxdist[proc+1])){
           /* Add to list */
           for (k=0; global_j < vtxdist[k+1]; k++)
           proclist[kk] = k;
           /* Copy (global_i, global_j) and corresponding weights to sendbuf */
           memcpy((char *) ptr1, (char *) &global_i, sizeof(idxtype)); 
           ptr1++;
           memcpy((char *) ptr1, (char *) &global_j, sizeof(idxtype)); 
           ptr1++;
           for (k=0; k<ewgt_dim; k++){
             memcpy((char *) ptr1, (char *) &adjwgt[ii*ewgt_dim+k], sizeof(idxtype)); 
             ptr1++;
           }
        }
      }
    }

    /* Do the irregular communication */
    LB_Comm_Create(&comm_plan, cross_edges, proclist, comm, TAG1, TRUE, &k);
    if (k != cross_edges){
        fprintf(stderr, "Zoltan error: Incorrect number of edges to/from proc %d\n",
                proc);
        ierr = LB_FATAL;
        goto error;
    }

    LB_Comm_Do(comm_plan, TAG2, sendbuf, mesg_size, recvbuf);

    LB_Comm_Destroy(&comm_plan);


    /* Third pass: Compare on-proc data to the off-proc data we received */
    /* sendbuf and recvbuf should contain the same data except (i,j) is (j,i) */
    for (i=0, ptr1=(idxtype *)sendbuf; i<cross_edges; i++, ptr1 += (2+ewgt_dim)){
      flag = 0;
      for (j=0, ptr2=(idxtype *)recvbuf; j<cross_edges; j++, ptr2 += (2+ewgt_dim)){
        if ((ptr2[0] == ptr1[1]) && (ptr2[1] == ptr1[0])){
          /* Found matching edge */
          flag = 1;
          /* Check weights */
          for (k=0; k<ewgt_dim; k++){
            if (ptr1[2+k] != ptr2[2+k]){
              fprintf(stderr, "Zoltan error: edge weight (%d,%d) is not symmetric: "
                      "%d != %d\n", ptr1[0], ptr1[1], ptr1[2+k], ptr2[2+k]);
              ierr = LB_FATAL;
              goto error;
            }
          }
        }
      }
      if (!flag){
        fprintf(stderr, "Zoltan error: Graph is not symmetric in %s. ",
                "Edge (%d,%d) exists, but not (%d,%d)\n", 
                yo, ptr1[0], ptr1[1], ptr1[1], ptr1[0]);
        ierr = LB_FATAL;
        goto error;
      }
    }

error:
    /* Free memory */
    LB_FREE(sendbuf);
    LB_FREE(recvbuf);
    LB_FREE(proclist);
  }

  /* Return error code */
  return ierr;
}

