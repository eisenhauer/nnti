#ifndef XPETRA_EPETRAMAP_HPP
#define XPETRA_EPETRAMAP_HPP

/* this file is automatically generated - do not edit (see script/epetra.py) */

#include "Xpetra_EpetraConfigDefs.hpp"

#include "Xpetra_Map.hpp"

#include <Epetra_Map.h>
#include <Epetra_BlockMap.h>

#include "Xpetra_Utils.hpp"
#include "Xpetra_EpetraUtils.hpp"

#include "Xpetra_ConfigDefs.hpp"

namespace Xpetra {

  // TODO: move that elsewhere
  const Epetra_Map & toEpetra(const Map<int,int> &);
  const Epetra_Map & toEpetra(const RCP< const Map<int, int> > &);
  //const RCP< const Map<int, int> > toXpetra(const RCP< const Epetra_Map > &);
  const RCP< const Map<int, int> > toXpetra(const Epetra_BlockMap &);
  //

  class EpetraMap
    : public virtual Map<int, int>
  {

    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::DefaultNode::DefaultNodeType Node;

  public:

    //! @name Constructors and destructor
    //@{

    //! Map constructor with Tpetra-defined contiguous uniform distribution.
    EpetraMap(global_size_t numGlobalElements, GlobalOrdinal indexBase, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, LocalGlobal lg=GloballyDistributed, const Teuchos::RCP< Node > &node=Kokkos::DefaultNode::getDefaultNode());

    //! Map constructor with a user-defined contiguous distribution.
    EpetraMap(global_size_t numGlobalElements, size_t numLocalElements, GlobalOrdinal indexBase, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, const Teuchos::RCP< Node > &node=Kokkos::DefaultNode::getDefaultNode());

    //! Map constructor with user-defined non-contiguous (arbitrary) distribution.
    EpetraMap(global_size_t numGlobalElements, const Teuchos::ArrayView< const GlobalOrdinal > &elementList, GlobalOrdinal indexBase, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, const Teuchos::RCP< Node > &node=Kokkos::DefaultNode::getDefaultNode());

    //! Destructor.
    ~EpetraMap() { }

    //@}

    //! @name Attributes
    //@{

    //! The number of elements in this Map.
    global_size_t getGlobalNumElements() const { XPETRA_MONITOR("EpetraMap::getGlobalNumElements"); return map_->NumGlobalElements(); }

    //! The number of elements belonging to the calling node.
    size_t getNodeNumElements() const { XPETRA_MONITOR("EpetraMap::getNodeNumElements"); return map_->NumMyElements(); }

    //! The index base for this Map.
    GlobalOrdinal getIndexBase() const { XPETRA_MONITOR("EpetraMap::getIndexBase"); return map_->IndexBase(); }

    //! The minimum local index.
    LocalOrdinal getMinLocalIndex() const { XPETRA_MONITOR("EpetraMap::getMinLocalIndex"); return map_->MinLID(); }

    //! The maximum local index.
    LocalOrdinal getMaxLocalIndex() const { XPETRA_MONITOR("EpetraMap::getMaxLocalIndex"); return map_->MaxLID(); }

    //! The minimum global index owned by this node.
    GlobalOrdinal getMinGlobalIndex() const { XPETRA_MONITOR("EpetraMap::getMinGlobalIndex"); return map_->MinMyGID(); }

    //! The maximum global index owned by this node.
    GlobalOrdinal getMaxGlobalIndex() const { XPETRA_MONITOR("EpetraMap::getMaxGlobalIndex"); return map_->MaxMyGID(); }

    //! The minimum global index over all nodes.
    GlobalOrdinal getMinAllGlobalIndex() const { XPETRA_MONITOR("EpetraMap::getMinAllGlobalIndex"); return map_->MinAllGID(); }

    //! The maximum global index over all nodes.
    GlobalOrdinal getMaxAllGlobalIndex() const { XPETRA_MONITOR("EpetraMap::getMaxAllGlobalIndex"); return map_->MaxAllGID(); }

    //! The local index corresponding to the given global index.
    LocalOrdinal getLocalElement(GlobalOrdinal globalIndex) const { XPETRA_MONITOR("EpetraMap::getLocalElement"); return map_->LID(globalIndex); }

    //! Process IDs and corresponding local IDs for a given list of global IDs.
    LookupStatus getRemoteIndexList(const Teuchos::ArrayView< const GlobalOrdinal > &GIDList, const Teuchos::ArrayView< int > &nodeIDList, const Teuchos::ArrayView< LocalOrdinal > &LIDList) const;

    //! Return the node IDs for a given list of global IDs.
    LookupStatus getRemoteIndexList(const Teuchos::ArrayView< const GlobalOrdinal > &GIDList, const Teuchos::ArrayView< int > &nodeIDList) const;

    //! Return a view of the global indices owned by this node.
    Teuchos::ArrayView< const GlobalOrdinal > getNodeElementList() const;

    //@}

    //! @name Boolean tests
    //@{

    //! True if the local index is valid for this Map on this node, else false.
    bool isNodeLocalElement(LocalOrdinal localIndex) const { XPETRA_MONITOR("EpetraMap::isNodeLocalElement"); return map_->MyLID(localIndex); }

    //! True if the global index is found in this Map on this node, else false.
    bool isNodeGlobalElement(GlobalOrdinal globalIndex) const { XPETRA_MONITOR("EpetraMap::isNodeGlobalElement"); return map_->MyGID(globalIndex); }

    //! True if this Map is distributed contiguously, else false.
    bool isContiguous() const { XPETRA_MONITOR("EpetraMap::isContiguous"); return map_->LinearMap(); }

    //! Whether this Map is globally distributed or locally replicated.
    bool isDistributed() const { XPETRA_MONITOR("EpetraMap::isDistributed"); return map_->DistributedGlobal(); }

    //! True if and only if map is compatible with this Map.
    bool isCompatible(const Map< LocalOrdinal, GlobalOrdinal, Node > &map) const { XPETRA_MONITOR("EpetraMap::isCompatible"); return map_->PointSameAs(toEpetra(map)); }

    //! True if and only if map is identical to this Map.
    bool isSameAs(const Map< LocalOrdinal, GlobalOrdinal, Node > &map) const { XPETRA_MONITOR("EpetraMap::isSameAs"); return map_->SameAs(toEpetra(map)); }

    //@}

    //! @name 
    //@{

    //! Get this Map's Comm object.
    const Teuchos::RCP< const Teuchos::Comm< int > >  getComm() const { XPETRA_MONITOR("EpetraMap::getComm"); return toXpetra(map_->Comm()); }

    //! Get this Map's Node object.
    const Teuchos::RCP< Node >  getNode() const;

    //@}

    //! @name 
    //@{

    //! Return a simple one-line description of this object.
    std::string description() const;

    //! Print this object with the given verbosity level to the given FancyOStream.
    void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const;

    //@}

    //! Return the global index for a given local index.  Note that this returns -1 if not found on this processor.  (This is different than Epetra's behavior!)
    GlobalOrdinal getGlobalElement(LocalOrdinal localIndex) const {
      GlobalOrdinal gid = map_->GID(localIndex);
      if (gid == map_->IndexBase()-1) return (-1);
      else                            return (gid);
     }

    //! @name Xpetra specific
    //@{

    //! EpetraMap constructor to wrap a Epetra_Map object
    EpetraMap(const Teuchos::RCP<const Epetra_BlockMap> &map) 
      : map_(map) { }

    //! Get the library used by this object (Epetra or Epetra?)
    UnderlyingLib lib() const { return Xpetra::UseEpetra; }

    //! Get the underlying Epetra map
    //const RCP< const Epetra_Map > & getEpetra_Map() const { return map_; }
    const Epetra_BlockMap& getEpetra_BlockMap() const { return *map_; }
    const Epetra_Map& getEpetra_Map() const { return (Epetra_Map &)*map_; } // Ugly, but the same is done in Epetra_CrsMatrix.h to get the map.

    //@}
   
  protected:

    RCP<const Epetra_BlockMap> map_;
    //const RCP< const Epetra_BlockMap > map_;
    //const RCP< const Epetra::Map< LocalOrdinal, GlobalOrdinal, Node > > map_;

  }; // EpetraMap class

} // Xpetra namespace

#endif // XPETRA_EPETRAMAP_HPP
