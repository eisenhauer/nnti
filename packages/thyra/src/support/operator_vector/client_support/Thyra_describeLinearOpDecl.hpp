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

#ifndef THYRA_DESCRIBE_LINEAR_OP_DECL_HPP
#define THYRA_DESCRIBE_LINEAR_OP_DECL_HPP

#include "Thyra_OperatorVectorTypes.hpp"
#include "Teuchos_FancyOStream.hpp"

namespace Thyra {

/** \brief Basic implementation of a describe function for a linear operator.
 *
 * \ingroup Thyra_Op_Vec_ANA_Developmnet_support_code_grp
 */
template<class RangeScalar, class DomainScalar>
void describeLinearOp(
  const LinearOpBase<RangeScalar,DomainScalar>   &A
  ,Teuchos::FancyOStream                         &out
  ,const Teuchos::EVerbosityLevel                verbLevel
  );

}	// end namespace Thyra

#endif // THYRA_DESCRIBE_LINEAR_OP_DECL_HPP
