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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "params_const.h"
#include "zoltan_types.h"
#include "zoltan_util.h"

/*****************************************************************************/
/***   Function prototypes:                                                ***/
/*****************************************************************************/

static void Zoltan_Print_Assigned_Param_Vals(PARAM_VARS * );

/*****************************************************************************/
/***   Local macros:                                                       ***/
/*****************************************************************************/
#define SET_PARAM_VAL(datatype,len,value) { \
  for (i=0; i<len; i++) \
    ((datatype *) param_ptr->ptr)[i] = value; \
  }
/*****************************************************************************/
/***   Function definitions:                                               ***/
/*****************************************************************************/

int      Zoltan_Assign_Param_Vals(
PARAM_LIST * change_list,	/* list of parameter values being changed */
PARAM_VARS * params,		/* structure describing parameters        */
int debug_level,                /* level for output of debugging info     */
int proc,                       /* processor # (controls debug printing)  */
int print_proc                  /* processor that should perform printing */
)
{	
    char     *yo = "Zoltan_Assign_Param_Vals";
    char      msg[256];
    char     *name;		/* name of parameter being reset */
    char     *val;		/* new value for parameter       */
    int       index;		/* index of parameter entry      */
    int       found;		/* is name found?                */
    int       ierr;		/* error code                    */
    int       i;		/* loop variable                 */
    PARAM_VARS *param_ptr;      /* pointer to current param      */

    ierr = ZOLTAN_OK;

    while (change_list != NULL) {
        param_ptr = params;
	name = change_list->name;
	val = change_list->new_val;
	index = change_list->index;

	found = 0;
	while (param_ptr->name != NULL) {
	    if (!strcmp(param_ptr->name, name)) {
		found = 1;
		break;
	    }
	    param_ptr++;
	}

	if (found) {		/* name found */

          /* Check that param_ptr->ptr isn't NULL */
          if (param_ptr->ptr == NULL) {
             ierr = ZOLTAN_WARN;
             if (debug_level > 0 && proc == print_proc) {
                sprintf(msg, "Parameter %s is not bound "
                       "to any variable.  Parameter ignored.\n", 
                        param_ptr->name);
                ZOLTAN_PRINT_WARN(proc, yo, msg);
             }
          }

          /* Check that index is in valid range */
          if ((index > param_ptr->length) || (index < -1)) {
             ierr = ZOLTAN_WARN;
             if (debug_level > 0 && proc == print_proc) {
                sprintf(msg, "Invalid index %d for parameter %s. "
                       "Parameter entry ignored.\n", 
                        index, param_ptr->name);
                ZOLTAN_PRINT_WARN(proc, yo, msg);
             }
          }

          if (ierr == ZOLTAN_OK) { /* OK so far. */

            if (index == -1)
              index = param_ptr->length;  /* Set all entries in a vector. */
            if (index == 0) 
              index = 1;                  /* Scalar parameter. */

	    /* Figure out what type it is and read value. */
	    if (!strcmp(param_ptr->type, "INT") || 
                !strcmp(param_ptr->type, "INTEGER")) {
		/* First special case if True or False */
		if (*val == 'T')
		    SET_PARAM_VAL(int, index, 1)
		else if (*val == 'F')
		    SET_PARAM_VAL(int, index, 0)
		else {
		    SET_PARAM_VAL(int, index, atoi(val))
		}
	    }

	    else if ((!strcmp(param_ptr->type, "FLOAT")) ||
	             (!strcmp(param_ptr->type, "REAL"))) {
		SET_PARAM_VAL(float, index, atof(val))
	    }

	    else if (!strcmp(param_ptr->type, "DOUBLE")) {
		SET_PARAM_VAL(double, index, atof(val))
	    }

	    else if (!strcmp(param_ptr->type, "LONG")) {
		/* First special case if True or False */
		if (*val == 'T')
		    SET_PARAM_VAL(long, index, 1)
		else if (*val == 'F')
		    SET_PARAM_VAL(long, index, 0)
		else {
		    SET_PARAM_VAL(long, index, atol(val))
		}
	    }

	    else if (!strcmp(param_ptr->type, "CHAR")) {
		SET_PARAM_VAL(char, index, (*val))
	    }

	    else if (!strcmp(param_ptr->type, "STRING")) {
                /* String parameters are assumed to be scalar. */
		strncpy((char *) param_ptr->ptr, val, MAX_PARAM_STRING_LEN);
	    }
	}
      }

      change_list = change_list->next;
    }

    if (debug_level > 0 && proc == print_proc)
        Zoltan_Print_Assigned_Param_Vals(params);

    return ierr;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

static void Zoltan_Print_Assigned_Param_Vals(
PARAM_VARS * params 		/* structure describing parameters */
)
{
/* Prints the parameter values in PARAM_VARS *param.     */
PARAM_VARS *param_ptr;       /* pointer to current param */
param_ptr = params;

    while (param_ptr->name != NULL) {
      if (param_ptr->ptr != NULL) {
        if (!strcmp(param_ptr->type, "INT") || 
            !strcmp(param_ptr->type, "INTEGER")) {
 
            printf("ZOLTAN Parameter %s = %d\n", 
                    param_ptr->name, *((int *) param_ptr->ptr));
                
        }
        else if (!strcmp(param_ptr->type, "FLOAT") ||
                 !strcmp(param_ptr->type, "REAL")) {
            printf("ZOLTAN Parameter %s = %f\n", 
                    param_ptr->name, *((float *) param_ptr->ptr));
        }
        else if (!strcmp(param_ptr->type, "DOUBLE")) {
            printf("ZOLTAN Parameter %s = %f\n", 
                    param_ptr->name, *((double *) param_ptr->ptr));
        }
        else if (!strcmp(param_ptr->type, "LONG")) {
            printf("ZOLTAN Parameter %s = %ld\n", 
                    param_ptr->name, *((long *) param_ptr->ptr));
        }
        else if (!strcmp(param_ptr->type, "STRING")) {
            printf("ZOLTAN Parameter %s = %s\n", 
                    param_ptr->name, (char *) param_ptr->ptr);
        }
        else if (!strcmp(param_ptr->type, "CHAR")) {
            printf("ZOLTAN Parameter %s = %c\n", 
                    param_ptr->name, *((char *) param_ptr->ptr));
        }
      }
      param_ptr++;
    }
}

#ifdef __cplusplus
} /* closing bracket for extern "C" */
#endif
