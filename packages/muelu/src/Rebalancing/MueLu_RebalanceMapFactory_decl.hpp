// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2013 Sandia Corporation
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
// Questions? Contact
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef MUELU_REBALANCEMAPFACTORY_DECL_HPP_
#define MUELU_REBALANCEMAPFACTORY_DECL_HPP_

#include <Xpetra_MapFactory.hpp>
#include <Xpetra_VectorFactory.hpp>
#include <Xpetra_Import.hpp>
#include <Xpetra_ImportFactory.hpp>

#include "MueLu_SingleLevelFactoryBase.hpp"
#include "MueLu_RebalanceMapFactory_fwd.hpp"

#include "MueLu_Level_fwd.hpp"
#include "MueLu_FactoryBase_fwd.hpp"
#include "MueLu_Utilities_fwd.hpp"

namespace MueLu {

  /*!
    @class RebalanceMapFactory
    @brief Factory which rebalances a map on current level using the Importer object generated by the RepartitionFactory
  */

  template <class LocalOrdinal = Xpetra::Map<>::local_ordinal_type,
            class GlobalOrdinal = typename Xpetra::Map<LocalOrdinal>::global_ordinal_type,
            class Node = typename Xpetra::Map<LocalOrdinal, GlobalOrdinal>::node_type>
  class RebalanceMapFactory : public SingleLevelFactoryBase {
    typedef double Scalar; // FIXME this is just a dummy scalar for being able to access Xpetra::Matrix

#undef MUELU_REBALANCEMAPFACTORY_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors/Destructors
    //@{

    //! Constructor
    RebalanceMapFactory() {}

    //! Destructor
    virtual ~RebalanceMapFactory() { }
    //@}

    RCP<const ParameterList> GetValidParameterList() const;

    //! @name Input
    //@{
    void DeclareInput(Level & level) const;
    //@}

    //! @name Build methods.
    //@{
    void Build(Level &level) const;

    //@}
  };

} //namespace MueLu

#define MUELU_REBALANCEMAPFACTORY_SHORT

#endif /* MUELU_REBALANCEMAPFACTORY_DECL_HPP_ */
