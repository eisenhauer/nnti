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

#ifndef MERIT_FUNC_NLP_MOD_L1_H
#define MERIT_FUNC_NLP_MOD_L1_H

#include "ConstrainedOptPack_MeritFuncNLP.hpp"
#include "ConstrainedOptPack_MeritFuncNLPDirecDeriv.hpp"
#include "ConstrainedOptPack_MeritFuncPenaltyParams.hpp"

namespace ConstrainedOptPack {

///
/** The modified L1 merit function using different penatly parameters for each constriant.
  *
  * phi(x) = f) + sum( mu(j) * abs(c(j)), j = 1,...,m )
  *
  * Dphi(x_k,d_k) = Gf_k' * d_k - sum( mu(j) * abs(c(j)), j = 1,...,m )
  *
  * Note that the definition of Dphi(x_k,d_k) assumes
  * that Gc_k'*d_k + c_k = 0.  In otherwords, d_k must
  * satisfiy the linearized equality constraints at
  * at x_k.
  *
  * Implicit copy constructor and assignment operators
  * are allowed.
  */
class MeritFuncNLPModL1
  : public MeritFuncNLP
  , public MeritFuncNLPDirecDeriv
  , public MeritFuncPenaltyParams
{
public:

  /// Initializes deriv() = 0 and mu() = 0
  MeritFuncNLPModL1();

  /** @name Overridden from MeritFuncNLP */
  //@{

  ///
  value_type value(
    value_type             f
    ,const Vector    *c
    ,const Vector    *h
    ,const Vector    *hl
    ,const Vector    *hu
    ) const;

  ///
  value_type deriv() const;

  ///
  void print_merit_func(
    std::ostream& out, const std::string& leading_str ) const;

  //@}

  /** @name Overridden from MeritFuncNLPDirecDeriv */
  //@{

  ///
  /** If the value n passed to resize(n) does not
    * equal the size of the vector parameters then
    * an exception #MeritFuncNLP::InvalidInitialization#
    * will be thrown.
    */
  value_type calc_deriv(
    const Vector    &Gf_k
    ,const Vector   *c_k
    ,const Vector   *h_k
    ,const Vector   *hl
    ,const Vector   *hu
    ,const Vector   &d_k
    );
  
  //@}

  /** @name Overridden from MeritFuncPenaltyParams */
  //@{

  ///
  void set_space_c( const VectorSpace::space_ptr_t& space_c );

  ///
  VectorMutable& set_mu();

  ///
  const Vector& get_mu() const;

  //@}

private:
  value_type                   deriv_;
  VectorSpace::vec_mut_ptr_t   mu_;

};	// end class MeritFuncNLPModL1

}	// end namespace ConstrainedOptPack

#endif	// MERIT_FUNC_NLP_MOD_L1_H
