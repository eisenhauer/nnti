// //////////////////////////////////////////////////////////////////////////////////
// MatrixWithOp.h
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

#ifndef ABSTRACT_LINALG_PACK_MATRIX_WITH_OP_H
#define ABSTRACT_LINALG_PACK_MATRIX_WITH_OP_H

#include <iosfwd>

#include "MatrixBase.h"
#include "Range1D.h"
#include "ref_count_ptr.h"

namespace AbstractLinAlgPack {

///
/** Base class for all matrices that support basic matrix operations.
 * 
 * These basic operations are:
 *
 * Level-1 BLAS
 *
 * <tt>mwo_lhs += alpha * op(M_rhs)</tt> (BLAS xAXPY)<br>
 * <tt>mwo_lhs += alpha * op(M_rhs)  * op(P_rhs)</tt><br>
 * <tt>mwo_lhs += alpha * op(P_rhs)  * op(M_rhs)</tt><br>
 * <tt>mwo_lhs += alpha * op(P1_rhs) * op(M_rhs) * op(P2_rhs)</tt><br>
 *
 * <tt>M_lhs += alpha * op(mwo_rhs)</tt> (BLAS xAXPY)<br>
 * <tt>M_lhs += alpha * op(mwo_rhs) * op(P_rhs)</tt><br>
 * <tt>M_lhs += alpha * op(P_rhs)   * op(mwo_rhs)</tt><br>
 * <tt>M_lhs += alpha * op(P1_rhs)  * op(mwo_rhs) * op(P2_rhs)</tt><br>
 *
 * Level-2 BLAS
 *
 * <tt>v_lhs = alpha * op(M_rhs1) * v_rhs2 + beta * v_lhs</tt> (BLAS xGEMV)<br>
 * <tt>v_lhs = alpha * op(M_rhs1) * sv_rhs2 + beta * v_lhs</tt> (BLAS xGEMV)<br>
 * <tt>v_lhs = alpha * op(P_rhs1) * op(M_rhs2) * v_rhs3 + beta * v_lhs</tt><br>
 * <tt>v_lhs = alpha * op(P_rhs1) * op(M_rhs2) * sv_rhs3 + beta * v_lhs</tt><br>
 * <tt>result = v_rhs1' * op(M_rhs2) * v_rhs3</tt><br>
 * <tt>result = sv_rhs1' * op(M_rhs2) * sv_rhs3</tt><br>
 *
 * Level-3 BLAS
 *
 * <tt>mwo_lhs = alpha * op(M_rhs1)   * op(mwo_rhs2) + beta * mwo_lhs</tt> (right) (xGEMM)<br>
 * <tt>mwo_lhs = alpha * op(mwo_rhs1) * op(M_rhs2)   + beta * mwo_lhs</tt> (left)  (xGEMM)<br>
 * <tt>M_lhs   = alpha * op(mwo_rhs1) * op(mwo_rhs2) + beta * M_lhs</tt>           (xGEMM)<br>
 *
 * All of the Level-1, Level-2 and Level-3 BLAS operations have default implementations
 * based on the Level-2 BLAS operation:<br>
 *
 * <tt>v_lhs = alpha * op(M_rhs1) * v_rhs2 + beta * v_lhs</tt> (BLAS xGEMV)<br>
 *
 * The only methods that have to be overridden are \c space_cols(), \c space_rows()
 * and the single \c Vp_StMtV() method shown above.  This is to allow fast prototyping of
 * matrix subclasses and for postponement of writting specialized methods of other time
 * critical operations until later if they are needed.
 *
 * The vector space objects returned by the methods \c space_cols() and \c space_rows() 
 * are specifically bound to this matrix object.  The vector space objects returned should
 * only be considered to be transient and may become invalid if \c this is modified in some
 * significant way (but not through this <tt>%MatrixWithOp</tt> interface obviously).
 *
 * Most of the Level-1 through Level-3 BLAS methods should not be called directly by clients,
 * but instead be called through the \ref MatrixWithOp_func_grp "provided non-member functions".
 * The Level-1 and Level-3 matrix methods of this class have a special protocal in order to deal
 * with the multiple dispatch problem.  In essence, a poor man's multiple dispatch
 * is used to allow each of the participating matrix objects a chance to implement
 * an operation.  In each case, a non-member function must be called by the client
 * which calls the virtual methods on the matrix arguments one at a time.
 * All of the Level-1 and Level-3 matrix methods are implemented for the case where
 * the lhs matrix supports the <tt>MultiVectorMutable</tt> interface.  These matrix
 * operations are then implemented in terms of <tt>AbstractLinAlgPack::Vp_StMtV(...)</tt>
 * which must be implemented for every matrix subclass.  Therefore, any combination of
 * rhs matrices can always be used in any matrix operation as long as a compatible
 * (i.e. vector spaces match up) <tt>MultiVectorMutable</tt> object is used as the
 * lhs matrix argument.
 *
 * Note, this behavior is only implemented by the *nonmember* functions
 * <tt>AbstractLinAlgPack::Mp_StM(...)</tt> or <tt>AbstractLinAlgPack::Mp_StMtM(...)</tt>.
 * All of the default virtual implementations of <tt>Mp_StM(...)</tt> and
 * <tt>Mp_StMtM(...)</tt> return false.
 *
 * This form of multiple dispatach is not ideal in the sense that the first matrix
 * argument that *can* implement the method will do so instead of perhaps the *best*
 * implementation that could be provided by another matrix argument.  Therefore,
 * a matrix subclass should only override one of these matrix methods if it can
 * provide a significantly better implementation than the default.  If a client
 * needs exact control of the implementation of a matrix operation, then they
 * should consider using a ``Strategy'' object.
 *
 * ToDo: Add more detailed documentation for the default Level-1 and Level-3 BLAS
 * methods.
 */
class MatrixWithOp : public virtual MatrixBase {
public:

