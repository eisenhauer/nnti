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
#include "MoochoPack_CalcReducedGradLagrangianStd_AddedStep.hpp"
#include "MoochoPack_NLPAlgoContainer.hpp"
#include "MoochoPack_moocho_algo_conversion.hpp"
#include "IterationPack_print_algorithm_step.hpp"
#include "AbstractLinAlgPack_MatrixOp.hpp"
#include "AbstractLinAlgPack_VectorSpace.hpp"
#include "AbstractLinAlgPack_VectorMutable.hpp"
#include "AbstractLinAlgPack_VectorOut.hpp"
#include "AbstractLinAlgPack_VectorStdOps.hpp"
#include "AbstractLinAlgPack_LinAlgOpPack.hpp"

namespace LinAlgOpPack {
  using AbstractLinAlgPack::Vp_StV;
  using AbstractLinAlgPack::Vp_StMtV;
}

namespace MoochoPack {

bool CalcReducedGradLagrangianStd_AddedStep::do_step(
  Algorithm& _algo, poss_type step_poss, IterationPack::EDoStepType type
  ,poss_type assoc_step_poss
  )
{
  using BLAS_Cpp::trans;
  using LinAlgOpPack::V_VpV;
  using LinAlgOpPack::V_MtV;
  using LinAlgOpPack::Vp_V;
  using LinAlgOpPack::Vp_MtV;

  NLPAlgo	        &algo  = rsqp_algo(_algo);
  NLPAlgoState    &s     = algo.rsqp_state();

  EJournalOutputLevel olevel = algo.algo_cntr().journal_output_level();
  EJournalOutputLevel ns_olevel = algo.algo_cntr().null_space_journal_output_level();
  std::ostream& out = algo.track().journal_out();

  // print step header.
  if( static_cast<int>(ns_olevel) >= static_cast<int>(PRINT_ALGORITHM_STEPS) ) {
    using IterationPack::print_algorithm_step;
    print_algorithm_step( algo, step_poss, type, assoc_step_poss, out );
  }

  // Calculate: rGL = rGf + Z' * nu + Uz' * lambda(equ_undecomp)

  IterQuantityAccess<VectorMutable>
    &rGL_iq  = s.rGL(),
    &nu_iq   = s.nu(),
    &Gf_iq   = s.Gf();

  VectorMutable &rGL_k = rGL_iq.set_k(0);

  if( nu_iq.updated_k(0) && nu_iq.get_k(0).nz() > 0 ) {
    // Compute rGL = Z'*(Gf + nu) to reduce the effect of roundoff in this
    // catastropic cancelation
    const Vector &nu_k = nu_iq.get_k(0);
    VectorSpace::vec_mut_ptr_t
      tmp = nu_k.space().create_member();

    if( (int)olevel >= (int)PRINT_VECTORS )
      out << "\nnu_k = \n" << nu_k;
    V_VpV( tmp.get(), Gf_iq.get_k(0), nu_k );
    if( (int)olevel >= (int)PRINT_VECTORS )
      out << "\nGf_k+nu_k = \n" << *tmp;
    V_MtV(	&rGL_k, s.Z().get_k(0), trans, *tmp );
    if( (int)ns_olevel >= (int)PRINT_VECTORS )
      out << "\nrGL_k = \n" << rGL_k;
  }
  else {
    rGL_k = s.rGf().get_k(0);
  }

  // ToDo: Add terms for undecomposed equalities and inequalities!
  // + Uz' * lambda(equ_undecomp)

  if( static_cast<int>(ns_olevel) >= static_cast<int>(PRINT_ALGORITHM_STEPS) ) {
    out	<< "\n||rGL_k||inf = " << rGL_k.norm_inf() << "\n";
  }

  if( static_cast<int>(ns_olevel) >= static_cast<int>(PRINT_VECTORS) ) {
    out	<< "\nrGL_k = \n" << rGL_k;
  }

  return true;
}

void CalcReducedGradLagrangianStd_AddedStep::print_step(
  const Algorithm& algo
  ,poss_type step_poss, IterationPack::EDoStepType type, poss_type assoc_step_poss
  ,std::ostream& out, const std::string& L
  ) const
{
  out
    << L << "*** Evaluate the reduced gradient of the Lagrangian\n"
    << L << "if nu_k is updated and nu_k.nz() > 0 then\n"
    << L << "    rGL_k = Z_k' * (Gf_k + nu_k) + Uz_k' * lambda_k(equ_undecomp)\n"
    << L << "else\n"
    << L << "    rGL_k = rGf_k + Uz_k' * lambda_k(equ_undecomp)\n"
    << L << "end\n";
}

} // end namespace MoochoPack
