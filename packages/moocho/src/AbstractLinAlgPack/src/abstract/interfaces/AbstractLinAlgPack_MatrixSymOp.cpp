// @HEADER
// ***********************************************************************
// 
// Moocho: Multi-functional Object-Oriented arCHitecture for Optimization
//                  Copyright (2003) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Roscoe A. Bartlett (rabartl@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#include <assert.h>

#include "AbstractLinAlgPack_MatrixSymOp.hpp"
#include "AbstractLinAlgPack_EtaVector.hpp"

namespace AbstractLinAlgPack {

MatrixSymOp::mat_mswo_mut_ptr_t
MatrixSymOp::clone_mswo()
{
  return Teuchos::null;
}

MatrixSymOp::mat_mswo_ptr_t
MatrixSymOp::clone_mswo() const
{
  return Teuchos::null;
}

void MatrixSymOp::Mp_StPtMtP(
  MatrixSymOp* sym_lhs, value_type alpha
  , EMatRhsPlaceHolder dummy_place_holder
  , const GenPermMatrixSlice& gpms_rhs, BLAS_Cpp::Transp gpms_rhs_trans
  , value_type beta ) const
{
  TEUCHOS_TEST_FOR_EXCEPT(true); // ToDo: Implement!
}

void MatrixSymOp::Mp_StMtMtM(
  MatrixSymOp* sym_lhs, value_type alpha
  , EMatRhsPlaceHolder dummy_place_holder
  , const MatrixOp& mwo_rhs, BLAS_Cpp::Transp mwo_rhs_trans
  , value_type beta ) const
{
  TEUCHOS_TEST_FOR_EXCEPT(true); // ToDo: Implement!
}

// Overridden from MatrixOp


size_type MatrixSymOp::cols() const
{
  return this->rows();
}

const VectorSpace& MatrixSymOp::space_rows() const
{
  return this->space_cols();
}

MatrixSymOp::mat_mut_ptr_t
MatrixSymOp::clone()
{
  return clone_mswo();
}

MatrixSymOp::mat_ptr_t
MatrixSymOp::clone() const
{
  return clone_mswo();
}

}	// end namespace AbstractLinAlgPack 
