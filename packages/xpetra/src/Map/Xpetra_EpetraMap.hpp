#ifndef XPETRA_EPETRAMAP_HPP
#define XPETRA_EPETRAMAP_HPP

/* this file is automatically generated - do not edit (see script/epetra.py) */

#include "Xpetra_EpetraConfigDefs.hpp"

#include "Xpetra_Map.hpp"

#include <Epetra_Map.h>
#include <Epetra_BlockMap.h>

#include "Xpetra_Utils.hpp"
#include "Xpetra_EpetraUtils.hpp"

namespace Tpetra { //TODO to be removed
  typedef size_t global_size_t;
}

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

    //! @name Constructor/Destructor Methods
    //@{

    //! Map constructor with Tpetra-defined contiguous uniform distribution. The elements are distributed among nodes so that the subsets of global elements are non-overlapping and contiguous and as evenly distributed across the nodes as possible.
    EpetraMap(global_size_t numGlobalElements, GlobalOrdinal indexBase, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, LocalGlobal lg=GloballyDistributed, const Teuchos::RCP< Node > &node=Kokkos::DefaultNode::getDefaultNode());

    //! Map constructor with a user-defined contiguous distribution. The elements are distributed among the nodes so that the subsets of global elements are non-overlapping and contiguous.
    EpetraMap(global_size_t numGlobalElements, size_t numLocalElements, GlobalOrdinal indexBase, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, const Teuchos::RCP< Node > &node=Kokkos::DefaultNode::getDefaultNode());

    //! Map constructor with user-defined non-contiguous (arbitrary) distribution.
    EpetraMap(global_size_t numGlobalElements, const Teuchos::ArrayView< const GlobalOrdinal > &elementList, GlobalOrdinal indexBase, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, const Teuchos::RCP< Node > &node=Kokkos::DefaultNode::getDefaultNode());

    //! Map destructor.
    ~EpetraMap() { }

    //@}

    //! @name Map Attribute Methods
    //@{

    //! Returns the number of elements in this Map.
    global_size_t getGlobalNumElements() const { return map_->NumGlobalElements(); }

    //! Returns the number of elements belonging to the calling node.
    size_t getNodeNumElements() const { return map_->NumMyElements(); }

    //! Returns the index base for this Map.
    GlobalOrdinal getIndexBase() const { return map_->IndexBase(); }

    //! Returns minimum local index.
    LocalOrdinal getMinLocalIndex() const { return map_->MinLID(); }

    //! Returns maximum local index.
    LocalOrdinal getMaxLocalIndex() const { return map_->MaxLID(); }

    //! Returns minimum global index owned by this node.
    GlobalOrdinal getMinGlobalIndex() const { return map_->MinMyGID(); }

    //! Returns maximum global index owned by this node.
    GlobalOrdinal getMaxGlobalIndex() const { return map_->MaxMyGID(); }

    //! Return the minimum global index over all nodes.
    GlobalOrdinal getMinAllGlobalIndex() const { return map_->MinAllGID(); }

    //! Return the maximum global index over all nodes.
    GlobalOrdinal getMaxAllGlobalIndex() const { return map_->MaxAllGID(); }

    //! Return the local index for a given global index.
    LocalOrdinal getLocalElement(GlobalOrdinal globalIndex) const { return map_->LID(globalIndex); }

    //! Returns the node IDs and corresponding local indices for a given list of global indices.
    LookupStatus getRemoteIndexList(const Teuchos::ArrayView< const GlobalOrdinal > &GIDList, const Teuchos::ArrayView< int > &nodeIDList, const Teuchos::ArrayView< LocalOrdinal > &LIDList) const;

    //! Returns the node IDs for a given list of global indices.
    LookupStatus getRemoteIndexList(const Teuchos::ArrayView< const GlobalOrdinal > &GIDList, const Teuchos::ArrayView< int > &nodeIDList) const;

    //! Return a list of the global indices owned by this node.
    Teuchos::ArrayView< const GlobalOrdinal > getNodeElementList() const;

    //! Returns true if the local index is valid for this Map on this node; returns false if it isn't.
    bool isNodeLocalElement(LocalOrdinal localIndex) const { return map_->MyLID(localIndex); }

    //! Returns true if the global index is found in this Map on this node; returns false if it isn't.
    bool isNodeGlobalElement(GlobalOrdinal globalIndex) const { return map_->MyGID(globalIndex); }

    //! Returns true if this Map is distributed contiguously; returns false otherwise.
    bool isContiguous() const { return map_->LinearMap(); }

    //! Returns true if this Map is distributed across more than one node; returns false otherwise.
    bool isDistributed() const { return map_->DistributedGlobal(); }

    //@}

    //! @name Boolean Tests
    //@{

    //! Returns true if map is compatible with this Map.
    bool isCompatible(const Map< LocalOrdinal, GlobalOrdinal, Node > &map) const { return map_->PointSameAs(toEpetra(map)); }

    //! Returns true if map is identical to this Map.
    bool isSameAs(const Map< LocalOrdinal, GlobalOrdinal, Node > &map) const { return map_->SameAs(toEpetra(map)); }

    //@}

    //! @name 
    //@{

    //! Get the Comm object for this Map.
    const Teuchos::RCP< const Teuchos::Comm< int > >  getComm() const { return toXpetra(map_->Comm()); }

    //! Get the Node object for this Map.
    const Teuchos::RCP< Node >  getNode() const;

    //@}

    //! @name 
    //@{

    //! Return a simple one-line description of this object.
    std::string description() const;

    //! Print the object with some verbosity level to a FancyOStream object.
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
   
  private:

    RCP<const Epetra_BlockMap> map_;
    //const RCP< const Epetra_BlockMap > map_;
    //const RCP< const Epetra::Map< LocalOrdinal, GlobalOrdinal, Node > > map_;

  }; // EpetraMap class

} // Xpetra namespace

#endif // XPETRA_EPETRAMAP_HPP
