// @HEADER
// ***********************************************************************
// 
//              Meros: Segregated Preconditioning Package
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

#ifndef MEROS_LSC_PRECONDITIONERFACTORY_H
#define MEROS_LSC_PRECONDITIONERFACTORY_H

/*! \file Meros_LSCPreconditionerFactory.h
 *  \brief Factory for building Least Squares Commutator block
 *         preconditioners.
 */

#include "Teuchos_ParameterListAcceptor.hpp"

#include "Thyra_SolveSupportTypes.hpp"
#include "Thyra_LinearOpSourceBase.hpp"
#include "Thyra_PreconditionerBase.hpp"
#include "Thyra_LinearOpWithSolveFactoryBase.hpp"
#include "Thyra_LinearOpWithSolveBase.hpp"
#include "Thyra_PreconditionerFactoryBase.hpp"

#include "Meros_LSCOperatorSource.h"


namespace Meros
{
  using namespace Teuchos;
  using namespace Thyra;

  /*! \ingroup PreconditionerFactory
   *
   *  \brief Factory for building least squares commutator
   *  style block preconditioner. 
   *
   *  Note that the LSC preconditioner assumes that we are using
   *  a stable discretization an a uniform mesh.
   *  
   *  The LDU factors of a saddle point system are given as follows:
   *  
   *  \f$
   *  \left[ \begin{array}{cc} A & B^T \\ B & C \end{array} \right]
   *  = \left[ \begin{array}{cc} I & \\ BF^{-1} & I \end{array} \right]
   *    \left[ \begin{array}{cc} F & \\  & -S \end{array} \right]
   *    \left[ \begin{array}{cc} I & F^{-1} B^T  \\  & I \end{array} \right],
   *  \f$
   *  
   *  where \f$S\f$ is the Schur complement \f$S = B F^{-1} B^T - C\f$.
   *  A pressure convection-diffusion style preconditioner is then given by
   *  
   *  \f$
   *  P^{-1} =
   *    \left[ \begin{array}{cc} F & B^T \\ & -\tilde S \end{array} \right]^{-1}
   *    = 
   *    \left[ \begin{array}{cc} F^{-1} &  \\  & I \end{array} \right]
   *    \left[ \begin{array}{cc} I & -B^T \\  & I \end{array} \right]
   *    \left[ \begin{array}{cc} I &  \\  & -\tilde S^{-1} \end{array} \right]
   *  \f$
   * 
   *  where for \f$\tilde S\f$ 
   *  is an approximation to the Schur complement S.
   * 
   *  To apply the above
   *  preconditioner, we need a linear solver on the (0,0) block
   *  and an approximation to the inverse of the Schur
   *  complement.
   *
   *  To build a concrete
   *  preconditioner object, we will also need a 2x2 block Thyra
   *  matrix or the 4 separate blocks as either Thyra or
   *  Epetra matrices.  If Thyra, assumes each block is a Thyra
   *  EpetraMatrix.
   */


  class LSCPreconditionerFactory 
    : public PreconditionerFactoryBase<double>
    {
    public:
      /** @name Constructors/initializers/accessors */
      //@{

      /** \brief Default constructor. */
      LSCPreconditionerFactory();
      

      /** \brief Constructor for Pressure Convection-Diffusion
       *  preconditioner factory. Takes an AztecOO parameter list for
       *  the F (convection-diffusion) solve and the B*Bt 
       *  solve.  */
      LSCPreconditionerFactory(RefCountPtr<ParameterList> azFParams,
			       RefCountPtr<ParameterList> azBBtParams);
      //@}

      /** @name Overridden from PreconditionerFactoryBase */
      //@{

      /** \brief Check that a <tt>LinearOperator</tt> object is compatible with
       * <tt>*this</tt> factory object.
       */
      bool isCompatible(const LinearOpSourceBase<double> &fwdOpSrc ) const;


      /** \brief Create an (uninitialized) <tt>LinearOperator</tt>
       * object to be initalized as the preconditioner later in
       * <tt>this->initializePrecOp()</tt>.
       *
       * Note that on output <tt>return->domain().get()==NULL</tt> may
       * be true which means that the operator is not fully
       * initialized.  In fact, the output operator object is not
       * guaranteed to be fully initialized until after it is passed
       * through <tt>this->initializePrecOp()</tt>.
       */
      RefCountPtr<PreconditionerBase<double> > createPrec() const;


      /** \brief Initialize the LSCPreconditioner object */
      void initializePrec(const RefCountPtr<const LinearOpSourceBase<double> >
			  &fwdOpSrc,
			  PreconditionerBase<double> *precOp,
			  const ESupportSolveUse 
			  supportSolveUse = SUPPORT_SOLVE_UNSPECIFIED) const;



      /** \brief Uninitialize the LSCPreconditioner object */
      void uninitializePrec(PreconditionerBase<double> *prec,
			    RefCountPtr<const LinearOpSourceBase<double> >  *fwdOpSrc,
			    ESupportSolveUse *supportSolveUse = NULL) const;


      //@}

      /** @name Overridden from ParameterListAcceptor */
      //@{
      
      /** \brief . */
      void setParameterList(RefCountPtr<ParameterList> const& paramList);
      /** \brief . */
      RefCountPtr<ParameterList> getParameterList();
      /** \brief . */
      RefCountPtr<ParameterList> unsetParameterList();
      /** \brief . */
      RefCountPtr<const ParameterList> getParameterList() const;
      /** \brief . */
      RefCountPtr<const Teuchos::ParameterList> getValidParameters() const;
      //@}

      /* Deal with all of the TSFHandleable features */
      /* GET_RCP(PreconditionerFactoryBase<double>); */

    private:
      mutable RefCountPtr<ParameterList>  validPL_;
      RefCountPtr<ParameterList>          paramList_;

      RefCountPtr<ParameterList> azFParams_;
      RefCountPtr<ParameterList> azBBtParams_;
    };

}  // namespace Meros

#endif  // MEROS_LSC_PRECONDITIONERFACTORY_H
