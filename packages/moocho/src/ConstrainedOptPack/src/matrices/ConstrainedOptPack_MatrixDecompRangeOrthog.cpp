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

#include "ConstrainedOptPack_MatrixDecompRangeOrthog.hpp"
#include "AbstractLinAlgPack_VectorSpace.hpp"
#include "AbstractLinAlgPack_VectorStdOps.hpp"
#include "AbstractLinAlgPack_MatrixSymOpNonsing.hpp"
#include "AbstractLinAlgPack_MatrixOpOut.hpp"
#include "AbstractLinAlgPack_AssertOp.hpp"
#include "AbstractLinAlgPack_LinAlgOpPack.hpp"
#include "Teuchos_TestForException.hpp"

namespace ConstrainedOptPack {

// Constructors/initializers

MatrixDecompRangeOrthog::MatrixDecompRangeOrthog()
{}

MatrixDecompRangeOrthog::MatrixDecompRangeOrthog(
	const C_ptr_t   &C_ptr
	,const D_ptr_t  &D_ptr
	,const S_ptr_t  &S_ptr
	)
{
	this->initialize(C_ptr,D_ptr,S_ptr);
}

void MatrixDecompRangeOrthog::initialize(
	const C_ptr_t   &C_ptr
	,const D_ptr_t  &D_ptr
	,const S_ptr_t  &S_ptr
	)
{
	const char func_name[] = "MatrixDecompRangeOrthog::initialize(...)";
	TEST_FOR_EXCEPTION(
		C_ptr.get() == NULL, std::invalid_argument
		,func_name << " : Error!" );
	TEST_FOR_EXCEPTION(
		D_ptr.get() == NULL, std::invalid_argument
		,func_name << " : Error!" );
	TEST_FOR_EXCEPTION(
		S_ptr.get() == NULL, std::invalid_argument
		,func_name << " : Error!" );
#ifdef ABSTRACT_LIN_ALG_PACK_CHECK_VEC_SPCS
	bool is_compatible = C_ptr->space_rows().is_compatible(D_ptr->space_cols());
	TEST_FOR_EXCEPTION(
		!is_compatible, VectorSpace::IncompatibleVectorSpaces
		,func_name << " : Error, C_ptr->space_rows().is_compatible(D_ptr->space_cols()) == false!" );
	is_compatible = S_ptr->space_cols().is_compatible(D_ptr->space_rows());
	TEST_FOR_EXCEPTION(
		!is_compatible, VectorSpace::IncompatibleVectorSpaces
		,func_name << " : Error, S_ptr->space_cols().is_compatible(D_ptr->space_rows()) == false!" );
#endif	
	C_ptr_ = C_ptr;
	D_ptr_ = D_ptr;
	S_ptr_ = S_ptr;
}

void MatrixDecompRangeOrthog::set_uninitialized()
{
	namespace rcp = MemMngPack;
	C_ptr_ = Teuchos::null;
	D_ptr_ = Teuchos::null;
	S_ptr_ = Teuchos::null;
}

// Overridden from MatrixOp

size_type MatrixDecompRangeOrthog::rows() const
{
	return C_ptr_.get() ? C_ptr_->rows() : 0;
}

size_type MatrixDecompRangeOrthog::cols() const
{
	return C_ptr_.get() ? C_ptr_->cols() : 0;
}

const VectorSpace& MatrixDecompRangeOrthog::space_cols() const
{
	return C_ptr_->space_cols();
}

const VectorSpace& MatrixDecompRangeOrthog::space_rows() const
{
	return C_ptr_->space_rows();
}

std::ostream& MatrixDecompRangeOrthog::output(std::ostream& out) const
{
	out << "This is a " << this->rows() << " x " << this->cols()
		<< " nonsingular matrix (I + D'*D)*C with inverse inv(C')*(I + D*inv(S)*D') where C, D and S are:\n";
	out << "C =\n" << *C_ptr();
	out << "D =\n" << *D_ptr();
	out << "S =\n" << *S_ptr();
	return out;
}

void MatrixDecompRangeOrthog::Vp_StMtV(
	VectorMutable* y, value_type a, BLAS_Cpp::Transp R_trans
	, const Vector& x, value_type b
	) const
{
	using BLAS_Cpp::no_trans;
	using BLAS_Cpp::trans;
	using AbstractLinAlgPack::Vp_StV;
	using AbstractLinAlgPack::Vp_StMtV;
	using LinAlgOpPack::Vp_V;
	using LinAlgOpPack::V_MtV;
	using LinAlgOpPack::Vp_MtV;
	using LinAlgOpPack::V_StMtV;

	assert_initialized("MatrixDecompRangeOrthog::Vp_StMtV(...)");
	AbstractLinAlgPack::Vp_MtV_assert_compatibility(y,*this,R_trans,x);

	const MatrixOpNonsing      &C = *C_ptr_;
	const MatrixOp             &D = *D_ptr_;
	const MatrixSymOpNonsing   &S = *S_ptr_;
	//
	// y = b*y + a*op(R)*x
	//
	VectorSpace::vec_mut_ptr_t               // ToDo: make workspace!
		tI = D.space_rows().create_member(),
		tD = D.space_cols().create_member();
	if(R_trans == no_trans) {
		//
		// y = b*y + a*C*(I + D*D')*x
		//
		// =>
		//
		// tI  = D'*x
		// tD  = x + D*tI
		// y   = b*y + a*C*tD
		//
		V_MtV( tI.get(), D, trans, x );          // tI  = D'*x
		*tD = x;                                 // tD = x + D*tI
		Vp_MtV( tD.get(), D, no_trans, *tI );    // ""
		Vp_StMtV( y, a, C, no_trans, *tD, b );   // y  = b*y + a*C*tD
	}
	else {
		//
		// y = b*y + a*(I + D*D')*C'*x
		//   = b*y + a*C'*x + D*D'*(a*C'*x)
		//
		// =>
		//
		// tD  = a*C'*x
		// tI  = D'*tD
		// y   = b*y + D*tI
		// y  += tD
		//
		V_StMtV( tD.get(), a, C, trans, x );      // tD  = a*C'*x
		V_MtV( tI.get(), D, trans, *tD );         // tI  = D'*tD
		Vp_MtV( y, D, no_trans, *tI, b );         // y   = b*y + D*tI
		Vp_V( y, *tD );                           // y  += tD
	}
}

// Overridden from MatrixOpNonsing

void MatrixDecompRangeOrthog::V_InvMtV(
	VectorMutable* y, BLAS_Cpp::Transp R_trans
	, const Vector& x
	) const
{
	using BLAS_Cpp::no_trans;
	using BLAS_Cpp::trans;
	using AbstractLinAlgPack::Vp_StV;
	using AbstractLinAlgPack::Vp_StMtV;
	using AbstractLinAlgPack::V_InvMtV;
	using LinAlgOpPack::Vp_V;
	using LinAlgOpPack::V_MtV;
	using LinAlgOpPack::V_StMtV;

	assert_initialized("MatrixDecompRangeOrthog::V_InvMtV(...)");
	AbstractLinAlgPack::Vp_MtV_assert_compatibility(y,*this,BLAS_Cpp::trans_not(R_trans),x);

	const MatrixOpNonsing      &C = *C_ptr_;
	const MatrixOp             &D = *D_ptr_;
	const MatrixSymOpNonsing   &S = *S_ptr_;
	//
	// y = inv(op(R))*x
	//
	VectorSpace::vec_mut_ptr_t               // ToDo: make workspace!
		tIa = D.space_rows().create_member(),
		tIb = D.space_rows().create_member(),
		tD  = D.space_cols().create_member();
	if(R_trans == no_trans) {
		//
		// y = (I - D*inv(S)*D')*inv(C)*x
		//   = inv(C)*x - D*inv(S)*D'*inv(C)*x
		//
		// =>
		//
		// y    = inv(C)*x
		// tIa  = D'*y
		// tIb  = inv(S)*tIa
		// y   += -D*tIb
		//
		V_InvMtV( y, C, no_trans, x );            // y    = inv(C)*x
		V_MtV( tIa.get(), D, trans, *y );         // tIa  = D'*y
		V_InvMtV( tIb.get(), S, no_trans, *tIa ); // tIb  = inv(S)*tIa
		Vp_StMtV( y, -1.0, D, no_trans, *tIb );   // y   += -D*tIb
	}
	else {
		//
		// y = inv(C')*(I - D*inv(S)*D')*x
		//
		// =>
		//
		// tIa  = D'*x
		// tIb  = inv(S)*tIa
		// tD   = x
		// tD  += -D*tIb
		// y    = Inv(C')*tD
		//
		V_MtV( tIa.get(), D, trans, x );                // tIa  = D'*x
		V_InvMtV( tIb.get(), S, no_trans, *tIa );       // tIb  = inv(S)*tIa
		*tD = x;                                        // tD   = x
		Vp_StMtV( tD.get(), -1.0, D, no_trans, *tIb );  // tD  += -D*tIb
		V_InvMtV( y, C, trans, *tD );                   // y    = Inv(C')*tD
	}
}

// private

void MatrixDecompRangeOrthog::assert_initialized(const char func_name[]) const
{
	TEST_FOR_EXCEPTION(
		C_ptr_.get() == NULL, std::logic_error
		,func_name << " : Error, Must call initialize(...) first!" );
}

} // end namespace ConstrainedOptPack
