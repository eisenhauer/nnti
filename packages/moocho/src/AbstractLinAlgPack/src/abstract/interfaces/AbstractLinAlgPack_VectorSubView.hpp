// ///////////////////////////////////////////////////////////////////
// VectorWithOpSubView.h
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

#ifndef VECTOR_WITH_OP_SUB_VIEW_H
#define VECTOR_WITH_OP_SUB_VIEW_H

#include "VectorWithOp.h"
#include "VectorSpaceSubSpace.h"

namespace AbstractLinAlgPack {

///
/** Concrete subclass for a default sub-view implementation for a VectorWithOp
 * object.
 *
 * Not all of the methods from VectorWithOp are overridden, only those that
 * need to be or may result in better performance.
 *
 * There is really not much to this vector subclass.  The subclass is only possible
 * because of the \c first_ele, \c sub_dim, and \c global_offset options with \c apply_reduction().  The
 * vector space object returned by <tt>this->space()</tt> is of type \c VectorSpaceSubSpace
 * which in turn relys on \c VectorSpace::sub_space().
 *
 * The default constructor and copy constructors are allowed but the default
 * assignment operator is not allowed.
 */
class VectorWithOpSubView : virtual public VectorWithOp {
public:

	///
	/** Calls <tt>this->initialize()</tt>.
	 */
	VectorWithOpSubView( const vec_ptr_t& full_vec, const Range1D& rng );
	///
	/** Initialize a sub-view based on a full vector.
	 *
	 * Constructs a view of the vector <tt>this = vec(rng)</tt>.
	 *
	 * Preconditions:<ul>
	 * <li> [<tt>full_vec.get() != NULL && rng.full_range() == false</tt>]
	 *      <tt>rng.lbound() <= full_vec->dim()</tt> (throw <tt>std::out_of_range</tt>).
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>full_vec.get() != NULL</tt>]
	 *      <tt>this->get_ele(i) == full_vec->get_ele(rng.lbound()-1+i)</tt>,
	 *      for <tt>i = 1...rng.size()</tt>
	 * </ul>
	 *
	 * @param  full_vec  [in] The original full vector.  It is allowed for <tt>full_vec.get() == NULL</tt>
	 *                   in which case <tt>this</tt> is uninitialized (i.e. <tt>this->dim() == 0</tt>).
	 * @param  rng       [in] The range of elements in <tt>full_vec</tt> that <tt>this</tt> vector will represent.
	 */
	void initialize( const vec_ptr_t& full_vec, const Range1D& rng );
	///
	const VectorWithOp* full_vec() const;
	///
	const VectorSpaceSubSpace& space_impl() const;

	/** @name Overridden from VectorWithOp */
	//@{

	///
	const VectorSpace& space() const;
	///
	index_type dim() const;
	///
	void apply_reduction(
		const RTOpPack::RTOp& op
		,const size_t num_vecs, const VectorWithOp** vecs
		,const size_t num_targ_vecs, VectorWithOpMutable** targ_vecs
		,RTOp_ReductTarget reduct_obj
		,const index_type first_ele, const index_type sub_dim, const index_type global_offset
		) const;
	///
	value_type get_ele(index_type i) const;
	///
	vec_ptr_t sub_view( const Range1D& rng ) const;
	///
	void get_sub_vector(
		const Range1D& rng, ESparseOrDense sparse_or_dense, RTOp_SubVector* sub_vec ) const;
	///
	void free_sub_vector( RTOp_SubVector* sub_vec ) const;

	//@}

private:

	vec_ptr_t                  full_vec_;   ///< If full_vec_.get() == NULL, the vector is uninitalized (dim == 0).
	VectorSpaceSubSpace        space_;      ///< The space that this vector belongs to.

	// Not defined and not to be called
	VectorWithOpSubView& operator=(const VectorWithOpSubView&);
	
}; // end class VectorWithOpSubView

// /////////////////////////////////////////////
// Inline members

inline
const VectorWithOp* VectorWithOpSubView::full_vec() const
{
	return full_vec_.get();
}

inline
const VectorSpaceSubSpace& VectorWithOpSubView::space_impl() const
{
	return space_;
}

} // end namespace AbstractLinAlgPack

#endif // VECTOR_WITH_OP_SUB_VIEW_H
