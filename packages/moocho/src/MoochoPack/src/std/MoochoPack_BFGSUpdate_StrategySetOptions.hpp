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

#ifndef REDUCED_HESSIAN_BFGS_STD_STEP_SET_OPTIONS_H
#define REDUCED_HESSIAN_BFGS_STD_STEP_SET_OPTIONS_H

#include "MoochoPack_BFGSUpdate_Strategy.hpp"
#include "OptionsFromStreamPack_SetOptionsFromStreamNode.hpp"
#include "OptionsFromStreamPack_SetOptionsToTargetBase.hpp"

namespace MoochoPack {

///
/** Set options for BFGSUpdate_Strategy from an OptionsFromStream
  * object.
  *
  * The default options group name is BFGSUpdate.
  *
  * The options group is:
  *
  \begin{verbatim}
  options_group BFGSUpdate {
      rescale_init_identity   = true;
      use_dampening           = true;
      secant_testing          = DEFAULT;
  *    secant_testing          = TEST;
  *    secant_testing          = NO_TEST;
      secant_warning_tol      = 1e-6;
      secant_error_tol        = 1e-2;
  }
  \end{verbatim}
  */
class BFGSUpdate_StrategySetOptions
  : public OptionsFromStreamPack::SetOptionsFromStreamNode 
    , public OptionsFromStreamPack::SetOptionsToTargetBase<
      BFGSUpdate_Strategy >
{
public:

  ///
  BFGSUpdate_StrategySetOptions(
      BFGSUpdate_Strategy* target = 0
    , const char opt_grp_name[] = "BFGSUpdate" );

protected:

  /// Overridden from SetOptionsFromStreamNode
  void setOption( int option_num, const std::string& option_value );

};	// end class BFGSUpdate_StrategySetOptions

}	// end namespace MoochoPack

#endif	// REDUCED_HESSIAN_BFGS_STD_STEP_SET_OPTIONS_H
