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

    /*
#define _DEBUG
    */
    
static ZOLTAN_PHG_MATCHING_FN pmatching_local; /* function for local matching */
static ZOLTAN_PHG_MATCHING_FN pmatching_ipm;   /* inner product matching */
static ZOLTAN_PHG_MATCHING_FN pmatching_alt_ipm;   /* alternating ipm */

static int Zoltan_PHG_match_isolated(ZZ *zz, HGraph *hg, Matching match, 
                                     int small_degree);


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
            hgp->matching = pmatching_local; 
            hgp->locmatching = hp.matching;
            hgp->matching_opt = hp.matching_opt;
        }
    } else if (!strcasecmp(hgp->redm_str, "c-ipm"))
        hgp->matching = pmatching_ipm;   
    else if (!strcasecmp(hgp->redm_str, "ipm"))
        hgp->matching = pmatching_ipm;
    else if (!strcasecmp(hgp->redm_str, "alt-ipm"))
        hgp->matching = pmatching_alt_ipm;
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
                      ZOLTAN_MALLOC (hg->nEdge * sizeof(float))))
         MEMORY_ERROR;
 
     Zoltan_PHG_Scale_Edges (zz, hg, new_ewgt, hgp);
     old_ewgt = hg->ewgt;
     hg->ewgt = new_ewgt;
  }

  /* Create/update scale vector for vertices for inner product */
  if (hgp->vtx_scaling) 
     Zoltan_PHG_Scale_Vtx (zz, hg, hgp);
  
  
  /* Do the matching */
  if (hgp->matching) {
    /* first match isolated vertices */
    Zoltan_PHG_match_isolated(zz, hg, match, 0);
    /* now do the real matching */
    ierr = hgp->matching (zz, hg, match, hgp);
    /* clean up by matching "near-isolated" vertices of degree 1 */
    /* only useful in special cases (e.g. near-diagonal matrices) */
    /* Zoltan_PHG_match_isolated(zz, hg, match, 1); */
  }

End: 

  /* Restore the old edge weights if scaling was used. */
  if (hgp->edge_scaling)
      hg->ewgt = old_ewgt;

  ZOLTAN_FREE ((void**) &new_ewgt);
  ZOLTAN_TRACE_EXIT (zz, yo);
  return ierr;
}