	/** @name Public types */
	//@{

#ifndef DOXYGEN_COMPILE
	///
	typedef MemMngPack::ref_count_ptr<const MatrixWithOp>    mat_ptr_t;
	///
	typedef MemMngPack::ref_count_ptr<MatrixWithOp>          mat_mut_ptr_t;
#endif

	/// Thrown if a method is not implemented
	class MethodNotImplemented : public std::runtime_error
	{public: MethodNotImplemented(const std::string& what_arg) : std::runtime_error(what_arg) {}};

	/// Thrown if matrices are not compatible
	class IncompatibleMatrices : public std::logic_error
	{public: IncompatibleMatrices(const std::string& what_arg) : std::logic_error(what_arg) {}};

	//@}

	/** @name Minimal modifying methods */
	//@{

	///
	/** M_lhs = 0 :  Zero out the matrix.
	 *
	 * The default implementation throws an exception.  This is not
	 * the best design but it meets some needs.  Any matrix implementation
	 * could implement this method and mimic the behavior (i.e. see the
	 * matrix subclass  <tt>MatrixZero</tt>).  However, only matrices that are
	 * going to be on the lhs (non-const) of a Level-1 or Level-3 BLAS
	 * operation need every implement this method.
	 */
	virtual void zero_out();

	///
	/** M_lhs *= alpha : Multiply a matrix by a scalar.
	 *
	 * The default implementation throws an exception.  This is not
	 * the best design but it meets some needs.  Any matrix implementation
	 * could implement this method and mimic the behavior (i.e.
	 * simply implement the matrix \c M as (<tt>alpha * M</tt>).
	 * This method is only called in a few specialized situations. 
	 */
	virtual void Mt_S( value_type alpha );

	///
	/** M_lhs = mwo_rhs : Virtual assignment operator.
	  *
	  * The default implementation just throws a std::logic_error
	  * exception if it is not assignment to self.  A more specialized
	  * implementation could use this to copy the state to <tt>this</tt> matrix
	  * object from a compatible <tt>M</tt> matrix object.
	  */
	virtual MatrixWithOp& operator=(const MatrixWithOp& mwo_rhs);

	//@}

	/** @name Clone */
	//@{

	///
	/** Clone the non-const matrix object (if supported).
	 *
	 * The primary purpose for this method is to allow a client to capture the
	 * current state of a matrix object and be guaranteed that some other client
	 * will not alter its behavior.  A smart implementation will use reference
	 * counting and lazy evaluation internally and will not actually copy any
	 * large amount of data unless it has to.
	 *
	 * The default implementation returns NULL which is perfectly acceptable.
	 * A matrix object is not required to return a non-NULL value but almost
	 * every good matrix implementation will.
	 */
	virtual mat_mut_ptr_t clone();

