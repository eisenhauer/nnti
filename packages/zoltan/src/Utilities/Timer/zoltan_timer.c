/*****************************************************************************
 * Zoltan Library for Parallel Applications                                  *
 * Copyright (c) 2000,2001,2002, Sandia National Laboratories.               *
 * This software is distributed under the GNU Lesser General Public License. *
 * For more info, see the README file in the top-level Zoltan directory.     *
 *****************************************************************************/
/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    $Revision$
 ****************************************************************************/


#include "zoltan_timer.h"
#include "zoltan_types.h"
#include "zoltan_util.h"
#include "zoltan_mem.h"

#ifdef __cplusplus
/* if C++, define the rest of this header file as extern C */
extern "C" {
#endif

/****************************************************************************/
/*
 * Functions that implement a Timer "class," creating a Timer object
 * with start, stop and print functions.
 *
 * This code was designed to be a stand-alone utility, relying only
 * on Zoltan_Time, Zoltan error codes, Zoltan utilities, and libzoltan_mem.a.
 * Some rearranging of the Zoltan_Time files is necessary to truly make
 * this utility a standalone one.
 */

/****************************************************************************/
/* Number of timers initially in Timer object. */
#define INITLENGTH 30   

/* Length of character strings naming each timer. */
/* If you change this constant, change the string format  */
/* in Zoltan_Timer_Print, too. */
#define MAXNAMELEN 23   

/* Flag indicating whether a timer is in use. */
#define INUSE 1

/* Flag indicating whether a timer is running. */
#define RUNNING 2

#define FATALERROR(yo, str) \
  { \
    int proc; \
    MPI_Comm_rank(MPI_COMM_WORLD, &proc); \
    ZOLTAN_PRINT_ERROR(proc, yo, str); \
    return ZOLTAN_FATAL; \
  }
  

/* Macro to ensure that a Timer object is non-NULL */
#define TESTTIMER(zt, yo) \
  if ((zt) == NULL) FATALERROR(yo, "NULL Zoltan_Timer")

/* Macro to ensure that a given timer index is valid. */
#define TESTINDEX(zt, ts_idx, yo) \
  if ((ts_idx) >= (zt)->NextTimeStruct) FATALERROR(yo, "Invalid Timer Index")


/****************************************************************************/
/* Structure that implements an individual timer. */
typedef struct TimeStruct {
  double Start_Time;      /* Most recent start time; 
                             set by Zoltan_Timer_Start */
  double Stop_Time;       /* Most recent end time;
                             set by Zoltan_Timer_Stop */
  char Start_File[MAXNAMELEN+1];  /* Filename for most recent Start */
  char Stop_File[MAXNAMELEN+1];   /* Filename for most recent Stop */
  int Start_Line;         /* Line # in Start_File for most recent Start */
  int Stop_Line;          /* Line # in Stop_File for most recent Stop */
  double My_Tot_Time;     /* Sum of stop_time-start_time over all invocations
                             of this timer */
  double Global_Tot_Time; /* Sum of max stop_time-start_time over all procs
                             (in communicator) over all invocations of this 
                             timer */
  int Use_Barrier;        /* Flag indicating whether to perform a barrier
                             operation before starting the timer. */
  int Status;             /* Flag indicating status of TimeStruct:
                                > 0  -->  In Use
                                > 2  -->  Running */
  char Name[MAXNAMELEN+1];/* String associated (and printed) with timer info */
} ZTIMER_TS;

/* Timer object consisting of many related timers. 
 * Applications access this structure. */
typedef struct Zoltan_Timer {
  int Timer_Flag;         /* Zoltan Timer_Flag flag passed to Zoltan_Time */
  int Length;             /* # of entries allocated in Times */
  int NextTimeStruct;     /* Index of next unused TimeStruct */
  ZTIMER_TS *Times;       /* Array of actual timing data -- individual timers */
} ZTIMER;

/****************************************************************************/
ZTIMER *Zoltan_Timer_Create(
  int timer_flag
)
{
/* Allocates a Timer object for the application; returns a pointer to it. 
 * Does not start any timers.
 */

ZTIMER *zt;
int i;

  zt = (ZTIMER *) ZOLTAN_MALLOC(sizeof(ZTIMER));
  zt->Times = (ZTIMER_TS *) ZOLTAN_MALLOC(sizeof(ZTIMER_TS) * INITLENGTH);
  zt->Timer_Flag = timer_flag;
  zt->Length = INITLENGTH;
  zt->NextTimeStruct = 0;

  for (i = 0; i < zt->Length; i++) 
    zt->Times[i].Status = 0;

  return zt;
}

/****************************************************************************/
int Zoltan_Timer_Init(
  ZTIMER *zt,           /* Ptr to Timer object */
  int use_barrier,      /* Flag indicating whether to perform a 
                           barrier operation before starting the
                           timer. */
  char *name            /* Name of this timer */
)
{
/* Function that returns the index of the next available Timer timer. */
int ret;
static char *yo = "Zoltan_Timer_Init";

  TESTTIMER(zt, yo);
  
  ret = zt->NextTimeStruct++;

  if (ret >= zt->Length) {
    /* Realloc -- need more individual timers */
    zt->Length += INITLENGTH;
    zt->Times = (ZTIMER_TS *) ZOLTAN_REALLOC(zt->Times, 
                                             zt->Length * sizeof(ZTIMER_TS));
  }

  Zoltan_Timer_Reset(zt, ret, use_barrier, name);

  return ret;
}


/****************************************************************************/
int Zoltan_Timer_Reset(
  ZTIMER *zt,
  int ts_idx,            /* Index of the timer to reset */
  int use_barrier,       /* Flag indicating whether to perform a 
                            barrier operation before starting the
                            timer. */
  char *name             /* Name of this timer */
)
{
/* Initialize a timer for INUSE; reset its values to zero. */
static char *yo = "Zoltan_Timer_Reset";
ZTIMER_TS *ts;

  TESTTIMER(zt, yo);
  TESTINDEX(zt, ts_idx, yo);

  ts = &(zt->Times[ts_idx]);

  ts->Status = INUSE;
  ts->Start_Time = 0.;
  ts->Stop_Time = 0.;
  ts->My_Tot_Time = 0.;
  ts->Global_Tot_Time = 0.;
  ts->Use_Barrier = use_barrier;
  strncpy(ts->Name, name, MAXNAMELEN);
  ts->Name[MAXNAMELEN] = '\0';
  ts->Start_File[0] = '\0';
  ts->Start_Line = -1;
  ts->Stop_File[0] = '\0';
  ts->Stop_Line = -1;

  return ZOLTAN_OK;
}

/****************************************************************************/
int Zoltan_Timer_ChangeFlag(
  ZTIMER *zt,
  int timer
)
{
static char *yo = "Zoltan_Timer_ChangeFlag";

  TESTTIMER(zt, yo);
  zt->Timer_Flag = timer;
  return ZOLTAN_OK;
}

/****************************************************************************/
int Zoltan_Timer_Start(
  ZTIMER *zt,            /* Ptr to Timer object */
  int ts_idx,            /* Index of the timer to use */
  MPI_Comm comm,         /* Communicator to use for synchronization, 
                            if requested */
  char *filename,        /* Filename of file calling the Start */
  int lineno             /* Line number where Start was called */
)
{
ZTIMER_TS *ts;
static char *yo = "Zoltan_Timer_Start";

  TESTTIMER(zt, yo);
  TESTINDEX(zt, ts_idx, yo);

  ts = &(zt->Times[ts_idx]);
  if (ts->Status > 2)  {
    char msg[256];
    sprintf(msg, 
            "Cannot start timer %d at %s:%d; timer already running from %s:%d.",
            ts_idx, filename, lineno, ts->Start_File, ts->Start_Line);
    FATALERROR(yo, msg)
  }

  ts->Status += RUNNING;
  if (ts->Use_Barrier)
    MPI_Barrier(comm);

  ts->Start_Time = Zoltan_Time(zt->Timer_Flag);
  strncpy(ts->Start_File, filename, MAXNAMELEN);
  ts->Start_Line = lineno;
  return ZOLTAN_OK;
}

/****************************************************************************/

int Zoltan_Timer_Stop(
  ZTIMER *zt,            /* Ptr to Timer object */
  int ts_idx,            /* Index of the timer to use */
  MPI_Comm comm,         /* Communicator to use for AllReduce */
  char *filename,        /* Filename of file calling the Stop */
  int lineno             /* Line number where Stop was called */
)
{
/* Function to stop a timer and accrue its information */
ZTIMER_TS *ts;
static char *yo = "Zoltan_Timer_Stop";
double my_time, global_time;

  TESTTIMER(zt, yo);
  TESTINDEX(zt, ts_idx, yo);

  ts = &(zt->Times[ts_idx]);
  if (ts->Status < 2) {
    if (ts->Stop_Line == -1)
      FATALERROR(yo, "Cannot stop timer; timer never started.")
    else {
      char msg[256];
      sprintf(msg, 
              "Cannot stop timer %d at %s:%d; "
              "timer already stopped from %s:%d.",
              ts_idx, filename, lineno, ts->Stop_File, ts->Stop_Line);
      FATALERROR(yo, msg)
    }
  }

  ts->Status -= RUNNING;
  ts->Stop_Time = Zoltan_Time(zt->Timer_Flag);
  ts->Stop_Line = lineno;
  strncpy(ts->Stop_File, filename, MAXNAMELEN);
  my_time = ts->Stop_Time - ts->Start_Time;

  ts->My_Tot_Time += my_time;

  MPI_Allreduce(&my_time, &global_time, 1, MPI_DOUBLE, MPI_MAX, comm);
  ts->Global_Tot_Time += global_time;

  return ZOLTAN_OK;
}


/****************************************************************************/
int Zoltan_Timer_Print(
  ZTIMER *zt,
  int ts_idx,
  int proc,
  FILE *fp
)
{
/* Print a single timer's information */
static char *yo = "Zoltan_Timer_Print";
ZTIMER_TS *ts;

  TESTTIMER(zt, yo);
  TESTINDEX(zt, ts_idx, yo);
  ts = &(zt->Times[ts_idx]);

  fprintf(fp, "%3d ZOLTAN_TIMER %3d %23s:  ProcTime %e  GlobalTime %e\n", 
         proc, ts_idx, ts->Name, ts->My_Tot_Time, ts->Global_Tot_Time);
  return ZOLTAN_OK;
}

/****************************************************************************/
int Zoltan_Timer_PrintAll(
  ZTIMER *zt,
  int proc,
  FILE *fp
)
{
/* Function to print all timer information */
static char *yo = "Zoltan_Timer_PrintAll";
int i, ierr = ZOLTAN_OK;

  TESTTIMER(zt, yo);
  for (i = 0; i < zt->NextTimeStruct; i++) 
    if ((ierr = Zoltan_Timer_Print(zt, i, proc, fp)) != ZOLTAN_OK)
      break;

  return ierr;
}

/****************************************************************************/
void Zoltan_Timer_Destroy(
  ZTIMER **zt
)
{
/* Destroy a Timer object */
  if (*zt != NULL) {
    ZOLTAN_FREE(&((*zt)->Times));
    ZOLTAN_FREE(zt);
  }
}

/****************************************************************************/
#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif
