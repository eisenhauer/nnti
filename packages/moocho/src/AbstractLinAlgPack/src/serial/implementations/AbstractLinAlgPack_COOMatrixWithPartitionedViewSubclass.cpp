// //////////////////////////////////////////////////////////////////////////////////
// COOMatrixWithPartitionedViewSubclass.cpp
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

#include "COOMatrixWithPartitionedViewSubclass.hpp"
#include "SparseVectorSliceOp.hpp"
#include "SparseElement.hpp"
#include "COOMPartitionOp.hpp"
#include "COOMPartitionOut.hpp"
#include "COOMatrixTmplConvertToSparseCompressedColumn.hpp"
#include "LinAlgPack/src/GenMatrixOp.hpp"
#include "LinAlgPack/src/LinAlgOpPack.hpp"

namespace LinAlgOpPack {

using SparseLinAlgPack::Vp_StV;
using SparseLinAlgPack::Vp_StMtV;
using SparseLinAlgPack::Mp_StM;
using SparseLinAlgPack::Mp_StMtM;

}	// end namespace LinAlgOpPack

namespace SparseLinAlgPack {

size_type COOMatrixWithPartitionedViewSubclass::nz() const
{
	return m().coom_view().nz();
}

std::ostream& COOMatrixWithPartitionedViewSubclass::output(std::ostream& out) const {
	return out << m().coom_view()();
}

// Level-1 BLAS

void COOMatrixWithPartitionedViewSubclass::Mp_StM(GenMatrixSlice* gms_lhs, value_type alpha
	, BLAS_Cpp::Transp trans_rhs) const
{
	SparseLinAlgPack::Mp_StM(gms_lhs,alpha,m().coom_view()(),trans_rhs);
}

// Level-2 BLAS

void COOMatrixWithPartitionedViewSubclass::Vp_StMtV(VectorSlice* vs_lhs, value_type alpha
	, BLAS_Cpp::Transp trans_rhs1, const VectorSlice& vs_rhs2, value_type beta) const
{
	SparseLinAlgPack::Vp_StMtV(vs_lhs, alpha, m().coom_view()(), trans_rhs1, vs_rhs2, beta);
}

void COOMatrixWithPartitionedViewSubclass::Vp_StMtV(VectorSlice* vs_lhs, value_type alpha
	, BLAS_Cpp::Transp trans_rhs1, const SpVectorSlice& sv_rhs2, value_type beta) const
{
	Vector v_rhs2;
	LinAlgOpPack::assign(&v_rhs2,sv_rhs2);
	SparseLinAlgPack::Vp_StMtV(vs_lhs, alpha, m().coom_view()(), trans_rhs1, v_rhs2(), beta);
}

value_type COOMatrixWithPartitionedViewSubclass::transVtMtV(const VectorSlice& vs_rhs1
	, BLAS_Cpp::Transp trans_rhs2, const VectorSlice& vs_rhs3) const
{
	Vector tmp;
	LinAlgOpPack::V_MtV(&tmp,m().coom_view()(),trans_rhs2,vs_rhs3);
	return LinAlgPack::dot(vs_rhs1,tmp());
}

value_type COOMatrixWithPartitionedViewSubclass::transVtMtV(const SpVectorSlice& sv_rhs1
	, BLAS_Cpp::Transp trans_rhs2, const SpVectorSlice& sv_rhs3) const
{
	Vector v_rhs3;
	LinAlgOpPack::assign(&v_rhs3,sv_rhs3);
	Vector tmp;
	LinAlgOpPack::V_MtV(&tmp,m().coom_view()(),trans_rhs2,v_rhs3());
	return dot(sv_rhs1,tmp());
}

// Level-3 BLAS

void COOMatrixWithPartitionedViewSubclass::Mp_StMtM(GenMatrixSlice* gms_lhs, value_type alpha
	, BLAS_Cpp::Transp trans_rhs1, const GenMatrixSlice& gms_rhs2
	, BLAS_Cpp::Transp trans_rhs2, value_type beta) const
{
	SparseLinAlgPack::Mp_StMtM(gms_lhs, alpha, m().coom_view()(), trans_rhs1, gms_rhs2, trans_rhs2, beta);
}

void COOMatrixWithPartitionedViewSubclass::Mp_StMtM(GenMatrixSlice* gms_lhs, value_type alpha, const GenMatrixSlice& gms_rhs1
	, BLAS_Cpp::Transp trans_rhs1, BLAS_Cpp::Transp trans_rhs2, value_type beta) const
{
	SparseLinAlgPack::Mp_StMtM(gms_lhs, alpha, gms_rhs1, trans_rhs1, m().coom_view()(), trans_rhs2, beta);
}

// Overridden from ConvertToSparseCompressedColumn

size_type COOMatrixWithPartitionedViewSubclass::num_in_column(
	  BLAS_Cpp::Transp					trans
	, size_type							col_offset
	, const IVector::value_type*		col_perm
	, size_type*						num_in_col	) const
{
	return COOM_num_in_column( m().coom_view()(), trans, col_offset
		, col_perm, num_in_col );
}
	
void COOMatrixWithPartitionedViewSubclass::insert_nonzeros(
	  BLAS_Cpp::Transp					trans
	, value_type						alpha
	, size_type							row_offset
	, size_type							col_offset
	, const IVector::value_type*		row_perm
	, const IVector::value_type*		col_perm
	, size_type*						next_nz_in_col
	, FortranTypes::f_dbl_prec*			D_val
	, FortranTypes::f_int*				D_row_i			) const
{
	COOM_insert_nonzeros( m().coom_view()(), trans, alpha, row_offset, col_offset, row_perm
			, col_perm, next_nz_in_col, D_val, D_row_i );
}

value_type COOMatrixWithPartitionedViewSubclass::insert_scaled_nonzeros(
	  BLAS_Cpp::Transp					trans
	, value_type						scaled_max_ele
	, size_type							row_offset
	, size_type							col_offset
	, const IVector::value_type*		row_perm
	, const IVector::value_type*		col_perm
	, size_type*						next_nz_in_col
	, FortranTypes::f_dbl_prec*			D_val
	, FortranTypes::f_int*				D_row_i			) const
{
	return COOM_insert_scaled_nonzeros( m().coom_view()(), trans, scaled_max_ele, row_offset
			, col_offset, row_perm, col_perm, next_nz_in_col, D_val, D_row_i );
}

