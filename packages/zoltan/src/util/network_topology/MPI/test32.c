#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
int i, j, k, lval, gval, numClasses;
int nextPart, rc, len, level, mine;
int num_groups, my_group, sender, receiver;
int me[NLEVELS], nprocs[NLEVELS];
int num_significant_levels=0;
int level_part_range[NLEVELS][2];
int level_number[NLEVELS];
int *classes=NULL;
int group_start, group_end;
char *errorstr;
comm_group level_info[NLEVELS];

  MPI_Init(&argc, &argv);

  for (i=0; i < NLEVELS; i++){
    rc = MPI_Comm_size(comm[i], nprocs + i);
    if (rc != MPI_SUCCESS){
      MPI_Error_string(rc, errorstr, &len);
      fprintf(stderr,"(%d) MPI_Comm_size %s : %s\n",me[0],commName[i],errorstr); 
    }
  
    rc = MPI_Comm_rank(comm[i], me + i);
    if (rc != MPI_SUCCESS){
      MPI_Error_string(rc, errorstr, &len);
      fprintf(stderr,"(%d) MPI_Comm_size %s : %s\n",me[0],commName[i],errorstr); 
    }
#ifdef DEBUG_ME
    MPI_Barrier(MPI_COMM_WORLD);
    printf("(%d) %s communicator, size %d, my rank %d\n",me[0],commName[i],nprocs[i],me[i]);
    MPI_Barrier(MPI_COMM_WORLD);
#endif
  }

  buf = (int *)malloc(sizeof(int) * nprocs[0]);
  classes = (int *)malloc(sizeof(int) * nprocs[0]);
  procToPart = (int *)malloc(sizeof(int) * nprocs[0]);
  partToProc = (int *)malloc(sizeof(int) * nprocs[0]);

  level_part_range[0][0] = 0;
  level_part_range[0][1] = nprocs[0]-1;

  for (i=0; i < NLEVELS - 1; i++){
    /*
     * classes[k] contains the rank (in level i) of the rank 0 element of element k's subcommunicator
     */
    level_down(i, classes, me, nprocs); 

    /*
     * my sub communicator will create which parts in the final partitioning?
     */
    new_part_range(classes, level_part_range[i], level_part_range[i+1], me[i], nprocs[i]);
  }

