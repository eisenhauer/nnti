// //////////////////////////////////////////
// NLPWBCounterExample.hpp
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

#ifndef NLP_WB_COUNTER_EXAMPLE_H
#define NLP_WB_COUNTER_EXAMPLE_H

#include "NLPInterfacePack/src/serial/NLPSerialPreprocessExplJac.hpp"

namespace NLPInterfacePack {

///
/** %NLP subclass for the Waechter and Biegler Counter Example.
 *
 *
 * The Waechter & Biegler counter example %NLP is defined as:
 \verbatim

    min    f(x)
    s.t.
           c(1) = x(1)^2 - x(2) + a = 0
           c(2) = x(1)   - x(3) - b = 0

           x(2),x(3) >= 0

    where:
         b >= 0
 \endverbatim
 *
 * and where <tt>a</tt> and <tt>b</tt> are constants.  In the counter
 * example, the form of the objective function <tt>f(x)</tt> is not
 * important, but we have to specify one here in order to have MOOCHO
 * solve the problem.  So we will specify the objective function of
 \beginverbatim
        / x(1)       : if linear_obj == true
 f(x) = |
        \ 0.5*x(1)^2 : if linear_obj == false
 \endverbatim
 * Note that an excellent basis selection is for <tt>x(2)</tt> and
 * <tt>x(3)</tt> to be in the basis since this gives the basis
 * matrix of <tt>C = -I</tt>.
 */
class NLPWBCounterExample
	: public NLPSerialPreprocessExplJac
{
public:

	/** @name Constructors / initializers */
	//@{

	///
	/** Constructor.
	 *
	 * @param  a       [in] The constant in constriant <tt>c(1)</tt>
	 * @param  b       [in] The constant in constriant <tt>c(2)</tt>
	 * @param  x1_init [in] Initial guess for <tt>x(1)</tt>
	 * @param  x2_init [in] Initial guess for <tt>x(2)</tt>
	 * @param  x3_init [in] Initial guess for <tt>x(3)</tt>
	 * @param  nlp_selects_basis
     *                 [in] If true, then this NLP will select
     *                 the basis variables as <tt>x(2)</tt> and
     *                 <tt>x(3)</tt> (which gives <tt>C = -I</tt>.
	 * @param  linear_obj
	 *                 [in] If true, the the objective is
	 *                 set to <tt>f(x) = x(1)</tt>, else it
	 *                 is set to <tt>f(x) = 0.5*x(1)^2</tt>
	 */
	NLPWBCounterExample(
		value_type    a                  = 0.0
		,value_type   b                  = 1.0
		,value_type   x1_init            = 0.0
		,value_type   x2_init            = 0.0
		,value_type   x3_init            = 0.0
		,bool         nlp_selects_basis  = true
		,bool         linear_obj         = true
		);

	//@}

	/** @name Overridden public members from NLP */
	//@{

	///
	void initialize(bool test_setup);
	///
	bool is_initialized() const;
	///
	value_type max_var_bounds_viol() const;

	//@}

	/** @name Overridden from NLPVarReductPerm */
	//@{
	
	///
	bool nlp_selects_basis() const;

	//@}

protected:

	/** @name Overridden protected methods from NLPSerialPreprocess */
	//@{

	///
	bool imp_nlp_has_changed() const;
	///
	size_type imp_n_orig() const;
	///
	size_type imp_m_orig() const;
	///
	size_type imp_mI_orig() const;
	///
	const DVectorSlice imp_xinit_orig() const;
	///
	bool imp_has_var_bounds() const;
	///
	const DVectorSlice imp_xl_orig() const;
	///
	const DVectorSlice imp_xu_orig() const;
	///
	const DVectorSlice imp_hl_orig() const;
	///
	const DVectorSlice imp_hu_orig() const;
	///
	void imp_calc_f_orig(
		const DVectorSlice &x_full, bool newx, const ZeroOrderInfoSerial &zero_order_info ) const;
	///
	void imp_calc_c_orig(
		const DVectorSlice &x_full, bool newx, const ZeroOrderInfoSerial &zero_order_info ) const;
	///
	void imp_calc_h_orig(
		const DVectorSlice &x_full, bool newx, const ZeroOrderInfoSerial &zero_order_info ) const;
	///
	void imp_calc_Gf_orig(
		const DVectorSlice &x_full, bool newx, const ObjGradInfoSerial &obj_grad_info ) const;
	///
	bool imp_get_next_basis(
		IVector *var_perm_full, IVector *equ_perm_full, size_type *rank_full, size_type *rank );
	///
	void imp_report_orig_final_solution(
		const DVectorSlice &x_orig, const DVectorSlice *lambda_orig
		,const DVectorSlice *lambdaI_orig, const DVectorSlice *nu_orig, bool is_optimal ) const;

	//@}
	
	/** @name Overridden protected methods from NLPSerialPreprocessExplJac */
	//@{

	///
	size_type imp_Gc_nz_orig() const;
	///
	size_type imp_Gh_nz_orig() const;
	///
	void imp_calc_Gc_orig(
		const DVectorSlice& x_full, bool newx
		, const FirstOrderExplInfo& first_order_expl_info
		) const;
	///
	void imp_calc_Gh_orig(
		const DVectorSlice& x_full, bool newx
		, const FirstOrderExplInfo& first_order_expl_info
		) const;

	//@}

private:

	// /////////////////////////////////////////
	// Private data members

	bool         is_initialized_;  ///< Flag of if <tt>this</tt> is initialized yet
	bool         nlp_selects_basis_; ///< Flag for if <tt>this</tt> selects first basis
	bool         basis_selection_was_given_; ///< Flag for if <tt>this</tt> already selected a basis
	bool         linear_obj_;      ///< Flag for if a linear objective is used or not
	size_type    n_orig_;          ///< Number of variables
	size_type    m_orig_;          ///< Number of equality constraints
	size_type    Gc_orig_nz_;      ///< Number of nonzeros in Jacobian
	value_type   a_;               ///< Constant for <tt>c(1)</tt>
	value_type   b_;               ///< Constant for <tt>c(2)</tt>
	DVector      xinit_orig_;      ///< Initial guess for <tt>x_orig</tt>
	DVector      xl_orig_;         ///< Lower bounds for <tt>x_orig</tt>
	DVector      xu_orig_;         ///< Upper bounds for <tt>x_orig</tt>  

	// /////////////////////////////////////////
	// Private member functions

	// Not defined and not to be called
	NLPWBCounterExample();
	NLPWBCounterExample(const NLPWBCounterExample&);
	NLPWBCounterExample& operator=(const NLPWBCounterExample&);

};	// end class NLPWBCounterExample

}	// end namespace NLPInterfacePack

#endif	// NLP_WB_COUNTER_EXAMPLE_H
