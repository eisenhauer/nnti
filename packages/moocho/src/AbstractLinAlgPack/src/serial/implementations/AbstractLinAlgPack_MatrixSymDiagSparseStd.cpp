// @HEADER
// ***********************************************************************
// 
// Moocho: Multi-functional Object-Oriented arCHitecture for Optimization
//                  Copyright (2003) Sandia Corporation
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
// Questions? Contact Roscoe A. Bartlett (rabartl@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#include "AbstractLinAlgPack_MatrixSymDiagSparseStd.hpp"
#include "AbstractLinAlgPack_SpVectorOut.hpp"
#include "DenseLinAlgPack_DVectorClass.hpp"
#include "DenseLinAlgPack_AssertOp.hpp"
#include "Teuchos_Assert.hpp"

namespace AbstractLinAlgPack {

MatrixSymDiagSparseStd::MatrixSymDiagSparseStd( const SpVectorSlice& diag )
  : diag_(diag)
{}

void MatrixSymDiagSparseStd::initialize( const SpVectorSlice& diag )
{
  diag_ = diag;
}

// Overridden from MatrixOp

MatrixOp& MatrixSymDiagSparseStd::operator=(const MatrixOp& m)
{
  if(&m == this) return *this;	// assignment to self
  const MatrixSymDiagSparseStd
    *p_m = dynamic_cast<const MatrixSymDiagSparseStd*>(&m);
  if(p_m) {
    diag_ = p_m->diag_;
  }
  else {
    TEUCHOS_TEST_FOR_EXCEPTION(
      true, std::invalid_argument
      ,"MatrixSymDiagSparseStd::operator=(const MatrixOp& m) : Error!"
      "The concrete type of m = \'" << typeName(m) << "\' is not a subclass of "
      "MatrixSymDiagSparseStd as expected"
      );
  }
  return *this;
}

// Overridden from MatrixDiagonalSparse

const SpVectorSlice MatrixSymDiagSparseStd::diag() const
{
  return diag_();
}


}	// end namespace AbstractLinAlgPack
