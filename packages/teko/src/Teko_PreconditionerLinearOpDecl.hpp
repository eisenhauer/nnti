/*
// @HEADER
// 
// ***********************************************************************
// 
//      Teko: A package for block and physics based preconditioning
//                  Copyright 2010 Sandia Corporation 
//  
// Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// Export of this program may require a license from the United States
// Government.
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
// Questions? Contact Eric C. Cyr (eccyr@sandia.gov)
// 
// ***********************************************************************
// 
// @HEADER

*/

#ifndef __Teko_PreconditionerLinearOpDecl_hpp__
#define __Teko_PreconditionerLinearOpDecl_hpp__

#include "Thyra_LinearOpBase.hpp"
#include "Thyra_PreconditionerBase.hpp"
#include "Thyra_VectorSpaceBase.hpp"

#include "Teuchos_ConstNonconstObjectContainer.hpp"

namespace Teko {

/** \brief Class that wraps a <code>PreconditionerBase</code> object it makes it behave
  *        like a linear operator.
  *        
  * Class that wraps a <code>PreconditionerBase</code> object it makes it behave
  * like a linear operator.
  */
template <typename ScalarT>
class PreconditionerLinearOp : public Thyra::LinearOpBase<ScalarT> {
public:
   PreconditionerLinearOp();
   PreconditionerLinearOp(const Teuchos::RCP<Thyra::PreconditionerBase<ScalarT> > & prec);
   PreconditionerLinearOp(const Teuchos::RCP<const Thyra::PreconditionerBase<ScalarT> > & prec);

   //! build a linear operator using this preconditioner, this initialization permits changes
   void initialize(const Teuchos::RCP<Thyra::PreconditionerBase<ScalarT> > & prec);

   //! build a linear operator using this preconditioner, this initialization refuses changes
   void initialize(const Teuchos::RCP<const Thyra::PreconditionerBase<ScalarT> > & prec);

   //! Disassociate this object with the currently owned preconditioner
   void uninitialize();

   /** @brief Range space of this operator */
   virtual Teuchos::RCP<const Thyra::VectorSpaceBase<ScalarT> > range() const;

   /** @brief Domain space of this operator */
   virtual Teuchos::RCP<const Thyra::VectorSpaceBase<ScalarT> > domain() const;

   virtual bool opSupportedImpl(const Thyra::EOpTransp M_trans) const;

   //! @brief Apply operation
   virtual void applyImpl(
     const Thyra::EOpTransp M_trans,
     const Thyra::MultiVectorBase<ScalarT> & x,
     const Teuchos::Ptr<Thyra::MultiVectorBase<ScalarT> > & y,
     const ScalarT alpha,
     const ScalarT beta
     ) const;

   //! Get a nonconstant <code>PreconditionerBase</code> object
   virtual Teuchos::RCP<Thyra::PreconditionerBase<ScalarT> > getNonconstPreconditioner();

   //! Get a constant <code>PreconditionerBase</code> object
   virtual Teuchos::RCP<const Thyra::PreconditionerBase<ScalarT> > getPreconditioner() const;

   //! Get teko linear operator
   Teko::LinearOp getOperator() const { return getOperator_cnoc().getConstObj(); }

   // Inherited from Teuchos::Describable
   void describe(Teuchos::FancyOStream & out_arg,const Teuchos::EVerbosityLevel verbLevel) const;
protected:
   //! get operator associated with the preconditioner
   Teuchos::ConstNonconstObjectContainer<Thyra::LinearOpBase<ScalarT> > getOperator_cnoc() const;

   //! get operator associated with the preconditioner
   Teuchos::ConstNonconstObjectContainer<Thyra::LinearOpBase<ScalarT> > getOperator_cnoc();

   Teuchos::ConstNonconstObjectContainer<Thyra::PreconditionerBase<ScalarT> > preconditioner_;
};

} // end namespace Teko

#endif
