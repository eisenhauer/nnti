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

#include "phg.h"
#include <limits.h>

#define COMM_TAG 23973


typedef struct tagVCycle {
    HGraph           *hg;         /* for finer==NULL, hg and Part contains   */
    Partition         Part;       /* original hg and Part, don't delete them */  
    int              *LevelMap;   /* necessary to uncoarsen                  */
                                  /* LevelMap size = hg->nVtx 
                                     LevelMap[i] is the vtx number of the
                                     coarse vertex containing fine vtx i 
                                     on the next level.
                                     LevelMap[i] = j >= 0 if local coarse vtx
                                     j is on the same processor as i;
                                     LevelMap[i] = -gno -1 < 0 if 
                                     coarse vtx gno is on a different proc
                                     from i. */
    int               LevelCnt;   /* 2 * count of external vertices matched to
                                     vertices owned by this proc. */
                                  /* # of negative values in LevelMap. 
                                     Number of external vertices being
                                     combined into coarse vertices on this
                                     processor */
    int               LevelSndCnt; /* number of vertices being returned by
                                      by the Zoltan_comm_do_reverse(). Used to
                                      establish the receive buffer size. 
                                      Number of vertices I own that are being
                                      combined into a coarse vertex on another
                                      processor. */
    int              *LevelData;  /* buffer for external vertex information  */
                                  /* LevelData size  = LevelCnt
                                     LevelCnt/2 pairs of (my_lno, external_gno)
                                     describing matches made across procs.
                                     Proc owning my_lno will have the 
                                     coarse vtx resulting from the match
                                     and, thus, will have to send part
                                     assignment to external_gno when 
                                     uncoarsening.  */
    struct tagVCycle *finer; 
    struct Zoltan_Comm_Obj  *comm_plan;    
} VCycle; 



/****************************************************************************/
/* Routine to set function pointers corresponding to input-string options. */
int Zoltan_PHG_Set_Part_Options (ZZ *zz, PHGPartParams *hgp)
{
  int err;
  char *yo = "Zoltan_PHG_Set_Part_Options";  

  if (hgp->bal_tol < 1.0)  {
    ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Invalid PHG_BALANCE_TOLERANCE.");
    return ZOLTAN_FATAL;
  }

  /* Set reduction method. */
  hgp->matching = NULL;
  if (!(Zoltan_PHG_Set_Matching_Fn (hgp)))  {
    ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Invalid PHG_REDUCTION_METHOD.");
    return ZOLTAN_FATAL;
  }

  /* Set (serial) coarse partitioning method.  NOTE: May need parallel
   * partitioning method later if reduction to 1 proc fails              */
  hgp->CoarsePartition = Zoltan_PHG_Set_CoarsePartition_Fn(hgp, &err);
  if (err != ZOLTAN_OK)  {
      ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Invalid PHG_COARSE_PARTITIONING.");
      return ZOLTAN_FATAL;
  }

  /* Set refinement method. */
  if (!(hgp->Refinement = Zoltan_PHG_Set_Refinement_Fn(hgp->refinement_str)))  {
    ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Invalid PHG_REFINEMENT.");
    return ZOLTAN_FATAL;
  }
  return ZOLTAN_OK;
}

/******************************************************************************/



static int allocVCycle(VCycle *v)
{
  if (!v->hg || !v->hg->nVtx)
    return ZOLTAN_OK;
    
  if (!v->Part && !(v->Part = (int*) ZOLTAN_CALLOC (v->hg->nVtx, sizeof(int))))
    return ZOLTAN_MEMERR;
 
  if (!v->LevelMap 
   && !(v->LevelMap = (int*) ZOLTAN_CALLOC (v->hg->nVtx, sizeof(int))))
     return ZOLTAN_MEMERR;

  return ZOLTAN_OK;
}

/*****************************************************************************/



static VCycle *newVCycle(HGraph *hg, Partition part, VCycle *finer)
{
  VCycle *vcycle;
    
  if (!(vcycle = (VCycle*) ZOLTAN_MALLOC (sizeof(VCycle)))) 
    return NULL;
        
  vcycle->finer    = finer;
  vcycle->Part     = part;
  vcycle->LevelMap = NULL;
  vcycle->LevelData = NULL;
  vcycle->LevelCnt = 0;
  vcycle->LevelSndCnt = 0;
  vcycle->comm_plan = NULL;
  vcycle->hg       = hg ? hg : (HGraph*) ZOLTAN_MALLOC (sizeof(HGraph));
  if (!vcycle->hg)  {
    ZOLTAN_FREE (&vcycle);
    return NULL;
  }

  if (hg && (allocVCycle(vcycle) != ZOLTAN_OK))  {
    ZOLTAN_FREE (&vcycle->hg);
    ZOLTAN_FREE (&vcycle);
  }
  return vcycle;
}

/****************************************************************************/



/*  Main partitioning function for hypergraph partitioning. */
int Zoltan_PHG_Partition (
  ZZ *zz,               /* Zoltan data structure */
  HGraph *hg,           /* Input hypergraph to be partitioned */
  int p,                /* Input:  number partitions to be generated */
  float *part_sizes,    /* Input:  array of length p containing percentages
                           of work to be assigned to each partition */
  Partition parts,      /* Input:  initial partition #s; aligned with vtx 
                           arrays. 
                           Output:  computed partition #s */
  PHGPartParams *hgp,   /* Input:  parameters for hgraph partitioning. */
  int level)
{

  PHGComm *hgc = hg->comm;
  VCycle  *vcycle=NULL, *del=NULL;
  int  i, err = ZOLTAN_OK, prevVcnt=2*hg->dist_x[hgc->nProc_x];
  char *yo = "Zoltan_PHG_Partition";
  static int timer_match = -1,    /* Timers for various stages */
             timer_coarse = -1,   /* Declared static so we can accumulate */
             timer_refine = -1,   /* times over calls to Zoltan_PHG_Partition */
             timer_project = -1;

  ZOLTAN_TRACE_ENTER(zz, yo);
    
  if (!(vcycle = newVCycle(hg, parts, NULL))) {
    ZOLTAN_PRINT_ERROR (zz->Proc, yo, "VCycle is NULL.");
    return ZOLTAN_MEMERR;
  }
          
  /****** Coarsening ******/    
  while ((hg->dist_x[hgc->nProc_x] > hg->redl)
      && (hg->dist_x[hgc->nProc_x] < 0.9 * prevVcnt)
      && hg->dist_y[hgc->nProc_y] && hgp->matching ) {
      int *match = NULL;
      VCycle *coarser=NULL;
        
      prevVcnt=hg->dist_x[hgc->nProc_x];
        
      if (hgp->output_level >= PHG_DEBUG_LIST) {
          uprintf(hgc,"START %3d |V|=%6d |E|=%6d |I|=%6d %d/%s/%s/%s p=%d...\n",
                  hg->info, hg->nVtx, hg->nEdge, hg->nPins, hg->redl, hgp->redm_str,
                  hgp->coarsepartition_str, hgp->refinement_str, p);
          if (hgp->output_level > PHG_DEBUG_LIST) {
              err = Zoltan_HG_Info(zz, hg);
              if (err != ZOLTAN_OK && err != ZOLTAN_WARN)
                  goto End;
          }
      }
      if (hgp->output_level >= PHG_DEBUG_PLOT)
       Zoltan_PHG_Plot(zz->Proc, hg->nVtx, p, hg->vindex, hg->vedge, NULL,
        "coarsening plot");

      if (hgp->use_timers > 1) {
        if (timer_match < 0) 
          timer_match = Zoltan_Timer_Init(zz->ZTime, 1, "Matching");
        ZOLTAN_TIMER_START(zz->ZTime, timer_match, hg->comm->Communicator);
      }

      /* Allocate and initialize Matching Array */
      if (hg->nVtx && !(match = (int*) ZOLTAN_MALLOC (hg->nVtx * sizeof(int)))) {
        ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Insufficient memory: Matching array");
        return ZOLTAN_MEMERR;
      }
      for (i = 0; i < hg->nVtx; i++)
        match[i] = i;
        
      /* Calculate matching (packing or grouping) */
      err = Zoltan_PHG_Matching (zz, hg, match, hgp);
      if (err != ZOLTAN_OK && err != ZOLTAN_WARN) {
        ZOLTAN_FREE ((void**) &match);
        goto End;
      }
      if (hgp->use_timers > 1)
        ZOLTAN_TIMER_STOP(zz->ZTime, timer_match, hg->comm->Communicator);

      if (hgp->use_timers > 1) {
        if (timer_coarse < 0) 
          timer_coarse = Zoltan_Timer_Init(zz->ZTime, 1, "Coarsening");
        ZOLTAN_TIMER_START(zz->ZTime, timer_coarse, hg->comm->Communicator);
      }
            
      if (!(coarser = newVCycle(NULL, NULL, vcycle))) {
        ZOLTAN_FREE ((void**) &match);
        ZOLTAN_PRINT_ERROR (zz->Proc, yo, "coarser is NULL.");
        goto End;
      }

      /* Construct coarse hypergraph and LevelMap */
      err = Zoltan_PHG_Coarsening (zz, hg, match, coarser->hg, vcycle->LevelMap,
       &vcycle->LevelCnt, &vcycle->LevelSndCnt, &vcycle->LevelData, 
       &vcycle->comm_plan);
      if (err != ZOLTAN_OK && err != ZOLTAN_WARN) 
        goto End;
        
      if (hgp->use_timers > 1)
        ZOLTAN_TIMER_STOP(zz->ZTime, timer_coarse, hg->comm->Communicator);

      ZOLTAN_FREE ((void**) &match);

      if ((err=allocVCycle(coarser))!= ZOLTAN_OK)
        goto End;
      vcycle = coarser;
      hg = vcycle->hg;
  }

  if (hgp->output_level >= PHG_DEBUG_LIST) {
    uprintf(hgc, "START %3d |V|=%6d |E|=%6d |I|=%6d %d/%s/%s/%s p=%d...\n",
     hg->info, hg->nVtx, hg->nEdge, hg->nPins, hg->redl, hgp->redm_str,
     hgp->coarsepartition_str, hgp->refinement_str, p);
    if (hgp->output_level > PHG_DEBUG_LIST) {
      err = Zoltan_HG_Info(zz, hg);
      if (err != ZOLTAN_OK && err != ZOLTAN_WARN)
        goto End;
    }
  }
  if (hgp->output_level >= PHG_DEBUG_PLOT)
    Zoltan_PHG_Plot(zz->Proc, hg->nVtx, p, hg->vindex, hg->vedge, NULL,
     "coarsening plot");

  /****** Coarse Partitioning ******/
  err = Zoltan_PHG_CoarsePartition (zz, hg, p, part_sizes, vcycle->Part, hgp);
  if (err != ZOLTAN_OK && err != ZOLTAN_WARN)
    goto End;

  del = vcycle;
  /****** Uncoarsening/Refinement ******/
  while (vcycle) {
    VCycle *finer = vcycle->finer;
    hg = vcycle->hg;

    if (hgp->use_timers > 1) {
      if (timer_refine < 0) 
        timer_refine = Zoltan_Timer_Init(zz->ZTime, 1, "Refinement");
      ZOLTAN_TIMER_START(zz->ZTime, timer_refine, hg->comm->Communicator);
    }

    err = Zoltan_PHG_Refinement (zz, hg, p, vcycle->Part, hgp);
        
    if (hgp->use_timers > 1)
      ZOLTAN_TIMER_STOP(zz->ZTime, timer_refine, hg->comm->Communicator);

    if (hgp->output_level >= PHG_DEBUG_LIST)     
      uprintf(hgc, "FINAL %3d |V|=%6d |E|=%6d |I|=%6d %d/%s/%s/%s p=%d bal=%.2f cutl=%.2f\n",
              hg->info, hg->nVtx, hg->nEdge, hg->nPins, hg->redl, hgp->redm_str,
              hgp->coarsepartition_str, hgp->refinement_str, p,
              Zoltan_PHG_Compute_Balance(zz, hg, p, vcycle->Part),
              Zoltan_PHG_hcut_size_links(hgc, hg, vcycle->Part, p));

    if (hgp->output_level >= PHG_DEBUG_PLOT)
      Zoltan_PHG_Plot(zz->Proc, hg->nVtx, p, hg->vindex, hg->vedge, vcycle->Part,
       "partitioned plot");
        
    if (hgp->use_timers > 1) {
      if (timer_project < 0) 
        timer_project = Zoltan_Timer_Init(zz->ZTime, 1, "Project Up");
      ZOLTAN_TIMER_START(zz->ZTime, timer_project, hg->comm->Communicator);
    }

    /* Project coarse partition to fine partition */
    if (finer)  { 
      int *rbuffer;
            
      /* easy to undo internal matches */
      for (i = 0; i < finer->hg->nVtx; i++)
        if (finer->LevelMap[i] >= 0)
          finer->Part[i] = vcycle->Part[finer->LevelMap[i]];
          
      /* fill sendbuffer with part data for external matches I owned */    
      for (i = 0; i < finer->LevelCnt; i++)  {
        ++i;          /* skip return lno */
        finer->LevelData[i] = finer->Part[finer->LevelData[i]]; 
      }
            
      /* allocate rec buffer */
      rbuffer = NULL;
      if (finer->LevelSndCnt > 0)  {
        rbuffer = (int*) ZOLTAN_MALLOC (2 * finer->LevelSndCnt * sizeof(int));
        if (!rbuffer)    {
          ZOLTAN_PRINT_ERROR (zz->Proc, yo, "Insufficient memory.");
          return ZOLTAN_MEMERR;
        }
      }       
      
      /* get partition assignments from owners of externally matchted vtxs */  
      Zoltan_Comm_Resize (finer->comm_plan, NULL, COMM_TAG, &i);
      Zoltan_Comm_Do_Reverse (finer->comm_plan, COMM_TAG+1, 
       (char*) finer->LevelData, 2 * sizeof(int), NULL, (char*) rbuffer);

      /* process data to undo external matches */
      for (i = 0; i < 2 * finer->LevelSndCnt;)  {
        int lno, partition;
        lno       = rbuffer[i++];
        partition = rbuffer[i++];      
        finer->Part[lno] = partition;         
      }

      ZOLTAN_FREE (&rbuffer);                  
      Zoltan_Comm_Destroy (&finer->comm_plan);                   
    }
    if (hgp->use_timers > 1) 
      ZOLTAN_TIMER_STOP(zz->ZTime, timer_project, hg->comm->Communicator);

    vcycle = finer;
  }       /* while (vcycle) */
    
End:
  vcycle = del;
  while (vcycle) {
    if (vcycle->finer) {   /* cleanup by level */
      Zoltan_HG_HGraph_Free (vcycle->hg);
      Zoltan_Multifree (__FILE__, __LINE__, 4, &vcycle->Part, &vcycle->LevelMap,
                        &vcycle->LevelData, &vcycle->hg);
    }
    else                   /* cleanup top level */
      Zoltan_Multifree (__FILE__, __LINE__, 2, &vcycle->LevelMap,
                        &vcycle->LevelData);
    del = vcycle;
    vcycle = vcycle->finer;
    ZOLTAN_FREE(&del);
  }

  ZOLTAN_TRACE_EXIT(zz, yo) ;
  return err;
}
    
/****************************************************************************/



/* Calculates the cutsize of a partition by summing the weight of all edges
   which span more than one part. Time O(|I|). */
double Zoltan_PHG_hcut_size_total (PHGComm *hgc, HGraph *hg, Partition part, int p)
{
  int i, j, *netpart, *allparts;    
  double cut = 0.0, totalcut=0.0;
  char *yo = "Zoltan_PHG_hcut_size_total";

  if (!(netpart = (int*) ZOLTAN_CALLOC (hg->nEdge, sizeof(int)))) {
    ZOLTAN_PRINT_ERROR (hgc->myProc, yo, "Insufficient memory.");
    return ZOLTAN_MEMERR;
  }

  if (!hgc->myProc_x)
    if (!(allparts = (int*) ZOLTAN_CALLOC (hgc->nProc_x*hg->nEdge, sizeof(int)))) {
      ZOLTAN_PRINT_ERROR (hgc->myProc, yo, "Insufficient memory.");
      return ZOLTAN_MEMERR;
    }

  for (i = 0; i < hg->nEdge; ++i) 
    if (hg->hindex[i] >= hg->hindex[i+1])
       netpart[i] = -1;
    else  {
       j = hg->hindex[i];
       netpart[i] = part[hg->hvertex[j]];
       for (++j; j < hg->hindex[i+1]  &&  part[hg->hvertex[j]] == netpart[i]; ++j);
         if (j != hg->hindex[i+1])
           netpart[i] = -2;
    }

  MPI_Gather(netpart, hg->nEdge, MPI_INT, allparts, hg->nEdge, MPI_INT, 0, hgc->row_comm);
  ZOLTAN_FREE (&netpart);

  if (!hgc->myProc_x) { 
    for (i = 0; i < hg->nEdge; ++i) {
      int p=-1;
      for (j = 0; j < hgc->nProc_x; ++j)
        if (allparts[j*hg->nEdge+i] == -2)
          break;
        else if (allparts[j*hg->nEdge+i] >= 0) {
          if (p == -1)
            p = allparts[j*hg->nEdge+i];
          else if (p != allparts[j*hg->nEdge+i])
            break;
        }            
      if (j < hgc->nProc_x)
        cut += (hg->ewgt ? hg->ewgt[i] : 1.0);
    }
        
    ZOLTAN_FREE (&allparts);
    MPI_Reduce (&cut, &totalcut, 1, MPI_DOUBLE, MPI_SUM, 0, hgc->col_comm);
  }

  MPI_Bcast (&totalcut, 1, MPI_DOUBLE, 0, hgc->Communicator);
  return totalcut;    
}

/****************************************************************************/



/* Calculates the cutsize of a partition. For each edge it calculates the number
   of parts it spans across. This value minus one is the cutsize of this edge
   and the total cutsize is the sum of the single cutsizes. Time O(|I|). */
double Zoltan_PHG_hcut_size_links (PHGComm *hgc, HGraph *hg, Partition part, int p)
{
    int i, j, *cuts=NULL, *rescuts=NULL, *parts, nparts;
    double cut = 0.0, totalcut=0.0;
    char *yo = "Zoltan_PHG_hcut_size_links";
    
    if (hg->nEdge) {
        if (!(cuts = (int*) ZOLTAN_CALLOC (p * hg->nEdge, sizeof(int)))) {
            ZOLTAN_PRINT_ERROR(hgc->myProc, yo, "Insufficient memory.");
            return ZOLTAN_MEMERR;
        }   
        if (!hgc->myProc_x)
            if (!(rescuts = (int*) ZOLTAN_CALLOC (p * hg->nEdge, sizeof(int)))) {
                ZOLTAN_PRINT_ERROR(hgc->myProc, yo, "Insufficient memory.");
                return ZOLTAN_MEMERR;
            }
        for (i = 0; i < hg->nEdge; ++i) {
            parts = &cuts[i*p];
            for (j = hg->hindex[i]; j < hg->hindex[i+1]; ++j) 
                ++parts[part[hg->hvertex[j]]];
        }
        
        MPI_Reduce (cuts, rescuts, p*hg->nEdge, MPI_INT, MPI_SUM, 0, hgc->row_comm);
        ZOLTAN_FREE (&cuts);
    }
    
    if (!hgc->myProc_x) {
        for (i = 0; i < hg->nEdge; ++i) {
            parts = &rescuts[i*p];
            for (j = nparts = 0; j< p; ++j)
                if (parts[j])
                    ++nparts;
            if (nparts>1)
                cut +=  ((nparts-1) * (hg->ewgt ? hg->ewgt[i] : 1.0));
            else if (nparts==0)
                printf("%s Error: hyperedge %i has no vertices!\n", yo, i);
        }        
        ZOLTAN_FREE (&rescuts);

        MPI_Reduce (&cut, &totalcut, 1, MPI_DOUBLE, MPI_SUM, 0, hgc->col_comm);
    }
    MPI_Bcast (&totalcut, 1, MPI_DOUBLE, 0, hgc->Communicator);
    return totalcut;
}

