// ///////////////////////////////////////////////////////////////////////////////////////
// QPSolverRelaxed.cpp
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

#include <assert.h>

#include <limits>

#include "ConstrainedOptPack/src/qpsolvers/QPSolverRelaxed.hpp"
#include "AbstractLinAlgPack/src/abstract/interfaces/MatrixSymOp.hpp"
#include "AbstractLinAlgPack/src/abstract/interfaces/MatrixOpOut.hpp"
#include "AbstractLinAlgPack/src/abstract/interfaces/VectorMutable.hpp"
#include "AbstractLinAlgPack/src/abstract/interfaces/VectorOut.hpp"
#include "AbstractLinAlgPack/src/abstract/tools/VectorAuxiliaryOps.hpp"
#include "MoochoMoreUtilities/src/profile_hack.hpp"
#include "Teuchos_TestForException.hpp"

namespace ConstrainedOptPack {

// public

QPSolverRelaxed::QPSolverRelaxed()
	:infinite_bound_(std::numeric_limits<value_type>::max())
{}

QPSolverStats::ESolutionType
QPSolverRelaxed::solve_qp(
	std::ostream* out, EOutputLevel olevel, ERunTests test_what
	,const Vector& g, const MatrixSymOp& G
	,value_type etaL
	,const Vector& dL, const Vector& dU
	,const MatrixOp& E, BLAS_Cpp::Transp trans_E, const Vector& b
	,const Vector& eL, const Vector& eU
	,const MatrixOp& F, BLAS_Cpp::Transp trans_F, const Vector& f
	,value_type* obj_d
	,value_type* eta, VectorMutable* d
	,VectorMutable* nu
	,VectorMutable* mu, VectorMutable* Ed
	,VectorMutable* lambda, VectorMutable* Fd
	)
{
	return solve_qp(out,olevel,test_what,g,G,etaL,&dL,&dU
		,&E,trans_E,&b,&eL,&eU,&F,trans_F,&f
		,obj_d,eta,d,nu,mu,Ed,lambda,Fd);
}

QPSolverStats::ESolutionType
QPSolverRelaxed::solve_qp(
	std::ostream* out, EOutputLevel olevel, ERunTests test_what
	,const Vector& g, const MatrixSymOp& G
	,value_type etaL
	,const Vector& dL, const Vector& dU
	,const MatrixOp& E, BLAS_Cpp::Transp trans_E, const Vector& b
	,const Vector& eL, const Vector& eU
	,value_type* obj_d
	,value_type* eta, VectorMutable* d
	,VectorMutable* nu
	,VectorMutable* mu, VectorMutable* Ed
	)
{
	return solve_qp(out,olevel,test_what,g,G,etaL,&dL,&dU
		,&E,trans_E,&b,&eL,&eU,NULL,BLAS_Cpp::no_trans,NULL
		,obj_d,eta,d,nu,mu,Ed,NULL,NULL);
}

QPSolverStats::ESolutionType
QPSolverRelaxed::solve_qp(
	std::ostream* out, EOutputLevel olevel, ERunTests test_what
	,const Vector& g, const MatrixSymOp& G
	,value_type etaL
	,const Vector& dL, const Vector& dU
	,const MatrixOp& F, BLAS_Cpp::Transp trans_F, const Vector& f
	,value_type* obj_d
	,value_type* eta, VectorMutable* d
	,VectorMutable* nu
	,VectorMutable* lambda, VectorMutable* Fd
	)
{
	return solve_qp(out,olevel,test_what,g,G,etaL,&dL,&dU
		,NULL,BLAS_Cpp::no_trans,NULL,NULL,NULL,&F,trans_F,&f
		,obj_d,eta,d,nu,NULL,NULL,lambda,Fd);
}


QPSolverStats::ESolutionType
QPSolverRelaxed::solve_qp(
	std::ostream* out, EOutputLevel olevel, ERunTests test_what
	,const Vector& g, const MatrixSymOp& G
	,const Vector& dL, const Vector& dU
	,value_type* obj_d
	,VectorMutable* d
	,VectorMutable* nu
	)
{
	value_type eta = 0, etaL = 0;
	return solve_qp(
		out,olevel,test_what,g,G,etaL,&dL,&dU
		,NULL,BLAS_Cpp::no_trans,NULL,NULL,NULL
		,NULL,BLAS_Cpp::no_trans,NULL
		,obj_d,&eta,d,nu,NULL,NULL,NULL,NULL);
}

QPSolverStats::ESolutionType
QPSolverRelaxed::solve_qp(
	std::ostream* out, EOutputLevel olevel, ERunTests test_what
	,const Vector& g, const MatrixSymOp& G
	,value_type etaL
	,const Vector* dL, const Vector* dU
	,const MatrixOp* E, BLAS_Cpp::Transp trans_E, const Vector* b
	,const Vector* eL, const Vector* eU
	,const MatrixOp* F, BLAS_Cpp::Transp trans_F, const Vector* f
	,value_type* obj_d
	,value_type* eta, VectorMutable* d
	,VectorMutable* nu
	,VectorMutable* mu, VectorMutable* Ed
	,VectorMutable* lambda, VectorMutable* Fd
	)
{
#ifdef PROFILE_HACK_ENABLED
	ProfileHackPack::ProfileTiming profile_timing( "QPSolverRelaxed::solve_qp(...)" );
#endif
	validate_input(
		infinite_bound(),g,G,etaL,dL,dU
		,E,trans_E,b,eL,eU,F,trans_F,f
		,obj_d,eta,d,nu,mu,Ed,lambda,Fd);
	print_qp_input(
		infinite_bound(),out,olevel,g,G,etaL,dL,dU,E,trans_E,b,eL,eU
		,F,trans_F,f,eta,d,nu,mu,lambda	);
	QPSolverStats::ESolutionType
		solve_return = imp_solve_qp(
			out,olevel,test_what,g,G,etaL,dL,dU
			,E,trans_E,b,eL,eU,F,trans_F,f
			,obj_d,eta,d,nu,mu,Ed,lambda,Fd);
	print_qp_output(
		infinite_bound(),out,olevel,obj_d,eta,d,nu,mu,Ed,lambda,Fd);
	return solve_return;
}

void QPSolverRelaxed::validate_input(
	const value_type infinite_bound
	,const Vector& g, const MatrixSymOp& G
	,value_type etaL
	,const Vector* dL, const Vector* dU
	,const MatrixOp* E, BLAS_Cpp::Transp trans_E, const Vector* b
	,const Vector* eL, const Vector* eU
	,const MatrixOp* F, BLAS_Cpp::Transp trans_F, const Vector* f
	,const value_type* obj_d
	,const value_type* eta, const Vector* d
	,const Vector* nu
	,const Vector* mu, const Vector* Ed
	,const Vector* lambda, const Vector* Fd
	)
{
	// Validate output arguments
	TEST_FOR_EXCEPTION(
		!d, std::invalid_argument
		,"QPSolverRelaxed::validate_input(...) : Error, "
		"If d!=NULL is not allowed." );
	TEST_FOR_EXCEPTION(
		( E || F ) && !eta, std::invalid_argument
		,"QPSolverRelaxed::validate_input(...) : Error, "
		"If eta!=NULL is not allowed if E!=NULL or F!=NULL." );

	// Validate the sets of constraints arguments
	TEST_FOR_EXCEPTION(
		dL && ( !dU || !nu ), std::invalid_argument
		,"QPSolverRelaxed::validate_input(...) : Error, "
		"If dL!=NULL then dU!=NULL and nu!=NULL must also be true." );
	TEST_FOR_EXCEPTION(
		E && ( !b || !eL || !eU || !mu ), std::invalid_argument
		,"QPSolverRelaxed::validate_input(...) : Error, "
		"If E!=NULL then b!=NULL, eL!=NULL, eU!=NULL and mu!=NULL must also "
		"be true." );
	TEST_FOR_EXCEPTION(
		F && ( !f || !lambda ), std::invalid_argument
		,"QPSolverRelaxed::validate_input(...) : Error, "
		"If F!=NULL then f!=NULL and lambda!=NULL must also "
		"be true." );

	// ToDo: Replace the below code with checks of compatibility of the vector spaces!

	// Validate the sizes of the arguments
	const size_type
		nd = d->dim();
	TEST_FOR_EXCEPTION(
		g.dim() != nd, std::invalid_argument
		,"QPSolverRelaxed::validate_input(...) : Error, "
		"g.dim() != d->dim()." );
	TEST_FOR_EXCEPTION(
		G.rows() != nd || G.cols() != nd, std::invalid_argument
		,"QPSolverRelaxed::validate_input(...) : Error, "
		"G.rows() != d->dim() or G.cols() != d->dim()." );
	TEST_FOR_EXCEPTION(
		dL && dL->dim() != nd, std::invalid_argument
		,"QPSolverRelaxed::validate_input(...) : Error, "
		"dL->dim() = " << dL->dim() << " != d->dim() = " << nd << "." );
	TEST_FOR_EXCEPTION(
		dU && dU->dim() != nd, std::invalid_argument
		,"QPSolverRelaxed::validate_input(...) : Error, "
		"dU->dim() = " << dU->dim() << " != d->dim() = " << nd << "." );
	if( E ) {
		const size_type
			m_in = BLAS_Cpp::rows( E->rows(), E->cols(), trans_E );
		TEST_FOR_EXCEPTION(
			BLAS_Cpp::cols( E->rows(), E->cols(), trans_E )	!= nd, std::invalid_argument
			,"QPSolverRelaxed::validate_input(...) : Error, op(E).cols() != d->dim()." );
		TEST_FOR_EXCEPTION(
			b->dim() != m_in, std::invalid_argument
			,"QPSolverRelaxed::validate_input(...) : Error, b->dim() != op(E).rows()." );
		TEST_FOR_EXCEPTION(
			eL->dim() != m_in, std::invalid_argument
			,"QPSolverRelaxed::validate_input(...) : Error, eL->dim() != op(E).rows()." );
		TEST_FOR_EXCEPTION(
			eU->dim() != m_in, std::invalid_argument
			,"QPSolverRelaxed::validate_input(...) : Error, eU->dim() != op(E).rows()." );
		TEST_FOR_EXCEPTION(
			Ed && Ed->dim() != m_in, std::invalid_argument
			,"QPSolverRelaxed::validate_input(...) : Error, Ed->dim() != op(E).rows()." );
	}
	if( F ) {
		const size_type
			m_eq = BLAS_Cpp::rows( F->rows(), F->cols(), trans_F );
		TEST_FOR_EXCEPTION(
			BLAS_Cpp::cols( F->rows(), F->cols(), trans_F )	!= nd, std::invalid_argument
			,"QPSolverRelaxed::validate_input(...) : Error, op(F).cols() != d->dim()." );
		TEST_FOR_EXCEPTION(
			f->dim() != m_eq, std::invalid_argument
			,"QPSolverRelaxed::validate_input(...) : Error, f->dim() != op(F).rows()." );
		TEST_FOR_EXCEPTION(
			lambda->dim() != m_eq, std::invalid_argument
			,"QPSolverRelaxed::validate_input(...) : Error, lambda->dim() != op(F).rows()." );
		TEST_FOR_EXCEPTION(
			Fd && Fd->dim() != m_eq, std::invalid_argument
			,"QPSolverRelaxed::validate_input(...) : Error, Fd->dim() != op(F).rows()." );
	}
}

void QPSolverRelaxed::print_qp_input( 
	const value_type infinite_bound
	,std::ostream* out, EOutputLevel olevel
	,const Vector& g, const MatrixSymOp& G
	,value_type etaL
	,const Vector* dL, const Vector* dU
	,const MatrixOp* E, BLAS_Cpp::Transp trans_E, const Vector* b
	,const Vector* eL, const Vector* eU
	,const MatrixOp* F, BLAS_Cpp::Transp trans_F, const Vector* f
	,value_type* eta, VectorMutable* d
	,VectorMutable* nu
	,VectorMutable* mu
	,VectorMutable* lambda
	)
{
	using AbstractLinAlgPack::num_bounded;
	if( out && (int)olevel >= (int)PRINT_ITER_STEPS ) {
		*out<< "\n*** Printing input to QPSolverRelaxed::solve_qp(...) ...\n";
		// g
		*out << "\n||g||inf = " << g.norm_inf() << std::endl;
		if( (int)olevel >= (int)PRINT_ITER_VECTORS )
			*out<< "g =\n" << g;
		// G
		if( (int)olevel >= (int)PRINT_EVERY_THING )
			*out<< "\nG =\n" << G;
		// etaL
		*out << "\netaL = " << etaL << std::endl;
		// eta
		*out << "\neta  = " << *eta << std::endl;
		if(dL) {
			// dL, dU
			*out << "\ndL.dim()   = " << dL->dim();
			*out << "\ndU.dim()   = " << dU->dim();
			*out << "\nnum_bounded(dL,dU) = " << num_bounded(*dL,*dU,infinite_bound) << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_VECTORS )
				*out << "dL =\n" << *dL;
			if( (int)olevel >= (int)PRINT_ITER_VECTORS )
				*out << "dU =\n" << *dU;
		}
		else {
			*out << "\ndL = -inf";
			*out << "\ndU = +inf";
		}
		// d
		*out << "\n||d||inf = " << d->norm_inf() << std::endl;
		if( (int)olevel >= (int)PRINT_ITER_VECTORS )
			*out<< "d =\n" << *d;
		// nu
		if(nu) {
			*out<< "\nnu->nz() = " << nu->nz() << std::endl
				<< "||nu||inf  = " << nu->norm_inf() << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_ACT_SET )
				*out<< "nu =\n" << *nu;
		}
		if(E) {
			// op(E)
			if( (int)olevel >= (int)PRINT_EVERY_THING )
				*out<< "\nE" << std::endl << *E
					<< "trans_E = " << BLAS_Cpp::trans_to_string(trans_E) << std::endl;
			// b
			*out << "\n||b||inf = " << b->norm_inf() << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_VECTORS )
			*out<< "b =\n" << *b;
			// eL, eU
			*out<< "\neL.dim()   = " << eL->dim();
			*out<< "\neU.dim()   = " << eU->dim();
			*out << "\nnum_bounded(eL,eU) = " << num_bounded(*eL,*eU,infinite_bound) << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_VECTORS )
				*out<< "eL =\n" << *eL;
			if( (int)olevel >= (int)PRINT_ITER_VECTORS )
				*out<< "eU =\n" << *eU;
			// mu
			*out<< "\nmu.nz()   = " << mu->nz() << std::endl
				<< "||mu||inf = " << mu->norm_inf() << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_ACT_SET )
				*out<< "mu =\n" << *mu;
		}
		if(F) {
			// op(F)
			if( (int)olevel >= (int)PRINT_EVERY_THING )
				*out<< "\nF" << std::endl << *F
					<< "trans_F = " << BLAS_Cpp::trans_to_string(trans_F) << std::endl;
			// f
			*out<< "\n||f||inf = " << f->norm_inf() << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_VECTORS )
				*out<< "f =\n" << *f;
			// lambda
			*out<< "\n||lambda||inf = " << lambda->norm_inf() << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_ACT_SET )
				*out<< "lambda =\n" << *lambda;
		}
		*out<< "\nEnd input to QPSolverRelaxed::solve_qp(...)\n";
	}
}

void QPSolverRelaxed::print_qp_output(
	const value_type infinite_bound
	,std::ostream* out, EOutputLevel olevel
	,const value_type* obj_d
	,const value_type* eta, const Vector* d
	,const Vector* nu
	,const Vector* mu, const Vector* Ed
	,const Vector* lambda, const Vector* Fd
	)
{
	if( out && (int)olevel > (int)PRINT_ITER_STEPS ) {
		*out<< "\n*** Printing output from QPSolverRelaxed::solve_qp(...) ...\n";
		// obj_d
		if(obj_d)
			*out << "\nobj_d = " << *obj_d << std::endl;
		// eta
		*out << "\neta = " << *eta << std::endl;
		// d
		*out << "\n||d||inf = " << d->norm_inf() << std::endl;
		if( (int)olevel >= (int)PRINT_ITER_VECTORS )
			*out<< "d =\n" << *d;
		// nu
		if(nu) {
			*out<< "\nnu.nz()   = " << nu->nz() << std::endl
				<< "||nu||inf = " << nu->norm_inf() << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_ACT_SET )
				*out<< "nu =\n" << *nu;
		}
		// Ed
		if(Ed) {
			*out << "\n||Ed||inf = " << Ed->norm_inf() << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_VECTORS )
				*out<< "Ed =\n" << *Ed;
		}
		// mu
		if(mu) {
			*out<< "\nmu.nz()   = " << mu->nz() << std::endl
				<< "||mu||inf = " << mu->norm_inf() << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_ACT_SET )
				*out<< "mu =\n" << *mu;
		}
		// lambda
		if(lambda) {
			*out<< "\n||lambda||inf = " << lambda->norm_inf() << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_ACT_SET )
				*out<< "lambda =\n" << *lambda;
		}
		// Fd
		if(Fd) {
			*out << "\n||Fd||inf = " << Fd->norm_inf() << std::endl;
			if( (int)olevel >= (int)PRINT_ITER_VECTORS )
				*out<< "Fd =\n" << *Fd;
		}
		*out<< "\nEnd output from QPSolverRelaxed::solve_qp(...)\n";
	}
}

}	// end namespace ConstrainedOptPack
