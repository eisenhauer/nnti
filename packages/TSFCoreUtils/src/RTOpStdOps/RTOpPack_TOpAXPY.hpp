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
// RTOpPack_TOpAXPY.hpp

#ifndef RTOPPACK_TOP_AXPY_HPP
#define RTOPPACK_TOP_AXPY_HPP

#include "RTOpPack_RTOpTHelpers.hpp"

namespace RTOpPack {

///
/** AXPY transforamtion operator: <tt>z0[i] += alpha*v0[i], i=1...n</tt>.
 */
template<class Scalar>
class TOpAXPY : public ROpScalarTransformationBase<Scalar> {
public:
  ///
  void alpha( const Scalar& alpha ) { this->scalarData(alpha); }
  ///
  Scalar alpha() const { return this->scalarData(); }
  ///
  TOpAXPY( const Scalar &alpha = Teuchos::ScalarTraits<Scalar>::zero() )
    : ROpScalarTransformationBase<Scalar>(alpha), RTOpT<Scalar>("TOpAXPY")
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
      RTOP_APPLY_OP_1_1(num_vecs,sub_vecs,num_targ_vecs,targ_sub_vecs);
      for( RTOp_index_type i = 0; i < subDim; ++i, v0_val += v0_s, z0_val += z0_s ) {
        *z0_val += alpha() * (*v0_val);
      }
    }
  //@}
}; // class TOpAXPY

} // namespace RTOpPack

#endif // RTOPPACK_TOP_AXPY_HPP
