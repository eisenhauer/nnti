/*@HEADER
// ***********************************************************************
// 
//       Ifpack: Object-Oriented Algebraic Preconditioner Package
//                 Copyright (2002) Sandia Corporation
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
//@HEADER
*/

#include "Thyra_IfpackPreconditionerFactory.hpp"
#include "Thyra_EpetraOperatorViewExtractorStd.hpp"
#include "Thyra_EpetraLinearOp.hpp"
#include "Ifpack_ValidParameters.h"
#include "Ifpack_Preconditioner.h"
#include "Ifpack.h"
#include "Epetra_RowMatrix.h"
#include "Teuchos_dyn_cast.hpp"

namespace Thyra {

// Constructors/initializers/accessors

IfpackPreconditionerFactory::IfpackPreconditionerFactory()
  :epetraFwdOpViewExtractor_(Teuchos::rcp(new EpetraOperatorViewExtractorStd()))
   ,precType_(Ifpack::ILU)
   ,overlap_(0)
{}

// Overridden from PreconditionerFactoryBase

bool IfpackPreconditionerFactory::isCompatible( const LinearOpBase<double> &fwdOp ) const
{
  Teuchos::RefCountPtr<const Epetra_Operator> epetraFwdOp;
  ETransp                                     epetraFwdOpTransp;
  EApplyEpetraOpAs                            epetraFwdOpApplyAs;
  EAdjointEpetraOp                            epetraFwdOpAdjointSupport;
  double                                      epetraFwdOpScalar;
  epetraFwdOpViewExtractor_->getEpetraOpView(
    Teuchos::rcp(&fwdOp,false)
    ,&epetraFwdOp,&epetraFwdOpTransp,&epetraFwdOpApplyAs,&epetraFwdOpAdjointSupport,&epetraFwdOpScalar
    );
  if( !dynamic_cast<const Epetra_RowMatrix*>(&*epetraFwdOp) )
    return false;
  return true;
}

bool IfpackPreconditionerFactory::applySupportsConj(EConj conj) const
{
  return true;
}

bool IfpackPreconditionerFactory::applyTransposeSupportsConj(EConj conj) const
{
  return false; // See comment below
}

Teuchos::RefCountPtr<LinearOpBase<double> >
IfpackPreconditionerFactory::createPrecOp() const
{
  return Teuchos::rcp(new Thyra::EpetraLinearOp);
}

void IfpackPreconditionerFactory::initializePrecOp(
  const Teuchos::RefCountPtr<const LinearOpBase<double> >    &fwdOp
  ,LinearOpBase<double>                                      *precOp
  ,const ESupportSolveUse                                    supportSolveUse
  ) const
{
  using Teuchos::dyn_cast;
  using Teuchos::RefCountPtr;
  using Teuchos::null;
  using Teuchos::rcp;
  using Teuchos::rcp_dynamic_cast;
  using Teuchos::rcp_const_cast;
  using Teuchos::set_extra_data;
  using Teuchos::get_optional_extra_data;
#ifdef _DEBUG
  TEST_FOR_EXCEPT(fwdOp.get()==NULL);
  TEST_FOR_EXCEPT(precOp==NULL);
#endif
  //
  // Unwrap and get the forward Epetra_Operator object
  //
  Teuchos::RefCountPtr<const Epetra_Operator> epetraFwdOp;
  ETransp                                     epetraFwdOpTransp;
  EApplyEpetraOpAs                            epetraFwdOpApplyAs;
  EAdjointEpetraOp                            epetraFwdOpAdjointSupport;
  double                                      epetraFwdOpScalar;
  epetraFwdOpViewExtractor_->getEpetraOpView(
    fwdOp,&epetraFwdOp,&epetraFwdOpTransp,&epetraFwdOpApplyAs,&epetraFwdOpAdjointSupport,&epetraFwdOpScalar
    );
  // Validate what we get is what we need
  RefCountPtr<const Epetra_RowMatrix>
    epetraFwdRowMat = rcp_dynamic_cast<const Epetra_RowMatrix>(epetraFwdOp,true);
  TEST_FOR_EXCEPTION(
    epetraFwdOpApplyAs != EPETRA_OP_APPLY_APPLY, std::logic_error
    ,"Error, incorrect apply mode for an Epetra_RowMatrix"
    );
  //
  // Get the EpetraLinearOp object that is used to implement precOp
  //
  EpetraLinearOp
    *epetra_precOp = &Teuchos::dyn_cast<EpetraLinearOp>(*precOp);
  //
  // Get the embedded Ifpack_Preconditioner object if it exists
  //
  Teuchos::RefCountPtr<Ifpack_Preconditioner>
    ifpack_precOp = rcp_dynamic_cast<Ifpack_Preconditioner>(
      epetra_precOp->epetra_op()
      ,true
      );
  //
  // Get the attached forward operator if it exists
  //
  if(ifpack_precOp.get()) {
    TEST_FOR_EXCEPT(true);// ToDo: Implement
  }
  //
  // Permform initialization if needed
  //
  const bool startingOver = (ifpack_precOp.get() == NULL);
  if(startingOver) {
    // Create the initial preconditioner
    ::Ifpack ifpackFcty; // Should be static!
    const std::string &precTypeName = toString(precType_);
    ifpack_precOp = rcp(
      ifpackFcty.Create(
        precTypeName
        ,const_cast<Epetra_RowMatrix*>(&*epetraFwdRowMat)
        ,overlap_
        )
      );
    // RAB: Above, I am just passing a string to Ifpack::Create(...) in order
    // get this code written.  However, in the future, it would be good to
    // copy the contents of what is in Ifpack::Create(...) into a local
    // function and then use switch(...) to create the initial
    // Ifpack_Preconditioner object.  This would result in better validation
    // and faster code.
    TEST_FOR_EXCEPTION(
      ifpack_precOp.get()==NULL, std::logic_error
      ,"Error, Ifpack::Create(precTypeName,...) returned NULL for precType = \""<<precTypeName<<"\"!"
      );
    // Set parameters if the list exists
    if(paramList_.get())
      TEST_FOR_EXCEPT(0!=ifpack_precOp->SetParameters(paramList_->sublist("Ifpack"))); // This will create new sublist if it does not exist!
    // Initailize the structure for the preconditioner
    TEST_FOR_EXCEPT(0!=ifpack_precOp->Initialize());
  }
  //
  // Attach the epetraFwdOp to the ifpack_precOp to guaranteed that it will not go away
  //
  set_extra_data(epetraFwdOp,"IFPF::epetraFwdOp",&ifpack_precOp,Teuchos::POST_DESTROY,false);
  //
  // Update the factorization
  //
  TEST_FOR_EXCEPT(0!=ifpack_precOp->Compute());
  //
  // Compute the conditioner number estimate if asked
  //

  // ToDo: Implement

  //
  // Attach fwdOp to the ifpack_precOp
  //
  set_extra_data(fwdOp,"IFPF::fwdOp",&ifpack_precOp,Teuchos::POST_DESTROY,false);
  //
  // Initialize the output EpetraLinearOp if this is the first time
  //
  if(startingOver) {
    epetra_precOp->initialize(
      ifpack_precOp
      ,epetraFwdOpTransp
      ,EPETRA_OP_APPLY_APPLY_INVERSE
      ,EPETRA_OP_ADJOINT_UNSUPPORTED
      );
    // RAB: Above, I mark that adjoints are not supported since at least the
    // "ILU" option does not support them (see Ifpack bug 1978).
  }
}

void IfpackPreconditionerFactory::uninitializePrecOp(
  LinearOpBase<double>                                *precOp
  ,Teuchos::RefCountPtr<const LinearOpBase<double> >  *fwdOp
  ,ESupportSolveUse                                   *supportSolveUse
  ) const
{
  TEST_FOR_EXCEPT(true);
}

// Overridden from ParameterListAcceptor

void IfpackPreconditionerFactory::setParameterList(Teuchos::RefCountPtr<Teuchos::ParameterList> const& paramList)
{
  TEST_FOR_EXCEPT(paramList.get()==NULL);
  paramList->validateParameters(*this->getValidParameters(),1);
  paramList_ = paramList;
  overlap_ = paramList_->get("Overlap",overlap_);
  std::ostringstream oss;
  oss << "(sub)list \""<<paramList->name()<<"\"parameter \"Prec Type\"";
  precType_ = Ifpack::precTypeNameToEnum(
    paramList_->get("Prec Type",toString(precType_))
    ,oss.str()
    );
}

Teuchos::RefCountPtr<Teuchos::ParameterList>
IfpackPreconditionerFactory::getParameterList()
{
  return paramList_;
}

Teuchos::RefCountPtr<Teuchos::ParameterList>
IfpackPreconditionerFactory::unsetParameterList()
{
  Teuchos::RefCountPtr<Teuchos::ParameterList> _paramList = paramList_;
  paramList_ = Teuchos::null;
  return _paramList;
}

Teuchos::RefCountPtr<const Teuchos::ParameterList>
IfpackPreconditionerFactory::getParameterList() const
{
  return paramList_;
}

Teuchos::RefCountPtr<const Teuchos::ParameterList>
IfpackPreconditionerFactory::getValidParameters() const
{
  return generateAndGetValidParameters();
}

// Public functions overridden from Teuchos::Describable

std::string IfpackPreconditionerFactory::description() const
{
  std::ostringstream oss;
  oss << "Thyra::IfpackPreconditionerFactory{";
  oss << "precType=\"" << toString(precType_) << "\"";
  oss << ",overlap=" << overlap_;
  oss << "}";
  return oss.str();
}

// private

Teuchos::RefCountPtr<const Teuchos::ParameterList>
IfpackPreconditionerFactory::generateAndGetValidParameters()
{
  static Teuchos::RefCountPtr<Teuchos::ParameterList> validParamList;
  if(validParamList.get()==NULL) {
    validParamList = Teuchos::rcp(new Teuchos::ParameterList("Thyra::IfpackPreconditionerFactory"));
    validParamList->set("Prec Type","ILU");
    validParamList->set("Overlap",0);
    validParamList->sublist("Ifpack").setParameters(Ifpack_GetValidParameters());
  }
  return validParamList;
}

} // namespace Thyra
