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
#include "g2l_hash.h"


static ZOLTAN_PHG_MATCHING_FN pmatching_ipm;         /* inner product matching */
static ZOLTAN_PHG_MATCHING_FN pmatching_agg_ipm;     /* agglomerative IPM */
static ZOLTAN_PHG_MATCHING_FN pmatching_local_ipm;   /* local ipm */
static ZOLTAN_PHG_MATCHING_FN pmatching_alt_ipm;     /* alternating ipm */
static ZOLTAN_PHG_MATCHING_FN pmatching_hybrid_ipm;  /* hybrid ipm */


static int Zoltan_PHG_match_isolated(ZZ* zz, HGraph* hg, PHGPartParams* hgp,
                                     Matching match, int small_degree);
static int Zoltan_PHG_compute_esize(ZZ* zz, HGraph* hg, PHGPartParams* hgp);

typedef struct triplet {
    int candidate;      /* gno of candidate vertex */
    int partner;        /* gno of best match found so far */
    float ip;           /* total inner product between candidate and partner */
} Triplet;

    
static HGraph *HG_Ptr;

/*
  Matching is NOT restricted (called as MATCH_OK) if
  1- we are not doing fixed vertex partitioning, or partitioning with preferred parts
     (i.e. !hgp->UsePrefPart)
  2- if one of the vertices is free (i.e. fv? < 0);
     in other words, it is OK if only one of them is fixed
  3- if both vertices are fixed in the "same" part; note that since we are achieving k-way
     partitioning via recursive bisection; "same" is defined as both vertices are on the
     same side of that level's bisection. For example for 4-way partitioning since
     bisec_split will be 2 in the first bisection; we'll allow vertices fixed in parts
     {0, 1} are matched among themselves, also the vertices fixed in {2, 3} are allowed to
     be matched among themselves. 
 */   
#define MATCH_OK(hgp, hg, fv1, fv2) \
        (!((hgp)->UsePrefPart)   ||     \
         ((fv1) < 0) ||     \
         ((fv2) < 0) ||     \
         ((((fv1) < (hg)->bisec_split) ? 0 : 1) == (((fv2) < (hg)->bisec_split) ? 0 : 1)))
    
/*
 In addition to above conditions, in Agglomerative matching, in order to reduce the extra
 communication, we will only allow matching if candidate vertex is free or its fixed part
 matches with the other vertex's fixed part. Hence the signature of the AGG_MATCH_OK is
 changed to identify which of two argument is candidate vertex's fixed part.
*/
#define AGG_MATCH_OK(hgp, hg, candf, fv2) \
    (!((hgp)->UsePrefPart) || (((candf) < 0) ||  \
     (((fv2)>=0) && ((((candf) < (hg)->bisec_split) ? 0 : 1) == (((fv2) < (hg)->bisec_split) ? 0 : 1)))))

    

/*****************************************************************************/
int Zoltan_PHG_Set_Matching_Fn (PHGPartParams *hgp)
{
    int exist=1;
    
    if (!strcasecmp(hgp->redm_str, "no"))
        hgp->matching = NULL;
    else if (!strcasecmp(hgp->redm_str, "none"))
        hgp->matching = NULL;
    else if (!strcasecmp(hgp->redm_str, "ipm"))
        hgp->matching = pmatching_ipm;
    else if (!strncasecmp(hgp->redm_str, "agg", 3)) { /* == "agg-ipm" */
        hgp->matching = pmatching_agg_ipm;
        hgp->match_array_type = 1;
    } else if (!strcasecmp(hgp->redm_str, "l-ipm"))
        hgp->matching = pmatching_local_ipm;           
    else if (!strcasecmp(hgp->redm_str, "c-ipm"))
        hgp->matching = pmatching_ipm;   
    else if (!strcasecmp(hgp->redm_str, "a-ipm"))
        hgp->matching = pmatching_alt_ipm;
    else if (!strcasecmp(hgp->redm_str, "h-ipm"))
        hgp->matching = pmatching_hybrid_ipm;
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
int   ierr = ZOLTAN_OK;
char  *yo = "Zoltan_PHG_Matching";

  ZOLTAN_TRACE_ENTER(zz, yo);

  /* Scale the weight of the edges */
  if (hgp->edge_scaling) {
     if (hg->nEdge && !(new_ewgt = (float*) 
                      ZOLTAN_MALLOC(hg->nEdge * sizeof(float))))
         MEMORY_ERROR;
 
     Zoltan_PHG_Scale_Edges (zz, hg, new_ewgt, hgp->edge_scaling);
     old_ewgt = hg->ewgt;
     hg->ewgt = new_ewgt;
  }

  /* Create/update scale vector for vertices for inner product */
  if (hgp->vtx_scaling) 
     Zoltan_PHG_Scale_Vtx (zz, hg, hgp);
  
  /* Do the matching */
  if (hgp->matching) {
    /* first match isolated vertices */
    Zoltan_PHG_match_isolated(zz, hg, hgp, match, 0);
    /* now do the real matching */
    if ((ierr = Zoltan_PHG_compute_esize(zz, hg, hgp))==ZOLTAN_OK)
        ierr = hgp->matching (zz, hg, match, hgp);
    /* clean up by matching "near-isolated" vertices of degree 1 */
    /* only useful in special cases (e.g. near-diagonal matrices).
       in many cases there is a slight increase in cuts, so 
       turn it off for now.                                      */
    /* Zoltan_PHG_match_isolated(zz, hg, hgp, match, 1); */
  }

End: 

  /* Restore the old edge weights if scaling was used. */
  if (hgp->edge_scaling)
      hg->ewgt = old_ewgt;

  ZOLTAN_FREE ((void**) &new_ewgt);
  ZOLTAN_TRACE_EXIT (zz, yo);
  return ierr;
}



/***************************************************************************/
/* MPI_Op for computing the maximum inner-product for each candidate */
static void phasethreereduce (
  void *tin, 
  void *tinout, 
  int  *tnum, 
  MPI_Datatype *mytype)
{
  int num = *tnum;
  Triplet *in    = (Triplet*) tin;
  Triplet *inout = (Triplet*) tinout;
  int i;

  for (i = 0; i < num; i++) {
    if (in[i].candidate == -1 && inout[i].candidate == -1) 
      continue;                         /* No values set for this candidate */

    if (in[i].ip > inout[i].ip)
      inout[i] = in[i];                 /* in has larger inner product */
    else if (in[i].ip == inout[i].ip) {
      int in_proc    = VTX_TO_PROC_X (HG_Ptr, in[i].partner);
      int inout_proc = VTX_TO_PROC_X (HG_Ptr, inout[i].partner);
      int cand_proc  = VTX_TO_PROC_X (HG_Ptr, in[i].candidate);

      /* Give preference to partners on candidate's processor */
      if (((in_proc == cand_proc) && (inout_proc == cand_proc))
       || ((in_proc != cand_proc) && (inout_proc != cand_proc))) {
        /* Both partners are on candidate's proc OR
           neither partner is on candidate's proc.
           Break ties by larger partner gno. */
        if (in[i].partner > inout[i].partner)
          inout[i] = in[i];
      }
      else if (in_proc == cand_proc) {
        inout[i] = in[i];   /* Give preference to local partner */
      }
    } 
  }
}


/* UVCUVC TODO CHECK: later consider about reusing edge size computed in
   recursive bisection split; but that also requires communication of
   those values in redistribution */ 
static int Zoltan_PHG_compute_esize(
  ZZ *zz,
  HGraph *hg,
  PHGPartParams *hgp
)
{
    int  i, *lsize=NULL;
    static char *yo = "Zoltan_PHG_compute_esize";
    int ierr = ZOLTAN_OK;

    if (hg->nEdge && !hg->esize) {
      if (!(lsize = (int*)  ZOLTAN_MALLOC(hg->nEdge*sizeof(int))))
        MEMORY_ERROR;
      if (!(hg->esize = (int*)  ZOLTAN_MALLOC(hg->nEdge*sizeof(int))))
        MEMORY_ERROR;

      for (i=0; i<hg->nEdge; ++i)
          lsize[i] = hg->hindex[i+1] - hg->hindex[i];
      MPI_Allreduce(lsize, hg->esize, hg->nEdge, MPI_INT, MPI_SUM, hg->comm->row_comm);
      
End:
      ZOLTAN_FREE(&lsize);
    }
    return ierr;
}


static int Zoltan_PHG_match_isolated(
  ZZ *zz,
  HGraph *hg,
  PHGPartParams *hgp,
  Matching match,
  int small_degree /* 0 or 1; 0 corresponds to truely isolated vertices */
)
{
    int v=-1, i, unmatched=0, *ldeg, *deg;
#ifdef _DEBUG
    int cnt=0;
#endif
    static char *yo = "Zoltan_PHG_match_isolated";
    int ierr = ZOLTAN_OK;

    if (hg->nVtx) {
      if (!(ldeg = (int*)  ZOLTAN_MALLOC(2*hg->nVtx*sizeof(int))))
        MEMORY_ERROR;
      deg = ldeg + hg->nVtx;
      /* match isolated vertices.
         UVCUVC: right now we match in the natural order,
         I don't think we need random matching but if needed
         we can match in random order. */
      for (i=0; i<hg->nVtx; ++i)
          ldeg[i] = hg->vindex[i+1] - hg->vindex[i];
      MPI_Allreduce(ldeg, deg, hg->nVtx, MPI_INT, MPI_SUM, hg->comm->col_comm);
      
      if (small_degree>0){
          /* Only match on procs with many unmatched vertices */
          unmatched= 0;
          for (i=0; i<hg->nVtx; ++i)
              if (match[i]==i) unmatched++;
      }
      if ((small_degree==0) || (unmatched > 0.8*hg->nVtx))
          for (i=0; i<hg->nVtx; ++i){
              if ((match[i]==i) && (deg[i] <= small_degree)) { 
#ifdef _DEBUG
                  ++cnt;
#endif
                  /* match with previous unmatched vertex */
                  /* EBEB For degree-1 vertices, we could be more clever
                     and match vertices that share a common neighbor */
                  if (v==-1)
                      v = i;
                  else if (MATCH_OK(hgp, hg, hg->pref_part[i], hg->pref_part[v])) {
                      match[v] = i;
                      match[i] = v;
                      v = -1;
                  }
              }
          }
#ifdef _DEBUG
      if (cnt)
          uprintf(hg->comm, "Local H(%d, %d, %d) and there were %d isolated vertices\n", hg->nVtx, hg->nEdge, hg->nPins, cnt);           
#endif
End:
      ZOLTAN_FREE(&ldeg);
    }
    return ierr;
}