	///
	/** Clone the const matrix object (if supported).
	 *
	 * The behavior of this method is the same as for the non-const version
	 * above except it returns a smart pointer to a const matrix object.
	 */
	virtual mat_ptr_t clone() const;

	//@}

	/** @name Output */
	//@{

	///
	/** Virtual output function.
	  *
	  * The default implementaion just extracts rows one at
	  * a time by calling <tt>this->Vp_StMtV()</tt> with 
	  * <tt>EtaVector</tt> objects and then prints the rows.
	  */
	virtual std::ostream& output(std::ostream& out) const;

	//@}

	/** @name Sub-matrix views */
	//@{

	///
	/** Create a transient constant sub-matrix view of this matrix (if supported).
	 *
	 * This view is to be used immediatly and then discarded.
	 *
	 * This method can only be expected to return <tt>return.get() != NULL</tt> if
	 * <tt>this->space_cols().sub_space(row_rng) != NULL</tt> and
	 * <tt>this->space_rows().sub_space(col_rng) != NULL</tt>.
	 *
	 * It is allows for a matrix implementation to return <tt>return.get() == NULL</tt>
	 * for any arbitrary subview.
	 *
	 * The default implementation uses the matrix subclass \c MatrixWithOpSubView
	 * and therefore, can return any arbitrary subview.  More specialized implementations
	 * may want to restrict the subview that can be created somewhat.
	 */
	virtual mat_ptr_t sub_view(const Range1D& row_rng, const Range1D& col_rng) const;
	
	///
	/** Inlined implementation calls <tt>this->sub_view(Range1D(rl,ru),Range1D(cl,cu))</tt>.
	 */
	mat_ptr_t sub_view(
		const index_type& rl, const index_type& ru
		,const index_type& cl, const index_type& cu
		) const;

	//@}
	
	/** @name Permuted views */
	//@{

	///
	/** Create a permuted view: <tt>M_perm = P_row' * M * P_col</tt>.
	 *
	 * @param  P_row  [in] Row permutation.  If <tt>P_row == NULL</tt> then the
	 *                indentity permutation is used.
	 * @param row_part
	 *                [in] Array (length <tt>num_row_part+1</tt>) storing the row indexes
	 *                that may be passed to <tt>return->sub_view(r1,r2,...)</tt>.  If
	 *                <tt>row_part == NULL</tt> then the assumed array is <tt>{ 1, this->rows() }</tt>. 
	 * @param  num_row_part
	 *                [in] Length of the array \c row_part.  If <tt>row_part == NULL</tt> then this
	 *                argument is ignored.
	 * @param  P_col  [in] Column permutation.  If <tt>P_col == NULL</tt> then the
	 *                indentity permutation is used.
	 * @param col_part
	 *                [in] Array (length <tt>num_col_part+1</tt>) storing the column indexes
	 *                that may be passed to <tt>return->sub_view(...,c1,c2)</tt>.  If
	 *                <tt>col_part == NULL</tt> then the assumed array is <tt>{ 1, this->cols() }</tt>. 
	 * @param  num_col_part
	 *                [in] Length of the array \c col_part.  If <tt>col_part == NULL</tt> then this
	 *                argument is ignored.
	 *
	 * Preconditions:<ul>
	 * <li> [<tt>P_row != NULL</tt>] <tt>P_row->space().is_compatible(this->space_cols())</tt>
	 *      (throw <tt>VectorSpace::IncompatibleVectorSpaces</tt>)
	 * <li> [<tt>P_col != NULL</tt>] <tt>P_col->space().is_compatible(this->space_rows())</tt>
	 *      (throw <tt>VectorSpace::IncompatibleVectorSpaces</tt>)
	 * <li> [<tt>row_part != NULL</tt>] <tt>1 <= row_part[i-1] < row_part[i] <= this->rows(), for i = 1...num_row_part</tt>
	 *      (throw ???)
	 * <li> [<tt>col_part != NULL</tt>] <tt>1 <= col_part[i-1] < col_part[i] <= this->cols(), for i = 1...num_col_part</tt>
	 *      (throw ???)
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li> The subviews <tt>return->sub_view(R,C)</tt> should be able to be created efficiently where
	 *      <tt>R = [row_part[kr-1],row_part[kr]-1], for kr = 1...num_row_part</tt> and
	 *      <tt>C = [col_part[kc-1],col_part[kc]-1], for kc = 1...num_col_part</tt>.
	 * </ul>
	 *
	 * The default implementation returns a <tt>MatrixPermAggr</tt> object.
	 */
	virtual mat_ptr_t perm_view(
		const Permutation          *P_row
		,const index_type          row_part[]
		,int                       num_row_part
		,const Permutation         *P_col
		,const index_type          col_part[]
		,int                       num_col_part
		) const;

