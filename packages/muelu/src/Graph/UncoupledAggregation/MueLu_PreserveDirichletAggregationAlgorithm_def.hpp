// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
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
/*
 * MueLu_PreserveDirichletAggregationAlgorithm_def.hpp
 *
 *  Created on: 12 Nov 2013
 *      Author: wiesner
 */

#ifndef MUELU_PRESERVEDIRICHLETAGGREGATIONALGORITHM_DEF_HPP_
#define MUELU_PRESERVEDIRICHLETAGGREGATIONALGORITHM_DEF_HPP_

#include <Teuchos_Comm.hpp>
#include <Teuchos_CommHelpers.hpp>

#include <Xpetra_Vector.hpp>

#include "MueLu_PreserveDirichletAggregationAlgorithm.hpp"

#include "MueLu_GraphBase.hpp"
#include "MueLu_Aggregates.hpp"
#include "MueLu_Exceptions.hpp"
#include "MueLu_Monitor.hpp"

namespace MueLu {

template <class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
PreserveDirichletAggregationAlgorithm<LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::PreserveDirichletAggregationAlgorithm(RCP<const FactoryBase> const &graphFact)
{
}

template <class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
void PreserveDirichletAggregationAlgorithm<LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::BuildAggregates(Teuchos::ParameterList const & params, GraphBase const & graph, Aggregates & aggregates, std::vector<unsigned>& aggStat, LO& numNonAggregatedNodes) const {
  Monitor m(*this, "BuildAggregates");

  // form new aggregates from non-aggregated nodes

  // vertex ids for output
  Teuchos::ArrayRCP<LocalOrdinal> vertex2AggId = aggregates.GetVertex2AggId()->getDataNonConst(0);
  Teuchos::ArrayRCP<LocalOrdinal> procWinner   = aggregates.GetProcWinner()->getDataNonConst(0);

  const int myRank = graph.GetComm()->getRank();
  const LocalOrdinal nRows = graph.GetNodeNumVertices();
  LocalOrdinal nLocalAggregates = aggregates.GetNumAggregates();    // number of local aggregates on current proc

  // loop over all local rows
  for (LocalOrdinal iNode=0; iNode<nRows; iNode++) {
    if (aggStat[iNode] == NodeStats::BOUNDARY) {

      aggregates.SetIsRoot(iNode);    // mark iNode as root node for new aggregate 'ag'
      Aggregate ag;
      ag.list.push_back(iNode);
      ag.index = nLocalAggregates++;

      // finalize aggregate
      for(size_t k=0; k<ag.list.size(); k++) {
        aggStat[ag.list[k]] = NodeStats::AGGREGATED;
        vertex2AggId[ag.list[k]] = ag.index;
        procWinner[ag.list[k]] = myRank;
      }
      numNonAggregatedNodes -= ag.list.size();

    } else if(aggStat[iNode] != NodeStats::AGGREGATED) { // find unaggregated nodes

      Teuchos::ArrayView<const LocalOrdinal> neighOfINode = graph.getNeighborVertices(iNode);
      if(neighOfINode.size() == 1) {
        // build aggregate for lonely Dirichlet node
        aggregates.SetIsRoot(iNode);    // mark iNode as root node for new aggregate 'ag'
        Aggregate ag;
        ag.list.push_back(iNode);
        ag.index = nLocalAggregates++;

        // finalize aggregate
        for(size_t k=0; k<ag.list.size(); k++) {
          aggStat[ag.list[k]] = NodeStats::AGGREGATED;
          vertex2AggId[ag.list[k]] = ag.index;
          procWinner[ag.list[k]] = myRank;
        }
        numNonAggregatedNodes -= ag.list.size();
      }
    }
  }   // end for

  // update aggregate object
  aggregates.SetNumAggregates(nLocalAggregates);

  // print aggregation information
  this->PrintAggregationInformation("Phase 3 (Keep Dirichlet nodes):", graph, aggregates, aggStat);
}

} // end namespace

#endif /* MUELU_PRESERVEDIRICHLETAGGREGATIONALGORITHM_DEF_HPP_ */
