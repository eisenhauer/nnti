/*
// @HEADER
// ***********************************************************************
// 
//    OptiPack: Collection of simple Thyra-based Optimization ANAs
//                 Copyright (2009) Sandia Corporation
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
// Questions? Contact Roscoe A. Bartlett (rabartl@sandia.gov) 
// 
// ***********************************************************************
// @HEADER
*/

#ifndef OPTIPACK_NONLINEAR_CG_DEF_HPP
#define OPTIPACK_NONLINEAR_CG_DEF_HPP


#include "OptiPack_NonlinearCG_decl.hpp"
#include "OptiPack_DefaultPolyLineSearchPointEvaluator.hpp"
#include "OptiPack_UnconstrainedOptMeritFunc1D.hpp"
#include "Thyra_ModelEvaluatorHelpers.hpp"
#include "Thyra_VectorStdOps.hpp"
#include "Teuchos_Tuple.hpp"


namespace OptiPack {


// Constructor/Initializers/Accessors


template<typename Scalar>
NonlinearCG<Scalar>::NonlinearCG()
  : paramIndex_(-1),
    responseIndex_(-1),
    alpha_init_(NonlinearCGUtils::alpha_init_default),
    alpha_reinit_(NonlinearCGUtils::alpha_reinit_default),
    and_conv_tests_(NonlinearCGUtils::and_conv_tests_default),
    minIters_(NonlinearCGUtils::minIters_default),
    maxIters_(NonlinearCGUtils::maxIters_default),
    g_mag_(NonlinearCGUtils::g_mag_default),
    numIters_(0)
{}


template<typename Scalar>
void NonlinearCG<Scalar>::initialize(
  const RCP<const Thyra::ModelEvaluator<Scalar> > &model,
  const int paramIndex,
  const int responseIndex,
  const RCP<GlobiPack::LineSearchBase<Scalar> > &linesearch
  )
{
  // ToDo: Validate input objects!
  model_ = model.assert_not_null();
  paramIndex_ = paramIndex;
  responseIndex_ = responseIndex;
  linesearch_ = linesearch.assert_not_null();
}


template<typename Scalar>
const typename NonlinearCG<Scalar>::ScalarMag
NonlinearCG<Scalar>::get_alpha_init() const
{
  return alpha_init_;
}


template<typename Scalar>
const bool NonlinearCG<Scalar>::get_alpha_reinit() const
{
  return alpha_reinit_;
}


template<typename Scalar>
const bool NonlinearCG<Scalar>::get_and_conv_tests() const
{
  return and_conv_tests_;
}


template<typename Scalar>
const int NonlinearCG<Scalar>::get_minIters() const
{
  return minIters_;
}


template<typename Scalar>
const int NonlinearCG<Scalar>::get_maxIters() const
{
  return maxIters_;
}


template<typename Scalar>
const typename NonlinearCG<Scalar>::ScalarMag
NonlinearCG<Scalar>::get_g_mag() const
{
  return g_mag_;
}


// Overridden from ParameterListAcceptor (simple forwarding functions)


template<typename Scalar>
void NonlinearCG<Scalar>::setParameterList(RCP<ParameterList> const& paramList)
{
  typedef ScalarTraits<Scalar> ST;
  typedef ScalarTraits<ScalarMag> SMT;
  namespace NCGU = NonlinearCGUtils;
  using Teuchos::getParameter;
  paramList->validateParametersAndSetDefaults(*this->getValidParameters());
  alpha_init_ = getParameter<double>(*paramList, NCGU::alpha_init_name);
  alpha_reinit_ = getParameter<bool>(*paramList, NCGU::alpha_reinit_name);
  and_conv_tests_ = getParameter<bool>(*paramList, NCGU::and_conv_tests_name);
  minIters_ = getParameter<int>(*paramList, NCGU::minIters_name);
  maxIters_ = getParameter<int>(*paramList, NCGU::maxIters_name);
  g_mag_ = getParameter<double>(*paramList, NCGU::g_mag_name);
  TEUCHOS_ASSERT_INEQUALITY( alpha_init_, >, SMT::zero() );
  TEUCHOS_ASSERT_INEQUALITY( minIters_, >=, 0 );
  TEUCHOS_ASSERT_INEQUALITY( minIters_, <, maxIters_ );
  TEUCHOS_ASSERT_INEQUALITY( g_mag_, >, SMT::zero() );
  setMyParamList(paramList);
}


template<typename Scalar>
RCP<const ParameterList>
NonlinearCG<Scalar>::getValidParameters() const
{
  namespace NCGU = NonlinearCGUtils;
  static RCP<const ParameterList> validPL;
  if (is_null(validPL)) {
    RCP<Teuchos::ParameterList>
      pl = Teuchos::rcp(new Teuchos::ParameterList());
    pl->set( NCGU::alpha_init_name, NCGU::alpha_init_default );
    pl->set( NCGU::alpha_reinit_name, NCGU::alpha_reinit_default );
    pl->set( NCGU::and_conv_tests_name, NCGU::and_conv_tests_default );
    pl->set( NCGU::minIters_name, NCGU::minIters_default );
    pl->set( NCGU::maxIters_name, NCGU::maxIters_default );
    pl->set( NCGU::g_mag_name, NCGU::g_mag_default );
    validPL = pl;
  }
  return validPL;
}


// Solve


template<typename Scalar>
NonlinearCGUtils::ESolveReturn
NonlinearCG<Scalar>::doSolve(
  const Ptr<Thyra::VectorBase<Scalar> > &p_inout,
  const Ptr<ScalarMag> &g_opt_out,
  const Ptr<const ScalarMag> &g_reduct_tol_in,
  const Ptr<const ScalarMag> &g_grad_tol_in,
  const Ptr<const ScalarMag> &alpha_init,
  const Ptr<int> &numIters_out
  )
{

  typedef ScalarTraits<Scalar> ST;
  typedef ScalarTraits<ScalarMag> SMT;
  
  using Teuchos::null;
  using Teuchos::as;
  using Teuchos::tuple;
  using Teuchos::rcpFromPtr;
  using Teuchos::optInArg;
  using Teuchos::inOutArg;
  using GlobiPack::computeValue;
  using Thyra::VectorSpaceBase;
  using Thyra::VectorBase;
  using Thyra::MultiVectorBase;
  using Thyra::scalarProd;
  using Thyra::createMember;
  using Thyra::createMembers;
  using Thyra::get_ele;
  using Thyra::norm;
  using Thyra::V_StV;
  using Thyra::Vt_S;
  using Thyra::eval_g_DgDp;
  typedef Thyra::ModelEvaluatorBase MEB;

  // Validate input

  g_opt_out.assert_not_null();

  // Set streams

  const RCP<Teuchos::FancyOStream> out = this->getOStream();
  linesearch_->setOStream(out);

  //
  // A) Set up the storage for the algorithm
  //
  
  const RCP<DefaultPolyLineSearchPointEvaluator<Scalar> >
    pointEvaluator = defaultPolyLineSearchPointEvaluator<Scalar>();

  const RCP<UnconstrainedOptMeritFunc1D<Scalar> >
    meritFunc = unconstrainedOptMeritFunc1D<Scalar>(
      model_, paramIndex_, responseIndex_ );

  const RCP<const VectorSpaceBase<Scalar> >
    p_space = model_->get_p_space(paramIndex_),
    g_space = model_->get_g_space(responseIndex_);

  // Stoarge for current iteration
  RCP<VectorBase<Scalar> >
    p = rcpFromPtr(p_inout),        // Current solution for p
    p_new = createMember(p_space),  // Trial point for p (in line search)
    g_v = createMember(g_space),    // Vector (size 1) form of objective g(p) 
    g_grad = createMember(p_space), // Gradient of g DgDp^T
    d = createMember(p_space);      // Search direction

  // Storage for previous iteration
  RCP<VectorBase<Scalar> >
    g_grad_last = createMember(p_space);
  ScalarMag
    alpha_last = SMT::zero(),
    g_last = SMT::zero(),
    g_grad_inner_g_grad_last = SMT::zero(),
    g_grad_inner_d_last = SMT::zero();
  

  //
  // B) Do the nonlinear CG iterations
  //

  *out << "\nStarting nonlinear CG iterations ...\n";

  if (and_conv_tests_) {
    *out << "\nNOTE: Using an AND of convergence tests!\n";
  }
  else {
    *out << "\nNOT: Using an OR of convergence tests!\n";
  }

  bool foundSolution = false;
  bool linesearchFailure = false;
  bool linsearchFailureLastIter = false;
  const Scalar g_reduct_tol =
    ( !is_null(g_reduct_tol_in) ? *g_reduct_tol_in : as<Scalar>(1e-5)); // ToDo: Get from PL
  const Scalar g_grad_tol =
    ( !is_null(g_grad_tol_in) ? *g_grad_tol_in : as<Scalar>(1e-5)); // ToDo: Get from PL

  for (numIters_ = 0; numIters_ < maxIters_; ++numIters_) {

    Teuchos::OSTab tab(out);

    *out << "\nNonlinear CG Iteration k = " << numIters_ << "\n";

    Teuchos::OSTab tab2(out);

    //
    // B.1) Evaluate the point (on first iteration)
    //
    
    eval_g_DgDp(
      *model_, paramIndex_, *p, responseIndex_,
      numIters_ == 0 ? g_v.ptr() : null, // Only on first iteration
      MEB::Derivative<Scalar>(g_grad, MEB::DERIV_MV_GRADIENT_FORM) );

    const ScalarMag g = get_ele(*g_v, 0);
    // Above: If numIters_ > 0, then g_v was updated in meritFunc->eval(...).

    //
    // B.2) Check for convergence
    //

    // B.2.a) ||g - g_last|| |g + g_mag| <= g_reduct_tol

    const ScalarMag g_reduct = g - g_last;

    *out << "\ng_k - g_km1 = "<<g_reduct<<"\n";

    const ScalarMag g_reduct_err =
      SMT::magnitude(g_reduct / SMT::magnitude(g + g_mag_));

    const bool g_reduct_converged = (g_reduct_err <= g_reduct_tol);

    *out << "\nCheck convergence: |g_k - g_km1| / |g_k + g_mag| = "<<g_reduct_err
         << (g_reduct_converged ? " <= " : " > ")
         << "g_reduct_tol = "<<g_reduct_tol<<"\n";

    // B.2.b) ||g_grad|| g_mag <= g_grad_tol

    const Scalar g_grad_inner_g_grad = scalarProd<Scalar>(*g_grad, *g_grad);
    const ScalarMag norm_g_grad = ST::magnitude(ST::squareroot(g_grad_inner_g_grad));

    *out << "\n||g_grad_k|| = "<<norm_g_grad << "\n";

    const ScalarMag g_grad_err = norm_g_grad / g_mag_;

    const bool g_grad_converged = (g_grad_err <= g_grad_tol);

    *out << "\nCheck convergence: ||g_grad_k|| / g_mag = "<<g_grad_err
         << (g_grad_converged ? " <= " : " > ")
         << "g_grad_tol = "<<g_grad_tol<<"\n";

    // B.2.c) Convergence status
    
    bool isConverged = false;
    if (and_conv_tests_) {
      isConverged = g_reduct_converged && g_grad_converged;
    }
    else {
      isConverged = g_reduct_converged || g_grad_converged;
    }

    if (isConverged) {
      if (numIters_ < minIters_) {
        *out << "\nnumIters="<<numIters_<<" < minIters="<<minIters_
             << ", continuing on!\n";
      }
      else {
        *out << "\nFound solution, existing algorithm!\n";
        foundSolution = true;
      }
    }
    else {
      *out << "\nNot converged!\n";
    }
    
    if (foundSolution) {
      break;
    }

    //
    // B.3) Compute the search direction
    //

    if (numIters_ == 0) {

      // p = -g_grad
      V_StV( d.ptr(), as<Scalar>(-1.0), *g_grad );

    }
    else {

      // beta_FR = inner(g_grad, g_grad) / inner(g_grad_last, g_grad_last)
      const Scalar beta =
        g_grad_inner_g_grad / g_grad_inner_g_grad_last;

      // d = beta * d_last + -g_grad
      Vt_S( d.ptr(), beta );
      Vp_StV( d.ptr(), as<Scalar>(-1.0), *g_grad );

    }
    
    //
    // B.4) Perform the line search
    //

    // B.4.a) Compute the initial step length

    Scalar alpha = as<Scalar>(-1.0);

    if (numIters_ == 0) {
      if (!is_null(alpha_init)) {
        alpha = *alpha_init;
      }
      else {
        TEST_FOR_EXCEPT(true); // ToDo: Grab from the PL!
      }
    }
    else {
      alpha = alpha_last;
      // ToDo: Add an option to the PL to always set this to the initial alpha
      // given by the user (either through the PL or through the function
      // argument alpha_init).
    }


    // B.4.b) Perform the linesearch (computing updated quantities in process)

    pointEvaluator->initialize(
      tuple<RCP<const VectorBase<Scalar> > >(p, d)() );

    const ScalarMag g_grad_inner_d = scalarProd(*g_grad, *d);

    // Set up the merit function to only compute the value but give it its
    // initial descent derivative.
    meritFunc->setEvaluationQuantities(
      pointEvaluator, p_new, g_v, null, optInArg(g_grad_inner_d) );

    ScalarMag g_new = computeValue(*meritFunc, alpha); // Updates p_new and g_v as well!

    const bool linesearchResult = linesearch_->doLineSearch(
      *meritFunc, g, inOutArg(alpha), inOutArg(g_new), null );

    if (!linesearchResult) {
      if (!linsearchFailureLastIter) {
        *out << "\nLine search failure, but doing another iteration anyway!\n";
        linsearchFailureLastIter = true;
      }
      else {
        *out << "\nLine search failure, terminating nonlinear CG algorithm!\n";
        linesearchFailure = true;
      }
    }

    if (linesearchFailure) {
      break;
    }

    //
    // B.5) Transition to the next iteration
    //
    
    alpha_last = alpha;
    g_last = g;
    g_grad_inner_g_grad_last = g_grad_inner_g_grad;
    g_grad_inner_d_last = g_grad_inner_d;
    std::swap(g_grad, g_grad_last);
    std::swap(p_new, p);
    
  }

  //
  // C) Final clean up
  //

  *g_opt_out = get_ele(*g_v, 0);

  // Make sure that the final value for p has been copied in!
  V_V( p_inout, *p );

  if (!is_null(numIters_out)) {
    *numIters_out = numIters_;
  }

  if (numIters_ == maxIters_) {
    *out << "\nMax nonlinear CG iterations exceeded!\n";
  }
  
  if (foundSolution) {
    return NonlinearCGUtils::SOLVE_SOLUTION_FOUND;
  }
  else if(linesearchFailure) {
    return NonlinearCGUtils::SOLVE_LINSEARCH_FAILURE;
  }

  // Else, the max number of iterations was exceeded
  return NonlinearCGUtils::SOLVE_MAX_ITERS_EXCEEDED;

}


} // namespace OptiPack


#endif // OPTIPACK_NONLINEAR_CG_DEF_HPP
