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

#ifndef Rythmos_UNITTEST_HELPERS_H
#define Rythmos_UNITTEST_HELPERS_H

#include "Teuchos_DefaultComm.hpp"

#include "Thyra_DefaultProductVectorSpace.hpp"
#include "Thyra_ProductVectorBase.hpp"
#include "Thyra_VectorSpaceBase.hpp"
#include "Thyra_DefaultSpmdVectorSpace.hpp"

namespace Rythmos {
  
// This function is needed to get a default vector space to work with.
template<class Scalar>
Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > createDefaultVectorSpace(int length) {
  const Teuchos::RCP<const Teuchos::Comm<Thyra::Index> >
    comm = Teuchos::DefaultComm<Thyra::Index>::getComm();
  Teuchos::RCP<const Thyra::DefaultSpmdVectorSpace<Scalar> > vs = 
    Teuchos::rcp(new const Thyra::DefaultSpmdVectorSpace<Scalar>(comm,length,-1) );
  return(vs); 
}

// This function returns a vector initialized with a value.
template<class Scalar>
Teuchos::RCP<Thyra::VectorBase<Scalar> > createDefaultVector(int length, Scalar value) {
  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > vs = createDefaultVectorSpace<Scalar>(length);
  Teuchos::RCP<Thyra::VectorBase<Scalar> > vec = Thyra::createMember(vs);
  Thyra::V_S(&*vec,value);
  return(vec);
}

// This function returns a product vector initialized with a value.
template<class Scalar>
Teuchos::RCP<Thyra::ProductVectorBase<Scalar> > createDefaultProductVector(int blocks, int length, Scalar value) {
  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > vs = createDefaultVectorSpace<Scalar>(length);
  Teuchos::RCP<const Thyra::VectorSpaceBase<Scalar> > pvs = productVectorSpace(vs,blocks);
  Teuchos::RCP<Thyra::ProductVectorBase<Scalar> > pvec = Teuchos::rcp_dynamic_cast<Thyra::ProductVectorBase<double> >(Thyra::createMember(pvs));
  Thyra::V_S(&*pvec,value);
  return(pvec);
}


} // namespace Rythmos
#endif // Rythmos_UNITTEST_HELPERS_H

