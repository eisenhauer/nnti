// ///////////////////////////////////////////////////////
// ReducedHessianSecantUpdateLPBFGS_Strategy.hpp
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

#ifndef REDUCED_HESSIAN_SECANT_UPDATE_LPBFGS_STRATEGY_H
#define REDUCED_HESSIAN_SECANT_UPDATE_LPBFGS_STRATEGY_H

#include "ReducedSpaceSQPPack/src/std/ReducedHessianSecantUpdateBFGSProjected_Strategy.hpp"
#include "ReducedSpaceSQPPack/src/std/BFGSUpdate_Strategy.hpp"
#include "ReducedSpaceSQPPack/src/std/quasi_newton_stats.hpp"
#include "ReducedSpaceSQPPack/src/std/act_set_stats.hpp"
#include "ConstrainedOptimizationPack/src/MatrixHessianSuperBasic.hpp"
#include "StandardCompositionMacros.hpp"
#include "StandardMemberCompositionMacros.hpp"

namespace ReducedSpaceSQPPack {

///
/** Perform BFGS updates on only the free independent (super basic) variables.
 *
 * This method should be very efficient for few super basic variables.
 */
class ReducedHessianSecantUpdateLPBFGS_Strategy : public ReducedHessianSecantUpdate_Strategy
{
public:
	
	///
	/** <<std comp>> members for the strategy object that will
	 * perform dense projected BFGS updating.
	 */
	STANDARD_COMPOSITION_MEMBERS( ReducedHessianSecantUpdateBFGSProjected_Strategy, proj_bfgs_updater )

	///
	/** Set the minimum number of BFGS updates to perform on the LBFGS matrix
	 * before considering switching to projected BFGS updating.
	 */
	STANDARD_MEMBER_COMPOSITION_MEMBERS( size_type, min_num_updates_proj_start )

	///
	/** Set the maximum number of BFGS updates to perform on the LBFGS matrix
	 * before automatically switching to the projected BFGS updating
	 * reguardless if the active set has calmed down or not.
	 */
	STANDARD_MEMBER_COMPOSITION_MEMBERS( size_type, max_num_updates_proj_start )

	///
	/** Set the maximum number of superbasic variables under which switching
	 * from limited memory to dense projected PBFGS updating will be allowed.
	 */
	STANDARD_MEMBER_COMPOSITION_MEMBERS( size_type, num_superbasics_switch_dense )

	///
	/** Set maximum number of previous BFGS updates to initialize the new dense
	 * projected BFGS matrix with.
	 */
	STANDARD_MEMBER_COMPOSITION_MEMBERS( size_type, num_add_recent_updates )

	///
    ReducedHessianSecantUpdateLPBFGS_Strategy(
		const proj_bfgs_updater_ptr_t&  proj_bfgs_updater            = NULL
		,size_type                      min_num_updates_proj_start   = 0
		,size_type                      max_num_updates_proj_start   = 999999
		,size_type                      num_superbasics_switch_dense = 500
		,size_type                      num_add_recent_updates       = 10
		);

	///
	bool perform_update(
		VectorSlice* s_bfgs, VectorSlice* y_bfgs, bool first_update
		,std::ostream& out, EJournalOutputLevel olevel, rSQPAlgo *algo, rSQPState *s
		,MatrixWithOp *rHL_k
		);
	///
	void print_step( std::ostream& out, const std::string& leading_str ) const;

private:
	
	// //////////////////////////////
	// Private types

	// /////////////////////////////
	// Private data members

	quasi_newton_stats_iq_member    quasi_newton_stats_;
	act_set_stats_iq_member         act_set_stats_;

}; // end class ReducedHessianSecantUpdateLPBFGS_Strategy

}  // end namespace ReducedSpaceSQPPack

#endif // REDUCED_HESSIAN_SECANT_UPDATE_LPBFGS_STRATEGY_H
