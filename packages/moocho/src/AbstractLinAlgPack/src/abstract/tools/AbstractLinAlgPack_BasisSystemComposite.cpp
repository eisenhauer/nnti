// ///////////////////////////////////////////////////////////////////
// BasisSystemComposite.cpp
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

#include "AbstractLinAlgPack/src/abstract/tools/BasisSystemComposite.hpp"
#include "AbstractLinAlgPack/src/abstract/interfaces/MatrixOpNonsing.hpp"
#include "AbstractLinAlgPack/src/abstract/tools/MatrixComposite.hpp"
#include "AbstractLinAlgPack/src/abstract/interfaces/MultiVectorMutable.hpp"
#include "AbstractLinAlgPack/src/abstract/tools/VectorSpaceBlocked.hpp"
#include "AbstractLinAlgPack/src/abstract/interfaces/LinAlgOpPack.hpp"
#include "ReleaseResource_ref_count_ptr.hpp"
#include "AbstractFactoryStd.hpp"
#include "dynamic_cast_verbose.hpp"
#include "Teuchos_TestForException.hpp"

namespace {

// Allocates a MultiVectorMutable object given a vector space
// object and the number of columns (num_vecs).

class AllocatorMultiVectorMutable {
public:
	AllocatorMultiVectorMutable(
		const AbstractLinAlgPack::VectorSpace::space_ptr_t&  vec_space
		,AbstractLinAlgPack::size_type                       num_vecs
		)
		:vec_space_(vec_space)
		,num_vecs_(num_vecs)
	{}
	typedef Teuchos::RefCountPtr<
		AbstractLinAlgPack::MultiVectorMutable>               ptr_t;
	ptr_t allocate() const
	{
		return vec_space_->create_members(num_vecs_);
	}
private:
	AbstractLinAlgPack::VectorSpace::space_ptr_t  vec_space_;
	AbstractLinAlgPack::size_type                 num_vecs_;
}; // end class AllocatorMultiVectorMutable

} // end namespace

namespace AbstractLinAlgPack {

// Static member functions

void BasisSystemComposite::initialize_space_x(
	const VectorSpace::space_ptr_t    &space_xD
	,const VectorSpace::space_ptr_t   &space_xI
	,Range1D                          *var_dep
	,Range1D                          *var_indep
	,VectorSpace::space_ptr_t         *space_x
	)
{
	namespace mmp = MemMngPack;
#ifdef _DEBUG
	TEST_FOR_EXCEPTION(
		space_xD.get() == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize_space_x(...): Error!" );
	TEST_FOR_EXCEPTION(
	    var_dep == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize_space_x(...): Error!" );
	TEST_FOR_EXCEPTION(
		space_xI.get() != NULL && var_indep == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize_space_x(...): Error!" );
	TEST_FOR_EXCEPTION(
		space_x == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize_space_x(...): Error!" );
#endif
	*var_dep   = Range1D(1,space_xD->dim());
	if(space_xI.get()) *var_indep = Range1D(var_dep->ubound()+1,var_dep->ubound()+space_xI->dim());
	else               *var_indep = Range1D::Invalid;
	if(space_xI.get()) {
		const VectorSpace::space_ptr_t
			vec_spaces[2] = { space_xD, space_xI };
		*space_x   = Teuchos::rcp(new VectorSpaceBlocked(vec_spaces,2));
	}
	else {
		*space_x = space_xD;
	}
}

const BasisSystemComposite::fcty_Gc_ptr_t
BasisSystemComposite::factory_Gc()
{
	namespace mmp = MemMngPack;
	return Teuchos::rcp( new mmp::AbstractFactoryStd<MatrixOp,MatrixComposite>() );
}

void BasisSystemComposite::initialize_Gc(
	const VectorSpace::space_ptr_t    &space_x
	,const Range1D                    &var_dep
	,const Range1D                    &var_indep
	,const VectorSpace::space_ptr_t   &space_c
	,const C_ptr_t                    &C
	,const N_ptr_t                    &N
	,MatrixOp                         *Gc
	)
{
	namespace mmp = MemMngPack;
	using DynamicCastHelperPack::dyn_cast;
#ifdef _DEBUG
	TEST_FOR_EXCEPTION(
		space_x.get() == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize_Gc(...): Error!" );
	TEST_FOR_EXCEPTION(
		space_c.get() == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize_Gc(...): Error!" );
#endif
	const size_type
		n            = space_x->dim(),
		m            = space_c->dim(),
		var_dep_size = var_dep.size();
#ifdef _DEBUG
	TEST_FOR_EXCEPTION(
		C.get() == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize_Gc(...): Error!" );
	TEST_FOR_EXCEPTION(
		var_dep_size < n && N.get() == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize_Gc(...): Error!" );
	TEST_FOR_EXCEPTION(
		Gc == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize_Gc(...): Error!" );
#endif

	MatrixComposite
		&Gc_comp = dyn_cast<MatrixComposite>(*Gc);
		
	//
	// Gc = [ C'; N' ]
	//
	
	Gc_comp.reinitialize(n,m);
	// Add the C matrix object
	typedef Teuchos::RefCountPtr<mmp::ReleaseResource_ref_count_ptr<MatrixOpNonsing> > C_rr_ptr_ptr_t;
	C_rr_ptr_ptr_t
		C_rr_ptr_ptr = Teuchos::rcp(new mmp::ReleaseResource_ref_count_ptr<MatrixOpNonsing>(C));
	Gc_comp.add_matrix(
		var_dep.lbound()-1, 0    // row_offset, col_offset
		,1.0                     // alpha
		,C_rr_ptr_ptr->ptr.get() // A
		,C_rr_ptr_ptr            // A_release
		,BLAS_Cpp::trans         // A_trans
		);
	if( n > m ) {
		// Add the N matrix object
		typedef Teuchos::RefCountPtr<mmp::ReleaseResource_ref_count_ptr<MatrixOp> > N_rr_ptr_ptr_t;
		N_rr_ptr_ptr_t
			N_rr_ptr_ptr = Teuchos::rcp(new mmp::ReleaseResource_ref_count_ptr<MatrixOp>(N));
		Gc_comp.add_matrix(
			var_indep.lbound()-1, 0  // row_offset, col_offset
			,1.0                     // alpha
			,N_rr_ptr_ptr->ptr.get() // A
			,N_rr_ptr_ptr            // A_release
			,BLAS_Cpp::trans         // A_trans
			);
	}
	// Finish construction
	Gc_comp.finish_construction( space_x, space_c );
}

void BasisSystemComposite::get_C_N(
	MatrixOp               *Gc
	,MatrixOpNonsing       **C
	,MatrixOp              **N
	)
{
	using DynamicCastHelperPack::dyn_cast;
#ifdef _DEBUG
	TEST_FOR_EXCEPTION(
		Gc == NULL, std::invalid_argument
		,"BasisSystemComposite::get_C_N(...): Error!" );
#endif
	const size_type
		n = Gc->rows(),
		m = Gc->cols();
#ifdef _DEBUG
	TEST_FOR_EXCEPTION(
		C == NULL, std::invalid_argument
		,"BasisSystemComposite::get_C_N(...): Error!" );
	TEST_FOR_EXCEPTION(
		n > m && N == NULL, std::invalid_argument
		,"BasisSystemComposite::get_C_N(...): Error!" );
#endif
	// Get reference to concrete Gc matrix subclass
	MatrixComposite
		&Gc_comp = dyn_cast<MatrixComposite>(*Gc);
	// Get referencs to the aggregate C and N matrtices
	MatrixComposite::matrix_list_t::const_iterator
		mat_itr = Gc_comp.matrices_begin(),
		mat_end = Gc_comp.matrices_end();
	if( mat_itr != mat_end ) {
		assert(mat_itr != mat_end);
		*C = &dyn_cast<MatrixOpNonsing>(
			const_cast<MatrixOp&>(*(mat_itr++)->A_) );
		if( n > m ) {
			assert(mat_itr != mat_end);
			*N = &const_cast<MatrixOp&>(*(mat_itr++)->A_);
		}
		assert(mat_itr == mat_end);
	}
	else {
		*C = NULL;
		*N = NULL;
	}
}

void BasisSystemComposite::get_C_N(
	const MatrixOp               &Gc
	,const MatrixOpNonsing       **C
	,const MatrixOp              **N
	)
{
	using DynamicCastHelperPack::dyn_cast;
	const size_type
		n = Gc.rows(),
		m = Gc.cols();
#ifdef _DEBUG
	TEST_FOR_EXCEPTION(
		C == NULL, std::invalid_argument
		,"BasisSystemComposite::get_C_N(...): Error!" );
	TEST_FOR_EXCEPTION(
		n > m && N == NULL, std::invalid_argument
		,"BasisSystemComposite::get_C_N(...): Error!" );
#endif
	// Get reference to concrete Gc matrix subclass
	const AbstractLinAlgPack::MatrixComposite
		&Gc_comp = dyn_cast<const AbstractLinAlgPack::MatrixComposite>(Gc);
	// Get referencs to the aggregate C and N matrtices
	MatrixComposite::matrix_list_t::const_iterator
		mat_itr = Gc_comp.matrices_begin(),
		mat_end = Gc_comp.matrices_end();
	if( mat_itr != mat_end ) {
		assert(mat_itr != mat_end);
		*C = &dyn_cast<const MatrixOpNonsing>(*(mat_itr++)->A_);
		if( n > m ) {
			assert(mat_itr != mat_end);
			*N = &dyn_cast<const MatrixOp>(*(mat_itr++)->A_);
		}
		assert(mat_itr == mat_end);
	}
	else {
		TEST_FOR_EXCEPTION(
			true, std::invalid_argument
			,"BasisSystemComposite::get_C_N(...): Error, "
			"The Gc matrix object has not been initialized with C and N!" );
	}
}

// Constructors / initializers

BasisSystemComposite::BasisSystemComposite()
	:BasisSystem(Teuchos::null,Teuchos::null)
{}

BasisSystemComposite::BasisSystemComposite(
	const VectorSpace::space_ptr_t       &space_x
	,const VectorSpace::space_ptr_t      &space_c
	,const mat_nonsing_fcty_ptr_t        &factory_C
	,const mat_sym_fcty_ptr_t            &factory_transDtD
	,const mat_sym_nonsing_fcty_ptr_t    &factory_S
	)
	:BasisSystem(Teuchos::null,Teuchos::null)
{
	namespace mmp = MemMngPack;
	const size_type n = space_x->dim(), m = space_c->dim();
	this->initialize(
		space_x,Range1D(1,space_c->dim()),(n>m ? Range1D(space_c->dim()+1,space_x->dim()) : Range1D::Invalid)
		,space_c,factory_C,factory_transDtD,factory_S
		);
}

BasisSystemComposite::BasisSystemComposite(
	const VectorSpace::space_ptr_t       &space_x
	,const Range1D                       &var_dep
	,const Range1D                       &var_indep
	,const VectorSpace::space_ptr_t      &space_c
	,const mat_nonsing_fcty_ptr_t        &factory_C
	,const mat_sym_fcty_ptr_t            &factory_transDtD
	,const mat_sym_nonsing_fcty_ptr_t    &factory_S
	,const mat_fcty_ptr_t                &factory_D
	)
	:BasisSystem(Teuchos::null,Teuchos::null)
{
	this->initialize(
		space_x,var_dep,var_indep,space_c,factory_C,factory_transDtD,factory_S
		,factory_D
		);
}
	
void BasisSystemComposite::initialize(
	const VectorSpace::space_ptr_t       &space_x
	,const Range1D                       &var_dep
	,const Range1D                       &var_indep
	,const VectorSpace::space_ptr_t      &space_c
	,const mat_nonsing_fcty_ptr_t        &factory_C
	,const mat_sym_fcty_ptr_t            &factory_transDtD
	,const mat_sym_nonsing_fcty_ptr_t    &factory_S
	,const mat_fcty_ptr_t                &factory_D
	)
{
	namespace mmp = MemMngPack;
#ifdef _DEBUG
	TEST_FOR_EXCEPTION(
		space_x.get() == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize(...): Error!" );
	TEST_FOR_EXCEPTION(
		space_c.get() == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize(...): Error!" );
#endif
	const size_type n = space_x->dim(), m = space_c->dim();
#ifdef _DEBUG
	TEST_FOR_EXCEPTION(
		var_dep.size() + var_indep.size() != space_x->dim(), std::invalid_argument
		,"BasisSystemComposite::initialize(...): Error!" );
	TEST_FOR_EXCEPTION(
		n > m && var_dep.lbound() < var_indep.lbound() && (var_dep.lbound() != 1 || var_dep.ubound()+1 != var_indep.lbound())
		, std::invalid_argument
		,"BasisSystemComposite::initialize(...): Error!" );
	TEST_FOR_EXCEPTION(
		n > m && var_dep.lbound() >= var_indep.lbound() && (var_indep.lbound() != 1 || var_indep.ubound()+1 != var_dep.lbound())
		, std::invalid_argument
		,"BasisSystemComposite::initialize(...): Error!" );
	TEST_FOR_EXCEPTION(
		factory_C.get() == NULL, std::invalid_argument
		,"BasisSystemComposite::initialize(...): Error!" );
#endif

	space_x_         = space_x;
	var_dep_         = var_dep;
	var_indep_       = var_indep;
	space_c_         = space_c;
	factory_C_       = factory_C;
	factory_D_       = factory_D;
	if( factory_D_.get() == NULL ) {
		factory_D_ = mmp::abstract_factory_std_alloc<MatrixOp,MultiVectorMutable>(
			AllocatorMultiVectorMutable(space_x_->sub_space(var_dep),var_indep.size() ) );
	}
	BasisSystem::initialize(factory_transDtD,factory_S);
}

void BasisSystemComposite::set_uninitialized()
{
	namespace mmp = MemMngPack;

	space_x_         = Teuchos::null;
	var_dep_         = Range1D::Invalid;
	var_indep_       = Range1D::Invalid;
	factory_C_       = Teuchos::null;
	factory_D_       = Teuchos::null;
}

const VectorSpace::space_ptr_t&
BasisSystemComposite::space_x() const
{
	return space_x_;
}

const VectorSpace::space_ptr_t&
BasisSystemComposite::space_c() const
{
	return space_c_;
}

// To be overridden by subclasses

void BasisSystemComposite::update_D(
	const MatrixOpNonsing       &C
	,const MatrixOp             &N
	,MatrixOp                   *D
	,EMatRelations              mat_rel
	) const
{
	using LinAlgOpPack::M_StInvMtM;
	M_StInvMtM( D, -1.0, C, BLAS_Cpp::no_trans, N, BLAS_Cpp::no_trans ); // D = -inv(C)*N
}

// Overridden from BasisSytem

const BasisSystem::mat_nonsing_fcty_ptr_t
BasisSystemComposite::factory_C() const
{
	return factory_C_;
}

const BasisSystem::mat_fcty_ptr_t
BasisSystemComposite::factory_D() const
{
	return factory_D_;
}

Range1D BasisSystemComposite::var_dep() const
{
	return var_dep_;
}

Range1D BasisSystemComposite::var_indep() const
{
	return var_indep_;
}

void BasisSystemComposite::update_basis(
	const MatrixOp          &Gc
	,MatrixOpNonsing        *C
	,MatrixOp               *D
	,MatrixOp               *GcUP
	,EMatRelations          mat_rel
	,std::ostream           *out
	) const
{
	using DynamicCastHelperPack::dyn_cast;
	const index_type
		n  = var_dep_.size() + var_indep_.size(),
		m  = var_dep_.size();
	TEST_FOR_EXCEPTION(
		n == 0, std::logic_error
		,"BasisSystemComposite::update_basis(...): Error, this must be initialized first!" );
	TEST_FOR_EXCEPTION(
		C == NULL && ( n > m ? D == NULL : false ), std::logic_error
		,"BasisSystemComposite::update_basis(...): Error, C or D must be non-NULL!" );
	// Get references to the aggregate C and N matrices
	const MatrixOpNonsing
		*C_aggr = NULL;
	const MatrixOp
		*N_aggr = NULL;
	get_C_N( Gc, &C_aggr, &N_aggr );
	// Setup C
	if( C ) {
		*C = *C_aggr;  // Assignment had better work!
	}
	// Compute D
	if( D ) {
		update_D(*C_aggr,*N_aggr,D,mat_rel);
	}
}

} // end namespace AbstractLinAlgPack
