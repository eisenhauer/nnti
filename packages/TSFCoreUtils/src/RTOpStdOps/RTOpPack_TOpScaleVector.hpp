// @HEADER
// ***********************************************************************
// 
//      TSFCoreUtils: Trilinos Solver Framework Utilities Package 
//                 Copyright (2004) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

// ///////////////////////////////
// RTOpPack_TOpScaleVector.hpp

#ifndef RTOPPACK_TOP_SCALE_VECTOR_HPP
#define RTOPPACK_TOP_SCALE_VECTOR_HPP

#include "RTOpPack_RTOpTHelpers.hpp"

namespace RTOpPack {

///
/** Simple transformation operator that scales every vector element by
 * a scalar <tt>alpha</tt>.
 */
template<class Scalar>
class TOpScaleVector : public ROpScalarTransformationBase<Scalar> {
public:
  ///
  void alpha( const Scalar& alpha ) { this->scalarData(alpha); }
  ///
  Scalar alpha() const { return this->scalarData(); }
  ///
  TOpScaleVector( const Scalar &alpha = Teuchos::ScalarTraits<Scalar>::zero() )
    : RTOpT<Scalar>("TOpScaleVector"), ROpScalarTransformationBase<Scalar>(alpha)
    {}
  /** @name Overridden from RTOpT */
  //@{
  ///
	void apply_op(
		const int   num_vecs,       const SubVectorT<Scalar>         sub_vecs[]
		,const int  num_targ_vecs,  const MutableSubVectorT<Scalar>  targ_sub_vecs[]
		,ReductTarget *reduct_obj
		) const
    {
      RTOP_APPLY_OP_0_1(num_vecs,sub_vecs,num_targ_vecs,targ_sub_vecs);
      const Scalar alpha = this->alpha();
      if( z0_s == 1 ) {
        for( RTOp_index_type i = 0; i < subDim; ++i )
          *z0_val++ *= alpha;
      }
      else {
        for( RTOp_index_type i = 0; i < subDim; ++i, z0_val += z0_s )
          *z0_val *= alpha;
      }
}
  //@}
}; // class TOpScaleVector

} // namespace RTOpPack

#endif // RTOPPACK_TOP_SCALE_VECTOR_HPP
