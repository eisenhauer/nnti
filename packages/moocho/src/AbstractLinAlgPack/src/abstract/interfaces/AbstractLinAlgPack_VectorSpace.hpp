// ///////////////////////////////////////////////////////////////
// VectorSpace.h
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

#ifndef VECTOR_SPACE_H
#define VECTOR_SPACE_H

#include "InnerProduct.h"
#include "AbstractFactory.h"
#include "Range1D.h"

namespace AbstractLinAlgPack {

///
/** Abstract interface for objects that represent a space for mutable coordinate vectors.
 *
 * This interface acts primarily as an "Abstract Factory" interface for creating \c VectorWithOpMutable
 * objects using the \c create_member() method.  A <tt>%VectorSpace</tt> object may also be able
 * to create \c MultiVectorMutable objects which represent a compact collection of vectors.
 * Every application area should be able to define a <tt>%MultiVectorMutable</tt> subclass if
 * it can define a <tt>%VectorWithOpMutable</tt> subclass.
 * A secondary role for <tt>%VectorSpace</tt> objects is to test for compatibility of vector spaces
 * (and the vectors and matrix using those spaces) objects using the \c is_compatible() method.
 *
 * Given a <tt>%VectorSpace</tt> object it may also be possible to create sub-spaces using the
 * \c sub_space() method.  This subspace is not a sub-space in the mathematical sense but instead
 * referese to a sub-range of vector elements.
 *
 * Any <tt>%VectorSpace</tt> object can be copied using the \c clone() method.  Therefore,
 * clients have complete control over the lifetime of <tt>%VectorSpace</tt> objects.
 *
 * A <tt>VectorSpace</tt> object can exist independent from any individual <tt>VectorWithOpMutable</tt>
 * (or \c MutiVectorMutable) object; Or, a <tt>VectorSpace</tt> object can have a lifetime that is
 * dependent on a single <tt>VectorWithOp</tt> ( or \c MultiVector) object.  The same interface can
 * serve both roles.
 *
 * A vector space is also where the inner product for the space is
 * defined.  It is anticipated that the same implementation of vectors
 * and vector spaces will be reused over and over again with different
 * definitions of the inner product.  Therefore, the inner product is
 * represented as a seperate strategy object.  For example, the same
 * parallel vector implementation can be used with several different
 * inner product definitions.  In some cases, the same inner product
 * stategy object may be able to be used with diffeent vector implemenations
 * (such as the dot product for example).
 *
 * Note that the default copy constructor will transfer the inner product object correctly
 * but the subclass must make sure to copy the inner product object in \c clone() operation.
 * This is little price to pay considering what this design takes care of already for
 * <tt>%VectorSpace</tt> subclasses.
 * However, the default copy constructor will only make a shallow copy of the inner product
 * object but since this object may never be changed, this is perhaps the correct behavior.
 * For any other behavior and the subbclass will have to take care of it.
 *
 * A vector space may also have another advanced feature; it may be able to define other
 * vector spaces based on a gatter operation of selected vector elements given a
 * <tt>GenPermMatrixSlice</tt> (<tt>GPMS</tt>) object.  For example, suppose that a
 * <tt>GPMS</tt> object \c P is defined which extracts a (relatively) small number of elements
 * of a vector \c v (from the vector space \c *this) and gathers them into a smaller vector \c q.
 \verbatim

 q = op(P)*v
 \endverbatim
 * The question is, to what vector space \c Q does the vector \c q belong?
 * The answer is returned by the method <tt>Q = this->space(P,P_trans)</tt>.
 * Theoretically, \c op(P) could
 * be any <tt>GPMS</tt> object and <tt>this->space(P,P_trans)</tt> should always be able
 * to return a non-NULL vector space object.  In reality, the client should only
 * expect <tt>this->space(P,P_trans).get() != NULL</tt> if
 * <tt>q_dim = BLAS_Cpp::rows( P.rows(), P.cols(), P_trans )</tt> is a relatively
 * small number (i.e. less than a few thousand).  If \c q_dim is small, then the vector
 * \c q can always be represented as a local serial vector.  This is not a terribly
 * difficult requirement and any <tt>%VectorSpace</tt> subclass should be able to
 * comply.
 * 	
 *
 */
class VectorSpace
	: public MemMngPack::AbstractFactory<VectorWithOpMutable>
{
public:

	/// Thrown if vector spaces are incompatible
	class IncompatibleVectorSpaces : public std::logic_error
	{public: IncompatibleVectorSpaces(const std::string& what_arg) : std::logic_error(what_arg) {}};
	///
	typedef MemMngPack::ref_count_ptr<InnerProduct>         inner_prod_ptr_t;
	///
	typedef MemMngPack::ref_count_ptr<const VectorSpace>    space_ptr_t;
	///
	typedef MemMngPack::ref_count_ptr<VectorWithOpMutable>  vec_mut_ptr_t;
	///
	typedef MemMngPack::ref_count_ptr<MultiVectorMutable>   multi_vec_mut_ptr_t;


	/** @name Constructors / initializers */
	//@{

	/// Calls \c inner_prod()
	VectorSpace( const inner_prod_ptr_t& inner_prod = MemMngPack::null );

	///
	/** Initialize with an inner product object.
	 *
	 * @param  inner_prod  [in] Smart pointer to inner product strategy object.
	 *                     If <tt>inner_prod.get()==NULL</tt> then an
	 *                     \c InnerProductDot object will be used instead.
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>inner_prod.get() != NULL</tt>] <tt>this->inner_prod().get() == inner_prod.get()</tt>
	 * <li> [<tt>inner_prod.get() == NULL</tt>] <tt>dynamic_cast<InnerProductDot*>(this->inner_prod().get()) != NULL</tt>
	 * </ul>
	 */
	virtual void inner_prod( const inner_prod_ptr_t& inner_prod );

	///
	/** Return the smart pointer to the inner product strategy object.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>return.get() != NULL</tt>
	 * </ul>
	 */
	virtual const inner_prod_ptr_t inner_prod() const;

	//@}

	/** @name Pure virtual functions that must be overridden */
	//@{

	///
	/** Create a clone of \c this vector space object.
	 *
	 * The returned vector space object is expected to be independent from \c this
	 * and have a lifetime that extends beyond \c this.  This makes a vector space
	 * class a little hander to implement by makes for much better flexibility
	 * for the client.  A complete implementation of <tt>%VectorSpace</tt> is not
	 * allowed to return \c NULL from this method.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>return.get() != NULL</tt>
	 * </ul>
	 */
	virtual space_ptr_t clone() const = 0;

	///
	/** Compare the compatibility of two vector spaces.
	 *
	 * If this function returns true, then vectors created from
	 * either of the vector spaces will be compatible and can
	 * be combined in vector operations.
	 *
	 * Invariants:<ul>
	 * <li> [<tt>this->is_compatible(vec_spc) == true</tt>] <tt>vec_spc.is_compatible(*this) == true</tt>
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>this->dim() != vec_spc.dim()</tt>] <tt>return == false</tt>
	 * </ul>
	 */
	virtual bool is_compatible(const VectorSpace& vec_spc ) const = 0;

	///
	/** Return the dimmension of the vector space.
	 */
	virtual index_type dim() const = 0;

	///
	/** Create a vector member from the vector space.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>return.get() != NULL</tt>
	 * <li> <tt>return->dim() == this->dim()</tt>
	 * <li> <tt>return->space().is_compatible(*this) == true</tt>
	 * </ul>
	 *
	 * @return  Returns a smart reference counted pointer to a dynamically
	 * allocated vector object.  After construction the values returnd by 
	 * <tt>return->get_ele(i)</tt> are unspecified (uninitialized).  This allows for
	 * faster execution times.  Note that <tt>&return->space()</tt> does not have to
	 * be equal to <tt>this</tt>.
	 */
	virtual vec_mut_ptr_t create_member() const = 0;

	//@}

	/** @name Virtual functions with default implementations */
	//@{

	///
	/** Create a vector member from the vector space initialized to a scalar.
	 *
	 * @param  alpha   [in] Scalar that all elements of allocated vector are
	 *                 initialized to.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>return.get() != NULL</tt>
	 * <li> <tt>return->dim() == this->dim()</tt>
	 * <li> <tt>return->space().is_compatible(*this) == true</tt>
	 * </ul>
	 *
	 * @return  Returns a smart reference counted pointer to a dynamically
	 * allocated vector object.  After construction the values returnd by 
	 * <tt>return->get_ele(i)</tt> are equal to <tt>alpha</tt>.
	 * Note that <tt>&return->space()</tt> does not have to
	 * be equal to <tt>this</tt>.
	 *
	 * The default implementation just calls \c create_member() and then
	 * assigns <tt>alpha</tt> before returning the smart pointer object.
	 */
	virtual vec_mut_ptr_t create_member(const value_type& alpha) const;

	///
	/** Create a set of vector members (a \c MultiVector) from the vector space.
	 *
	 * Preconditions:<ul>
	 * <li> <tt>num_vecs >= 1</tt> (throw <tt>???</tt>)
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>return.get() != NULL</tt>] <tt>return->space_cols().is_compatible(*this) == true</tt>
	 * <li> [<tt>return.get() != NULL</tt>] <tt>return->space_rows().dim() == num_vecs</tt>
	 * <li> [<tt>return.get() != NULL</tt>] <tt>(return->access_by() & MultiVector::COL_ACCESS) == true</tt>
	 * </ul>
	 *
	 * @return  Returns a smart reference counted pointer to a dynamically
	 * allocated multi-vector object.  After construction the values returnd by 
	 * <tt>return->col(j)->get_ele(i)</tt> are unspecified (uninitialized).  This
	 * allows for faster execution times.  Note that <tt>&return->space_cols()</tt>
	 * does not have to be equal to <tt>this</tt>.  It is allowed for a vector
	 * space implementation to refuse to create multi-vectors and can return
	 * \c NULL from this method.
	 *
	 * The default implementation just returns \c NULL.
	 */
	virtual multi_vec_mut_ptr_t create_members(size_type num_vecs) const;

	//@}

	///
	/** Create a transient sub-space of the current vector space.
	 *
	 * Preconditions:<ul>
	 * <li> <tt>rng.ubound() <= this->dim()</tt> (<tt>throw std::out_of_range</tt>)
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>return.get() != NULL</tt>] <tt>return->dim() == rng->size()</tt>
	 * </ul>
	 *
	 * @param  rng  [in] The range of the elements to extract a vector sub-space.
	 *
	 * @return  Returns a smart reference counted pointer to a dynamically
	 * allocated vector space object.  Note that the vector object returned
	 * by <tt>this->sub_space(rng).create_member()</tt> should be exactly equivalent
	 * to the vector returned by
	 * <tt>this->create_member()->sub_view(rng)->space()->create_member()</tt>.
	 * It is allowed for the implementation to return <tt>return->get() == NULL</tt>
	 * for arbitrary values of <tt>rng</tt>.  Only some <tt>rng</tt> ranges may be allowed
	 * but they will be appropriate for the application at hand.  However, a
	 * very good implementation should be able to accomidate any valid <tt>rng</tt>
	 * that meets the basic preconditions.
	 *
	 * Note that if two vector space objects <tt>X</tt> and <tt>Y</tt> are compatible
	 * (i.e. <tt>X.is_compatible(Y) == true</tt>, then it is also expected that
	 * <tt>X.sub_space(rng)->is_compatible(*Y.sub_space(rng))</tt> will also be \c true.
	 * However, in general it can not be expected that
	 * <tt>X.sub_space(rng1)->is_compatible(*X.sub_space(rng2)) == true</tt>, even if
	 * <tt>rng1.size() == rng2.size()</tt>.  For serial vectors, it may
	 * be but for parallel vectors it will most certainly not be.  Therefore, in
	 * general, don't assume that arbitrary subsets of the vector spaces will be
	 * compatible, even if the sizes of these subspaces are the same.
	 *
	 * The default implementation uses the subclass \c VectorSpaceSubSpace to
	 * represent any arbitrary sub-space but this can be very inefficient if the
	 * sub-space is very small compared this this full vector space.
	 */
	virtual space_ptr_t sub_space(const Range1D& rng) const;

	/// Inlined to call <tt>this->sub_space(Range1D(il,iu))</tt>.
	space_ptr_t sub_space( const index_type il, const index_type iu ) const;

	//@}
	
	/** @name Overridden from AbstractFactory */
	//@{

	/// Just calls <tt>this->create_member()</tt> by default!
	obj_ptr_t create() const;

	//@}

private:
#ifdef DOXYGEN_COMPILE
	InnerProduct       *inner_prod;
#else
	inner_prod_ptr_t   inner_prod_;
#endif

}; // end class VectorSpace

// ////////////////////////////////////////////////
// Inline members

inline
VectorSpace::space_ptr_t
VectorSpace::sub_space( const index_type il, const index_type iu ) const
{
	return this->sub_space(Range1D(il,iu));
}

} // end namespace AbstractLinAlgPack

#endif  // VECTOR_SPACE_H
