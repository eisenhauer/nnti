// ///////////////////////////////////////////////////////////////////////
// LinAlgOpPackHack.cpp
//
// Copyright (C) 2001 Roscoe Ainsworth Bartlett
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the "Artistic License" (see the web site
//   http://www.opensource.org/licenses/artistic-license.html).
// This license is spelled out in the file COPYING.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// above mentioned "Artistic License" for more details.
//

#include "SparseLinAlgPack/include/LinAlgOpPackHack.h"
#include "SparseLinAlgPack/include/VectorWithOpMutableDense.h"
#include "SparseLinAlgPack/include/VectorDenseEncap.h"
#include "SparseLinAlgPack/include/MatrixWithOpGetGMS.h"
#include "AbstractLinAlgPack/include/MatrixWithOpNonsingular.h"
#include "AbstractLinAlgPack/include/MultiVectorMutable.h"
#include "AbstractLinAlgPack/include/VectorWithOpMutable.h"
#include "AbstractLinAlgPack/include/VectorSpace.h"
#include "AbstractLinAlgPack/include/GenPermMatrixSlice.h"
#include "AbstractLinAlgPack/include/LinAlgOpPack.h"
#include "LinAlgPack/include/GenMatrixOp.h"

void LinAlgOpPack::Mp_StM(
	GenMatrixSlice* C, value_type a
	,const MatrixWithOp& B, BLAS_Cpp::Transp B_trans
	)
{
	using AbstractLinAlgPack::VectorSpace;
	using SparseLinAlgPack::VectorDenseEncap;
	using SparseLinAlgPack::MatrixWithOpGetGMS;
	using SparseLinAlgPack::MatrixDenseEncap;
	const MatrixWithOpGetGMS
		*B_get_gms = dynamic_cast<const MatrixWithOpGetGMS*>(&B);
	if(B_get_gms) {
		LinAlgPack::Mp_StM( C, a, MatrixDenseEncap(*B_get_gms)(), B_trans );		
	}
	else {
		const size_type num_cols = C->cols();
		VectorSpace::multi_vec_mut_ptr_t
			B_mv = ( B_trans == BLAS_Cpp::no_trans 
					? B.space_cols() : B.space_rows()
					).create_members(num_cols);
		assign(B_mv.get(),B,B_trans);
		for( size_type j = 1; j <= num_cols; ++j ) {
			LinAlgPack::Vp_StV(&C->col(j),a,VectorDenseEncap(*B_mv->col(j))());
		}
	}
}

void LinAlgOpPack::Vp_StMtV(
	VectorSlice* y, value_type a, const MatrixWithOp& M
	,BLAS_Cpp::Transp M_trans, const VectorSlice& x, value_type b
	)
{
	using BLAS_Cpp::no_trans;
	using SparseLinAlgPack::VectorDenseMutableEncap;
	VectorSpace::vec_mut_ptr_t
		ay = ( M_trans == no_trans ? M.space_cols() : M.space_rows() ).create_member(),
		ax = ( M_trans == no_trans ? M.space_rows() : M.space_cols() ).create_member();
	(VectorDenseMutableEncap(*ay))() = *y;
	(VectorDenseMutableEncap(*ax))() = x;
	AbstractLinAlgPack::Vp_StMtV( ay.get(), a, M, M_trans, *ax, b );
	*y = VectorDenseMutableEncap(*ay)();
}

void LinAlgOpPack::Vp_StMtV(
	VectorSlice* y, value_type a, const MatrixWithOp& M
	,BLAS_Cpp::Transp M_trans, const SpVectorSlice& x, value_type b
	)
{
	using BLAS_Cpp::no_trans;
	using SparseLinAlgPack::VectorDenseMutableEncap;
	VectorSpace::vec_mut_ptr_t
		ay = ( M_trans == no_trans ? M.space_cols() : M.space_rows() ).create_member();
	(VectorDenseMutableEncap(*ay))() = *y;
	AbstractLinAlgPack::Vp_StMtV( ay.get(), a, M, M_trans, x, b );
	*y = VectorDenseMutableEncap(*ay)();
}

void LinAlgOpPack::V_InvMtV(
	VectorSlice* y, const MatrixWithOpNonsingular& M
	,BLAS_Cpp::Transp M_trans, const VectorSlice& x
	)
{
	using BLAS_Cpp::trans;
	using SparseLinAlgPack::VectorDenseMutableEncap;
	VectorSpace::vec_mut_ptr_t
		ay = ( M_trans == trans ? M.space_cols() : M.space_rows() ).create_member(),
		ax = ( M_trans == trans ? M.space_rows() : M.space_cols() ).create_member();
	(VectorDenseMutableEncap(*ay))() = *y;
	(VectorDenseMutableEncap(*ax))() = x;
	AbstractLinAlgPack::V_InvMtV( ay.get(), M, M_trans, *ax );
	*y = VectorDenseMutableEncap(*ay)();
}

void LinAlgOpPack::V_InvMtV(
	Vector* y, const MatrixWithOpNonsingular& M
	,BLAS_Cpp::Transp M_trans, const VectorSlice& x
	)
{
	using BLAS_Cpp::trans;
	using SparseLinAlgPack::VectorDenseMutableEncap;
	VectorSpace::vec_mut_ptr_t
		ay = ( M_trans == trans ? M.space_cols() : M.space_rows() ).create_member(),
		ax = ( M_trans == trans ? M.space_rows() : M.space_cols() ).create_member();
	(VectorDenseMutableEncap(*ax))() = x;
	AbstractLinAlgPack::V_InvMtV( ay.get(), M, M_trans, *ax );
	*y = VectorDenseMutableEncap(*ay)();
}

void LinAlgOpPack::V_InvMtV(
	VectorSlice* y, const MatrixWithOpNonsingular& M
	,BLAS_Cpp::Transp M_trans, const SpVectorSlice& x
	)
{
	using BLAS_Cpp::trans;
	using SparseLinAlgPack::VectorDenseMutableEncap;
	VectorSpace::vec_mut_ptr_t
		ay = ( M_trans == trans ? M.space_cols() : M.space_rows() ).create_member();
	AbstractLinAlgPack::V_InvMtV( ay.get(), M, M_trans, x );
	*y = VectorDenseMutableEncap(*ay)();
}

void LinAlgOpPack::V_InvMtV(
	Vector* y, const MatrixWithOpNonsingular& M
	,BLAS_Cpp::Transp M_trans, const SpVectorSlice& x
	)
{
	y->resize(M.rows());
	LinAlgOpPack::V_InvMtV( &(*y)(), M, M_trans, x );
}

// These methods below are a real problem to implement in general.
//
// If the column space of op(M) could not return the above vector space
// for op(M).space_cols().space(P,P_trans) then we will try, as a last
// resort, to a dense serial vector and see what happens.

void LinAlgOpPack::Vp_StPtMtV(
	VectorSlice* y, value_type a
	,const GenPermMatrixSlice& P, BLAS_Cpp::Transp P_trans
	,const MatrixWithOp& M, BLAS_Cpp::Transp M_trans
	,const VectorSlice& x, value_type b
	)
{
	namespace mmp = MemMngPack;
	using BLAS_Cpp::no_trans;
	using SparseLinAlgPack::VectorWithOpMutableDense;
	using SparseLinAlgPack::VectorDenseMutableEncap;
	using AbstractLinAlgPack::Vp_StPtMtV;
	VectorSpace::space_ptr_t
		ay_space = ( M_trans == no_trans ? M.space_cols() : M.space_rows() ).space(P,P_trans);
	VectorSpace::vec_mut_ptr_t
		ay =  ( ay_space.get()
				? ay_space->create_member()
				: mmp::rcp_implicit_cast<VectorWithOpMutable>(
					mmp::rcp(new VectorWithOpMutableDense(BLAS_Cpp::rows(P.rows(),P.cols(),P_trans)))
					) ),
		ax = ( M_trans == no_trans ? M.space_rows() : M.space_cols() ).create_member();
	(VectorDenseMutableEncap(*ay))() = *y;
	(VectorDenseMutableEncap(*ax))() = x;
	Vp_StPtMtV( ay.get(), a, P, P_trans, M, M_trans, *ax, b );
	*y = VectorDenseMutableEncap(*ay)();
}

void LinAlgOpPack::Vp_StPtMtV(
	VectorSlice* y, value_type a
	,const GenPermMatrixSlice& P, BLAS_Cpp::Transp P_trans
	,const MatrixWithOp& M, BLAS_Cpp::Transp M_trans
	,const SpVectorSlice& x, value_type b
	)
{
	using BLAS_Cpp::no_trans;
	using SparseLinAlgPack::VectorWithOpMutableDense;
	using SparseLinAlgPack::VectorDenseMutableEncap;
	using AbstractLinAlgPack::Vp_StPtMtV;
	VectorSpace::vec_mut_ptr_t
		ay = ( M_trans == no_trans ? M.space_cols() : M.space_rows() ).space(P,P_trans)->create_member();
	(VectorDenseMutableEncap(*ay))() = *y;
	Vp_StPtMtV( ay.get(), a, P, P_trans, M, M_trans, x, b );
	*y = VectorDenseMutableEncap(*ay)();
}