	///
	/** Reinitialize a permuted view: <tt>M_perm = P_row' * M * P_col</tt>.
	 *
	 * @param  P_row  [in] Same as input to \c perm_view().
	 * @param  row_part
	 *                [in] Same as input to \c perm_view().
	 * @param  num_row_part
	 *                [in] Same as input to \c perm_view().
	 * @param  P_col  [in] Same as input to \c perm_view().
	 * @param  col_part
	 *                [in] Same as input to \c perm_view().
	 * @param  num_col_part
	 *                [in] Same as input to \c perm_view().
	 * @param  perm_view
	 *                [in] Smart pointer to a permuted view
	 *                returned from <tt>this->perm_view()</tt>.
	 *
	 * Preconditions:<ul>
	 * <li> [<tt>P_row != NULL</tt>] <tt>P_row->space().is_compatible(this->space_cols())</tt>
	 *      (throw <tt>VectorSpace::IncompatibleVectorSpaces</tt>)
	 * <li> [<tt>P_col != NULL</tt>] <tt>P_col->space().is_compatible(this->space_rows())</tt>
	 *      (throw <tt>VectorSpace::IncompatibleVectorSpaces</tt>)
	 * <li> [<tt>row_part != NULL</tt>] <tt>1 <= row_part[i-1] < row_part[i] <= this->rows(), for i = 1...num_row_part</tt>
	 *      (throw ???)
	 * <li> [<tt>col_part != NULL</tt>] <tt>1 <= col_part[i-1] < col_part[i] <= this->cols(), for i = 1...num_col_part</tt>
	 *      (throw ???)
	 * </ul>
	 *
	 * The default implementation simply returns <tt>this->perm_view()</tt>
	 */
	virtual mat_ptr_t perm_view_update(
		const Permutation          *P_row
		,const index_type          row_part[]
		,int                       num_row_part
		,const Permutation         *P_col
		,const index_type          col_part[]
		,int                       num_col_part
		,const mat_ptr_t           &perm_view
		) const;

	//@}

	/** @name Level-1 BLAS */
	//@{

	///
	/** mwo_lhs += alpha * op(M_rhs) (BLAS xAXPY).
	 *
	 * The default implementation does nothing returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StM()</tt>
	 */
	virtual bool Mp_StM(
		MatrixWithOp* mwo_lhs, value_type alpha
		, BLAS_Cpp::Transp trans_rhs) const;

	///
	/** M_lhs += alpha * op(mwo_rhs) (BLAS xAXPY).
	 *
	 * The default implementation does nothing and returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StM()</tt>.
	 */
	virtual bool Mp_StM(
		value_type alpha,const MatrixWithOp& M_rhs, BLAS_Cpp::Transp trans_rhs);

	///
	/** mwo_lhs += alpha * op(M_rhs) * op(P_rhs).
	 *
	 * The default implementation does nothing and returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StMtP()</tt>.
	 */
	virtual bool Mp_StMtP(
		MatrixWithOp* mwo_lhs, value_type alpha
		,BLAS_Cpp::Transp M_trans
		,const GenPermMatrixSlice& P_rhs, BLAS_Cpp::Transp P_rhs_trans
		) const;

	///
	/** M_lhs += alpha * op(mwo_rhs) * op(P_rhs).
	 *
	 * The default implementation does nothing and returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StMtP()</tt>.
	 */
	virtual bool Mp_StMtP(
		value_type alpha
		,const MatrixWithOp& mwo_rhs, BLAS_Cpp::Transp M_trans
		,const GenPermMatrixSlice& P_rhs, BLAS_Cpp::Transp P_rhs_trans
		);

