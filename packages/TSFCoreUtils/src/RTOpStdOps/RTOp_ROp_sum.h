/* /////////////////////////////////////////////
// RTOp_ROp_sum.h
//
// Copyright (C) 2001 Roscoe Ainsworth Bartlett
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the "Artistic License" (see the web site
//   http://www.opensource.org/licenses/artistic-license.html).
// This license is spelled out in the file COPYING.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// above mentioned "Artistic License" for more details.
*/

#ifndef RTOP_ROP_SUM_H
#define RTOP_ROP_SUM_H

#include "RTOp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file RTOp_ROp_sum.h Reduction operator for taking the sum of the elements of a vector.
  *
  * <tt>targ_obj <- sum( v[0](i), i=1...n )</tt>
  *
  * This operator is only defined to allow one vector argument
  * (<tt>num_vecs == 1</tt>) <tt>v[0]</tt> but it can handle a dense or sparse vector.
  */
/*@{*/

/** Name of this reduction operator class */
extern const char RTOp_ROp_sum_name[];

/** Virtual function table */
extern const struct RTOp_RTOp_vtbl_t RTOp_ROp_sum_vtbl;

/** Constructor */
int RTOp_ROp_sum_construct( struct RTOp_RTOp* op );

/** Destructor */
int RTOp_ROp_sum_destroy( struct RTOp_RTOp* op );

/** Extract the value of the sum */
RTOp_value_type RTOp_ROp_sum_val(RTOp_ReductTarget targ_obj);

/*@}*/

#ifdef __cplusplus
}
#endif

#endif  /* RTOP_ROP_SUM_H */
