/* /////////////////////////////////////////////
// RTOp_TOp_scale_vector.h
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

#ifndef RTOP_TOP_SCALAR_VECTOR_H
#define RTOP_TOP_SCALAR_VECTOR_H

#include "RTOp.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \file RTOp_TOp_scale_vector.h Scale the elements of a vector.
  *
  * <tt>targ_vec(i) <- alpha * targ_vec(i), for i = 1...n</tt>.
  *
  * This operator is only defined for a self transformation (<tt>num_vecs == 0</tt>).
  */
/*@{*/

/** Name of this transformation operator class */
extern const char RTOp_TOp_scale_vector_name[];

/** Virtual function table */
extern const struct RTOp_RTOp_vtbl_t RTOp_TOp_scale_vector_vtbl;

/** Constructor */
int RTOp_TOp_scale_vector_construct( RTOp_value_type alpha, struct RTOp_RTOp* op );

/** Destructor */
int RTOp_TOp_scale_vector_destroy( struct RTOp_RTOp* op );

/** Reset alpha */
int RTOp_TOp_scale_vector_set_alpha( RTOp_value_type alpha, struct RTOp_RTOp* op );

/*@}*/

#ifdef __cplusplus
}
#endif

#endif  /* RTOP_TOP_SCALAR_VECTOR_H */
