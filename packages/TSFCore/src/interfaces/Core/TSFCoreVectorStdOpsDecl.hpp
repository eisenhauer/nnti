// @HEADER
// ***********************************************************************
// 
//               TSFCore: Trilinos Solver Framework Core
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

// ////////////////////////////////////////////////////////////////////
// VectorStdOpsDecl.hpp

#ifndef TSFCORE_VECTOR_STD_OPS_DECL_HPP
#define TSFCORE_VECTOR_STD_OPS_DECL_HPP

#include "TSFCoreTypes.hpp"

namespace TSFCore {

/** \defgroup TSFCore_VectorStdOps_grp Collection of standard vector operations.
 */
//@{

///
/** <tt>result = sum( v(i), i = 1...v.space()->dim() )</tt>
 */
template<class Scalar>
Scalar sum( const Vector<Scalar>& v );

///
/** <tt>result = sqrt(<v,v>)</tt> : Natural norm of a vector.
 *
 * Returns <tt>Teuchos::ScalarTraits<Scalar>::squareroot(v.space()->scalarProd(v,v))</tt>
 */
template<class Scalar>
Scalar norm( const Vector<Scalar>& v );

///
/** <tt>result = ||v||1</tt>
 */
template<class Scalar>
Scalar norm_1( const Vector<Scalar>& v );

///
/** <tt>result = ||v||2</tt>
 */
template<class Scalar>
Scalar norm_2( const Vector<Scalar>& v );

///
/** <tt>result = ||v||inf</tt>
 */
template<class Scalar>
Scalar norm_inf( const Vector<Scalar>& v_rhs );

///
/** <tt>result = x'*y</tt>
 */
template<class Scalar>
Scalar dot( const Vector<Scalar>& x, const Vector<Scalar>& y );

///
/** <tt>result = v(i)</tt>
 */
template<class Scalar>
Scalar get_ele( const Vector<Scalar>& v, Index i );

///
/** <tt>v(i) = alpha</tt>
 */
template<class Scalar>
void set_ele( Index i, Scalar alpha, Vector<Scalar>* v );

///
/** <tt>y = alpha</tt>
 */
template<class Scalar>
void assign( Vector<Scalar>* y, const Scalar& alpha );

///
/** <tt>y = x</tt>
 */
template<class Scalar>
void assign( Vector<Scalar>* y, const Vector<Scalar>& x );

///
/** <tt>y += alpha</tt>
 */
template<class Scalar>
void Vp_S( Vector<Scalar>* y, const Scalar& alpha );

///
/** <tt>y *= alpha</tt>
 *
 * This takes care of the special cases of <tt>alpha == 0.0</tt>
 * (set <tt>y = 0.0</tt>) and <tt>alpha == 1.0</tt> (don't
 * do anything).
 */
template<class Scalar>
void Vt_S( Vector<Scalar>* y, const Scalar& alpha );

///
/** <tt>y = alpha * x + y</tt>
 */
template<class Scalar>
void Vp_StV( Vector<Scalar>* y, const Scalar& alpha, const Vector<Scalar>& x );

///
/** <tt>y(i) += alpha * x(i) * v(i), i = 1...y->space()->dim()</tt>
 */
template<class Scalar>
void ele_wise_prod( const Scalar& alpha, const Vector<Scalar>& x, const Vector<Scalar>& v, Vector<Scalar>* y );

///
/** <tt>y(i) = alpha * x(i) / v(i), i = 1...y->space()->dim()</tt>
 */
template<class Scalar>
void ele_wise_divide( const Scalar& alpha, const Vector<Scalar>& x, const Vector<Scalar>& v, Vector<Scalar>* y );

///
/** Seed the random number generator used in <tt>random_vector</tt>
 */
template<class Scalar>
void seed_randomize( unsigned int );

///
/** Generate a random vector with elements uniformly distrubuted
 * elements.
 * 
 * The elements <tt>v->getEle(i)</tt> are randomly generated between
 * <tt>[l,u]</tt>.
 *
 * The seed is set using <tt>seed_randomize()</tt>
 */
template<class Scalar>
void randomize( Scalar l, Scalar u, Vector<Scalar>* v );

//@}

} // end namespace TSFCore

// ////////////////////////////
// Inline functions

template<class Scalar>
inline
Scalar TSFCore::norm( const Vector<Scalar>& v )
{
	return Teuchos::ScalarTraits<Scalar>::squareroot(v.space()->scalarProd(v,v));
}


#endif // TSFCORE_VECTOR_STD_OPS_DECL_HPP
