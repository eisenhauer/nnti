/*****************************************************************************
 * Zoltan Dynamic Load-Balancing Library for Parallel Applications           *
 * Copyright (c) 2000, Sandia National Laboratories.                         *
 * For more info, see the README file in the top-level Zoltan directory.     *  
 *****************************************************************************/
/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    $Revision$
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "lb_const.h"
#include "all_allo_const.h"
#include "params_const.h"

/* Fortran memory allocation callback functions */

static LB_FORT_MALLOC_INT_FN *LB_Fort_Malloc_int;
static LB_FORT_MALLOC_GID_FN *LB_Fort_Malloc_GID;
static LB_FORT_MALLOC_LID_FN *LB_Fort_Malloc_LID;
static LB_FORT_FREE_INT_FN *LB_Fort_Free_int;
static LB_FORT_FREE_GID_FN *LB_Fort_Free_GID;
static LB_FORT_FREE_LID_FN *LB_Fort_Free_LID;


int LB_Set_Malloc_Param(
char *name,			/* name of variable */
char *val)			/* value of variable */
{
    int status;
    PARAM_UTYPE result;		/* value returned from Check_Param */
    int index;			/* index returned from Check_Param */
    PARAM_VARS malloc_params[] = {
	{ "DEBUG_MEMORY", NULL, "INT" },
	{ NULL, NULL, NULL }
    };

    status = LB_Check_Param(name, val, malloc_params, &result, &index);
    if (status == 0 && index == 0) {
	LB_Set_Memory_Debug(result.ival);
	status = 3;
    }

    return(status);
}


/******************************************************************************
 * Special allocation for routines that allocate an array and return pointer.
 *
 * LB_Special_Malloc allows the allocation to be done from either C or Fortran.
 *
 * LB_Special_Free frees memory allocated by LB_Special_Malloc
 *
 * LB_Register_Fort_Malloc is called by the wrappers for the Fortran
 * interface to provide pointers to the Fortran allocation/free routines.
 *
 * int LB_Special_Malloc(struct LB_Struct *lb, void **array, int size,
 *                       LB_SPECIAL_MALLOC_TYPE type)
 *
 *   lb    -- the load balancing structure in use
 *   array -- int**, struct LB_GID**, or struct LB_LID**; returned as a
 *            pointer to the allocated space
 *   size  -- number of elements to be allocated in the array
 *   type  -- the type of array; LB_SPECIAL_MALLOC_INT, LB_SPECIAL_MALLOC_GID,
 *            or LB_SPECIAL_MALLOC_LID
 *
 * The return value is 1 if the allocation succeeded, 0 if it failed.
 *
 * int LB_Special_Free(struct LB_Struct *lb, void **array,
                       LB_SPECIAL_MALLOC_TYPE type)
 *
 *****************************************************************************/

void LB_Register_Fort_Malloc(LB_FORT_MALLOC_INT_FN *fort_malloc_int,
                             LB_FORT_MALLOC_GID_FN *fort_malloc_GID,
                             LB_FORT_MALLOC_LID_FN *fort_malloc_LID,
                             LB_FORT_FREE_INT_FN *fort_free_int,
                             LB_FORT_FREE_GID_FN *fort_free_GID,
                             LB_FORT_FREE_LID_FN *fort_free_LID)
{
   LB_Fort_Malloc_int = fort_malloc_int;
   LB_Fort_Malloc_GID = fort_malloc_GID;
   LB_Fort_Malloc_LID = fort_malloc_LID;
   LB_Fort_Free_int = fort_free_int;
   LB_Fort_Free_GID = fort_free_GID;
   LB_Fort_Free_LID = fort_free_LID;
}

int LB_Special_Malloc(struct LB_Struct *lb, void **array, int size,
                      LB_SPECIAL_MALLOC_TYPE type)
{
   int *ret_addr, success;
   char *yo = "LB_Special_Malloc";

   success = 1;
   if (lb->Fortran) {

/* allocation from Fortran */

      switch(type) {
      case LB_SPECIAL_MALLOC_INT:
#ifdef PGI /* special case for PGI Fortran compiler */
         LB_Fort_Malloc_int((int *)(array[1]),&size,&ret_addr,array[2]);
#else
#ifdef FUJITSU /* special case for Fujitsu and Lahey Fortran compilers */
         LB_Fort_Malloc_int((int *)(array[1]),&size,&ret_addr,array[2],0,0);
#else
         LB_Fort_Malloc_int((int *)(array[1]),&size,&ret_addr);
#endif
#endif
         if (ret_addr==0) success=0;
         break;
      case LB_SPECIAL_MALLOC_GID:
         if (LB_GID_IS_INT) {
#ifdef PGI
            LB_Fort_Malloc_int((int *)(array[1]),&size,&ret_addr,array[2]);
#else
#ifdef FUJITSU
            LB_Fort_Malloc_int((int *)(array[1]),&size,&ret_addr,array[2],0,0);
#else
            LB_Fort_Malloc_int((int *)(array[1]),&size,&ret_addr);
#endif
#endif
         }else{
#ifdef PGI
            LB_Fort_Malloc_GID((LB_GID *)(array[1]),&size,&ret_addr,array[2]);
#else
#ifdef FUJITSU
            LB_Fort_Malloc_GID((LB_GID *)(array[1]),&size,&ret_addr,array[2],0,0);
#else
            LB_Fort_Malloc_GID((LB_GID *)(array[1]),&size,&ret_addr);
#endif
#endif
         }
         if (ret_addr==0) success=0;
         break;
      case LB_SPECIAL_MALLOC_LID:
         if (LB_LID_IS_INT) {
#ifdef PGI
            LB_Fort_Malloc_int((int *)(array[1]),&size,&ret_addr,array[2]);
#else
#ifdef FUJITSU
            LB_Fort_Malloc_int((int *)(array[1]),&size,&ret_addr,array[2],0,0);
#else
            LB_Fort_Malloc_int((int *)(array[1]),&size,&ret_addr);
#endif
#endif
         }else{
#ifdef PGI
            LB_Fort_Malloc_LID((LB_LID *)(array[1]),&size,&ret_addr,array[2]);
#else
#ifdef FUJITSU
            LB_Fort_Malloc_LID((LB_LID *)(array[1]),&size,&ret_addr,array[2],0,0);
#else
            LB_Fort_Malloc_LID((LB_LID *)(array[1]),&size,&ret_addr);
#endif
#endif
         }
         if (ret_addr==0) success=0;
         break;
      default:
         fprintf(stderr, "Error: illegal value passed for type in %s\n", yo);
         success = 0;
      }
      if (success) {
         array[0] = ret_addr;
      }else{
         array[0] = NULL;
      }

   }else{

/* allocation from C */

      switch(type) {
      case LB_SPECIAL_MALLOC_INT:
         *array = (int *) LB_MALLOC(size*sizeof(int));
         break;
      case LB_SPECIAL_MALLOC_GID:
         *array = (LB_GID *) LB_MALLOC(size*sizeof(LB_GID));
         break;
      case LB_SPECIAL_MALLOC_LID:
         *array = (LB_LID *) LB_MALLOC(size*sizeof(LB_LID));
         break;
      default:
         fprintf(stderr, "Error: illegal value passed for type in %s\n", yo);
         *array = NULL;
      }
      if (*array==NULL) success=0;
   }
   return success;
}

/****************************************************************************/
/****************************************************************************/
/****************************************************************************/

int LB_Special_Free(struct LB_Struct *lb, void **array,
                    LB_SPECIAL_MALLOC_TYPE type)
{
   int success;
   char *yo = "LB_Special_Free";

   success = 1;
   if (lb->Fortran) {

/* deallocation from Fortran */

      switch(type) {
      case LB_SPECIAL_MALLOC_INT:
#ifdef PGI /* special case for PGI Fortran compiler */
         LB_Fort_Free_int((int *)(array[1]),array[2]);
#else
#ifdef FUJITSU /* special case for Fujitsu and Lahey Fortran compilers */
         LB_Fort_Free_int((int *)(array[1]),array[2]);
#else
         LB_Fort_Free_int((int *)(array[1]));
#endif
#endif
         break;
      case LB_SPECIAL_MALLOC_GID:
         if (LB_GID_IS_INT) {
#ifdef PGI
            LB_Fort_Free_int((int *)(array[1]),array[2]);
#else
#ifdef FUJITSU
            LB_Fort_Free_int((int *)(array[1]),array[2]);
#else
            LB_Fort_Free_int((int *)(array[1]));
#endif
#endif
         }else{
#ifdef PGI
            LB_Fort_Free_GID((LB_GID *)(array[1]),array[2]);
#else
#ifdef FUJITSU
            LB_Fort_Free_GID((LB_GID *)(array[1]),array[2]);
#else
            LB_Fort_Free_GID((LB_GID *)(array[1]));
#endif
#endif
         }
         break;
      case LB_SPECIAL_MALLOC_LID:
         if (LB_LID_IS_INT) {
#ifdef PGI
            LB_Fort_Free_int((int *)(array[1]),array[2]);
#else
#ifdef FUJITSU
            LB_Fort_Free_int((int *)(array[1]),array[2]);
#else
            LB_Fort_Free_int((int *)(array[1]));
#endif
#endif
         }else{
#ifdef PGI
            LB_Fort_Free_LID((LB_LID *)(array[1]),array[2]);
#else
#ifdef FUJITSU
            LB_Fort_Free_LID((LB_LID *)(array[1]),array[2]);
#else
            LB_Fort_Free_LID((LB_LID *)(array[1]));
#endif
#endif
         }
         break;
      default:
         fprintf(stderr, "Error: illegal value passed for type in %s\n", yo);
         success = 0;
      }

   }else{

/* deallocation from C */

      LB_FREE(array);
   }
   return success;
}

/*****************************************************************************/
/*                      END of all_allo.c                                     */
/*****************************************************************************/
