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

#include <assert.h>

#include "AbstractLinAlgPack_VectorMutableThyra.hpp"
#include "AbstractLinAlgPack_ThyraAccessors.hpp"
#include "Teuchos_TestForException.hpp"
#include "Teuchos_Workspace.hpp"
#include "Teuchos_dyn_cast.hpp"

namespace AbstractLinAlgPack {

// Constructors / Initializers

VectorMutableThyra::VectorMutableThyra()
{}

VectorMutableThyra::VectorMutableThyra(
	const Teuchos::RefCountPtr<Thyra::VectorBase<value_type> >& thyra_vec
	)
{
	this->initialize(thyra_vec);
}

void VectorMutableThyra::initialize(
	const Teuchos::RefCountPtr<Thyra::VectorBase<value_type> >& thyra_vec
	)
{
	namespace mmp = MemMngPack;
	TEST_FOR_EXCEPTION(
		thyra_vec.get()==NULL, std::invalid_argument
		,"VectorMutableThyra::initialize(thyra_vec): Error!"
		);
	thyra_vec_ = thyra_vec;
	space_.initialize(thyra_vec->space());
	this->has_changed();
}

Teuchos::RefCountPtr<Thyra::VectorBase<value_type> > 
VectorMutableThyra::set_uninitialized()
{
	Teuchos::RefCountPtr<Thyra::VectorBase<value_type> > tmp_thyra_vec = thyra_vec_;
	thyra_vec_ = Teuchos::null;
	space_.set_uninitialized();
	this->has_changed();
	return tmp_thyra_vec;
}

// Methods overridden from Vector

const VectorSpace&
VectorMutableThyra::space() const
{
	return space_;
}

void VectorMutableThyra::apply_op(
	const RTOpPack::RTOp       &op
	,const size_t              num_vecs
	,const Vector*             vecs[]
	,const size_t              num_targ_vecs
	,VectorMutable*            targ_vecs[]
	,RTOpPack::ReductTarget    *reduct_obj
	,const index_type          first_ele
	,const index_type          sub_dim
	,const index_type          global_offset
	) const
{
	using Teuchos::dyn_cast;
	namespace mmp = MemMngPack;
	using Teuchos::Workspace;
	Teuchos::WorkspaceStore* wss = Teuchos::get_default_workspace_store().get();
	// If these are in-core vectors then just let a default implementation
	// take care of this.
	if( space_.is_in_core() ) {
		this->apply_op_serial(
			op,num_vecs,vecs,num_targ_vecs,targ_vecs,reduct_obj
			,first_ele,sub_dim,global_offset
			);
		return;
	}
	// Convert the non-mutable vectors into non-mutable Thyra vectors
	Workspace< Teuchos::RefCountPtr<const Thyra::VectorBase<value_type> > > thyra_vecs_sptr(wss,num_vecs);
	Workspace<const Thyra::VectorBase<value_type>*> thyra_vecs(wss,num_vecs);
	for(int k = 0; k < num_vecs; ++k ) {
		get_thyra_vector( space_, *vecs[k], &thyra_vecs_sptr[k] );
		thyra_vecs[k] = &*thyra_vecs_sptr[k];
	}
	// Convert the mutable vetors into mutable Thyra vectors
	Workspace< Teuchos::RefCountPtr<Thyra::VectorBase<value_type> > > targ_thyra_vecs_sptr(wss,num_targ_vecs);
	Workspace<Thyra::VectorBase<value_type>*> targ_thyra_vecs(wss,num_targ_vecs);
	for(int k = 0; k < num_targ_vecs; ++k ) {
		get_thyra_vector( space_, targ_vecs[k], &targ_thyra_vecs_sptr[k] );
		targ_thyra_vecs[k] = &*targ_thyra_vecs_sptr[k];
	}
	// Call the Thyra::apply_op(...)
	Thyra::applyOp(
		op
		,num_vecs,      num_vecs      ? &thyra_vecs[0]      : NULL
		,num_targ_vecs, num_targ_vecs ? &targ_thyra_vecs[0] : NULL
		,reduct_obj
		,first_ele,sub_dim,global_offset
		);
	// Free/commit the Thyra vector views
	for(int k = 0; k < num_vecs; ++k ) {
		free_thyra_vector( space_, *vecs[k], &thyra_vecs_sptr[k] );
	}
	for(int k = 0; k < num_targ_vecs; ++k ) {
		commit_thyra_vector( space_, targ_vecs[k], &targ_thyra_vecs_sptr[k] );
	}
}

index_type VectorMutableThyra::dim() const
{
	return space_.dim();
}

void VectorMutableThyra::get_sub_vector(
	const Range1D& rng, RTOpPack::SubVector* sub_vec
	) const
{
  RTOpPack::ConstSubVectorView<RTOp_value_type> _sub_vec = *sub_vec;
	thyra_vec_->acquireDetachedView(convert(rng),&_sub_vec);
  *sub_vec = _sub_vec;
}

void VectorMutableThyra::free_sub_vector(
	RTOpPack::SubVector* sub_vec
	) const
{
  RTOpPack::ConstSubVectorView<RTOp_value_type> _sub_vec = *sub_vec;
	thyra_vec_->releaseDetachedView(&_sub_vec);
  *sub_vec = _sub_vec;
}

// Methods overridden from VectorMutable

void VectorMutableThyra::get_sub_vector( const Range1D& rng, RTOpPack::MutableSubVector* sub_vec	)
{
  RTOpPack::SubVectorView<RTOp_value_type> _sub_vec = *sub_vec;
	thyra_vec_->acquireDetachedView(convert(rng),&_sub_vec);
  *sub_vec = _sub_vec;
}

void VectorMutableThyra::commit_sub_vector( RTOpPack::MutableSubVector* sub_vec )
{
  RTOpPack::SubVectorView<RTOp_value_type> _sub_vec = *sub_vec;
	thyra_vec_->commitDetachedView(&_sub_vec);
  *sub_vec = _sub_vec;
	this->has_changed();
}

void VectorMutableThyra::set_sub_vector( const RTOpPack::SparseSubVector& sub_vec	)
{
	thyra_vec_->setSubVector(sub_vec);
	this->has_changed();
}

} // end namespace AbstractLinAlgPack
