// //////////////////////////////////////////////////////////////////////////
// BasisSystemComposite.hpp
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

#ifndef BASIS_SYSTEM_COMPOSITE_STD_H
#define BASIS_SYSTEM_COMPOSITE_STD_H

#include "AbstractLinAlgPack/src/abstract/interfaces/BasisSystem.hpp"
#include "AbstractLinAlgPack/src/abstract/interfaces/VectorSpace.hpp"
#include "Range1D.hpp"

namespace AbstractLinAlgPack {

///
/** Simple <tt>%BasisSystem</tt> subclass the case where the client sets up seperate \c C and \c N matrices.
 *
 * This interface is based an implementation where \c C and \c N are manipulated by the application and
 * are concatenated into <tt>Gc = [ C'; N' ]</tt>.  Here, there are no undecomposed equality constraints allowed.
 *
 * For this implementation, the basis matrix \c C must override the method
 * <tt>MatrixOp::operator=()</tt> for correct behavior.  A smart implementation of the
 * basis matrix subclass will use lazy evaluation and not copy data inside of
 * <tt>MatrixOp::operator=()</tt> unless necessary later on.
 */
class BasisSystemComposite
	: public AbstractLinAlgPack::BasisSystem
{
public:

	/** @name Public types */
	//@{

	///
	typedef MemMngPack::ref_count_ptr<const MemMngPack::AbstractFactory<MatrixOp> >  fcty_Gc_ptr_t;
	///
	typedef MemMngPack::ref_count_ptr<MatrixOpNonsing>                               C_ptr_t;
	///
	typedef MemMngPack::ref_count_ptr<MatrixOp>                                      N_ptr_t;

	//@}

	/** @name Static member functions */
	//@{

	///
	/** Initialize the composite vector space for <tt>x = [ xD; xI ]</tt> as well as \c var_dep and \c var_indep.
	 *
	 * @param  space_xD  [in/out] DVector space for the dependent variables.  On output
	 *                   <tt>space_xD.count()</tt> will be incremented by 1.
	 * @param  space_xI  [in/out] DVector space for the independent variables.  On output
	 *                   <tt>space_xI.count()</tt> will be incremented by 1.
	 * @param  var_dep   [out] Range for dependent variables in output \c space_x
	 * @param  var_indep [out] Range for independent variables in output \c space_x
	 * @param  space_x   [out] Newly formed composite vector space <tt>space_x = [ space_xD; space_xI ]</tt>.
	 *                   The object <tt>*space_x</tt> will be dependent on the objects <tt>*space_xD</tt>
	 *                   <tt>*space_xI</tt>.  If the client wants <tt>*space_x</tt> to be independent from
	 *                   these vector space objects then <tt>space_x->clone()</tt> can be used.
	 *
	 * Preconditions:<ul>
	 * <li> <tt>space_xD.get() != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>space_xI.get() != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>var_dep != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>var_indep != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li> <tt>var_dep->size() == space_xD->dim()</tt> 
	 * <li> <tt>var_indep->size() == space_xI->dim()</tt>
	 * <li> \c var_dep and \c var_indep are non-overlapping ranges.
	 * <li> <tt>space_x->dim() == var_dep->size() + var_indep->size()</tt>
	 * <tt> <tt>space_x->sub_space(*var_dep).get() == space_xD.get()</tt>
	 * <tt> <tt>space_x->sub_space(*var_indep).get() == space_xI.get()</tt>
	 * </ul>
	 */
	static void initialize_space_x(
		const VectorSpace::space_ptr_t    &space_xD
		,const VectorSpace::space_ptr_t   &space_xI
		,Range1D                          *var_dep
		,Range1D                          *var_indep
		,VectorSpace::space_ptr_t         *space_x
		);

	///
	/** Return a matrix factory object for the composte \c Gc matrix object.
	 */
	static const fcty_Gc_ptr_t factory_Gc();
	
	///
	/** Initialize the Gc matrix object given created from <tt>space_Gc()->create()</tt>.
	 *
	 * Initializes the composite matrix object:
	 \verbatim

	 Gc = [ C'; N' ]
	 \endverbatim
	 *
	 * @param  space_x   [in] DVector space for the variables (returned from \c initialize_space_x()).
	 * @param  var_dep   [in] Range for dependent variables in \c space_x.
	 * @param  var_indep [in] Range for independent variables in \c space_x.
	 * @param  space_c   [in] DVector space for the equality constraints.
	 * @param  C         [in/out] Nonsingular basis matrix, initialized and ready to go.  On output
	 *                   <tt>C.count()</tt> will be incremented by 1.
	 * @param  N         [in/out] Non-basis matrix, initialized and ready to go.  On output
	 *                   <tt>N.count()</tt> will be incremented by 1.
	 * @param  Gc        [in/out] Composite matrix object that on output is initialized with.
	 *                   \c C and \c N.
	 *
	 * Preconditions:<ul>
	 * <li> <tt>space_x.get() != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>space_c.get() != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>C.get() != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>N.get() != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>Gc != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * </ul>
	 * 
	 * Postconditions:<ul>
	 * <li> <tt>&return->space_cols() == space_x.get()</tt>
	 * <li> <tt>&return->space_rows() == space_c.get()</tt>
	 * <li> ToDo: Finish!
	 * </ul>
	 */
	static void initialize_Gc(
		const VectorSpace::space_ptr_t    &space_x
		,const Range1D                    &var_dep
		,const Range1D                    &var_indep
		,const VectorSpace::space_ptr_t   &space_c
		,const C_ptr_t                    &C
		,const N_ptr_t                    &N
		,MatrixOp                         *Gc
		);

	///
	/** Get the non-const aggregate matrices \c C and \c N (or NULL pointers if not initialized).
	 *
	 * @param  Gc        [in] Composite matrix object <tt>Gc = [ C'; N' ]</tt>
	 * @param  C         [out] Pointer to basis matrix object \c C.  If \c Gc has not
	 *                   been initialized then <tt>*C == NULL</tt> on output.
	 * @param  N         [out] Pointer to nonbasis matrix object \c N.  If \c Gc has not
	 *                   been initialized then <tt>*N == NULL</tt> on output.
	 * Preconditions:<ul>
	 * <li> <tt>Gc != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>C != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>N != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * </ul>
	 */
	static void get_C_N(
	    MatrixOp               *Gc
		,MatrixOpNonsing       **C
		,MatrixOp              **N
		);

	///
	/** Get the const aggregate matrices C and N.
	 *
	 * @param  Gc        [in] Composite matrix object <tt>Gc = [ C'; N' ]</tt>.  If
	 *                   this matrix object has not been initialized with \c C
	 *                   and \c N matrix objects then an exception is thown.
	 * @param  C         [out] Pointer to basis matrix object \c C.
	 * @param  N         [out] Pointer to nonbasis matrix object \c N.
	 *
	 * Preconditions:<ul>
	 * <li> \c Gc is setup with non-null \c C and \c N matrix objects
	 *      (throw <tt>std::logic_error</tt>).
	 * <li> <tt>C != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>N != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * </ul>
	 *
	 * Preconditions:<ul>
	 * <li> <tt>C != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>N != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * </ul>
	 */
	static void get_C_N(
		const MatrixOp               &Gc
		,const MatrixOpNonsing       **C
		,const MatrixOp              **N
		);

	//@}

	/** @name Constructors / initializers */
	//@{

	/** Default constructor.
	 */
	BasisSystemComposite();

	///
	/**  Calls <tt>this->initialize()</tt> in a way that is consistant with above helper functions.
	 */
	BasisSystemComposite(
		const VectorSpace::space_ptr_t       &space_x
		,const VectorSpace::space_ptr_t      &space_c
		,const mat_nonsing_fcty_ptr_t        &factory_C
		,const mat_sym_fcty_ptr_t            &factory_transDtD
		,const mat_sym_nonsing_fcty_ptr_t    &factory_S
		);

	/// Calls <tt>this->initialize()</tt>
	BasisSystemComposite(
		const VectorSpace::space_ptr_t       &space_x
		,const Range1D                       &var_dep
		,const Range1D                       &var_indep
		,const VectorSpace::space_ptr_t      &space_c
		,const mat_nonsing_fcty_ptr_t        &factory_C
		,const mat_sym_fcty_ptr_t            &factory_transDtD
		,const mat_sym_nonsing_fcty_ptr_t    &factory_S
		,const mat_fcty_ptr_t                &factory_D        = MemMngPack::null
		);
	
	///
	/** Initialize.
	 *
	 * @param  space_x    [in] Smart pointer to vector space for \c x.
	 * @param  var_dep    [in] Range for dependent variables \c xD.
	 * @param  var_indep  [in] Range for independent variables \c xI.
	 * @param  factory_C  [in] Smart pointer to factory object for basis matrix \c C.
	 * @param  factory_transDtD
	 *                    [in] Smart point to the factory object for the matrix <tt>J = D'*D</tt>.
	 * @param  factory_S  [in] Smart point to the factory object for the matrix <tt>S = I + D'*D</tt>.
	 * @param  factory_D  [in] Smart pointer to factory object for direct sensitivity matrix
	 *                    \c D.  If <tt>factory_D == NULL</tt> then an <tt>AbstractFactoryStd<></tt>
	 *                    object will be used which calls <tt>space_xD->create_members(space_xI->dim())</tt>.
	 *                    which in turn of course creates \c MultiVectorMutable objects.
	 *
	 * Preconditions:<ul>
	 * <li> <tt>space_xD.get() != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>space_xI.get() != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * <li> <tt>factory_C.get() != NULL</tt> (throw <tt>std::invalid_argument</tt>)
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li> <tt>this->var_dep() == [1,space_xD->dim()]</tt>
	 * <li> <tt>this->var_indep() == [space_xD->dim()+1,space_xD->dim()+space_xI->dim()</tt>
	 * <li> <tt>this->equ_decomp() == [1,space_xD->dim()]</tt>
	 * <li> <tt>this->equ_undecomp().size() == 0</tt>
	 * <li> <tt>this->factory_C().get() != NULL</tt>
	 * <li> <tt>this->factory_D().get() != NULL</tt>
	 * </ul>
	 */
	void initialize(
		const VectorSpace::space_ptr_t       &space_x
		,const Range1D                       &var_dep
		,const Range1D                       &var_indep
		,const VectorSpace::space_ptr_t      &space_c
		,const mat_nonsing_fcty_ptr_t        &factory_C
		,const mat_sym_fcty_ptr_t            &factory_transDtD
		,const mat_sym_nonsing_fcty_ptr_t    &factory_S
		,const mat_fcty_ptr_t                &factory_D        = MemMngPack::null
		);

	///
	/** Set uninitialized.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>this->var_dep().size() == 0</tt>
	 * <li> <tt>this->var_indep().size() == 0</tt>
	 * <li> <tt>this->equ_decomp().size() == 0</tt>
	 * <li> <tt>this->equ_undecomp().size() == 0</tt>
	 * <tt> <tt>this->inequ_decomp().size() == 0</tt>
	 * <tt> <tt>this->equ_undecomp().size() == 0</tt>
	 * <li> <tt>this->factory_C().get() == NULL</tt>
	 * <li> <tt>this->factory_D().get() == NULL</tt>
	 * <tt> <tt>this->factory_GcUP().get() == NULL</tt>
	 * <tt> <tt>this->factory_GhUP().get() == NULL</tt>
	 * </ul>
	 */
	virtual void set_uninitialized();

	//@}

	/** @name Access */
	//@{

	///
	const VectorSpace::space_ptr_t& space_x() const;
	///
	const VectorSpace::space_ptr_t& space_c() const;

	//@}

	/** @name To be overridden by subclasses */
	//@{

	///
	/** Overridden by subclasses to update \c D if a specialized implementation is needed.
	 *
	 * The default implementation just relies on the <tt>MultiVectorMutable</tt>
	 * interface and the <tt>M_StInvMtV()</tt> method.
	 */
	virtual void update_D(
		const MatrixOpNonsing       &C
		,const MatrixOp             &N
		,MatrixOp                   *D
		,EMatRelations              mat_rel
		) const;

	//@}

	/** @name Overridden from BasisSystem */
	//@{

	///
	const mat_nonsing_fcty_ptr_t factory_C() const;
	///
	const mat_fcty_ptr_t factory_D() const;
	///
	Range1D var_dep() const;
	///
	Range1D var_indep() const;
	///
	void update_basis(
		const MatrixOp          *Gc
		,MatrixOpNonsing        *C
		,MatrixOp               *D
		,MatrixOp               *GcUP
		,EMatRelations          mat_rel
		,std::ostream           *out
		) const;

	//@}

private:
	
#ifndef DOXYGEN_COMPILE
	VectorSpace::space_ptr_t   space_x_;
	Range1D                    var_dep_;
	Range1D                    var_indep_;
	VectorSpace::space_ptr_t   space_c_;
	mat_nonsing_fcty_ptr_t     factory_C_;
	mat_fcty_ptr_t             factory_D_;
#endif

}; // end class BasisSystemComposite

} // end namespace AbstractPack

#endif // BASIS_SYSTEM_COMPOSITE_STD_H