/*****************************************************************************/
/* inner product matching                                                    */ 
/* based on implementations by Rob Bisseling and Umit Catalyurek             */
/* for each vertex, we match with the unmatched vertex which has the most    */
/* hyperedges in common with it (ie, the pair with greatest inner product).  */
/* by Aaron Becker, UIUC, Summer 2004                                        */
/* 8/5/04  Erik says matching_ipm is nearly equivalent to matching_rhm;
   but rhm uses a scaled inner product. */
static int matching_ipm(ZZ *zz, HGraph *hg, PHGPartParams *hgp,
                        Matching match, int *limit)
{
    int   i, j, k, n, v1, v2, edge, maxindex;
    float maxip;
    int   *adj = NULL;
    int   *order = NULL;
    float *ips = NULL; 
    char  *yo = "matching_ipm";

    if (hg->nVtx && 
        (!(ips = (float*) ZOLTAN_MALLOC(hg->nVtx * sizeof(float))) 
     || !(adj = (int*) ZOLTAN_MALLOC(hg->nVtx * sizeof(int)))
     || !(order = (int*) ZOLTAN_MALLOC(hg->nVtx * sizeof(int))))) {
        Zoltan_Multifree(__FILE__, __LINE__, 3, &ips, &adj, &order);
        ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Insufficient memory.");
        return ZOLTAN_MEMERR;
    }
    
    for (i = 0; i < hg->nVtx; i++){
        ips[i] = .0;
        order[i] = i;
    }
 
    /* random node visit order is default */
    /* other options may be added later */
    Zoltan_Rand_Perm_Int (order, hg->nVtx, NULL);

    /* for every vertex */
    for (k = 0; k < hg->nVtx  &&  *limit > 0; k++) {
        v1 = order[k];

        if (match[v1] != v1)
            continue;
        
        n = 0; /* number of neighbors */
        /* for every hyperedge containing the vertex */
        for (i = hg->vindex[v1]; i < hg->vindex[v1+1]; i++) {
            edge = hg->vedge[i];

            if (hg->esize[edge] < hgp->MatchEdgeSizeThreshold) {
                /* for every other vertex in the hyperedge */
                for (j = hg->hindex[edge]; j < hg->hindex[edge+1]; j++) {
                    v2 = hg->hvertex[j];
                    if (match[v2] != v2) {
                        /* row swapping goes here */
                    }
                    else {
                        if (ips[v2]==0.0)
                            adj[n++] = v2;
                        ips[v2] += (hg->ewgt ? hg->ewgt[edge] : 1.0);
                    }
                }
            }
        }

        /* now choose the vector with greatest inner product */
        maxip = 0;
        maxindex = -1;
        for (i = 0; i < n; i++) {
            v2 = adj[i];
            if (ips[v2] > maxip && v2 != v1 && match[v2] == v2
                && MATCH_OK(hgp, hg, hg->pref_part[v1], hg->pref_part[v2])) {
                maxip = ips[v2];
                maxindex = v2;
            }
            ips[v2] = 0;
        }
        if (maxindex != -1) {
            match[v1] = maxindex;
            match[maxindex] = v1;
            (*limit)--;
        } 
        
        /*
          printf("Done with %d, best match is %d with product %d\n",
                  v1, maxindex, maxip);
         */
    }

    /*
      printf("Final Matching:\n");
      for(i = 0; i < hg->nVtx; i++)
          printf("%2d ",i);
      printf("\n");
      for(i = 0; i < hg->nVtx; i++)
          printf("%2d ",match[i]);
      printf("\n");
    */

    Zoltan_Multifree(__FILE__, __LINE__, 3, &ips, &adj, &order);
    return ZOLTAN_OK;
}


static int pmatching_local_ipm(
  ZZ *zz,
  HGraph *hg,
  Matching match,
  PHGPartParams *hgp
)
{
    int limit=hg->nVtx, err=ZOLTAN_OK;
    PHGComm *hgc=hg->comm;
    int root_matchcnt, root_rank;

    /* UVC: In order to simplify adding/testing old sequential matching codes easy
       I'm keeping the interface same; just move your favorite matching code to this file
       and change the function name in the next line */ 
    err = matching_ipm(zz, hg, hgp, match, &limit);
    
    /* UVC: again to simplify testing Optimization; I'm leaving the call here, just
     move your function and rename the call
     if (hgp->matching_opt) 
        err = hgp->matching_opt (zz, hg, match, &limit);
    */
    
    /* find the index of the proc in column group with the best match
       (max #matches); it will be our root proc */
    Zoltan_PHG_Find_Root(hg->nVtx-limit, hgc->myProc_y, hgc->col_comm,
                         &root_matchcnt, &root_rank);
    
    MPI_Bcast(match, hg->nVtx, MPI_INT, root_rank, hgc->col_comm);
    
    return err;
}


/**************************************************************************
  Alternating ipm method. Alternate between full ipm and fast method. 
 *************************************************************************/
static int pmatching_alt_ipm(
  ZZ *zz,
  HGraph* hg,
  Matching match,
  PHGPartParams *hgp
)
{
  int ierr = ZOLTAN_OK;
  char redm_orig[MAX_PARAM_STRING_LEN];

  strcpy(redm_orig, hgp->redm_str); /* save original parameter string */

  /* first level is 0 */
  if ((hg->info&1) == 0)  /* alternate even-odd levels */
    strcpy(hgp->redm_str, hgp->redm_fast); /* fast method is typically l-ipm */
  else
    strcpy(hgp->redm_str, "ipm");  

  Zoltan_PHG_Set_Matching_Fn (hgp);  /* set pointer to the desired matching function */
  ierr = hgp->matching (zz, hg, match, hgp);
  hgp->matching = pmatching_alt_ipm;  /* reset function pointer */

  /* set redm parameter back to original */
  strcpy(hgp->redm_str, redm_orig);
  
  return ierr;
}

/**************************************************************************
  Hybrid ipm method. First partial c-ipm, then full ipm on unmatched vtxs.
 *************************************************************************/
static int pmatching_hybrid_ipm(
  ZZ *zz,
  HGraph* hg,
  Matching match,
  PHGPartParams *hgp
)
{
  int ierr = ZOLTAN_OK;
  char redm_orig[MAX_PARAM_STRING_LEN];

  strcpy(redm_orig, hgp->redm_str); /* save original parameter ("h-ipm") */

  /* Set parameter for how good ip values to keep. */
  /* This could perhaps be a user parameter. */
  hgp->hybrid_keep_factor = 1.0;

  /* First do (partial) c-ipm. */
  strcpy(hgp->redm_str, "c-ipm");
  ierr = pmatching_ipm(zz, hg, match, hgp);  

  /* Reset hybrid_keep_factor to be safe. */
  hgp->hybrid_keep_factor = 0.0;

  /* Then do full ipm on remaining unmatched vertices. */
  strcpy(hgp->redm_str, "ipm");  
  ierr = pmatching_ipm(zz, hg, match, hgp);  

  /* Reset redm parameter back to original */
  strcpy(hgp->redm_str, redm_orig);
  
  return ierr;
}


/****************************************************************************
 * inner product matching (with user selectable column variant, c-ipm)
 * Parallelized version of the serial algorithm (see hg_match.c)
 * Based on conversations with Rob Bisseling by Aaron Becker, UIUC, summer 2004
 * completed by R. Heaphy. New features by Karen Devine & Erik Boman.
 */
               
 /* match[] is local slice of global matching array. Initially, match[i] = i.
  * After matching, match[i]=i indicates an unmatched vertex. A matching between
  * vertices i & j is indicated by match[i] = j & match[j] = i.  NOTE: Positive
  * integers are local numbering (zero-based).  A match to an off processor
  * vertex is indicated by a negative number, -(gno+1), and must use global
  * numbers (gno's) and not local numbers, lno's.                             */ 
 
#define ROUNDS_CONSTANT 8     /* controls the number of candidate vertices */ 
#define IPM_TAG        28731  /* MPI message tag, arbitrary value */
#define CONFLICT_TAG   IPM_TAG+1 
#define HEADER_SIZE    3          /* Phase 2 send buffer header size in ints */

/* these thresholds need to become parameters in phg - maybe ??? */
#define PSUM_THRESHOLD 0.0    /* ignore partial inner products < threshold */
#define TSUM_THRESHOLD 0.0    /* ignore total inner products < threshold */
                                 
/* Forward declaration for a routine that encapsulates the common calls to use
** the Zoltan unstructured communications library for the matching code */
static int communication_by_plan (ZZ* zz, int sendcnt, int* dest, int* size, 
 int scale, int* send, int* reccnt, int* recsize, int* nRec, int** rec,
 MPI_Comm comm, int tag);


/* Actual inner product calculations between candidates (in rec buffer)    */
/* and local vertices.  Not inlined because ARG is changing for each edge */
/* in inner loop and also uses hgp->MatchEdgeSizeThreshold */
#define INNER_PRODUCT1(ARG)\
  for (i = 0; i < count; r++, i++) {\
    if (((ARG)>0.0) && (hg->esize[*r] < hgp->MatchEdgeSizeThreshold)) {\
      for (j = hg->hindex[*r]; j < hg->hindex[*r + 1]; j++) {\
        if (cmatch[hg->hvertex[j]] == hg->hvertex[j])  {\
          if (sums[hg->hvertex[j]] == 0.0)\
            index[m++] = hg->hvertex[j];\
          sums[hg->hvertex[j]] += (ARG);\
        }\
      }\
    }\
  }

#define AGG_INNER_PRODUCT1(ARG)\
  for (i = 0; i < count; r++, i++) {\
    if (((ARG)>0.0) && (hg->esize[*r]<hgp->MatchEdgeSizeThreshold)) {\
      for (j = hg->hindex[*r]; j < hg->hindex[*r + 1]; j++) {\
        int v=lhead[hg->hvertex[j]];\
        if (sums[v] == 0.0)\
          aux[m++] = v;\
        sums[v] += (ARG);\
      }\
    }\
  }
  

/* Mostly identical inner product calculation to above for c-ipm variant. Here */
/* candidates are a subset of local vertices and are not in a separate buffer  */
#define INNER_PRODUCT2(ARG)\
   for (i = hg->vindex[candidate_gno]; i < hg->vindex[candidate_gno+1]; i++)  {\
     edge = hg->vedge[i];\
     if (((ARG)>0.0) && (hg->esize[edge]<hgp->MatchEdgeSizeThreshold)) {\
       for (j = hg->hindex[edge]; j < hg->hindex[edge+1]; j++)  {\
         if (cmatch[hg->hvertex[j]] == hg->hvertex[j])    {\
           if (sums[hg->hvertex[j]] == 0.0)\
             index[m++] = hg->hvertex[j];\
           sums[hg->hvertex[j]] += (ARG);\
         }\
       }\
     }\
   }  



