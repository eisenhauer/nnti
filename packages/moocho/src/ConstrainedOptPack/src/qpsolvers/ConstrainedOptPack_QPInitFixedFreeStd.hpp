// //////////////////////////////////////////////////////////////
// QPInitFixedFreeStd.hpp
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

#ifndef QP_INIT_FIXED_FREE_STD_H
#define QP_INIT_FIXED_FREE_STD_H

#include "QPSchur.hpp"

namespace ConstrainedOptimizationPack {
namespace QPSchurPack {

///
/** General (and flexible) implementation class for a QPSchur QP
 * problem.
 *
 * The basic idea of this class is to just build the QP from its
 * various components in a way that is easy and flexible for the
 * client.  The class will also do consistency testing if asked
 * to.
 */
class QPInitFixedFreeStd : public QP {
public:

	/// Construct uninitialized
	QPInitFixedFreeStd();

	///
	/** Initialize.
	 *
	 * The pointers and references to the objects pointed to by the
	 * arguments to this function must not be modified by the caller.
	 * Copies of these objects are not made internally so these
	 * objects must remain valid while this object is in use.
	 *
	 * If the sizes of the arguments do not match up or some consistency
	 * test fails then exceptions may be thrown with (hopefully) helpful
	 * messages.
	 *
	 *	@param	g 	[in] vector (size <tt>n</tt>): objective gradient
	 *	@param	G	[in] matrix (size <tt>n x n</tt>): objective Hessian
	 *	@param	A	[in] matrix (size <tt>n x m</tt>): full rank equality constraints
	 *					in <tt>Ko</tt>.  If <tt>A==NULL</tt> then there are no equality constraints
	 *					in <tt>Ko</tt> and m will be zero.
	 *	@param	n_R	[in] number of initially free variables
	 *	@param	i_x_free
	 *				[in] array (size <tt>n_R</tt>): <tt>i_x_free[l-1], l = 1...n_R</tt> defines
	 *					the matrix <tt>Q_R</tt> as:<br>
	 *					<tt>Q_R(:,l) = e(i_x_free[l-1]), l = 1...n_R</tt><br>
	 *					The ordering of these indices is significant.  It is allowed
	 *                 for <tt>i_x_free == NULL</tt> in which case it will be
	 *                 considered to be identity.
	 *	@param	i_x_fixed
	 *				[in] array (size <tt>n_X = n - n_R</tt>):
	 *					<tt>i_x_fixed[l-1], l = 1...n_X</tt> defines the matrix <tt>Q_X</tt> as:<br>
	 *					<tt>Q_X(:,l) = e(i_x_fixed[l-1]), l = 1...n_X</tt><br>
	 *					The ordering of these indices is significant.
	 *	@param	bnd_fixed
	 *				[in] array (size <tt>n_X = n - n_R</tt>):
 	 *					<tt>bnd_fixed[l-1], l = 1...n_X</tt> defines the initial active set as:<br>
	 \begin{verbatim}
	                    / LOWER : b_X(l) = xL(i_x_fixed[l-1])
	  bnd_fixed[l-1] = |  UPPER : b_X(l) = xU(i_x_fixed[l-1])
	                    \ EQUALITY : b_X(l) = xL(i) = xU(i) (i = i_x_fixed[l-1])
	 \end{verbatim}
	 *	@param	b_X	[in] vector (size <tt>n_X = n - n_R</tt>):
	 *				Initial varaible bounds (see <tt>bnd_fixed</tt>)
	 *	@param	Ko	[in] matrix (size <tt>(n_R+m) x (n_R+m)</tt>):  Initial KKT matrix
	 *	@param	fo 	[in] vector (size <tt>n_R + m</tt>): Initial KKT system rhs vector
	 *	@param	constraints
	 *				[in] Constraints object for the extra constraints
	 *					<tt>cL_bar <= A_bar'*x <= cU_bar</tt>
	 *	@param	out	[out] If <tt>out!=NULL</tt>, then any warning or error messages will
	 *					be printed here.
	 *	@param	test_setup
	 *				[in] If set to true, then consistency checks will be
	 *					made on all the input arguments.  The cost of the
	 *					tests will not be too excessive in runtime or
	 *					storge costs and do not completly validate everything
	 *	@param	waring_tol
	 *				[in] Warning tolerance for tests.
	 *	@param	error_tol
	 *				[in] Error tolerance for tests.  If the relative error
	 *					of any test exceeds this limit, then an error
	 *					message will be printed to out (if <tt>out!=NULL</tt>) and then
	 *					a runtime exception will be thrown.
	 *	@param	print_all_warnings
	 *				[in] If set to <tt>true</tt>, then any relative errors for tests
	 *					that are above <tt>warning_tol</tt> will be printed to
	 *					<tt>out</tt> (if <tt>out!= NULL</tt>) (O(<tt>n</tt>) output).
	 *					Otherwise, if <tt>false</tt>, then
	 *					only the number of violations and the maximum
	 *					violation will be printed (O(1) output).
	 */
	void initialize(
		const VectorSlice						&g
		,const MatrixSymWithOp					&G
		,const MatrixWithOp						*A
		,size_type								n_R
		,const size_type						i_x_free[]
		,const size_type						i_x_fixed[]
		,const EBounds							bnd_fixed[]
		,const VectorSlice						&b_X
		,const MatrixSymWithOpNonsingular		&Ko
		,const VectorSlice						&fo
		,Constraints							*constraints
		,std::ostream							*out				= NULL
		,bool									test_setup			= false
		,value_type								warning_tol			= 1e-10
		,value_type								error_tol			= 1e-5
		,bool									print_all_warnings	= false
		);

