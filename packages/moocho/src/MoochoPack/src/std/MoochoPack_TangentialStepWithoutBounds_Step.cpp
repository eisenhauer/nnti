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

#include <ostream>

#include "MoochoPack_TangentialStepWithoutBounds_Step.hpp"
#include "MoochoPack_EvalNewPointTailoredApproach_Step.hpp"
#include "MoochoPack_Exceptions.hpp"
#include "MoochoPack_moocho_algo_conversion.hpp"
#include "IterationPack_print_algorithm_step.hpp"
#include "NLPInterfacePack_NLPDirect.hpp"
#include "AbstractLinAlgPack_MatrixSymOpNonsing.hpp"
#include "AbstractLinAlgPack_MatrixOpOut.hpp"
#include "AbstractLinAlgPack_VectorMutable.hpp"
#include "AbstractLinAlgPack_VectorStdOps.hpp"
#include "AbstractLinAlgPack_VectorOut.hpp"
#include "AbstractLinAlgPack_assert_print_nan_inf.hpp"
#include "AbstractLinAlgPack_LinAlgOpPack.hpp"
#include "Teuchos_dyn_cast.hpp"
#include "Teuchos_TestForException.hpp"

namespace LinAlgOpPack {
	using AbstractLinAlgPack::Vp_StMtV;
}

namespace MoochoPack {

bool TangentialStepWithoutBounds_Step::do_step(
	Algorithm& _algo, poss_type step_poss, IterationPack::EDoStepType type
	,poss_type assoc_step_poss
	)
{
	using BLAS_Cpp::no_trans;
	using Teuchos::dyn_cast;
	using AbstractLinAlgPack::assert_print_nan_inf;
	using AbstractLinAlgPack::Vt_S;
	using AbstractLinAlgPack::Vp_StV;
	using AbstractLinAlgPack::V_InvMtV;
	using LinAlgOpPack::V_StV;
	using LinAlgOpPack::V_MtV;

	NLPAlgo	&algo	= rsqp_algo(_algo);
	NLPAlgoState	&s		= algo.rsqp_state();

	EJournalOutputLevel olevel = algo.algo_cntr().journal_output_level();
	EJournalOutputLevel ns_olevel = algo.algo_cntr().null_space_journal_output_level();
	std::ostream& out = algo.track().journal_out();

	// print step header.
	if( static_cast<int>(olevel) >= static_cast<int>(PRINT_ALGORITHM_STEPS) ) {
		using IterationPack::print_algorithm_step;
		print_algorithm_step( algo, step_poss, type, assoc_step_poss, out );
	}

	if( algo.nlp().num_bounded_x() )
		TEST_FOR_EXCEPTION(
			true, std::logic_error
			,"TangentialStepWithoutBounds_Step::do_step(...): Error, "
			"can't solve for pz for NLP with undecomposed constraints or "
			"has bounds on the variables");

	// Comupte qp_grad which is an approximation to rGf + Z' * HL * Y * py

	// qp_grad_k = rGf_k
	VectorMutable &qp_grad_k = s.qp_grad().set_k(0) = s.rGf().get_k(0);

	IterQuantityAccess<value_type>               &zeta_iq       = s.zeta();
	IterQuantityAccess<VectorMutable>      &w_iq          = s.w();
	if( w_iq.updated_k(0) && zeta_iq.updated_k(0) ) {
		// qp_grad += zeta * w
		Vp_StV( &qp_grad_k, zeta_iq.get_k(0), w_iq.get_k(0) );
	}

	// Solve the system pz = - inv(rHL) * qp_grad
	VectorMutable               &pz_k  = s.pz().set_k(0);
	const MatrixSymOpNonsing  &rHL_k = dyn_cast<MatrixSymOpNonsing>(s.rHL().get_k(0));
	V_InvMtV( &pz_k, rHL_k, no_trans, qp_grad_k );
	Vt_S( &pz_k, -1.0 );

	// nu = 0.0
	s.nu().set_k(0) = 0.0;

	// Zpz = Z * pz
	V_MtV( &s.Zpz().set_k(0), s.Z().get_k(0), no_trans, pz_k );

	if( static_cast<int>(olevel) >= static_cast<int>(PRINT_ALGORITHM_STEPS) ) {
		out	<< "\n||pz_k||inf   = " << s.pz().get_k(0).norm_inf()
			<< "\n||Zpz_k||2    = " << s.Zpz().get_k(0).norm_2()  << std::endl;
	}

	if( static_cast<int>(ns_olevel) >= static_cast<int>(PRINT_VECTORS) ) {
		out << "\npz_k = \n" << s.pz().get_k(0);
		out << std::endl;
	}

	if( static_cast<int>(olevel) >= static_cast<int>(PRINT_VECTORS) ) {
		out << "\nnu_k = \n" << s.nu().get_k(0);
		out << "\nZpz_k = \n" << s.Zpz().get_k(0);
		out << std::endl;
	}

	if(algo.algo_cntr().check_results()) {
		assert_print_nan_inf(s.pz().get_k(0),  "pz_k",true,&out);
		assert_print_nan_inf(s.Zpz().get_k(0), "Zpz_k",true,&out);
	}

	return true;
}

void TangentialStepWithoutBounds_Step::print_step( const Algorithm& algo
	, poss_type step_poss, IterationPack::EDoStepType type, poss_type assoc_step_poss
	, std::ostream& out, const std::string& L ) const
{
	out
		<< L << "*** Calculate the null space step by solving an unconstrainted QP\n"
		<< L << "qp_grad_k = rGf_k + zeta_k * w_k\n"
		<< L << "solve:\n"
		<< L << "    min     qp_grad_k' * pz_k + 1/2 * pz_k' * rHL_k * pz_k\n"
		<< L << "    pz_k <: R^(n-r)\n"
		<< L << "Zpz_k = Z_k * pz_k\n"
		<< L << "nu_k = 0\n";
}

} // end namespace MoochoPack
