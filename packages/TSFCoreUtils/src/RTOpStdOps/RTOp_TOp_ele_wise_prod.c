/* /////////////////////////////////////////////
// RTOp_TOp_ele_wise_prod.c
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

#include <assert.h>
#include <malloc.h>

#include "RTOp_TOp_ele_wise_prod.h"
#include "RTOp_obj_null_vtbl.h"
#include "RTOp_obj_value_vtbl.h"

/* Implementation functions for RTOp_RTOp */

static int RTOp_TOp_ele_wise_prod_apply_op(
  const struct RTOp_RTOp_vtbl_t* vtbl, const void* obj_data
  , const int num_vecs, const struct RTOp_SubVector vecs[]
  , const int num_targ_vecs, const struct RTOp_MutableSubVector targ_vecs[]
  , RTOp_ReductTarget targ_obj )
{
  RTOp_value_type        alpha;
  RTOp_index_type        sub_dim;
  RTOp_value_type        *z_val;
  ptrdiff_t              z_val_s;
  const RTOp_value_type  *v0_val;
  ptrdiff_t              v0_val_s;
  const RTOp_value_type  *v1_val;
  ptrdiff_t              v1_val_s;
  register RTOp_index_type k;

  /*
  // Validate the input
  */
  if( num_vecs != 2 || vecs == NULL )
    return RTOp_ERR_INVALID_NUM_VECS;
  if( num_targ_vecs != 1 || targ_vecs == NULL )
    return RTOp_ERR_INVALID_NUM_TARG_VECS;
  if( targ_vecs[0].sub_dim != vecs[0].sub_dim
    || targ_vecs[0].sub_dim != vecs[1].sub_dim )
    return RTOp_ERR_INCOMPATIBLE_VECS;
  assert(obj_data);


  /*
  // Get pointers to data
  */

  /* alpha */
  alpha = *(RTOp_value_type*)obj_data;
  /* z */
  sub_dim       = targ_vecs[0].sub_dim;
  z_val         = targ_vecs[0].values;
  z_val_s       = targ_vecs[0].values_stride;
  /* v0 */
  v0_val         = vecs[0].values;
  v0_val_s       = vecs[0].values_stride;
  /* v1 */
  v1_val         = vecs[1].values;
  v1_val_s       = vecs[1].values_stride;

  /*
  // Element-wise product
  */

  if( z_val_s == 1 && v0_val_s == 1 && v1_val_s == 1 ) {
    /* Slightly faster loop for unit stride vectors */
    for( k = 0; k < sub_dim; ++k )
      *z_val++ += alpha*(*v0_val++)*(*v1_val++);
  }
  else {
    /* More general implementation for non-unit strides */
    for( k = 0; k < sub_dim; ++k, z_val+=z_val_s, v0_val+=v0_val_s, v1_val+=v1_val_s )
      *z_val += alpha*(*v0_val)*(*v1_val);
  }

  return 0; /* success? */
}

/* Name of this transformation operator class */
const char RTOp_TOp_ele_wise_prod_name[] = "TOp_ele_wise_prod";

/* Virtual function table */
const struct RTOp_RTOp_vtbl_t RTOp_TOp_ele_wise_prod_vtbl =
{
  &RTOp_obj_value_vtbl /* alpha */
  ,&RTOp_obj_null_vtbl /* use null type for target object */
  ,NULL /* use default from reduct_vtbl */
  ,RTOp_TOp_ele_wise_prod_apply_op
  ,NULL
  ,NULL
};

/* Class specific functions */

int RTOp_TOp_ele_wise_prod_construct( RTOp_value_type alpha, struct RTOp_RTOp* op )
{
#ifdef RTOp_DEBUG
  assert(op);
#endif
  op->obj_data  = NULL;
  op->vtbl      = &RTOp_TOp_ele_wise_prod_vtbl;
  op->vtbl->obj_data_vtbl->obj_create(NULL,NULL,&op->obj_data);
  return RTOp_TOp_ele_wise_prod_set_alpha(alpha,op);
}

int RTOp_TOp_ele_wise_prod_set_alpha( RTOp_value_type alpha, struct RTOp_RTOp* op )
{
#ifdef RTOp_DEBUG
  assert(op);
  assert(op->obj_data);
#endif
  *(RTOp_value_type*)op->obj_data = alpha;
  return 0;
}

int RTOp_TOp_ele_wise_prod_destroy( struct RTOp_RTOp* op )
{
  op->obj_data  = NULL;
  op->vtbl      = NULL;
  return 0; /* success? */
}
