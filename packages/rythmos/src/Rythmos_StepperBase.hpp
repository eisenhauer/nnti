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

#ifndef Rythmos_STEPPER_BASE_H
#define Rythmos_STEPPER_BASE_H


#include "Rythmos_InterpolationBufferBase.hpp"
#include "Rythmos_StepperSupportTypes.hpp"
#include "Thyra_VectorBase.hpp"
#include "Teuchos_Describable.hpp"
#include "Teuchos_RefCountPtr.hpp"
#include "Thyra_ModelEvaluator.hpp"


namespace Rythmos {


/** \brief Base class for defining stepper functionality.
 *
 * A stepper object is only defined to step forward in time, never backward in
 * time.  Therefore a negative step length in the context of this interface is
 * an invalid step length.
 *
 * ToDo: Finish documentation!
 *
 * 2007/05/17: rabartl: ToDo: Consider implementing a <tt>undoStep()</tt>
 * function that would erase the timestep that was just taken.  This type of
 * functionality would be needed for many different types of composed
 * algorithms like operator split methods and for the staggered correct
 * forward sensitivity stepper implementation with error control on the
 * sensitivity variables.
 */
template<class Scalar> 
class StepperBase : virtual public InterpolationBufferBase<Scalar>
{
public:

  /** \brief Return if this stepper supports cloning or not.
   *
   * If <tt>returnVal==true</tt>, then <tt>cloneStepper()</tt> will clone
   * <tt>*this</tt> object and return an non-null RCP.
   *
   * The default implementation of this function simply returns false.
   */
  virtual bool supportsCloning() const;

  /** \brief Clone the stepper object if supported.
   *
   * <b>Postconditions:</b><ul>
   * <li>[<tt>supportsCloning()==true</tt>] <tt>returnVal != Teuchos::null</tt>
   * <li>[<tt>supportsCloning()==false</tt>] <tt>returnVal == Teuchos::null</tt>
   * </ul>
   *
   * Cloning a stepper in this case does not imply that the full state will be
   * copied, shallow or deep.  Instead, here cloning means to just clone the
   * stepper algorithm and it will do a showllow copy of the model if a model
   * is set.  Since the model is stateless, this should be okay.  Therefore,
   * do not assume that the state of <tt>*returnVal</tt> is exactly the same
   * as the state of <tt>*this</tt>.  You have been warned!
   *
   * The default implementation returns <tt>Teuchos::null</tt> which is
   * consistent with the default implementation of
   * <tt>supportsCloning()==false</tt>.  If this function is overridden in a
   * derived class to support cloning, then <tt>supportsCloning()</tt> must be
   * overridden to return <tt>true</tt>.
   */
  virtual Teuchos::RefCountPtr<StepperBase<Scalar> > cloneStepper() const;

  /** \brief Return if this stepper is an implicit stepper.
   *
   * The default implemntation returns <tt>false</tt> and therefore, by
   * default, a stepper is considered to be an excplicit stepper.
   */
  virtual bool isImplicit() const;

  /** \brief Return if this stepper accepts a model.
   *
   *  While it makes sense for most stepper objects to accept a compatible
   *  model, in some extreme cases it is not well defined what this model
   *  should be or how it relates to what is presented in the interpolation
   *  buffer interface from which this interface inherits from.
   *
   * The default implemntation returns true and therefore, by default, a
   * stepper must accept the model to be intergrated.
   */
  virtual bool acceptsModel() const;
    
  /** \brief Specify the model problem to integrate.
   *
   * By default, the intial condition will be taken from
   * <tt>model->getNomainValues()</tt>.  If this intial condition is not
   * complete or is not the desired initial condition, it can be set using
   * <tt>setInitialCondition()</tt>.
   *
   * <b>Preconditions:</b><ul>
   * <li><tt>acceptsModel()==true</tt>
   * <li><tt>!is_null(model)</tt>
   * <li><tt>model->createInArgs().supports(MEB::IN_ARG_t)==true</tt>
   * <li><tt>model->createInArgs().supports(MEB::IN_ARG_x)==true</tt>
   * <li><tt>model->createOutArgs().supports(MEB::OUT_ARG_f)==true</tt>
   * <li>[<tt>isImplicit()</tt>] <tt>model->createInArgs().supports(MEB::IN_ARG_x_dot)==true</tt>
   * <li>[<tt>isImplicit()</tt>] <tt>model->createInArgs().supports(MEB::IN_ARG_alpha)==true</tt>
   * <li>[<tt>isImplicit()</tt>] <tt>model->createInArgs().supports(MEB::IN_ARG_beta)==true</tt>
   * <li>[<tt>isImplicit()</tt>] <tt>model->createOutArgs().supports(MEB::OUT_ARG_W)==true</tt>
   * </ul>
   *
   * 2007/06/10: rabartl : ToDo: Create helper macros that will assert these
   * preconditions and call these macros in every stepper subclass that
   * implements these function.  We will have one for explicit steppers and
   * one for implicit steppers.
   *
   * <b>Postconditions:</b><ul>
   * <li><tt>this->getModel() == model</tt>
   * </ul>
   */
  virtual void setModel(
    const Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> > &model
    ) = 0;
    
  /** \brief Get the model.
   *
   * Every stepper is expected to return the model that represents problem
   * that it is integrating, even if <tt>acceptsModel()==false</tt>.  Exposing
   * this model is necessary in order to get at the spaces and create the
   * <tt>InArgs</tt> object needed to set the initial condition.
   */
  virtual Teuchos::RefCountPtr<const Thyra::ModelEvaluator<Scalar> >
  getModel() const = 0;

  /** \brief Specify initial condition and re-initialize.
   *
   * <b>Preconditions:</b><ul>
   * <li><tt>!is_null(this->getModel())</tt>
   * </ul>
   *
   * The default implementation throwns an exception.
   *
   * ToDo: Remove this default implementation and make every concrete subclass
   * implement this!  Every stepper should except an initial condition that is
   * separate from the model object.
   */
  virtual void setInitialCondition(
    const Thyra::ModelEvaluatorBase::InArgs<Scalar> &initialCondition
    );
  
  /** \brief Take a step.
   *
   * \param  dt
   *           [in] The size of the step to take.
   * \param  stepType
   *           [in] The type of step to take.
   *
   * <b>Preconditions:</b><ul>
   * <li><tt>dt > 0.0</tt> (i.e. only forward steps are allowed)
   * <li><tt>!is_null(this->getModel())</tt>
   * </ul>
   *
   * \returns If <tt>returnVal > 0.0</tt>, then a step of size
   * <tt>returnVal</tt> was taken.  If <tt>returnVal <= 0.0</tt>, then the
   * step could not be taken for some reason.  If <tt>returnVal <= 0.0</tt>,
   * then <tt>*this</tt> is guaranteed to be in the same state after the
   * function returns as it was before the function was called.  This allows a
   * client to try a different step size or make other adjustments.
   */
  virtual Scalar takeStep(Scalar dt, StepSizeType stepType) = 0;

  /** \brief Get current stepper status after a step has been taken.
   *
   * ToDo: Determine what this returns before a step has ever been taken.
   */
  virtual const StepStatus<Scalar> getStepStatus() const = 0;

};


// ///////////////////////////////
// Implementations


template<class Scalar>
bool StepperBase<Scalar>::isImplicit() const
{
  return false;
}


template<class Scalar>
bool StepperBase<Scalar>::acceptsModel() const
{
  return true;
}


template<class Scalar>
bool StepperBase<Scalar>::supportsCloning() const
{
  return false;
}


template<class Scalar>
Teuchos::RefCountPtr<StepperBase<Scalar> >
StepperBase<Scalar>::cloneStepper() const
{
  return Teuchos::null;
}


template<class Scalar>
void StepperBase<Scalar>::setInitialCondition(
  const Thyra::ModelEvaluatorBase::InArgs<Scalar> &initialCondition
  )
{
  TEST_FOR_EXCEPTION(
    true, std::logic_error,
    "Error, the function setIntialCondition(...) is not implemented\n"
    "in the class \"" << Teuchos::typeName(*this) << "\"!" );
  // ToDo: Remove this default implementation and make every concrete
  // subclass implement this!
}


} // namespace Rythmos

#endif //Rythmos_STEPPER_BASE_H
