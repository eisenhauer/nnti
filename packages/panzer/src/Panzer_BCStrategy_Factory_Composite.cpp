// @HEADER
// ***********************************************************************
//
//           Panzer: A partial differential equation assembly
//       engine for strongly coupled complex multiphysics systems
//                 Copyright (2011) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
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
// Questions? Contact Roger P. Pawlowski (rppawlo@sandia.gov) and
// Eric C. Cyr (eccyr@sandia.gov)
// ***********************************************************************
// @HEADER

#include "Panzer_BCStrategy_Factory_Composite.hpp"
#include "Panzer_BC.hpp"

namespace panzer {
  
  BCFactoryComposite::BCFactoryComposite(const std::vector<Teuchos::RCP<panzer::BCStrategyFactory> >& factories) :
    m_bc_strategy_factories(factories)
  {
    
  }
  
  Teuchos::RCP<panzer::BCStrategy_TemplateManager<panzer::Traits> >
  BCFactoryComposite::
  buildBCStrategy(const panzer::BC& bc, 
		  const Teuchos::RCP<panzer::GlobalData>& global_data) const
  {    
    Teuchos::RCP<panzer::BCStrategy_TemplateManager<panzer::Traits> > bcs_tm;
    
    bool found = false;

    for (std::vector<Teuchos::RCP<panzer::BCStrategyFactory> >::const_iterator factory = m_bc_strategy_factories.begin(); 
         factory != m_bc_strategy_factories.end(); ++factory) {

      bcs_tm = (*factory)->buildBCStrategy(bc,global_data);
      
      if (nonnull(bcs_tm)) {
        found = true;
        break;
      }

    }
        
    TEUCHOS_TEST_FOR_EXCEPTION(!found, std::logic_error, 
			       "Error - the BC Strategy called \"" << bc.strategy() <<
			       "\" is not a valid identifier in the BCStrategyFactory.  Either add " <<
                               "a valid implementation to the factory or fix the input file.  The " <<
                               "relevant boundary condition is:\n\n" << bc << std::endl);
    
    return bcs_tm;
    
  }
  
}
