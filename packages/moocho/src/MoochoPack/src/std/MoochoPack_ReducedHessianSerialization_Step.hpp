// @HEADER
// ***********************************************************************
// 
// Moocho: Multi-functional Object-Oriented arCHitecture for Optimization
//                  Copyright (2003) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Roscoe A. Bartlett (rabartl@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef REDUCED_HESSIAN_SERIALIZATION_STEP_HPP
#define REDUCED_HESSIAN_SERIALIZATION_STEP_HPP

#include "MoochoPack_ReducedHessianSecantUpdate_Strategy.hpp"
#include "MoochoPack_quasi_newton_stats.hpp"
#include "IterationPack_AlgorithmStep.hpp"
#include "Teuchos_StandardCompositionMacros.hpp"
#include "Teuchos_StandardMemberCompositionMacros.hpp"

namespace MoochoPack {

/** \brief Serializes rHL_k to and from a file.
 *
 * ToDo: Finish documentation!
 */
class ReducedHessianSerialization_Step
  : public IterationPack::AlgorithmStep // doxygen needs full path
{
public:

  /// Pick the file name to read in the reduced Hessian from
  STANDARD_MEMBER_COMPOSITION_MEMBERS( std::string, reduced_hessian_input_file_name );

  /// Pick the file name to write in the reduced Hessian to
  STANDARD_MEMBER_COMPOSITION_MEMBERS( std::string, reduced_hessian_output_file_name );

  /** \brief . */
  ReducedHessianSerialization_Step(
    const std::string    &reduced_hessian_input_file_name   = "reduced_hessian.in"
    ,const std::string   &reduced_hessian_output_file_name  = "reduced_hessian.out"
    );
  
  /** @name Overridden from AlgorithmStep */
  //@{
  /** \brief . */
  bool do_step(
    Algorithm& algo, poss_type step_poss, IterationPack::EDoStepType type
    ,poss_type assoc_step_poss
    );
  /** \brief . */
  void finalize_step(
    Algorithm& algo, poss_type step_poss, IterationPack::EDoStepType type
    ,poss_type assoc_step_poss
    );
  /** \brief . */
  void print_step(
    const Algorithm& algo, poss_type step_poss, IterationPack::EDoStepType type
    ,poss_type assoc_step_poss, std::ostream& out, const std::string& leading_str
    ) const;
  //@}
  
};	// end class ReducedHessianSerialization_Step

}	// end namespace MoochoPack 

#endif	// REDUCED_HESSIAN_SERIALIZATION_STEP_HPP