/* simple macro to start timer */
#define MACRO_TIMER_START(arg, message, sync) \
  if (hgp->use_timers > 3)  {\
    if (timer->matchstage[arg] < (arg))\
      timer->matchstage[arg] = Zoltan_Timer_Init(zz->ZTime, sync, message);\
    ZOLTAN_TIMER_START(zz->ZTime, timer->matchstage[arg], hg->comm->Communicator);\
  }   

/* simple corresponding macro to stop timer */
#define MACRO_TIMER_STOP(arg) \
  if (hgp->use_timers > 3) \
    ZOLTAN_TIMER_STOP(zz->ZTime, timer->matchstage[arg], hg->comm->Communicator);

/* convenience macro to encapsulate resizing a buffer when necessary. Note: */
/* currently ZOLTAN_REALLOC aborts on any error and doesn't return - But... */
#define MACRO_REALLOC(new_size, old_size, buffer)  {\
  old_size = (new_size);\
  if (!(buffer = (int*) ZOLTAN_REALLOC (buffer, old_size * sizeof(int)))) \
    MEMORY_ERROR; \
  } 

/* instead of realloc; just free and alloc new one to avoid memcpy in realloc */
#define MACRO_RESIZE(new_size, old_size, buffer)  {\
  if ((new_size)>(old_size)) {\
    old_size = (new_size);\
    ZOLTAN_FREE(&buffer);\
    if (!(buffer = (int*) ZOLTAN_MALLOC (old_size * sizeof(int)))) \
        MEMORY_ERROR; \
    } \
} 
 


  
/****************************************************************************/
/* Because this calculation is done in two locations it has been converted to a
** subroutine to assure consistancy. Inline is not yet always available!
** ERIK: need to calculate nCandidates based on # of unmatched vertices     */
static int calc_nCandidates (int num_vtx, int procs)
{
  /* Constant 2 below because each match pairs 2 vertices */
  return num_vtx ? 1 + num_vtx/(2 * procs * ROUNDS_CONSTANT) : 0;
}
 