	///
	/** mwo_lhs += alpha * op(P_rhs) * op(M_rhs).
	 *
	 * The default implementation does nothing and returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StPtM()</tt>.
	 */
	virtual bool Mp_StPtM(
		MatrixWithOp* mwo_lhs, value_type alpha
		,const GenPermMatrixSlice& P_rhs, BLAS_Cpp::Transp P_rhs_trans
		,BLAS_Cpp::Transp M_trans
		) const;

	///
	/** M_lhs += alpha * op(P_rhs) * op(mwo_rhs).
	 *
	 * The default implementation does nothing and returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StPtM()</tt>.
	 */
	virtual bool Mp_StPtM(
		value_type alpha
		,const GenPermMatrixSlice& P_rhs, BLAS_Cpp::Transp P_rhs_trans
		,const MatrixWithOp& mwo_rhs, BLAS_Cpp::Transp M_trans
		);

	///
	/** mwo_lhs += alpha * op(P_rhs1) * op(M_rhs) * op(P_rhs2).
	 *
	 * The default implementation does nothing and returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StPtMtP()</tt>.
	 */
	virtual bool Mp_StPtMtP(
		MatrixWithOp* mwo_lhs, value_type alpha
		,const GenPermMatrixSlice& P_rhs1, BLAS_Cpp::Transp P_rhs1_trans
		,BLAS_Cpp::Transp M_trans
		,const GenPermMatrixSlice& P_rhs2, BLAS_Cpp::Transp P_rhs2_trans
		) const;

	///
	/** M_lhs += alpha * op(P_rhs1) * op(mwo_rhs) * op(P_rhs2).
	 *
	 * The default implementation does nothing and returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StPtMtP()</tt>.
	 */
	virtual bool Mp_StPtMtP(
		value_type alpha
		,const GenPermMatrixSlice& P_rhs1, BLAS_Cpp::Transp P_rhs1_trans
		,const MatrixWithOp& mwo_rhs, BLAS_Cpp::Transp M_trans
		,const GenPermMatrixSlice& P_rhs2, BLAS_Cpp::Transp P_rhs2_trans
		);

	//		end Level-1 BLAS
	//@}

	/** @name Level-2 BLAS */
	//@{

	/// v_lhs = alpha * op(M_rhs1) * v_rhs2 + beta * v_lhs (BLAS xGEMV)
	virtual void Vp_StMtV(
		VectorWithOpMutable* v_lhs, value_type alpha, BLAS_Cpp::Transp trans_rhs1
		, const VectorWithOp& v_rhs2, value_type beta) const = 0;

	/// v_lhs = alpha * op(M_rhs1) * sv_rhs2 + beta * v_lhs (BLAS xGEMV)
	virtual void Vp_StMtV(
		VectorWithOpMutable* v_lhs, value_type alpha, BLAS_Cpp::Transp trans_rhs1
		, const SpVectorSlice& sv_rhs2, value_type beta) const;

	/// v_lhs = alpha * op(P_rhs1) * op(M_rhs2) * v_rhs3 + beta * v_rhs
	virtual void Vp_StPtMtV(
		VectorWithOpMutable* v_lhs, value_type alpha
		, const GenPermMatrixSlice& P_rhs1, BLAS_Cpp::Transp P_rhs1_trans
		, BLAS_Cpp::Transp M_rhs2_trans
		, const VectorWithOp& v_rhs3, value_type beta) const;

	/// v_lhs = alpha * op(P_rhs1) * op(M_rhs2) * sv_rhs3 + beta * v_rhs
	virtual void Vp_StPtMtV(
		VectorWithOpMutable* v_lhs, value_type alpha
		, const GenPermMatrixSlice& P_rhs1, BLAS_Cpp::Transp P_rhs1_trans
		, BLAS_Cpp::Transp M_rhs2_trans
		, const SpVectorSlice& sv_rhs3, value_type beta) const;

	/// result = v_rhs1' * op(M_rhs2) * v_rhs3
	virtual value_type transVtMtV(
		const VectorWithOp& v_rhs1, BLAS_Cpp::Transp trans_rhs2
		, const VectorWithOp& v_rhs3) const;

	/// result = sv_rhs1' * op(M_rhs2) * sv_rhs3
	virtual value_type transVtMtV(
		const SpVectorSlice& sv_rhs1, BLAS_Cpp::Transp trans_rhs2
		, const SpVectorSlice& sv_rhs3) const;

	///
	/** Perform a specialized rank-2k update of a dense symmetric matrix of the form:
	 *
	 * <tt>symwo_lhs += alpha*op(P1')*op(M)*op(P2) + alpha*op(P2')*op(M')*op(P1) + beta*symwo_lhs</tt>
	 *
	 * The reason that this operation is being classified as a level-2 operation is that the
	 * total flops should be of <tt>O(n^2)</tt> and not <tt>O(n^2*k)</tt>.
	 *
	 * The default implementation is based on <tt>Mp_StMtP(...)</tt> and <tt>Mp_StPtM(...)</tt>.
	 * Of course in situations where this default implemention is inefficient the subclass should
	 * override this method.
	 */
	virtual void syr2k(
		 BLAS_Cpp::Transp M_trans, value_type alpha
		, const GenPermMatrixSlice& P1, BLAS_Cpp::Transp P1_trans
		, const GenPermMatrixSlice& P2, BLAS_Cpp::Transp P2_trans
		, value_type beta, MatrixSymWithOp* symwo_lhs ) const;

	//@}

	/** @name Level-3 BLAS */
	//@{

	///
	/** mwo_lhs = alpha * op(M_rhs1) * op(mwo_rhs2) + beta * mwo_lhs (left) (xGEMM).
	 *
	 * The default implementation does nothing and returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StMtM()</tt>.
	 */
	virtual bool Mp_StMtM(
		MatrixWithOp* mwo_lhs, value_type alpha
		,BLAS_Cpp::Transp trans_rhs1
		,const MatrixWithOp& mwo_rhs2, BLAS_Cpp::Transp trans_rhs2
		,value_type beta ) const;

	///
	/** mwo_lhs = alpha * op(mwo_rhs1) * op(M_rhs2) + beta * mwo_lhs (right) (xGEMM)
	 *
	 * The default implementation does nothing and returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StMtM()</tt>.
	 */
	virtual bool Mp_StMtM(
		MatrixWithOp* mwo_lhs, value_type alpha
		,const MatrixWithOp& mwo_rhs1, BLAS_Cpp::Transp trans_rhs1
		,BLAS_Cpp::Transp trans_rhs2
		,value_type beta ) const;

	///
	/** M_lhs = alpha * op(mwo_rhs1) * op(mwo_rhs2) + beta * mwo_lhs (left) (xGEMM)
	 *
	 * The default implementation does nothing and returns false.
	 *
	 * Warning! A client should never call this method
	 * call this method directly.  Instead, use
	 * <tt>AbstractLinAlgPack::Mp_StMtM()</tt>.
	 */
	virtual bool Mp_StMtM(
		value_type alpha
		,const MatrixWithOp& mwo_rhs1, BLAS_Cpp::Transp trans_rhs1
		,const MatrixWithOp& mwo_rhs2,BLAS_Cpp::Transp trans_rhs2
		,value_type beta );
	
	///
	/** Perform a rank-k update of a dense symmetric matrix of the form:
	 *
	 * <tt>symwo_lhs += alpha*op(M)*op(M') + beta*symwo_lhs</tt>
	 *
	 * The default implementation relays on <tt>dynamic_cast<MultiVectorSymMutable*>(sym_lhs)</tt>
	 * and is based on <tt>this->Vp_StMtV()</tt>.  If <tt>dynamic_cast</tt> returns <tt>NULL</tt>, the this
	 * default implementation throws <tt>MethodNotImplemented</tt>.  Of course in situations where this
	 * default implemention is inefficient the subclass should override this method.
	 */
	virtual void syrk(
		 BLAS_Cpp::Transp M_trans, value_type alpha
		, value_type beta, MatrixSymWithOp* sym_lhs ) const;

