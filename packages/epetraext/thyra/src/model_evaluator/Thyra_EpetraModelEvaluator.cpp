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

#include "Thyra_EpetraModelEvaluator.hpp"
#include "Thyra_EpetraThyraWrappers.hpp"
#include "Thyra_EpetraLinearOp.hpp"

namespace Thyra {

// Constructors/initializers/accessors.

EpetraModelEvaluator::EpetraModelEvaluator()
{}

EpetraModelEvaluator::EpetraModelEvaluator(
  const Teuchos::RefCountPtr<const EpetraExt::ModelEvaluator>               &epetraModel
  ,const Teuchos::RefCountPtr<const LinearOpWithSolveFactoryBase<double> >  &W_factory
  )
{
  initialize(epetraModel,W_factory);
}

void EpetraModelEvaluator::initialize(
  const Teuchos::RefCountPtr<const EpetraExt::ModelEvaluator>               &epetraModel
  ,const Teuchos::RefCountPtr<const LinearOpWithSolveFactoryBase<double> >  &W_factory
  )
{
  epetraModel_ = epetraModel;
  //
  W_factory_ = W_factory;
  //
  x_space_ = create_MPIVectorSpaceBase( x_map_ = epetraModel_->get_x_map() );
  f_space_ = create_MPIVectorSpaceBase( f_map_ = epetraModel_->get_f_map() );
  //
  EpetraExt::ModelEvaluator::InArgs inArgs = epetraModel_->createInArgs();
  p_map_.resize(inArgs.Np()); p_space_.resize(inArgs.Np());
  for( int l = 1; l <= static_cast<int>(p_space_.size()); ++l )
    p_space_[l-1] = create_MPIVectorSpaceBase( p_map_[l-1] = epetraModel_->get_p_map(l) );
  //
  EpetraExt::ModelEvaluator::OutArgs outArgs = epetraModel_->createOutArgs();
  g_map_.resize(outArgs.Ng()); g_space_.resize(outArgs.Ng());
  for( int j = 1; j <= static_cast<int>(g_space_.size()); ++j )
    g_space_[j-1] = create_MPIVectorSpaceBase( g_map_[j-1] = epetraModel_->get_g_map(j) );
}

Teuchos::RefCountPtr<const EpetraExt::ModelEvaluator> EpetraModelEvaluator::getEpetraModel() const
{
  return epetraModel_;
}

void EpetraModelEvaluator::uninitialize(
  Teuchos::RefCountPtr<const EpetraExt::ModelEvaluator>               *epetraModel
  ,Teuchos::RefCountPtr<const LinearOpWithSolveFactoryBase<double> >  *W_factory
  )
{
  if(epetraModel) *epetraModel = epetraModel_;
  if(W_factory) *W_factory = W_factory_;
  epetraModel_ = Teuchos::null;
  W_factory_ = Teuchos::null;
}

// Overridden from ModelEvaulator.

int EpetraModelEvaluator::Np() const
{
  return p_space_.size();
}

int EpetraModelEvaluator::Ng() const
{
  return g_space_.size();
}

Teuchos::RefCountPtr<const VectorSpaceBase<double> >
EpetraModelEvaluator::get_x_space() const
{
  return x_space_;
}

Teuchos::RefCountPtr<const VectorSpaceBase<double> >
EpetraModelEvaluator::get_f_space() const
{
  return f_space_;
}

Teuchos::RefCountPtr<const VectorSpaceBase<double> >
EpetraModelEvaluator::get_p_space(int l) const
{
  TEST_FOR_EXCEPT( ! ( 1 <= l && l <= this->Np() ) );
  return p_space_[l-1];
}

Teuchos::RefCountPtr<const VectorSpaceBase<double> >
EpetraModelEvaluator::get_g_space(int j) const
{
  TEST_FOR_EXCEPT( ! ( 1 <= j && j <= this->Ng() ) );
  return g_space_[j-1];
}

Teuchos::RefCountPtr<const VectorBase<double> >
EpetraModelEvaluator::get_x_init() const
{
  return create_MPIVectorBase( epetraModel_->get_x_init(), x_space_ );
}

Teuchos::RefCountPtr<const VectorBase<double> >
EpetraModelEvaluator::get_p_init(int l) const
{
  TEST_FOR_EXCEPT( ! ( 1 <= l && l <= this->Np() ) );
  return create_MPIVectorBase( epetraModel_->get_p_init(l), p_space_[l-1] );
}

double EpetraModelEvaluator::get_t_init() const
{
  return epetraModel_->get_t_init();
}

Teuchos::RefCountPtr<const VectorBase<double> >
EpetraModelEvaluator::get_x_lower_bounds() const
{
  return create_MPIVectorBase( epetraModel_->get_x_lower_bounds(), x_space_ );
}

Teuchos::RefCountPtr<const VectorBase<double> >
EpetraModelEvaluator::get_x_upper_bounds() const
{
  return create_MPIVectorBase( epetraModel_->get_x_upper_bounds(), x_space_ );
}

Teuchos::RefCountPtr<const VectorBase<double> >
EpetraModelEvaluator::get_p_lower_bounds(int l) const
{
  TEST_FOR_EXCEPT( ! ( 1 <= l && l <= this->Np() ) );
  return create_MPIVectorBase( epetraModel_->get_p_lower_bounds(l), p_space_[l-1] );
}

Teuchos::RefCountPtr<const VectorBase<double> >
EpetraModelEvaluator::get_p_upper_bounds(int l) const
{
  TEST_FOR_EXCEPT( ! ( 1 <= l && l <= this->Np() ) );
  return create_MPIVectorBase( epetraModel_->get_p_upper_bounds(l), p_space_[l-1] );
}

double EpetraModelEvaluator::get_t_lower_bound() const
{
  return epetraModel_->get_t_lower_bound();
}

double EpetraModelEvaluator::get_t_upper_bound() const
{
  return epetraModel_->get_t_upper_bound();
}

Teuchos::RefCountPtr<LinearOpWithSolveBase<double> >
EpetraModelEvaluator::create_W() const
{
  TEST_FOR_EXCEPTION(
    W_factory_.get()==NULL, std::logic_error
    ,"Thyra::EpetraModelEvaluator::create_W(): Error, the client did not set a LinearOpWithSolveFactoryBase"
    " object for W!"
    );
  return W_factory_->createOp();
}

EpetraModelEvaluator::InArgs<double> EpetraModelEvaluator::createInArgs() const
{
  const EpetraExt::ModelEvaluator &epetraModel = *epetraModel_;
  InArgsSetup<double> inArgs;
  typedef EpetraExt::ModelEvaluator EME;
  EME::InArgs epetraInArgs = epetraModel.createInArgs();
  inArgs.setModelEvalDescription(this->description());
  inArgs.set_Np(epetraInArgs.Np());
  inArgs.setSupports(IN_ARG_x_dot,epetraInArgs.supports(EME::IN_ARG_x_dot));
  inArgs.setSupports(IN_ARG_x,epetraInArgs.supports(EME::IN_ARG_x));
  inArgs.setSupports(IN_ARG_t,epetraInArgs.supports(EME::IN_ARG_t));
  inArgs.setSupports(IN_ARG_alpha,epetraInArgs.supports(EME::IN_ARG_alpha));
  inArgs.setSupports(IN_ARG_beta,epetraInArgs.supports(EME::IN_ARG_beta));
  return inArgs;
}

EpetraModelEvaluator::OutArgs<double> EpetraModelEvaluator::createOutArgs() const
{
  const EpetraExt::ModelEvaluator &epetraModel = *epetraModel_;
  OutArgsSetup<double> outArgs;
  typedef EpetraExt::ModelEvaluator EME;
  EME::OutArgs epetraOutArgs = epetraModel.createOutArgs();
  outArgs.setModelEvalDescription(this->description());
  outArgs.set_Ng(epetraOutArgs.Ng());
  outArgs.setSupports(OUT_ARG_f,epetraOutArgs.supports(EME::OUT_ARG_f));
  outArgs.setSupports(OUT_ARG_W,epetraOutArgs.supports(EME::OUT_ARG_W));
  outArgs.set_W_properties(convert(epetraOutArgs.get_W_properties()));
  return outArgs;
}

void EpetraModelEvaluator::evalModel( const InArgs<double>& inArgs, const OutArgs<double>& outArgs ) const
{

  using Thyra::get_Epetra_Vector;
  using Teuchos::RefCountPtr;

  typedef EpetraExt::ModelEvaluator EME;

  // InArgs

  EME::InArgs epetraInArgs = epetraModel_->createInArgs();

  RefCountPtr<const VectorBase<double> > x_dot;
  if( inArgs.supports(IN_ARG_x_dot) && (x_dot = inArgs.get_x_dot()).get() )
    epetraInArgs.set_x_dot(get_Epetra_Vector(*x_map_,x_dot));

  RefCountPtr<const VectorBase<double> > x;
  if( inArgs.supports(IN_ARG_x) && (x = inArgs.get_x()).get() )
    epetraInArgs.set_x(get_Epetra_Vector(*x_map_,x));

  if( inArgs.supports(IN_ARG_t) )
    epetraInArgs.set_t(inArgs.get_t());

  if( inArgs.supports(IN_ARG_alpha) )
    epetraInArgs.set_alpha(inArgs.get_alpha());

  if( inArgs.supports(IN_ARG_beta) )
    epetraInArgs.set_beta(inArgs.get_beta());

  // OutArgs

  EME::OutArgs epetraOutArgs = epetraModel_->createOutArgs();

  RefCountPtr<VectorBase<double> > f;
  if( outArgs.supports(OUT_ARG_f) && (f = outArgs.get_f()).get() )
    epetraOutArgs.set_f(get_Epetra_Vector(*f_map_,f));

  RefCountPtr<LinearOpWithSolveBase<double> > W;
  RefCountPtr<const LinearOpBase<double> >    fwdW;
  Teuchos::RefCountPtr<Epetra_Operator>       eW;
  if( outArgs.supports(OUT_ARG_W) && (W = outArgs.get_W()).get() ) {
    W_factory_->uninitializeOp(&*W,&fwdW);
    if( fwdW.get() ) {
      eW = const_cast<EpetraLinearOp&>(Teuchos::dyn_cast<const EpetraLinearOp>(*fwdW)).epetra_op();
    }
    else {
      eW = epetraModel_->create_W();
    }
    epetraOutArgs.set_W(eW);
  }

  // Do the evaluation

  epetraModel_->evalModel(epetraInArgs,epetraOutArgs);

  // Postprocess arguments

  if( W.get() ) {
    if( !fwdW.get() ) {
      fwdW = Teuchos::rcp(new EpetraLinearOp(eW));
    }
    W_factory_->initializeOp(fwdW,&*W);
  }

}

// Public functions overridden from Teuchos::Describable

std::string EpetraModelEvaluator::description() const
{
  std::ostringstream oss;
  oss << "EpetraExt::EpetraModelEvaluator{epetraModel=";
  if(epetraModel_.get())
    oss << "\'"<<epetraModel_->description()<<"\'";
  else
    oss << "NULL";
  oss << "}";
  return oss.str();
}

} // namespace Thyra


//
// Non-member utility functions
//

Thyra::ModelEvaluatorBase::DerivativeProperties
Thyra::convert( const EpetraExt::ModelEvaluator::DerivativeProperties &derivativeProperties )
{
  ModelEvaluatorBase::EDerivativeLinearity linearity;
  switch(derivativeProperties.linearity) {
    case EpetraExt::ModelEvaluator::DERIV_LINEARITY_UNKNOWN:
      linearity = ModelEvaluatorBase::DERIV_LINEARITY_UNKNOWN;
      break;
    case EpetraExt::ModelEvaluator::DERIV_LINEARITY_CONST:
      linearity = ModelEvaluatorBase::DERIV_LINEARITY_CONST;
      break;
    case  EpetraExt::ModelEvaluator::DERIV_LINEARITY_NONCONST:
      linearity = ModelEvaluatorBase::DERIV_LINEARITY_NONCONST;
      break;
    default:
      TEST_FOR_EXCEPT(true);
  }
  ModelEvaluatorBase::ERankStatus rank;
  switch(derivativeProperties.rank) {
    case EpetraExt::ModelEvaluator::DERIV_RANK_UNKNOWN:
      rank = ModelEvaluatorBase::DERIV_RANK_UNKNOWN;
      break;
    case EpetraExt::ModelEvaluator::DERIV_RANK_FULL:
      rank = ModelEvaluatorBase::DERIV_RANK_FULL;
      break;
    case EpetraExt::ModelEvaluator::DERIV_RANK_DEFICIENT:
      rank = ModelEvaluatorBase::DERIV_RANK_DEFICIENT;
      break;
    default:
      TEST_FOR_EXCEPT(true);
  }
  return ModelEvaluatorBase::DerivativeProperties(linearity,rank,derivativeProperties.supportsAdjoint);
}
