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

#ifndef THYRA_DEFAULT_INVERSE_LINEAR_OP_HPP
#define THYRA_DEFAULT_INVERSE_LINEAR_OP_HPP

#include "Thyra_DefaultInverseLinearOpDecl.hpp"
#include "Thyra_VectorStdOps.hpp"
#include "Thyra_AssertOp.hpp"
#include "Teuchos_Utils.hpp"
#include "Teuchos_TypeNameTraits.hpp"


namespace Thyra {


// Constructors/initializers/accessors


template<class Scalar>
DefaultInverseLinearOp<Scalar>::DefaultInverseLinearOp()
{}


template<class Scalar>
DefaultInverseLinearOp<Scalar>::DefaultInverseLinearOp(
  const Teuchos::RCP<LinearOpWithSolveBase<Scalar> > &lows,
  const SolveCriteria<Scalar> *fwdSolveCriteria,
  const EThrowOnSolveFailure throwOnFwdSolveFailure,
  const SolveCriteria<Scalar> *adjSolveCriteria,
  const EThrowOnSolveFailure throwOnAdjSolveFailure
  )
{
  initializeImpl(
    lows,fwdSolveCriteria,throwOnFwdSolveFailure
    ,adjSolveCriteria,throwOnAdjSolveFailure
    );
}


template<class Scalar>
DefaultInverseLinearOp<Scalar>::DefaultInverseLinearOp(
  const Teuchos::RCP<const LinearOpWithSolveBase<Scalar> > &lows,
  const SolveCriteria<Scalar> *fwdSolveCriteria,
  const EThrowOnSolveFailure throwOnFwdSolveFailure,
  const SolveCriteria<Scalar> *adjSolveCriteria,
  const EThrowOnSolveFailure throwOnAdjSolveFailure
  )
{
  initializeImpl(
    lows,fwdSolveCriteria,throwOnFwdSolveFailure
    ,adjSolveCriteria,throwOnAdjSolveFailure
    );
}


template<class Scalar>
void DefaultInverseLinearOp<Scalar>::initialize(
  const Teuchos::RCP<LinearOpWithSolveBase<Scalar> > &lows,
  const SolveCriteria<Scalar> *fwdSolveCriteria,
  const EThrowOnSolveFailure throwOnFwdSolveFailure,
  const SolveCriteria<Scalar> *adjSolveCriteria,
  const EThrowOnSolveFailure throwOnAdjSolveFailure
  )
{
  initializeImpl(
    lows,fwdSolveCriteria,throwOnFwdSolveFailure
    ,adjSolveCriteria,throwOnAdjSolveFailure
    );
}


template<class Scalar>
void DefaultInverseLinearOp<Scalar>::initialize(
  const Teuchos::RCP<const LinearOpWithSolveBase<Scalar> > &lows,
  const SolveCriteria<Scalar> *fwdSolveCriteria,
  const EThrowOnSolveFailure throwOnFwdSolveFailure,
  const SolveCriteria<Scalar> *adjSolveCriteria,
  const EThrowOnSolveFailure throwOnAdjSolveFailure
  )
{
  initializeImpl(
    lows,fwdSolveCriteria,throwOnFwdSolveFailure
    ,adjSolveCriteria,throwOnAdjSolveFailure
    );
}


template<class Scalar>
void DefaultInverseLinearOp<Scalar>::uninitialize()
{
  lows_.uninitialize();
  fwdSolveCriteria_ = Teuchos::null;
  adjSolveCriteria_ = Teuchos::null;
}


// Overridden form InverseLinearOpBase


template<class Scalar>
bool DefaultInverseLinearOp<Scalar>::isLowsConst() const
{
  return lows_.isConst();
}


template<class Scalar>
Teuchos::RCP<LinearOpWithSolveBase<Scalar> >
DefaultInverseLinearOp<Scalar>::getNonconstLows()
{
  return lows_.getNonconstObj();
}


template<class Scalar>
Teuchos::RCP<const LinearOpWithSolveBase<Scalar> >
DefaultInverseLinearOp<Scalar>::getLows() const
{
  return lows_.getConstObj();
}


// Overridden from LinearOpBase


template<class Scalar>
Teuchos::RCP< const VectorSpaceBase<Scalar> >
DefaultInverseLinearOp<Scalar>::range() const
{
  assertInitialized();
  return lows_.getConstObj()->domain();
}


template<class Scalar>
Teuchos::RCP< const VectorSpaceBase<Scalar> >
DefaultInverseLinearOp<Scalar>::domain() const
{
  assertInitialized();
  return lows_.getConstObj()->range();
}


template<class Scalar>
Teuchos::RCP<const LinearOpBase<Scalar> >
DefaultInverseLinearOp<Scalar>::clone() const
{
  return Teuchos::null; // Not supported yet but could be!
}


// Overridden from Teuchos::Describable

                                                
template<class Scalar>
std::string DefaultInverseLinearOp<Scalar>::description() const
{
  assertInitialized();
  std::ostringstream oss;
  oss
    << Teuchos::Describable::description() << "{"
    << "lows="<<lows_.getConstObj()->description()
    << ",fwdSolveCriteria="<<(fwdSolveCriteria_.get()?"...":"DEFAULT")
    << ",adjSolveCriteria="<<(adjSolveCriteria_.get()?"...":"DEFAULT")
    << "}";
  return oss.str();
}


template<class Scalar>
void DefaultInverseLinearOp<Scalar>::describe(
  Teuchos::FancyOStream &out,
  const Teuchos::EVerbosityLevel verbLevel
  ) const
{
  using Teuchos::RCP;
  using Teuchos::OSTab;
  assertInitialized();
  OSTab tab(out);
  switch(verbLevel) {
    case Teuchos::VERB_DEFAULT:
    case Teuchos::VERB_LOW:
      out << this->description() << std::endl;
      break;
    case Teuchos::VERB_MEDIUM:
    case Teuchos::VERB_HIGH:
    case Teuchos::VERB_EXTREME:
    {
      out
        << Teuchos::Describable::description() << "{"
        << "rangeDim=" << this->range()->dim()
        << ",domainDim=" << this->domain()->dim() << "}:\n";
      OSTab tab2(out);
      out <<  "lows = ";
      if(!lows_.getConstObj().get()) {
        out << " NULL\n";
      }
      else {
        out << Teuchos::describe(*lows_.getConstObj(),verbLevel);
/*
  // ToDo: Implement printing of solve criteria!
        out << "fwdSolveCriteria:";
        if(fwdSolveCriteria_.get())
          OSTab(out).o() << "\n" << *fwdSolveCriteria_;
        else
          out << " NULL\n";
        if(adjSolveCriteria_.get())
          OSTab(out).o() << "\n" << *adjSolveCriteria_;
        else
          out << " NULL\n";
*/
      }
      break;
    }
    default:
      TEST_FOR_EXCEPT(true); // Should never be called!
  }
}


// protected


// Overridden from SingleScalarLinearOpBase


template<class Scalar>
bool DefaultInverseLinearOp<Scalar>::opSupported(EOpTransp M_trans) const
{
  assertInitialized();
  return solveSupports(*lows_.getConstObj(),M_trans);
}


template<class Scalar>
void DefaultInverseLinearOp<Scalar>::apply(
  const EOpTransp M_trans,
  const MultiVectorBase<Scalar> &X,
  MultiVectorBase<Scalar> *Y,
  const Scalar alpha,
  const Scalar beta
  ) const
{
  typedef Teuchos::ScalarTraits<Scalar> ST;
  assertInitialized();
  TEST_FOR_EXCEPT(Y==NULL);
  // ToDo: Put in hooks for propogating verbosity level
  //
  // Y = alpha*op(M)*X + beta*Y
  //
  //   =>
  //
  // Y = beta*Y
  // Y += alpha*inv(op(lows))*X
  //
  Teuchos::RCP<MultiVectorBase<Scalar> >
    T;
  if(beta==ST::zero()) {
    T = Teuchos::rcp(Y,false);
  }
  else {
    T = createMembers(Y->range(),Y->domain()->dim());
    scale(beta,Y);
  }
  //
  const SolveCriteria<Scalar>
    *solveCriteria
    = (
      real_trans(M_trans)==NOTRANS
      ? fwdSolveCriteria_.get()
      : adjSolveCriteria_.get()
      );
  assign(T.get(), ST::zero()); // Have to initialize before solve!
  SolveStatus<Scalar>
    solveStatus = Thyra::solve<Scalar>(
      *lows_.getConstObj(), M_trans
      ,X,&*T
      ,solveCriteria
      );

  TEST_FOR_EXCEPTION(
    solveCriteria && solveStatus.solveStatus!=SOLVE_STATUS_CONVERGED
    && ( real_trans(M_trans)==NOTRANS
         ? throwOnFwdSolveFailure_==THROW_ON_SOLVE_FAILURE
         : throwOnAdjSolveFailure_==THROW_ON_SOLVE_FAILURE )
    ,CatastrophicSolveFailure
    ,"Error, the LOWS object " << lows_.getConstObj()->description() << " returned an unconverged"
    "status of " << toString(solveStatus.solveStatus) << " with the message "
    << solveStatus.message << "."
    );
  //
  if(beta==ST::zero()) {
    scale(alpha,Y);
  }
  else {
    update( alpha, *T, Y );
  }
}


// private


template<class Scalar>
template<class LOWS>
void DefaultInverseLinearOp<Scalar>::initializeImpl(
  const Teuchos::RCP<LOWS> &lows,
  const SolveCriteria<Scalar> *fwdSolveCriteria,
  const EThrowOnSolveFailure throwOnFwdSolveFailure,
  const SolveCriteria<Scalar> *adjSolveCriteria,
  const EThrowOnSolveFailure throwOnAdjSolveFailure
  )
{
  lows_.initialize(lows);
  if(fwdSolveCriteria)
    fwdSolveCriteria_ = Teuchos::rcp(new SolveCriteria<Scalar>(*fwdSolveCriteria));
  else
    fwdSolveCriteria_ = Teuchos::null;
  if(adjSolveCriteria)
    adjSolveCriteria_ = Teuchos::rcp(new SolveCriteria<Scalar>(*adjSolveCriteria));
  else
    adjSolveCriteria_ = Teuchos::null;
  throwOnFwdSolveFailure_ = throwOnFwdSolveFailure;
  throwOnAdjSolveFailure_ = throwOnAdjSolveFailure;
  const std::string lowsLabel = lows_.getConstObj()->getObjectLabel();
  if(lowsLabel.length())
    this->setObjectLabel( "inv("+lowsLabel+")" );
}


} // end namespace Thyra


// Related non-member functions


template<class Scalar>
Teuchos::RCP<Thyra::LinearOpBase<Scalar> >
Thyra::nonconstInverse(
  const Teuchos::RCP<LinearOpWithSolveBase<Scalar> > &A,
  const SolveCriteria<Scalar> *fwdSolveCriteria,
  const EThrowOnSolveFailure throwOnFwdSolveFailure,
  const SolveCriteria<Scalar> *adjSolveCriteria,
  const EThrowOnSolveFailure throwOnAdjSolveFailure
  )
{
  return Teuchos::rcp(
    new DefaultInverseLinearOp<Scalar>(
      A,fwdSolveCriteria,throwOnFwdSolveFailure,
      adjSolveCriteria,throwOnAdjSolveFailure
      )
    );
}

template<class Scalar>
Teuchos::RCP<Thyra::LinearOpBase<Scalar> >
Thyra::inverse(
 const Teuchos::RCP<const LinearOpWithSolveBase<Scalar> > &A,
 const SolveCriteria<Scalar> *fwdSolveCriteria,
 const EThrowOnSolveFailure throwOnFwdSolveFailure,
 const SolveCriteria<Scalar> *adjSolveCriteria,
 const EThrowOnSolveFailure throwOnAdjSolveFailure
  )
{
  return Teuchos::rcp(
    new DefaultInverseLinearOp<Scalar>(
      A,fwdSolveCriteria,throwOnFwdSolveFailure,
      adjSolveCriteria,throwOnAdjSolveFailure
      )
    );
}


#endif	// THYRA_DEFAULT_INVERSE_LINEAR_OP_HPP