static int Zoltan_PHG_match_isolated(
  ZZ *zz,
  HGraph *hg,
  Matching match,
  int small_degree /* 0 or 1; 0 corresponds to truely isolated vertices */
)
{
    int v=-1, i, *ldeg, *deg;
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
      
      for (i=0; i<hg->nVtx; ++i)
          if (deg[i] <= small_degree) { /* isolated vertex */
#ifdef _DEBUG
              ++cnt;
#endif
              if (match[i]==i){
                  /* vtx i not matched yet */
                  if (v==-1)
                      v = i;
                  else {
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

static int pmatching_local(
  ZZ *zz,
  HGraph *hg,
  Matching match,
  PHGPartParams *hgp
)
{
    int limit=hg->nVtx, err=ZOLTAN_OK;
    PHGComm *hgc=hg->comm;
    int root_matchcnt, root_rank;
    
    err = hgp->locmatching (zz, hg, match, &limit);
    
    /* Optimization */
    if (hgp->matching_opt) 
        err = hgp->matching_opt (zz, hg, match, &limit);
    
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
  static int level=0;
  static int old_nvtx=0;

  strcpy(redm_orig, hgp->redm_str); /* save original parameter string */

  if (hg->nVtx > old_nvtx){
    /* larger hgraph; must have started new bisection v-cycle */
    level= 0;
  }

  /* first level is 0 */
  if ((level&1) == 0)  /* alternate even-odd levels */
    strcpy(hgp->redm_str, hgp->redm_fast); /* fast method is c-ipm for now */
  else
    strcpy(hgp->redm_str, "ipm");  

  ierr = pmatching_ipm(zz, hg, match, hgp);  /* only works for ipm and c-ipm! */

  ++level;  /* we don't have access to level data, so keep track this way */
  old_nvtx = hg->nVtx;

  /* set redm parameter back to original */
  strcpy(hgp->redm_str, redm_orig);
  
  return ierr;
}

/****************************************************************************
 * inner product matching (with user selectable column variant, c-ipm)
 * Parallelized version of the serial algorithm (see hg_match.c)
 * Based on conversations with Rob Bisseling by Aaron Becker, UIUC, summer 2004
 * completed by R. Heaphy
 */
               
#define ROUNDS_CONSTANT 8     /* controls the number of candidate vertices */ 
#define IPM_TAG        28731  /* MPI message tag, arbitrary value */
#define HEADER_COUNT    4     /* Phase 2 send buffer header size in ints */

/* these thresholds need to become parameters in phg - maybe ??? */
#define PSUM_THRESHOLD 0.0    /* ignore inner products (i.p.) < threshold */
#define TSUM_THRESHOLD 0.0    /* ignore inner products (i.p.) < threshold */
                                 
/* Forward declaration for a routine that encapsulates the common calls to use
** the Zoltan unstructured communications library for the matching code */
static int communication_by_plan (ZZ* zz, int sendcnt, int* dest, int* size, 
 int scale, int* send, int* reccnt, int* recsize, int* nRec, int** rec,
 MPI_Comm comm, int tag);

/* Actual inner product calculations between candidates (in rec buffer) */
/* and local vertices.  Not inlined because inline is not universal, yet.  */
#define INNER_PRODUCT1(ARG)\
  for (i = 0; i < count; r++, i++)\
    for (j = hg->hindex [*r]; j < hg->hindex [*r + 1]; j++) {\
      if (cmatch[hg->hvertex[j]] == hg->hvertex[j])  {\
        if (sums [hg->hvertex[j]] == 0.0)\
          index [m++] = hg->hvertex[j];\
        sums [hg->hvertex[j]] += (ARG);\
      }\
    }
            
/* Mostly identical inner product calculation to above for c-ipm variant. Here */
/* candidates are a subset of local vertices and are not in a separate buffer  */     
#define INNER_PRODUCT2(ARG)\
   for (i = hg->vindex[gno]; i < hg->vindex[gno+1]; i++)  {\
     edge = hg->vedge[i];\
     for (j = hg->hindex [edge]; j < hg->hindex [edge+1]; j++)  {\
       if (cmatch[hg->hvertex[j]] == hg->hvertex[j])    {\
         if (sums [hg->hvertex[j]] == 0.0)\
           index [m++] = hg->hvertex[j];\
         sums [hg->hvertex[j]] += (ARG);\
       }\
     }\
   }  

/* simple macro to start timer */
#define MACRO_TIMER_START(arg, message) \
  if (hgp->use_timers > 3)  {\
    if (timer[arg] < (arg))\
      timer[arg] = Zoltan_Timer_Init(zz->ZTime, 0, message);\
    ZOLTAN_TIMER_START(zz->ZTime, timer[arg], hg->comm->Communicator);\
  }   

/* simple corresponding macro to stop timer */
#define MACRO_TIMER_STOP(arg) \
  if (hgp->use_timers > 3) \
    ZOLTAN_TIMER_STOP(zz->ZTime, timer[arg], hg->comm->Communicator);

/* convenience macro to encapsulate resizing a buffer when necessary */
#define MACRO_REALLOC(new_size, old_size, buffer)  {\
  old_size = new_size;\
  if (!(buffer = (int*) ZOLTAN_REALLOC (buffer, old_size * sizeof(int)))) {\
    ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Memory error.");\
    err = ZOLTAN_MEMERR;\
    goto fini;\
    }\
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
/* this routine is static so that it does not create a linker symbol.       */
static int pmatching_ipm (ZZ *zz,
  HGraph* hg,
  Matching match,
  PHGPartParams *hgp)
{
  int i, j, k, n, m, round, vindex, kstart, *r, *s;   /* loop counters  */
  int lno, gno, count, old_kstart;                    /* temp variables */
  int sendcnt, sendsize, reccnt, recsize, msgsize;    /* temp variables */
  int nRounds;         /* # of matching rounds to be performed;       */
                       /* identical on all procs in hgc->Communicator.*/
  int nCandidates;     /* # of candidates on this proc; identical     */
                       /* on all procs in hgc->col_comm.              */
  int nTotal;          /* sum of # of non-empty candidates across proc row. */
  int max_nTotal;      /* max of nTotal across proc cols.        */
  int *send = NULL,    /* working buffers, may be reused              */
      *dest = NULL,
      *size = NULL,
      *rec = NULL,
      *index = NULL,
      *aux = NULL;
  int *visit = NULL,   /* fixed usage arrays, candidate visit order */
      *cmatch = NULL,  /* working copy of match array */
      *select = NULL,  /* current selected candidates */
      *permute = NULL, /* reorder of candidates after global communicatio */
      *edgebuf = NULL; /* holds received candidates for processing */
  int nSend, nDest, nSize, nRec, nIndex, nEdgebuf; /* currently allocated size
                                                      of the corresponding
                                                      working buffers */
  float bestsum;      /* holds current best inner product */
  float *sums = NULL, /* holds inner product of one candidate for each vertex */
        *f = NULL;    /* used to stuff floating value into integer message */
  PHGComm *hgc = hg->comm;
  int err = ZOLTAN_OK, old_row, row, col;
  int max_nPins, max_nVtx;       /* Global max # pins/proc and vtx/proc */
  int **rows = NULL;             /* used only in merging process */
  int bestlno, vertex, nselect, edge;
  int *master_data = NULL, *master_procs = NULL, *mp = NULL, nmaster = 0;
  int cFLAG;                    /* if set, do only a column matching, c-ipm */
  static int timer[7] = {-1, -1, -1, -1, -1, -1, -1};
  char *yo = "pmatching_ipm";
  
  
  ZOLTAN_TRACE_ENTER (zz, yo);
  MACRO_TIMER_START (0, "matching setup");
 
  /* this restriction may be removed later, but for now NOTE this test */
  if (sizeof(int) < sizeof (float))  {
    ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Code must be modified before using");
    err = ZOLTAN_FATAL;
    goto fini;
  }

  /* set a flag if user wants a column matching or a full matching */
  cFLAG = strcasecmp(hgp->redm_str, "c-ipm") ? 0 : 1;

  /* determine basic working parameters */
  nRounds     = cFLAG ? ROUNDS_CONSTANT : hgc->nProc_x * ROUNDS_CONSTANT;
  nCandidates = calc_nCandidates (hg->nVtx, cFLAG ? 1 : hgc->nProc_x); 
    
  /* determine maximum number of Vtx and Pins for storage allocation */
  /* determine initial sum of all candidates, nTotal for storage allocation */
  if (cFLAG)  {
    nTotal    = nCandidates;
    max_nVtx  = hg->nVtx;
    max_nPins = hg->nPins;
  }
  else  {
    MPI_Allreduce(&hg->nPins, &max_nPins, 1, MPI_INT,MPI_MAX,hgc->Communicator);
    max_nVtx = nTotal = 0;
    for (i = 0; i < hgc->nProc_x; i++)  {
      count = hg->dist_x[i+1]-hg->dist_x[i];  /* number of vertices on proc i */
      if (count > max_nVtx)
        max_nVtx = count;
      nTotal += calc_nCandidates (count, hgc->nProc_x);
    }
  }
                 
  /* allocate "complicated" fixed sized array storage */
  nIndex   = 1 + MAX (MAX(nTotal, max_nVtx), hgc->nProc_y);
  nDest    = 1 + MAX (MAX(hgc->nProc_x,hgc->nProc_y), MAX(nTotal,max_nVtx));
  nSize    = 1 + MAX (MAX(hgc->nProc_x,hgc->nProc_y), MAX(nTotal,max_nVtx));

  /* These 3 buffers are REALLOC'd iff necessary; this should be very rare */
  nSend    = max_nPins;   /* nSend/nEdgebuf are used for candidate exchange   */
  nEdgebuf = max_nPins;   /* candidates sent as <gno, #pins, pin_list>        */
  nRec     = max_nPins;

  if (hg->nVtx)  
    if (!(cmatch = (int*)   ZOLTAN_MALLOC (hg->nVtx * sizeof (int)))
     || !(visit  = (int*)   ZOLTAN_MALLOC (hg->nVtx * sizeof (int)))
     || !(aux    = (int*)   ZOLTAN_MALLOC (hg->nVtx * sizeof (int)))     
     || !(sums   = (float*) ZOLTAN_CALLOC (hg->nVtx,  sizeof (float)))) {
       ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Memory error.");
       err = ZOLTAN_MEMERR;
       goto fini;
    }
    
  if (!cFLAG)
    if (!(edgebuf      = (int*)  ZOLTAN_MALLOC (nEdgebuf   * sizeof (int)))
     || !(master_data  = (int*)  ZOLTAN_MALLOC (3 * nTotal * sizeof (int)))
     || !(master_procs = (int*)  ZOLTAN_MALLOC (nTotal     * sizeof (int)))) {
       ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Memory error.");
       err = ZOLTAN_MEMERR;
       goto fini;
    }  
  
  if ((nTotal && !(permute = (int*)  ZOLTAN_MALLOC (nTotal  * sizeof (int))))
   || (nCandidates && !(select  = (int*)  ZOLTAN_MALLOC (nCandidates * sizeof (int))))
   || (nSend && !(send = (int*)  ZOLTAN_MALLOC (nSend       * sizeof (int))))
   || !(dest    = (int*)  ZOLTAN_MALLOC (nDest              * sizeof (int)))
   || !(size    = (int*)  ZOLTAN_MALLOC (nSize              * sizeof (int)))
   || (nRec && !(rec = (int*)  ZOLTAN_MALLOC (nRec          * sizeof (int))))
   || !(index   = (int*)  ZOLTAN_MALLOC (nIndex             * sizeof (int)))
   || !(rows    = (int**) ZOLTAN_MALLOC ((hgc->nProc_y + 1) * sizeof (int*)))) {
     ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Memory error.");
     err = ZOLTAN_MEMERR;
     goto fini;
  }
    
  /* match[] is local slice of global matching array.  It uses local numbering 
   * (zero-based). Initially, match[i] = i. After matching, match[i]=i indicates
   * an unmatched vertex. A matching between vertices i & j is indicated by 
   * match[i] = j & match [j] = i.  NOTE: a match to an off processor vertex is
   * indicated by a negative number, -(gno+1), and must use global numbers
   * (gno's) and not local numbers, lno's, which are zero based.        */

  /* Compute candidates' vertex visit order (selection). Random is default. */
  Zoltan_PHG_Vertex_Visit_Order (zz, hg, hgp, visit);
  
  /* Loop processing ncandidates vertices per column each round.
   * Each loop has 4 phases:
   * Phase 1: send ncandidates vertices for global matching - horizontal comm
   * Phase 2: sum  inner products, find best in column - vertical communication
   * Phase 3: return best sums to owner's column    - horizontal communication
   * Phase 4: return actual match selections        - horizontal communication
   *
   * No conflict resolution required because temp locking prevents conflicts. */

  MACRO_TIMER_STOP (0);
  vindex = 0;                                /* marks position in visit array */
  for (round = 0; round < nRounds; round++)  {
    MACRO_TIMER_START (1, "matching phase 1");
    
    /************************ PHASE 1: ***************************************/
    
    mp = master_data;              /* mp is a pointer to the master data */
    nmaster = 0;                   /* count of data accumulted in master row */    
    memcpy (cmatch, match, hg->nVtx * sizeof(int));  /* for temporary locking */

    if (cFLAG)
      /* select upto nCandidates unmatched vertices to locally match */
      for (nTotal=0; nTotal < nCandidates && vindex < hg->nVtx; vindex++)  {
        if (cmatch[visit[vindex]] == visit[vindex])  {         /* unmatched */
          permute[nTotal++] = visit[vindex];    /* select it as a candidate */
          cmatch[visit[vindex]] = -1;           /* mark it as a pending match */
        }
      }
    else  {       
      /* Select upto nCandidates unmatched vertices to globally match. */
      for (sendcnt = 0; sendcnt < nCandidates && vindex < hg->nVtx; vindex++)  {
        if (cmatch[visit[vindex]] == visit[vindex])  {         /* unmatched */
          select[sendcnt++] = visit[vindex];     /* select it as a candidate */
          cmatch[visit[vindex]] = -1;            /* mark it as a pending match */
        }
      }
      nselect = sendcnt;                          /* save for later use */
                        
      /* assure send buffer is large enough by first computing required size */
      sendsize = 2 * sendcnt;                      /* takes care of header */
      for (i = 0; i < sendcnt; i++)  {
        lno = select[i];
        sendsize += (hg->vindex[lno+1] - hg->vindex[lno]);
        }
      if (sendsize > nSend)
        MACRO_REALLOC (sendsize, nSend, send);     /* make send buffer bigger */
    
      /* fill send buff as list of <gno, gno's edge count, list of edge lno's> */
      s = send;
      n = 0;
      for (i = 0; i < sendcnt; i++)   {
        lno = select[i];
        /* Optimization: Only send data for vertices that are non-empty locally */
        if (hg->vindex[lno+1] > hg->vindex[lno]) {
          n++;  /* non-empty vertex */
          *s++ = VTX_LNO_TO_GNO (hg, lno);                                /* gno */
          *s++ = hg->vindex[lno+1] - hg->vindex[lno];                   /* count */
          for (j = hg->vindex[lno]; j < hg->vindex[lno+1]; j++)  
            *s++ = hg->vedge[j];                                  /* lno of edge */
        }
      }
      sendsize = s - send;
/*
      uprintf(hgc, "Debug: sendsize=%d, n=%d\n", sendsize, n);
      uprintf(hgc, "Debug: %d out of %d candidates nonempty (%g%%)\n",
            n, sendcnt, 100.*n/sendcnt);
*/
    
      /* determine actual global number of candidates this round */
      /* n is actual number of local non-empty vertices */
      /* nTotal is the global number of candidate vertices in this row */
      MPI_Allreduce (&n, &nTotal, 1, MPI_INT, MPI_SUM, hgc->row_comm);
      MPI_Allreduce (&nTotal, &max_nTotal, 1, MPI_INT, MPI_MAX, hgc->col_comm);
      if (max_nTotal == 0) {
        if (hgp->use_timers > 3)
           ZOLTAN_TIMER_STOP (zz->ZTime, timer[1], hg->comm->Communicator);
        break;                          /* globally all work is done, so quit */
      }
      
      /* communication to determine global size & displacements of rec buffer */
      MPI_Allgather (&sendsize, 1, MPI_INT, size, 1, MPI_INT, hgc->row_comm);
     
      /* determine size of the rec buffer & reallocate bigger iff necessary */
      recsize = 0;
      for (i = 0; i < hgc->nProc_x; i++)
        recsize += size[i];          /* compute total size of edgebuf in ints */
      if (recsize > nEdgebuf)
        MACRO_REALLOC (recsize, nEdgebuf, edgebuf);        /* enlarge edgebuf */
    
      /* setup displacement array necessary for MPI_Allgatherv */
      dest[0] = 0;
      for (i = 1; i < hgc->nProc_x; i++)
        dest[i] = dest[i-1] + size[i-1];

      /* communicate vertices & their edges to all row neighbors */
      MPI_Allgatherv (send, sendsize, MPI_INT, edgebuf, size, dest, MPI_INT,
       hgc->row_comm);
         
      /* create random permutation of index into the edge buffer */
      i = 0;
      for (j = 0 ; j < nTotal  &&  i < recsize; j++)   {
        permute[j] = i++;             /* save index of gno in permute[] */
        count      = edgebuf[i++];    /* count of edges */
        i += count;                   /* skip over count edges */
      }

      /* Communication has grouped candidates by processor, rescramble! */
      /* Otherwise all candidates from proc column 0 will be matched first, */
      /* Future: Instead of Zoltan_Rand_Perm_Int, we could use              */
      /* Zoltan_PHG_Vertex_Visit_Order() to reorder the candidates          */
      /* but that routine uses a local hg so won't work on the candidates.  */
      if (hgc->nProc_x > 1) {
        Zoltan_Srand_Sync(Zoltan_Rand(NULL), &(hgc->RNGState_col),hgc->col_comm);
        Zoltan_Rand_Perm_Int (permute, nTotal, &(hgc->RNGState_col));
      }
    }                           /* DONE:  if (cFLAG) else ...}  */
    MACRO_TIMER_STOP (1);
    
    /************************ PHASE 2: ***************************************/
      
    /* for each candidate vertex, compute all local partial inner products */
    kstart = old_kstart = 0;         /* next candidate (of nTotal) to process */
/* KMDKMD OLD VERSION -- REMOVE IF FIX IS OK
    while (kstart < nTotal)  { 
   KMDKMD I think the override of the send-buffering with realloc removes
   KMDKMD the need for the while loop.  Indeed, since nTotal can vary within
   KMDKMD a processor column, and since there is column communication within
   KMDKMD this while loop, the while condition will cause hangs when some
   KMDKMD processor has nTotal=0.  We'll have to revisit this problem when
   KMDKMD we reactivate the fixed-size send buffer.
*/
      MACRO_TIMER_START (2, "Matching kstart A");
      sendsize = 0;                      /* position in send buffer */
      sendcnt = 0;                       /* count of messages in send buffer */
      s = send;                          /* start at send buffer origin */
      for (k = kstart; k < nTotal; k++)  {  
        if (!cFLAG)  { 
          r     = &edgebuf[permute[k]];
          gno   = *r++;                      /* gno of candidate vertex */
          count = *r++;                      /* count of following hyperedges */
        }
        else
          gno = permute[k];       /* need to use next local vertex */
                                  /* here gno is really a local id */
                  
        /* now compute the row's nVtx inner products for kth candidate */
        m = 0;
        if (!cFLAG) {
          if      ((hg->ewgt == NULL) && (hgp->vtx_scal == NULL))
            INNER_PRODUCT1(1.0)
          else if ((hg->ewgt == NULL) && (hgp->vtx_scal != NULL))
            INNER_PRODUCT1(hgp->vtx_scal[hg->hvertex[j]])
          else if ((hg->ewgt != NULL) && (hgp->vtx_scal == NULL))
            INNER_PRODUCT1(hg->ewgt[*r])
          else if ((hg->ewgt != NULL) && (hgp->vtx_scal != NULL))
            INNER_PRODUCT1(hgp->vtx_scal[hg->hvertex[j]] * hg->ewgt[*r])
                  
        } else /* cFLAG */ {
          if      ((hg->ewgt == NULL) && (hgp->vtx_scal == NULL))
            INNER_PRODUCT2(1.0)
          else if ((hg->ewgt == NULL) && (hgp->vtx_scal != NULL))
            INNER_PRODUCT2(hgp->vtx_scal[hg->hvertex[j]])
          else if ((hg->ewgt != NULL) && (hgp->vtx_scal == NULL))
            INNER_PRODUCT2(hg->ewgt [edge])
          else if ((hg->ewgt != NULL) && (hgp->vtx_scal != NULL))
            INNER_PRODUCT2(hgp->vtx_scal[hg->hvertex[j]] * hg->ewgt[edge])   
        }
          
        /* if local vtx, remove self inner product (useless maximum) */
        if (cFLAG)
          sums [gno] = 0.0;             /* here gno is really a local id */
        else if (VTX_TO_PROC_X (hg, gno) == hgc->myProc_x)
          sums [VTX_GNO_TO_LNO (hg, gno)] = 0.0;
         
        /* count partial sums exceeding PSUM_THRESHOLD */   
        count = 0;
        for (i = 0; i < m; i++)  {
          lno = index[i];
          if (sums[lno] > PSUM_THRESHOLD)
            aux[count++] = lno;      /* save lno for significant partial sum */
          else
            sums[lno] = 0.0;         /* clear unwanted entries */  
        }     
        if (count == 0)
          continue;         /* no partial sums to append to message */

        /* HEADER_COUNT (row, col, gno, count of <lno, psum> pairs) */                    
        msgsize = HEADER_COUNT + 2 * count;

        /* EBEB Avoid buffer overflow by realloc. 
                Temp hack for testing. Revisit this!  
        */
        if (msgsize+sendsize >= nSend){
          int offset=s-send;
          MACRO_REALLOC (2*(msgsize+nSend), nSend, send);
          s = send+offset;    
        }
        
        /* iff necessary, resize send buffer to fit at least first message */
        if (sendcnt == 0 && (msgsize > nSend))  {
          MACRO_REALLOC (5*msgsize, nSend, send);  /* make send buffer bigger */
          s = send;    
        }

        if (sendsize + msgsize <= nSend)  {
          /* current partial sums fit, so put them into the send buffer */
          dest[sendcnt]   = gno % hgc->nProc_y;  /* proc to compute tsum */
          size[sendcnt++] = msgsize;             /* size of message */
          sendsize       += msgsize;          /* cummulative size of message */
          
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
        else  {           /* psum message doesn't fit into buffer */
          for (i = 0; i < count; i++)              
            sums[aux[i]] = 0.0;        
          break;
        }  
      }                  /* DONE: loop over k */                    

      MACRO_TIMER_STOP (2);
      MACRO_TIMER_START (3, "Matching kstart B");
     
      /* synchronize all rows in this column to next kstart value */
      /* EBEB Skip synchronization. */
      old_kstart = kstart;      
      kstart = k;
      /* 
      MPI_Allreduce (&k, &kstart, 1, MPI_INT, MPI_MIN, hgc->col_comm);
      */
            
      /* Send inner product data in send buffer to appropriate rows */
      err = communication_by_plan (zz, sendcnt, dest, size, 1, send, &reccnt, 
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
      for (i = k; i <= hgc->nProc_y; i++)
        rows[i] = &rec[recsize];       /* in case no data came from a row */
      
      /* merge partial i.p. sum data to compute total inner products */
      s = send; 
      for (n = old_kstart; n < kstart; n++)  {
        m = 0;        
        /* here gno is really a local id when cFLAG */
        gno = (cFLAG) ? permute[n] : edgebuf [permute[n]];
               
        /* Not sure if this test makes any speedup ???, works without! */
        if (gno % hgc->nProc_y != hgc->myProc_y)
          continue;                           /* this gno's partial IPs not sent
                                                 to this proc */
        
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
            rows[i] += 3;                 /* skip past current lno, row, col */       
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
            sendsize = s - send;
            MACRO_REALLOC (nSend + 2*(1+count), nSend, send); /*enlarge buffer*/
            s = send + sendsize;   /* since realloc buffer could move */ 
          }      
          *s++ = gno;
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

/* KDDKDD SHOULD THIS BE MPI_Gather to row 0 (MASTER ROW)? */
      MPI_Allgather (&sendsize, 1, MPI_INT, size, 1, MPI_INT, hgc->col_comm);

/* KDDKDD DOES THE FOLLOWING MEMORY MANAGEMENT APPLY ONLY TO 
   KDDKDD myProc_y == 0?  ONLY ROOT OF MPI_GATHERV SHOULD NEED IT. */
      recsize = 0;
      for (i = 0; i < hgc->nProc_y; i++)
        recsize += size[i];        
          
      dest[0] = 0;
      for (i = 1; i < hgc->nProc_y; i++)
        dest[i] = dest[i-1] + size[i-1];
        
      if (recsize > nRec)
        MACRO_REALLOC (recsize, nRec, rec);      /* make rec buffer bigger */
      if (recsize)  
        MPI_Gatherv (send, sendsize, MPI_INT, rec, size, dest, MPI_INT, 0,
         hgc->col_comm);
       
      /* Determine best vertex and best sum for each candidate */
      if (hgc->myProc_y == 0)     /* do following only if I am the MASTER ROW */
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
         
          if (cFLAG && bestsum > TSUM_THRESHOLD)  {
            match[bestlno]  = gno;
            match[gno]      = bestlno;
            cmatch[bestlno] = -1;         
          }
                        
          if (!cFLAG && bestsum > TSUM_THRESHOLD)  {
            cmatch[bestlno] = -1;  /* mark pending match to avoid conflicts */
            *mp++ = gno;
            *mp++ = VTX_LNO_TO_GNO (hg, bestlno);
             f = (float*) mp++;
            *f = bestsum;
            master_procs[nmaster++] = VTX_TO_PROC_X (hg, gno);
          }
        }
        
      if (cFLAG) {  /* Broadcast what we matched so far */
        MPI_Bcast (match, hg->nVtx, MPI_INT, 0, hgc->col_comm); 
      }
      MACRO_TIMER_STOP (3);    
/* KMDKMD  REMOVING THE WHILE LOOP
    }   */           /* DONE: kstart < max_nTotal loop */ 
    if (cFLAG)
      continue;      /* skip phases 3 and 4, continue rounds */ 
    
         /************************ PHASE 3: **********************************/

    MACRO_TIMER_START (4, "Matching Phase 3");   
    
    /* MASTER ROW only: send best results to candidates' owners */
    err = communication_by_plan (zz, nmaster, master_procs, NULL, 3,
     master_data, &reccnt, &recsize, &nRec, &rec, hgc->row_comm, IPM_TAG+5);
    if (err != ZOLTAN_OK)
      goto fini;  

    /* read each message (candidate id, best match id, and best i.p.) */ 
    if (hgc->myProc_y == 0) 
      for (r = rec; r < rec + 3 * reccnt; )   {
        gno    = *r++;
        vertex = *r++;
        f      = (float*) r++;
        bestsum = *f;            
                             
        /* Note: ties are broken to favor local over global matches */   
        lno =  VTX_GNO_TO_LNO (hg, gno);  
        if ((bestsum > sums [lno]) || (bestsum == sums [lno]
         && VTX_TO_PROC_X (hg, gno)    != hgc->myProc_x
         && VTX_TO_PROC_X (hg, vertex) == hgc->myProc_x))    {        
            index [lno] = vertex;
            sums  [lno] = bestsum;
        }                   
      }
    MACRO_TIMER_STOP (4);
    MACRO_TIMER_START (5, "Matching Phase 4");
                                          
    /************************ PHASE 4: ************************************/
        
    /* fill send buffer with messages. A message is two matched gno's */
    /* Note: match to self if inner product is below threshold */
    /* KDDKDD  SHOULD WE COMMUNICATE MESSAGE IF MATCHING VERTEX TO SELF??  
     * KDDKDD  GNO and VERTEX==GNO ARE BOTH ON THIS PROCESSOR. 
     * KDDKDD  CAN WE SET MATCH ARRAY APPROPRIATELY AND 
     * KDDKDD  SKIP THE MESSAGE? */
    s = send; 
    sendcnt = 0;
    if (hgc->myProc_y == 0)
      for (i = 0; i < nselect; i++)   {
        int d1, d2;
        lno = select[i];
        *s++ = gno = VTX_LNO_TO_GNO (hg, lno);
        *s++ = vertex = (sums [lno] > TSUM_THRESHOLD) ? index[lno] : gno;
            
        /* Matching gno to vertex with IP=sums[lno] */
        /* each distinct owner (gno or vertex) needs its copy of the message.*/
        d1 = VTX_TO_PROC_X (hg, gno);
        d2 = VTX_TO_PROC_X (hg, vertex);
        dest[sendcnt++] = d1;
        if (d1 != d2)  {
          *s++ = gno;
          *s++ = vertex;        
          dest[sendcnt++] = d2;
        }
      }
        
    /* send match results only to impacted parties */
    err = communication_by_plan (zz, sendcnt, dest, NULL, 2, send, &reccnt,
     &recsize, &nRec, &rec, hgc->row_comm, IPM_TAG+10);
    if (err != ZOLTAN_OK)
      goto fini;

    /* update match array with current selections */
    /* Note: -gno-1 designates an external match as a negative number */
    if (hgc->myProc_y == 0)
      for (r = rec; r < rec + 2 * reccnt; )  {   
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
    
    /* update match array to the entire column */   
    MPI_Bcast (match, hg->nVtx, MPI_INT, 0, hgc->col_comm);
    MACRO_TIMER_STOP (5);                       /* end of phase 4 */
  }                                             /* DONE: loop over rounds */
  MACRO_TIMER_START (6, "Matching Cleanup");          
  
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
      else               cmatch[i] = VTX_LNO_TO_GNO (hg, match[i]);

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

fini:
  Zoltan_Multifree (__FILE__, __LINE__, 15, &cmatch, &visit, &sums, &send,
   &dest, &size, &rec, &index, &aux, &permute, &edgebuf, &select, &rows,
   &master_data, &master_procs);
  ZOLTAN_TRACE_EXIT(zz, yo);
  return err;
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

#undef MACRO_REALLOC
#undef MACRO_TIMER_START
#undef MACRO_TIMER_STOP
#undef INNER_PRODUCT
#undef INNER_PRODUCT2
#undef ROUNDS_CONSTANT
#undef IPM_TAG
#undef HEADER_COUNT
#undef PSUM_THRESHOLD
#undef TSUM_THRESHOLD

#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif


