//@HEADER
// ***********************************************************************
//
//                     Rythmos Package
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

#ifndef Rythmos_INTERPOLATION_BUFFER_AS_STEPPER_H
#define Rythmos_INTERPOLATION_BUFFER_AS_STEPPER_H

#include "Rythmos_InterpolationBufferBase.hpp"
#include "Rythmos_Stepper.hpp"

namespace Rythmos {

/** \brief Base class for defining interpolation buffer functionality. */
template<class Scalar> 
class InterpolationBufferAsStepper : virtual public Rythmos::InterpolationBufferBase<Scalar>
{
  public:

    typedef typename Teuchos::ScalarTraits<Scalar>::magnitudeType ScalarMag;

    /// Destructor
    ~InterpolationBufferAsStepper() {};

    /// Constructors
    InterpolationBufferAsStepper();
    InterpolationBufferAsStepper(
      const Teuchos::RefCountPtr<Rythmos::Stepper<Scalar> > &stepper_
      ,const Teuchos::RefCountPtr<Rythmos::InterpolationBufferBase<Scalar> > &IB_
      ,const Teuchos::RefCountPtr<Teuchos::ParameterList> &parameterList_ = Teuchos::null
      );

    /// Set InterpolationBufferBase:
    void setInterpolationBuffer(const Teuchos::RefCountPtr<Rythmos::InterpolationBufferBase<Scalar> > &IB_);

    /// Set Stepper:
    void setStepper(const Teuchos::RefCountPtr<Rythmos::Stepper<Scalar> > &stepper_);
    
    /// Set ParameterList:
    void setParameterList(const Teuchos::RefCountPtr<Teuchos::ParameterList> &parameterList_);

    /// Redefined from InterpolationBufferBase
    /// This is a pass-through to the underlying InterpolationBufferBase:
    bool SetPoints(
      const std::vector<Scalar>& time_vec
      ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& x_vec
      ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& xdot_vec
      ,const std::vector<ScalarMag> & accuracy_vec 
      );

    // This is not a pass-through.
    bool GetPoints(
      const std::vector<Scalar>& time_vec_
      ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* x_vec_
      ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* xdot_vec_
      ,std::vector<ScalarMag>* accuracy_vec_) const;

    /// This is a pass-through to the underlying InterpolationBufferBase:
    bool SetRange(
      const Scalar& time_lower
      ,const Scalar& time_upper
      ,const InterpolationBufferBase<Scalar>& IB_);

    /// This is a pass-through to the underlying InterpolationBufferBase:
    bool GetNodes(std::vector<Scalar>* time_vec) const;

    /// This is a pass-through to the underlying InterpolationBufferBase:
    virtual bool RemoveNodes(std::vector<Scalar>& time_vec);

    /// This is a pass-through to the underlying InterpolationBufferBase:
    int GetOrder() const;

    /// Redefined from Teuchos::Describable
    /** \brief . */
    std::string description() const;

    /** \brief . */
    void describe(
      Teuchos::FancyOStream       &out
      ,const Teuchos::EVerbosityLevel      verbLevel
      ) const;


  private:

    // Interpolation Buffer used to store past results
    Teuchos::RefCountPtr<Rythmos::InterpolationBufferBase<Scalar> > IB;

    // Stepper used to fill interpolation buffer.
    Teuchos::RefCountPtr<Rythmos::Stepper<Scalar> > stepper;

    // ParameterList to control behavior
    Teuchos::RefCountPtr<Teuchos::ParameterList> parameterList;

#ifdef Rythmos_DEBUG
    int debugLevel;
    Teuchos::RefCountPtr<Teuchos::FancyOStream> debug_out;
#endif // Rythmos_DEBUG

};

// ////////////////////////////
// Defintions
template<class Scalar>
InterpolationBufferAsStepper<Scalar>::InterpolationBufferAsStepper(
    const Teuchos::RefCountPtr<Rythmos::Stepper<Scalar> > &stepper_
    ,const Teuchos::RefCountPtr<Rythmos::InterpolationBufferBase<Scalar> > &IB_
    ,const Teuchos::RefCountPtr<Teuchos::ParameterList> &parameterList_ 
    )
{
#ifdef Rythmos_DEBUG
  debugLevel = 2;
  debug_out = Teuchos::VerboseObjectBase::getDefaultOStream();
  debug_out->precision(15);
  debug_out->setMaxLenLinePrefix(40);
  debug_out->pushLinePrefix("Rythmos::InterpolationBufferAsStepper");
  debug_out->setShowLinePrefix(true);
  debug_out->setTabIndentStr("    ");
  *debug_out << "Initializing InterpolationBuferAsStepper" << std::endl;
  if (debugLevel > 1)
    *debug_out << "Calling setStepper..." << std::endl;
#endif // Rythmos_DEBUG
  setStepper(stepper_);
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
    *debug_out << "Calling setInterpolationBuffer..." << std::endl;
#endif // Rythmos_DEBUG
  setInterpolationBuffer(IB_);
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
    *debug_out << "Calling setParameterList..." << std::endl;
#endif // Rythmos_DEBUG
  setParameterList(parameterList_);
}

template<class Scalar>
void InterpolationBufferAsStepper<Scalar>::setStepper(
    const Teuchos::RefCountPtr<Rythmos::Stepper<Scalar> > &stepper_
    )
{
  // 10/9/06 tscoffe:  What should we do if this is called after initialization?
  //                   Basically, you're swapping out the stepper for a new one.
  //                   Since we're copying the data into IB after each
  //                   stepper->TakeStep() call, this should be fine, and it
  //                   will essentially result in changing the stepper
  //                   mid-stream.  If the new stepper has a time value before
  //                   the end of the data in IB, then you will not get new
  //                   stepper data until you ask for time values after the end
  //                   of IB's data.  And then the stepper will walk forward
  //                   inserting new (potentially inconsistent) data into IB
  //                   until it can give you the time values you asked for.
  //                   Then IB will potentially have old and new data in it.
  //                   On the other hand, if you swap out the stepper and the
  //                   time value is synchronized with the old stepper, then
  //                   you will essentially change the integrator mid-stream
  //                   and everything should proceed without problems.
  //                   Note also:  this functionality is important for checkpointing.
  stepper = stepper_;
#ifdef Rythmos_DEBUG
  Teuchos::OSTab ostab(debug_out,1,"setStepper");
  if (debugLevel > 1)
  {
    *debug_out << "stepper = " << stepper->description() << std::endl;
  }
#endif // Rythmos_DEBUG
}

template<class Scalar>
void InterpolationBufferAsStepper<Scalar>::setInterpolationBuffer(
    const Teuchos::RefCountPtr<Rythmos::InterpolationBufferBase<Scalar> > &IB_
    )
{
  // 10/9/06 tscoffe:  What should we do if this is called after initialization?
  //                   Basically, you're swapping out the history for a new
  //                   one.  This could be the result of upgrading or
  //                   downgrading the accuracy of the buffer, or something I
  //                   haven't thought of yet.  Since IB's node_vec is checked
  //                   each time GetPoints is called, this should be fine.  And
  //                   the time values in IB need not be synchronized with
  //                   stepper.
  //                   Note also:  this functionality is important for checkpointing.
  IB = IB_;
#ifdef Rythmos_DEBUG
  Teuchos::OSTab ostab(debug_out,1,"setInterpolationBuffer");
  if (debugLevel > 1)
  {
    *debug_out << "IB = " << IB->description() << std::endl;
  }
#endif // Rythmos_DEBUG
}

template<class Scalar>
void InterpolationBufferAsStepper<Scalar>::setParameterList( 
    const Teuchos::RefCountPtr<Teuchos::ParameterList> &parameterList_
    )
{
  if (parameterList_ == Teuchos::null)
    parameterList = Teuchos::rcp(new Teuchos::ParameterList);
  else
    parameterList = parameterList_;
#ifdef Rythmos_DEBUG
  Teuchos::OSTab ostab(debug_out,1,"setParameterList");
  if (debugLevel > 1)
  {
    *debug_out << "parameterList = " << parameterList->print(*debug_out) << std::endl;
  }
#endif // Rythmos_DEBUG
}


template<class Scalar>
bool InterpolationBufferAsStepper<Scalar>::SetPoints(
      const std::vector<Scalar>& time_vec
      ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& x_vec
      ,const std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >& xdot_vec
      ,const std::vector<ScalarMag> & accuracy_vec 
      ) 
{
  return(IB->SetPoints(time_vec,x_vec,xdot_vec,accuracy_vec));
}

template<class Scalar>
bool InterpolationBufferAsStepper<Scalar>::GetPoints(
      const std::vector<Scalar>& time_vec_
      ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* x_vec_ptr_
      ,std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > >* xdot_vec_ptr_
      ,std::vector<ScalarMag>* accuracy_vec_ptr_
      ) const
{
#ifdef Rythmos_DEBUG
  Teuchos::OSTab ostab(debug_out,1,"GetPoints");
  if (debugLevel > 1)
  {
    *debug_out << "time_vec_ = " << std::endl;
    for (int i=0 ; i<time_vec_.size() ; ++i)
      *debug_out << "time_vec_[" << i << "] = " << time_vec_[i] << std::endl;
    if (x_vec_ptr_ == NULL)
      *debug_out << "x_vec_ptr_ = NULL" << std::endl;
    else if (x_vec_ptr_->size() == 0)
      *debug_out << "x_vec_ptr_ = ptr to empty vector" << std::endl;
    else
    {
      *debug_out << "x_vec_ptr_ = " << std::endl;
      for (int i=0 ; i<x_vec_ptr_->size() ; ++i)
      {
        *debug_out << "x_vec[" << i << "] = " << std::endl;
        (*x_vec_ptr_)[i]->describe(*debug_out,Teuchos::VERB_EXTREME);
      }
    }
    if (xdot_vec_ptr_ == NULL)
      *debug_out << "xdot_vec_ptr_ = NULL" << std::endl;
    else if (xdot_vec_ptr_->size() == 0)
      *debug_out << "xdot_vec_ptr_ = ptr to empty vector" << std::endl;
    else
    {
      *debug_out << "xdot_vec = " << std::endl;
      for (int i=0 ; i<xdot_vec_ptr_->size() ; ++i)
      {
        *debug_out << "xdot_vec[" << i << "] = " << std::endl;
        (*xdot_vec_ptr_)[i]->describe(*debug_out,Teuchos::VERB_EXTREME);
      }
    }
    if (accuracy_vec_ptr_ == NULL)
      *debug_out << "accuracy_vec_ptr_ = NULL" << std::endl;
    else if (accuracy_vec_ptr_->size() == 0)
      *debug_out << "accuracy_vec_ptr_ = ptr to empty vector" << std::endl;
    else
    {
      *debug_out << "accuracy_vec = " << std::endl;
      for (int i=0 ; i<accuracy_vec_ptr_->size() ; ++i)
        *debug_out << "accuracy_vec[" << i << "] = " << (*accuracy_vec_ptr_)[i] << std::endl;
    }
  }
#endif // Rythmos_DEBUG
  bool status = IB->GetPoints(time_vec_,x_vec_ptr_,xdot_vec_ptr_,accuracy_vec_ptr_);
  if (status) return(status);
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
    *debug_out << "IB->GetPoints unsuccessful" << std::endl;
#endif // Rythmos_DEBUG
  status = true;
  std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > > &x_vec = *x_vec_ptr_;
  std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > > &xdot_vec = *xdot_vec_ptr_;
  std::vector<ScalarMag> &accuracy_vec = *accuracy_vec_ptr_;
  // Sort time_vec_
  std::vector<Scalar> local_time_vec = time_vec_;
  std::sort(local_time_vec.begin(),local_time_vec.end());
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
  {
    *debug_out << "sorted local time_vec:" << std::endl;
    for (int i=0 ; i<local_time_vec.size() ; ++i)
      *debug_out << "local_time_vec[" << i << "] = " << local_time_vec[i] << std::endl;
  }
#endif // Rythmos_DEBUG
  // Get nodes out of IB:
  std::vector<Scalar> node_vec; 
  status = IB->GetNodes(&node_vec); 
  if (!status) return(status);
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
  {
    if (node_vec.size() == 0)
      *debug_out << "IB->GetNodes: node_vec = empty vector" << std::endl;
    else
    {
      *debug_out << "IB->GetNodes:" << std::endl;
      for (int i=0 ; i<node_vec.size() ; ++i)
        *debug_out << "node_vec[" << i << "] = " << node_vec[i] << std::endl;
    }
  }
#endif // Rythmos_DEBUG
  Scalar node_begin = *(node_vec.begin());
  Scalar node_end = *(node_vec.end());
  // Check for valid input of local_time_vec:  (check initialization conditions)
  if ((*(local_time_vec.end()) < node_vec[0]) || (*(local_time_vec.begin()) < node_vec[0]))
    return(false);
#ifdef Rythmos_DEBUG
  if (debugLevel > 1)
    *debug_out << "Requested times are valid." << std::endl;
#endif // Rythmos_DEBUG
  // Get time out of stepper:
  std::vector<Scalar> stepper_vec;
  stepper->GetNodes(&stepper_vec);
  Scalar stepper_begin = *(stepper_vec.begin());
  Scalar stepper_end = *(stepper_vec.end());
  int num = local_time_vec.size();
  for (int i=0; i<num ; ++i)
  {
    if ( ( node_begin < local_time_vec[i] ) && ( local_time_vec[i] < node_end ) )
    {
      std::vector<Scalar> tmp_time_vec; 
      tmp_time_vec.push_back(local_time_vec[i]);
      std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > > tmp_x_vec, tmp_xdot_vec;
      std::vector<ScalarMag> tmp_accuracy_vec;
      status = IB->GetPoints(tmp_time_vec, &tmp_x_vec, &tmp_xdot_vec, &tmp_accuracy_vec); 
      x_vec[i] = tmp_x_vec[0];
      xdot_vec[i] = tmp_xdot_vec[0];
      accuracy_vec[i] = tmp_accuracy_vec[0];
      if (!status) return(status);
    }
    else if ( ( stepper_begin < local_time_vec[i] ) && ( local_time_vec[i] < stepper_end ) )
    {
      std::vector<Scalar> tmp_time_vec; 
      tmp_time_vec.push_back(local_time_vec[i]);
      std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > > tmp_x_vec, tmp_xdot_vec;
      std::vector<ScalarMag> tmp_accuracy_vec;
      status = stepper->GetPoints(tmp_time_vec, &tmp_x_vec, &tmp_xdot_vec, &tmp_accuracy_vec); 
      x_vec[i] = tmp_x_vec[0];
      xdot_vec[i] = tmp_xdot_vec[0];
      accuracy_vec[i] = tmp_accuracy_vec[0];
      if (!status) return(status);
    }
    else
    {
      while (stepper_end < local_time_vec[i])
      {
        // integrate forward with stepper 
        Scalar step_taken;
        if (parameterList->isParameter("fixed_dt"))
          step_taken = stepper->TakeStep(parameterList->get<Scalar>("fixed_dt"));
        else
          step_taken = stepper->TakeStep();
        // Pass information from stepper to IB:
        status = IB->SetRange(stepper_end,stepper_end+step_taken,*stepper);
        if (!status) return(status);
        // Check to see if we're past the current requested local_time_vec[i] point:
        if (local_time_vec[i] <= stepper_end+step_taken)
        {
          std::vector<Scalar> tmp_time_vec;
          tmp_time_vec.push_back(local_time_vec[i]);
          std::vector<Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > > tmp_x_vec, tmp_xdot_vec;
          std::vector<ScalarMag> tmp_accuracy_vec;
          status = stepper->GetPoints(tmp_time_vec, &tmp_x_vec, &tmp_xdot_vec, &tmp_accuracy_vec); 
          x_vec[i] = tmp_x_vec[0];
          xdot_vec[i] = tmp_xdot_vec[0];
          accuracy_vec[i] = tmp_accuracy_vec[0];
          if (!status) return(status);
        }
        // Update end-points:
        node_end += step_taken;
        stepper_begin = stepper_end;
        stepper_end += step_taken;
      }
    }
  }
  return(status);
}

template<class Scalar>
bool InterpolationBufferAsStepper<Scalar>::SetRange(
      const Scalar& time_lower
      ,const Scalar& time_upper
      ,const InterpolationBufferBase<Scalar> & IB_
      )
{
  return(IB->SetRange(time_lower,time_upper,IB_));
}

template<class Scalar>
bool InterpolationBufferAsStepper<Scalar>::GetNodes(
    std::vector<Scalar>* time_vec
    ) const
{
  return(IB->GetNodes(time_vec));
}

template<class Scalar>
bool InterpolationBufferAsStepper<Scalar>::RemoveNodes(
    std::vector<Scalar>& time_vec
    ) 
{
  return(IB->RemoveNodes(time_vec));
}

template<class Scalar>
int InterpolationBufferAsStepper<Scalar>::GetOrder() const
{
  return(IB->GetOrder());
}

template<class Scalar>
std::string InterpolationBufferAsStepper<Scalar>::description() const
{
  std::string name = "Rythmos::InterpolationBufferAsStepper";
  return(name);
}

template<class Scalar>
void InterpolationBufferAsStepper<Scalar>::describe(
      Teuchos::FancyOStream                &out
      ,const Teuchos::EVerbosityLevel      verbLevel
      ) const
{
  if (verbLevel == Teuchos::VERB_EXTREME)
  {
    out << description() << "::describe" << std::endl;
    out << "interpolation buffer = " << IB->description() << std::endl;
    out << "stepper = " << stepper->description() << std::endl;
    out << "parameterList = " << parameterList->print(out) << std::endl;
  }
}

} // namespace Rythmos

#endif //Rythmos_INTERPOLATION_BUFFER_AS_STEPPER_H
