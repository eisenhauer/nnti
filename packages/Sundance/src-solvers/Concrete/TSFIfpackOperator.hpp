/* @HEADER@ */
/* ***********************************************************************
// 
//           TSFExtended: Trilinos Solver Framework Extended
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
// **********************************************************************/
 /* @HEADER@ */

#ifndef TSFIFPACKOPERATOR_HPP
#define TSFIFPACKOPERATOR_HPP


#include "TSFEpetraMatrix.hpp"
#include "TSFOpWithBackwardsCompatibleApply.hpp"
#include "Ifpack_IlukGraph.h"
#include "Ifpack_CrsRiluk.h"

namespace TSFExtended
{
/**
 *
 */
class IfpackOperator : virtual public LinearOpBase<double, double>,
  public OpWithBackwardsCompatibleApply<double>
{
public:
  /** */
  IfpackOperator(const EpetraMatrix* A,
    int fillLevels,
    int overlapFill,
    double relaxationValue,
    double relativeThreshold,
    double absoluteThreshold);

   /** Thyra apply method */
  void apply(
    const Thyra::EConj                             conj,
    const Thyra::MultiVectorBase<double>    &X,
    Thyra::MultiVectorBase<double>           *Y,
    const double alpha,
    const double beta
    ) const 
    {
      OpWithBackwardsCompatibleApply<double>::apply(conj,X,Y,alpha,beta);
    }


  /** Thyra apply transpose method */
  void applyTranspose(
    const Thyra::EConj                            conj,
    const Thyra::MultiVectorBase<double>    &X,
    Thyra::MultiVectorBase<double>         *Y,
    const double                     alpha,
    const double                     beta
    ) const 
    {
      OpWithBackwardsCompatibleApply<double>::applyTranspose(conj,X,Y,alpha,beta);
    }

  /** 
   * Apply operator to a vector in the domain space and return a vector
   * in the range space.
   */
  virtual void generalApply(const Thyra::ETransp M_trans,
    const Thyra::VectorBase<double>    &x,
    Thyra::VectorBase<double>          *y,
    const double            alpha=1.0,
    const double            beta=0.0) const ;


  /** Return the domain of the operator */
  virtual RefCountPtr< const Thyra::VectorSpaceBase<double> > domain() const {return domain_;}

  /** Return the range of the operator */
  virtual RefCountPtr< const Thyra::VectorSpaceBase<double> > range() const {return range_;}


private:
  RefCountPtr<Ifpack_IlukGraph> precondGraph_;

  RefCountPtr<Ifpack_CrsRiluk> precond_;

  RefCountPtr<const Thyra::VectorSpaceBase<double> > domain_;

  RefCountPtr<const Thyra::VectorSpaceBase<double> > range_;
};
}

#endif /* TSFIFPACKOPERATOR_HPP */
