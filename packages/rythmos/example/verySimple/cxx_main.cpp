//@HEADER
// ************************************************************************
// 
//               Epetra: Linear Algebra Services Package 
//                 Copyright (2001) Sandia Corporation
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
// ************************************************************************
//@HEADER

#include "Epetra_SerialComm.h"
#include "Epetra_Map.h"
#include "Epetra_Vector.h"
#include "Epetra_Version.h"

#include "Thyra_EpetraThyraWrappers.hpp"
#include "Thyra_EpetraLinearOp.hpp"


int main(int argc, char *argv[])
{
  using Teuchos::RefCountPtr;
  using Teuchos::rcp;
  using Thyra::VectorSpaceBase;
  using Thyra::create_MPIVectorSpaceBase;
  using Thyra::VectorBase;
  using Thyra::createMember;
  using Thyra::randomize;
  using Thyra::V_StV;
  using Thyra::norm_2;

  cout << Epetra_Version() << endl << endl;

  int NumElements = 1000;

  typedef double Scalar;

  RefCountPtr<const Epetra_Comm> epetra_comm;
  epetra_comm = rcp(new Epetra_SerialComm);
  // Construct a Map with NumElements and index base of 0
  RefCountPtr<const Epetra_Map> epetra_map;
  epetra_map = rcp(new Epetra_Map(NumElements, 0, *epetra_comm));
  // Construct a VectorSpace from the map
  RefCountPtr<const VectorSpaceBase<Scalar> > epetra_vs;
  epetra_vs = create_MPIVectorSpaceBase(epetra_map);
  

  // Create x and b vectors
  RefCountPtr<VectorBase<Scalar> > b = createMember(epetra_vs);
  RefCountPtr<VectorBase<Scalar> > x = createMember(epetra_vs);

  randomize(0.0,1.0,&*b); // b = random
  V_StV(&*x,2.0,*b); // x = 2*b

  double bnorm, xnorm;
  xnorm = norm_2(*x);
  bnorm = norm_2(*b);

  cout << "2 norm of x = " << xnorm << endl
       << "2 norm of b = " << bnorm << endl;

  return 0;
}

