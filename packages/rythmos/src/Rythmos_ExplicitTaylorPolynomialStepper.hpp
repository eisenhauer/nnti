//@HEADER
// ***********************************************************************
//
//                           Rythmos Package
//                 Copyright (2006) Sandia Corporation
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
// Questions? Contact Todd S. Coffey (tscoffe@sandia.gov)
//
// ***********************************************************************
//@HEADER

#ifndef RYTHMOS_EXPLICIT_TAYLOR_POLYNOMIAL_STEPPER_H
#define RYTHMOS_EXPLICIT_TAYLOR_POLYNOMIAL_STEPPER_H

#include "Rythmos_StepperBase.hpp"
#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Thyra_VectorBase.hpp"
#include "Thyra_ModelEvaluator.hpp"
#include "Thyra_ModelEvaluatorHelpers.hpp"
#include "RTOpPack_RTOpTHelpers.hpp"

namespace Rythmos {

  //! Reduction operator for a logarithmic infinity norm
  /*!
   * This class implements a reduction operator for computing the 
   * logarithmic infinity norm of a vector:
   * \f[
   *      \|1 + log(x)\|_\infty.
   * \f]
   */
  RTOP_ROP_1_REDUCT_SCALAR( ROpLogNormInf,
    typename ScalarTraits<Scalar>::magnitudeType, // Reduction object type
    RTOpPack::REDUCT_TYPE_MAX // Reduction object reduction
    )
  {
    using Teuchos::as;
    typedef ScalarTraits<Scalar> ST;
    typedef typename ST::magnitudeType ScalarMag;
    const ScalarMag mag = std::log(as<ScalarMag>(1e-100) + ST::magnitude(v0));
    reduct = TEUCHOS_MAX( mag, reduct );
  }

