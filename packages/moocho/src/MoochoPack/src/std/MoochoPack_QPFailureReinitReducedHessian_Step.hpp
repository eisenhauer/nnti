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

#ifndef QP_FAILURE_REINIT_REDUCED_HESSIAN_STEP_H
#define QP_FAILURE_REINIT_REDUCED_HESSIAN_STEP_H

#include "MoochoPack_Types.hpp"
#include "IterationPack_AlgorithmStep.hpp"
#include "Teuchos_StandardCompositionMacros.hpp"

namespace MoochoPack {

///
/** Directs the algorithm to reinitalize the reduced Hessian on the event
 * of a QP failure.
 *
 * If the delegated Step object throws a \c QPFailure exception
 * then this Step object wipes out all reduced Hessian info rHL
 * for the current and previous iterations and then directs the algorithm
 * back to the ReducedHessian step (see the printed step description).
 */
class QPFailureReinitReducedHessian_Step
	: public IterationPack::AlgorithmStep // doxygen needs full path
{
public:

	///
	STANDARD_COMPOSITION_MEMBERS( IterationPack::AlgorithmStep, null_space_step )

	///
	QPFailureReinitReducedHessian_Step( const null_space_step_ptr_t& null_space_step );

	// ////////////////////
	// Overridden

	///
	bool do_step(Algorithm& algo, poss_type step_poss, IterationPack::EDoStepType type
		, poss_type assoc_step_poss);
	///
	void print_step( const Algorithm& algo, poss_type step_poss, IterationPack::EDoStepType type
		, poss_type assoc_step_poss, std::ostream& out, const std::string& leading_str ) const;

private:
	int last_qp_failure_k_;

	// not defined and not to be called
	QPFailureReinitReducedHessian_Step();
	QPFailureReinitReducedHessian_Step(const QPFailureReinitReducedHessian_Step&);
	QPFailureReinitReducedHessian_Step& operator=(const QPFailureReinitReducedHessian_Step&);
	
};	// end class QPFailureReinitReducedHessian_Step

}	// end namespace MoochoPack 

#endif	// QP_FAILURE_REINIT_REDUCED_HESSIAN_STEP_H
