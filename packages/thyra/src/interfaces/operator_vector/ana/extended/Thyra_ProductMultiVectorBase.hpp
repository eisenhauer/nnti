// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
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

#ifndef THYRA_PRODUCT_MULTI_VECTOR_HPP
#define THYRA_PRODUCT_MULTI_VECTOR_HPP

#include "Thyra_MultiVectorBase.hpp"

namespace Thyra {

/** \brief Base interface for product multi-vectors.
 *
 * This class defines an abstract interface for a multi-vector that is
 * built out of the one or more other multi-vectors to form a product
 * multi-vector.  This class is only an interface.  A standard
 * implementation of this interface that should be sufficient for 99%
 * or so of use cases is provided in the concrete subclass
 * <tt>ProductMultiVector</tt>.
 *
 * ToDo: Finish documentation!
 *
 * \ingroup Thyra_Op_Vec_Interoperability_Extended_Interfaces_grp
 */
template<class Scalar>
class ProductMultiVectorBase : virtual public MultiVectorBase<Scalar> {
public:

  /** \brief Returns the associated product vector space.
   *
   * If <tt>*this</tt> is uninitialized then
   * <tt>return.get()==NULL</tt>.
   */
  virtual Teuchos::RefCountPtr<const ProductVectorSpaceBase<Scalar> > productSpace() const = 0;

  /** \brief Returns a non-persisting non-<tt>const</tt> view of the (zero-based)
   * <tt>kth</tt> block multi-vector.
   *
   * @param k [in] The (zero-based) <tt>kth</tt> block index specifying which block to access.
   *
   * Preconditions:<ul>
   * <li> <tt>productSpace().get()!=NULL</tt>
   * <li> <tt>0 <= k <= productSpace()->numBlocks()-1</tt>
   * </ul>
   *
   * Note that <tt>*this</tt> is not guaranteed to be modified until
   * the smart pointer returned from this function, as well as any
   * other smart pointers created from this smart pointer, are
   * destroyed.  This requirement allows more flexibility in how this
   * function is implemented.
   *
   * Also note that no further interactions with <tt>*this</tt> should
   * be performed until the view returned from this function is
   * released as described above.
   */
  virtual Teuchos::RefCountPtr<MultiVectorBase<Scalar> > getBlock(const int k) = 0;

  /** \brief Returns a non-persisting <tt>const</tt> view of the (zero-based)
   * <tt>kth</tt> block multi-vector.
   *
   * @param k [in] The (zero-based) <tt>ith</tt> block index specifying which block to access.
   *
   * Preconditions:<ul>
   * <li> <tt>productSpace().get()!=NULL</tt>
   * <li> <tt>0 <= i <= productSpace()->numBlocks()-1</tt>
   * </ul>
   */
  virtual Teuchos::RefCountPtr<const MultiVectorBase<Scalar> > getBlock(const int k) const = 0;

};

} // namespace Thyra

#endif // THYRA_PRODUCT_MULTI_VECTOR_HPP
