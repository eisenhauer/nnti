#ifndef MUELU_GRAPH_DECL_HPP
#define MUELU_GRAPH_DECL_HPP

#include "MueLu_ConfigDefs.hpp"

#include <Teuchos_ArrayRCP.hpp>

#include <Xpetra_ConfigDefs.hpp>
#include <Xpetra_CrsGraph.hpp>
#include <Xpetra_MapFactory.hpp>

#include "MueLu_BaseClass.hpp"
#include "MueLu_Exceptions.hpp"

/******************************************************************************
   MueLu representation of a graph.
******************************************************************************/

namespace MueLu {

  template <class LocalOrdinal  = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps>
  class Graph 
    : public BaseClass {

#include "MueLu_UseShortNamesOrdinal.hpp"

  public:

    Graph(const RCP<const CrsGraph> & graph, const std::string & objectLabel="") : graph_(graph) { 
      //setObjectLabel(objectLabel); 
    }

    virtual ~Graph() {}
    
    size_t GetNodeNumVertices() const { return graph_->getNodeNumRows(); }
    size_t GetNodeNumEdges()    const { return graph_->getNodeNumEntries(); }
    
    Xpetra::global_size_t GetGlobalNumEdges() const { return graph_->getGlobalNumEntries(); }

    const RCP<const Teuchos::Comm<int> > GetComm() const { return graph_->getComm(); }
    const RCP<const Map> GetDomainMap() const { return graph_->getDomainMap(); }
    const RCP<const Map> GetImportMap() const {
      //std::cout << "Graph::GetImportMap: PROC: " << graph_->getColMap()->getComm()->getRank() << " " << graph_->getColMap()->getNodeNumElements() << "/" << graph_->getColMap()->getGlobalNumElements() << " numCols (maps)" << std::endl;
      //std::cout << "Graph::GetImportMap: PROC: " << graph_->getColMap()->getComm()->getRank() << " " << graph_->getNodeNumCols() << "/" << graph_->getGlobalNumCols() << " numCols"<< std::endl;
      return graph_->getColMap();
    }
    const RCP<const Map> GetImportDofMap() const;



    //! Return the list of vertices adjacent to the vertex 'v'
    Teuchos::ArrayView<const LocalOrdinal> getNeighborVertices(LocalOrdinal v) const;

    //!
    void SetAmalgamationParams(RCP<std::map<GlobalOrdinal,std::vector<LocalOrdinal> > > globalamalblockid2myrowid,RCP<std::map<GlobalOrdinal,std::vector<LocalOrdinal> > > globalamalblockid2globalrowid) const;

    //!
    RCP<std::map<GlobalOrdinal,std::vector<LocalOrdinal> > > GetAmalgamationParams() const;

#ifdef MUELU_UNUSED
    size_t GetNodeNumGhost() const;
#endif

    /// Return a simple one-line description of this object.
    std::string description() const;

    //! Print the object with some verbosity level to an FancyOStream object.
    //using MueLu::Describable::describe; // overloading, not hiding
    //void describe(Teuchos::FancyOStream &out, const VerbLevel verbLevel = Default) const;;
    void print(Teuchos::FancyOStream &out, const VerbLevel verbLevel = Default) const;

  private:

    RCP<const CrsGraph> graph_;

    //! @name amalgamation information variables
    //@{

    /// map: global block id of amalagamated matrix -> vector of local row ids of unamalgamated matrix (only for global block ids of current proc)
    mutable RCP<std::map<GlobalOrdinal,std::vector<LocalOrdinal> > > globalamalblockid2myrowid_;
    mutable RCP<std::map<GlobalOrdinal,std::vector<GlobalOrdinal> > > globalamalblockid2globalrowid_;

    //@}

  };

} // namespace MueLu

#define MUELU_GRAPH_SHORT
#endif // MUELU_GRAPH_DECL_HPP