	/** @name Overridden from QP */
	//@{ 

	///
	size_type n() const;
	///
	size_type m() const;
	///
	const VectorSlice g() const;
	///
	const MatrixSymWithOp& G() const;
	///
	const MatrixWithOp& A() const;
	///
	size_type n_R() const;
	///
	const x_init_t& x_init() const;
	///
	const l_x_X_map_t& l_x_X_map() const;
	///
	const i_x_X_map_t& i_x_X_map() const;
	///
	const VectorSlice b_X() const;
	///
	const GenPermMatrixSlice& Q_R() const;
	///
	const GenPermMatrixSlice& Q_X() const;
	///
	const MatrixSymWithOpNonsingular& Ko() const;
	///
	const VectorSlice fo() const;
	///
	Constraints& constraints();
	///
	const Constraints& constraints() const;

	//@}

private:

	// ///////////////////////////////////
	// Private types

	typedef std::vector<size_type>		row_i_t;
	typedef std::vector<size_type>		col_j_t;
	
	// ///////////////////////////////////
	// Private data members


	size_type				n_;
	size_type				n_R_;
	size_type				m_;
	VectorSlice				g_;	// will not be modified!
	const MatrixSymWithOp	*G_;
	const MatrixWithOp		*A_;	// If NULL not no equalities in Ko
	x_init_t				x_init_;
	l_x_X_map_t				l_x_X_map_;
	i_x_X_map_t				i_x_X_map_;
	VectorSlice				b_X_;	// will not be modified!
	GenPermMatrixSlice		Q_R_;
	row_i_t					Q_R_row_i_;
	col_j_t					Q_R_col_j_;
	GenPermMatrixSlice		Q_X_;
	row_i_t					Q_X_row_i_;
	col_j_t					Q_X_col_j_;
	const MatrixSymWithOpNonsingular
							*Ko_;
	VectorSlice				fo_;	// will not be modified
	Constraints				*constraints_;

	// Private member function
	void assert_initialized() const;

};	// end class QPInitFixedFreeStd

}	// end namespace QPSchurPack
}	// end namespace ConstrainedOptimizationPack 

#endif	// QP_INIT_FIXED_FREE_STD_H