/****************************************************************************/
static int pmatching_ipm (ZZ *zz,
  HGraph* hg,
  Matching match,
  PHGPartParams *hgp)
{
  int i, j, k, n, m, round, vindex;                        /* loop counters  */
  int *r, *s;                                /* pointers to send/rec buffers */
  int lno, count, kstart, old_kstart;                      /* temp variables */
  int candidate_gno;                             /* gno of current candidate */
  int sendcnt, sendsize, reccnt, recsize, msgsize;         /* temp variables */
  int nRounds;                /* # of matching rounds to be performed;       */
                              /* identical on all procs in hgc->Communicator.*/
  int nCandidates;            /* # of candidates on this proc; identical     */
                              /* on all procs in hgc->col_comm.              */
  int total_nCandidates;      /* Sum of nCandidates across row. */
  int *send = NULL,    nSend,             /* working buffers and their sizes */
      *dest = NULL,    nDest,  
      *size = NULL,    nSize,
      *rec = NULL,     nRec,
      *index = NULL,   nIndex,
      *edgebuf = NULL, nEdgebuf,  /* holds candidates for processing (ipm)   */
      *aux = NULL;
  int *visit = NULL,       /* candidate visit order (all candidates) */
      *cmatch = NULL,      /* working copy of match array */
      *select = NULL,      /* current selected candidates (this round) */
      *permute = NULL;    /* reorder of candidates after global communication */
  float bestsum;      /* holds current best inner product */
  float *sums = NULL, /* holds candidate's inner products with each local vtx */
        *f = NULL;    /* used to stuff floating value into integer message */
  PHGComm *hgc = hg->comm;
  int ierr = ZOLTAN_OK;
  int max_nPins, max_nVtx;       /* Global max # pins/proc and vtx/proc */
  int *rows = NULL;              /* used only in merging process */
  int bestlno, partner_gno, edge;
  Triplet *master_data = NULL, *global_best = NULL;
  int *master_procs = NULL;
  int cFLAG;                    /* if set, do only a column matching, c-ipm */
  MPI_Op phasethreeop;
  MPI_Datatype phasethreetype;
  int candidate_index, first_candidate_index;
  int pref, num_matches_considered = 0;
  double ipsum = 0.;
  struct phg_timer_indices *timer = zz->LB.Data_Structure;
  char *yo = "pmatching_ipm";
  
   
  ZOLTAN_TRACE_ENTER (zz, yo);
  MACRO_TIMER_START (0, "matching setup", 0);
  Zoltan_Srand_Sync (Zoltan_Rand(NULL), &(hgc->RNGState_col), hgc->col_comm);
     
  /* this restriction may be removed later, but for now NOTE this test */
  if (sizeof(int) < sizeof(float))  {
    ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Code must be modified before using");
    ierr = ZOLTAN_FATAL;
    goto End;
  }

  /* set a flag if user wants a column matching or a full matching */
  cFLAG = strcasecmp (hgp->redm_str, "c-ipm") ? 0 : 1;
  if (!cFLAG) {
    MPI_Type_contiguous (sizeof(Triplet), MPI_CHAR, &phasethreetype);
    MPI_Type_commit (&phasethreetype);
    MPI_Op_create (&phasethreereduce, 1, &phasethreeop);
  }

  /* determine basic working parameters */
  nRounds     = cFLAG ? ROUNDS_CONSTANT : hgc->nProc_x * ROUNDS_CONSTANT;
  nCandidates = calc_nCandidates (hg->nVtx, cFLAG ? 1 : hgc->nProc_x); 
    
  /* determine maximum number of Vtx and Pins for storage allocation */
  /* determine initial sum of all candidates = total_nCandidates==>allocation */
  if (cFLAG)  {
    max_nVtx          = hg->nVtx;
    max_nPins         = hg->nPins;
    total_nCandidates = nCandidates;    
  }
  else  {
    MPI_Allreduce(&hg->nPins, &max_nPins, 1, MPI_INT,MPI_MAX,hgc->Communicator);
    max_nVtx = total_nCandidates = 0;
    for (i = 0; i < hgc->nProc_x; i++)  {
      count = hg->dist_x[i+1] - hg->dist_x[i]; /* number of vertices on proc i */
      if (count > max_nVtx)
        max_nVtx = count;
      if (i == hgc->myProc_x)
        first_candidate_index = total_nCandidates;
      total_nCandidates += calc_nCandidates (count, hgc->nProc_x);
    }
  }
                 
  /* allocate "complicated" fixed-sized array storage */
  msgsize = MAX (total_nCandidates, max_nVtx);
  nIndex = 1 + MAX (msgsize, MAX (hgc->nProc_x, hgc->nProc_y));
  nDest  = nIndex;
  nSize  = nIndex;

  /* These 3 buffers are REALLOC'd iff necessary; this should be very rare  */
  nSend    = max_nPins;   /* nSend/nEdgebuf are used for candidate exchange */
  nRec     = max_nPins;   /* nSend/nRec for all other paired communication */
  nEdgebuf = max_nPins;   /* <candidate_gno, candidate_index, #pins, <pins>> */

  if (hg->nVtx)  
    if (!(cmatch = (int*)   ZOLTAN_MALLOC (hg->nVtx * sizeof(int)))
     || !(visit  = (int*)   ZOLTAN_MALLOC (hg->nVtx * sizeof(int)))
     || !(aux    = (int*)   ZOLTAN_MALLOC (hg->nVtx * sizeof(int)))     
     || !(sums   = (float*) ZOLTAN_CALLOC (hg->nVtx,  sizeof(float))))
        MEMORY_ERROR;

  if (!cFLAG && total_nCandidates && (hgc->myProc_y == 0)) {  /* Master row */
    i = total_nCandidates * sizeof(Triplet);  
    if (!(master_data = (Triplet*) ZOLTAN_MALLOC(i))
     || !(global_best = (Triplet*) ZOLTAN_MALLOC(i)))
        MEMORY_ERROR;
    for (i = 0; i < total_nCandidates; i++) {
      master_data[i].candidate = -1;
      master_data[i].partner   = -1;
      master_data[i].ip        = -1.0;
    }
  } 

  if (!cFLAG)
    if (!(edgebuf = (int*) ZOLTAN_MALLOC (nEdgebuf * sizeof(int))))
        MEMORY_ERROR;
  
  if (!(dest    = (int*) ZOLTAN_MALLOC (nDest              * sizeof(int)))
   || !(size    = (int*) ZOLTAN_MALLOC (nSize              * sizeof(int)))
   || !(index   = (int*) ZOLTAN_MALLOC (nIndex             * sizeof(int)))
   || !(rows    = (int*) ZOLTAN_MALLOC ((hgc->nProc_y + 1) * sizeof(int)))
   || (nSend && !(send = (int*) ZOLTAN_MALLOC (nSend * sizeof(int))))
   || (nRec  && !(rec  = (int*) ZOLTAN_MALLOC (nRec  * sizeof(int))))
   || (total_nCandidates && !(permute = (int*) ZOLTAN_MALLOC (total_nCandidates
   * sizeof(int))))
   || (total_nCandidates && !(select = (int*) ZOLTAN_MALLOC (total_nCandidates
   * sizeof(int)))))
      MEMORY_ERROR;
  
  /* Compute candidates' vertex visit order (selection). Random is default. */
  Zoltan_PHG_Vertex_Visit_Order (zz, hg, hgp, visit);
  
  /* Loop processing ncandidates vertices per column each round.
   * Each loop has 3 phases, phase 3 may be repeated as necessary
   * Phase 1: send ncandidates vertices for global matching - horizontal comm
   * Phase 2: sum  inner products, find best in column - vertical communication
   * Phase 3: return best match selections        - horizontal communication
   *
   * No conflict resolution is required since temp locking prevents conflicts. */

  MACRO_TIMER_STOP (0);
  vindex = 0;                        /* marks current position in visit array */
  for (round = 0; round < nRounds; round++) {
    MACRO_TIMER_START (1, "matching phase 1", 0);
    
    /************************ PHASE 1: ****************************************/
    
    memcpy (cmatch, match, hg->nVtx * sizeof(int));  /* for temporary locking */    
    if (cFLAG)  {
      int nTotal;
      /* select upto nCandidates unmatched vertices to locally match */
      for (i = 0; i < total_nCandidates; i++)
        permute[i] = -1;                       /* to flag missing candidates  */
        
      for (nTotal = 0; nTotal < nCandidates && vindex < hg->nVtx; vindex++)
        if (cmatch [visit[vindex]] == visit[vindex])  {         /* unmatched */
          permute [nTotal++] = visit[vindex];    /* select it as a candidate */
          cmatch [visit[vindex]] = -visit[vindex]-1;  /* mark as pending match */
        }
      total_nCandidates = nTotal;
      for (i = 0; i < total_nCandidates; i++)
        select[i] = i;      
    }
    else  {       
      /* Select upto nCandidates unmatched vertices to globally match. */
      for (sendcnt = 0; sendcnt < nCandidates && vindex < hg->nVtx; vindex++)
        if (cmatch[visit[vindex]] == visit[vindex])  {         /* unmatched */
          select [sendcnt++] = visit[vindex];    /* select it as a candidate */
          cmatch [visit[vindex]] = -visit[vindex]-1;  /* mark as pending match */
        }
                        
      /* assure send buffer is large enough by first computing required size */
      sendsize = 0;
      for (i = 0; i < sendcnt; i++)  {
        lno = select[i];
        if (hg->vindex[lno+1] > hg->vindex[lno])
            sendsize += hg->vindex[lno+1] - hg->vindex[lno] + HEADER_SIZE 
                + (hgp->UsePrefPart ? 1 : 0); 
      }
      if (sendsize > nSend)
        MACRO_REALLOC (1.2 * sendsize, nSend, send);    /* resize send buffer */    
      /* put <candidate_gno, candidate_index, count, <edge>> into send buffer */
      s = send;
      for (i = 0; i < sendcnt; i++)   {
        lno = select[i];
        if (hg->vindex[lno+1] > hg->vindex[lno]) {
          *s++ = VTX_LNO_TO_GNO(hg, lno);                  /* gno of candidate */
          *s++ = i + first_candidate_index;                 /* candidate index */
          if (hgp->UsePrefPart)          
              *s++ = hg->pref_part[lno];                /* pref partition info */
          *s++ = hg->vindex[lno+1] - hg->vindex[lno];            /* edge count */
          for (j = hg->vindex[lno]; j < hg->vindex[lno+1]; j++)  
            *s++ = hg->vedge[j];                                   /* edge lno */
        }
      }
      sendsize = s - send;
         
      /* communication to determine global size of rec buffer */
      MPI_Allgather (&sendsize, 1, MPI_INT, size, 1, MPI_INT, hgc->row_comm);
     
      /* determine size of the rec buffer & reallocate bigger iff necessary */
      recsize = 0;
      for (i = 0; i < hgc->nProc_x; i++)
        recsize += size[i];          /* compute total size of edgebuf in ints */
      if (recsize > nEdgebuf)
        MACRO_REALLOC (1.2 * recsize, nEdgebuf, edgebuf);  /* enlarge edgebuf */
    
      /* setup displacement array necessary for MPI_Allgatherv */
      dest[0] = 0;
      for (i = 1; i < hgc->nProc_x; i++)
        dest[i] = dest[i-1] + size[i-1];

      /* communicate vertices & their edges to all row neighbors */
      MPI_Allgatherv(send, sendsize, MPI_INT, edgebuf, size, dest, MPI_INT,
       hgc->row_comm);
         
      /* Communication has grouped candidates by processor, rescramble!     */
      /* Otherwise all candidates from proc column 0 will be matched first, */
      for (i = 0; i < total_nCandidates; i++)
        select[i] = i;
      Zoltan_Rand_Perm_Int (select, total_nCandidates, &(hgc->RNGState_col));
      
      for (i = 0; i < total_nCandidates; i++)
        permute[i] = -1;                 /* to flag missing sparse candidates */
      for (i = 0 ; i < recsize; i += count)   {
        int indx        = i++;              /* position of next gno in edgebuf */
        candidate_index = edgebuf[i++];
        if (hgp->UsePrefPart)        
            pref        = edgebuf[i++];   /* skip over pref vertex information */
        count           = edgebuf[i++];     /* count of edges */      
        permute[candidate_index] = indx ;   /* save position of gno in edgebuf */
      }
    }            /* DONE:  if (cFLAG) else ...  */                          
    MACRO_TIMER_STOP (1);
    
    /************************ PHASE 2: ***************************************/
      
    /* for each candidate vertex, compute all local partial inner products */    

    kstart = old_kstart = 0;         /* next candidate (of nTotal) to process */
    while (kstart < total_nCandidates) {
      MACRO_TIMER_START (2, "Matching kstart A", 0);
      for (i = 0; i < hgc->nProc_y; i++)
        rows[i] = -1;                  /* to flag data not found for that row */
      sendsize = 0;                    /* position in send buffer */
      sendcnt  = 0;                    /* count of messages in send buffer */
      s        = send;                 /* start at send buffer origin */
      for (k = kstart; k < total_nCandidates; k++)  {
        if (permute[select[k]] == -1) 
          continue;                /* don't have this sparse candidate locally */
        
        if (!cFLAG)  {
          r = &edgebuf[permute[select[k]]];
          candidate_gno   = *r++;          /* gno of candidate vertex */
          candidate_index = *r++;          /* candidate_index of vertex */
          if (hgp->UsePrefPart)          
              pref = *r++;                 /* pref vertex information */          
          count           = *r++;          /* count of following hyperedges */
        }
        else  {
          candidate_index = k;
          candidate_gno   = permute[k];  /* need to use next local vertex */
          if (hgp->UsePrefPart)          
              pref = hg->pref_part[candidate_gno];          
        }                          /* here candidate_gno is really a local id */
                  
        /* now compute the row's nVtx inner products for kth candidate */
        m = 0;
        if (!cFLAG) {
          if ((hg->ewgt != NULL) && (hgp->vtx_scal == NULL))
            INNER_PRODUCT1(hg->ewgt[*r])
          else if ((hg->ewgt == NULL) && (hgp->vtx_scal == NULL))
            INNER_PRODUCT1(1.0)
          else if ((hg->ewgt != NULL) && (hgp->vtx_scal != NULL))
            INNER_PRODUCT1(hgp->vtx_scal[hg->hvertex[j]] * hg->ewgt[*r])
          else /* UVC: no need: if ((hg->ewgt == NULL) && (hgp->vtx_scal != NULL)) */
            INNER_PRODUCT1(hgp->vtx_scal[hg->hvertex[j]])
        } else   {                                            /* cFLAG */
          if      ((hg->ewgt == NULL) && (hgp->vtx_scal == NULL))
            INNER_PRODUCT2(1.0)
          else if ((hg->ewgt == NULL) && (hgp->vtx_scal != NULL))
            INNER_PRODUCT2(hgp->vtx_scal[hg->hvertex[j]])
          else if ((hg->ewgt != NULL) && (hgp->vtx_scal == NULL))
            INNER_PRODUCT2(hg->ewgt[edge])
          else if ((hg->ewgt != NULL) && (hgp->vtx_scal != NULL))
            INNER_PRODUCT2(hgp->vtx_scal[hg->hvertex[j]] * hg->ewgt[edge])
        }
          
        /* if local vtx, remove self inner product (useless maximum) */
        if (cFLAG)
          sums[candidate_gno] = 0.0;     /* since candidate_gno is really lno */
        else if (VTX_TO_PROC_X(hg, candidate_gno) == hgc->myProc_x)
          sums[VTX_GNO_TO_LNO(hg, candidate_gno)] = 0.0;
         
        /* count partial sums exceeding PSUM_THRESHOLD */   
        count = 0;
        for (i = 0; i < m; i++)  {
          lno = index[i];
          if (sums[lno] > PSUM_THRESHOLD
              && MATCH_OK(hgp, hg, hg->pref_part[lno], pref))
            aux[count++] = lno;      /* save lno for significant partial sum */
          else
            sums[lno] = 0.0;         /* clear unwanted entries */  
        }     
        if (count == 0)
          continue;         /* no partial sums to append to message */       

        /* iff necessary, resize send buffer to fit at least first message */
        msgsize = HEADER_SIZE + 2 * count;
        if (sendcnt == 0 && (msgsize > nSend)) {
          MACRO_REALLOC (1.2 * msgsize, nSend, send);  /* increase buffer size */
          s = send;         
        }
        
        /* message is <candidate_gno, candidate_index, count, <lno, psum>> */
        if (sendsize + msgsize <= nSend)  {
          /* flag first data in each destination row for merging */
          if (rows[candidate_gno % hgc->nProc_y] != 1)  {
            rows[candidate_gno % hgc->nProc_y] = 1;
            candidate_index = -candidate_index -1;
          }
          
          /* current partial sums fit, so put them into the send buffer */
          dest[sendcnt]   = candidate_gno % hgc->nProc_y;    /* destination */
          size[sendcnt++] = msgsize;          /* size of message */
          sendsize       += msgsize;          /* cummulative size of message */
          *s++ = candidate_gno;
          *s++ = candidate_index;        
          *s++ = count;
          for (i = 0; i < count; i++)  {          
            *s++ = aux[i];                          /* lno of partial sum */
             f = (float*) s++;
            *f = sums[aux[i]];                      /* partial sum */           
            sums[aux[i]] = 0.0;
          }          
        }
        else  {           /* psum message doesn't fit into buffer */
          for (i = 0; i < count; i++)              
            sums[aux[i]] = 0.0;
          break;   
        }  
      }                  /* DONE: loop over k */                    
    
      MACRO_TIMER_STOP (2);
      MACRO_TIMER_START (3, "Matching kstart B", 0);
    
      /* synchronize all rows in this column to next kstart value */
      old_kstart = kstart;
      MPI_Allreduce (&k, &kstart, 1, MPI_INT, MPI_MIN, hgc->col_comm);

      /* Send inner product data in send buffer to appropriate rows */
      ierr = communication_by_plan (zz, sendcnt, dest, size, 1, send, &reccnt, 
       &recsize, &nRec, &rec, hgc->col_comm, IPM_TAG);

      
      if (ierr != ZOLTAN_OK)
        goto End;
    
      /* build index into receive buffer pointer for each proc's row of data */
      for (i = 0; i < hgc->nProc_y; i++)
        rows[i] = recsize;       /* sentinal in case no row's data available */
      k = 0;
      for (r = rec; r < rec+recsize; r+=(HEADER_SIZE+2*(*(r+2)))) {
        if (*(r+1) < 0)  {
          *(r+1) = -(*(r+1)) - 1;           /* make sentinal candidate_index positive */
          rows[k++] = (r - rec);  /* points to gno */
        }
      }

    
      /* merge partial i.p. sum data to compute total inner products */
      s = send; 
      for (n = old_kstart; n < kstart; n++) {
        m = 0;       
        for (i = 0; i < hgc->nProc_y; i++) 
          if (rows[i] < recsize && rec [rows[i]+1] == select[n])  {
            candidate_gno   = rec [rows[i]++];
            candidate_index = rec [rows[i]++];
            count           = rec [rows[i]++];
            for (j = 0; j < count; j++)  {
              lno = rec [rows[i]++];                    
              if (sums[lno] == 0.0)       /* is this first time for this lno? */
                aux[m++] = lno;           /* then save the lno */
              f = (float*) (rec + rows[i]++);        
              sums[lno] += *f;            /* sum the psums */
            }
          }

        /* determine how many total inner products exceed threshold */
        count = 0;
        for (i = 0; i < m; i++)
          if (sums[aux[i]] > TSUM_THRESHOLD)
            count++;

        /* Put <candidate_gno, candidate_index, count, <lno, tsum>> into send */
        if (count > 0)  {
          if (s - send + HEADER_SIZE + 2 * count > nSend ) {
            sendsize = s - send;
            MACRO_REALLOC (1.2*(sendsize + HEADER_SIZE + 2*count), nSend, send);
            s = send + sendsize;         /* since realloc buffer could move */
          }      
          *s++ = candidate_gno;
          *s++ = candidate_index;
          *s++ = count;
        }  
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
      sendsize = s - send;   /* size (in ints) of send buffer */
    
      /* Communicate total inner product results to MASTER ROW */
      MPI_Gather(&sendsize, 1, MPI_INT, size, 1, MPI_INT, 0, hgc->col_comm);
    
      if (hgc->myProc_y == 0) {
        recsize = 0;
        for (i = 0; i < hgc->nProc_y; i++)
          recsize += size[i];        
        
        dest[0] = 0;
        for (i = 1; i < hgc->nProc_y; i++)
          dest[i] = dest[i-1] + size[i-1];
        
        if (recsize > nRec)
          MACRO_REALLOC (1.2 * recsize, nRec, rec);      /* make rec buffer bigger */
      }

      MPI_Gatherv (send, sendsize, MPI_INT, rec, size, dest, MPI_INT, 0,
                   hgc->col_comm);
       
      /* Determine best vertex and best sum for each candidate */
      if (hgc->myProc_y == 0) {   /* do following only if I am the MASTER ROW */
        for (r = rec; r < rec + recsize;)  {
          candidate_gno   = *r++;
          candidate_index = *r++;
          count           = *r++;                    /* count of nonzero pairs */
          bestsum = -1.0;                        /* any negative value will do */
          bestlno = -1;                          /* any negative value will do */
          for (i = 0; i < count; i++)  {
            lno =          *r++;
            f   =  (float*) r++;     
            if ((*f > bestsum) && cmatch[lno] == lno)  { 
              bestsum = *f;
              bestlno = lno;
            }      
          }
         
          /* For hybrid ipm, keep matches that are above average in c-ipm */
          if (bestsum>0){
             /* ipsum is cumulative sum of best inner products (bestsum) */
             ipsum += bestsum;
             num_matches_considered++;        
          }
          if (cFLAG && bestsum > MAX(TSUM_THRESHOLD, 
              hgp->hybrid_keep_factor*ipsum/num_matches_considered))  {            
            cmatch[bestlno] = -1;                   
            match[bestlno]       = candidate_gno;
            match[candidate_gno] = bestlno;
          }
                        
          if (!cFLAG && bestsum > TSUM_THRESHOLD)  {
            cmatch[bestlno] = -1;  /* mark pending match to avoid conflicts */
            master_data[candidate_index].candidate = candidate_gno;
            master_data[candidate_index].partner = VTX_LNO_TO_GNO (hg, bestlno);
            master_data[candidate_index].ip = bestsum;
          }
        }
      } 
      MACRO_TIMER_STOP (3);    
    }            /* DONE: kstart < max_nTotal loop */

    if (cFLAG)  {
      MPI_Bcast (match, hg->nVtx, MPI_INT, 0, hgc->col_comm);          
      continue;      /* skip phases 3 and 4, continue rounds */ 
    }    

    /************************ NEW PHASE 3: ********************************/
    
    MACRO_TIMER_START (4, "Matching Phase 3", 1);
    
    /* Only MASTER ROW computes best global match for candidates */
    /* EBEB or perhaps we can do this fully distributed? */
    if (hgc->myProc_y == 0) {
      HG_Ptr = hg;
      MPI_Allreduce(master_data, global_best, total_nCandidates, phasethreetype,
                    phasethreeop, hgc->row_comm);

      /* Look through array of "winners" and update match array. */
      /* Local numbers are used for local matches, otherwise
         -(gno+1) is used in the match array.                    */
      for (i = 0; i < total_nCandidates; i++) {
        int cproc, vproc;
        candidate_gno = global_best[i].candidate;

        /* Reinitialize master_data for next round */
        master_data[i].candidate = -1;
        master_data[i].partner   = -1;
        master_data[i].ip        = -1.0;
        if (candidate_gno == -1)
          continue;

        partner_gno = global_best[i].partner;
        cproc = VTX_TO_PROC_X(hg, candidate_gno);
        vproc = VTX_TO_PROC_X(hg, partner_gno);
        if (cproc == hgc->myProc_x) {
          if (vproc == hgc->myProc_x)   {
            int v1 = VTX_GNO_TO_LNO(hg, partner_gno);
            int v2 = VTX_GNO_TO_LNO(hg, candidate_gno);
            match[v1] = v2;
            match[v2] = v1;
          }
          else 
            match[VTX_GNO_TO_LNO(hg, candidate_gno)] = -partner_gno - 1;
        }                         
        else if (vproc == hgc->myProc_x)
          match[VTX_GNO_TO_LNO(hg, partner_gno)] = -candidate_gno - 1;
      }
    } /* End (hgc->myProc_y == 0) */

    /* broadcast match array to the entire column */
    MPI_Bcast (match, hg->nVtx, MPI_INT, 0, hgc->col_comm);
    MACRO_TIMER_STOP (4);                       /* end of phase 3 */
  }                                             /* DONE: loop over rounds */
  
  MACRO_TIMER_START (6, "Matching Cleanup", 0);

  /* optional sanity tests */
  if (zz->Debug_Level > 4 && hgc->myProc_x == 0 && hgc->myProc_y == 0)  {
    int local = 0, global = 0, unmatched = 0;
    for (i = 0; i < hg->nVtx; i++)  {
      if      (match[i] == i)  unmatched++;
      else if (match[i] < 0)   global++;
      else                     local++;
    }
    uprintf (hgc, "%d RTHRTH %d unmatched, %d external, %d local of %d\n",
     hg->info, unmatched, global, local, hg->nVtx);
  }

  if (zz->Debug_Level > 4 && hgc->myProc_x==0 && hgc->myProc_y==0)
    fprintf (stdout, "RTHRTH rounds %d\n", nRounds);

  if (zz->Debug_Level > 4)  {
    /* The following tests that the global match array is a valid permutation */
    /* NOTE:  THESE TESTS ARE NOT MANDATORY; THEY CAN BE EXCLUDED AFTER WE    */
    /* COMPLETE TESTING OF matching_ipm.                                      */

    for (i = 0; i < hg->nVtx; i++)
      if (match[i] < 0)  cmatch[i] = -match[i] - 1;
      else               cmatch[i] = VTX_LNO_TO_GNO(hg, match[i]);

    MPI_Allgather (&hg->nVtx, 1, MPI_INT, size, 1, MPI_INT, hgc->row_comm); 

    recsize = 0;
    for (i = 0; i < hgc->nProc_x; i++)
      recsize += size[i];
  
    dest[0] = 0;
    for (i = 1; i < hgc->nProc_x; i++)
      dest[i] = dest[i-1] + size[i-1];
  
    if (nRec < recsize)
      MACRO_REALLOC (recsize, nRec, rec); /* make rec buffer bigger */
    MPI_Allgatherv (cmatch, hg->nVtx, MPI_INT, rec, size, dest, MPI_INT,
     hgc->row_comm);

    if (nSend < recsize)
      MACRO_REALLOC (recsize, nSend, send);  /* make send buffer bigger */
  
    for (i = 0; i < recsize; i++)
      send[i] = 0;
    for (i = 0; i < recsize; i++)
      ++send[rec[i]];

    count = 0;
    for (i = 0; i < recsize; i++)
      if (send[i] != 1)
        count++;
    if (count)    
      uprintf (hgc, "RTHRTH %d FINAL MATCH ERRORS of %d\n", count, recsize); 
  }
  MACRO_TIMER_STOP (6);

End:
  if (!cFLAG) {
    MPI_Op_free(&phasethreeop);
    MPI_Type_free(&phasethreetype);
    ZOLTAN_FREE(&global_best);
  }
  
  Zoltan_Multifree (__FILE__, __LINE__, 15, &cmatch, &visit, &sums, &send,
   &dest, &size, &rec, &index, &aux, &permute, &edgebuf, &select, &rows,
   &master_data, &master_procs);
  ZOLTAN_TRACE_EXIT(zz, yo);
  return ierr;
}


  
/****************************************************************************/
static int communication_by_plan (ZZ* zz, int sendcnt, int* dest, int* size, 
 int scale, int* send, int* reccnt, int *recsize, int* nRec, int** rec,
 MPI_Comm comm, int tag)
{
   ZOLTAN_COMM_OBJ *plan = NULL;
   int err;
   char *yo = "communication_by_plan";
   
   /* communicate send buffer messages to other row/columns in my comm */  
   err = Zoltan_Comm_Create (&plan, sendcnt, dest, comm, tag, reccnt);
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
   else
     *recsize = *reccnt * scale;
   
   /* realloc rec buffer if necessary */  
   if (*recsize > *nRec)  {   
     *nRec = *recsize;
     if (!(*rec = (int*) ZOLTAN_REALLOC (*rec, *nRec * sizeof(int))))  {
       ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Memory error");
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




/****************************************************************************/
static int pmatching_agg_ipm (ZZ *zz,
                              HGraph* hg,
                              Matching match,
                              PHGPartParams *hgp)
{
  int ierr = ZOLTAN_OK;    
  int i, j, k, n, m, round, vindex;                        /* loop counters  */
  int *r, *s;                                /* pointers to send/rec buffers */
  int lno, count, kstart, old_kstart;                      /* temp variables */
  int candidate_gno;                             /* gno of current candidate */
  int sendcnt, sendsize, reccnt=0, recsize, msgsize;       /* temp variables */
  int nRounds;                /* # of matching rounds to be performed;       */
  /* identical on all procs in hgc->Communicator.*/
  int nCandidates;            /* # of candidates on this proc; identical     */
  /* on all procs in hgc->col_comm.              */
  int total_nCandidates;      /* Sum of nCandidates across row. */
  int *send = NULL,    nSend,             /* working buffers and their sizes */
    *dest = NULL,    nDest,  
    *size = NULL,    nSize,
    *rec = NULL,     nRec,
    *aux = NULL,   
    *edgebuf = NULL, nEdgebuf;  /* holds candidates for processing (ipm)   */
  char *visited = NULL;
  int *visit = NULL,       /* candidate visit order (all candidates) */
    *lhead = NULL,       /* to accumulate ipm values correctly */
    *lheadpref = NULL,
    *locCandidates = NULL, locCandCnt,      /* current selected candidates (this round) & number */
    *candvisit=NULL,   /* to randomize visit order of candidates*/
    *idxptr = NULL;    /* reorder of candidates after global communication */
  float bestsum;      /* holds current best inner product */
  float *sums = NULL, /* holds candidate's inner products with each local vtx */
    *cw = NULL,   /* current vertex weight */
    *tw=NULL, *maxw = NULL, *candw = NULL,
    *f = NULL;    /* used to stuff floating value into integer message */
  PHGComm *hgc = hg->comm;
  int max_nPins, max_nVtx;       /* Global max # pins/proc and vtx/proc */
  int *rows = NULL;              /* used only in merging process */
  int bestlno, partner_gno;
  Triplet *master_data = NULL, *global_best = NULL;
  int *master_procs = NULL;
  MPI_Op phasethreeop;
  MPI_Datatype phasethreetype;
  int candidate_index, *candIdx;
  int VtxDim = (hg->VtxWeightDim>0) ? hg->VtxWeightDim : 1;
  int pref;
  int replycnt;
  struct phg_timer_indices *timer = zz->LB.Data_Structure;
  char *yo = "pmatching_agg_ipm";
  KVHash hash;
  
  
  ZOLTAN_TRACE_ENTER (zz, yo);
  MACRO_TIMER_START (0, "matching setup", 0);
  Zoltan_Srand_Sync (Zoltan_Rand(NULL), &(hgc->RNGState_col), hgc->col_comm);
  
  /* this restriction may be removed later, but for now NOTE this test */
  if (sizeof(int) < sizeof(float))  {
    ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Code must be modified before using");
    ierr = ZOLTAN_FATAL;
    goto End;
  }
  
  
  MPI_Type_contiguous (sizeof(Triplet), MPI_CHAR, &phasethreetype);
  MPI_Type_commit (&phasethreetype);
  MPI_Op_create (&phasethreereduce, 1, &phasethreeop);
  
  /* compute nCandidates per process;
     unless hypergraph is too small use user given parameter
     otherwise use nVtx/3 to allow some matching
  */
  nCandidates = MAX(1, MIN(hgp->nCand, hg->nVtx/3));
  if (!(candIdx = (int*) ZOLTAN_MALLOC ((1+hgc->nProc_x) * sizeof(int)))) 
    MEMORY_ERROR;

    
  /* determine maximum number of Vtx and Pins for storage allocation */
  /* determine initial sum of all candidates = total_nCandidates==>allocation */
  MPI_Allreduce(&hg->nPins, &max_nPins, 1, MPI_INT,MPI_MAX,hgc->Communicator);
  max_nVtx = total_nCandidates = 0;
  for (i = 0; i < hgc->nProc_x; i++)  {
    count = hg->dist_x[i+1] - hg->dist_x[i]; /* number of vertices on proc i */
    if (count > max_nVtx)
      max_nVtx = count;
    candIdx[i] = total_nCandidates;
    total_nCandidates += MIN(hgp->nCand, count);
  }
  candIdx[i] = total_nCandidates;

                 
  /* allocate "complicated" fixed-sized array storage */
  msgsize = MAX (total_nCandidates, max_nVtx);
  nSize = nDest = 1 + MAX (msgsize, MAX (hgc->nProc_x, hgc->nProc_y));

  max_nPins += total_nCandidates * (1+HEADER_SIZE);

  /* These 3 buffers are REALLOC'd iff necessary; this should be very rare  */
  nSend    = max_nPins;   /* nSend/nEdgebuf are used for candidate exchange */
  nRec     = max_nPins;   /* nSend/nRec for all other paired communication */
  nEdgebuf = max_nPins;   /* <candidate_gno, candidate_index, #pins, <pins>> */

    
  if (hg->nVtx)  
    if (!(lhead  = (int*)   ZOLTAN_MALLOC (hg->nVtx * sizeof(int)))
        || (hgp->UsePrefPart && !(lheadpref = (int*)   ZOLTAN_MALLOC (hg->nVtx * sizeof(int))))
        || !(visited= (char*)  ZOLTAN_MALLOC (hg->nVtx * sizeof(char)))
        || !(cw     = (float*) ZOLTAN_MALLOC (VtxDim * hg->nVtx * sizeof(float)))
        || !(visit  = (int*)   ZOLTAN_MALLOC (hg->nVtx * sizeof(int)))
        || !(aux    = (int*)   ZOLTAN_MALLOC (hg->nVtx * sizeof(int)))     
        || !(sums   = (float*) ZOLTAN_CALLOC (hg->nVtx,  sizeof(float))))
      MEMORY_ERROR;

  if (VtxDim) 
    if (!(tw = (float*) ZOLTAN_MALLOC (VtxDim * sizeof(float)))
        || !(maxw = (float*) ZOLTAN_MALLOC (VtxDim * sizeof(float))))
      MEMORY_ERROR;

  if (hgc->myProc_y==0 && total_nCandidates ) {  
    i = total_nCandidates * sizeof(Triplet);  
    if (!(master_data = (Triplet*) ZOLTAN_MALLOC(i))
        || !(global_best = (Triplet*) ZOLTAN_MALLOC(i)))
      MEMORY_ERROR;
    for (i = 0; i < total_nCandidates; i++) {
      master_data[i].candidate = -1;
      master_data[i].partner   = -1;
      master_data[i].ip        = -1.0;
    }
  } 

  
  if (!(edgebuf = (int*) ZOLTAN_MALLOC (nEdgebuf * sizeof(int)))
      || !(dest    = (int*) ZOLTAN_MALLOC (nDest              * sizeof(int)))
      || !(size    = (int*) ZOLTAN_MALLOC (nSize              * sizeof(int)))
      || !(rows    = (int*) ZOLTAN_MALLOC ((hgc->nProc_y + 1) * sizeof(int)))
      || (nSend && !(send = (int*) ZOLTAN_MALLOC (nSend * sizeof(int))))
      || (nRec  && !(rec  = (int*) ZOLTAN_MALLOC (nRec  * sizeof(int)))))
    MEMORY_ERROR;
  if (total_nCandidates) {
    if (!(idxptr   = (int*)   ZOLTAN_MALLOC (total_nCandidates * sizeof(int)))
        || !(candvisit = (int*)   ZOLTAN_MALLOC (total_nCandidates * sizeof(int)))                
        || !(locCandidates = (int*)   ZOLTAN_MALLOC (nCandidates * sizeof(int)))
        || !(candw  = (float*) ZOLTAN_MALLOC (VtxDim * total_nCandidates * sizeof(float))))
      MEMORY_ERROR;
    if (hgc->myProc_y==0) {
        int hsize;
        if ((Zoltan_GenPrime(2*(1+hg->nVtx), &hsize)==ZOLTAN_MEMERR)
            || (Zoltan_KVHash_Create(&hash, hsize)==ZOLTAN_MEMERR))
            MEMORY_ERROR;
    }
  }
    
/* Compute candidates' vertex visit order (selection). Random is default. */
  Zoltan_PHG_Vertex_Visit_Order (zz, hg, hgp, visit);
  
/* Loop processing ncandidates vertices per column each round.
 * Each loop has 3 phases, phase 3 may be repeated as necessary
 * Phase 1: send ncandidates vertices for global matching - horizontal comm
 * Phase 2: sum  inner products, find best in column - vertical communication
 * Phase 3: return best match selections        - horizontal communication
 * Phase 4: finalize decision/conflict resolution */

  MACRO_TIMER_STOP (0);
  for (i=0; i< hg->nVtx; ++i) {
    visited[i] = 0;
    lhead[i] = i;
    if (hgp->UsePrefPart) lheadpref[i] = hg->pref_part[i];
    match[i] = VTX_LNO_TO_GNO(hg, i);
  }
  for (i=0; i<VtxDim; ++i)
    tw[i] = 0.0;
  if (hg->vwgt) {
    for (i=0; i< hg->nVtx; ++i)
      for (j=0; j<VtxDim; ++j)
        tw[j] += (cw[i*VtxDim+j] = hg->vwgt[i*VtxDim+j]);
  } else {
    tw[0] += hg->nVtx;
    for (i=0; i< hg->nVtx; ++i)                 
      cw[i] = 1.0;
  }
  MPI_Allreduce(tw, maxw, VtxDim, MPI_FLOAT, MPI_SUM, hgc->row_comm);
  for (j=0; j<VtxDim; ++j)
    maxw[j] /= 4.0; /* we don't allow a cluster to grow more than half of
                       a part */

                    
  vindex = 0;                        /* marks current position in visit array */
  nRounds = 1;
  for (round = 0; nRounds; ++round) {
    MACRO_TIMER_START (1, "matching phase 1", 0);
    
    /************************ PHASE 1: ****************************************/

    /* Select upto nCandidates unmatched vertices to globally match. */

    for (locCandCnt = 0; locCandCnt < nCandidates && vindex < hg->nVtx; ++vindex) {
      int v=visit[vindex];          
      if (!visited[v])  {         /* unmatched */
        locCandidates [locCandCnt++] = v;    /* select it as a candidate */
        visited[v] = 1;
        match[v] = -match[v] - 1;
      }
    }
/*    uprintf(hgc, "Round %d:   locCandCnt=%d\n", round, locCandCnt); */
    /* assure send buffer is large enough by first computing required size */
    sendsize = 0;
    for (i = 0; i < locCandCnt; i++)  {
      lno = locCandidates[i];
      /* UVCUVC: CHECK if it is possible to use sparse communication
         current code only works if all candidates have been communicated */
/*      if (hg->vindex[lno+1] > hg->vindex[lno]) */
      sendsize += hg->vindex[lno+1] - hg->vindex[lno];
    }
    sendsize += locCandCnt * ( hg->VtxWeightDim + HEADER_SIZE 
                              + (hgp->UsePrefPart ? 1 : 0) ); 
    if (sendsize > nSend)
      MACRO_RESIZE (1.2 * sendsize, nSend, send);    /* resize send buffer */    
    /* put <candidate_gno, candidate_index, weight(s), [pref], count, <edge>> into send buffer */
    s = send;
    for (i = 0; i < locCandCnt; i++)   {
      lno = locCandidates[i];
/*      if (hg->vindex[lno+1] > hg->vindex[lno]) { */  /* UVCUVC CHECK sparse comm? */
        *s++ = VTX_LNO_TO_GNO(hg, lno);                  /* gno of candidate */
        *s++ = candIdx[hgc->myProc_x] + i;               /* candidate index */
        memcpy(s, &cw[lno*VtxDim], sizeof(float) * VtxDim);
        s += VtxDim;
        if (hgp->UsePrefPart)          
          *s++ = hg->pref_part[lno];       /* pref partition info */
        *s++ = hg->vindex[lno+1] - hg->vindex[lno];            /* edge count */
        for (j = hg->vindex[lno]; j < hg->vindex[lno+1]; j++)  
          *s++ = hg->vedge[j];                                   /* edge lno */
/*      } */
    }
    sendsize = s - send;
         
    /* communication to determine global size of rec buffer */
    MPI_Allgather (&sendsize, 1, MPI_INT, size, 1, MPI_INT, hgc->row_comm);
    
    /* determine size of the rec buffer & reallocate bigger iff necessary */
    recsize = 0;
    for (i = 0; i < hgc->nProc_x; i++)
      recsize += size[i];          /* compute total size of edgebuf in ints */
    if (recsize > nEdgebuf)
      MACRO_RESIZE (1.2 * recsize, nEdgebuf, edgebuf);  /* enlarge edgebuf */
    
    /* setup displacement array necessary for MPI_Allgatherv */
    dest[0] = 0;
    for (i = 1; i < hgc->nProc_x; i++)
      dest[i] = dest[i-1] + size[i-1];
    
    /* communicate vertices & their edges to all row neighbors */
    MPI_Allgatherv(send, sendsize, MPI_INT, edgebuf, size, dest, MPI_INT,
                   hgc->row_comm);
    
    /* Communication has grouped candidates by processor, rescramble!     */
    /* Otherwise all candidates from proc column 0 will be matched first, */
    for (i = 0; i < total_nCandidates; i++)
      candvisit[i] = i;
    Zoltan_Rand_Perm_Int (candvisit, total_nCandidates, &(hgc->RNGState_col));
    
    for (i = 0; i < total_nCandidates; i++)
      idxptr[i] = -1;                 /* to flag missing sparse candidates */
    for (i = 0 ; i < recsize; i += count)   {
      int indx        = i++;              /* position of next gno in edgebuf */
      candidate_index = edgebuf[i++];
      idxptr[candidate_index] = indx ;    /* save position of gno in edgebuf */
      memcpy(&candw[candidate_index*VtxDim], &edgebuf[i], sizeof(float)*VtxDim);
      i += VtxDim;
      if (hgp->UsePrefPart)        
        pref   = edgebuf[i++];   /* skip over pref vertex information */
      count           = edgebuf[i++];     /* count of edges */      
    }

    MACRO_TIMER_STOP (1);
    
    /************************ PHASE 2: ***************************************/
      
    /* for each candidate vertex, compute all local partial inner products */    

    kstart = old_kstart = 0;         /* next candidate (of nTotal) to process */
    while (kstart < total_nCandidates) {
      MACRO_TIMER_START (2, "Matching kstart A", 0);
      for (i = 0; i < hgc->nProc_y; i++)
        rows[i] = -1;                  /* to flag data not found for that row */
      sendsize = 0;                    /* position in send buffer */
      sendcnt  = 0;                    /* count of messages in send buffer */
      s        = send;                 /* start at send buffer origin */
      for (k = kstart; k < total_nCandidates; k++)  {
        if (idxptr[candvisit[k]] == -1) 
          continue;                /* don't have this sparse candidate locally */
        
        r = &edgebuf[idxptr[candvisit[k]]];
        candidate_gno   = *r++;          /* gno of candidate vertex */
        candidate_index = *r++;          /* candidate_index of vertex */
        r += VtxDim;
        if (hgp->UsePrefPart)          
          pref = *r++;     /* pref vertex information */          
        count           = *r++;          /* count of following hyperedges */
        
        /* now compute the row's nVtx inner products for kth candidate */
        m = 0;
        if (hg->ewgt != NULL) 
          AGG_INNER_PRODUCT1(hg->ewgt[*r])
        else 
          AGG_INNER_PRODUCT1(1.0)

           /* if local vtx, remove self inner product (useless maximum) */
        if (VTX_TO_PROC_X(hg, candidate_gno) == hgc->myProc_x)
          sums[VTX_GNO_TO_LNO(hg, candidate_gno)] = 0.0;
        
        /* if it is partitioning with preferred parts and/or fixed vertices
           check if matches are OK also eliminate sending value 0.0*/
        for (count=0; count<m; ) 
            if (sums[aux[count]]>PSUM_THRESHOLD
                && AGG_MATCH_OK(hgp, hg, pref, lheadpref[aux[count]]))
                ++count;
            else {
                sums[aux[count]] = 0.0;
                aux[count] = aux[--m];
            }
        if (count == 0)
          continue;         /* no partial sums to append to message */       

        /* iff necessary, resize send buffer to fit at least first message */
        msgsize = HEADER_SIZE + 2 * count;
        if (sendcnt == 0 && (msgsize > nSend)) {
          MACRO_RESIZE (1.2 * msgsize, nSend, send);  /* increase buffer size */
          s = send;         
        }

/*        uprintf(hgc, "cand=%d  sendsize(%d) + msgsize(%d) <= nSend (%d)\n", candidate_gno, sendsize, msgsize, nSend);*/

        /* message is <candidate_gno, candidate_index, count, <lno, psum>> */
        if (sendsize + msgsize <= nSend)  {
          /* flag first data in each destination row for merging */
          if (rows[candidate_gno % hgc->nProc_y] != 1)  {
            rows[candidate_gno % hgc->nProc_y] = 1;
            candidate_index = -candidate_index -1;
          } 

          /* current partial sums fit, so put them into the send buffer */
          dest[sendcnt]   = candidate_gno % hgc->nProc_y;    /* destination */            
          size[sendcnt++] = msgsize;          /* size of message */
          sendsize       += msgsize;          /* cummulative size of message */
          *s++ = candidate_gno;
          *s++ = candidate_index;        
          *s++ = count;
          for (i = 0; i < count; i++)  {          
            *s++ = aux[i];                          /* lno of partial sum */
            f = (float*) s++;
            *f = sums[aux[i]];                      /* partial sum */           
            sums[aux[i]] = 0.0;
          }          
        }
        else  {           /* psum message doesn't fit into buffer */
          for (i = 0; i < count; i++)              
            sums[aux[i]] = 0.0;
/*          uprintf(hgc, "WARYNING: psum msg doesn't fit into buffer!!!! sub-phases!!!!!\n"); */
          break;   
        }  
      }                  /* DONE: loop over k */                    
    
      MACRO_TIMER_STOP (2);
      MACRO_TIMER_START (3, "Matching kstart B", 0);
    
      /* synchronize all rows in this column to next kstart value */
      old_kstart = kstart;
      MPI_Allreduce (&k, &kstart, 1, MPI_INT, MPI_MIN, hgc->col_comm);

      /* Send inner product data in send buffer to appropriate rows */
      ierr = communication_by_plan (zz, sendcnt, dest, size, 1, send, &reccnt, 
                                    &recsize, &nRec, &rec, hgc->col_comm, IPM_TAG);
      if (ierr != ZOLTAN_OK)
        goto End;
    
      /* build index into receive buffer pointer for each proc's row of data */
      for (i = 0; i < hgc->nProc_y; i++)
        rows[i] = recsize;       /* sentinal in case no row's data available */
      k = 0;
      for (r = rec; r < rec+recsize; r+=(HEADER_SIZE+2*(*(r+2)))) {
        if (*(r+1) < 0)  {
          *(r+1) = -(*(r+1)) - 1;           /* make sentinal candidate_index positive */
          rows[k++] = (r - rec);  /* points to gno */
        }
      }

      /* UVCUVC */
      if (k>hgc->nProc_y)
          errexit("k(%d)!=nProc_y(%d) recsize %d", k, hgc->nProc_y, recsize);
    
      /* merge partial i.p. sum data to compute total inner products */
      s = send; 
      for (n = old_kstart; n < kstart; n++) {
        m = 0;       
        for (i = 0; i < hgc->nProc_y; i++) 
          if (rows[i] < recsize && rec [rows[i]+1] == candvisit[n])  {
            candidate_gno   = rec [rows[i]++];
            candidate_index = rec [rows[i]++];
            count           = rec [rows[i]++];
            for (j = 0; j < count; j++)  {
              lno = rec [rows[i]++];
              f = (float *) (rec + rows[i]++);
              if (sums[lno] == 0.0)   /* is this first time for this lno? */ 
                  aux[m++] = lno;     /* then save the lno */
              sums[lno] += *f;    /* sum the psums */
            }
          }

        bestsum = -1.0;                        /* any negative value will do */
        bestlno = -1;                          /* any negative value will do */
/*
  if (m)
            uprintf(hgc, "for index=%d Candidate = %d\n", n, candidate_gno);
        else
            uprintf(hgc, "for index=%d no local partner\n", n);
*/
        for (i = 0; i < m; i++)  {
          float val, mcw=0.0;
          int  lno=aux[i];


          for (j=0; j<VtxDim; ++j)
              if (cw[lno*VtxDim+j]>mcw)
                  mcw = cw[lno*VtxDim+j];
          if (mcw==0.0)
              mcw = 1.0;
          val = sums[lno] / mcw;
/*          printf("[v=%d (lno=%d) ip=%f mcw=%f val=%f] ", VTX_LNO_TO_GNO(hg, lno), lno, sums[lno], mcw, val); */
          if (val > bestsum && match[lno]>=0)  {
            bestsum = val;
            bestlno = lno;
          }
          sums[lno] = 0.0;  
        }

        /*
        if (m)
            printf("\n");
        */
        
        /* Put <candidate_gno, candidate_index, count, <lno, tsum>> into send */
        if (bestlno >= 0)  {
          if (s - send + HEADER_SIZE + 2 * m > nSend ) {
            sendsize = s - send;
            uprintf(hgc, "resize with nSend=%d sendsize=%d m=%d\n", nSend, sendsize, m);
            MACRO_REALLOC (1.2*(sendsize + HEADER_SIZE + 2*m), nSend, send);
            s = send + sendsize;         /* since realloc buffer could move */
          }      
          *s++ = candidate_gno;
          *s++ = candidate_index;            
          *s++ = bestlno;
          f = (float*) s++;
          *f = bestsum;
/*          uprintf(hgc, "cand_gno=%d partner_lno=%d with ip=%f\n", candidate_gno, bestlno, bestsum);*/
        }     
      }
      sendsize = s - send;   /* size (in ints) of send buffer */ 
    
      /* Communicate total inner product results to MASTER ROW */
      MPI_Gather(&sendsize, 1, MPI_INT, size, 1, MPI_INT, 0, hgc->col_comm);
    
      if (hgc->myProc_y == 0) {
        recsize = 0;
        for (i = 0; i < hgc->nProc_y; i++)
          recsize += size[i];        
          
        dest[0] = 0;
        for (i = 1; i < hgc->nProc_y; i++)
          dest[i] = dest[i-1] + size[i-1];
          
        if (recsize > nRec)
          MACRO_RESIZE (1.2 * recsize, nRec, rec);      /* make rec buffer bigger */
      }
      
      MPI_Gatherv (send, sendsize, MPI_INT, rec, size, dest, MPI_INT, 0,
                   hgc->col_comm);

/*      uprintf(hgc, "recsize=%d\n", recsize);*/
      
      /* Determine best vertex and best sum for each candidate */
      if (hgc->myProc_y == 0) {   /* do following only if I am the MASTER ROW */
        for (r = rec; r < rec + recsize;)  {
          candidate_gno   = *r++;
          candidate_index = *r++;
          bestlno         = *r++;                    /* count of nonzero pairs */
          f               =  (float*) r++;
          bestsum         = *f;
        
/*          uprintf(hgc, "local best for cand %d (idx=%d) is %d with ip=%f\n", candidate_gno, candidate_index, (bestlno<0) ? -1 : match[bestlno], bestsum);*/
          master_data[candidate_index].candidate = candidate_gno;
          master_data[candidate_index].partner = match[bestlno];          
          master_data[candidate_index].ip = bestsum;
          if (match[bestlno]<0)
              errexit("hey for bestlno: match[%d]=%d\n", bestlno, match[bestlno]);
        }
      } 
      MACRO_TIMER_STOP (3);    
    }            /* DONE: kstart < max_nTotal loop */
    
    
    /************************ PHASE 3 & 4 ********************************/
    
    MACRO_TIMER_START (4, "Matching Phase 3&4", 1);

    replycnt=0;
    if (hgc->myProc_y == 0) {      
      HG_Ptr = hg;

      MPI_Allreduce(master_data, global_best, total_nCandidates,
                    phasethreetype, phasethreeop, hgc->row_comm);

      /*
      uprintf(hgc, "Cand\tPartner\tIP\n");
      for (i = 0; i < total_nCandidates; i++)
        uprintf(hgc, "%d\t%d\t%.3f\n",  global_best[i].candidate,
                global_best[i].partner, global_best[i].ip);
      */

      /* Reinitialize master_data for next round */
      for (i = 0; i < total_nCandidates; i++) {        
        master_data[i].candidate = -1;
        master_data[i].partner   = -1;
        master_data[i].ip        = -1.0;
      }



      msgsize = total_nCandidates*(2+VtxDim+(hgp->UsePrefPart ? 1: 0));
      if (msgsize > nSend) 
        MACRO_RESIZE (msgsize, nSend, send);  /* increase buffer size */
      s = send;         
      
      for (i = 0; i < total_nCandidates; i++) {
        int cproc, pproc;
        candidate_gno = global_best[i].candidate;
        
        if (candidate_gno == -1)
          continue;
        
        partner_gno = global_best[i].partner;
        cproc = VTX_TO_PROC_X(hg, candidate_gno);
        pproc = VTX_TO_PROC_X(hg, partner_gno);
        if (pproc == hgc->myProc_x)   { /* partner is mine */
          /* check weight constraints */
          
          int plno = VTX_GNO_TO_LNO(hg, partner_gno);
          for (j=0; j<VtxDim; ++j)
            if (cw[plno*VtxDim+j]+candw[i*VtxDim+j]>maxw[j])
              break;
          dest[replycnt++] = cproc;
          *s++ = i;
          *s++ = plno;
          if (hgp->UsePrefPart)
              *s++ = lheadpref[plno];
          f = (float *) s;
          if (j<VtxDim) { /* reject due to weight constraint*/
            *f = -1.0; /* negative means rejected */
            f += VtxDim;
/*            uprintf(hgc, "I'm rejecting (%d, %d)\n", candidate_gno, partner_gno); */
          } else { /* accept */ 
            for (j=0; j<VtxDim; ++j) { /* modify weight immediately */
              cw[plno*VtxDim+j] += candw[i*VtxDim+j];
              *f++ = cw[plno*VtxDim+j];
            }
            visited[plno] = 1;
/* this printf only works if all vertices are local
   uprintf(hgc, "I'm acceptiong (%d [%d], %d[%d])\n", candidate_gno, lheadpref[candidate_gno], partner_gno, lheadpref[partner_gno]);
*/
            /* was partner a candidate ?*/
            if (match[plno]<0) 
              errexit("HEY HEY HEY  match[%d(gno=%d)]=%d\n", plno, partner_gno, match[plno]);
          }
          s = (int *)f;
        }
      }
    }

    for (i=0; i<locCandCnt; ++i) {
        int lno=locCandidates[i];
        if (match[lno]<0)
            match[lno] = -match[lno]-1;
        else
            errexit("hey hey hey match[%d]=%d\n", lno, match[lno]);
    }

    /* bcast accepted match to column so that they can
       set visited array for local partners and also set cw properly */   
    MPI_Bcast (&replycnt, 1, MPI_INT, 0, hgc->col_comm);
    
    if (hgc->myProc_y!=0 && replycnt*(2+VtxDim+(hgp->UsePrefPart ? 1: 0))>nSend)
      MACRO_REALLOC (replycnt*(2+VtxDim+(hgp->UsePrefPart ? 1: 0)), nSend, send);  /* increase buffer size */
    
    MPI_Bcast (send, replycnt*(2+VtxDim+(hgp->UsePrefPart ? 1: 0)), MPI_INT, 0, hgc->col_comm);
    if (hgc->myProc_y!=0) {
      int plno;
      
      s = send;
      for (i=0; i<replycnt; ++i) {
        ++s; /* skip candidate_index*/

        plno=*s++;
        if (hgp->UsePrefPart) ++s; /* skip fixed vertex */
        f = (float *) s;
        if (*f<0.0) { /* reject due to weight constraint*/
        } else {
/*        uprintf(hgc, "Set visited flag of %d (gno=%d)\n", plno, VTX_LNO_TO_GNO(hg, plno)); */
          visited[plno] = 1;          
          memcpy(&cw[plno*VtxDim], s, sizeof(float)*VtxDim);
        }
        s += VtxDim;
      }      
    }
    

    if (hgc->myProc_y == 0) {    
      /* send accept/reject message */
      communication_by_plan(zz, replycnt, dest, NULL, 2+VtxDim+(hgp->UsePrefPart ? 1: 0), send,
                            &reccnt, &recsize, &nRec, &rec, hgc->row_comm, CONFLICT_TAG);

      if (reccnt*(3+VtxDim+(hgp->UsePrefPart ? 1: 0)) > nSend) 
        MACRO_RESIZE (reccnt*(3+VtxDim+(hgp->UsePrefPart ? 1: 0)), nSend, send);  /* increase buffer size */
      s = send;         
      for (r = rec; r < rec + recsize;) {
        int ci=*r++, lno=VTX_GNO_TO_LNO(hg, global_best[ci].candidate),
            lheadno, partner=global_best[ci].partner, pref=-1;

        ++r;  /* skip plno */
        if (hgp->UsePrefPart)
            pref = *r++;
        f = (float *) r;
        if (*f<0.0) { /* rejected */
          f += VtxDim;
          lheadno = -1;
/*          uprintf(hgc, "(%d, %d) has been rejected\n", global_best[ci].candidate, global_best[ci].partner);*/
        } else { /* accepted */
          lheadno = Zoltan_KVHash_Insert(&hash, partner, lno); 


          for (j=0; j<VtxDim; ++j) 
            cw[lheadno*VtxDim+j] = *f++;
          if (hgp->UsePrefPart)
              lheadpref[lno] = pref;
          lhead[lno] = lheadno;
          match[lno] = partner;
          
/*          uprintf(hgc, "(%d, %d) has been accepted\n", global_best[ci].candidate, global_best[ci].partner);*/
        }
        r = (int *) f;
        *s++ = lno;
        *s++ = lheadno;
        *s++ = partner;
        if (hgp->UsePrefPart)
            *s++ = pref;
        if (lheadno!=-1)
          memcpy(s, &cw[lheadno*VtxDim], sizeof(float)*VtxDim);
        s += VtxDim;
      }
    }
    
    recsize = s - send;
    MPI_Bcast (&recsize, 1, MPI_INT, 0, hgc->col_comm); /* bcase nSend */
    if (recsize>nSend) /* for procs other than 0; that might be true */
      MACRO_RESIZE (recsize, nSend, send);  /* increase buffer size */

    MPI_Bcast (send, recsize, MPI_INT, 0, hgc->col_comm);

    if (hgc->myProc_y !=0) { /* master row already done this */
      for (s = send; s < send + recsize; ) {
        int lno     = *s++;
        int lheadno = *s++;
        int partner = *s++, pref;
        
        if (hgp->UsePrefPart)
            pref = *s++;
        if (lheadno!=-1) { 
          lhead[lno] = lheadno;
          match[lno] = partner;
          memcpy(&cw[lheadno*VtxDim], s, sizeof(float)*VtxDim);
          if (hgp->UsePrefPart)
              lheadpref[lheadno] = pref;
        }
        s += VtxDim;        
      }
    }

    for (; vindex < hg->nVtx && visited[visit[vindex]]; ++vindex);
    
    i = vindex < hg->nVtx;
    MPI_Allreduce(&i, &nRounds, 1, MPI_INT, MPI_SUM, hgc->row_comm);
    MACRO_TIMER_STOP (5);                       /* end of phase 4 */
  }                                             /* DONE: loop over rounds */


 End:
  MPI_Op_free(&phasethreeop);
  MPI_Type_free(&phasethreetype);
  ZOLTAN_FREE(&global_best);



  if (hgc->myProc_y==0 && total_nCandidates)
    Zoltan_KVHash_Destroy(&hash);
    
  Zoltan_Multifree (__FILE__, __LINE__, 22, &candIdx, &cw, &tw, &maxw, &candw, &lhead, &lheadpref,
                    &visit, &visited, &sums, &send, &dest, &size, &rec, &aux, &idxptr, &candvisit,
                    &edgebuf, &locCandidates, &rows, &master_data, &master_procs);
  ZOLTAN_TRACE_EXIT(zz, yo);
    
  return ierr;
}



#undef MACRO_REALLOC
#undef MACRO_TIMER_START
#undef MACRO_TIMER_STOP
#undef INNER_PRODUCT
#undef INNER_PRODUCT2
#undef ROUNDS_CONSTANT
#undef IPM_TAG
#undef HEADER_SIZE
#undef PSUM_THRESHOLD
#undef TSUM_THRESHOLD

#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif


