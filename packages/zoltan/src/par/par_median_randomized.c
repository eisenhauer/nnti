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
 *    Revision: 1.6.2.1 $
 ****************************************************************************/


#ifdef __cplusplus
/* if C++, define the rest of this header file as extern C */
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include "zoltan_mem.h"
#include "par_median_const.h"
#include "par_tflops_special_const.h"
#include "par_average_const.h"
#include "zoltan_timer.h"

#define TINY   1.0e-6
#define ABS(x) ( ((x)>0) ? (x) : (-(x)))
#define LOPART 0
#define HIPART 1
#define MEDPART 3

#define WATCH_MEDIAN_FIND

typedef struct _commStruct{
  int Tflops_Special;
  int proclower;
  int rank;
  int proc;
  int num_procs;
  MPI_Comm comm;
}commStruct;

int rb_sort_doubles_increasing(const void *a, const void *b)
{
double v1, v2;

  v1 = *(double *)a;
  v2 = *(double *)b;

  if (v1 < v2) return -1;
  else if (v1 > v2) return 1;
  else return 0;
}

static int med3(double *, int, int, int);
static int mark_median(int *, int *, int, int, double *, double, int);
static int reorder_list(double *, int, int, int *);
static void sample_partition(int, int *, double *, double, int, double *);
static int test_candidate(int *, int *, double *, int *, double *, double, int, 
   double, double, double, commStruct *, double *, double *, int *, int *);
static void mark_lo_and_hi(double, double, double *, double *, int *, int *, 
                           double *, double *, int *, commStruct *);
static int get_median_candidates(double *, int, double *, int, double, commStruct *);
static int get_3plus_candidates(int, int *, double *, double, commStruct *, double *);
static double serial_find_median2(double *, double *, int *, int, int *);

static double *msgBuf=NULL;

/************ R O U T I N E S   I N   T H I S   F I L E  **********************

       NAME                             TYPE
----------------------------------------------------------------------
	Zoltan_RB_find_median_randomized			void

******************************************************************************/

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

int Zoltan_RB_find_median_randomized(
  int Tflops_Special,   /* Flag indicating whether Tflops_Special handling 
                           of communicators should be done (to avoid memory
                           leaks in tflops' Comm_Dup and Comm_Split).        */
  double *dots,         /* array of coordinates                              */
  double *wgts,         /* array of weights associated with dots             */
  int *dotmark,         /* returned list of which side of the median
                           each dot is on:
                                0 - dot is < valuehalf
                                1 - dot is > valuehalf                       */
  int dotnum,           /* number of dots (length of three previous arrays   */
  int proc,             /* this proc number (rank)                           */
  double fractionlo,    /* fraction of weight that should be in bottom half  */
  MPI_Comm local_comm,  /* MPI communicator on which to find median          */
  double *valuehalf,    /* on entry - first guess at median (if first_guess set)                           on exit - the median value                        */
  int first_guess,      /* if set, use value in valuehalf as first guess     */
  int *counter,         /* returned for stats, # of median interations       */
  int nprocs,           /* Total number of processors (Tflops_Special)       */
  int num_procs,        /* Number of procs in set (Tflops_Special)     */
  int proclower,        /* Lowest numbered proc in set (Tflops_Special)*/
  int num_parts,        /* Number of partitions in set (Tflops_Special) */
  int wgtflag,          /* True if user supplied weights */
  double valuemin,      /* minimum value in partition (input) */
  double valuemax,      /* maximum value in partition (input) */
  double weight,        /* weight of entire partition (input) */
  double *wgtlo,        /* weight of lower partition (output) */
  double *wgthi,        /* weight of upper partition (output) */
  int    *dotlist,      /* list of active dots */
  int rectilinear_blocks,/*if set all dots with same value on same side of cut*/
  int average_cuts       /* force cut to be halfway between two closest dots. */
)
{
/* Local declarations. */
  double  wtmax;
  double  tolerance;                 /* largest single weight of a dot */
  double  targetlo;                  /* desired wt in lower half */
  double  weightlo, weighthi;        /* wt in lower/upper half of non-active */
  double  tmp_half = 0.0;
  double  invalidDot, validMax;
  double  loBound, hiBound;
  double *medians = NULL;

  int     i, numlist, ndots, ierr;
  int     markactive;                /* which side of cut is active = 0/1 */
  int     rank=0;                    /* rank in partition (Tflops_Special) */
  int     *iterationSum = NULL;       /* Sum of iterations reqd for each non-serial cut */
  int     *serialIterations = NULL;   /* Sum on each process of iterations when num_procs=1 */


  int     found_median;
  int     left, right, middle;
  int     loopCount;
  int     return_fail = 0;
  int     return_ok   = 1;

  commStruct comm;

#ifdef WATCH_MEDIAN_FIND
  char debugText[64];
  if (first_guess)
    sprintf(debugText,"(%d - %d) first guess %lf ",proclower,proclower+num_procs-1,*valuehalf);
  else
    sprintf(debugText,"(%d - %d) ",proclower,proclower+num_procs-1);
#endif
 
  if (counter){
    iterationSum = counter + 7;
    serialIterations = counter + 8;
  }

  loopCount=0;
  msgBuf = NULL;
  ierr = return_ok;

  /**************************** BEGIN EXECUTION ******************************/

  msgBuf = (double *)ZOLTAN_MALLOC(num_procs * 3 * sizeof(double));
  if (!msgBuf){
    fprintf(stderr,"Memory error");
    ierr = return_fail;
    goto End;
  }

  /*
   * intialize the dotlist array
   * while looping through, find:
   *	wtmax		- max weight on this proc
   *
   * weight = summed weight of entire partition
   * search tolerance = largest single weight (plus epsilon)
   * targetlo = desired weight in lower half of partition
   * targethi = desired weight in upper half of partition
   */
  wtmax = 0.0;
  numlist = dotnum;
  for (i = 0; i < dotnum; i++) {
    dotlist[i] = i;

    if (wgtflag)
      if (wgts[i] > wtmax) wtmax = wgts[i];
  }

  rank = proc - proclower;

  comm.Tflops_Special = Tflops_Special;
  comm.proclower = proclower;
  comm.rank = rank;
  comm.proc = proc;
  comm.num_procs = num_procs;
  comm.comm = local_comm;

  if (Tflops_Special) {
    if (wgtflag) {
      /* find tolerance (max of wtmax) */
      tolerance = wtmax;
      Zoltan_RB_max_double(&tolerance, 1, proclower, rank, num_procs, local_comm);
    }
    else 
      tolerance = 1.0;   /* if user did not supply weights, all are 1.0 */
  }
  else {
    if (wgtflag)
      MPI_Allreduce(&wtmax,&tolerance,1,MPI_DOUBLE,MPI_MAX,local_comm);
    else
      tolerance = 1.0;   /* if user did not supply weights, all are 1.0 */
  }


  tolerance *= 0.5 + TINY;  /* ctv - changed from        1.0 + TINY
               The larger tolerance allowed one side of the cut to be larger
               than the target weight by a node of largest weight (the other
               side would be smaller by the same amount).  In that case a
               node of largest weight could be moved from the side whose weight
               is larger than its target to the other side and they would both
               be in balance with the target weight.  A tolerance less than
               half of the largest weight would allow infinite looping as a
               node of largest weight was passed back and forth. */
  targetlo = fractionlo * weight;

  invalidDot = valuemax + 1.0;
  validMax = valuemax + .5;

  if (num_procs < 3)
    medians = (double *)ZOLTAN_MALLOC(5 * sizeof(double));
  else
    medians = (double *)ZOLTAN_MALLOC((num_procs+2) * sizeof(double));

  if (!Tflops_Special || num_procs > 1) { /* don't need to go thru if only
                                             one proc with Tflops_Special. 
                                             Input argument Tflops_Special 
                                             should be 0 for
                                             serial partitioning. */

    weightlo = weighthi = 0.0;
    found_median = 0;

    if (first_guess){                /* If we have a first guess, try it */
      tmp_half = *valuehalf;

      found_median = test_candidate(&numlist, dotlist, dots, dotmark, wgts, weight,
                         rectilinear_blocks, tolerance, targetlo,
                         tmp_half, &comm, &weightlo, &weighthi, &markactive, &loopCount);

      if (!found_median){
        /* We'll generate random candidates and continue search in while loop below */

        /* TODO - if we have a first guess, we should generate candidates close
         *   to the first guess and do a binary search of those.  Otherwise, 
         *   unless first guess is median, we're not really using this info. 
         */
        medians[0] = valuemin;
        medians[1] = valuemax;
        left = 0;
        right = 1;
      }
    }
    else{            /* otherwise begin with the local median on each process */
      if (dotnum > 0){
        if (num_procs == 1)
          tmp_half = serial_find_median2(dots, wgts, dotlist, dotnum, &loopCount);
        else
          tmp_half = serial_find_median2(dots, wgts, dotlist, dotnum, NULL);
      }
      else{
        tmp_half = invalidDot;
      }
  
      /* Create sorted array of unique local median values.  True global median is 
       * between the minimum and maximum of these local medians. 
       */
  
      ndots = get_median_candidates(&tmp_half, 1, medians, num_procs, validMax, &comm);
  
      left = 0;
      right = ndots-1;

      if (ndots == 1){ /*   If only one dot - it must be median */
      
        /* Write dotmark array, compute weightlo, weighthi */
        test_candidate(&numlist, dotlist, dots, dotmark, wgts, weight,
                           rectilinear_blocks, tolerance, targetlo,
                           medians[0], &comm, &weightlo, &weighthi, &markactive, &loopCount);
  
        found_median = 1;
        tmp_half = medians[0];
      }
      else if (ndots == 2){
         /* Median is between medians[0] and medians[1], or it's one
          * or the other.  Call test_candidate to mark dots, pare active list.
          */
  
        found_median = test_candidate(&numlist, dotlist, dots, dotmark, 
                           wgts, weight, rectilinear_blocks, 
                           tolerance, targetlo,
                           medians[0], &comm, &weightlo, &weighthi, &markactive, &loopCount);
  
        if (found_median){
          tmp_half = medians[0];
        }
        else{
          found_median = test_candidate(&numlist, dotlist, dots, dotmark, 
                           wgts, weight, rectilinear_blocks, 
                           tolerance, targetlo,
                           medians[1], &comm, &weightlo, &weighthi, &markactive, &loopCount);
  
          /* If candidate was not the median, we'll search between medians[0] and 
           * medians[1] in while loop below */

          if (found_median){
            tmp_half = medians[1];
          }
        }
      }
      else{

        /* to save time in test_candidate, mark all dots we know can not be median */

        mark_lo_and_hi(medians[0], medians[ndots-1], &weightlo, &weighthi,
                       &numlist, dotlist, dots, wgts, dotmark, &comm);
      }
    }

    /* Begin with a binary search through candidate medians if we have them. 
     * Then select random candidates after this search narrows down the range.
     */

    loBound = medians[left];          /* median > loBound */
    hiBound = medians[right];         /* median < hiBound */

    while (!found_median) {

      while (left < (right-1)){

        middle = (left + right) >> 1;
        tmp_half = medians[middle];

        found_median = test_candidate(&numlist, dotlist, dots, dotmark, wgts, weight,
                         rectilinear_blocks, tolerance, targetlo,
                         tmp_half, &comm, &weightlo, &weighthi, &markactive, &loopCount);

        if (found_median) break;

        /* move to left or right half of array of candidates */
        if (markactive == HIPART){
          left = middle;
          loBound = medians[left];
        }
        else{
          right = middle;
          hiBound = medians[right];
        }
      }  /* test next candidate */

      if (!found_median){

        /* Quickly generate at least 3 new candidates within active dots. */
        /* Normally we're looking for one candidate from each process.    */

        ndots = get_3plus_candidates(numlist, dotlist, dots, valuemax, &comm, medians+1);

        if (ndots == 1){   /* must be median, mark dots & compute weights */

          found_median = test_candidate(&numlist, dotlist, dots, dotmark, wgts, weight,
                         rectilinear_blocks, tolerance, targetlo,
                         medians[0], &comm, &weightlo, &weighthi, &markactive, &loopCount);
          tmp_half = medians[0];
          found_median = 1;  /* shouldn't need this */
        }
        else if (ndots == 2){   /* one must be the median */

          found_median = test_candidate(&numlist, dotlist, dots, dotmark, wgts, weight,
                         rectilinear_blocks, tolerance, targetlo,
                         medians[0], &comm, &weightlo, &weighthi, &markactive, &loopCount);
          if (found_median){
            tmp_half = medians[0];
          }
          else{
            found_median = test_candidate(&numlist, dotlist, dots, dotmark, wgts, weight,
                         rectilinear_blocks, tolerance, targetlo,
                         medians[1], &comm, &weightlo, &weighthi, &markactive, &loopCount);
            tmp_half = medians[1];
            found_median = 1;  /* shouldn't need this */
          }
        }
        else{
          ndots += 2;
          medians[0] = loBound;
          medians[ndots-1] = hiBound;
          left = 0;
          right = ndots-1;
        }
      }
    }
  }
  else { /* if one processor set all dots to 0 (Tflops_Special) */
    for (i = 0; i < numlist; i++)
      dotmark[i] = 0;
    weightlo = weight;
    weighthi = 0.;
    tmp_half = valuemax;
  }

  /* found median */
  *valuehalf = tmp_half;

  if (average_cuts) 
    *valuehalf = Zoltan_RB_Average_Cut(Tflops_Special, dots, dotmark, dotnum,
                                       num_procs, rank, proc, local_comm,
                                       *valuehalf);

  *wgtlo = weightlo;
  *wgthi = weighthi;

#ifdef WATCH_MEDIAN_FIND
  if ((num_procs>1) && (rank==0)){
    fprintf(stderr,"%s loop count %d interval size %d median (%lf - %lf) %lf\n",
           debugText, loopCount, dotnum, valuemin, valuemax, *valuehalf);
  }
#endif


  if (counter != NULL){
    if (num_procs > 1){
      /* Total iterations on each process (excluding serial)  */
      (*counter) += loopCount;

      /* Total iterations for each cut (excluding serial calculations) */
      if (rank==0) (*iterationSum) += loopCount;
    }
    else{
      /* Total iterations on each process for serial median find */
      (*serialIterations) += loopCount;
    }
  }


End:

  ZOLTAN_FREE(&medians);
  ZOLTAN_FREE(&msgBuf);

  return ierr;
}
/*
** index of the array element with median value of three values
*/
static int med3(double *v, int a, int b, int c)
{
double v1 = v[a];
double v2 = v[b];
double v3 = v[c];

  if (((v1 <= v2) && (v1 >= v3)) ||
      ((v1 >= v2) && (v1 <= v3))){
    return a;
  }
  else if (((v2 <= v1) && (v2 >= v3)) ||
      ((v2 >= v1) && (v2 <= v3))){
    return b;
  }
  else{
    return c;
  }
}
static int mark_median(int *dotlist, int *dotmark, 
        int start, int nmeds, double *dots, double med, int mark)
{
int i, j;
int count=0;

  for (j = start; ; j++) {
    i = dotlist[j];
    if (dots[i] == med) { 
      dotmark[i] = mark;
      if (++count == nmeds) break;
    }
  }
  return j+1;
}
#define exchange(a, b) {   \
  tempVal = vals[a];    \
  vals[a] = vals[b];       \
  vals[b] = tempVal;    \
  if (idxList){            \
  tempInt = idxList[a];    \
  idxList[a] = idxList[b]; \
  idxList[b] = tempInt; } }

static int reorder_list(double *vals, int len, int pivotIdx, int *idxList)
{
int l, r, i, j;
double p = vals[pivotIdx];
double tempVal;
int tempInt;

  i = l = 0;
  j = r = len-1;

  /* error here */
  exchange(l, pivotIdx);
  if (vals[r] >= p){
    exchange(r, l);
  }
  while (i < j){
    exchange(i, j);
    while (vals[++i] < p);
    while ( (j > l) && (vals[--j] >= p));
  }

  if (vals[l] == p){
    exchange(l, j);
  }
  else{
    j++;
    exchange(j, r);
  }

  return (j-l);
}

/* Get any three dots from list of active dots.  May not be unique. */
/* If there are no active dots, all three have "invalidDot" value.  */
/* Dots are sorted increasing.                                      */
/* If forceUnique=1, will try to find three unique if possible.     */

static void sample_partition(int numlist, int *dotlist, double *dots, 
                            double invalidDot, int forceUnique,
                            double *samples)
{
int i, j;
int numUnique=0;
int mid = numlist >> 1;

  /* get three "random" dots in increasing order */

  samples[0] = samples[1] = samples[2] = invalidDot;

  if (numlist >= 1){
    samples[0] = dots[dotlist[0]];
    samples[1] = dots[dotlist[mid]];
    samples[2] = dots[dotlist[numlist-1]];

    qsort(samples, 3, sizeof(double), rb_sort_doubles_increasing);

    numUnique = 1;
    if (samples[1] > samples[0]) numUnique++;
    if (samples[2] > samples[1]) numUnique++;

    if ((numUnique < 3) && (numlist > 3) && forceUnique){
      /* try to get three unique dots */
      for (j=1; j < numlist-1; j++){
        if ((samples[1] != samples[0]) && (samples[2] != samples[1]) &&
            (samples[2] != samples[0])) break;
        i = dotlist[j];
        if ((samples[1] == samples[0]) &&
            (dots[i] != samples[1]) && (dots[i] != samples[2]))
           samples[0] = dots[i];
        else if ((samples[2] == samples[0]) &&
            (dots[i] != samples[1]) && (dots[i] != samples[2]))
           samples[0] = dots[i];
        else if ((samples[2] == samples[1]) &&
            (dots[i] != samples[1]) && (dots[i] != samples[0]))
           samples[2] = dots[i];
      }
      qsort(samples, 3, sizeof(double), rb_sort_doubles_increasing);
    }
  }
} 

/*
 * Given a candidate median value, sum up weights to left and right of it.
 *
 * If candidate is not the median, update the dotlist array (indices of active
 * dots) to include the dots to the right or left only of the candidate.  Update
 * numlist (the size of the dotlist array).
 *
 * Update weightlo, weighthi and markactive (whether search moves to left or right).
 *
 * Return 1 if candidate is median, 0 if it's not.
 *
 * Write dotmark array with LOPART and HIPART markers.
 */
static int test_candidate(int *numlist, int *dotlist, double *dots, int *dotmark, 
                          double *wgts, double totalweight, int rectilinear_blocks,
                          double tolerance, double targetlo,
                          double candidate, commStruct *comm,
                          double *weightlo, double *weighthi, int *markactive, int *count)
{
int i, j, k, ndots;
double global[3], local[3];
double totallo, totalmed, totalhi;
double leftTotal, rightTotal, diff1, diff2, wtupto, wtsum;
int found_median = 0;
int Tflops_Special = comm->Tflops_Special;
int proclower = comm->proclower;
int rank = comm->rank;
int proc = comm->proc;
int num_procs = comm->num_procs;
MPI_Comm local_comm = comm->comm;
double mylo=0.0, mymed=0.0, myhi=0.0;
int countlo=0, countmed=0, counthi=0, indexmed=-1;

  if (count) (*count)++;

  for (j = 0; j < *numlist; j++) {
    i = dotlist[j];
    if (dots[i] < candidate) {
      mylo += wgts[i];
      dotmark[i] = LOPART;
      countlo++;
    }
    else if (dots[i] == candidate) {
      mymed += wgts[i];
      countmed++;
      dotmark[i] = MEDPART;
      if (indexmed < 0) indexmed = j;
    }
    else{
      myhi += wgts[i];
      dotmark[i] = HIPART;
      counthi++;
    }
  }

  if (Tflops_Special) {
    global[0] = mylo;
    global[1] = mymed;
    global[2] = myhi;
    Zoltan_RB_sum_double(global, 3, proclower, rank, num_procs, local_comm);
  }
  else {
    local[0] = mylo;
    local[1] = mymed;
    local[2] = myhi;
    global[0] = global[1] = global[2] = 0.0;
    MPI_Allreduce(local, global, 3, MPI_DOUBLE, MPI_SUM, local_comm);
  }

  totallo = global[0];
  totalmed = global[1];
  totalhi = global[2];

  leftTotal = *weightlo + totallo;
  rightTotal = *weighthi + totalhi;

  if (leftTotal + totalmed < targetlo){  /* left half too small */
    *weightlo = leftTotal + totalmed;
    if (indexmed >= 0){
      /* candidate elements go in the left half */
      mark_median(dotlist, dotmark, indexmed, countmed, dots, candidate, LOPART);
    }

    if (targetlo - *weightlo <= tolerance){  /* close enough */
      *weighthi = totalweight - *weightlo;
      found_median = 1;
    }
    else{
      *markactive = HIPART;   /* median value is in the right half */
    }
  }
  else if (leftTotal > targetlo){          /* left half is too large */
    *weighthi = rightTotal + totalmed;
    if (indexmed >= 0){
      /* candidate elements go in the right half */
      mark_median(dotlist, dotmark, indexmed, countmed, dots, candidate, HIPART);
    }
    if (leftTotal - targetlo <= tolerance){  /* close enough */
      *weightlo = totalweight - *weighthi;
      found_median = 1;
    }
    else{
      *markactive = LOPART;   /* median value is in the left half */
    }
  }
  else{                                     /* median is candidate */
    *weightlo = leftTotal;
    *weighthi = rightTotal;
    found_median = 1;

    diff1 = targetlo - (leftTotal + totalmed);
    diff2 = targetlo - leftTotal;

    if (Tflops_Special){
      local[0] = (double)countmed;
      Zoltan_RB_sum_double(local, 1, proclower, rank, num_procs, local_comm);
      ndots = (int)local[0];
    }
    else{
      MPI_Allreduce(&countmed, &ndots, 1, MPI_INT, MPI_SUM, local_comm);
    }

    if ((ndots == 1) ||         /* there's only one element with median value */
        (rectilinear_blocks)){  /* all median elements have to stay together */

      if (ABS(diff1) < ABS(diff2)){
        if (indexmed >= 0){
          mark_median(dotlist, dotmark, indexmed, countmed, dots, candidate, LOPART);
        }
        *weightlo += totalmed;
      }
      else{
        if (indexmed >= 0){
          mark_median(dotlist, dotmark, indexmed, countmed, dots, candidate, HIPART);
        }
        *weighthi += totalmed;
      }
    }
    else{ /* divide median elements between left & right for best balance */

      if (Tflops_Special){
        Zoltan_RB_scan_double(&mymed, &wtupto, 1, local_comm, proc, rank, num_procs);
      }
      else{
        MPI_Scan(&mymed, &wtupto, 1, MPI_DOUBLE, MPI_SUM, local_comm);
      }
      mylo = myhi = 0;

      if (indexmed >= 0){

        if (leftTotal + wtupto - mymed >= targetlo - tolerance){
          /* all my median elements can go on the right side */
          mark_median(dotlist, dotmark, indexmed, countmed, dots, candidate, HIPART);
          myhi = mymed;
        }
        else if (leftTotal + wtupto <= targetlo + tolerance){
          /* all my median elements can go on the left side */
          mark_median(dotlist, dotmark, indexmed, countmed, dots, candidate, LOPART);
          mylo = mymed;
        }
        else {
          /* my median elements are split between left and right sides */
          j = indexmed;
          wtsum = leftTotal + wtupto - mymed;
          k = 0;
          for (i=0; i<countmed; i++){
            j = mark_median(dotlist, dotmark, j, 1, dots, candidate, LOPART);
            mylo += wgts[dotlist[j-1]];
            k++;
            if (wtsum + mylo >= targetlo - tolerance){
              break;
            }
          }
          for (i=0; i < countmed-k; i++){
            j = mark_median(dotlist, dotmark, j, 1, dots, candidate, HIPART);
            myhi += wgts[dotlist[j-1]];
          }
        }
      }

      if (Tflops_Special) {
        global[0] = mylo;
        global[1] = myhi;
        Zoltan_RB_sum_double(global, 2, proclower, rank, num_procs, local_comm);
      }
      else {
        local[0] = mylo;
        local[1] = myhi;
        global[0] = global[1] = 0.0;
        MPI_Allreduce(local, global, 2, MPI_DOUBLE, MPI_SUM, local_comm);
      }

      *weightlo += global[0];
      *weighthi += global[1];
    }
  }
  if (!found_median){   /* rewrite active list for next time */
    k = 0;
    for (j = 0; j < *numlist; j++) {
      i = dotlist[j];
      if (dotmark[i] == *markactive) dotlist[k++] = i;
    }
    *numlist = k;
  }

  return found_median;
}

/* Mark dots below loBound as LOPART and dots above hiBound as HIPART 
 * and remove them from the active list, and accumulate their weights
 * in weightlo and weighthi.
 */

static void mark_lo_and_hi(double loBound, double hiBound,
               double *weightlo, double *weighthi,
               int *numlist, int *dotlist, double *dots, double *wgts,  
               int *dotmark, commStruct *comm)
{
int i, j, k;
double wlo=0.0;
double whi=0.0;
double local[2], global[2];
int Tflops_Special = comm->Tflops_Special;
int proclower = comm->proclower;
int rank = comm->rank;
int num_procs = comm->num_procs;
MPI_Comm local_comm = comm->comm;

  k = 0;
  for (j=0; j < *numlist; j++){
    i = dotlist[j];
    if (dots[i] < loBound){
      wlo += wgts[i];
      dotmark[i] = LOPART;
    }
    else if (dots[i] > hiBound){
      whi += wgts[i];
      dotmark[i] = HIPART;
    }
    else{
      if (k < j) dotlist[k] = i;
      k++;
    }
  }

  *numlist = k;

  if (Tflops_Special) {
    global[0] = wlo;
    global[1] = whi;
    Zoltan_RB_sum_double(global, 2, proclower, rank, num_procs, local_comm);
  }
  else {
    local[0] = wlo;
    local[1] = whi;
    global[0] = global[1] = 0.0;
    MPI_Allreduce(local, global, 2, MPI_DOUBLE, MPI_SUM, local_comm);
  }

  *weightlo += global[0];
  *weighthi += global[1];
}

/* We want no more than nmedians unique dot values, sorted in increasing */
/* order.  Each process supplies nmymed dot values, some or all could be */
/* duplicates or invalid.  nmymed must be the same on all processes.     */
/* nmedians must be at least num_procs.                                  */

/* For now we assume "nmymed" is 3 or less.  Can change this.            */

static int get_median_candidates(double *mymed, int nmymed, 
                                 double *medians, int nmedians,
                                 double validmax, commStruct *comm)
{
int ndots, nvalid, i;
int Tflops_Special = comm->Tflops_Special;
int proclower = comm->proclower;
int rank = comm->rank;
int num_procs = comm->num_procs;
MPI_Comm local_comm = comm->comm;
double *nextval;

  if (nmymed > 3){
    fprintf(stderr,"Rewrite so preallocated msgBuf is larger (%d)\n",nmymed);
    exit(0);
  }

  if (Tflops_Special){
    nextval = msgBuf;
    for (i=0; i<nmymed; i++){
      Zoltan_RB_gather_double(mymed[i], nextval, proclower, 0, rank, num_procs, local_comm);
      Zoltan_RB_bcast_doubles(nextval, num_procs, proclower, 0, rank, num_procs, local_comm);
      nextval += num_procs;
    }
  }
  else{
    MPI_Allgather(mymed, nmymed, MPI_DOUBLE, msgBuf, nmymed, MPI_DOUBLE, local_comm);
  }

  /* make list of valid dots only */
  nvalid = 0;
  for (i=0; i<num_procs*nmymed; i++){
    if (msgBuf[i] < validmax){
      if (nvalid < i) msgBuf[nvalid] = msgBuf[i];
      nvalid++;
    }
  }

  qsort(msgBuf, nvalid, sizeof(double), rb_sort_doubles_increasing);

  /* make list unique, length nmedians at most */
  ndots = 1;
  medians[0] = msgBuf[0];
  for (i=1; i<nvalid; i++){
    if (msgBuf[i] > medians[ndots-1]){
      medians[ndots] = msgBuf[i];
      ndots++;
      if (ndots == nmedians) break;
    }
  }

  return ndots;
}

static int get_3plus_candidates(int numlist, int *dotlist, double *dots,
                                double valuemax, commStruct *comm,
                                double *candidates)
{
double vals[3];
double invalidDot = valuemax + 1.0;
double validmax = valuemax + .5;
int ndots;
int num_procs = comm->num_procs;

  /* Quickly generate at least 3 unique candidates within active dots (if possible). */

  if (num_procs < 3){
    /* get 3 unique dots from local active dots, if possible */
    sample_partition(numlist, dotlist, dots, invalidDot, 1, vals);

    /* get 3 unique dots from global active dots, if possible */
    ndots = get_median_candidates(vals, 3, candidates, 3, validmax, comm);
  }
  else{
    /* try making candidate list with one dot from each process */
    sample_partition(numlist, dotlist, dots, invalidDot, 0, vals);
    ndots = get_median_candidates(vals + 1, 1, candidates, num_procs, validmax, comm);

    if (ndots < 3){
      /* try again, using more dots from each process */
      sample_partition(numlist, dotlist, dots, invalidDot, 1, vals);
      ndots = get_median_candidates(vals, 3, candidates, num_procs, validmax, comm);
    }
  }
  return ndots;
}


/* Doesn't rewrite dots array.  Takes array of active dots. */
static double serial_find_median2(double *dots, double *wgts, int *dotidx, int dotnum, int *count)
{
int lb, ub, idx, i;
int *widx=NULL;
double median=-1, lowBal=0.0, upBal=0.0;
double pivotBal, w1, w2;
double *dotCopy = NULL;

  if (dotnum < 1) return 0;
  else if (dotnum < 2) return dots[dotidx[0]];

  dotCopy = (double *)ZOLTAN_MALLOC(dotnum * sizeof(double));

  if (wgts){
    widx = (int *)ZOLTAN_MALLOC(dotnum * sizeof(int));
    for (i=0; i<dotnum; i++){
      widx[i] = dotidx[i];  /* map from dot to its weight */
      dotCopy[i] = dots[dotidx[i]];
    }
  }
  else{
    for (i=0; i<dotnum; i++){
      dotCopy[i] = dots[dotidx[i]];
    }
  }

  lb = 0;
  ub = dotnum-1;
  while (lb < ub){

    if (count) (*count)++;

    /* choose a random pivot value */
    idx = med3(dotCopy, lb, ub, (lb+ub) >> 1);

    /* rearrange list around pivot, get index of pivot */
    if (widx){
      idx = reorder_list(dotCopy+lb, ub-lb+1, idx-lb, widx+lb);
    }
    else{
      idx = reorder_list(dotCopy+lb, ub-lb+1, idx-lb, NULL);
    }

    idx += lb;   /* relative to dotCopy[0] */

    /* sum the weights in both halves */

    if (wgts){
      w1 = lowBal;
      w2 = upBal;
      for (i=lb; i<idx; i++){
        w1 += wgts[widx[i]];
      }
      for (i=idx+1; i<=ub; i++){
        w2 += wgts[widx[i]];
      }
      pivotBal = wgts[widx[idx]];
    }
    else{
      /* weights are all 1.0 */
      w1 = idx;
      pivotBal = 1.0;
      w2 = dotnum - idx - 1;
    }

    if (w1 >= (pivotBal + w2)){
      ub = idx - 1;
      if (wgts){
        upBal = pivotBal + w2;
      }
    }
    else if ((w1 + pivotBal) >= w2 ){
      median = dotCopy[idx];
      break;
    }
    else{
      lb = idx + 1;
      if (wgts){
        lowBal = w1 + pivotBal;
      }
    }
  }

  if (lb == ub){
    median = dotCopy[lb];
  }

  ZOLTAN_FREE(&widx);
  ZOLTAN_FREE(&dotCopy);


  return median;
}


#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif
