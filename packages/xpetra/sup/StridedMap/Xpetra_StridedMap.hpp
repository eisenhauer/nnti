// @HEADER
//
// ***********************************************************************
//
//             Xpetra: A linear algebra interface package
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

// WARNING: This code is experimental. Backwards compatibility should not be expected.

#ifndef XPETRA_STRIDEDMAP_HPP
#define XPETRA_STRIDEDMAP_HPP

/* this file is automatically generated - do not edit (see script/interfaces.py) */

#include <Kokkos_DefaultNode.hpp>
#include <Kokkos_DefaultKernels.hpp>

#include <Teuchos_Describable.hpp>
#include <Teuchos_OrdinalTraits.hpp>

#include "Xpetra_ConfigDefs.hpp"
#include "Xpetra_Exceptions.hpp"

#include "Xpetra_Map.hpp"
#include "Xpetra_MapFactory.hpp"

// MPI helper
#define minAll(rcpComm, in, out)                                        \
  Teuchos::reduceAll(*rcpComm, Teuchos::REDUCE_MIN, in, Teuchos::outArg(out));
#define sumAll(rcpComm, in, out)                                        \
  Teuchos::reduceAll(*rcpComm, Teuchos::REDUCE_SUM, in, Teuchos::outArg(out));


namespace Xpetra {

  /*!
    @class StridedMap
    @brief Class that stores a strided map

    StridedMap extends the functionality of Xpetra::Map

    It derives from Xpetra::Map and adds a std::vector, which contains the striding information.
    E.g. for a strided map with 3dofs per node (2 velocity dofs, 1 pressure dof) the striding
    information looks like:
    std::vector<size_t> stridingInformation;
    stridingInformation.push_back(2); // 2 velocity dofs
    stridingInformation.push_back(1); // 1 pressure dof

    For this example the getFixedBlockSize() returns 3 (3 dofs per node).
    Providing a stridedBlockId parameter in the constructor the strided map only contains dofs of
    one strided block, e.g. with above stridingInformation the call

    StridingMap M(33,0,stridiningInformation,comm,0); // striding block 0 (velocity dofs)
    returns a map with the gids
    0, 1, 3, 4, 6, 7, ... (which contains only the velocity dofs)

    and
    StridingMap M(33,0,stridiningInformation,comm,1); // striding block 1 (pressure dofs)
    creates a map with only the pressure dofs
    2, 5, 8, ...

    @note: there's no support for global offset, yet.
  */
  template <class LocalOrdinal, class GlobalOrdinal = LocalOrdinal, class Node = KokkosClassic::DefaultNode::DefaultNodeType>
  class StridedMap : public virtual Map<LocalOrdinal, GlobalOrdinal, Node> {
    typedef typename KokkosClassic::DefaultKernels<void,LocalOrdinal,Node>::SparseOps LocalMatOps;

#undef XPETRA_STRIDEDMAP_SHORT
#include "Xpetra_UseShortNamesOrdinal.hpp"

  public:

    //! @name Constructor/Destructor Methods
    //@{

     /** \brief Map constructor with contiguous uniform distribution.
     *
     *  Map constructor with contiguous uniform distribution.
     *  The elements are distributed among nodes so that the subsets of global
     *  elements are non-overlapping and contiguous and as evenly distributed
     *  across the nodes as possible.
     *
     *  If numGlobalElements ==
     *  Teuchos::OrdinalTraits<global_size_t>::invalid(), the number
     *  of global elements will be computed via a global
     *  communication.  Otherwise, it must be equal to the sum of the
     *  local elements across all nodes. This will only be verified if
     *  Trilinos' Teuchos package was built with debug support (CMake
     *  Boolean option TEUCHOS_ENABLE_DEBUG=ON).  If verification
     *  fails, a std::invalid_argument exception will be thrown.
     *
     *  \pre stridingInfo.size() > 0
     *  \pre numGlobalElements % getFixedBlockSize() == 0
     */
    StridedMap(UnderlyingLib xlib, global_size_t numGlobalElements, GlobalOrdinal indexBase, std::vector<size_t>& stridingInfo,
               const Teuchos::RCP< const Teuchos::Comm< int > >& comm, LocalOrdinal stridedBlockId = -1, GlobalOrdinal offset = 0,
               LocalGlobal lg = GloballyDistributed, const Teuchos::RCP< Node >& node = KokkosClassic::DefaultNode::getDefaultNode())
    : stridingInfo_(stridingInfo), stridedBlockId_(stridedBlockId), offset_(offset), indexBase_(indexBase) {

      size_t blkSize = getFixedBlockSize();
      TEUCHOS_TEST_FOR_EXCEPTION(stridingInfo.size() == 0, Exceptions::RuntimeError,
                                 "StridedMap::StridedMap: stridingInfo not valid: stridingInfo.size() = 0?");
      TEUCHOS_TEST_FOR_EXCEPTION(numGlobalElements == Teuchos::OrdinalTraits<global_size_t>::invalid(), std::invalid_argument,
                                 "StridedMap::StridedMap: numGlobalElements is invalid");
      TEUCHOS_TEST_FOR_EXCEPTION(numGlobalElements % blkSize != 0, Exceptions::RuntimeError,
                                 "StridedMap::StridedMap: stridingInfo not valid: getFixedBlockSize is not an integer multiple of numGlobalElements.");
      if (stridedBlockId != -1)
        TEUCHOS_TEST_FOR_EXCEPTION(stridingInfo.size() < Teuchos::as<size_t>(stridedBlockId), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: stridedBlockId > stridingInfo.size()");

      std::cout << "[" << comm->getRank() << "] Here!" << std::endl;

      // check input data and reorganize map
      global_size_t numGlobalNodes = numGlobalElements / blkSize;

      // build an equally distributed node map
      RCP<Map> nodeMap = MapFactory::Build(xlib, numGlobalNodes, indexBase, comm, lg, node);
      global_size_t numLocalNodes = nodeMap->getNodeNumElements();

      // translate local node ids to local dofs
      size_t nStridedOffset = 0;
      size_t nDofsPerNode = blkSize; // dofs per node for local striding block
      if (stridedBlockId > -1) {
        for (int j = 0; j < stridedBlockId; j++)
          nStridedOffset += stridingInfo_[j];

        nDofsPerNode = stridingInfo_[stridedBlockId];
        numGlobalElements = numGlobalNodes * Teuchos::as<global_size_t>(nDofsPerNode);
      }
      size_t numLocalElements = numLocalNodes * Teuchos::as<size_t>(nDofsPerNode);

      std::vector<GlobalOrdinal> dofgids(numLocalElements);
      for (LocalOrdinal i = 0; i < Teuchos::as<LocalOrdinal>(numLocalNodes); i++) {
        GlobalOrdinal nodeGID = nodeMap->getGlobalElement(i);

        for (size_t j = 0; j < nDofsPerNode; j++)
          dofgids[i*nDofsPerNode + j] = indexBase_ + offset_ + (nodeGID - indexBase_)*Teuchos::as<GlobalOrdinal>(blkSize) + Teuchos::as<GlobalOrdinal>(nStridedOffset + j);
      }

      map_ = MapFactory::Build(xlib, numGlobalElements, dofgids, indexBase, comm, node);

      if (stridedBlockId == -1) {
        TEUCHOS_TEST_FOR_EXCEPTION(getNodeNumElements() != Teuchos::as<size_t>(nodeMap->getNodeNumElements()*nDofsPerNode), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: wrong distribution of dofs among processors.");
        TEUCHOS_TEST_FOR_EXCEPTION(getGlobalNumElements() != Teuchos::as<size_t>(nodeMap->getGlobalNumElements()*nDofsPerNode), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: wrong distribution of dofs among processors.");

      } else {
        int nDofsInStridedBlock = stridingInfo[stridedBlockId];
        TEUCHOS_TEST_FOR_EXCEPTION(getNodeNumElements() != Teuchos::as<size_t>(nodeMap->getNodeNumElements()*nDofsInStridedBlock), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: wrong distribution of dofs among processors.");
        TEUCHOS_TEST_FOR_EXCEPTION(getGlobalNumElements() != Teuchos::as<size_t>(nodeMap->getGlobalNumElements()*nDofsInStridedBlock), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: wrong distribution of dofs among processors.");
      }

      TEUCHOS_TEST_FOR_EXCEPTION(CheckConsistency() == false, Exceptions::RuntimeError, "StridedTpetraMap::StridedTpetraMap: CheckConsistency() == false");
    }

    //! Map constructor with a user-defined contiguous distribution.
     /** \brief Map constructor with a user-defined contiguous distribution.
     *
     *  Map constructor with a user-defined contiguous distribution.
     *  The elements are distributed among nodes so that the subsets of global
     *  elements are non-overlapping and contiguous and as evenly distributed
     *  across the nodes as possible.
     *
     *  If numGlobalElements ==
     *  Teuchos::OrdinalTraits<global_size_t>::invalid(), the number
     *  of global elements will be computed via a global
     *  communication.  Otherwise, it must be equal to the sum of the
     *  local elements across all nodes. This will only be verified if
     *  Trilinos' Teuchos package was built with debug support (CMake
     *  Boolean option TEUCHOS_ENABLE_DEBUG=ON).  If verification
     *  fails, a std::invalid_argument exception will be thrown.
     *
     *  \pre stridingInfo.size() > 0
     *  \pre numGlobalElements % getFixedBlockSize() == 0
     *  \pre numLocalElements % getFixedBlockSize() == 0
     */
    StridedMap(UnderlyingLib xlib, global_size_t numGlobalElements, size_t numLocalElements, GlobalOrdinal indexBase, std::vector<size_t>& stridingInfo,
               const Teuchos::RCP< const Teuchos::Comm< int > > &comm, LocalOrdinal stridedBlockId = -1, GlobalOrdinal offset = 0,
               const Teuchos::RCP< Node > &node = KokkosClassic::DefaultNode::getDefaultNode())
    : stridingInfo_(stridingInfo), stridedBlockId_(stridedBlockId), offset_(offset), indexBase_(indexBase)
    {
      size_t blkSize = getFixedBlockSize();
      TEUCHOS_TEST_FOR_EXCEPTION(stridingInfo.size() == 0, Exceptions::RuntimeError,
                                 "StridedMap::StridedMap: stridingInfo not valid: stridingInfo.size() = 0?");
      if (numGlobalElements != Teuchos::OrdinalTraits<global_size_t>::invalid()) {
        TEUCHOS_TEST_FOR_EXCEPTION(numGlobalElements % blkSize != 0, Exceptions::RuntimeError,
                                   "StridedMap::StridedMap: stridingInfo not valid: getFixedBlockSize is not an integer multiple of numGlobalElements.");
#ifdef HAVE_TPETRA_DEBUG
        // We have to do this check ourselves, as we don't necessarily construct the full Tpetra map
        global_size_t sumLocalElements;
        sumAll(comm, Teuchos::as<global_size_t>(numLocalElements), sumLocalElements);
        TEUCHOS_TEST_FOR_EXCEPTION(sumLocalElements != numGlobalElements, std::invalid_argument,
                                   "StridedMap::StridedMap: sum of numbers of local elements is different from the provided number of global elements.");
#endif
      }
      TEUCHOS_TEST_FOR_EXCEPTION(numLocalElements % blkSize != 0, Exceptions::RuntimeError,
                                 "StridedMap::StridedMap: stridingInfo not valid: getFixedBlockSize is not an integer multiple of numLocalElements.");
      if (stridedBlockId != -1)
        TEUCHOS_TEST_FOR_EXCEPTION(stridingInfo.size() < Teuchos::as<size_t>(stridedBlockId), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: stridedBlockId > stridingInfo.size()");

      // check input data and reorganize map
      global_size_t numGlobalNodes = Teuchos::OrdinalTraits<global_size_t>::invalid();
      if (numGlobalElements != Teuchos::OrdinalTraits<global_size_t>::invalid())
        numGlobalNodes = numGlobalElements / blkSize;
      global_size_t numLocalNodes = numLocalElements / blkSize;

      // build an equally distributed node map
      RCP<Map> nodeMap = MapFactory::Build(xlib, numGlobalNodes, numLocalNodes, indexBase, comm, node);

      // translate local node ids to local dofs
      size_t nStridedOffset = 0;
      size_t nDofsPerNode = blkSize; // dofs per node for local striding block
      if (stridedBlockId > -1) {
        for (int j = 0; j < stridedBlockId; j++)
          nStridedOffset += stridingInfo_[j];

        nDofsPerNode = stridingInfo_[stridedBlockId];
        numGlobalElements = nodeMap->getGlobalNumElements() * Teuchos::as<global_size_t>(nDofsPerNode);
      }
      numLocalElements = numLocalNodes * Teuchos::as<size_t>(nDofsPerNode);

      std::vector<GlobalOrdinal> dofgids(numLocalElements);
      for (LocalOrdinal i = 0; i < Teuchos::as<LocalOrdinal>(numLocalNodes); i++) {
        GlobalOrdinal nodeGID = nodeMap->getGlobalElement(i);

        for (size_t j = 0; j < nDofsPerNode; j++)
          dofgids[i*nDofsPerNode + j] = indexBase_ + offset_ + (nodeGID - indexBase_)*Teuchos::as<GlobalOrdinal>(blkSize) + Teuchos::as<GlobalOrdinal>(nStridedOffset + j);
      }

      map_ = MapFactory::Build(xlib, numGlobalElements, dofgids, indexBase, comm, node);

      if (stridedBlockId == -1) {
        TEUCHOS_TEST_FOR_EXCEPTION(getNodeNumElements() != Teuchos::as<size_t>(nodeMap->getNodeNumElements()*nDofsPerNode), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: wrong distribution of dofs among processors.");
        TEUCHOS_TEST_FOR_EXCEPTION(getGlobalNumElements() != Teuchos::as<size_t>(nodeMap->getGlobalNumElements()*nDofsPerNode), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: wrong distribution of dofs among processors.");

      } else {
        int nDofsInStridedBlock = stridingInfo[stridedBlockId];
        TEUCHOS_TEST_FOR_EXCEPTION(getNodeNumElements() != Teuchos::as<size_t>(nodeMap->getNodeNumElements()*nDofsInStridedBlock), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: wrong distribution of dofs among processors.");
        TEUCHOS_TEST_FOR_EXCEPTION(getGlobalNumElements() != Teuchos::as<size_t>(nodeMap->getGlobalNumElements()*nDofsInStridedBlock), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: wrong distribution of dofs among processors.");
      }

      TEUCHOS_TEST_FOR_EXCEPTION(CheckConsistency() == false, Exceptions::RuntimeError, "StridedTpetraMap::StridedTpetraMap: CheckConsistency() == false");
    }

    /** \brief Map constructor with user-defined non-contiguous (arbitrary) distribution.
     *
     *  createse a strided map using the GIDs in elementList and the striding information
     *  provided by user.
     *
     *  \pre stridingInfo.size() > 0
     *  \pre numGlobalElements % getFixedBlockSize() == 0
     *  \pre elementList.size() % getFixedBlockSize() == 0
     *  \post CheckConsistency() == true
     */
    StridedMap(UnderlyingLib xlib, global_size_t numGlobalElements, const Teuchos::ArrayView< const GlobalOrdinal > &elementList, GlobalOrdinal indexBase,
               std::vector<size_t>& stridingInfo, const Teuchos::RCP< const Teuchos::Comm< int > > &comm, LocalOrdinal stridedBlockId = -1,
               const Teuchos::RCP< Node > &node = KokkosClassic::DefaultNode::getDefaultNode())
    : stridingInfo_(stridingInfo), stridedBlockId_(stridedBlockId), indexBase_(indexBase)
    {
      size_t blkSize = getFixedBlockSize();

      TEUCHOS_TEST_FOR_EXCEPTION(stridingInfo.size() == 0, Exceptions::RuntimeError,
                                 "StridedMap::StridedMap: stridingInfo not valid: stridingInfo.size() = 0?");
      if (stridedBlockId != -1)
        TEUCHOS_TEST_FOR_EXCEPTION(stridingInfo.size() < Teuchos::as<size_t>(stridedBlockId), Exceptions::RuntimeError,
                                   "StridedTpetraMap::StridedTpetraMap: stridedBlockId > stridingInfo.size()");
      if (numGlobalElements != Teuchos::OrdinalTraits<global_size_t>::invalid()) {
        TEUCHOS_TEST_FOR_EXCEPTION(numGlobalElements % blkSize != 0, Exceptions::RuntimeError,
                                   "StridedMap::StridedMap: stridingInfo not valid: getFixedBlockSize is not an integer multiple of numGlobalElements.");
#ifdef HAVE_TPETRA_DEBUG
        // We have to do this check ourselves, as we don't necessarily construct the full Tpetra map
        global_size_t sumLocalElements, numLocalElements = elementList.size();
        sumAll(comm, numLocalElements, sumLocalElements);
        TEUCHOS_TEST_FOR_EXCEPTION(sumLocalElements != numGlobalElements, std::invalid_argument,
                                   "StridedMap::StridedMap: sum of numbers of local elements is different from the provided number of global elements.");
#endif
      }

      if (stridedBlockId == -1) {
        // numGlobalElements can be -1! FIXME
        // TEUCHOS_TEST_FOR_EXCEPTION(numGlobalElements  % blkSize != 0, Exceptions::RuntimeError,
                                   // "StridedMap::StridedMap: stridingInfo not valid: getFixedBlockSize is not an integer multiple of numGlobalElements.");
        TEUCHOS_TEST_FOR_EXCEPTION(elementList.size() % blkSize != 0, Exceptions::RuntimeError,
                                   "StridedMap::StridedMap: stridingInfo not valid: getFixedBlockSize is not an integer multiple of elementList.size().");

      } else {
        // numGlobalElements can be -1! FIXME
        // TEUCHOS_TEST_FOR_EXCEPTION(numGlobalElements  % stridingInfo[stridedBlockId] != 0, Exceptions::RuntimeError,
                                   // "StridedMap::StridedMap: stridingInfo not valid: stridingBlockInfo[stridedBlockId] is not an integer multiple of numGlobalElements.");
        TEUCHOS_TEST_FOR_EXCEPTION(elementList.size() % stridingInfo[stridedBlockId] != 0, Exceptions::RuntimeError,
                                   "StridedMap::StridedMap: stridingInfo not valid: stridingBlockInfo[stridedBlockId] is not an integer multiple of elementList.size().");
      }

      map_ = MapFactory::Build(xlib, numGlobalElements, elementList, indexBase, comm, node);

      // set parameters for striding information
      TEUCHOS_TEST_FOR_EXCEPTION(CheckConsistency() == false, Exceptions::RuntimeError, "StridedTpetraMap::StridedTpetraMap: CheckConsistency() == false");

      // calculate offset_

      // find minimum GID over all procs
      GlobalOrdinal minGidOnCurProc = Teuchos::OrdinalTraits<GlobalOrdinal>::max();
      for (Teuchos_Ordinal k = 0; k < elementList.size(); k++) // TODO fix occurence of Teuchos_Ordinal
        if (elementList[k] < minGidOnCurProc)
          minGidOnCurProc = elementList[k];

      minAll(comm, minGidOnCurProc, offset_);

      // calculate striding index
      size_t nStridedOffset = 0;
      for (int j = 0; j < stridedBlockId; j++)
        nStridedOffset += stridingInfo[j];
      const GlobalOrdinal goStridedOffset = Teuchos::as<GlobalOrdinal>(nStridedOffset);

      // adapt offset_
      offset_ -= goStridedOffset + indexBase_;
    }

    StridedMap(const RCP<const Map>& map, std::vector<size_t>& stridingInfo, GlobalOrdinal indexBase, LocalOrdinal stridedBlockId = -1, GlobalOrdinal offset = 0)
    : stridingInfo_(stridingInfo), stridedBlockId_(stridedBlockId), offset_(offset), indexBase_(map->getIndexBase())
    {
      map_ = map;
    }


    //! Destructor.
    virtual ~StridedMap() { }

   //@}

    //! @name Access functions for striding data
    //@{

    std::vector<size_t> getStridingData() const             { return stridingInfo_; }

    void setStridingData(std::vector<size_t> stridingInfo)  { stridingInfo_ = stridingInfo; }

    size_t getFixedBlockSize() const {
      size_t blkSize = 0;
      for (std::vector<size_t>::const_iterator it = stridingInfo_.begin(); it != stridingInfo_.end(); ++it)
        blkSize += *it;
      return blkSize;
    }

    /// returns strided block id of the dofs stored in this map
    /// or -1 if full strided map is stored in this map
    LocalOrdinal getStridedBlockId() const                  { return stridedBlockId_; }

    /// returns true, if this is a strided map (i.e. more than 1 strided blocks)
    bool isStrided()                                        { return stridingInfo_.size() > 1 ? true : false; }

    /// returns true, if this is a blocked map (i.e. more than 1 dof per node)
    /// either strided or just 1 block per node
    bool isBlocked()                                        { return getFixedBlockSize() > 1 ? true : false; }

    GlobalOrdinal getOffset() const                         { return offset_; }

    void setOffset(GlobalOrdinal offset)                    { offset_ = offset; }

    // returns number of strided block id which gid belongs to.
    size_t GID2StridingBlockId(GlobalOrdinal gid) const {
      GlobalOrdinal tgid = gid - offset_ - indexBase_;
      tgid = tgid % getFixedBlockSize();

      size_t nStridedOffset = 0;
      size_t stridedBlockId = 0;
      for (size_t j = 0; j < stridingInfo_.size(); j++) {
        nStridedOffset += stridingInfo_[j];
        if (Teuchos::as<size_t>(tgid) < nStridedOffset) {
          stridedBlockId = j;
          break;
        }
      }
      return stridedBlockId;
    }

    //! @name Xpetra specific
    //@{

    RCP<const Map> getMap() const { return map_; }

    //@}

    /* // function currently not needed but maybe useful
    std::vector<GlobalOrdinal> NodeId2GlobalDofIds(GlobalOrdinal nodeId) const {
      TEUCHOS_TEST_FOR_EXCEPTION(stridingInfo_.size() == 0, Exceptions::RuntimeError, "StridedMap::NodeId2GlobalDofIds: stridingInfo not valid: stridingInfo.size() = 0?");
      std::vector<GlobalOrdinal> dofs;
      if(stridedBlockId_ > -1) {
          TEUCHOS_TEST_FOR_EXCEPTION(stridingInfo_[stridedBlockId_] == 0, Exceptions::RuntimeError, "StridedMap::NodeId2GlobalDofIds: stridingInfo not valid: stridingInfo[stridedBlockId] = 0?");

          // determine nStridedOffset
          size_t nStridedOffset = 0;
          for(int j=0; j<stridedBlockId_; j++) {
            nStridedOffset += stridingInfo_[j];
          }

          for(size_t i = 0; i<stridingInfo_[stridedBlockId_]; i++) {
            GlobalOrdinal gid =
                nodeId * Teuchos::as<GlobalOrdinal>(getFixedBlockSize()) +
                offset_ +
                Teuchos::as<GlobalOrdinal>(nStridedOffset) +
                Teuchos::as<GlobalOrdinal>(i);
            dofs.push_back(gid);
          }
      } else {
        for(size_t i = 0; i<getFixedBlockSize(); i++) {
          GlobalOrdinal gid =
              nodeId * Teuchos::as<GlobalOrdinal>(getFixedBlockSize()) +
              offset_ +
              Teuchos::as<GlobalOrdinal>(i);
          dofs.push_back(gid);
        }
      }
      return dofs;
    }*/
    //@}

  private:
    virtual bool CheckConsistency() {
      if (getStridedBlockId() == -1) {
        // Strided map contains the full map
        if (getNodeNumElements()   % getFixedBlockSize() != 0 ||    // number of local  elements is not a multiple of block size
            getGlobalNumElements() % getFixedBlockSize() != 0)      // number of global    -//-
          return false;

      } else {
        // Strided map contains only the partial map
        Teuchos::ArrayView<const GlobalOrdinal> dofGids = getNodeElementList();
        // std::sort(dofGids.begin(), dofGids.end());

        if (dofGids.size() == 0)  // special treatment for empty processors
          return true;

        if (dofGids.size() % stridingInfo_[stridedBlockId_] != 0)
          return false;


        // Calculate nStridedOffset
        size_t nStridedOffset = 0;
        for (int j = 0; j < stridedBlockId_; j++)
          nStridedOffset += stridingInfo_[j];

        const GlobalOrdinal goStridedOffset = Teuchos::as<GlobalOrdinal>(nStridedOffset);
        const GlobalOrdinal goZeroOffset    = (dofGids[0] - nStridedOffset - offset_ - indexBase_) / Teuchos::as<GlobalOrdinal>(getFixedBlockSize());

        GlobalOrdinal cnt = 0;
        for (size_t i = 0; i < Teuchos::as<size_t>(dofGids.size())/stridingInfo_[stridedBlockId_]; i += stridingInfo_[stridedBlockId_]) {
          const GlobalOrdinal first_gid = dofGids[i];

          // We expect this to be the same for all DOFs of the same node
          cnt = (first_gid - goStridedOffset - offset_ - indexBase_) / Teuchos::as<GlobalOrdinal>(getFixedBlockSize()) - goZeroOffset;

          // Loop over all DOFs that belong to current node
          for (size_t j = 0; j < stridingInfo_[stridedBlockId_]; j++) {
            const GlobalOrdinal gid = dofGids[i+j];
            const GlobalOrdinal r   = (gid - Teuchos::as<GlobalOrdinal>(j) - goStridedOffset - offset_ - indexBase_) /
                                      Teuchos::as<GlobalOrdinal>(getFixedBlockSize()) - goZeroOffset - cnt;
            if (r != Teuchos::OrdinalTraits<GlobalOrdinal>::zero() ) {
              std::cout << "goZeroOffset   : " <<  goZeroOffset << std::endl
                        << "dofGids[0]     : " <<  dofGids[0] << std::endl
                        << "stridedOffset  : " <<  nStridedOffset << std::endl
                        << "offset_        : " <<  offset_ << std::endl
                        << "goStridedOffset: " <<  goStridedOffset << std::endl
                        << "getFixedBlkSize: " <<  getFixedBlockSize() << std::endl
                        << "gid: " << gid << " GID: " << r << std::endl;

              return false;
            }
          }
        }
      }

      return true;
    }

  private:
    RCP<const Map>      map_;

    std::vector<size_t> stridingInfo_;      //!< vector with size of strided blocks (dofs)
    LocalOrdinal        stridedBlockId_;    //!< member variable denoting which dofs are stored in map
                                            //     stridedBlock == -1: the full map (with all strided block dofs)
                                            //     stridedBlock  > -1: only dofs of strided block with index "stridedBlockId" are stored in this map
    GlobalOrdinal       offset_;		    //!< offset for gids in map (default = 0)
    GlobalOrdinal       indexBase_;         //!< index base for the strided map (default = 0)

  public:

    //! @name Map Attribute Methods
    //@{

    //! Returns the number of elements in this Map.
    global_size_t getGlobalNumElements() const { return map_->getGlobalNumElements(); }

    //! Returns the number of elements belonging to the calling node.
    size_t getNodeNumElements() const { return map_->getNodeNumElements(); }

    //! Returns the index base for this Map.
    GlobalOrdinal getIndexBase() const { return map_->getIndexBase(); }

    //! Returns minimum local index.
    LocalOrdinal getMinLocalIndex() const { return map_->getMinLocalIndex(); }

    //! Returns maximum local index.
    LocalOrdinal getMaxLocalIndex() const { return map_->getMaxLocalIndex(); }

    //! Returns minimum global index owned by this node.
    GlobalOrdinal getMinGlobalIndex() const { return map_->getMinGlobalIndex(); }

    //! Returns maximum global index owned by this node.
    GlobalOrdinal getMaxGlobalIndex() const { return map_->getMaxGlobalIndex(); }

    //! Return the minimum global index over all nodes.
    GlobalOrdinal getMinAllGlobalIndex() const { return map_->getMinAllGlobalIndex(); }

    //! Return the maximum global index over all nodes.
    GlobalOrdinal getMaxAllGlobalIndex() const { return map_->getMaxAllGlobalIndex(); }

    //! Return the local index for a given global index.
    LocalOrdinal getLocalElement(GlobalOrdinal globalIndex) const { return map_->getLocalElement(globalIndex); }

    //! Return the global index for a given local index.
    GlobalOrdinal getGlobalElement(LocalOrdinal localIndex) const { return map_->getGlobalElement(localIndex); }

    //! Returns the node IDs and corresponding local indices for a given list of global indices.
    LookupStatus getRemoteIndexList(const Teuchos::ArrayView<const GlobalOrdinal> &GIDList, const Teuchos::ArrayView<int> &nodeIDList, const Teuchos::ArrayView<LocalOrdinal> &LIDList) const {
      return map_->getRemoteIndexList(GIDList, nodeIDList, LIDList);
    }

    //! Returns the node IDs for a given list of global indices.
    LookupStatus getRemoteIndexList(const Teuchos::ArrayView<const GlobalOrdinal> &GIDList, const Teuchos::ArrayView< int > &nodeIDList) const {
      return map_->getRemoteIndexList(GIDList, nodeIDList);
    }

    //! Return a list of the global indices owned by this node.
    Teuchos::ArrayView< const GlobalOrdinal > getNodeElementList() const { return map_->getNodeElementList(); }

    //! Returns true if the local index is valid for this Map on this node; returns false if it isn't.
    bool isNodeLocalElement(LocalOrdinal localIndex) const { return map_->isNodeLocalElement(localIndex); }

    //! Returns true if the global index is found in this Map on this node; returns false if it isn't.
    bool isNodeGlobalElement(GlobalOrdinal globalIndex) const { return map_->isNodeGlobalElement(globalIndex); }

    //! Returns true if this Map is distributed contiguously; returns false otherwise.
    bool isContiguous() const { return map_->isContiguous(); }

    //! Returns true if this Map is distributed across more than one node; returns false otherwise.
    bool isDistributed() const { return map_->isDistributed(); }

    //@}

    //! Returns true if map is compatible with this Map.
    bool isCompatible(const Map& map) const { return map_->isCompatible(map); }

    //! Returns true if map is identical to this Map.
    bool isSameAs(const Map& map) const { return map_->isSameAs(map); }

    //! Get the Comm object for this Map.
    const Teuchos::RCP< const Teuchos::Comm< int > > getComm() const { return map_->getComm(); }

    //! Get the Node object for this Map.
    const Teuchos::RCP<Node>  getNode() const { return map_->getNode(); }

    RCP<const Map> removeEmptyProcesses  () const { return map_->removeEmptyProcesses(); }
    RCP<const Map> replaceCommWithSubset (const Teuchos::RCP<const Teuchos::Comm<int> >& newComm) const { return map_->replaceCommWithSubset(newComm); }

    //! Return a simple one-line description of this object.
    std::string description() const { return map_->description(); }

    //! Print the object with some verbosity level to a FancyOStream object.
    void describe(Teuchos::FancyOStream& out, const Teuchos::EVerbosityLevel verbLevel = Teuchos::Describable::verbLevel_default) const { map_->describe(out, verbLevel); }

    //! Get the library used by this object (Tpetra or Epetra?)
    UnderlyingLib lib() const { return map_->lib(); }

  }; // StridedMap class

} // Xpetra namespace

#define XPETRA_STRIDEDMAP_SHORT
#endif // XPETRA_STRIDEDMAP_HPP
