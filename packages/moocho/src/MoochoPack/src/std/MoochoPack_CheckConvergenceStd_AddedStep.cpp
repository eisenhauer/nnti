// ////////////////////////////////////////////////////////////////////////////
// CheckConvergenceStd_AddedStep.cpp

// disable VC 5.0 warnings about debugger limitations
#pragma warning(disable : 4786)	

#include <assert.h>

#include <ostream>
#include <limits>
#include <sstream>

#include "../../include/std/CheckConvergenceStd_AddedStep.h"
#include "../../include/rSQPAlgoContainer.h"
#include "../../include/rsqp_algo_conversion.h"
#include "GeneralIterationPack/include/print_algorithm_step.h"
#include "ConstrainedOptimizationPack/include/VectorWithNorms.h"
#include "LinAlgPack/include/VectorClass.h"
#include "LinAlgPack/include/VectorOp.h"
#include "LinAlgPack/include/VectorOut.h"
#include "SparseLinAlgPack/include/SpVectorClass.h"
#include "LinAlgPack/include/assert_print_nan_inf.h"

namespace ReducedSpaceSQPPack {

CheckConvergenceStd_AddedStep::CheckConvergenceStd_AddedStep(
		  EOptErrorCheck opt_error_check
		, EScaleKKTErrorBy scale_kkt_error_by
		, bool scale_opt_error_by_Gf
		)
	 : opt_error_check_(opt_error_check), scale_kkt_error_by_(scale_kkt_error_by)
	 	, scale_opt_error_by_Gf_(scale_opt_error_by_Gf)
{}

bool CheckConvergenceStd_AddedStep::do_step(Algorithm& _algo
	, poss_type step_poss, GeneralIterationPack::EDoStepType type, poss_type assoc_step_poss)
{
	using LinAlgPack::norm_inf;
	using LinAlgPack::norm_2;
	using LinAlgPack::assert_print_nan_inf;

	rSQPAlgo	&algo	= rsqp_algo(_algo);
	rSQPState	&s		= algo.rsqp_state();
	NLP			&nlp	= algo.nlp();

	EJournalOutputLevel olevel = algo.algo_cntr().journal_output_level();
	std::ostream& out = algo.track().journal_out();

	// print step header.
	if( static_cast<int>(olevel) >= static_cast<int>(PRINT_ALGORITHM_STEPS) ) {
		using GeneralIterationPack::print_algorithm_step;
		print_algorithm_step( algo, step_poss, type, assoc_step_poss, out );
	}

	// scale_kkt_factor
	value_type
		scale_kkt_factor;
	switch(scale_kkt_error_by()) {
		case SCALE_BY_ONE:
			scale_kkt_factor = 1.0;
			break;
		case SCALE_BY_NORM_2_X:
			scale_kkt_factor = 1.0 + s.x().get_k(0).norm_2();
			break;
		case SCALE_BY_NORM_INF_X:
			scale_kkt_factor = 1.0 + s.x().get_k(0).norm_inf();
			break;
		default:
			assert(0);	// Should never be called
	}


	// opt_err = (||rGL||inf or ||GL||) / (||Gf|| + scale_kkt_factor)
	value_type
		norm_inf_Gf_k = 0.0,
		norm_inf_GLrGL_k = 0.0;
	if( scale_opt_error_by_Gf() ) {
		assert_print_nan_inf( norm_inf_Gf_k = s.Gf().get_k(0).norm_inf()
			,"||Gf_k||inf",true,&out);
	}
	if( opt_error_check() == OPT_ERROR_REDUCED_GRADIENT_LAGR ) {
		assert_print_nan_inf( norm_inf_GLrGL_k = s.rGL().get_k(0).norm_inf()
			,"||rGL_k||inf",true,&out);
	}
	else {
		assert_print_nan_inf( norm_inf_GLrGL_k = s.GL().get_k(0).norm_inf()
			,"||GL_k||inf",true,&out);
	}
	const value_type
		opt_scale_factor = 1.0 + norm_inf_Gf_k,
		opt_err = norm_inf_GLrGL_k / opt_scale_factor;

	// feas_err
	const value_type
		feas_err = s.c().get_k(0).norm_inf();
	assert_print_nan_inf( feas_err,"||c_k||inf",true,&out);

	// kkt_err
	const value_type
		opt_kkt_err_k  = s.opt_kkt_err().set_k(0) = opt_err/scale_kkt_factor,
		feas_kkt_err_k = s.feas_kkt_err().set_k(0) = feas_err/scale_kkt_factor;

	// step_err
	value_type
		step_err = 0.0;
	if( s.d().updated_k(0) ) {
		const Vector
			&d = s.d().get_k(0).v(),
			&x = s.x().get_k(0).v();
		Vector::const_iterator
			d_itr = d.begin(),
			x_itr = x.begin();
		while( d_itr != d.end() )
			step_err = std::_MAX( step_err, ::fabs(*d_itr++)/(1.0+::fabs(*x_itr++)) );
	}

	assert_print_nan_inf( step_err,"max(d(i)/max(1,x(i)),i=1...n)",true,&out);

	const value_type
		opt_tol		= algo.algo_cntr().opt_tol(),
		feas_tol	= algo.algo_cntr().feas_tol(),
		step_tol	= algo.algo_cntr().step_tol();

	const bool found_solution = opt_kkt_err_k < opt_tol && feas_kkt_err_k < feas_tol && step_err < step_tol;

	if( int(olevel) >= int(PRINT_ALGORITHM_STEPS) || (int(olevel) >= int(PRINT_BASIC_ALGORITHM_INFO) && found_solution) )
	{
		out	<< "\nscale_kkt_factor = " << scale_kkt_factor
			<< " (scale_kkt_error_by = " << (scale_kkt_error_by()==SCALE_BY_ONE ? "SCALE_BY_ONE"
											 : (scale_kkt_error_by()==SCALE_BY_NORM_INF_X ? "SCALE_BY_NORM_INF_X"
												: "SCALE_BY_NORM_2_X" ) ) << ")"
			<< "\nopt_scale_factor = " << opt_scale_factor
			<< " (scale_opt_error_by_Gf = " << (scale_opt_error_by_Gf()?"true":"false") << ")"
			<< "\nopt_kkt_err_k    = " << opt_kkt_err_k << ( opt_kkt_err_k < opt_tol ? " < " : " > " )
			<< "opt_tol = " << opt_tol
			<< "\nfeas_kkt_err_k   = " << feas_kkt_err_k << ( feas_kkt_err_k < feas_tol ? " < " : " > " )
			<< "feas_tol = " << feas_tol
			<< "\nstep_err         = " << step_err << ( step_err < step_tol ? " < " : " > " )
			<< "step_tol = " << step_tol
			<< std::endl;
		if( found_solution )
			out	<< "\nJackpot!  Found the solution!!!!!! (k = " << algo.state().k() << ")\n";
		else
			out	<< "\nHave not found the solution yet, have to keep going :-(\n";
	}

	if( found_solution ) {
		nlp.report_final_solution(
			  s.x().get_k(0)()
			, s.lambda().updated_k(0)	? &s.lambda().get_k(0)()	: 0
			, s.nu().updated_k(0)		? &s.nu().get_k(0)()		: 0
			, true
			);
		algo.terminate(true);	// found min
		return false; // skip the other steps and terminate
	}

	// We are not at the solution so keep going
	return true;
}

void CheckConvergenceStd_AddedStep::print_step( const Algorithm& algo
	, poss_type step_poss, GeneralIterationPack::EDoStepType type, poss_type assoc_step_poss
	, std::ostream& out, const std::string& L ) const
{
	out
		<< L << "*** Check to see if the KKT error is small enough for convergence\n"
		<< L << "if scale_kkt_error_by == SCALE_BY_ONE then\n"
		<< L << "    scale_kkt_factor = 1.0\n"
		<< L << "else if scale_by == SCALE_BY_NORM_2_X then\n"
		<< L << "    scale_kkt_factor = 1.0 + norm_2(x_k)\n"
		<< L << "else if scale_by == SCALE_BY_NORM_INF_X then\n"
		<< L << "    scale_kkt_factor = 1.0 + norm_inf(x_k)\n"
		<< L << "else\n"
		<< L << "if scale_opt_error_by_Gf == true then\n"
		<< L << "    opt_scale_factor = 1.0 + norm_inf(Gf_k)\n"
		<< L << "else\n"
		<< L << "    opt_scale_factor = 1.0\n"
		<< L << "end\n";
	if( opt_error_check() == OPT_ERROR_REDUCED_GRADIENT_LAGR )
		out
			<< L << "opt_err = norm_inf(rGL_k)/opt_scale_factor\n";
	else
		out
			<< L << "opt_err = norm_inf(GL_k)/opt_scale_factor\n";
	out
		<< L << "feas_err = norm_inf_c_k\n"
		<< L << "opt_kkt_err_k = opt_err/scale_kkt_factor\n"
		<< L << "feas_kkt_err_k = feas_err/scale_kkt_factor\n"
		<< L << "if d_k is updated then\n"
		<< L << "    step_err = max( |d_k(i)|/(1+|x_k(i)|), i=1..n )\n"
		<< L << "else\n"
		<< L << "    step_err = 0\n"
		<< L << "end\n"
		<< L << "if opt_kkt_err_k < opt_tol\n"
		<< L << "       and feas_kkt_err_k < feas_tol\n"
		<< L << "       and step_err < step_tol then\n"
		<< L << "   report optimal x_k, lambda_k and nu_k to the nlp\n"
		<< L << "   terminate, the solution has beed found!\n"
		<< L << "end\n";
}

}	// end namespace ReducedSpaceSQPPack
