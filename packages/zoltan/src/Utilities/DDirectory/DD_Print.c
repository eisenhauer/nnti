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

#include <stdio.h>
#include <stdlib.h>

#include "DD.h"



/*  NOTE: See file, README, for associated documentation. (RTH) */






/********************   Zoltan_DD_Print()  ***********************/

int Zoltan_DD_Print (
 Zoltan_DD_Directory *dd)        /* contains directory state information */
   {
   int      i,j ;
   DD_Node *ptr ;
   char    *yo = "Zoltan_DD_Print" ;


   /* input sanity checks */
   if (dd == NULL)
      {
      ZOLTAN_PRINT_ERROR (ZOLTAN_DD_NO_PROC, yo, "NULL dd input argument.") ;
      return ZOLTAN_DD_INPUT_ERROR ;
      }
   if (dd->debug_level > 1)
      ZOLTAN_TRACE_IN (dd->my_proc, yo, NULL) ;

   /* walk linked list printing each node */
   for (i = 0 ; i < dd->table_length ; i++)
      for (ptr = dd->table[i] ; ptr != NULL ; ptr = ptr->next)
         {
         printf ("ZOLTAN DD Print(%d): \tList %3d, \tGID ", dd->my_proc, i) ;
         printf("(");
         for (j = 0 ; j < dd->gid_length ; j++)
            printf("%u ", ptr->gid[j]);
         printf(") ");
         printf ("\tOwner %d\n", ptr->owner) ;
         }

   if (dd->debug_level > 1)
      ZOLTAN_TRACE_OUT (dd->my_proc, yo, NULL) ;
   return ZOLTAN_DD_NORMAL_RETURN ;
   }