  /*!
   * \brief Implementation of Rythmos::Stepper for explicit Taylor polynomial
   * time integration of ODEs.
   */
  /*!
   * Let 
   * \f[
   *     \frac{dx}{dt} = f(x,t), \quad x(t_0) = a
   * \f]
   * be an ODE initial-value problem.  This class implements a single time
   * step of an explicit Taylor polynomial time integration method for
   * computing numerical solutions to the IVP.  The method consists of 
   * computing a local truncated Taylor series solution to the ODE (section
   * \ref Rythmos_ETI_local_TS), estimating a step size within the radius
   * of convergence of the Taylor series (section \ref Rythmos_ETI_stepsize)
   * and then summing the polynomial at that step to compute the next
   * point in the numerical integration (section \ref Rythmos_ETI_sum).
   * The algorithmic parameters to the method are controlled through the
   * <tt> params </tt> argument to the constructor which are described in
   * section \ref Rythmos_ETI_params.
   *
   * \section Rythmos_ETI_local_TS Computing the Taylor Polynomial
   * 
   * Let
   * \f[
   *      x(t) = \sum_{k=0}^\infty x_k (t-t_0)^k 
   * \f]
   * be a power series solution to the IVP above.  Then \f$f(x(t))\f$ can
   * be expaned in a power series along the solution curve \f$x(t)\f$:
   * \f[
   *      f(x(t),t) = \sum_{k=0}^\infty f_k (t-t_0)^k
   * \f]
   * where 
   * \f[
   *      f_k = \left.\frac{1}{k!}\frac{d^k}{dt^k} f(x(t),t)\right|_{t=t_0}.
   * \f]
   * By differentiating the power series for \f$x(t)\f$ to compute a power
   * series for \f$dx/dt\f$ and then comparing coefficients in the 
   * equation \f$dx/dt=f(x(t),t)\f$ we find the following recurrence
   * relationship for the Taylor coefficients \f$\{x_k\}\f$:
   * \f[
   *     x_{k+1} = \frac{1}{k+1} f_k, \quad k=0,1,\dots
   * \f]
   * where each coefficient \f$f_k\f$ is a function only of 
   * \f$x_0,\dots,x_k\f$ and can be efficiently computed using the Taylor
   * polynomial mode of automatic differentation.  This allows the Taylor
   * coefficients to be iteratively computed starting with the initial point
   * \f$x_0\f$ up to some fixed degree \f$d\f$ to yield a local truncated 
   * Taylor polynomial approximating the solution to the IVP.
   *
   * \section Rythmos_ETI_stepsize Computing a Step Size
   *
   * With the truncated Taylor polynomial solution 
   * \f$\tilde{x}(t) = \sum_{k=0}^d x_k (t-t_0)^k\f$ in hand, a step size
   * is chosen by estimating the truncation error in the polynomial solution
   * and forcing this error to be less than some prescribed tolerance.  Let
   * \f[
   *     \rho = \max_{d/2\leq k\leq d} (1+\|x_k\|_\infty)^{1/k}
   * \f]
   * so \f$\|x_k\|_\infty\leq\rho^k\f$ for \f$d/2\leq k \leq d\f$.  Assume 
   * \f$\|x_k\|\leq\rho^k\f$ for \f$k>d\f$ as well, then for any \f$h<1/\rho\f$
   * it can be shown that the truncation error is bounded by
   * \f[
   *      \frac{(\rho h)^{d+1}}{1-\rho h}.
   * \f]
   * A step size \f$h\f$ is then given by
   * \f[
   *      h = \exp\left(\frac{1}{d+1}\log\varepsilon-\log\rho\right)
   * \f]
   * for some error tolerance \f$\varepsilon\f$ given an error of approximatly
   * \f$\varepsilon\f$.
   *
   * \section Rythmos_ETI_sum Summing the Polynomial
   *
   * With a step size \f$h\f$ computed, 
   * \f[
   *     x^\ast = \sum_{k=0}^d x_k h^k
   * \f]
   * is used as the next integration point where a new Taylor series is
   * calculated.  Local error per step can also be controlled by computing
   * \f$\|dx^\ast/dt - f(x^\ast)\|_\infty\f$.  If this error is too large,
   * the step size can be reduced to an appropriate size.
   *
   * \section Rythmos_ETI_params Parameters
   *
   * This method recognizes the following algorithmic parameters that can
   * be set in the <tt> params </tt> argument to the constructor:
   * <ul>
   * <li> "Initial Time" (Scalar) [Default = 0] Initial integration time
   * <li> "Final Time"   (Scalar) [Default = 1] Final integration time
   * <li> "Local Error Tolerance" (Magnitude) [Default = 1.0e-10] Error tolerance on \f$\|dx^\ast/dt - f(x^\ast)\|_\infty\f$ as described above.
   * <li> "Minimum Step Size" (Scalar) [Default = 1.0e-10] Minimum step size
   * <li> "Maximum Step Size" (Scalar) [Default = 1.0] Maximum step size
   * <li> "Taylor Polynomial Degree" (int) [Default = 40] Degree of local Taylor polynomial approximation.
   * </ul>
   */

  template<class Scalar>
  class ExplicitTaylorPolynomialStepper : virtual public StepperBase<Scalar>
  {
    public:

    //! Typename of magnitude of scalars
    typedef typename Teuchos::ScalarTraits<Scalar>::magnitudeType ScalarMag;
    
    //! Constructor
    ExplicitTaylorPolynomialStepper();
    
    //! Destructor
    ~ExplicitTaylorPolynomialStepper();

    //! Return the space for <tt>x</tt> and <tt>x_dot</tt>
    RCP<const Thyra::VectorSpaceBase<Scalar> > get_x_space() const;

    //! Set model
    void setModel(const Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> > &model);

    /** \brief . */
    Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> >
    getModel() const;
    
    //! Take a time step of magnitude \c dt
    Scalar takeStep(Scalar dt, StepSizeType flag);

    /** \brief . */
    const StepStatus<Scalar> getStepStatus() const;

    /// Redefined from Teuchos::ParameterListAcceptor
    /** \brief . */
    void setParameterList(Teuchos::RCP<Teuchos::ParameterList> const& paramList);

    /** \brief . */
    Teuchos::RCP<Teuchos::ParameterList> getNonconstParameterList();

    /** \brief . */
    Teuchos::RCP<Teuchos::ParameterList> unsetParameterList();

    /** \brief . */
    std::string description() const;

    /** \brief . */
    std::ostream& describe(
      std::ostream                &out
      ,const Teuchos::EVerbosityLevel      verbLevel
      ,const std::string          leadingIndent
      ,const std::string          indentSpacer
      ) const;

    /// Redefined from InterpolationBufferBase 
    /// Add points to buffer
    void addPoints(
      const Array<Scalar>& time_vec
      ,const Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >& x_vec
      ,const Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >& xdot_vec
      );
    
    /// Get values from buffer
    void getPoints(
      const Array<Scalar>& time_vec
      ,Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >* x_vec
      ,Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >* xdot_vec
      ,Array<ScalarMag>* accuracy_vec) const;

    /// Fill data in from another interpolation buffer
    void setRange(
      const TimeRange<Scalar>& range,
      const InterpolationBufferBase<Scalar> & IB
      );

    /** \brief . */
    TimeRange<Scalar> getTimeRange() const;

    /// Get interpolation nodes
    void getNodes(Array<Scalar>* time_vec) const;

    /// Remove interpolation nodes
    void removeNodes(Array<Scalar>& time_vec);

    /// Get order of interpolation
    int getOrder() const;

    private:

    //! Computes a local Taylor series solution to the ODE
    void computeTaylorSeriesSolution_();

    /*! 
     * \brief Computes of log of the estimated radius of convergence of the 
     * Taylor series.
     */
    ScalarMag estimateLogRadius_();

    //! Underlying model
    Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> > model_;

    //! Parameter list
    Teuchos::RCP<Teuchos::ParameterList> parameterList_;

    //! Current solution vector
    Teuchos::RCP<Thyra::VectorBase<Scalar> > x_vector_;

    //! Vector store approximation to \f$dx/dt\f$
    Teuchos::RCP<Thyra::VectorBase<Scalar> > x_dot_vector_;

    //! Vector store ODE residual
    Teuchos::RCP<Thyra::VectorBase<Scalar> > f_vector_;

    //! Polynomial for x
    Teuchos::RCP<Teuchos::Polynomial<Thyra::VectorBase<Scalar> > > x_poly_;

    //! Polynomial for f
    Teuchos::RCP<Teuchos::Polynomial<Thyra::VectorBase<Scalar> > > f_poly_;

    //! Current time
    Scalar t_;

    //! Current step size
    Scalar dt_;

    //! Initial integration time
    Scalar t_initial_;

    //! Final integration time
    Scalar t_final_;

    //! Local error tolerance for each time step
    ScalarMag local_error_tolerance_;

    //! Smallest acceptable time step size
    Scalar min_step_size_;

    //! Largest acceptable time step size
    Scalar max_step_size_;

    //! Degree of local Taylor series expansion
    unsigned int degree_;

    //! Used in time step size computation
    Scalar linc_;
  };

  //! Computs logarithmic infinity norm of a vector using ROpLogNormInf.
  template <typename Scalar>
  typename Teuchos::ScalarTraits<Scalar>::magnitudeType
  log_norm_inf(const Thyra::VectorBase<Scalar>& x)
  {
    ROpLogNormInf<Scalar> log_norm_inf_op;
    Teuchos::RCP<RTOpPack::ReductTarget> log_norm_inf_targ = 
      log_norm_inf_op.reduct_obj_create();
    const Thyra::VectorBase<Scalar>* vecs[] = { &x };
    Thyra::applyOp<Scalar>(log_norm_inf_op,1,vecs,0,
			   (Thyra::VectorBase<Scalar>**)NULL,
			   log_norm_inf_targ.get());
    
    return log_norm_inf_op(*log_norm_inf_targ);
  }

  template<class Scalar>
  ExplicitTaylorPolynomialStepper<Scalar>::ExplicitTaylorPolynomialStepper()
  {
  }

  template<class Scalar>
  ExplicitTaylorPolynomialStepper<Scalar>::~ExplicitTaylorPolynomialStepper()
  {
  }

  template<class Scalar>
  void ExplicitTaylorPolynomialStepper<Scalar>::setModel(
    const Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> > &model
    )
  {
    TEST_FOR_EXCEPT(model == Teuchos::null)
    
    model_ = model;
    x_vector_ = model_->getNominalValues().get_x()->clone_v();
    x_dot_vector_ = Thyra::createMember(model_->get_x_space());
    f_vector_ = Thyra::createMember(model_->get_f_space());
  }


  template<class Scalar>
  Teuchos::RCP<const Thyra::ModelEvaluator<Scalar> >
  ExplicitTaylorPolynomialStepper<Scalar>::getModel() const
  {
    return model_;
  }

  template<class Scalar>
  Scalar 
  ExplicitTaylorPolynomialStepper<Scalar>::takeStep(Scalar dt, StepSizeType flag)
  {
    if (x_poly_ == Teuchos::null)
      x_poly_ = 
	Teuchos::rcp(new Teuchos::Polynomial<
		     Thyra::VectorBase<Scalar> >(0,*x_vector_,degree_));

    if (f_poly_ == Teuchos::null)
      f_poly_ = 
	Teuchos::rcp(new Teuchos::Polynomial<
		     Thyra::VectorBase<Scalar> >(0, *f_vector_, degree_));
    if (flag == STEP_TYPE_VARIABLE) {
      // If t_ > t_final_, we're done
      if (t_ > t_final_) {
        dt_ = Teuchos::ScalarTraits<Scalar>::zero();
        return dt_;
      }

      // Compute a local truncated Taylor series solution to system
      computeTaylorSeriesSolution_();

      // Estimate log of radius of convergence of Taylor series
      Scalar rho = estimateLogRadius_();

      // Set step size
      Scalar dt = std::exp(linc_ - rho);

      // If step size is too big, reduce
      if (dt > max_step_size_) {
        dt = max_step_size_;
      }

      // If step goes past t_final_, reduce
      if (t_+dt > t_final_) {
        dt = t_final_-t_;
      }

      ScalarMag local_error;

      do {

        // compute x(t_+dt), xdot(t_+dt)
        x_poly_->evaluate(dt, x_vector_.get(), x_dot_vector_.get());

        // compute f( x(t_+dt), t_+dt )
        Thyra::eval_f(*model_, *x_vector_, t_+dt, f_vector_.get());

        // compute || xdot(t_+dt) - f( x(t_+dt), t_+dt ) ||
        Thyra::Vp_StV(x_dot_vector_.get(), -Teuchos::ScalarTraits<Scalar>::one(),
          *f_vector_);
        local_error = norm_inf(*x_dot_vector_);

        if (local_error > local_error_tolerance_) {
          dt *= 0.7;
        }

      } while (local_error > local_error_tolerance_ && dt > min_step_size_);

      // Check if minimum step size was reached
      TEST_FOR_EXCEPTION(dt < min_step_size_, 
            std::runtime_error,
            "ExplicitTaylorPolynomialStepper<Scalar>::takeStep(): " 
            << "Step size reached minimum step size " 
            << min_step_size_ << ".  Failing step." );

      // Increment t_
      t_ += dt;

      dt_ = dt;

      return dt;

    } else {

      // If t_ > t_final_, we're done
      if (t_ > t_final_) {
        dt_ = Teuchos::ScalarTraits<Scalar>::zero();
        return dt_;
      }

      // Compute a local truncated Taylor series solution to system
      computeTaylorSeriesSolution_();

      // If step size is too big, reduce
      if (dt > max_step_size_) {
        dt = max_step_size_;
      }

      // If step goes past t_final_, reduce
      if (t_+dt > t_final_) {
        dt = t_final_-t_;
      }

      // compute x(t_+dt)
      x_poly_->evaluate(dt, x_vector_.get());

      // Increment t_
      t_ += dt;

      dt_ = dt;

      return dt;
    }
  }


