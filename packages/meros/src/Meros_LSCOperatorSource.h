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

#ifndef MEROS_LSC_OPERATOR_SOURCE_H
#define MEROS_LSC_OPERATOR_SOURCE_H

#include "Thyra_LinearOpSourceBase.hpp"
#include "Thyra_VectorImpl.hpp" 
#include "Thyra_VectorSpaceImpl.hpp" 
#include "Thyra_LinearOperatorDecl.hpp"
#include "Epetra_RowMatrix.h"
#include "Teuchos_RCP.hpp"


namespace Meros 
{

  using namespace Thyra;
  
  /** \brief Meros implementation of a Thyra
   * <tt>LinearOpSourceBase</tt> that accepts and gives up 
   * linear operators for an LSC preconditioner
   */
  //template <class double, class double = double>
  class LSCOperatorSource : virtual public LinearOpSourceBase<double>
    {
    public:

      /** @name Constructors/initializers/accessors */
      //@{

      /** \brief Construct to uninitialized.
       */
      LSCOperatorSource();


      /** \brief Construct with saddle LinearOperator
       */
      LSCOperatorSource(ConstLinearOperator<double> op);

      /** \brief Construct with saddle and Qu LinearOperator
       */
      LSCOperatorSource(ConstLinearOperator<double> op,
			ConstLinearOperator<double> Qu);

      /** \brief Construct with epetra operators 
       */
      LSCOperatorSource(Epetra_RowMatrix* S00,
			Epetra_RowMatrix* S01,
			Epetra_RowMatrix* S10,
			Epetra_RowMatrix* S11);


      /** \brief Construct with epetra operators 
       */
      LSCOperatorSource(Epetra_RowMatrix* S00,
			Epetra_RowMatrix* S01,
			Epetra_RowMatrix* S10,
			Epetra_RowMatrix* S11,
			Epetra_RowMatrix* Qu);



/*       /\** \brief Initialize with saddle LinearOperator */
/*        *\/ */
/*       void initialize(ConstLinearOperator<double> op); */


      /** \brief Initialize with saddle and Qu LinearOperators
       */
      void initialize(ConstLinearOperator<double> op,
		      ConstLinearOperator<double> Qu);


      /** \brief Uninitialize.
       *
       * Note: If the client wants to access the underlying linear
       * operator, then it had better grab them with the below access
       * functions before calling this function.
       */
      void uninitialize();
      
      
      /** \brief . */
      bool isOpConst() const;

      /** \brief . */
      RCP<const LinearOpBase<double> > getOp() const;

      /** \brief . */
      RCP<LinearOpBase<double> > getNonconstOp() ;

      /** \brief . */
      ConstLinearOperator<double> getSaddleOp() const;

      /** \brief . */
      ConstLinearOperator<double> getQu() const;


  
    private:
      
      Teuchos::ConstNonconstObjectContainer<LinearOpBase<double> >  op_;
      Teuchos::ConstNonconstObjectContainer<LinearOpBase<double> >  Qu_;

    };


} // namespace Meros

#endif // MEROS_LSC_OPERATOR_SOURCE_H
