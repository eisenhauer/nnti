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


#ifndef THYRA_DELAYED_LINEAR_OP_WITH_SOLVE_DECL_HPP
#define THYRA_DELAYED_LINEAR_OP_WITH_SOLVE_DECL_HPP


#include "Thyra_LinearOpWithSolveBase.hpp"
#include "Thyra_LinearOpSourceBase.hpp"


namespace Thyra {


/** \brief Delayed linear solver construction LinearOpWithSolveBase decorator
 * class.
 *
 * This simple class takes a LinearOpWithSolveFactoryBase object and the
 * arguments for a call to LinearOpWithSolveBase::initializeOp() (or one of
 * its related functions) and then waits until a solve is called before it
 * constructs the preconditioner.
 *
 * This class should never be created directly by a client.  Instead, it
 * should be created by a DelayedLinearOpWithSolveFactory object.
 */
template <class Scalar>
class DelayedLinearOpWithSolve : virtual public LinearOpWithSolveBase<Scalar>
{
public:

  /** \name Constructor/Initializers */
  //@{

  /** \brief . */
  DelayedLinearOpWithSolve();

  /** \brief . */
  void initialize(
    const RCP<const LinearOpSourceBase<Scalar> > &fwdOpSrc,
    const RCP<const PreconditionerBase<Scalar> > &prec,
    const RCP<const LinearOpSourceBase<Scalar> > &approxFwdOpSrc,
    const ESupportSolveUse supportSolveUse,
    const RCP<LinearOpWithSolveFactoryBase<Scalar> > &lowsf
    );

  /** \brief . */
  RCP<const LinearOpSourceBase<Scalar> > getFwdOpSrc() const;

  /** \brief . */
  RCP<const PreconditionerBase<Scalar> > getPrec() const;
 
  /** \brief . */
  RCP<const LinearOpSourceBase<Scalar> > getApproxFwdOpSrc() const;

  /** \brief . */
  ESupportSolveUse getSupportSolveUse() const;

  //@}

  /** \name Overridden from Teuchos::Describable. */
  //@{

  /** \brief . */
  std::string description() const;

  //@}

  /** @name Overridden from LinearOpBase */
  //@{

  /** \brief . */
  RCP< const VectorSpaceBase<Scalar> > range() const;
  /** \brief . */
  RCP< const VectorSpaceBase<Scalar> > domain() const;
  /** \brief . */
  RCP<const LinearOpBase<Scalar> > clone() const;

  //@}

protected:

  /** \brief Overridden from Teuchos::VerboseObjectBase */
  //@{
  /** \brief . */
  void informUpdatedVerbosityState() const;
  //@}

  /** @name Overridden from LinearOpBase  */
  //@{
  /** \brief . */
  virtual bool opSupportedImpl(EOpTransp M_trans) const;
  /** \brief . */
  virtual void applyImpl(
    const EOpTransp M_trans,
    const MultiVectorBase<Scalar> &X,
    const Ptr<MultiVectorBase<Scalar> > &Y,
    const Scalar alpha,
    const Scalar beta
    ) const;
  //@}

  /** @name Overridden from LinearOpWithSolveBase. */
  //@{
  /** \brief . */
  virtual bool solveSupportsImpl(EOpTransp M_trans) const;
  /** \brief . */
  virtual bool solveSupportsSolveMeasureTypeImpl(
    EOpTransp M_trans, const SolveMeasureType& solveMeasureType
    ) const;
  /** \brief . */
  SolveStatus<Scalar> solveImpl(
    const EOpTransp transp,
    const MultiVectorBase<Scalar> &B,
    const Ptr<MultiVectorBase<Scalar> > &X,
    const Ptr<const SolveCriteria<Scalar> > solveCriteria
    ) const;
  //@}

private:

  // //////////////////////
  // Private data members

  RCP<const LinearOpSourceBase<Scalar> > fwdOpSrc_;
  RCP<const PreconditionerBase<Scalar> > prec_;
  RCP<const LinearOpSourceBase<Scalar> > approxFwdOpSrc_;
  ESupportSolveUse supportSolveUse_;
  RCP<LinearOpWithSolveFactoryBase<Scalar> > lowsf_;

  RCP<const LinearOpBase<Scalar> > fwdOp_;

  mutable bool lows_is_valid_;
  mutable RCP<LinearOpWithSolveBase<Scalar> > lows_;

  // ///////////////////////////
  // Private member functions

  void updateSolver() const;

};


/** \brief Nonmember constuctor.
 *
 * \relates DelayedLinearOpWithSolve
 */
template <class Scalar>
RCP<DelayedLinearOpWithSolve<Scalar> > delayedLinearOpWithSolve()
{
  return Teuchos::rcp(new DelayedLinearOpWithSolve<Scalar>());
}


/** \brief Nonmember constuctor.
 *
 * \relates DelayedLinearOpWithSolve
 */
template <class Scalar>
RCP<DelayedLinearOpWithSolve<Scalar> >
delayedLinearOpWithSolve(
  const RCP<const LinearOpSourceBase<Scalar> > &fwdOpSrc,
  const RCP<LinearOpWithSolveFactoryBase<Scalar> > &lowsf
  )
{
  using Teuchos::null;
  const RCP<DelayedLinearOpWithSolve<Scalar> > dlows =
    delayedLinearOpWithSolve<Scalar>();
  dlows->initialize(fwdOpSrc, null, null, SUPPORT_SOLVE_UNSPECIFIED, lowsf);
  return dlows;
}


} // namespace Thyra


#endif // THYRA_DELAYED_LINEAR_OP_WITH_SOLVE_DECL_HPP
