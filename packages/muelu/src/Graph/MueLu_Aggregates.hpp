#ifndef MUELU_AGGREGATES_HPP
#define MUELU_AGGREGATES_HPP

#include <Teuchos_Describable.hpp>

#include "MueLu_Graph.hpp"

#define MUELU_UNAGGREGATED  -1   /* indicates that a node is unassigned to  */
                                 /* any aggregate.                          */

#define MUELU_UNASSIGNED    -1   /* indicates a vertex is not yet claimed   */
                                 /* by a processor during aggregation.      */
                                 /* Note, it is possible at                 */
                                 /* this stage that some processors may have*/
                                 /* claimed their copy of a vertex for one  */
                                 /* of their aggregates.  However, some     */
                                 /* arbitration still needs to occur.       */
                                 /* The corresponding procWinner[]'s remain */
                                 /* as MUELU_UNASSIGNED until               */
                                 /* MueLu_ArbitrateAndCommunicate() is      */
                                 /* invoked to arbitrate.                   */

/***************************************************************************** 
   Structure holding aggregate information. Right now, nAggregates, IsRoot,
   Vertex2AggId, procWinner are populated.  This allows us to look at a node
   and determine the aggregate to which it has been assigned and the id of the 
   processor that owns this aggregate. It is not so easy to determine vertices
   within the kth aggregate or the size of the kth aggregate. Thus, it might be
   useful to have a secondary structure which would be a rectangular CrsGraph 
   where rows (or vertices) correspond to aggregates and colunmns (or edges) 
   correspond to nodes. While not strictly necessary, it might be convenient.
 *****************************************************************************/

namespace MueLu {

   template <class LocalOrdinal  = int, 
            class GlobalOrdinal = LocalOrdinal, 
            class Node          = Kokkos::DefaultNode::DefaultNodeType, 
            class LocalMatOps   = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps > //TODO: or BlockSparseOp ?
   class Aggregates : public Teuchos::Describable {

#include "MueLu_UseShortNames.hpp"
    
   public:
     
     Aggregates(const MueLu::Graph<LocalOrdinal, GlobalOrdinal, Node, LocalMatOps> & graph, const std::string & objectLabel = "");
     ~Aggregates();
     
     inline int GetNumAggregates()                      { return nAggregates_;            }
     inline void SetNumAggregates(int nAggregates)      { nAggregates_ = nAggregates;     }
     inline RCP<LOVector> & GetVertex2AggId() { return vertex2AggId_;           } // LO or GO ?
     inline RCP<LOVector> & GetProcWinner()   { return procWinner_;             }
     inline bool IsRoot(int i)                          { return isRoot_[i];              }
     inline void SetIsRoot(int vertex, bool value=true) { isRoot_[vertex] = value; }
     
     inline const RCP<const Cthulhu::Map<int,int> > GetMap() { return GetVertex2AggId()->getMap(); }

   private:
    int   nAggregates_;             /* Number of aggregates on this processor  */
    
    RCP<LOVector> vertex2AggId_;/* vertex2AggId[k] gives a local id        */
                                    /* corresponding to the aggregate to which */
                                    /* local id k has been assigned.  While k  */
    RCP<LOVector> procWinner_;  /* is the local id on my processor (MyPID),*/
                                    /* vertex2AggId[k] is the local id on the  */
                                    /* processor which actually owns the       */
                                    /* aggregate. This owning processor has id */
                                    /* given by procWinner[k].                 */

    bool *isRoot_;                  /* IsRoot[i] indicates whether vertex i  */
                                    /* is a root node.                       */

  };

  // Constructors to create aggregates.
   template <class LocalOrdinal ,
             class GlobalOrdinal,
             class Node         ,
            class LocalMatOps  >
   Aggregates<LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Aggregates(const MueLu::Graph<LocalOrdinal, GlobalOrdinal, Node, LocalMatOps> & graph, const std::string & objectLabel)
  {
    
    setObjectLabel(objectLabel);
    
    nAggregates_  = 0;
    
    vertex2AggId_ = LOVectorFactory::Build(graph.GetImportMap()); //RTODO
    vertex2AggId_->putScalar(MUELU_UNAGGREGATED);
    
    procWinner_ = LOVectorFactory::Build(graph.GetImportMap()); //RTODO
    procWinner_->putScalar(MUELU_UNASSIGNED);
    
    //RTODO 
    isRoot_ = new bool[graph.GetImportMap()->getNodeNumElements()]; //TODO: use ArrayRCP
    for (size_t i=0; i < graph.GetImportMap()->getNodeNumElements(); i++)
      isRoot_[i] = false;

  }

  // Destructor for aggregates.
template <class LocalOrdinal ,
          class GlobalOrdinal,
          class Node        , 
            class LocalMatOps  >
  Aggregates<LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::~Aggregates()
  {
    if (isRoot_       != NULL) delete [] (isRoot_);
  }

}

#define MUELU_AGGREGATES_SHORT
#endif