	// Overridden from MatrixConvertToSparseFortranCompatible

FortranTypes::f_int
COOMatrixWithPartitionedViewSubclass::num_nonzeros( EExtractRegion extract_region ) const
{
	// ToDo: Implement upper and lower triangular regions when needed!
	assert( extract_region == EXTRACT_FULL_MATRIX );

	return this->nz();	
}

void COOMatrixWithPartitionedViewSubclass::coor_extract_nonzeros(
	  EExtractRegion extract_region
	, const FortranTypes::f_int len_Aval
		, FortranTypes::f_dbl_prec Aval[]
	, const FortranTypes::f_int len_Aij
		, FortranTypes::f_int Arow[]
		, FortranTypes::f_int Acol[]
		, const FortranTypes::f_int row_offset
		, const FortranTypes::f_int col_offset
	 ) const
{
	// ToDo: Implement upper and lower triangular regions when needed!
	assert( extract_region == EXTRACT_FULL_MATRIX );


	// Get the permuted view (Partition<> object)
	typedef COOMatrixWithPartitionedView::partitioned_view_type::partition_type
		part_view_t;
	const part_view_t
		part_view = m().coom_view()();

	const FortranTypes::f_int
		nz = part_view.nz();

	// Validate the input
	assert( len_Aval == 0 || (len_Aval == nz && Aval)			);
	assert( len_Aij  == 0 || (len_Aij  == nz && Arow && Acol)	);
	
	// Get overall row and column offsets
	const part_view_t::difference_type
		r_off = part_view.row_offset() + row_offset,
		c_off = part_view.col_offset() + col_offset;

	// Fill the nonzeros and structure.
	part_view_t::const_iterator
		itr		= part_view.begin();
	FortranTypes::f_dbl_prec
		*l_Aval	= Aval;
	FortranTypes::f_int
		*l_Arow	= Arow,
		*l_Acol	= Acol;
	for( ; itr != part_view.end(); ++itr ) {
		if( len_Aval )
			*l_Aval++ = itr->value();
		if( len_Aij ) {
			*l_Arow++ = itr->row_i() + r_off;
			*l_Acol++ = itr->col_j() + c_off;
		}
	}
}

}	// end namespace SparseLinAlgPack
