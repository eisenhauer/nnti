// /////////////////////////////////////
// MatrixHessianRelaxed.cpp
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

#include <assert.h>

#include "ConstrainedOptimizationPack/src/MatrixHessianRelaxed.hpp"
#include "SparseLinAlgPack/src/MatrixSymOp.hpp"
#include "SparseLinAlgPack/src/GenPermMatrixSlice.hpp"
#include "SparseLinAlgPack/src/SpVectorClass.hpp"
#include "SparseLinAlgPack/src/SpVectorOp.hpp"
#include "DenseLinAlgPack/src/LinAlgOpPack.hpp"

namespace LinAlgOpPack {
	using SparseLinAlgPack::Vp_StV;
	using SparseLinAlgPack::Vp_StMtV;
}

namespace ConstrainedOptimizationPack {

MatrixHessianRelaxed::MatrixHessianRelaxed()
	:
		n_(0)
		,H_(NULL)
		,bigM_(0.0)
{}

void MatrixHessianRelaxed::initialize(
	  const MatrixSymOp	&H
	, value_type			bigM
	)
{
	n_	= H.rows();
	H_	= &H;
	bigM_	= bigM;
}

// Overridden from Matrix

size_type MatrixHessianRelaxed::rows() const
{
	return n_ ? n_ + 1 : 0;
}

// Overridden from MatrixOp

void MatrixHessianRelaxed::Vp_StMtV(
	  DVectorSlice* y, value_type a, BLAS_Cpp::Transp M_trans
	, const DVectorSlice& x, value_type b ) const
{
	using BLAS_Cpp::no_trans;
	using BLAS_Cpp::trans;
	using SparseLinAlgPack::Vp_StMtV;
	//
	// y = b*y + a * M * x
	// 
	//   = b*y + a * [ H  0    ] * [ x1 ]
	//               [ 0  bigM ]   [ x2 ]
	//               
	// =>              
	//               
	// y1 = b*y1 + a*H*x1
	// 
	// y2 = b*y2 + bigM * x2
	//
	LinAlgOpPack::Vp_MtV_assert_sizes(y->size(),rows(),cols(),M_trans,x.size());

	DVectorSlice
		y1 = (*y)(1,n_);
	value_type
		&y2 = (*y)(n_+1);
	const DVectorSlice
		x1 = x(1,n_);
	const value_type
		x2 = x(n_+1);

	// y1 = b*y1 + a*H*x1
	Vp_StMtV( &y1, a, *H_, no_trans, x1, b );

	// y2 = b*y2 + bigM * x2
	if( b == 0.0 )
		y2 = bigM_ * x2;
	else
		y2 = b*y2 + bigM_ * x2;
	
}

void MatrixHessianRelaxed::Vp_StMtV(
	  DVectorSlice* y, value_type a, BLAS_Cpp::Transp M_trans
	, const SpVectorSlice& x, value_type b ) const
{
	using BLAS_Cpp::no_trans;
	using BLAS_Cpp::trans;
	using SparseLinAlgPack::Vp_StMtV;
	//
	// y = b*y + a * M * x
	// 
	//   = b*y + a * [ H  0    ] * [ x1 ]
	//               [ 0  bigM ]   [ x2 ]
	//               
	// =>              
	//               
	// y1 = b*y1 + a*H*x1
	// 
	// y2 = b*y2 + bigM * x2
	//
	LinAlgOpPack::Vp_MtV_assert_sizes(y->size(),rows(),cols(),M_trans,x.size());

	DVectorSlice
		y1 = (*y)(1,n_);
	value_type
		&y2 = (*y)(n_+1);
	const SpVectorSlice
		x1 = x(1,n_);
	const SpVectorSlice::element_type
		*x2_ele = x.lookup_element(n_+1);
	const value_type
		x2 = x2_ele ? x2_ele->value() : 0.0;

	// y1 = b*y1 + a*H*x1
	Vp_StMtV( &y1, a, *H_, no_trans, x1, b );

	// y2 = b*y2 + bigM * x2
	if( b == 0.0 )
		y2 = bigM_ * x2;
	else
		y2 = b*y2 + bigM_ * x2;
	
}

void MatrixHessianRelaxed::Vp_StPtMtV(
	DVectorSlice* y, value_type a
	, const GenPermMatrixSlice& P, BLAS_Cpp::Transp P_trans
	, BLAS_Cpp::Transp M_trans
	, const DVectorSlice& x, value_type b ) const
{
	using BLAS_Cpp::no_trans;
	using BLAS_Cpp::trans;
	namespace GPMSIP = SparseLinAlgPack::GenPermMatrixSliceIteratorPack;
	//
	// y = b*y + a * op(P) * M * x
	// 
	//   = b*y + a * [ op(P1)  op(P2) ] *  [ H   0   ] * [ x1 ]
	//                                     [ 0  bigM ]   [ x2 ]
	//               
	// =>              
	//               
	// y = b*y + a*op(P1)*H*x1 + a*op(P2)*bigM*x2
	//
	LinAlgOpPack::Vp_MtV_assert_sizes(y->size(),P.rows(),P.cols(),P_trans
		, BLAS_Cpp::rows( rows(), cols(), M_trans) );
	LinAlgOpPack::Vp_MtV_assert_sizes( BLAS_Cpp::cols( P.rows(), P.cols(), P_trans)
		,rows(),cols(),M_trans,x.size());

	// For this to work (as shown below) we need to have P sorted by
	// row if op(P) = P' or sorted by column if op(P) = P.  If
	// P is not sorted properly, we will just use the default
	// implementation of this operation.
	if( 	( P.ordered_by() == GPMSIP::BY_ROW && P_trans == no_trans )
	    || 	( P.ordered_by() == GPMSIP::BY_COL && P_trans == trans ) )
	{
		// Call the default implementation
		MatrixOp::Vp_StPtMtV(y,a,P,P_trans,M_trans,x,b);
		return;
	}

	if( P.is_identity() )
		assert( BLAS_Cpp::rows( P.rows(), P.cols(), P_trans ) == n_ );

	const GenPermMatrixSlice
		P1 = ( P.is_identity() 
			   ? GenPermMatrixSlice( n_, n_, GenPermMatrixSlice::IDENTITY_MATRIX )
			   : P.create_submatrix(Range1D(1,n_),P_trans==trans?GPMSIP::BY_ROW:GPMSIP::BY_COL)
			),
		P2 = ( P.is_identity()
			   ? GenPermMatrixSlice(
				   P_trans == no_trans ? n_ : 1
				   , P_trans == no_trans ? 1 : n_
				   , GenPermMatrixSlice::ZERO_MATRIX )
			   : P.create_submatrix(Range1D(n_+1,n_+1),P_trans==trans?GPMSIP::BY_ROW:GPMSIP::BY_COL)
			);
	
	const DVectorSlice
		x1 = x(1,n_);
	const value_type
		x2 = x(n_+1);
	// y = b*y + a*op(P1)*H*x1
	SparseLinAlgPack::Vp_StPtMtV( y, a, P1, P_trans, *H_, no_trans, x1, b );
	// y += a*op(P2)*bigM*x2
	if( P2.nz() ){
		assert(P2.nz() == 1);
		const size_type
			i = P_trans == no_trans ? P2.begin()->row_i() : P2.begin()->col_j();
		(*y)(i) += a * bigM_ * x2;
	}
}

void MatrixHessianRelaxed::Vp_StPtMtV(
	DVectorSlice* y, value_type a
	, const GenPermMatrixSlice& P, BLAS_Cpp::Transp P_trans
	, BLAS_Cpp::Transp M_trans
	, const SpVectorSlice& x, value_type b ) const
{
	using BLAS_Cpp::no_trans;
	using BLAS_Cpp::trans;
	namespace GPMSIP = SparseLinAlgPack::GenPermMatrixSliceIteratorPack;
	//
	// y = b*y + a * op(P) * M * x
	// 
	//   = b*y + a * [ op(P1)  op(P2) ] *  [ H   0   ] * [ x1 ]
	//                                     [ 0  bigM ]   [ x2 ]
	//               
	// =>              
	//               
	// y = b*y + a*op(P1)*H*x1 + a*op(P2)*bigM*x2
	//
	LinAlgOpPack::Vp_MtV_assert_sizes(y->size(),P.rows(),P.cols(),P_trans
		, BLAS_Cpp::rows( rows(), cols(), M_trans) );
	LinAlgOpPack::Vp_MtV_assert_sizes( BLAS_Cpp::cols( P.rows(), P.cols(), P_trans)
		,rows(),cols(),M_trans,x.size());

	// For this to work (as shown below) we need to have P sorted by
	// row if op(P) = P' or sorted by column if op(P) = P.  If
	// P is not sorted properly, we will just use the default
	// implementation of this operation.
	if( 	( P.ordered_by() == GPMSIP::BY_ROW && P_trans == no_trans )
	    || 	( P.ordered_by() == GPMSIP::BY_COL && P_trans == trans ) )
	{
		// Call the default implementation
		MatrixOp::Vp_StPtMtV(y,a,P,P_trans,M_trans,x,b);
		return;
	}

	if( P.is_identity() )
		assert( BLAS_Cpp::rows( P.rows(), P.cols(), P_trans ) == n_ );

	const GenPermMatrixSlice
		P1 = ( P.is_identity() 
			   ? GenPermMatrixSlice( n_, n_, GenPermMatrixSlice::IDENTITY_MATRIX )
			   : P.create_submatrix(Range1D(1,n_),P_trans==trans?GPMSIP::BY_ROW:GPMSIP::BY_COL)
			),
		P2 = ( P.is_identity()
			   ? GenPermMatrixSlice(
				   P_trans == no_trans ? n_ : 1
				   , P_trans == no_trans ? 1 : n_
				   , GenPermMatrixSlice::ZERO_MATRIX )
			   : P.create_submatrix(Range1D(n_+1,n_+1),P_trans==trans?GPMSIP::BY_ROW:GPMSIP::BY_COL)
			);

	const SpVectorSlice
		x1 = x(1,n_);
	const SpVectorSlice::element_type
		*x2_ele = x.lookup_element(n_+1);
	const value_type
		x2 = x2_ele ? x2_ele->value() : 0.0;
	// y = b*y + a*op(P1)*H*x1
	SparseLinAlgPack::Vp_StPtMtV( y, a, P1, P_trans, *H_, no_trans, x1, b );
	// y += a*op(P2)*bigM*x2
	if( P2.nz() ){
		assert(P2.nz() == 1);
		const size_type
			i = P_trans == no_trans ? P2.begin()->row_i() : P2.begin()->col_j();
		(*y)(i) += a * bigM_ * x2;
	}
}

value_type MatrixHessianRelaxed::transVtMtV(
	const SpVectorSlice& x1, BLAS_Cpp::Transp M_trans
	, const SpVectorSlice& x2 ) const
{
	using BLAS_Cpp::no_trans;
	//
	// a = x1' * M * x2
	// 
	//   = [ x11'  x12' ] * [ H   0   ] * [ x21 ]
	//                      [ 0  bigM ]   [ x22 ]
	//               
	// =>              
	//               
	// a = x11'*H*x21 + x12'*bigM*x22
	//
	DenseLinAlgPack::Vp_MtV_assert_sizes(x1.size(),rows(),cols(),M_trans,x2.size());

	if( &x1 == &x2 ) {
		// x1 and x2 are the same sparse vector
		const SpVectorSlice
			x11 = x1(1,n_);
		const SpVectorSlice::element_type
			*x12_ele = x1.lookup_element(n_+1);
		const value_type
			x12 = x12_ele ? x12_ele->value() : 0.0;
		return SparseLinAlgPack::transVtMtV( x11, *H_, no_trans, x11) 
			+ x12 * bigM_ * x12;
	}
	else {
		// x1 and x2 could be different sparse vectors
		assert(0); // ToDo: Implement this!
	}
	return 0.0; // Will never be executed!
}

}	// end namespace ConstrainedOptimizationPack
