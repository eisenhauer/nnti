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
 
#ifdef __cplusplus
/* if C++, define the rest of this header file as extern C */
extern "C" {
#endif

#include <stdlib.h>
#include "phg.h"


static ZOLTAN_PHG_MATCHING_FN matching_local;   /* function for local matching */
static ZOLTAN_PHG_MATCHING_FN matching_ipm;     /* inner product matching */
static ZOLTAN_PHG_MATCHING_FN matching_col_ipm; /* local ipm along proc columns*/


/*****************************************************************************/
int Zoltan_PHG_Set_Matching_Fn (PHGPartParams *hgp)
{
    int exist=1;
    
    if (!strcasecmp(hgp->redm_str, "no"))
        hgp->matching = NULL;
    else if (!strncasecmp(hgp->redm_str, "l-", 2))  {
        HGPartParams hp;

        strcpy(hp.redm_str, hgp->redm_str+2);
        strcpy(hp.redmo_str, hgp->redmo_str);
        if (!Zoltan_HG_Set_Matching_Fn(&hp)) {
            exist = 0;
            hgp->matching = NULL;
        } else {   
            /* just to make sure that coarsening will continue. We'll not call
             * this code for global matching. Actually, we'll pick the best 
             * local, but code structure doesn't allow us to use a function */
            hgp->matching = matching_local; 
            hgp->locmatching = hp.matching;
            hgp->matching_opt = hp.matching_opt;
        }
    } else if (!strcasecmp(hgp->redm_str, "c-ipm"))
        hgp->matching = matching_col_ipm;   
    else if (!strcasecmp(hgp->redm_str, "ipm"))
        hgp->matching = matching_ipm;
    else {
        exist = 0;
        hgp->matching = NULL;
    }
    
    return exist;
}



/*****************************************************************************/
int Zoltan_PHG_Matching (
  ZZ *zz,
  HGraph *hg,
  Matching match,
  PHGPartParams *hgp)
{
float *old_ewgt = NULL, *new_ewgt = NULL;
int   err = ZOLTAN_OK;
char  *yo = "Zoltan_PHG_Matching";

  ZOLTAN_TRACE_ENTER(zz, yo);

  /* Scale the weight of the edges */
  if (hgp->edge_scaling) {
     if (!(new_ewgt = (float*) ZOLTAN_MALLOC (hg->nEdge * sizeof(float)))) {
        ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Insufficient memory.");
        err = ZOLTAN_MEMERR;
        goto End;
     }
 
     Zoltan_PHG_Scale_Edges (zz, hg, new_ewgt, hgp);
     old_ewgt = hg->ewgt;
     hg->ewgt = new_ewgt;
  }

  /* Create/update scale vector for vertices for inner product */
  if (hgp->vtx_scaling) {
     Zoltan_PHG_Scale_Vtx (zz, hg, hgp);
  }


  /* Do the matching */
  if (hgp->locmatching) {  /* run local matching */
      int limit=hg->nVtx;
      PHGComm *hgc=hg->comm;
      int root_matchcnt, root_rank;
                
      if (hgp->matching)
          err = hgp->locmatching (zz, hg, match, &limit);

      /* Optimization */
      if (hgp->matching_opt) 
          err = hgp->matching_opt (zz, hg, match, &limit);

      /* find the index of the proc in column group with the best match
         (max #matches); it will be our root proc */
      Zoltan_PHG_Find_Root(hg->nVtx-limit, hgc->myProc_y, hgc->col_comm,
                           &root_matchcnt, &root_rank);

      MPI_Bcast(match, hg->nVtx, MPI_INT, root_rank, hgc->col_comm);

  } else if (hgp->matching) /* run global or column/row matching algorithms */
     err = hgp->matching (zz, hg, match, hgp);

End: 

  /* Restore the old edge weights if scaling was used. */
  if (hgp->edge_scaling)
      hg->ewgt = old_ewgt;

  ZOLTAN_FREE ((void**) &new_ewgt);
  ZOLTAN_TRACE_EXIT (zz, yo);
  return err;
}


static int matching_local(ZZ *zz, HGraph *hg, Matching match, PHGPartParams *hgp)
{
    uprintf(hg->comm, "Something wrong! This function should not be called!\n");
    /* UVC: NOTE:
       The reason that we're not doing local matchin in this function, we don't
       have access to parameter structure. So there is no way to figure
       out which "local" matching needs to be called. Hence we do it
       in Zoltan_PHG_Matching */
    /* EBEB: Added the parameter struct as an input argument, so now we 
       could change the structure.  */
    return ZOLTAN_OK;
}


    
/* local inner product matching among vertices in each proc column */
/* code adapted from serial matching_ipm method */
#define MAX_NNZ 50  /* Max number of nonzeros to store for each inner product */
static int matching_col_ipm(ZZ *zz, HGraph *hg, Matching match, PHGPartParams *hgp)
{
    int   i, j, k, v1, v2, edge, best_vertex;
    int   nadj, dense_comm;
    float maxip= 0.;
    int   *adj=NULL;
    int   *order=NULL;
    float *lips, *gips; /* local and global inner products */
    float *ptr;
    char  *sendbuf, *recvbuf; /* comm buffers */
    char   msg[160];          /* error messages */
    char  *yo = "matching_col_ipm";
    PHGComm *hgc = hg->comm;  
    float lquality[3] = {0,0,0}; /* local  matchcount, matchweight */
    float gquality[3] = {0,0,0}; /* global matchcount, matchweight */

    lips = gips = NULL;
    sendbuf = recvbuf = NULL;

    if (!(lips = (float*) ZOLTAN_MALLOC(hg->nVtx * sizeof(float))) 
     || !(gips = (float*) ZOLTAN_MALLOC(hg->nVtx * sizeof(float)))
     || !(adj =  (int*)  ZOLTAN_MALLOC(hg->nVtx * sizeof(int))) 
     || !(order = (int*)  ZOLTAN_MALLOC(hg->nVtx * sizeof(int)))){
        Zoltan_Multifree(__FILE__, __LINE__, 4, &lips, &gips, &adj, &order);
        ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Insufficient memory.");
        return ZOLTAN_MEMERR;
    }
    
    for (i = 0; i < hg->nVtx; i++){
        lips[i] = gips[i] = .0;
    }

    /* Do dense communication for small problems. */
    dense_comm = (hg->nVtx < 2*MAX_NNZ);

    if (!dense_comm){

      /* allocate buffers */
      if (!(sendbuf = (char*) ZOLTAN_MALLOC(2*MAX_NNZ * sizeof(float)))) {
        ZOLTAN_FREE(&sendbuf);
        ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Insufficient memory.");
        return ZOLTAN_MEMERR;
      }
      if (hgc->myProc_y==0 &&
        !(recvbuf = (char*) ZOLTAN_MALLOC(2*MAX_NNZ*hgc->nProc_y* sizeof(float)))) {
        ZOLTAN_FREE(&sendbuf);
        ZOLTAN_FREE(&recvbuf);
        ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Insufficient memory.");
        return ZOLTAN_MEMERR;
      }
    }

    /* Compute vertex visit order. */
    Zoltan_PHG_Vertex_Visit_Order(zz, hg, hgp, order);

    /* for every vertex */
    for (k = 0; k < hg->nVtx; k++) {
        v1 = order[k];

        if (match[v1] != v1)
            continue;

        nadj = 0;  /* number of neighbors */

        /* for every hyperedge containing the vertex */
        for (i = hg->vindex[v1]; i < hg->vindex[v1+1]; i++) {
            edge = hg->vedge[i];
                
            /* for every other vertex in the hyperedge */
            for (j = hg->hindex[edge]; j < hg->hindex[edge+1]; j++) {
                v2 = hg->hvertex[j];
                if (match[v2] == v2) {
                    /* v2 is not matched yet */
                    if (lips[v2]==0.0)   /* v2 is a new neighbor */
                        adj[nadj++] = v2;
                    /* Check for vertex scaling. For efficiency we may 
                       have to move the 'if' test out of the inner loop. 
                       Only scale for v2 not v1 to save flops.              */ 
                    if (hgp->vtx_scal)
                      lips[v2] += hgp->vtx_scal[v2]*
                                  (hg->ewgt ? hg->ewgt[edge] : 1.0);
                    else
                      lips[v2] += (hg->ewgt ? hg->ewgt[edge] : 1.0);
                }
            }
        }

        /* sum up local inner products along proc column */
        /* currently we communicate for just one vertex at a time */
        /* future: use sparse communication for a chunk of vertices */

#ifdef DEBUG_EB
        printf("[%1d] Debug: %d out of %d entries are nonzero\n", 
          zz->Proc, nadj, hg->nVtx);
#endif

        if (dense_comm){
          /* dense communication: treat lips & gips as dense vectors */
          MPI_Reduce(lips, gips, hg->nVtx, MPI_FLOAT, MPI_SUM, 0,
            hgc->col_comm);              
          /* clear lips array for next iter  */
          for (i = 0; i < hg->nVtx; i++) {
            lips[i] = 0.0;
          }
        }
        else {
          /* sparse communication */
          /* we send partial inner products to root row */

          /* pack data into send buffer as pairs (index, value) */
          ptr = (float *) sendbuf;
#ifdef DEBUG_EB
          printf("Debug: Send data; myProc_y=%d, v1=%d\n", hgc->myProc_y, v1);
#endif
          if (nadj <= MAX_NNZ){
            for (i=0; i<MIN(nadj,MAX_NNZ); i++){
              *ptr++ = (float) adj[i];
              *ptr++ = lips[adj[i]];
#ifdef DEBUG_EB
              printf("%d %4.2f, ", adj[i], *(ptr-1));
#endif
            }
            if (i<MAX_NNZ-1){
              /* marker to say there is no more data */
              *ptr++ = -1.;
              *ptr++ = -1.;
#ifdef DEBUG_EB
              printf("%d %d, ", -1, -1);
#endif
            }
#ifdef DEBUG_EB
            printf("\n");
#endif
          }
          else {
            /* pick highest values if too many nonzeros */
            /* naive algorithm to find top MAX_NNZ values */ 
            /* quickselect would be faster! */
#ifdef DEBUG_EB
            printf("Debug: nadj= %d > MAX_NNZ= %d, inexact inner product!\n",
                nadj, MAX_NNZ);
#endif
            for (i=0; i<MAX_NNZ; i++){
              maxip = 0.0;
              for (j=0; j<nadj; j++){
                if (lips[adj[j]]>maxip){
                  best_vertex = adj[j];
                  maxip = lips[best_vertex];
                  lips[best_vertex] = 0.0;
                }
              }
              *ptr++ = (float) best_vertex;
              *ptr++ = maxip;
            }
          }

          /* send partial inner product values to root row */
          /* use fixed size, probably faster than variable sized Gatherv */
          MPI_Gather (sendbuf, 2*MAX_NNZ, MPI_FLOAT, recvbuf,
           2*MAX_NNZ, MPI_FLOAT, 0, hgc->col_comm);

          /* root unpacks data into gips array */
          if (hgc->myProc_y==0){
            nadj = 0;
            for (i=0; i<hgc->nProc_y; i++){
#ifdef DEBUG_EB
              printf("Debug: Received data, v1=%d, col=%d\n", v1, i);
#endif
              ptr = (float *) recvbuf;
              ptr += i*2*MAX_NNZ;
              for (j=0; j<MAX_NNZ; j++){
                v2 = *ptr++;
                if (v2<0) break; /* skip to data from next proc */
                /* EBEB Sanity check for debugging */
                if (v2 > hg->nVtx){
                  sprintf(msg, "vertex %d > %d is out of range!\n", v2, hg->nVtx);
                  ZOLTAN_PRINT_ERROR(zz->Proc, yo, msg);
                }
#ifdef DEBUG_EB
                printf(" %d %4.2f, ", v2, *ptr);
#endif
                if (gips[v2]==0.0)
                  adj[nadj++] = v2;
                gips[v2] += *ptr++;
              }
#ifdef DEBUG_EB
              printf("\n");
#endif
            }
          }
        }

        /* now choose the vector with greatest inner product */
        /* do only on root proc, then broadcast to column */
        if (hgc->myProc_y==0){
          maxip = 0.;
          best_vertex = -1;
          if (dense_comm){
#ifdef DEBUG_EB
            printf("Debug: v1=%d, gips=\n", v1);
#endif
            for (i = 0; i < hg->nVtx; i++) {
              v2 = order[i];
#ifdef DEBUG_EB
              if (gips[v2]>0) printf(" %d=%4.2f, ", v2, gips[v2]);
#endif
              if (gips[v2] > maxip && v2 != v1 && match[v2] == v2) {
                  maxip = gips[v2];
                  best_vertex = v2;
              }
            }
#ifdef DEBUG_EB
            printf("\n");
#endif
          }
          else { /* sparse */
#ifdef DEBUG_EB
            printf("Debug: v1=%d, gips=\n", v1);
#endif
            for (i = 0; i < nadj; i++) {
              v2 = adj[i];
#ifdef DEBUG_EB
              if (gips[v2]>0) printf(" %d=%4.2f, ", v2, gips[v2]);
#endif
              /* Tie-breaking strategy makes a difference!
               * Random or using the order array seem to work well. */
              if (((gips[v2] > maxip) || (gips[v2]==maxip && 
                order[v2] < order[best_vertex]))
                && v2 != v1 && match[v2] == v2) {
                  maxip = gips[v2];
                  best_vertex = v2;
              }
              lips[v2] = gips[v2] = .0; /* clear arrays for next iter */
            }
#ifdef DEBUG_EB
            printf("\n");
#endif
          }
        } 

        /* broadcast the winner, best_vertex */
        MPI_Bcast(&best_vertex, 1, MPI_INT, 0, hgc->col_comm); 

        /* match if inner product > 0 */
        if (best_vertex > -1) {
            match[v1] = best_vertex;
            match[best_vertex] = v1;
            if (hgc->myProc_y==0){
              /* match quality computation */
              lquality[0] += maxip;
              lquality[1] += 1.0;
            }
        } 

        
    }

    if (hgc->myProc_y==0){
      lquality[2] = hg->nVtx; /* to find global number of vertices */
      MPI_Allreduce(lquality, gquality, 3, MPI_FLOAT, MPI_SUM, hgc->row_comm); 
       
      uprintf (hgc, "LOCAL (GLOBAL) i.p. sum %.2f (%.2f), matched pairs %d (%d), "
       "total vertices %d\n", lquality[0], gquality[0], (int)lquality[1],
       (int)gquality[1], (int)gquality[2]);  
    }

    /*
    printf("DEBUG: Final Matching:\n");
    for(i = 0; i < hg->nVtx; i++)
        printf("%d, %d\n", i, match[i]);
    printf("\n");
    */

    if (!dense_comm){
      ZOLTAN_FREE(&sendbuf);
      ZOLTAN_FREE(&recvbuf);
    }
    Zoltan_Multifree(__FILE__, __LINE__, 4, &lips, &gips, &adj, &order);
    return ZOLTAN_OK;
}



/****************************************************************************
 * inner product matching
 * Parallelized version of the serial algorithm (see hg_match.c)
 * Based on conversations with Rob Bisseling by Aaron Becker, UIUC, summer 2004
 * completed by R. Heaphy
 */
               
#define ROUNDS_CONSTANT 8     /* forces candidates to have enough freedom */ 
#define PSUM_THRESHOLD 0.0    /* ignore inner products (i.p.) < threshold */
#define TSUM_THRESHOLD 0.0    /* ignore inner products (i.p.) < threshold */
#define IPM_TAG        28731

static int communication_by_plan (ZZ* zz, int nsend, int* dest, int* size, 
 int scale, int* send, int* nrec, int* recsize, int* nRec, int** rec,
 MPI_Comm comm, int tag);
 

static int matching_ipm (ZZ *zz, HGraph* hg, Matching match, PHGPartParams *hgp)
{
  int i, j, k, n, m, round, pvisit, kstart, *r, *s;   /* loop counters */
  int lno, gno, count, old_kstart;                    /* temp variables */
  int nsend, sendsize, nrec, recsize, msgsize;        /* temp variables */
  int nRounds, nCandidates, nTotal;                   /* almost constants */
  int *send, *dest, *size, *rec, *index, *aux;        /* working buffers */
  int nSend, nDest, nSize, nRec, nIndex, nAux;        /* buffer sizes in ints */
  float *sums, *f, bestsum;                           /* working buffer, float */
  int *visit, *cmatch, *select, *permute, *edgebuf;   /* fixed usage arrays */
  int nEdgebuf;                                       /* buffer size in ints */
  PHGComm *hgc = hg->comm;
  int err = ZOLTAN_OK, old_row, row, col, nPins, nVtx;
  int *rows[hgc->nProc_y + 1], bestlno, vertex, nselect;
  char *yo = "matching_ipm";
  
  /* this restriction will be removed later, but for now NOTE this test */
  if (sizeof(int) != sizeof (float))  {
    uprintf (hgc, "This code must be modified before using\n");
    err = ZOLTAN_FATAL;
    goto fini;
  }
  
  /* determine basic working limits and row rank */
  nRounds     = hgc->nProc_x * ROUNDS_CONSTANT;
  nCandidates = hg->nVtx/(2 * nRounds) + 1;  /* 2: each match pairs 2 vertices */
  
  /* determine maximum global number of candidates (nTotal), nVtx, nPins */
  MPI_Allreduce (&hg->nPins, &nPins, 1, MPI_INT, MPI_MAX, hgc->row_comm);
  nVtx = nTotal = 0;
  for (i = 0; i < hgc->nProc_x; i++)  {
    count = hg->dist_x[i+1]-hg->dist_x[i];    /* number of vertices on proc i */
    if (count > nVtx)
       nVtx = count;
    nTotal += (1 + (count / (2 * hgc->nProc_x * ROUNDS_CONSTANT)));
  }
            
  /* allocate working and fixed sized array storage */
  sums = NULL;
  send = dest = size = rec = index = aux = visit = cmatch = select = NULL;
  permute = edgebuf = NULL;
  nDest = nSize = nIndex = nAux = 1 + MAX(nTotal, nVtx);    /* initial sizing */
  nSend = nRec = nEdgebuf       = 1 + MAX(nPins, nVtx+2);   /* initial sizing */
    
  if (!(cmatch = (int*)   ZOLTAN_MALLOC (hg->nVtx    * sizeof (int)))
   || !(visit  = (int*)   ZOLTAN_MALLOC (hg->nVtx    * sizeof (int)))
   || !(permute= (int*)   ZOLTAN_MALLOC (hg->nVtx    * sizeof (int)))
   || !(edgebuf= (int*)   ZOLTAN_MALLOC (nEdgebuf    * sizeof (int)))
   || !(select = (int*)   ZOLTAN_MALLOC (nCandidates * sizeof (int)))        
   || !(sums   = (float*) ZOLTAN_CALLOC (hg->nVtx,     sizeof (float)))
   || !(send   = (int*)   ZOLTAN_MALLOC (nSend       * sizeof (int)))
   || !(dest   = (int*)   ZOLTAN_MALLOC (nDest       * sizeof (int)))
   || !(size   = (int*)   ZOLTAN_MALLOC (nSize       * sizeof (int)))
   || !(rec    = (int*)   ZOLTAN_MALLOC (nRec        * sizeof (int)))
   || !(index  = (int*)   ZOLTAN_MALLOC (nIndex      * sizeof (int)))
   || !(aux    = (int*)   ZOLTAN_MALLOC (nAux        * sizeof (int))))  {
     ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Insufficient memory.");
     err = ZOLTAN_MEMERR;
     goto fini;
  }
    
  /* match[] is local slice of global matching array.  It uses local numbering 
   * (zero-based). Initially, match[i] = i.  After matching, match[i]=i indicates
   * an unmatched vertex. A matching between vertices i & j is indicated by 
   * match[i] = j & match [j] = i.  NOTE: a match to an off processor vertex is
   * indicated by a negative number, -(gno+1), and must use global numbers
   * (gno's) and not local numbers, lno's which are zero based.        */

  /* Compute vertex visit order. Random is default. */
  Zoltan_PHG_Vertex_Visit_Order(zz, hg, hgp, visit);
  
  /* Loop processing ncandidates vertices per column each round.
   * Each loop has 4 phases:
   * Phase 1: send ncandidates vertices for global matching - horizontal comm
   * Phase 2: sum  inner products, find best        - vertical communication
   * Phase 3: return best sums to owning column     - horizontal communication
   * Phase 4: return actual match selections        - horizontal communication
   *
   * No conflict resolution required because temp locking prevents conflicts. */
  
  pvisit = 0;                                    /* marks position in visit[] */
  for (round = 0; round < nRounds; round++)  {
    memcpy (cmatch, match, hg->nVtx * sizeof(int));   /* for temporary locking */
         
    /************************ PHASE 1: ***************************************/
            
    /* Select upto nCandidates unmatched vertices to globally match. */
    for (nsend = 0; nsend < nCandidates && pvisit < hg->nVtx; pvisit++)
      if (cmatch[visit[pvisit]] == visit[pvisit])  {         /* unmatched */
        select[nsend++] = visit[pvisit];      /* select it as a candidate */
        cmatch[visit[pvisit]] = -1;           /* mark it as a pending match */
      }  
    nselect = nsend;                          /* save for later use */
                        
    /* fill send buff as list of <gno, gno's edge count, list of edge gno's> */
    /* NOTE: can't overfill send buffer by definition of initial sizing */
    s = send ;
    for (i = 0; i < nsend; i++)   {
      lno = select[i];
      *s++ = VTX_LNO_TO_GNO (hg, lno);                                 /* gno */
      *s++ = hg->vindex[lno+1] - hg->vindex[lno];                    /* count */
      for (j = hg->vindex[lno]; j < hg->vindex[lno+1]; j++)  
        *s++ = hg->vedge[j];                                   /* lno of edge */
    }        
    sendsize = s - send;           
    
    /* determine actual global number of candidates this round */
    MPI_Allreduce (&nsend, &nTotal, 1, MPI_INT, MPI_SUM, hgc->row_comm);     
    if (nTotal == 0)
      break;             /* unlikely, but globally, all work is done, so quit */
            
    /* communication to determine global size and displacements of rec buffer */
    MPI_Allgather (&sendsize, 1, MPI_INT, size, 1, MPI_INT, hgc->row_comm); 
     
    /* determine the size of the rec buffer & reallocate bigger iff necessary */
    recsize = 0;
    for (i = 0; i < hgc->nProc_x; i++)
      recsize += size[i];            /* compute total size of edgebuf in ints */
    if (recsize > nEdgebuf)  {
      nEdgebuf = recsize;
      if (!(edgebuf = (int*) ZOLTAN_REALLOC(edgebuf, nEdgebuf * sizeof(int)))) {
        ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Insufficient memory.");
        err = ZOLTAN_MEMERR;
        goto fini;
      }
    }                          
    
    /* setup displacement array necessary for MPI_Allgatherv */
    dest[0] = 0;
    for (i = 1; i < hgc->nProc_x; i++)
      dest[i] = dest[i-1] + size[i-1];

    /* send vertices & their edges to all row neighbors */
    MPI_Allgatherv (send, sendsize, MPI_INT, edgebuf, size, dest, MPI_INT,
     hgc->row_comm);

    /************************ PHASE 2: ***************************************/
         
    /* create random permutation of index into the edge buffer */
    i = 0;
    for (j = 0 ; j < nTotal  &&  i < recsize; j++)   {
      permute[j] = i++;             /* save index of gno in permute[] */
      count      = edgebuf[i++];    /* count of edges */
      i += count;                   /* skip over count edges */
    }
    Zoltan_Rand_Perm_Int (permute, nTotal);      /* randomly permute vertices */
            
    /* for each candidate vertex, compute all local partial inner products */
    kstart = old_kstart = 0;         /* next candidate (of nTotal) to process */
    while (kstart < nTotal)  {
      sendsize = 0;                    /* position in send buffer */
      nsend = 0;                       /* count of messages in send buffer */
      s = send;                        /* start at send buffer origin */
      for (k = kstart; k < nTotal; k++)   {   
        r     = &edgebuf[permute[k]];     
        gno   = *r++;
        count = *r++;                        /* count of following hyperedges */
        
        /* now compute the row's nVtx inner products for kth candidate */
        m = 0;
        if ((hg->ewgt == NULL) && (hgp->vtx_scal == NULL))
          for (i = 0; i < count; r++, i++)
            for (j = hg->hindex [*r]; j < hg->hindex [*r + 1]; j++) {
              if (sums [hg->hvertex[j]] == 0.0)
                index [m++] = hg->hvertex[j];
              sums [hg->hvertex[j]] += 1.0;
            }
        else if ((hg->ewgt == NULL) && (hgp->vtx_scal != NULL))
          for (i = 0; i < count; i++)
            for (j = hg->hindex [*r]; j < hg->hindex [*r + 1]; j++) {
              if (sums [hg->hvertex[j]] == 0.0)
                index [m++] = hg->hvertex[j];
              sums [hg->hvertex[j]] += hgp->vtx_scal[hg->hvertex[j]];
            }
        else if ((hg->ewgt != NULL) && (hgp->vtx_scal == NULL))
          for (i = 0; i < count; i++)
            for (j = hg->hindex [*r]; j < hg->hindex [*r + 1]; j++)  {
              if (sums [hg->hvertex[j]] == 0.0)
                index [m++] = hg->hvertex[j];
              sums [hg->hvertex[j]] += hg->ewgt [*r];
            }
        else     /* if ((hg->ewgt != NULL) && (hgp->vtx_scal != NULL)) */
          for (i = 0; i < count; i++)
            for (j = hg->hindex [*r]; j < hg->hindex [*r + 1]; j++)  {
              if (sums [hg->hvertex[j]] == 0.0)
                index [m++] = hg->hvertex[j];
              sums [hg->hvertex[j]] +=hgp->vtx_scal[hg->hvertex[j]]*hg->ewgt[*r];
            }         
                                                 
        /* if local vtx, remove self inner product (useless maximum) */
        if (VTX_TO_PROC_X (hg, gno) == hgc->myProc_x)
          sums [VTX_GNO_TO_LNO (hg, gno)] = 0.0;
         
        /* count unmatched partial sums exceeding PSUM_THRESHOLD */   
        count = 0;
        for (i = 0; i < m; i++)  {
          lno = index[i];
          if (match[lno] == lno  &&  sums[lno] > PSUM_THRESHOLD)
            aux[count++] = lno;        /* save lno for significant partial sum */
          else
            sums[lno] = 0.0;  /* (partially) clear buffer for next calc */  
        }     
        if (count == 0)
          continue;
                    
        msgsize = 4 + 2 * count;       /* row, col, gno, count of <lno, psum> */
        if (sendsize + msgsize <= nSend)  {
          /* current partial sums fit, so put them into the send buffer */
          dest[nsend]   = gno % hgc->nProc_y;  /* processor to compute tsum */
          size[nsend++] = msgsize;             /* size of message */
          sendsize     += msgsize;             /* cummulative size of message */
          
          *s++ = hgc->myProc_y;      /* save my row (for merging) */
          *s++ = hgc->myProc_x;      /* save my col (for debugging) */
          *s++ = gno;          
          *s++ = count;
          for (i = 0; i < count; i++)  {          
            *s++ = aux[i];                          /* lno of partial sum */
             f = (float*) s++;
            *f = sums[aux[i]];                      /* partial sum */           
            sums[aux[i]] = 0.0;
          }          
        }
        else             /* psum message doesn't fit into buffer */
          break;
      }                  /* DONE: loop over k */   

      /* synchronize all rows in this column to next kstart value */
      old_kstart = kstart;      
      MPI_Allreduce (&k, &kstart, 1, MPI_INT, MPI_MIN, hgc->col_comm);
      
      /* Send inner product data in send buffer to appropriate rows */
      err = communication_by_plan (zz, nsend, dest, size, 1, send, &nrec, 
       &recsize, &nRec, &rec, hgc->col_comm, IPM_TAG);
      if (err != ZOLTAN_OK)
        goto fini;
        
      /* build index into receive buffer pointer for each new row of data */
      old_row = -1;
      k = 0;
      for (r = rec; r < rec + recsize  &&  k < hgc->nProc_y; )  {     
        row = *r++;        
        col = *r++;               /* column only for debugging, may go away */
        if (row != old_row)  {
          index[k++] = r - rec;   /* points at gno, not row or col */
          old_row = row;
        }
        gno   = *r++;
        count = *r++;
        r += (count * 2);
      }
      
      /* save current positions into source rows within rec buffer */
      for (i = 0; i < k; i++)
        rows[i] = &rec[index[i]];
      for (i = k; i < hgc->nProc_y; i++)
        rows[i] = &rec[recsize];       /* in case no data came from a row */
      rows[i] = &rec[recsize];
      
      /* merge partial i.p. sum data to compute total inner products */ 
      s = send; 
      for (n = old_kstart; n < kstart; n++)  {
        m = 0;              
        gno = edgebuf [permute[n]];
                
        /* merge step: look for target gno from each row's data */
        for (i = 0; i < hgc->nProc_y; i++)  {
          if (rows[i] < &rec[recsize] && *rows[i] == gno)  {       
            count = *(++rows[i]);
            for (j = 0; j < count; j++)  {
              lno = *(++rows[i]);              
              if (sums[lno] == 0.0)       /* is this first time for this lno? */
                aux[m++] = lno;           /* then save the lno */          
              sums[lno] += *(float*) (++rows[i]);    /* sum the psums */
            }
            rows[i] += 3;                    /* skip past row to next gno */              
          }
        }
          
        /* determine how many total inner products exceed threshold */  
        count = 0;
        for (i = 0; i < m; i++)
          if (sums[aux[i]] > TSUM_THRESHOLD)
            count++;   

        /* create <gno, count, <lno, tsum>> in send array */           
        if (count > 0)  {
          if ( (s - send) + (2 + 2 * count) > nSend ) {
            nSend += (2 + 2 * count);
            send = (int*) ZOLTAN_REALLOC (send, nSend * sizeof(int));
            if (send == NULL)  {
              ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Insufficient memory");
              return ZOLTAN_MEMERR;
            }
          }      
          *s++ = gno;
          *s++ = count;
          for (i = 0; i < m; i++)   {
            lno = aux[i];             
            if (sums[lno] > TSUM_THRESHOLD)  {
              *s++ = lno;
               f = (float*) s++;
              *f = sums[lno];
            }  
            sums[lno] = 0.0;  
          }
        }
      }
      sendsize = s - send;   /* size (in ints) of send buffer */
      
      /* Communicate total inner product results to MASTER ROW */
      MPI_Allgather (&sendsize, 1, MPI_INT, size, 1, MPI_INT, hgc->col_comm);
      recsize = 0;
      for (i = 0; i < hgc->nProc_y; i++)
        recsize += size[i];
          
      dest[0] = 0;
      for (i = 1; i < hgc->nProc_y; i++)
        dest[i] = dest[i-1] + size[i-1];
        
      if (recsize > nRec) {
        nRec = recsize;
        rec = (int*) ZOLTAN_REALLOC (rec, nRec * sizeof(int));
        if (rec == NULL)  {
          ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Insufficient memory");
          return ZOLTAN_MEMERR;
        }
      }      
               
      MPI_Gatherv (send, sendsize, MPI_INT, rec, size, dest, MPI_INT, 0,
       hgc->col_comm);
       
      /* Determine best vertex and best sum for each candidate */
      s = send;
      nsend = 0;
      if (hgc->myProc_y == 0)
        for (r = rec; r < rec + recsize;)  {
          gno   = *r++;                    /* candidate's GNO */
          count = *r++;                    /* count of nonzero pairs */
          bestsum = -1.0;                  /* any negative value will do */
          bestlno = -1;                    /* any negative value will do */
          for (i = 0; i < count; i++)  {
            lno =          *r++;
            f   =  (float*) r++;                                     
            if (*f > bestsum  &&  cmatch[lno] == lno)  {
              bestsum = *f;
              bestlno = lno;
            }
          }

          
         /************************ PHASE 3: **********************************/
          
          if (bestsum > TSUM_THRESHOLD)  {
            cmatch[bestlno] = -1;  /* mark pending match to avoid conflicts */
            *s++ = gno;
            *s++ = VTX_LNO_TO_GNO (hg, bestlno);
             f = (float*) s++;
            *f = bestsum;
            dest[nsend++] = VTX_TO_PROC_X (hg, gno);
          }
        }
        
      /* MASTER ROW only: send best results to candidates' owners */
      err = communication_by_plan (zz, nsend, dest, NULL, 3, send, &nrec,
       &recsize, &nRec, &rec, hgc->row_comm, IPM_TAG+5);
      if (err != ZOLTAN_OK)
        goto fini;

               
      /* read each message (candidate id, best match id, and best i.p.) */ 
      if (hgc->myProc_y == 0) 
        for (r = rec; r < rec + 3 * nrec; )   {
          gno    = *r++;
          vertex = *r++;
          f      = (float*) r++;
          bestsum = *f;            
                     
          /* Note: ties are broken to favor local over global matches */   
          lno =  VTX_GNO_TO_LNO (hg, gno);  
          if ((bestsum > sums [lno]) || (bestsum == sums[lno]
           && VTX_TO_PROC_X (hg, gno)    != hgc->myProc_x
           && VTX_TO_PROC_X (hg, vertex) == hgc->myProc_x))    {        
              index [lno] = vertex;
              sums  [lno] = bestsum;
          }                   
        }   
                                    
      /************************ PHASE 4: ************************************/
        
      /* fill send buffer with messages. A message is two matched gno's */
      /* Note: match to self if inner product is below threshold */
      s = send; 
      nsend = 0;
      if (hgc->myProc_y == 0)
        for (i = 0; i < nselect; i++)   {
          int d1, d2;
          lno = select[i];
          *s++ = gno = VTX_LNO_TO_GNO (hg, lno);
          *s++ = vertex = (sums [lno] > TSUM_THRESHOLD) ? index[lno] : gno;
          
          /* each distict owner (gno or vertex) needs its copy of the message */
          d1 = VTX_TO_PROC_X (hg, gno);
          d2 = VTX_TO_PROC_X (hg, vertex);
          dest[nsend++] = d1;
          if (d1 != d2)  {
            *s++ = gno;
            *s++ = vertex;        
            dest[nsend++] = d2;
          }
        }
        
      /* send match results only to impacted parties */
      err = communication_by_plan (zz, nsend, dest, NULL, 2, send, &nrec,
       &recsize, &nRec, &rec, hgc->row_comm, IPM_TAG+10);
      if (err != ZOLTAN_OK)
        goto fini;
                    
      /* update match array with current selections */
      /* Note: -gno-1 designates an external match as a negative number */
      if (hgc->myProc_y == 0)
        for (r = rec; r < rec + 2 * nrec; )  {
          gno    = *r++;
          vertex = *r++; 

          if (VTX_TO_PROC_X (hg, gno)    == hgc->myProc_x
           && VTX_TO_PROC_X (hg, vertex) == hgc->myProc_x)   {
              int v1 = VTX_GNO_TO_LNO (hg, vertex);             
              int v2 = VTX_GNO_TO_LNO (hg, gno);                
              match [v1] = v2;
              match [v2] = v1;
          }                            
          else if (VTX_TO_PROC_X (hg, gno) == hgc->myProc_x)
            match [VTX_GNO_TO_LNO (hg, gno)]    = -vertex - 1;
          else              
            match [VTX_GNO_TO_LNO (hg, vertex)] = -gno - 1;
        }      
    }                            /* DONE: kstart < nTotal loop */
              
    /* crude way to update match array to the entire column */  
    MPI_Bcast (match, hg->nVtx, MPI_INT, 0, hgc->col_comm); 
  }                                           /* DONE: loop over rounds */

fini:
  Zoltan_Multifree (__FILE__, __LINE__, 12, &cmatch, &visit, &sums, &send,
   &dest, &size, &rec, &index, &aux, &permute, &edgebuf, &select);
  return err;
}


  
static int communication_by_plan (ZZ* zz, int nsend, int* dest, int* size, 
 int scale, int* send, int* nrec, int *recsize, int* nRec, int** rec,
 MPI_Comm comm, int tag)
   {
   ZOLTAN_COMM_OBJ *plan = NULL;
   int err;
   char *yo = "communication_by_plan";
   
   
   /* communicate send buffer messages to other row/columns in my comm */  
   err = Zoltan_Comm_Create (&plan, nsend, dest, comm, tag, nrec);
   if (err != ZOLTAN_OK) {
     ZOLTAN_PRINT_ERROR (zz->Proc, yo, "failed to create plan");
     return err;
   }
   
   /* resize plan if necessary */
   if (size != NULL) {
     err = Zoltan_Comm_Resize (plan, size, tag+1, recsize);
     if (err != ZOLTAN_OK) {
       ZOLTAN_PRINT_ERROR (zz->Proc, yo, "failed to resize plan");
       return err;
     }
     scale = 1;       
   }
   
   /* realloc rec buffer if necessary */  
   if (*recsize > *nRec)  {   
     *nRec = *recsize;
     if (!(*rec = (int*) ZOLTAN_REALLOC (*rec, *nRec * sizeof(int))))  {
       ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Insufficient memory");
       return ZOLTAN_MEMERR;
     }
   }
   
   /* send messages from send buffer to destinations */      
   err = Zoltan_Comm_Do (plan, tag+2, (char*) send, scale * sizeof(int),
    (char*) *rec);
   if (err != ZOLTAN_OK)  {
      ZOLTAN_PRINT_ERROR (zz->Proc, yo, "failed in Comm_Do");
      return err;
   }
   
   /* free memory associated with the plan */
   Zoltan_Comm_Destroy (&plan); 
   return ZOLTAN_OK;
   }
   
   

#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif
