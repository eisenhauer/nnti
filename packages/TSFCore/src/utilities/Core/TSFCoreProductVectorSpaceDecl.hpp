// @HEADER
// ***********************************************************************
// 
//               TSFCore: Trilinos Solver Framework Core
//                 Copyright (2004) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

// //////////////////////////////////////////////////////////////
// TSFCoreProductVectorSpace.hpp

#ifndef TSFCORE_PRODUCT_VECTOR_SPACE_DECL_HPP
#define TSFCORE_PRODUCT_VECTOR_SPACE_DECL_HPP

#include "TSFCoreProductVectorSpaceBase.hpp"

namespace TSFCore {

///
/** Standard concrete implementation of a product vector space.
 *
 * This subclass allows <tt>%VectorSpace</tt> objects to be built out
 * of one or more other vector space objects to form a product space.
 * The specific type of vector created by
 * <tt>this->createMember()</tt> is of type <tt>ProductVector</tt> but
 * the client need not ever know this or deal with this type directly.
 * However, a client may want to dynamic_cast to access the
 * <tt>ProductVectorBase</tt> interface.
 *
 * To demonstate how to use this class, suppose one has <tt>p</tt>
 * vector spaces <tt>V[k]</tt> for <tt>k = 0...p-1</tt> and one wants
 * to form a concatenated vector space <tt>Z</tt> containing all of
 * these vector spaces stacked on top of each other to form:

 \verbatim

     [ V[0]   ]
 Z = [ V[1]   ]
     [ .      ]
     [ V[p-1] ]
 \endverbatim

 * Such a vector space can be constructed as shown in the following
 * function:

 \code
 void construct(
   const Teuchos::RefCountPtr<const VectorSpace<Scalar> > V[], int p
   ,ProductVectorSpace* Z
   )
 {
     Z->initialize( V, p );
 }
 \endcode

 * Once a <tt>%ProductVectorSpace</tt> object is initialized, it can
 * be used just like any other <tt>%VectorSpace</tt> object.  The
 * method <tt>createMember()</tt> will create <tt>ProductVector</tt>
 * objects containing members from the constituent vector spaces.
 *
 * There are several methods that can be used by clients that need to
 * work with the individual constituent vector spaces.  The method
 * <tt>numBlocks()</tt> give the number of constituent vector spaces
 * while <tt>vecSpaces()</tt> returns a pointer to a copy of the array
 * of the constituent vector spaces passed to <tt>initialize()</tt>.
 * Some other useful utility methods are also defined.  The method
 * <tt>vecSpacesOffsets()</tt> returns a pointer to an array that
 * gives the offset of each constitient vector in the overall
 * composite product vector.  For example, the (zero-based)
 * <tt>kth</tt> vector space <tt>this->%vecSpaces()[k]</tt> owns the
 * element indexes <tt>this->%vecSpacesOffsets()[k]+1</tt> to
 * <tt>this->%vecSpacesOffsets()[k+1]</tt>.  Determining which
 * constitient vector space owns a element index can be determined by
 * calling <tt>getVecSpcPoss()</tt>.
 *
 * The default assignment operator is allowed since it has the correct
 * semantics.  The default copy constructor is also allowed but only
 * performs a shallow copy of the constituent vector space objects.
 * If you want to copy the constituent vector space objects also you
 * need to use the <tt>clone()</tt> method.  The default constructor
 * is not allowed (declared private) to avoid accidents.
 */
template<class Scalar>
class ProductVectorSpace : virtual public ProductVectorSpaceBase<Scalar> {
public:

	/** @name Constructors/initializers/accessors */
	//@{

	/// Construct to an initialized state (calls <tt>initialize</tt>)
	ProductVectorSpace(
		const int                                                   numBlocks
		,const Teuchos::RefCountPtr<const VectorSpace<Scalar> >     vecSpaces[]
		);
	
	///
	/** Initialize with a list of constituent vector spaces.
	 *
	 * @param  numBlocks [in] The number of constituent vector spaces.
	 * @param  vecSpaces [in] If <tt>vecSpaces!=NULL</tt> then <tt>vecSpaces</tt>
	 *                   must point to an array of length <tt>this->numBlocks</tt>
	 *                   and on output <tt>vecSpace[i]</tt> will be set to <tt>this->vecSpaces()[i]</tt>
	 *                   for <tt>i=0,..,this->numBlocks()-1</tt>.
	 *
	 * Preconditions:<ul>
	 * <li> <tt>numBlocks > 0</tt>
	 * <li> <tt>vecSpaces != NULL</tt>
	 * <li> <tt>vecSpaces[i].get() != NULL, i=0,...,numBlocks</tt>
	 * <li> The vector space create by <tt><tt>vecSpace[i]->smallVecSpcFcty()->createVecSpc(k)</tt>
	 *      must be compatible.  In other words,
	 *      <tt>vecSpace[i]->smallVecSpcFcty()->createVecSpc(k)->isCompatible(
	 *        *vecSpace[j]->smallVecSpcFcty()->createVecSpc(k)
	 *        ) == true </tt> for all <tt>i=[0,numBlocks]</tt>, <tt>j=[0,numBlocks]</tt>
	 *        and valid <tt>k > 0</tt>.
	 *      This is required to insure that product multi-vectors can be created
	 *      with constitient mult-vector blocks that have compatible <tt>domain()</tt>
	 *      vector spaces.
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li> <tt>this->numBlocks()==numBlocks</tt>
	 * <li> <tt>getBlock(i).get() == vecSpaces[i].get(), i=0,...,numBlocks-1</tt>
	 * <li> <tt>vecSpaces()[i].get() == vecSpaces[i].get(), i=0,...,numBlocks-1</tt>
	 * </ul>
	 */
	virtual void initialize(
		const int                                                   numBlocks
		,const Teuchos::RefCountPtr<const VectorSpace<Scalar> >     vecSpaces[]
		);

	///
	/** Uninitialize.
	 *
	 * @param  numBlocks [out] If <tt>numBlocks!=NULL</tt> then on output <tt>*numBlocks</tt>
	 *                   will be set to <tt>this->numBlocks()</tt>.
	 * @param  vecSpaces [out] If <tt>vecSpaces!=NULL</tt> then <tt>vecSpaces</tt>
	 *                   must point to an array of length <tt>this->numBlocks</tt>
	 *                   and on output <tt>vecSpace[i]</tt> will be set to <tt>this->vecSpaces()[i]</tt>
	 *                   for <tt>i=0,..,this->numBlocks()-1</tt>.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>this->numBlocks()==0</tt>
	 * <li> <tt>vecSpaces()==NULL</tt>
	 * </ul>
	 */
	virtual void uninitialize(
		int                                                         *numBlocks  = NULL
		,Teuchos::RefCountPtr<const VectorSpace<Scalar> >           vecSpaces[] = NULL
		);

	///
	/** Returns a pointer to an array (of length <tt>this->numBlocks()</tt>)
	 * to the constituent vector spaces.
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>this->numBlocks() == 0</tt>] <tt>return == NULL</tt>
	 * <li> [<tt>this->numBlocks() > 0</tt>] <tt>return != NULL</tt>
	 * </ul>
	 */
	virtual const Teuchos::RefCountPtr<const VectorSpace<Scalar> >* vecSpaces() const;

	///
	/** Returns a pointer to an array (of length <tt>this->numBlocks()+1</tt>)
	 * of offset into each constituent vector space.
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>this->numBlocks() == 0</tt>] <tt>return == NULL</tt>
	 * <li> [<tt>this->numBlocks() > 0</tt>] <tt>return != NULL</tt>
	 * </ul>
	 */
	virtual const Index* vecSpacesOffsets() const;


	///
	/** Get the position of the vector space object and its offset into
	 * a composite vector that owns the <tt>ith</tt> index in the
	 * composite vector.
	 *
	 * Preconditions:<ul>
	 * <li> <tt>1 <= k <= this->dim()</tt>
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li> <tt>kth_global_offset + 1 <= i <= kth_global_offset + this->getBlock()[kth_vector_space]->dim()</tt>
	 * </ul>
	 *
	 * @param  i    [in] The index of the element to find the vector space object for.
	 * @param  kth_vector_space
	 *              [out] The index for <tt>this->vector_spaces()[kth_vector_space]</tt> that owns the element <tt>i</tt>.
	 * @param  kth_global_offset
	 *              [out] The global offset for <tt>this->vector_spaces()[kth_vector_space]</tt> in the composite
	 *              vector.
	 */
	void getVecSpcPoss( Index i, int* kth_vector_space, Index* kth_global_offset ) const;
	
	//@}

	/** @name Overridden from ProductVectorSpace */
	//@{

	///
	int numBlocks() const;
	///
	Teuchos::RefCountPtr<const VectorSpace<Scalar> > getBlock(const int k) const; 

	//@}

	/** @name Overridden from VectorSpace */
	//@{
	
	///
	Index dim() const;
	///
	bool isCompatible( const VectorSpace<Scalar>& vecSpc ) const;
	///
	Teuchos::RefCountPtr< Vector<Scalar> > createMember() const;
	///
	Scalar scalarProd( const Vector<Scalar>& x, const Vector<Scalar>& y ) const;
	///
	void scalarProds( const MultiVector<Scalar>& X, const MultiVector<Scalar>& Y, Scalar scalar_prods[] ) const;
	///
	bool isInCore() const;
	///
	Teuchos::RefCountPtr< const VectorSpaceFactory<Scalar> > smallVecSpcFcty() const;
	///
	Teuchos::RefCountPtr< MultiVector<Scalar> > createMembers(int numMembers) const;
	//@}

private:
 
	// ///////////////////////////////////
	// Private data members

	int                                                              numBlocks_;
	std::vector<Teuchos::RefCountPtr<const VectorSpace<Scalar> > >   vecSpaces_;
	std::vector<Index>                                               vecSpacesOffsets_;
	// cached info
	Index     dim_;
	bool      isInCore_;
	
protected:
	// ///////////////////////////////////
	// Protected member functions
  // Added to allow TSFExtended ProductVectorSpace to derive from this 
  ProductVectorSpace(){;}

};

// /////////////////////////////////
// Inline members


template<class Scalar>
const Teuchos::RefCountPtr<const VectorSpace<Scalar> >*
ProductVectorSpace<Scalar>::vecSpaces() const
{
	return ( dim_ ? &vecSpaces_[0] : NULL );
}


template<class Scalar>
const Index* ProductVectorSpace<Scalar>::vecSpacesOffsets() const
{
	return ( dim_ ? &vecSpacesOffsets_[0] : NULL );
}

  // The following was changed by ptboggs so that 
  // TSFExtended ProductVectorSpace
  // could derive from this.



} // namespace TSFCore

#endif // TSFCORE_PRODUCT_VECTOR_SPACE_DECL_HPP
