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

#ifndef ETA_VECTOR_H
#define ETA_VECTOR_H

#include "AbstractLinAlgPack_SpVectorClass.hpp"

namespace AbstractLinAlgPack {

///
/** Create an eta vector (scaled by alpha = default 1).
  *
  * The created vector is of size n and has the single nonzero
  * element of eta(i) = alpha.
  * 
  * The default constructor and assignment functions are not
  * allowed.
  */
class EtaVector {
public:
	
	typedef SpVectorSlice::element_type		ele_t;


	///
	EtaVector( ele_t::index_type i, size_type n, ele_t::value_type alpha = 1.0 )
		: ele_(i,alpha), sv_(&ele_,1,0,n,true)
	{}

	/// Implicit conversion to a SpVectorSlice object.
	operator const SpVectorSlice() const
	{
		return sv_;
	}

	/// Explicit conversion to a SpVectorSlice object.
	const SpVectorSlice& operator()() const
	{
		return sv_;
	}

private:
	ele_t			ele_;
	SpVectorSlice	sv_;

	// not defined and not to be called
	EtaVector();
	EtaVector& operator=(const EtaVector&);

};	// end class EtaVector


}	// namespace AbstractLinAlgPack

#endif	// ETA_VECTOR_H
