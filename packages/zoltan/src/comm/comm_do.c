/*****************************************************************************
 * Zoltan Dynamic Load-Balancing Library for Parallel Applications           *
 * Copyright (c) 2000, Sandia National Laboratories.                         *
 * Zoltan is distributed under the GNU Lesser General Public License 2.1.    * 
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
#include <memory.h>
#include "mpi.h"
#include "comm_const.h"
#include "all_allo_const.h"


/* Given the communication object, perform a communication operation as
   efficiently as possible. */


int       LB_Comm_Do(
struct Comm_Obj * plan,		/* communication data structure */
int tag,			/* message tag for communicating */
char *send_data,		/* array of data I currently own */
int nsize,			/* # bytes per data item */
char *recv_data)		/* array of data I'll own after comm */
{	
    char     *send_buff;	/* space to buffer outgoing data */
    char     *recv_array;	/* place to receive messages */
    int       my_proc;		/* processor ID */
    int       self_recv_address;/* where in recv_data self info starts */
    int       self_num;		/* where in proc list my_proc appears */
    int       index;		/* offset into data array */
    int       offset;		/* offset into array I'm copying into */
    int       self_index;	/* offset for data I'm keeping */
    int       i, j, k;		/* loop counters */

    MPI_Comm_rank(plan->comm, &my_proc);

    /* Post irecvs */

    if (plan->indices_from == NULL) {	/* receive directly into final location */
	recv_array = recv_data;
    }
    else {			/* need to buffer receive to copy */
	recv_array = (char *) LB_MALLOC(plan->total_recv_length * nsize);
    }

    j = 0;
    k = 0;
    for (i = 0; i < plan->nrecvs + plan->self_msg; i++) {
	if (plan->procs_from[i] != my_proc) {
	    MPI_Irecv((void *) &recv_array[j], plan->lengths_from[i] * nsize,
		      (MPI_Datatype) MPI_BYTE, plan->procs_from[i], tag,
		      plan->comm, &plan->request[k]);
	    k++;
	}
	else {
	    self_recv_address = j;
	}
	j += plan->lengths_from[i] * nsize;
    }

    MPI_Barrier(plan->comm);

    /* Send out data */
    if (plan->indices_to == NULL) {	/* data already blocked by processor. */
	index = 0;
	for (i = 0; i < plan->nsends + plan->self_msg; i++) {
	    if (plan->procs_to[i] != my_proc) {
		MPI_Rsend((void *) &send_data[index * nsize],
			 plan->lengths_to[i] * nsize,
			 (MPI_Datatype) MPI_BYTE, plan->procs_to[i], tag,
			 plan->comm);
		index += plan->lengths_to[i];
	    }
	    else {
		self_num = i;
		self_index = index;
		index += plan->lengths_to[i];
	    }
	}
	if (plan->self_msg) {	/* Copy data to self. */
	    memcpy(&recv_array[self_recv_address],
		   &send_data[self_index * nsize],
		   nsize * plan->lengths_to[self_num]);
	}
    }

    else {			/* Not blocked by processor.  Need to buffer. */
	send_buff = (char *) LB_MALLOC(plan->max_send_length * nsize);
	j = 0;
	for (i = 0; i < plan->nsends + plan->self_msg; i++) {
	    if (plan->procs_to[i] != my_proc) {
		/* Need to pack message first. */
		offset = 0;
		for (k = 0; k < plan->lengths_to[i]; k++) {
		    memcpy(&send_buff[offset],
			   &send_data[plan->indices_to[j++] * nsize], nsize);
		    offset += nsize;
		}
		MPI_Rsend((void *) send_buff, plan->lengths_to[i] * nsize,
		      (MPI_Datatype) MPI_BYTE, plan->procs_to[i], tag, plan->comm);
	    }
	    else {
		self_num = i;
		self_index = j;
	        j += plan->lengths_to[i];
	    }
	}
	if (plan->self_msg) {	/* Copy data to self. */
	    for (k = 0; k < plan->lengths_to[self_num]; k++) {
		memcpy(&recv_array[self_recv_address],
		       &send_data[plan->indices_to[self_index++] * nsize], nsize);
		self_recv_address += nsize;
	    }
	}

	LB_FREE((void **) &send_buff);
    }

    /* Wait for messages to arrive. */
    /* Note: since request is in plan, could wait in later routine. */
    if (plan->nrecvs > 0) {
	MPI_Waitall(plan->nrecvs, plan->request, plan->status);
    }

    if (plan->indices_from != NULL) {	/* Need to copy into recv_data. */
	for (i = 0; i < plan->total_recv_length; i++) {
	    memcpy(&recv_data[plan->indices_from[i] * nsize],
		   &recv_array[i * nsize], nsize);
	}

	LB_FREE((void **) &recv_array);
    }

    return(LB_OK);
}