  template<class Scalar>
  const StepStatus<Scalar>
  ExplicitTaylorPolynomialStepper<Scalar>::getStepStatus() const
  {
    typedef Teuchos::ScalarTraits<Scalar> ST;
    StepStatus<Scalar> stepStatus;

    stepStatus.stepSize = dt_;
    stepStatus.order = this->getOrder();
    stepStatus.time = t_;
    stepStatus.solution = x_vector_;
    stepStatus.solutionDot = x_dot_vector_;
    stepStatus.residual = f_vector_;

    return(stepStatus);
  }

  template<class Scalar>
  void ExplicitTaylorPolynomialStepper<Scalar>::setParameterList(Teuchos::RCP<Teuchos::ParameterList> const& paramList)
  {
    typedef Teuchos::ScalarTraits<Scalar> ST;

    parameterList_ = paramList;

    // Get initial time
    t_initial_ = parameterList_->get("Initial Time", ST::zero());

    // Get final time
    t_final_ = parameterList_->get("Final Time", ST::one());

    // Get local error tolerance
    local_error_tolerance_ = 
      parameterList_->get("Local Error Tolerance", ScalarMag(1.0e-10));

    // Get minimum step size
    min_step_size_ = parameterList_->get("Minimum Step Size", Scalar(1.0e-10));

    // Get maximum step size
    max_step_size_ = parameterList_->get("Maximum Step Size", Scalar(1.0));

    // Get degree_ of Taylor polynomial expansion
    degree_ = parameterList_->get("Taylor Polynomial Degree", 40);

    linc_ = Scalar(-16.0*std::log(10.0)/degree_);
    t_ = t_initial_;
  }

  template<class Scalar>
  Teuchos::RCP<Teuchos::ParameterList>
  ExplicitTaylorPolynomialStepper<Scalar>::getNonconstParameterList()
  {
    return parameterList_;
  }

  template<class Scalar>
  Teuchos::RCP<Teuchos::ParameterList>
  ExplicitTaylorPolynomialStepper<Scalar>:: unsetParameterList()
  {
    Teuchos::RCP<Teuchos::ParameterList> temp_param_list = parameterList_;
    parameterList_ = Teuchos::null;
    return temp_param_list;
  }

  template<class Scalar>
  std::string ExplicitTaylorPolynomialStepper<Scalar>::description() const
  {
    std::string name = "Rythmos::ExplicitTaylorPolynomialStepper";
    return name;
  }

  template<class Scalar>
  std::ostream& ExplicitTaylorPolynomialStepper<Scalar>::describe(
        std::ostream                &out
        ,const Teuchos::EVerbosityLevel      verbLevel
        ,const std::string          leadingIndent
        ,const std::string          indentSpacer
        ) const
  {
    if (verbLevel == Teuchos::VERB_EXTREME) {
      out << description() << "::describe" << std::endl;
      out << "model_ = " << std::endl;
      out << model_->describe(out,verbLevel,leadingIndent,indentSpacer) << std::endl;
      out << "x_vector_ = " << std::endl;
      out << x_vector_->describe(out,verbLevel,leadingIndent,indentSpacer) << std::endl;
      out << "x_dot_vector_ = " << std::endl;
      out << x_dot_vector_->describe(out,verbLevel,leadingIndent,indentSpacer) << std::endl;
      out << "f_vector_ = " << std::endl;
      out << f_vector_->describe(out,verbLevel,leadingIndent,indentSpacer) << std::endl;
      out << "x_poly_ = " << std::endl;
      out << x_poly_->describe(out,verbLevel,leadingIndent,indentSpacer) << std::endl;
      out << "f_poly_ = " << std::endl;
      out << f_poly_->describe(out,verbLevel,leadingIndent,indentSpacer) << std::endl;
      out << "t_ = " << t_ << std::endl;
      out << "t_initial_ = " << t_initial_ << std::endl;
      out << "t_final_ = " << t_final_ << std::endl;
      out << "local_error_tolerance_ = " << local_error_tolerance_ << std::endl;
      out << "min_step_size_ = " << min_step_size_ << std::endl;
      out << "max_step_size_ = " << max_step_size_ << std::endl;
      out << "degree_ = " << degree_ << std::endl;
      out << "linc_ = " << linc_ << std::endl;
    }
    return(out);
  }


  template<class Scalar>
  void ExplicitTaylorPolynomialStepper<Scalar>::addPoints(
    const Array<Scalar>& time_vec
    ,const Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >& x_vec
    ,const Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >& xdot_vec
    )
  {
    TEST_FOR_EXCEPTION(true,std::logic_error,"Error, addPoints is not implemented for the ExplicitTaylorPolynomialStepper.\n");
  }

  template<class Scalar>
  void ExplicitTaylorPolynomialStepper<Scalar>::getPoints(
    const Array<Scalar>& time_vec
    ,Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >* x_vec
    ,Array<Teuchos::RCP<const Thyra::VectorBase<Scalar> > >* xdot_vec
    ,Array<ScalarMag>* accuracy_vec) const
  {
    TEST_FOR_EXCEPTION(true,std::logic_error,"Error, getPoints is not implemented for the ExplicitTaylorPolynomialStepper.\n");
  }

  template<class Scalar>
  TimeRange<Scalar> ExplicitTaylorPolynomialStepper<Scalar>::getTimeRange() const
  {
    return invalidTimeRange<Scalar>();
  }

  template<class Scalar>
  void ExplicitTaylorPolynomialStepper<Scalar>::getNodes(Array<Scalar>* time_list) const
  {
    TEST_FOR_EXCEPTION(true,std::logic_error,"Error, getNodes is not implemented for the ExplicitTaylorPolynomialStepper.\n");
  }

  template<class Scalar>
  void ExplicitTaylorPolynomialStepper<Scalar>::removeNodes(Array<Scalar>& time_vec)
  {
    TEST_FOR_EXCEPTION(true,std::logic_error,"Error, removeNodes is not implemented for the ExplicitTaylorPolynomialStepper.\n");
  }


  template<class Scalar>
  int ExplicitTaylorPolynomialStepper<Scalar>::getOrder() const
  {
    return degree_;
  }

  //
  // Definitions of protected methods
  //

  template<class Scalar>
  void
  ExplicitTaylorPolynomialStepper<Scalar>::computeTaylorSeriesSolution_()
  {
    Teuchos::RCP<Thyra::VectorBase<Scalar> > tmp;

    // Set degree_ of polynomials to 0
    x_poly_->setDegree(0);
    f_poly_->setDegree(0);

    // Set degree_ 0 coefficient
    x_poly_->setCoefficient(0, *x_vector_);

    for (unsigned int k=1; k<=degree_; k++) {

      // compute [f] = f([x])
      Thyra::eval_f_poly(*model_, *x_poly_, t_, f_poly_.get());

      x_poly_->setDegree(k);
      f_poly_->setDegree(k);
      
      // x[k] = f[k-1] / k
      tmp = x_poly_->getCoefficient(k);
      copy(*(f_poly_->getCoefficient(k-1)), tmp.get());
      scale(Scalar(1.0)/Scalar(k), tmp.get());
    }

  }

  template<class Scalar>
  typename ExplicitTaylorPolynomialStepper<Scalar>::ScalarMag
  ExplicitTaylorPolynomialStepper<Scalar>::estimateLogRadius_()
  {
    ScalarMag rho = 0;
    ScalarMag tmp;
    for (unsigned int k=degree_/2; k<=degree_; k++) {
      tmp = log_norm_inf(*(x_poly_->getCoefficient(k))) / k;
      if (tmp > rho) {
        rho = tmp;
      }
    }
    return rho;
  }

  template<class Scalar>
  RCP<const Thyra::VectorSpaceBase<Scalar> > ExplicitTaylorPolynomialStepper<Scalar>::get_x_space() const
  {
    return(x_vector_->space());
  }

} // namespace Rythmos

#endif // RYTHMOS_EXPLICIT_TAYLOR_POLYNOMIAL_STEPPER_H
