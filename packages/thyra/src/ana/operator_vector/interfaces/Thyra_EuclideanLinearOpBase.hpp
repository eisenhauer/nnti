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

#ifndef THYRA_EUCLIDEAN_LINEAR_OP_HPP
#define THYRA_EUCLIDEAN_LINEAR_OP_HPP

#include "Thyra_EuclideanLinearOpBaseDecl.hpp"
#include "Thyra_LinearOpBase.hpp"
#include "Thyra_ScalarProdBase.hpp"
#include "Thyra_AssertOp.hpp"

namespace Thyra {

// Virtual functions with default implementations

template<class Scalar>
void EuclideanLinearOpBase<Scalar>::euclideanApply(
    const ETransp                     M_trans
    ,const MultiVectorBase<Scalar>    &X
    ,MultiVectorBase<Scalar>          *Y
    ,const Scalar                     alpha
    ,const Scalar                     beta
    ) const
{
#ifdef _DEBUG
  THYRA_ASSERT_LINEAR_OP_MULTIVEC_APPLY_SPACES("EuclideanLinearOpBase<Scalar>::euclideanApply(...)",*this,M_trans,X,Y);
#endif
  const Index numMv = X.domain()->dim();
  for( int j = 1; j <= numMv; ++j )
    this->euclideanApply( M_trans, *X.col(j), &*Y->col(j), alpha, beta );
}

// Overridden functions from OpBase */

template<class Scalar>
Teuchos::RefCountPtr<const VectorSpaceBase<Scalar> >
EuclideanLinearOpBase<Scalar>::range() const
{
  return this->rangeScalarProdVecSpc();
}

template<class Scalar>
Teuchos::RefCountPtr<const VectorSpaceBase<Scalar> >
EuclideanLinearOpBase<Scalar>::domain() const
{
  return this->domainScalarProdVecSpc();
}

// Overridden functions from LinearOpBase

template<class Scalar>
void EuclideanLinearOpBase<Scalar>::apply(
  const ETransp                M_trans
  ,const VectorBase<Scalar>    &x
  ,VectorBase<Scalar>          *y
  ,const Scalar                alpha
  ,const Scalar                beta
  ) const
{
  if( M_trans==NOTRANS )
    this->domainScalarProdVecSpc()->getScalarProd()->apply(*this,M_trans,x,y,alpha,beta);
  else
    this->rangeScalarProdVecSpc()->getScalarProd()->apply(*this,M_trans,x,y,alpha,beta);
}

template<class Scalar>
void EuclideanLinearOpBase<Scalar>::apply(
  const ETransp                     M_trans
  ,const MultiVectorBase<Scalar>    &X
  ,MultiVectorBase<Scalar>          *Y
  ,const Scalar                     alpha
  ,const Scalar                     beta
  ) const
{
  euclidean_apply_impl(M_trans,X,Y,alpha,beta);
}

// protected

template<class Scalar>
void EuclideanLinearOpBase<Scalar>::euclidean_apply_impl(
  const ETransp                     M_trans
  ,const MultiVectorBase<Scalar>    &X
  ,MultiVectorBase<Scalar>          *Y
  ,const Scalar                     alpha
  ,const Scalar                     beta
  ) const
{
  if( M_trans==NOTRANS )
    this->domainScalarProdVecSpc()->getScalarProd()->apply(*this,M_trans,X,Y,alpha,beta);
  else
    this->rangeScalarProdVecSpc()->getScalarProd()->apply(*this,M_trans,X,Y,alpha,beta);
}

} // namespace Thyra

#endif // THYRA_EUCLIDEAN_LINEAR_OP_HPP
