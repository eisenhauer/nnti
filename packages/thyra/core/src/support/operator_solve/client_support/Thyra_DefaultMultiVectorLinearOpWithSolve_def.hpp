// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
//                 Copyright (2004) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef THYRA_MULTI_VECTOR_LINEAR_OP_WITH_SOLVE_HPP
#define THYRA_MULTI_VECTOR_LINEAR_OP_WITH_SOLVE_HPP

#include "Thyra_DefaultMultiVectorLinearOpWithSolve_decl.hpp"
#include "Thyra_DefaultDiagonalLinearOp.hpp"
#include "Thyra_LinearOpWithSolveBase.hpp"
#include "Thyra_DefaultMultiVectorProductVectorSpace.hpp"
#include "Thyra_DefaultMultiVectorProductVector.hpp"
#include "Thyra_AssertOp.hpp"
#include "Teuchos_dyn_cast.hpp"


namespace Thyra {


// Constructors/initializers/accessors


template<class Scalar>
DefaultMultiVectorLinearOpWithSolve<Scalar>::DefaultMultiVectorLinearOpWithSolve()
{}


template<class Scalar>
void DefaultMultiVectorLinearOpWithSolve<Scalar>::nonconstInitialize(
  const RCP<LinearOpWithSolveBase<Scalar> > &lows,
  const RCP<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecRange,
  const RCP<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecDomain
  )
{
  validateInitialize(lows,multiVecRange,multiVecDomain);
  lows_ = lows;
  multiVecRange_ = multiVecRange;
  multiVecDomain_ = multiVecDomain;
}


template<class Scalar>
void DefaultMultiVectorLinearOpWithSolve<Scalar>::initialize(
  const RCP<const LinearOpWithSolveBase<Scalar> > &lows,
  const RCP<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecRange,
  const RCP<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecDomain
  )
{
  validateInitialize(lows,multiVecRange,multiVecDomain);
  lows_ = lows;
  multiVecRange_ = multiVecRange;
  multiVecDomain_ = multiVecDomain;
}


template<class Scalar>
RCP<LinearOpWithSolveBase<Scalar> >
DefaultMultiVectorLinearOpWithSolve<Scalar>::getNonconstLinearOpWithSolve()
{
  return lows_.getNonconstObj();
}


template<class Scalar>
RCP<const LinearOpWithSolveBase<Scalar> >
DefaultMultiVectorLinearOpWithSolve<Scalar>::getLinearOpWithSolve() const
{
  return lows_.getConstObj();
}


template<class Scalar>
void DefaultMultiVectorLinearOpWithSolve<Scalar>::uninitialize()
{
  lows_.uninitialize();
  multiVecRange_ = Teuchos::null;
  multiVecDomain_ = Teuchos::null;
}


// Overridden from LinearOpBase


template<class Scalar>
RCP< const VectorSpaceBase<Scalar> >
DefaultMultiVectorLinearOpWithSolve<Scalar>::range() const
{
  return multiVecRange_;
}


template<class Scalar>
RCP< const VectorSpaceBase<Scalar> >
DefaultMultiVectorLinearOpWithSolve<Scalar>::domain() const
{
  return multiVecDomain_;
}


template<class Scalar>
RCP<const LinearOpBase<Scalar> >
DefaultMultiVectorLinearOpWithSolve<Scalar>::clone() const
{
  return Teuchos::null; // ToDo: Implement if needed ???
}


// protected


// Overridden from LinearOpBase


template<class Scalar>
bool DefaultMultiVectorLinearOpWithSolve<Scalar>::opSupportedImpl(
  EOpTransp M_trans
  ) const
{
  return Thyra::opSupported(*lows_.getConstObj(),M_trans);
}


template<class Scalar>
void DefaultMultiVectorLinearOpWithSolve<Scalar>::applyImpl(
  const EOpTransp M_trans,
  const MultiVectorBase<Scalar> &XX,
  const Ptr<MultiVectorBase<Scalar> > &YY,
  const Scalar alpha,
  const Scalar beta
  ) const
{

  using Teuchos::dyn_cast;
  typedef DefaultMultiVectorProductVector<Scalar> MVPV;

  const Ordinal numCols = XX.domain()->dim();

  for (Ordinal col_j = 0; col_j < numCols; ++col_j) {

    const RCP<const VectorBase<Scalar> > x = XX.col(col_j);
    const RCP<VectorBase<Scalar> > y = YY->col(col_j);
 
    RCP<const MultiVectorBase<Scalar> >
      X = dyn_cast<const MVPV>(*x).getMultiVector().assert_not_null();
    RCP<MultiVectorBase<Scalar> >
      Y = dyn_cast<MVPV>(*y).getNonconstMultiVector().assert_not_null();
    
    Thyra::apply( *lows_.getConstObj(), M_trans, *X, Y.ptr(), alpha, beta );

  }

}


// Overridden from LinearOpWithSolveBase


template<class Scalar>
bool
DefaultMultiVectorLinearOpWithSolve<Scalar>::solveSupportsImpl(
  EOpTransp M_trans
  ) const
{
  return Thyra::solveSupports(*lows_.getConstObj(),M_trans);
}


template<class Scalar>
bool
DefaultMultiVectorLinearOpWithSolve<Scalar>::solveSupportsSolveMeasureTypeImpl(
  EOpTransp M_trans, const SolveMeasureType& solveMeasureType
  ) const
{
  return Thyra::solveSupportsSolveMeasureType(
    *lows_.getConstObj(),M_trans,solveMeasureType);
}


template<class Scalar>
SolveStatus<Scalar>
DefaultMultiVectorLinearOpWithSolve<Scalar>::solveImpl(
  const EOpTransp transp,
  const MultiVectorBase<Scalar> &BB,
  const Ptr<MultiVectorBase<Scalar> > &XX,
  const Ptr<const SolveCriteria<Scalar> > solveCriteria
  ) const
{

  using Teuchos::dyn_cast;
  using Teuchos::outArg;
  using Teuchos::inOutArg;
  typedef DefaultMultiVectorProductVector<Scalar> MVPV;

  const Ordinal numCols = BB.domain()->dim();

  SolveStatus<Scalar> overallSolveStatus;
  accumulateSolveStatusInit(outArg(overallSolveStatus));
  
  for (Ordinal col_j = 0; col_j < numCols; ++col_j) {

    const RCP<const VectorBase<Scalar> > b = BB.col(col_j);
    const RCP<VectorBase<Scalar> > x = XX->col(col_j);

    RCP<const MultiVectorBase<Scalar> >
      B = dyn_cast<const MVPV>(*b).getMultiVector().assert_not_null();
    RCP<MultiVectorBase<Scalar> >
      X = dyn_cast<MVPV>(*x).getNonconstMultiVector().assert_not_null();

    const SolveStatus<Scalar> solveStatus =
      Thyra::solve(*lows_.getConstObj(), transp, *B, X.ptr(), solveCriteria);

    accumulateSolveStatus(
      SolveCriteria<Scalar>(), // Never used
      solveStatus, inOutArg(overallSolveStatus) );

  }
  
  return overallSolveStatus;

}


// private


template<class Scalar>
void DefaultMultiVectorLinearOpWithSolve<Scalar>::validateInitialize(
  const RCP<const LinearOpWithSolveBase<Scalar> > &lows,
  const RCP<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecRange,
  const RCP<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecDomain
  )
{
#ifdef TEUCHOS_DEBUG
  TEST_FOR_EXCEPT(is_null(lows));
  TEST_FOR_EXCEPT(is_null(multiVecRange));
  TEST_FOR_EXCEPT(is_null(multiVecDomain));
  TEST_FOR_EXCEPT( multiVecRange->numBlocks() != multiVecDomain->numBlocks() );
  THYRA_ASSERT_VEC_SPACES(
    "DefaultMultiVectorLinearOpWithSolve<Scalar>::initialize(lows,multiVecRange,multiVecDomain)",
    *lows->range(), *multiVecRange->getBlock(0) );
  THYRA_ASSERT_VEC_SPACES(
    "DefaultMultiVectorLinearOpWithSolve<Scalar>::initialize(lows,multiVecRange,multiVecDomain)",
    *lows->domain(), *multiVecDomain->getBlock(0) );
#endif
}


}	// end namespace Thyra


#endif	// THYRA_MULTI_VECTOR_LINEAR_OP_WITH_SOLVE_HPP
