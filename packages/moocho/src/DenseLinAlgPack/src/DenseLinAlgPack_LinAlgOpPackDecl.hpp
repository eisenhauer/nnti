// //////////////////////////////////////////////////////////////////////////////////
// LinAlgOpPackDecl.hpp
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

#ifndef LIN_ALG_OP_PACK_DECL_H
#define LIN_ALG_OP_PACK_DECL_H

#include "DVectorOp.hpp"
#include "DMatrixOp.hpp"

namespace LinAlgOpPack {

typedef DenseLinAlgPack::value_type value_type;

using DenseLinAlgPack::DVector;
using DenseLinAlgPack::DVectorSlice;
using DenseLinAlgPack::DMatrix;
using DenseLinAlgPack::DMatrixSlice;

using DenseLinAlgPack::Vt_S;
using DenseLinAlgPack::Vp_StV;
using DenseLinAlgPack::Vp_StMtV;
using DenseLinAlgPack::V_InvMtV;
using DenseLinAlgPack::Mp_StM;
using DenseLinAlgPack::Mp_StMtM;
using DenseLinAlgPack::M_StInvMtM;

// //////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////
/** @name Default Linear Algebra implementation operations.
  *
  * These are template functions that can be used to perform simpler
  * linear algebra operations given more elaborate ones.  The idea is that
  * for each combination of vector and matrix types, the BLAS like operations
  * must be provided and then these template functions provide related
  * linear algebra operations.  The user can override these default implementations
  * by defining the exact functions himself.
  *
  * Warning\\
  * In general it is not allowed for the lhs argument to be used in the rhs expression.
  * Concidering aliasing would have make the operations much more complicated.
  * So unless you are sure that it is okay, do not use a vector or matrix object
  * in both the lhs and rhs expressions.
  *
  * The nameing covension for these functions is the same as for the linear algebra
  * functions for #DVectorSlice# and #DMatrixSlice#.
  */
//@{

// //////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////
/** @name Level 1 BLAS for Vectors
  *
  * For these functions to work for the type V the following function must
  * be defined:
  *
  * // vs_lhs += alpha * V_rhs	\\
  * void Vp_StV(DVectorSlice* vs_lhs, value_type alpha, const V& V_rhs);
  *
  * The rest of these level 1 BLAS functions implement the variations.
  */
//@{

// //////////////////////////////////////////////////////////////////////////////
/** @name += operations 
  */
//@{

///
/** vs_lhs += V_rhs.
  *
  * Calls: #Vp_StV(vs_lhs,1.0,V_rhs);#
  */
template <class V>
void Vp_V(DVectorSlice* vs_lhs, const V& V_rhs);

//		end += operations
//@}
 
// //////////////////////////////////////////////////////////////////////////////
/** @name = operations with DVector as lhs
  */
//@{

///
/** v_lhs = V_rhs.
  *
  * Calls: #Vp_V(&(*v_lhs)(),V_rhs);#
  */
template <class V>
void assign(DVector* v_lhs, const V& V_rhs);

///
/** v_lhs = alpha * V_rhs.
  *
  * Calls: #Vp_StV(&(*v_lhs)(),alpha,V_rhs);#
  */
template <class V>
void V_StV(DVector* v_lhs, value_type alpha, const V& V_rhs);

///
/** v_lhs = - V_rhs.
  *
  * Calls: #V_StV(&(*v_lhs)(),-1.0,V_rhs);#
  */
template <class V>
void V_mV(DVector* v_lhs, const V& V_rhs);

/// 
/** v_lhs = V1_rhs1 + V2_rhs2.
  *
  * Calls: #Vp_V(&(*v_lhs)(),V1_rhs1); Vp_V(&(*v_lhs)(),V1_rhs2);#
  */
template <class V1, class V2>
void V_VpV(DVector* v_lhs, const V1& V1_rhs1, const V2& V2_rhs2);

///
/** v_lhs = V_rhs1 - V_rhs2.
  *
  * Calls: #Vp_V(&(*v_lhs)(),V1_rhs1); Vp_StV(&(*v_lhs)(),-1.0,V2_rhs2);#
  */
template <class V1, class V2>
void V_VmV(DVector* v_lhs, const V1& V1_rhs1, const V2& V2_rhs2);
///
/** v_lhs = alpha * V_rhs1 + vs_rhs2.
  *
  * Calls: #Vp_StV(&(*v_lhs)(),alpha,V_rhs1);#
  */
template <class V>
void V_StVpV(DVector* v_lhs, value_type alpha, const V& V_rhs1
	, const DVectorSlice& vs_rhs2);

//		end = operations with DVector as lhs
//@}

// //////////////////////////////////////////////////////////////////////////////
/** @name = operations with DVectorSlice as lhs
  */
//@{

///
/** vs_lhs = V_rhs.
  *
  * Calls: #Vp_V(vs_lhs,V_rhs);#
  */
template <class V>
void assign(DVectorSlice* vs_lhs, const V& V_rhs);

///
/** vs_lhs = alpha * V_rhs.
  *
  * Calls: #Vp_StV(vs_lhs,alpha,V_rhs);#
  */
template <class V>
void V_StV(DVectorSlice* vs_lhs, value_type alpha, const V& V_rhs);

///
/** vs_lhs = - V_rhs.
  *
  * Calls: #V_StV(vs_lhs,-1.0,V_rhs);#
  */
template <class V>
void V_mV(DVectorSlice* vs_lhs, const V& V_rhs);

/// 
/** vs_lhs = V1_rhs1 + V2_rhs2.
  *
  * Calls: #Vp_V(vs_lhs,V1_rhs1); Vp_V(vs_lhs,V1_rhs2);#
  */
template <class V1, class V2>
void V_VpV(DVectorSlice* vs_lhs, const V1& V1_rhs1, const V2& V2_rhs2);

///
/** vs_lhs = V_rhs1 - V_rhs2.
  *
  * Calls: #Vp_V(vs_lhs,V1_rhs1); Vp_StV(vs_lhs,-1.0,V2_rhs2);#
  */
template <class V1, class V2>
void V_VmV(DVectorSlice* vs_lhs, const V1& V1_rhs1, const V2& V2_rhs2);

///
/** vs_lhs = alpha * V_rhs1 + vs_rhs2.
  *
  * Calls: #Vp_StV(vs_lhs,alpha,V_rhs1);#
  */
template <class V>
void V_StVpV(DVectorSlice* vs_lhs, value_type alpha, const V& V_rhs1
	, const DVectorSlice& vs_rhs2);

//		end = operations with DVectorSlice as lhs
//@}

//		end Level 1 BLAS for Vectors
//@}

// //////////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////////
/** @name Level 1 BLAS for Matrices
  *
  * For these functions to work for the type M the following function must
  * be defined:
  *
  * // gms_lhs += alpha * op(M_rhs)	\\
  * void Mp_StM(DMatrixSlice* vs_lhs, value_type alpha, const V& V_rhs, BLAS_Cpp::Transp);
  *
  * The rest of these level 1 BLAS functions implement the variations.
  */
//@{

// //////////////////////////////////////////////////////////////////////////////
/** @name += operations 
  */
//@{

///
/** gms_lhs += op(M_rhs).
  *
  * Calls: #Mp_StM(gms_lhs,1.0,M_rhs,trans_rhs);#
  */
template <class M>
void Mp_M(DMatrixSlice* gms_lhs, const M& M_rhs, BLAS_Cpp::Transp trans_rhs);

//		end += operations
//@}
 
// //////////////////////////////////////////////////////////////////////////////
/** @name = operations with DMatrix as lhs
  */
//@{

///
/** gm_lhs = op(M_rhs).
  *
  * Calls: #Mp_M(&(*gm_lhs)(),M_rhs,trans_rhs);#
  */
template <class M>
void assign(DMatrix* gm_lhs, const M& M_rhs, BLAS_Cpp::Transp trans_rhs);

///
/** gm_lhs = alpha * M_rhs.
  *
  * Calls: #Mp_StM(&(*gm_lhs)(),alpha,M_rhs,trans_rhs);#
  */
template <class M>
void M_StM(DMatrix* v_lhs, value_type alpha, const M& M_rhs
	, BLAS_Cpp::Transp trans_rhs);

///
/** gm_lhs = - op(M_rhs).
  *
  * Calls: #M_StM(&(*gm_lhs)(),-1.0,M_rhs,trans_rhs);#
  */
template <class M>
void M_mM(DMatrix* gm_lhs, const M& M_rhs, BLAS_Cpp::Transp trans_rhs) ;

/// 
/** gm_lhs = op(M1_rhs1) + op(M2_rhs2).
  *
  * Calls: #Mp_M(&(*gm_lhs)(),M1_rhs1,trans_rhs1); Mp_M(&(*gm_lhs)(),M1_rhs2,trans_rhs2);#
  */
template <class M1, class M2>
void M_MpM(DMatrix* gm_lhs, const M1& M1_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2);

///
/** gm_lhs = op(M_rhs1) - op(M_rhs2).
  *
  * Calls: #Mp_M(&(*gm_lhs)(),M1_rhs1,trans_rhs1); Mp_StM(&(*gm_lhs)(),-1.0,M2_rhs2,trans_rhs2);#
  */
template <class M1, class M2>
void M_MmM(DMatrix* gm_lhs, const M1& M1_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2);

///
/** gm_lhs = alpha * op(M_rhs1) + op(gms_rhs2).
  *
  * Calls: #Mp_StM(&(*gm_lhs)(),alpha,M_rhs1,trans_rhs1);#
  */
template <class M>
void M_StMpM(DMatrix* gm_lhs, value_type alpha, const M& M_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const DMatrixSlice& gms_rhs2, BLAS_Cpp::Transp trans_rhs2);

//		end = operations with DMatrix as lhs
//@}

// //////////////////////////////////////////////////////////////////////////////
/** @name = operations with DMatrixSlice as lhs
  */
//@{

///
/** gms_lhs = op(M_rhs).
  *
  * Calls: #Mp_M(gms_lhs,M_rhs,trans_rhs);#
  */
template <class M>
void assign(DMatrixSlice* gms_lhs, const M& M_rhs, BLAS_Cpp::Transp trans_rhs);

///
/** gm_lhs = alpha * M_rhs.
  *
  * Calls: #Mp_StM(&(*gm_lhs)(),alpha,M_rhs,trans_rhs);#
  */
template <class M>
void M_StM(DMatrixSlice* gms_lhs, value_type alpha, const M& M_rhs
	, BLAS_Cpp::Transp trans_rhs);

///
/** gm_lhs = - op(M_rhs).
  *
  * Calls: #M_StM(&(*gm_lhs)(),-1.0,M_rhs,trans_rhs);#
  */
template <class M>
void M_mM(DMatrixSlice* gms_lhs, const M& M_rhs, BLAS_Cpp::Transp trans_rhs) ;

/// 
/** gms_lhs = op(M1_rhs1) + op(M2_rhs2).
  *
  * Calls: #Mp_M(gms_lhs,M1_rhs1,trans_rhs1); Mp_M(gms_lhs,M1_rhs2,trans_rhs2);#
  */
template <class M1, class M2>
void M_MpM(DMatrixSlice* gms_lhs, const M1& M1_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2);

///
/** gms_lhs = op(M_rhs1) - op(M_rhs2).
  *
  * Calls: #Mp_M(gms_lhs,M1_rhs1,trans_rhs1); Mp_StM(gms_lhs,-1.0,M2_rhs2,trans_rhs2);#
  */
template <class M1, class M2>
void M_MmM(DMatrixSlice* gms_lhs, const M1& M1_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2);

///
/** gms_lhs = alpha * op(M_rhs1) + op(gms_rhs2).
  *
  * Calls: #Mp_StM(gms_lhs,alpha,M_rhs1,trans_rhs1);#
  */
template <class M>
void M_StMpM(DMatrixSlice* gms_lhs, value_type alpha, const M& M_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const DMatrixSlice& gms_rhs2, BLAS_Cpp::Transp trans_rhs2);

//		end = operations with DMatrixSlice as lhs
//@}

//		end Level 1 BLAS for Matrices
//@}

// //////////////////////////////////////////////////////////////////////////////
// /////////////////////////////////////////////////////////////////////// 
/** @name Level 2 BLAS
  *
  * These operations implement variations on the Level-2 BLAS operation:\\
  *
  * vs_lhs = alpha * op(M_rhs1) * V_rhs2 + beta * vs_lhs
  */
//@{

// //////////////////////////////////////////////////////////////////////////////
/** @name += operations
  */
//@{

///
/** vs_lhs += op(M_rhs1) * V_rhs2.
  *
  * Calls: #Vp_StMtV(vs_lhs,1.0,M_rhs1,trans_rhs1,V_rhs2);#
  */
template <class M, class V>
void Vp_MtV(DVectorSlice* vs_lhs, const M& M_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const V& V_rhs2);

///
/** vs_lhs = op(M_rhs1) * V_rhs2 + beta * vs_lhs.
  *
  * Calls: #Vp_StMtV(vs_lhs,1.0,M_rhs1,trans_rhs1,V_rhs2,beta);#
  */
template <class M, class V>
void Vp_MtV(DVectorSlice* vs_lhs, const M& M_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const V& V_rhs2, value_type beta);

//		end += operations
//@}
 
// //////////////////////////////////////////////////////////////////////////////
/** @name = operations with DVector as lhs
  */
//@{

///
/** v_lhs = alpha * op(M_rhs1) * V_rhs2.
  *
  * Calls: #Vp_StMtV(&(*v_lhs)(),alpha,M_rhs1,trans_rhs1,V_rhs2);#
  */
template <class M, class V>
void V_StMtV(DVector* v_lhs, value_type alpha, const M& M_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const V& V_rhs2);

///
/** v_lhs = op(M_rhs1) * V_rhs2.
  *
  * Calls: #Vp_MtV(&(*v_lhs)(),M_rhs1,trans_rhs1,V_rhs2);#
  */
template <class M, class V>
void V_MtV(DVector* v_lhs, const M& M_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const V& V_rhs2);

//		end = operations with DVector as lhs
//@}

// //////////////////////////////////////////////////////////////////////////////
/** @name = operations with DVectorSlice as lhs
  */
//@{

///
/** vs_lhs = alpha * op(M_rhs1) * V_rhs2.
  *
  * Calls: #Vp_StMtV(vs_lhs,alpha,M_rhs1,trans_rhs1,V_rhs2);#
  */
template <class M, class V>
void V_StMtV(DVectorSlice* vs_lhs, value_type alpha, const M& M_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const V& V_rhs2);

///
/** vs_lhs = op(M_rhs1) * V_rhs2.
  *
  * Calls: #Vp_MtV(vs_lhs,M_rhs1,trans_rhs1,V_rhs2);#
  */
template <class M, class V>
void V_MtV(DVectorSlice* vs_lhs, const M& M_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const V& V_rhs2);

//		end = operations with DVectorSlice as lhs
//@}

//		end Level 2 BLAS
//@}

// //////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////
/** @name Level 3 BLAS
  *
  * These operations are based on the Level-3 BLAS operation:
  *
  * gms_lhs = alpha * op(M1_rhs1) * op(M2_rhs2) + beta * gms_lhs
  */
//@{

// //////////////////////////////////////////////////////////////////////////////
/** @name += operations 
  */
//@{

///
/** gms_lhs += op(M1_rhs1) * op(M2_rhs2).
  *
  * Calls: #Mp_StMtM(gms_lhs,1.0,M1_rhs1,trans_rhs1,M2_rhs2,trans_rhs2);#
  */
template <class M1, class M2>
void Mp_MtM(DMatrixSlice* gms_lhs, const M1& M1_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2);

///
/** gms_lhs = op(M1_rhs1) * op(M2_rhs2) + beta * gms_rhs.
  *
  * Calls: #Mp_StMtM(gms_lhs,1.0,M1_rhs1,trans_rhs1,M2_rhs2,trans_rhs2,beta);#
  */
template <class M1, class M2>
void Mp_MtM(DMatrixSlice* gms_lhs, const M1& M1_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2, value_type beta);

//		end += operations
//@}
 
// //////////////////////////////////////////////////////////////////////////////
/** @name = operations with DMatrix as lhs
  */
//@{

///
/** gm_lhs = alpha * op(M1_rhs1) * op(M2_rhs2).
  *
  * Calls: #Mp_StMtM(&(*gm_lhs)(),alpha,M1_rhs1,trans_rhs1,M2_rhs2,trans_rhs2);#
  */
template <class M1, class M2>
void M_StMtM(DMatrix* gm_lhs, value_type alpha, const M1& M1_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2);

///
/** gm_lhs = op(M1_rhs1) * op(M2_rhs2).
  *
  * Calls: #Mp_MtM(&(*gm_lhs)(),M1_rhs1,trans_rhs1,M2_rhs2,trans_rhs2);#
  */
template <class M1, class M2>
void M_MtM(DMatrix* gm_lhs, const M1& M1_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2);

//		end = operations with DMatrix as lhs
//@}

// //////////////////////////////////////////////////////////////////////////////
/** @name = operations with DMatrixSlice as lhs
  */
//@{

///
/** gms_lhs = alpha * op(M1_rhs1) * op(M2_rhs2).
  *
  * Calls: #Mp_StMtM(gms_lhs,alpha,M1_rhs1,trans_rhs1,M2_rhs2,trans_rhs2);#
  */
template <class M1, class M2>
void M_StMtM(DMatrixSlice* gms_lhs, value_type alpha, const M1& M1_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2);

///
/** gms_lhs = op(M1_rhs1) * op(M2_rhs2).
  *
  * Calls: #Mp_MtM(gms_lhs,M1_rhs1,trans_rhs1,M2_rhs2,trans_rhs2);#
  */
template <class M1, class M2>
void M_MtM(DMatrixSlice* gms_lhs, const M1& M1_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2);
//		end = operations with DMatrixSlice as lhs
//@}

//		end Level 3 BLAS
//@}

//		end Default Linear Algebra Implementations
//@}

// ///////////////////////////////////////////////////////////////////////////
// ///////////////////////////////////////////////////////////////////////////
// Inline definitions

// ///////////////////////////////////////////////////////////////////////////////
// Level 1 BLAS for Vectors

// vs_lhs += V_rhs.
template <class V>
inline
void Vp_V(DVectorSlice* vs_lhs, const V& V_rhs) {
	Vp_StV(vs_lhs,1.0,V_rhs);
}

// v_lhs = - V_rhs.
template <class V>
inline
void V_mV(DVector* v_lhs, const V& V_rhs) {
	V_StV(v_lhs,-1.0,V_rhs);
}

// vs_lhs = - V_rhs.
template <class V>
inline
void V_mV(DVectorSlice* vs_lhs, const V& V_rhs) {
	V_StV(vs_lhs,-1.0,V_rhs);
}

// ///////////////////////////////////////////////////////////////////////////////
// Level 1 BLAS for Matrices

// gms_lhs += op(M_rhs).
template <class M>
inline
void Mp_M(DMatrixSlice* gms_lhs, const M& M_rhs, BLAS_Cpp::Transp trans_rhs) {
	Mp_StM(gms_lhs,1.0,M_rhs,trans_rhs);
}

// gm_lhs = - op(M_rhs).
template <class M>
inline
void M_mM(DMatrix* gm_lhs, const M& M_rhs, BLAS_Cpp::Transp trans_rhs) {
	M_StM(gm_lhs,-1.0,M_rhs,trans_rhs);
}

// gms_lhs = - op(M_rhs).
template <class M>
inline
void M_mM(DMatrixSlice* gms_lhs, const M& M_rhs, BLAS_Cpp::Transp trans_rhs) {
	M_StM(gms_lhs,-1.0,M_rhs,trans_rhs);
}

// /////////////////////////////////////////////////////////////////////// 
// Level 2 BLAS

// vs_lhs += op(M_rhs1) * V_rhs2.
template <class M, class V>
inline
void Vp_MtV(DVectorSlice* vs_lhs, const M& M_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const V& V_rhs2)
{
	Vp_StMtV(vs_lhs,1.0,M_rhs1,trans_rhs1,V_rhs2);
}

// vs_lhs = op(M_rhs1) * V_rhs2 + beta * vs_lhs.
template <class M, class V>
inline
void Vp_MtV(DVectorSlice* vs_lhs, const M& M_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const V& V_rhs2, value_type beta)
{
	Vp_StMtV(vs_lhs,1.0,M_rhs1,trans_rhs1,V_rhs2,beta);
}

// //////////////////////////////////////////////////////////////////////////////
// Level 3 BLAS

// gms_lhs += op(M1_rhs1) * op(M2_rhs2).
template <class M1, class M2>
inline
void Mp_MtM(DMatrixSlice* gms_lhs, const M1& M1_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2)
{
	Mp_StMtM(gms_lhs,1.0,M1_rhs1,trans_rhs1,M2_rhs2,trans_rhs2);
}

// gms_lhs = op(M1_rhs1) * op(M2_rhs2) + beta * gms_rhs.
template <class M1, class M2>
inline
void Mp_MtM(DMatrixSlice* gms_lhs, const M1& M1_rhs1, BLAS_Cpp::Transp trans_rhs1
	, const M2& M2_rhs2, BLAS_Cpp::Transp trans_rhs2, value_type beta)
{
	Mp_StMtM(gms_lhs,1.0,M1_rhs1,trans_rhs1,M2_rhs2,trans_rhs2,beta);
}


} // end namespace LinAlgOpPack


#endif	// LIN_ALG_OP_PACK_DECL_H