	//		end Level-3 BLAS
	//@}

};	// end class MatrixWithOp

// ////////////////////////////////////////////////////////////////////////////////////////////////
/** \defgroup MatrixWithOp_func_grp MatrixWithOp non-member functions that call virtual functions.
  *
  * These allow nonmember functions to act like virtual functions.  If any of these methods
  * on the subclasses are not implemented for a particular set of matrix arguments, then the
  * exception <tt>AbstractLinAlgPack::MatrixWithOp::MethodNotImplemented</tt> is thrown.
  * This will not happen as long as a compatible (vector spaces are compatible) lhs matrix
  * argument is passed in and <tt>dynamic_cast<MultiVectorMatrix*>(lhs) != NULL</tt>.
  */
//@{

/** @name Level-1 BLAS */
//@{

//
/** mwo_lhs *= alpha.
 *
 * If <tt>alpha == 0.0</tt> then <tt>mwo_lhs->zero_out()</tt> will be called,
 * otherwise <tt>mwo_lhs->Mt_S(alpha)</tt> will be called.  If <tt>alpha == 1.0</tt>
 * then nothing is done.
 */
void Mt_S( MatrixWithOp* mwo_lhs, value_type alpha );

//
/** mwo_lhs += alpha * op(M_rhs) (BLAS xAXPY).
 *
 * Entry point for (poor man's) multiple dispatch.
 *
 * This method first calls <tt>M_rhs->Mp_StM(mwo_lhs,alpha,trans_rhs)</tt>
 * to give the rhs argument a chance to implement the operation.  If
 * <tt>M_rhs->Mp_StM(...)</tt> returns false, then
 * <tt>mwo_lhs->Mp_StM(alpha,*this,trans_rhs)</tt>
 * is called to give the lhs matrix argument a chance to implement the method.
 * If <tt>mwo_lhs->Mp_StM(...)</tt> returns false, then
 * an attempt to perform a dynamic cast the lhs matrix argument to
 * <tt>MultiVectorMutable</tt> is attempted.  If this cast failes,
 * then an exception is thrown.
 */
void Mp_StM(
	MatrixWithOp* mwo_lhs, value_type alpha, const MatrixWithOp& M_rhs
	, BLAS_Cpp::Transp trans_rhs);

///
/** mwo_lhs += alpha * op(M_rhs) * op(P_rhs).
 *
 * Entry point for (poor man's) multiple dispatch.
 * 
 * ToDo: Finish documentation!
 */
void Mp_StMtP(
	MatrixWithOp* mwo_lhs, value_type alpha
	, const MatrixWithOp& M_rhs, BLAS_Cpp::Transp M_trans
	, const GenPermMatrixSlice& P_rhs, BLAS_Cpp::Transp P_rhs_trans
	);

///
/** mwo_lhs += alpha * op(P) * op(M_rhs).
 *
 * Entry point for (poor man's) multiple dispatch.
 * 
 * ToDo: Finish documentation!
 */
void Mp_StPtM(
	MatrixWithOp* mwo_lhs, value_type alpha
	, const GenPermMatrixSlice& P_rhs, BLAS_Cpp::Transp P_rhs_trans
	, const MatrixWithOp& M_rhs, BLAS_Cpp::Transp M_trans
	);

///
/** mwo_lhs += alpha * op(P_rhs1) * op(M_rhs) * op(P_rhs2).
 *
 * Entry point for (poor man's) multiple dispatch.
 * 
 * ToDo: Finish documentation!
 */
void Mp_StPtMtP(
	MatrixWithOp* mwo_lhs, value_type alpha
	, const GenPermMatrixSlice& P_rhs1, BLAS_Cpp::Transp P_rhs1_trans
	, const MatrixWithOp& M_rhs, BLAS_Cpp::Transp trans_rhs
	, const GenPermMatrixSlice& P_rhs2, BLAS_Cpp::Transp P_rhs2_trans
	);

//		end Level-1 BLAS
//@}

/** @name Level-2 BLAS */
//@{

/// v_lhs = alpha * op(M_rhs1) * v_rhs2 + beta * v_lhs (BLAS xGEMV)
inline void Vp_StMtV(
	VectorWithOpMutable* v_lhs, value_type alpha, const MatrixWithOp& M_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const VectorWithOp& v_rhs2, value_type beta = 1.0)
{
	M_rhs1.Vp_StMtV(v_lhs,alpha,trans_rhs1,v_rhs2,beta);
}

/// v_lhs = alpha * op(M_rhs1) * sv_rhs2 + beta * v_lhs (BLAS xGEMV)
inline void Vp_StMtV(
	VectorWithOpMutable* v_lhs, value_type alpha, const MatrixWithOp& M_rhs1
	, BLAS_Cpp::Transp trans_rhs1, const SpVectorSlice& sv_rhs2, value_type beta = 1.0)
{
	M_rhs1.Vp_StMtV(v_lhs,alpha,trans_rhs1,sv_rhs2,beta);
}

/// v_lhs = alpha * op(P_rhs1) * op(M_rhs2) * v_rhs3 + beta * v_rhs
inline void Vp_StPtMtV(
	VectorWithOpMutable* v_lhs, value_type alpha
	, const GenPermMatrixSlice& P_rhs1, BLAS_Cpp::Transp P_rhs1_trans
	, const MatrixWithOp& M_rhs2, BLAS_Cpp::Transp M_rhs2_trans
	, const VectorWithOp& v_rhs3, value_type beta = 1.0) 
{
	M_rhs2.Vp_StPtMtV(v_lhs,alpha,P_rhs1,P_rhs1_trans,M_rhs2_trans,v_rhs3,beta);
}

/// v_lhs = alpha * op(P_rhs1) * op(M_rhs2) * sv_rhs3 + beta * v_rhs
inline void Vp_StPtMtV(
	VectorWithOpMutable* v_lhs, value_type alpha
	, const GenPermMatrixSlice& P_rhs1, BLAS_Cpp::Transp P_rhs1_trans
	, const MatrixWithOp& M_rhs2, BLAS_Cpp::Transp M_rhs2_trans
	, const SpVectorSlice& sv_rhs3, value_type beta = 1.0)
{
	M_rhs2.Vp_StPtMtV(v_lhs,alpha,P_rhs1,P_rhs1_trans,M_rhs2_trans,sv_rhs3,beta);
}

/// result = v_rhs1' * op(M_rhs2) * v_rhs3
inline value_type transVtMtV(
	const VectorWithOp& v_rhs1, const MatrixWithOp& M_rhs2
	, BLAS_Cpp::Transp trans_rhs2, const VectorWithOp& v_rhs3)
{
	return M_rhs2.transVtMtV(v_rhs1,trans_rhs2,v_rhs3);
}

/// result = sv_rhs1' * op(M_rhs2) * sv_rhs3
inline value_type transVtMtV(
	const SpVectorSlice& sv_rhs1, const MatrixWithOp& M_rhs2
	, BLAS_Cpp::Transp trans_rhs2, const SpVectorSlice& sv_rhs3)
{
	return M_rhs2.transVtMtV(sv_rhs1,trans_rhs2,sv_rhs3);
}

//		end Level-2 BLAS
//@}

/** @name Level-3 BLAS */
//@{

///
/** mwo_lhs = alpha * op(mwo_rhs1) * op(mwo_rhs2) + beta * mwo_lhs (right) (xGEMM).
 *
 * This method first calls <tt>mwo_rhs1.Mp_StMtM(...)</tt> to perform the opeation.
 * If <tt>mwo_rhs1.Mp_StMtM(...)</tt> returns false, then <tt>mwo_rhs2.Mp_StMtM(...)</tt>
 * is called.  If <tt>mwo_rhs2.Mp_StMtM(...)</tt> returns false, then
 * <tt>mwo_lhs.Mp_StMtM(...)</tt> is called.
 *
 * As a last resort, the function
 * attempts to cast <tt>dynamic_cast<MultiVectorMutable*>(mwo_lhs)</tt>.
 * If this dynamic cast fails, the this function throws an exception.
 * Otherwise, the operation is implemented in terms of <tt>Vp_StMtV()</tt>.
 */
void Mp_StMtM(
	MatrixWithOp* mwo_lhs, value_type alpha
	,const MatrixWithOp& mwo_rhs1, BLAS_Cpp::Transp trans_rhs1
	,const MatrixWithOp& mwo_rhs2, BLAS_Cpp::Transp trans_rhs2
	, value_type beta = 1.0 );

//		end Level-3 BLAS
//@}

//		end non-member functions
//@}

// //////////////////////////////////////////////////
// Inlined methods for MatrixWithOp

inline
MatrixWithOp::mat_ptr_t
MatrixWithOp::sub_view(
	const index_type& rl, const index_type& ru
	,const index_type& cl, const index_type& cu
	) const
{
	return this->sub_view(Range1D(rl,ru),Range1D(cl,cu));
}

}	// end namespace AbstractLinAlgPack

#endif	// ABSTRACT_LINALG_PACK_MATRIX_WITH_OP_H