/****************************************************************************/



double Zoltan_PHG_Compute_Balance (
  ZZ *zz,
  HGraph *hg,
  int p,
  Partition part
)
{
  int i;
  double *lsize_w, *size_w, max_size_w, tot_w;
  char *yo = "Zoltan_PHG_Compute_Balance";
  
  if (!hg || !hg->comm || !hg->comm->row_comm)  {
    ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Unable to compute balance");
    return 1.0;
  }  
  
  if (!(lsize_w = (double*) ZOLTAN_CALLOC (p, sizeof(double))) 
   || !(size_w  = (double*) ZOLTAN_CALLOC (p, sizeof(double)))) {
      ZOLTAN_PRINT_ERROR(zz->Proc, yo, "Insufficient memory.");
      return ZOLTAN_MEMERR;
  }
  
  if (hg->vwgt)
    for (i = 0; i < hg->nVtx; i++)
      lsize_w[part[i]] += hg->vwgt[i];
  else
    for (i = 0; i < hg->nVtx; i++)
      lsize_w[part[i]]++;
        
  MPI_Allreduce(lsize_w, size_w, p, MPI_DOUBLE, MPI_SUM, hg->comm->row_comm);
  
  max_size_w = tot_w = 0.0;
  for (i = 0; i < p; i++) {
    if (size_w[i] > max_size_w)
      max_size_w = size_w[i];
    tot_w += size_w[i];
  }

  Zoltan_Multifree(__FILE__,__LINE__, 2, &size_w, &lsize_w);

  return tot_w ? max_size_w * p / tot_w : 1.0;
}



#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif
