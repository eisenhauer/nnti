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

#ifndef TSFCORE_PRODUCT_VECTOR_SPACE_STD_HPP
#define TSFCORE_PRODUCT_VECTOR_SPACE_STD_HPP

#include "TSFCoreProductVectorSpaceDecl.hpp"
#include "TSFCoreProductVector.hpp"
#include "TSFCoreProductMultiVectorBase.hpp"
#include "Teuchos_dyn_cast.hpp"

namespace TSFCore {

// Constructors/initializers/accessors

template<class Scalar>
ProductVectorSpace<Scalar>::ProductVectorSpace(
	const int                                                   numBlocks
	,const Teuchos::RefCountPtr<const VectorSpace<Scalar> >     vecSpaces[]
	)
{
	initialize(numBlocks,vecSpaces);
}

template<class Scalar>
void ProductVectorSpace<Scalar>::initialize(
	const int                                                   numBlocks
	,const Teuchos::RefCountPtr<const VectorSpace<Scalar> >     vecSpaces[]
	)
{
	//
	// Check preconditions and compute cached quantities
	//
	TEST_FOR_EXCEPT( numBlocks < 0 );
	TEST_FOR_EXCEPT( vecSpaces == NULL );
	bool  isInCore = true;
	for( int k = 0; k < numBlocks; ++k ) {
		TEST_FOR_EXCEPTION(
			vecSpaces[k].get() == NULL, std::invalid_argument
			,"Error, the smart pointer vecSpaces["<<k<<"] can not be NULL!"
			);
		if( !vecSpaces[k]->isInCore() ) isInCore = false;
	}
	//
	// Setup private data members (should not throw an exception from here)
	//
	vecSpaces_.resize(numBlocks);
	std::copy( vecSpaces, vecSpaces+numBlocks, vecSpaces_.begin() );
	vecSpacesOffsets_.resize(numBlocks);
	vecSpacesOffsets_[0] = 0;
	dim_ = 0;
	for( int k = 1; k <= numBlocks; ++k ) {
		const Index dim_km1 = vecSpaces[k-1]->dim();
		vecSpacesOffsets_[k] = vecSpacesOffsets_[k-1] + dim_km1;
		dim_ += dim_km1;
	}
	isInCore_ = isInCore;
}

template<class Scalar>
void ProductVectorSpace<Scalar>::uninitialize(
	int                                                         *numBlocks
	,Teuchos::RefCountPtr<const VectorSpace<Scalar> >           vecSpaces[]
	)
{
	vecSpaces_.resize(0);
	dim_      = 0;
	isInCore_ = false;
}

template<class Scalar>
void ProductVectorSpace<Scalar>::getVecSpcPoss(
	Index i, int* kth_vector_space, Index* kth_global_offset
	) const
{
	// Validate the preconditions
#ifdef _DEBUG
	TEST_FOR_EXCEPTION(
		i < 1 || this->dim() < i, std::out_of_range
		,"VectorSpaceBlocked::get_vector_space_position(...): Error, i = "
		<< i << " is not in range [1,"<<this->dim()<<"]"
		);
#endif
	*kth_vector_space  = 0;
	*kth_global_offset = 0;
	while( *kth_vector_space < static_cast<int>(vecSpaces_.size()) ) {
		const Index off_kp1 = vecSpacesOffsets_[*kth_vector_space+1];
		if( off_kp1 + 1 > i ) {
			*kth_global_offset = vecSpacesOffsets_[*kth_vector_space];
			break;
		}
		++(*kth_vector_space);
	}
	TEST_FOR_EXCEPT( !(*kth_vector_space < static_cast<int>(vecSpaces_.size())) );
}

// Overridden from ProductVectorSpace

template<class Scalar>
int ProductVectorSpace<Scalar>::numBlocks() const
{
	return vecSpaces_.size();
}

template<class Scalar>
Teuchos::RefCountPtr<const VectorSpace<Scalar> >
ProductVectorSpace<Scalar>::getBlock(const int k) const
{
	TEST_FOR_EXCEPT( k < 0 || static_cast<int>(vecSpaces_.size()-1) < k );
	return vecSpaces_[k];
}

// Overridden from VectorSpace

template<class Scalar>
Index ProductVectorSpace<Scalar>::dim() const
{
	return dim_;
}

template<class Scalar>
bool ProductVectorSpace<Scalar>::isCompatible( const VectorSpace<Scalar>& vecSpc ) const
{
	// Check for in-core
	if( this->isInCore() && vecSpc.isInCore() && ( this->dim() == vecSpc.dim() ) )
		return true;
	// Check for product vector interface
	const ProductVectorSpaceBase<Scalar> *pvsb = dynamic_cast<const ProductVectorSpaceBase<Scalar>*>(&vecSpc);
	if( !pvsb )
		return false;
	// Validate that constituent vector spaces are compatible
	const int numBlocks = this->numBlocks(); 
	if( numBlocks != pvsb->numBlocks() )
		return false;
	for( int i = 0; i < numBlocks; ++i ) {
		if( !this->getBlock(i)->isCompatible(*pvsb->getBlock(i)) )
			return false;
	}
	return true;
}

template<class Scalar>
Teuchos::RefCountPtr< Vector<Scalar> >
ProductVectorSpace<Scalar>::createMember() const
{
	return Teuchos::rcp(new ProductVector<Scalar>(Teuchos::rcp(this,false),NULL));
}

template<class Scalar>
Scalar ProductVectorSpace<Scalar>::scalarProd(
	const Vector<Scalar>   &x_in
	,const Vector<Scalar>  &y_in
	) const
{
	const int numBlocks = this->numBlocks(); 
	const ProductVectorBase<Scalar>
		&x = Teuchos::dyn_cast<const ProductVectorBase<Scalar> >(x_in),
		&y = Teuchos::dyn_cast<const ProductVectorBase<Scalar> >(y_in);
	TEST_FOR_EXCEPT( numBlocks!=x.productSpace()->numBlocks() || numBlocks!=y.productSpace()->numBlocks() );
	Scalar scalarProd = Teuchos::ScalarTraits<Scalar>::zero();
	for( int k = 0; k < numBlocks; ++k )
		scalarProd += vecSpaces_[k]->scalarProd(*x.getBlock(k),*y.getBlock(k));
	return scalarProd;
}

template<class Scalar>
void ProductVectorSpace<Scalar>::scalarProds(
	const MultiVector<Scalar>    &X_in
	,const MultiVector<Scalar>   &Y_in
	,Scalar                      scalar_prods[]
	) const
{
	const int numBlocks = this->numBlocks(); 
	const ProductMultiVectorBase<Scalar>
		&X = Teuchos::dyn_cast<const ProductMultiVectorBase<Scalar> >(X_in),
		&Y = Teuchos::dyn_cast<const ProductMultiVectorBase<Scalar> >(Y_in);
	TEST_FOR_EXCEPT( numBlocks!=X.productSpace()->numBlocks() || numBlocks!=Y.productSpace()->numBlocks() );
	const VectorSpace<Scalar> &domain = *X.domain();
	TEST_FOR_EXCEPT( !domain.isCompatible(*Y.domain()) );
	const Index m = domain.dim();
	std::fill_n( scalar_prods, m, Teuchos::ScalarTraits<Scalar>::zero() );
	for( int k = 0; k < numBlocks; ++k )
		vecSpaces_[k]->scalarProds(*X.getBlock(k),*Y.getBlock(k),scalar_prods);
}

template<class Scalar>
bool ProductVectorSpace<Scalar>::isInCore() const
{
	return isInCore_;
}

template<class Scalar>
Teuchos::RefCountPtr< const VectorSpaceFactory<Scalar> >
ProductVectorSpace<Scalar>::smallVecSpcFcty() const
{
	if( dim_ ) return vecSpaces_[0]->smallVecSpcFcty(); // They should all be compatible!
	return Teuchos::null;
}

template<class Scalar>
Teuchos::RefCountPtr< MultiVector<Scalar> >
ProductVectorSpace<Scalar>::createMembers(int numMembers) const
{
	return VectorSpace<Scalar>::createMembers(numMembers); // ToDo: Specialize when needed!
}

} // namespace TSFCore

#endif // TSFCORE_PRODUCT_VECTOR_SPACE_STD_HPP