#ifdef DEBUG_ME
  MPI_Barrier(MPI_COMM_WORLD);
  for (i=0; i < nprocs[0]; i++){
    if (i == me[0]){
      printf("(%d) ranges: %s (%d %d iam %d) %s (%d %d iam %d) %s (%d %d iam %d) %s (%d %d iam %d) \n", 
        me[0],
        commName[0], level_part_range[0][0],  level_part_range[0][1], level_part_range[0][0] + me[0], 
        commName[1], level_part_range[1][0],  level_part_range[1][1], level_part_range[1][0] + me[1],
        commName[2], level_part_range[2][0],  level_part_range[2][1], level_part_range[2][0] + me[2],
        commName[3], level_part_range[3][0],  level_part_range[3][1], level_part_range[3][0] + me[3]);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
  }
#endif

  /* Figure out which levels are significant */

  num_significant_levels = 1;
  level_number[0] = 0;
  gval = 0;

  for (i=0; i < NLEVELS - 1; i++){
    if ((nprocs[i+1] == nprocs[i]) || (nprocs[i+1] == 1)){
      lval=1;   /* insignificant */
    }
    else{
      lval=0;   /* meaningful level in hierarchy */
    }

    MPI_Allreduce(&lval, &gval, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

    if (gval < nprocs[0]){
      /* next level in hierarchy is significant for at least some processes */
      level_number[num_significant_levels++] = i + 1;
    }
  }

#ifdef DEBUG_ME
  MPI_Barrier(MPI_COMM_WORLD);
  printf("(%d) %d significant levels %d %d %d %d\n",me[0], num_significant_levels,
  level_number[0], level_number[1], level_number[2], level_number[3]);
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  /* Save global info about topology */

  level = level_number[num_significant_levels-1];

  my_part_number = level_part_range[level][0] + me[level];

  MPI_Allgather(&my_part_number, 1, MPI_INT, procToPart, 1, MPI_INT, MPI_COMM_WORLD);

  for (i=0; i < nprocs[0]; i++){
    partToProc[procToPart[i]] = i;
  }

  level_info[0].comm = MPI_COMM_WORLD;
  level_info[0].name = commName[0];
  level_info[0].nGroups = 1;
  level_info[0].myGroup = 0;
  level_info[0].offsets = (int *)malloc(sizeof(int) * 2);
  level_info[0].offsets[0] = 0;
  level_info[0].offsets[1] = nprocs[0];
  level_info[0].myLocalPart = my_part_number;

  for (i=1; i < num_significant_levels; i++){

    level = level_number[i];

    memset((void *)&level_info[i], 0, sizeof(comm_group));
    mine = 0;

    MPI_Allgather(&(level_part_range[level][0]), 1, MPI_INT, buf, 1, MPI_INT, MPI_COMM_WORLD);
    memset(classes, 0, sizeof(int) * nprocs[0]);
    for (j=0; j < nprocs[0]; j++){
      classes[buf[j]]++;
    }
    memset(buf, 0, sizeof(int) * nprocs[0]);
    for (j=0, numClasses=0; j < nprocs[0]; j++){
      if (classes[j] > 0){

        if (j < level_part_range[level][0]) mine++;

        buf[numClasses++] = classes[j];   /* number of parts/procs in sub communicator */
      }
    }

    level_info[i].comm = comm[level];
    level_info[i].name = commName[level];
    level_info[i].nGroups = numClasses;
    level_info[i].myGroup = mine;
    level_info[i].offsets = (int *)malloc(sizeof(int) * (numClasses + 1));
    level_info[i].offsets[0] = 0;
    for (k=0; k < numClasses; k++){
      level_info[i].offsets[k+1] = level_info[i].offsets[k] + buf[k];
    }
    level_info[i].myLocalPart = me[level];
  }

  free(buf);
  free(classes);

/*#ifdef DEBUG_ME*/
  if (my_part_number == 0){
    for (i=0; i < num_significant_levels; i++){
      printf("Level %d (%s) has %d groups: \n",i, level_info[i].name, level_info[i].nGroups);
      for (j=0; j < level_info[i].nGroups; j++){
        printf("  Group %d has parts %d through %d\n",j, level_info[i].offsets[j], level_info[i].offsets[j+1]-1);
      }
    }
  }
/*#endif*/

  /* do some ping pong tests to compare the levels in the topology */

  for (i=1; i < num_significant_levels; i++){

    if (level_info[i].nGroups <= 1) continue;

    /* ping pong within an entity */

    sender = partToProc[0];
    receiver = partToProc[level_info[i].offsets[1] - 1];

    if (me[0] == sender){
      printf("\nPing pong test within a %s communicator (between parts %d and %d):\n",
                level_info[i].name, 0, level_info[i].offsets[1]-1);
    }

    ping_pong_test(MPI_COMM_WORLD, me[0], sender, receiver, 100);

    /* ping pong across an entity */

    if (me[0] == sender){
      printf("\nPing pong test between two %s communicators (between parts %d and %d):\n",
                level_info[i].name, 0, level_info[i].offsets[1]);
    }

    sender = partToProc[0];
    receiver = partToProc[level_info[i].offsets[1]];

    ping_pong_test(MPI_COMM_WORLD, me[0], sender, receiver, 100);
  }

  MPI_Finalize();
}

/* Following ping-pong test is converted from a program to a function by lriesen@sandia.gov */

/*                  pong.c Generic Benchmark code
 *               Dave Turner - Ames Lab - July of 1994+++
 *
 *  Most Unix timers can't be trusted for very short times, so take this
 *  into account when looking at the results.  This code also only times
 *  a single message passing event for each size, so the results may vary
 *  between runs.  For more accurate measurements, grab NetPIPE from
 *  http://www.scl.ameslab.gov/ .
 */
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

void ping_pong_test(MPI_Comm comm, int myproc, int senderRank, int receiverRank, int ntests)
{
   int size, other_proc, i, last, testno, j;
   double accuracy;
   double t0, t1, time, tsum;
   double *a, *b;
   double max_rate = 0.0, min_latency = 10e6;
   MPI_Request request, request_a, request_b;
   MPI_Status status;

   if ((myproc != senderRank) && (myproc != receiverRank)) return;

#if defined (_CRAYT3E)
   a = (double *) shmalloc (132000 * sizeof (double));
   b = (double *) shmalloc (132000 * sizeof (double));
#else
   a = (double *) malloc (132000 * sizeof (double));
   b = (double *) malloc (132000 * sizeof (double));
#endif

   for (i = 0; i < 132000; i++) {
      a[i] = (double) i;
      b[i] = 0.0;
   }

   other_proc = ((myproc == senderRank) ? receiverRank : senderRank);

/* Timer accuracy test */

   if (myproc == senderRank){
     t0 = MPI_Wtime();
     t1 = MPI_Wtime();

     while (t1 == t0) t1 = MPI_Wtime();

     accuracy = (t1 - t0) * 1000000;

     printf("Timer accuracy of ~%f usecs\n\n", accuracy);
     fflush(stdout);
   }

/* Communications between nodes 
 *   - Blocking sends and recvs
 *   - No guarantee of prepost, so might pass through comm buffer
 */

#if 0
   for (size = 8; size <= 1048576; size *= 2) {
      for (i = 0; i < size / 8; i++) {
         a[i] = (double) i;
         b[i] = 0.0;
      }
      last = size / 8 - 1;

      MPI_Barrier(comm);

      if (myproc == senderRank) {

         t0 = MPI_Wtime();
         MPI_Send(a, size/8, MPI_DOUBLE, other_proc, 0, comm);
         MPI_Recv(b, size/8, MPI_DOUBLE, other_proc, 0, comm, &status);
         t1 = MPI_Wtime();
         time = 1.e6 * (t1 - t0);

      } else if (myproc == receiverRank){

         MPI_Recv(b, size/8, MPI_DOUBLE, other_proc, 0, comm, &status);

         b[0] += 1.0;
         if (last != 0)
         b[last] += 1.0;

         MPI_Send(b, size/8, MPI_DOUBLE, other_proc, 0, comm);
      }

      MPI_Barrier(comm);

      if (myproc == senderRank){
        if ((b[0] != 1.0 || b[last] != last + 1)) {
           printf("(%d) ERROR - b[0] = %f b[%d] = %f\n", my_part_number, b[0], last, b[last]);
           exit (1);
        }
        for (i = 1; i < last - 1; i++)
           if (b[i] != (double) i)
              printf("(%d) ERROR - b[%d] = %f\n", my_part_number, i, b[i]);
        if (time > accuracy) {
           printf(" %7d bytes took %9.0f usec (%8.3f MB/sec)\n",
                       size, time, 2.0 * size / time);
           if (2 * size / time > max_rate) max_rate = 2 * size / time;
           if (time / 2 < min_latency) min_latency = time / 2;
        } else {
           printf(" %7d bytes took less than the timer accuracy\n", size);
        }
      }
   }
#endif

/* Async communications
 *   - Prepost receives to guarantee bypassing the comm buffer
 */

   if (myproc == senderRank){
      printf("\n  Asynchronous ping-pong, averages over %d ping-pong tests\n\n",ntests);
      fflush(stdout);
   }

   for (size = 8; size <= 1048576; size *= 2) {

      tsum = 0;

      for (j=0; j < ntests; j++){

        if (myproc == senderRank) {

           MPI_Recv(NULL, 0, MPI_CHAR, receiverRank, 99, MPI_COMM_WORLD, &status);   /* ok to start */
           MPI_Irecv(b, size/8, MPI_DOUBLE, other_proc, j, comm, &request);
  
           t0 = MPI_Wtime();
           MPI_Send(a, size/8, MPI_DOUBLE, other_proc, j, comm);
           MPI_Wait(&request, &status);
           tsum += MPI_Wtime() - t0;
  
        } else if (myproc == receiverRank) {

           MPI_Irecv(b, size/8, MPI_DOUBLE, other_proc, j, comm, &request);

           MPI_Send(NULL, 0, MPI_CHAR, senderRank, 99, MPI_COMM_WORLD);    /* ok to start */
  
           MPI_Wait(&request, &status);

           MPI_Send(b, size/8, MPI_DOUBLE, other_proc, j, comm);
        }
      }

      if (myproc == senderRank) {
        time = (1.e6 * tsum) / ntests;
        if (time > accuracy) {
           printf(" %7d bytes took %9.0f usec (%8.3f MB/sec)\n",
                    size, time, 2.0 * size / time);
           if (2 * size / time > max_rate) max_rate = 2 * size / time;
           if (time / 2 < min_latency) min_latency = time / 2;
        } else {
           printf(" %7d bytes took less than the timer accuracy\n", size);
        }
        fflush(stdout);
      }
   }

#if 0
/* Bidirectional communications
 *   - Prepost receives to guarantee bypassing the comm buffer
 */

   MPI_Barrier(comm);
   if (myproc == senderRank) printf("\n  Bi-directional asynchronous ping-pong\n\n");

   for (size = 8; size <= 1048576; size *= 2) {

      if ((myproc == senderRank) || (myproc == receiverRank)){
        for (i = 0; i < size / 8; i++) {
           a[i] = (double) i;
           b[i] = 0.0;
        }
        last = size / 8 - 1;

        MPI_Irecv(b, size/8, MPI_DOUBLE, other_proc, 0, comm, &request_b);
        MPI_Irecv(a, size/8, MPI_DOUBLE, other_proc, 0, comm, &request_a);
      }

      MPI_Barrier(comm);

      if ((myproc == senderRank) || (myproc == receiverRank)){
        t0 = MPI_Wtime();
        MPI_Send(a, size/8, MPI_DOUBLE, other_proc, 0, comm);
        MPI_Wait(&request_b, &status);
  
        b[0] += 1.0;
        if (last != 0)
        b[last] += 1.0;
  
        MPI_Send(b, size/8, MPI_DOUBLE, other_proc, 0, comm);
        MPI_Wait(&request_a, &status);
  
        t1 = MPI_Wtime();
        time = 1.e6 * (t1 - t0);
      }

      MPI_Barrier(comm);

      if ((myproc == senderRank) || (myproc == receiverRank)){
        if ((a[0] != 1.0 || a[last] != last + 1))
           printf("(%d) ERROR - a[0] = %f a[%d] = %f\n", my_part_number, a[0], last, a[last]);
        for (i = 1; i < last - 1; i++)
        if (a[i] != (double) i)
           printf("(%d) ERROR - a[%d] = %f\n", my_part_number, i, a[i]);
        if (myproc == senderRank && time > 0.000001) {
           printf(" %7d bytes took %9.0f usec (%8.3f MB/sec)\n",
                      size, time, 2.0 * size / time);
           if (2 * size / time > max_rate) max_rate = 2 * size / time;
           if (time / 2 < min_latency) min_latency = time / 2;
        } else if (myproc == senderRank) {
           printf(" %7d bytes took less than the timer accuracy\n", size);
        }
      }
   }
#endif

   if (myproc == senderRank)
      printf("\n Max rate = %f MB/sec  Min latency = %f usec\n",
               max_rate, min_latency);
}


