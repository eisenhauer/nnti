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

#include "AbstractLinAlgPack_LinAlgOpPackHack.hpp"
#include "AbstractLinAlgPack_COOMatrixPartitionViewSubclass.hpp"
#include "AbstractLinAlgPack_SparseVectorSliceOp.hpp"
#include "AbstractLinAlgPack_SparseElement.hpp"
#include "AbstractLinAlgPack_COOMPartitionOp.hpp"
#include "DenseLinAlgPack_DMatrixOp.hpp"

namespace LinAlgOpPack {

using AbstractLinAlgPack::Vp_StV;
using AbstractLinAlgPack::Vp_StMtV;
using AbstractLinAlgPack::Mp_StM;
using AbstractLinAlgPack::Mp_StMtM;

}	// end namespace LinAlgOpPack

namespace AbstractLinAlgPack {

size_type COOMatrixPartitionViewSubclass::rows() const {
  return trans_ == BLAS_Cpp::no_trans ? m().rows() : m().cols();
}

size_type COOMatrixPartitionViewSubclass::cols() const {
  return trans_ == BLAS_Cpp::no_trans ? m().cols() : m().rows();
}

MatrixOp& COOMatrixPartitionViewSubclass::operator=(const MatrixOp& m) {
  if(&m == this) return *this;	// assignment to self
  const COOMatrixPartitionViewSubclass *p_m = dynamic_cast<const COOMatrixPartitionViewSubclass*>(&m);
  if(p_m) {
    throw std::invalid_argument("COOMatrixPartitionViewSubclass::operator=(const MatrixOp& m)"
      " :  There is not an assignment operator defined for COOMatrixWithPartitionedView::partition_type"
      ".   Only assignment to self can be handeled" );
  }
  else {
    throw std::invalid_argument("COOMatrixPartitionViewSubclass::operator=(const MatrixOp& m)"
      " : The concrete type of m is not a subclass of COOMatrixPartitionViewSubclass as expected" );
  }
  return *this;
}

// Level-1 BLAS

void COOMatrixPartitionViewSubclass::Mp_StM(DMatrixSlice* gms_lhs, value_type alpha
  , BLAS_Cpp::Transp trans_rhs) const
{
  AbstractLinAlgPack::Mp_StM(gms_lhs,alpha,m(),op(trans_rhs));
}

// Level-2 BLAS

void COOMatrixPartitionViewSubclass::Vp_StMtV(DVectorSlice* vs_lhs, value_type alpha
  , BLAS_Cpp::Transp trans_rhs1, const DVectorSlice& vs_rhs2, value_type beta) const
{
  AbstractLinAlgPack::Vp_StMtV(vs_lhs, alpha, m(), op(trans_rhs1), vs_rhs2, beta);
}

void COOMatrixPartitionViewSubclass::Vp_StMtV(DVectorSlice* vs_lhs, value_type alpha
  , BLAS_Cpp::Transp trans_rhs1, const SpVectorSlice& sv_rhs2, value_type beta) const
{
  DVector v_rhs2;
  LinAlgOpPack::assign(&v_rhs2,sv_rhs2);
  AbstractLinAlgPack::Vp_StMtV(vs_lhs, alpha, m(), op(trans_rhs1), v_rhs2(), beta);
}

value_type COOMatrixPartitionViewSubclass::transVtMtV(const DVectorSlice& vs_rhs1
  , BLAS_Cpp::Transp trans_rhs2, const DVectorSlice& vs_rhs3) const
{
  DVector tmp;
  LinAlgOpPack::V_MtV(&tmp,m(),op(trans_rhs2),vs_rhs3);
  return DenseLinAlgPack::dot(vs_rhs1,tmp());
}

value_type COOMatrixPartitionViewSubclass::transVtMtV(const SpVectorSlice& sv_rhs1
  , BLAS_Cpp::Transp trans_rhs2, const SpVectorSlice& sv_rhs3) const
{
  DVector v_rhs3;
  LinAlgOpPack::assign(&v_rhs3,sv_rhs3);
  DVector tmp;
  LinAlgOpPack::V_MtV(&tmp,m(),op(trans_rhs2),v_rhs3());
  return dot(sv_rhs1,tmp());
}

// Level-3 BLAS

void COOMatrixPartitionViewSubclass::Mp_StMtM(DMatrixSlice* gms_lhs, value_type alpha
  , BLAS_Cpp::Transp trans_rhs1, const DMatrixSlice& gms_rhs2
  , BLAS_Cpp::Transp trans_rhs2, value_type beta) const
{
  AbstractLinAlgPack::Mp_StMtM(gms_lhs, alpha, m(), op(trans_rhs1), gms_rhs2, trans_rhs2, beta);
}

void COOMatrixPartitionViewSubclass::Mp_StMtM(DMatrixSlice* gms_lhs, value_type alpha, const DMatrixSlice& gms_rhs1
  , BLAS_Cpp::Transp trans_rhs1, BLAS_Cpp::Transp trans_rhs2, value_type beta) const
{
  AbstractLinAlgPack::Mp_StMtM(gms_lhs, alpha, gms_rhs1, trans_rhs1, m(), op(trans_rhs2), beta);
}

}	// end namespace AbstractLinAlgPack
