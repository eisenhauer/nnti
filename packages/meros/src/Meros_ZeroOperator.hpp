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

#ifndef MEROS_ZERO_OPERATOR_H
#define MEROS_ZERO_OPERATOR_H

#include "Thyra_VectorImpl.hpp" // need for LinOpDecl
#include "Thyra_VectorSpaceImpl.hpp" // need for LinOpDecl
#include "Thyra_LinearOperatorImpl.hpp"
#include "Thyra_DefaultZeroLinearOp.hpp"


namespace Meros 
{
  using namespace Teuchos;
  using namespace Thyra;

  /** \brief 
   * Clean interface for creating a Thyra zero operator
   */
  template <class Scalar>
  class ZeroOperator
    : public Handleable<LinearOpBase<Scalar> >,
      public DefaultZeroLinearOp<Scalar>
  {
  public:
    /** */
    ZeroOperator(const VectorSpace<Scalar>& range,
                 const VectorSpace<Scalar>& domain)
      : DefaultZeroLinearOp<Scalar>(range.constPtr(),
                                    domain.constPtr())
    {
    }
    
    /* */
    TEUCHOS_GET_RCP(LinearOpBase<Scalar> );
  };


} // namespace Meros

#endif // MEROS_PCD_OPERATOR_SOURCE_H
