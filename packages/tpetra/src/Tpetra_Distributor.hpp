// @HEADER
// ***********************************************************************
// 
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2004) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef TPETRA_DISTRIBUTOR_HPP
#define TPETRA_DISTRIBUTOR_HPP

#include <Teuchos_RCP.hpp>
#include <Teuchos_OrdinalTraits.hpp>
#include "Tpetra_Util.hpp"
#include <Teuchos_Object.hpp>
#include <Teuchos_Comm.hpp>
#include <Teuchos_CommHelpers.hpp>


// FINISH: some of the get accessors may not be necessary anymore. clean up.

namespace Tpetra {
  
  //! Tpetra::Distributor:  The Tpetra Gather/Scatter Setup Class.
  /*! The Distributor class is an interface that encapsulates the general
        information and services needed for other Tpetra classes to perform gather/scatter
        operations on a parallel computer.
  */

  template<typename Ordinal>
  class Distributor : public Teuchos::Object {
  public:

    //@{ \name Constructor/Destructor

    //! Comm Constuctor (default ctr)
    Distributor(const Teuchos::RCP< Teuchos::Comm<Ordinal> > & comm);

    //! Copy Constructor
    Distributor(const Distributor<Ordinal> & distributor);

    //! Destructor.
    ~Distributor();

    //@}


    //@{ \name Gather/Scatter Constructors

    //! Create Distributor object using list of ImageIDs to send to
    /*! Take a list of ImageIDs and construct a plan for efficiently scattering to these images.
        Return the number of IDs being sent to me.
      \param exportImageIDs In
             List of images that will get the exported data. Image IDs less than zero
             are ignored; their placement corresponds to null sends in any
             future exports.
      \param numRemoteIDs Out
             Number of imports this image will be receiving.
    */
    void createFromSends(const std::vector<Ordinal> & exportImageIDs,
                         Ordinal & numRemoteIDs);

    //! Create Distributor object using list of Image IDs to receive from
    /*! Take a list of global IDs and construct a plan for efficiently scattering to these images.
        Return the number and list of IDs being sent by me.
      \param remoteGIDs In
             List of IDs that this image wants.
      \param remoteImageIDs In
             List of images that will send the remote IDs.
      \param exportGIDs Out
             List of IDs that need to be sent from this image.
      \param exportImageIDs Out
             List of images that will get the exported IDs.
    */
    void createFromRecvs(const std::vector<Ordinal> & remoteGIDs, 
                         const std::vector<Ordinal> & remoteImageIDs, 
                         std::vector<Ordinal>& exportGIDs, 
                         std::vector<Ordinal>& exportImageIDs);

    //@}

    //@{ \name Attribute Accessor Methods

    //! getTotalReceiveLength
    const Ordinal & getTotalReceiveLength() const;

    //! getNumReceives
    const Ordinal & getNumReceives() const;

    //! getSelfMessage - flag for if we're sending to ourself
    /*! If we are sending any elements to ourself, returns true. If we aren't, returns false. */
    bool getSelfMessage() const;

    //! getNumSends
    const Ordinal & getNumSends() const;

    //! getMaxSendLength - maximum number of elements we're sending to a remote image
    const Ordinal & getMaxSendLength() const;

    //! getImagesFrom - list of images sending elements to us
    const std::vector<Ordinal> & getImagesFrom() const;

    //! getLengthsFrom - number of elements we're receiving from each image
    /*! We will receive lengthsFrom[i] elements from image imagesFrom[i] */
    const std::vector<Ordinal> & getLengthsFrom() const;

    //! getImagesTo - list of images we're sending elements to
    const std::vector<Ordinal> & getImagesTo() const;

    //! getIndicesTo
    /*! (Used only if exportImageIDs was not blocked by image.)
        Gives the order to the export buffer, in order to get
      a version that is sorted by imageID. */
    const std::vector<Ordinal> & getIndicesTo() const;

    //! getStartsTo - list of offsets into export buffer
    /*! Given an export buffer that contains all of the elements we're sending out, 
        image i's block of elements will start at position startsTo[i] */
    const std::vector<Ordinal> & getStartsTo() const;

    //! getLengthsTo - number of elements we're sending to each image
    /*! We will send lengthsTo[i] elements to image imagesTo[i] */
    const std::vector<Ordinal> & getLengthsTo() const;

    //@}

    //@{ \name Reverse Communication Methods

    // getReverse
    //! Returns a Distributor with a reverse plan of this Distributor's plan
    /*! Creates the reverse Distributor if this is the first time this function
        has been called.
    */
    const Distributor<Ordinal> & getReverse() const;

    //@}

    //@{ \name Execute Distributor Plan Methods

    //! doPostsAndWaits
    /*! Execute a plan specified by the distributor object.
      \param exports In
             On entry, contains the values we're exporting.
      \param imports Out
             On exit, contains the values exported to us. (\c imports will be resized
             if necessary, and any existing values will be overwritten.)
    */
    template <typename Packet>
    void doPostsAndWaits(const std::vector<Packet>& exports,
                         const Ordinal numPackets,
                               std::vector<Packet>& imports);

    //! doPosts
    template <typename Packet>
    void doPosts(const std::vector<Packet>& exports,
                 const Ordinal numPackets,
                       std::vector<Packet>& imports);

    //! doWaits
    void doWaits();

    //! doReversePostsAndWaits
    /*! Execute a reverse plan specified by the distributor object.
      \param exports In
             On entry, contains the values we're exporting.
      \param imports Out
             On exit, contains the values exported to us. (imports will be resized
             if necessary, and any existing values will be overwritten.)
    */
    template <typename Packet>
    void doReversePostsAndWaits(const std::vector<Packet>& exports,
                                const Ordinal numPackets,
                                      std::vector<Packet>& imports);

    //! doReversePosts
    template <typename Packet>
    void doReversePosts(const std::vector<Packet>& exports,
                        const Ordinal numPackets,
                              std::vector<Packet>& imports);
    
    //! doReverseWaits
    void doReverseWaits();
    
    //@}

    //@{ \name I/O Methods

    //! Implements Teuchos::Object::print.
    void print(ostream& os) const;

    //@}

  private:

    // private data members
    Teuchos::RCP< Teuchos::Comm<Ordinal> > comm_;

    Ordinal numExports_;
    // selfMessage_ is whether I have a send for myself
    bool selfMessage_;
    // numSends_ is number of sends to other nodes
    Ordinal numSends_;
    // imagesTo_, startsTo_ and lengthsTo_ each have size 
    //   numSends_ + selfMessage_
    std::vector<Ordinal> imagesTo_;
    std::vector<Ordinal> startsTo_;
    std::vector<Ordinal> lengthsTo_;
    // maxSendLength_ is the maximum send to another node: 
    //   max(lengthsTo_[i]) for i != me
    Ordinal maxSendLength_;
    std::vector<Ordinal> indicesTo_;
    // numReceives_ is the number of receives by me from other procs, not
    // counting self receives
    Ordinal numReceives_;
    // totalReceiveLength_ is the total number of Packet received, used to 
    // allocate the receive buffer
    Ordinal totalReceiveLength_;
    // imagesFrom_, startsFrom_ and lengthsFrom_ each have size 
    //   numReceives_ + selfMessage_
    std::vector<Ordinal> lengthsFrom_;
    std::vector<Ordinal> imagesFrom_;
    std::vector<Ordinal> startsFrom_;
    std::vector<Ordinal> indicesFrom_;

    // requests associated with non-blocking receives
    std::vector<Teuchos::RCP<Teuchos::CommRequest> > requests_;

    mutable Teuchos::RCP< Distributor<Ordinal> > reverseDistributor_;

    // compute receive info from sends
    void computeReceives();

    // compute send info from receives
    void computeSends(const std::vector<Ordinal> & importIDs,
                      const std::vector<Ordinal> & importImageIDs,
                      std::vector<Ordinal>& exportIDs,
                      std::vector<Ordinal>& exportImageIDs);

    // create a distributor for the reverse communciation pattern (pretty much
    // swap all send and receive info)
    void createReverseDistributor() const;

  }; // class Distributor


  template <typename Ordinal>
  Distributor<Ordinal>::Distributor(Teuchos::RCP< Teuchos::Comm<Ordinal> > const& comm) 
    : Teuchos::Object("Tpetra::Distributor")
    , comm_(comm)
    , numExports_(Teuchos::OrdinalTraits<Ordinal>::zero())
    , selfMessage_(false)
    , numSends_(Teuchos::OrdinalTraits<Ordinal>::zero())
    , maxSendLength_(Teuchos::OrdinalTraits<Ordinal>::zero())
    , numReceives_(Teuchos::OrdinalTraits<Ordinal>::zero())
    , totalReceiveLength_(Teuchos::OrdinalTraits<Ordinal>::zero())
  {}

  template <typename Ordinal>
  Distributor<Ordinal>::Distributor(Distributor<Ordinal> const& distributor) 
    : Teuchos::Object(distributor.label())
    , comm_(distributor.comm_)
    , numExports_(distributor.numExports_)
    , selfMessage_(distributor.selfMessage_)
    , numSends_(distributor.numSends_)
    , maxSendLength_(distributor.maxSendLength_)
    , numReceives_(distributor.numReceives_)
    , totalReceiveLength_(distributor.totalReceiveLength_)
    , reverseDistributor_(distributor.reverseDistributor_)
  {}

  template <typename Ordinal>
  Distributor<Ordinal>::~Distributor() 
  {
  // we shouldn't have any outstanding requests at this point; verify
# ifdef TEUCHOS_DEBUG
    for (typename std::vector<Teuchos::RCP<Teuchos::CommRequest> >::const_iterator i=requests_.begin(); 
         i != requests_.end(); ++i) 
    {
      TEST_FOR_EXCEPTION(*i != Teuchos::null, std::runtime_error,
          "Tpetra::Distributor<"<<Teuchos::OrdinalTraits<Ordinal>::name()
          <<">::doWaits(): Requests should be null after call to Teuchos::waitAll().");
    }
#endif
  }

  template <typename Ordinal>
  void Distributor<Ordinal>::createFromSends(
      const std::vector<Ordinal> & exportImageIDs,
      Ordinal & numRemoteIDs) 
  {
    const Ordinal ZERO = Teuchos::OrdinalTraits<Ordinal>::zero();
    const Ordinal one  = Teuchos::OrdinalTraits<Ordinal>::one();

    numExports_ = exportImageIDs.size();

    const int myImageID = comm_->getRank();
    const int numImages = comm_->getSize();

    // exportImageIDs tells us the communication pattern for this distributor
    // it dictates the way that the export data will be interpretted in doPosts()
    // we want to perform at most one communication per node; this is for two
    // reasons:
    //   * minimize latency/overhead in the comm routines (nice)
    //   * match the number of receives and sends between nodes (necessary)
    // Teuchos::Comm requires that the data for a send is contiguous in a send
    // buffer
    // therefore, if the data in the send buffer for doPosts() is not
    // contiguous, it will need to be copied into a contiguous buffer
    // 
    // the user has specified this pattern and we can't do anything about it,
    // 
    // howevery, if they do not provide an efficient pattern, we will warn them 
    // if one of
    //    THROW_TPETRA_EFFICIENCY_WARNINGS 
    //    PRINT_TPETRA_EFFICIENCY_WARNINGS 
    // is on.
    //
    // if the data is contiguous, then we can post the sends in situ
    // 
    // determine contiguity. there are a number of ways to do this:
    // * if the export IDs are sorted, then all exports to a particular 
    //   node must contiguous. this is how epetra does it. 
    // * if the export ID of the current export already has been listed,
    //   then the previous listing should correspond to the same export.
    //   this tests contiguity, but not sortedness.
    // both of these tests require O(n), where n is the number of 
    // exports. however, the latter will positively identify a greater
    // portion of contiguous patterns. we will go with it.
    // 
    // Check to see if items are grouped by images without gaps
    // If so, indices_to -> 0

    // Setup data structures for quick traversal of arrays
    // this contains the number of sends for each image id
    std::vector<Ordinal> starts(numImages + 1, ZERO);

    // numActive is the number of sends that are not Null
    Ordinal numActive = ZERO;
    int needSendBuff = 0;

    for (int i = 0; i < numExports_; ++i) {
      Ordinal exportID = exportImageIDs[i];
      if (exportID >= ZERO) {
        // increment starts[exportID]
        ++starts[exportID];
        // if after incrementing it is greater than one, check that the
        // previous export went to this node
        // this is a safe comparison, because starts[exportID] > 1
        // implies that i > 1. 
        // null entries break continuity.
        // e.g.,  [ 0, 0, 0, 1, -99, 1, 2, 2, 2] is not considered contiguous
        if (needSendBuff==0 && starts[exportID]>1 && exportID != exportImageIDs[i-1]) {
          needSendBuff = 1;
        }
        ++numActive;
      }
    }

#   if defined(THROW_TPETRA_EFFICIENCY_WARNINGS) || defined(PRINT_TPETRA_EFFICIENCY_WARNINGS)
    {
      int global_needSendBuff;
      Teuchos::reduceAll(*comm_,Teuchos::REDUCE_SUM,needSendBuff,&global_needSendBuff);
      std::string err;
      err += "Tpetra::Distributor<" + Teuchos::TypeNameTraits<Ordinal>::name() 
        + ">::createFromSends(): Grouping export IDs together leads to improved performance.";
#   if defined(THROW_TPETRA_EFFICIENCY_WARNINGS)
      TEST_FOR_EXCEPTION(global_needSendBuff != 0, std::runtime_error, err);
#   else // print warning
      if (global_needSendBuff) {
        std::cerr << err << std::endl;
      }
    }
#   endif
#   endif

    if (starts[myImageID] != ZERO) {
      selfMessage_ = true;
    }
    else {
      selfMessage_ = false;
    }


    if (!needSendBuff) {
      // grouped by image, no send buffer or indicesTo_ needed
      numSends_ = ZERO;
      for (int i=0; i < numImages; ++i) {
        if (starts[i]) ++numSends_;
      }

      // not only do we not need these, but empty indicesTo is also a flag for
      // later
      indicesTo_.resize(0);
      // size these to numSends_; note, at the moment, numSends_ includes sends
      // to myself; set their values to zeros
      imagesTo_.assign(numSends_,ZERO);
      startsTo_.assign(numSends_,ZERO);
      lengthsTo_.assign(numSends_,ZERO);

      // set startsTo to the offsent for each send (i.e., each image ID)
      // set imagesTo to the image ID for each send
      // in interpretting this code, remember that we are assuming contiguity
      // that is why index skips through the ranks
      {
        Ordinal index = ZERO;
        for (Ordinal i = ZERO; i < numSends_; ++i) {
          startsTo_[i] = index;
          Ordinal imageID = exportImageIDs[index];
          imagesTo_[i] = imageID;
          index += starts[imageID];
        }
#ifdef TEUCHOS_DEBUG
        SHARED_TEST_FOR_EXCEPTION(index != numActive, std::logic_error,
            "Tpetra::Distributor::createFromSends: logic error. please notify Tpetra team.",*comm_);
#endif
      }

      // sort the startsTo and image IDs together, in ascending order, according
      // to image IDs
      if (numSends_ > ZERO) {
        sortArrays(imagesTo_, startsTo_);
      }

      // compute the maximum send length
      maxSendLength_ = ZERO;
      for (int i = 0; i < numSends_; ++i) {
        Ordinal imageID = imagesTo_[i];
        lengthsTo_[i] = starts[imageID];
        if ((imageID != myImageID) && (lengthsTo_[i] > maxSendLength_)) {
          maxSendLength_ = lengthsTo_[i];
        }
      }
    }
    else { 
      // not grouped by image, need send buffer and indicesTo_

      // starts[i] is the number of sends to node i
      // numActive equals number of sends total, \sum_i starts[i]

      // the loop starts at starts[1], so explicitly check starts[0]
      if (starts[0] == ZERO ) {
        numSends_ = ZERO;
      }
      else {
        numSends_ = one;
      }
      for (int i = 1; i < numImages; --i) {
        if (starts[i] != ZERO) {
          ++numSends_;
        }
        starts[i] += starts[i-1];
      }
      // starts[i] now contains the number of exports to nodes 0 through i

      for (int i = numImages-1; i != 0; --i) {
        starts[i] = starts[i-1];
      }
      starts[0] = ZERO;
      // starts[i] now contains the number of exports to nodes 0 through
      // i-1, i.e., all nodes before node i

      indicesTo_.resize(numActive);

      for (Ordinal i = ZERO; i < numExports_; ++i) {
        if (exportImageIDs[i] >= ZERO) {
          // record the offset to the sendBuffer for this export
          indicesTo_[starts[exportImageIDs[i]]] = i;
          // now increment the offset for this node
          ++starts[exportImageIDs[i]];
        }
      }
      // our send buffer will contain the export data for each of the nodes
      // we communicate with, in order by node id
      // sendBuffer = {node_0_data, node_1_data, ..., node_np-1_data}
      // indicesTo now maps each export to the location in our send buffer
      // associated with the export
      // data for export i located at sendBuffer[indicesTo[i]]
      //
      // starts[i] once again contains the number of exports to 
      // nodes 0 through i
      for (int node = numImages-1; node != 0; --node) {
        starts[node] = starts[node-1];
      }
      starts.front() = ZERO;       
      starts[numImages] = numActive;
      // 
      // starts[node] once again contains the number of exports to 
      // nodes 0 through node-1
      // i.e., the start of my data in the sendBuffer

      // this contains invalid data at nodes we don't care about, that is okay
      imagesTo_.resize(numSends_);
      startsTo_.resize(numSends_);
      lengthsTo_.resize(numSends_);

      // for each group of sends/exports, record the destination node,
      // the length, and the offset for this send into the 
      // send buffer (startsTo_)
      maxSendLength_ = ZERO;
      int snd = 0;
      for (int node = 0; node < numImages; ++node ) {
        if (starts[node+1] != starts[node]) {
          lengthsTo_[snd] = starts[node+1] - starts[node];
          startsTo_[snd] = starts[node];
          // record max length for all off-node sends
          if ((node != myImageID) && (lengthsTo_[snd] > maxSendLength_)) {
            maxSendLength_ = lengthsTo_[snd];
          }
          imagesTo_[snd] = node;
          ++snd;
        }
      }
#ifdef TEUCHOS_DEBUG
      SHARED_TEST_FOR_EXCEPTION(snd != numSends_, std::logic_error, 
          "Tpetra::Distributor::createFromSends: logic error. please notify Tpetra team.", *comm_);
#endif
    }

    if (selfMessage_) --numSends_;

    // Invert map to see what msgs are received and what length
    computeReceives();

    numRemoteIDs = totalReceiveLength_;
  }

  template <typename Ordinal>
  void Distributor<Ordinal>::createFromRecvs(
      const std::vector<Ordinal> & remoteGIDs, 
      const std::vector<Ordinal> & remoteImageIDs, 
      std::vector<Ordinal> & exportGIDs, 
      std::vector<Ordinal> & exportImageIDs)
  {
    computeSends(remoteGIDs, remoteImageIDs, exportGIDs, exportImageIDs);
    Ordinal testNumRemoteIDs; // dummy
    createFromSends(exportImageIDs, testNumRemoteIDs);
  }

  template <typename Ordinal>
  const Ordinal & Distributor<Ordinal>::getTotalReceiveLength() const 
  { return(totalReceiveLength_); }

  template <typename Ordinal>
  const Ordinal & Distributor<Ordinal>::getNumReceives() const 
  { return(numReceives_); }

  template <typename Ordinal>
  bool Distributor<Ordinal>::getSelfMessage() const 
  { return(selfMessage_); }

  template <typename Ordinal>
  const Ordinal & Distributor<Ordinal>::getNumSends() const 
  { return(numSends_); }

  template <typename Ordinal>
  const Ordinal & Distributor<Ordinal>::getMaxSendLength() const 
  { return(maxSendLength_); }

  template <typename Ordinal>
  const std::vector<Ordinal> & Distributor<Ordinal>::getImagesFrom() const 
  { return(imagesFrom_); }

  template <typename Ordinal>
  const std::vector<Ordinal> & Distributor<Ordinal>::getLengthsFrom() const 
  { return(lengthsFrom_); }

  template <typename Ordinal>
  const std::vector<Ordinal> & Distributor<Ordinal>::getImagesTo() const 
  { return(imagesTo_); }

  template <typename Ordinal>
  const std::vector<Ordinal> & Distributor<Ordinal>::getIndicesTo() const 
  { return(indicesTo_); }

  template <typename Ordinal>
  const std::vector<Ordinal> & Distributor<Ordinal>::getStartsTo() const 
  { return(startsTo_); }

  template <typename Ordinal>
  const std::vector<Ordinal> & Distributor<Ordinal>::getLengthsTo() const 
  { return(lengthsTo_); }

  template <typename Ordinal>
  const Distributor<Ordinal> & Distributor<Ordinal>::getReverse() const
  {
    if (reverseDistributor_ == Teuchos::null) { 
      // need to create reverse distributor
      createReverseDistributor();
    }
    return(*reverseDistributor_);
  }

  template <typename Ordinal>
  void Distributor<Ordinal>::createReverseDistributor() const {
    const Ordinal zero = Teuchos::OrdinalTraits<Ordinal>::zero();

    reverseDistributor_ = Teuchos::rcp(new Distributor<Ordinal>(comm_));

    // compute new totalSendLength
    Ordinal totalSendLength = zero;
    for (Ordinal i = zero; i < (numSends_ + (selfMessage_ ? 1 : 0)); ++i) {
      totalSendLength += lengthsTo_[i];
    }

    // compute new maxReceiveLength
    Ordinal maxReceiveLength = zero;
    const int myImageID = comm_->getRank();
    for (Ordinal i = zero; i < numReceives_; ++i) {
      if (imagesFrom_[i] != myImageID) {
        if (lengthsFrom_[i] > maxReceiveLength) {
          maxReceiveLength = lengthsFrom_[i];
        }
      }
    }

    // initialize all of reverseDistributor's data members
    reverseDistributor_->lengthsTo_ = lengthsFrom_;
    reverseDistributor_->imagesTo_ = imagesFrom_;
    reverseDistributor_->indicesTo_ = indicesFrom_;
    reverseDistributor_->startsTo_ = startsFrom_;
    reverseDistributor_->lengthsFrom_ = lengthsTo_;
    reverseDistributor_->imagesFrom_ = imagesTo_;
    reverseDistributor_->indicesFrom_ = indicesTo_;
    reverseDistributor_->startsFrom_ = startsTo_;
    reverseDistributor_->numSends_ = numReceives_;
    reverseDistributor_->numReceives_ = numSends_;
    reverseDistributor_->selfMessage_ = selfMessage_;
    reverseDistributor_->maxSendLength_ = maxReceiveLength;
    reverseDistributor_->totalReceiveLength_ = totalSendLength;
    // Note: numExports_ was not copied
  }

  template <typename Ordinal>
  template <typename Packet>
  void Distributor<Ordinal>::doPostsAndWaits(
      const std::vector<Packet>& exports,
      const Ordinal numPackets,
            std::vector<Packet>& imports) 
  {
    doPosts(exports, numPackets, imports);
    doWaits();
  }

  template <typename Ordinal>
  template <typename Packet>
  void Distributor<Ordinal>::doPosts(
      const std::vector<Packet>& exports,
      const Ordinal numPackets,
            std::vector<Packet>& imports) 
  {
    using Teuchos::ArrayRCP;

    // start of actual doPosts function
    const Ordinal ZERO = Teuchos::OrdinalTraits<Ordinal>::zero();
    const Ordinal ONE  = Teuchos::OrdinalTraits<Ordinal>::one();
    const Ordinal myImageID = comm_->getRank();
    int selfReceiveOffset = 0;

    imports.resize(totalReceiveLength_ * numPackets);

    // allocate space in requests
    requests_.resize(0);
    requests_.reserve(numReceives_);

    // start up the Irecv's
    {
      int curBufferOffset = 0;
      for (int i = 0; i < (numReceives_ + (selfMessage_ ? 1 : 0)); ++i) {
        if (imagesFrom_[i] != myImageID) { 
          // receiving this one from another image
          // setup reference into imports of the appropriate size and at the appropriate place
          ArrayRCP<Packet> impptr = Teuchos::arcp(&imports[curBufferOffset],0,lengthsFrom_[i]*numPackets,false);
          requests_.push_back( Teuchos::ireceive<Ordinal,Packet>(*comm_,impptr,imagesFrom_[i]) );
        }
        else {
          // receiving this one from myself 
          // note that offset
          selfReceiveOffset = curBufferOffset;
        }
        curBufferOffset += lengthsFrom_[i]*numPackets;
      }
    }

    // wait for everyone else before posting ready-sends below to ensure that 
    // all non-blocking receives above have been posted
    Teuchos::barrier(*comm_);

    // setup scan through imagesTo_ list starting with higher numbered images
    // (should help balance message traffic)
    Ordinal numBlocks = numSends_+ selfMessage_;
    Ordinal imageIndex = ZERO;
    while ((imageIndex < numBlocks) && (imagesTo_[imageIndex] < myImageID)) {
      ++imageIndex;
    }
    if (imageIndex == numBlocks) {
      imageIndex = ZERO;
    }

    Ordinal selfNum = ZERO;
    Ordinal selfIndex = ZERO;

    if (indicesTo_.empty()) { // data is already blocked by processor
      for (Ordinal i = ZERO; i < numBlocks; ++i) {
        Ordinal p = i + imageIndex;
        if (p > (numBlocks - ONE)) {
          p -= numBlocks;
        }

        if (imagesTo_[p] != myImageID) {
          // sending it to another image
          Teuchos::ArrayView<const Packet> tmpSend(&exports[startsTo_[p]*numPackets],lengthsTo_[p]*numPackets);
          Teuchos::readySend<Ordinal,Packet>(*comm_,tmpSend,imagesTo_[p]);
        }
        else {
          // sending it to ourself
          selfNum = p;
        }
      }

      if (selfMessage_) {
        std::copy(exports.begin()+startsTo_[selfNum]*numPackets, exports.begin()+startsTo_[selfNum]*numPackets+lengthsTo_[selfNum]*numPackets, 
                  imports.begin()+selfReceiveOffset);
      }
    }
    else { // data is not blocked by image, use send buffer
      // allocate sendArray buffer
      std::vector<Packet> sendArray(maxSendLength_*numPackets); 

      for (Ordinal i = ZERO; i < numBlocks; ++i) {
        Ordinal p = i + imageIndex;
        if (p > (numBlocks - ONE)) {
          p -= numBlocks;
        }

        if (imagesTo_[p] != myImageID) { 
          // sending it to another image
          typename std::vector<Packet>::const_iterator srcBegin, srcEnd;
          int sendArrayOffset = 0;
          int j = startsTo_[p];
          for (Ordinal k = ZERO; k < lengthsTo_[p]; ++k, ++j) {
            srcBegin = exports.begin() + indicesTo_[j]*numPackets;
            srcEnd   = srcBegin + numPackets;
            std::copy( srcBegin, srcEnd, sendArray.begin()+sendArrayOffset );
            sendArrayOffset += numPackets;
          }
          Teuchos::ArrayView<const Packet> tmpSend(&sendArray[0],lengthsTo_[p]*numPackets);
          Teuchos::readySend<Ordinal,Packet>(*comm_,tmpSend,imagesTo_[p]);
        }
        else { 
          // sending it to myself
          selfNum = p;
          selfIndex = startsTo_[p];
        }
      }

      if (selfMessage_) {
        for (Ordinal k = ZERO; k < lengthsTo_[selfNum]; ++k) {
          std::copy( exports.begin()+indicesTo_[selfIndex]*numPackets,
                     exports.begin()+indicesTo_[selfIndex]*numPackets + numPackets,
                     imports.begin() + selfReceiveOffset );
          ++selfIndex;
          selfReceiveOffset += numPackets;
        }
      }
    }
  }

  template <typename Ordinal>
  void Distributor<Ordinal>::doWaits() 
  {
    const Ordinal & numReceives = getNumReceives();
    if (numReceives > Teuchos::OrdinalTraits<Ordinal>::zero()) {
      Teuchos::waitAll(*comm_,arrayViewFromVector(requests_));
      // Requests should all be null, clear them
#ifdef TEUCHOS_DEBUG
      for (typename std::vector<Teuchos::RCP<Teuchos::CommRequest> >::const_iterator i=requests_.begin(); 
           i != requests_.end(); ++i) 
      {
        TEST_FOR_EXCEPTION(*i != Teuchos::null, std::runtime_error,
            "Tpetra::Distributor<"<<Teuchos::OrdinalTraits<Ordinal>::name()
            <<">::doWaits(): Requests should be null after call to Teuchos::waitAll().");
      }
#endif
      requests_.clear();
    }
  }

  template <typename Ordinal>
  template <typename Packet>
  void Distributor<Ordinal>::doReversePostsAndWaits(
      const std::vector<Packet>& exports,
      const Ordinal numPackets,
            std::vector<Packet>& imports) 
  {
    doReversePosts(exports, numPackets, imports);
    doReverseWaits();
  }

  template <typename Ordinal>
  template <typename Packet>
  void Distributor<Ordinal>::doReversePosts(
      const std::vector<Packet>& exports,
      const Ordinal numPackets,
            std::vector<Packet>& imports) 
  {
    TEST_FOR_EXCEPTION(!indicesTo_.empty(),std::runtime_error,
        "Tpetra::Distributor<"<<Teuchos::OrdinalTraits<Ordinal>::name()<<">::doReversePosts(): Can only do reverse comm when original data is blocked by image.");
    if (reverseDistributor_ == Teuchos::null) {
      createReverseDistributor();
    }
    reverseDistributor_->doPosts(exports,numPackets,imports);
  }

  template <typename Ordinal>
  void Distributor<Ordinal>::doReverseWaits() 
  {
    // call doWaits() on the reverse Distributor, if it exists
    if (reverseDistributor_ != Teuchos::null) {
      reverseDistributor_->doWaits();
    }
  }

  //! print method inherited from Teuchos::Object
  template <typename Ordinal>
  void Distributor<Ordinal>::print(ostream& os) const 
  {
    int const myImageID = comm_->getRank();
    int const numImages = comm_->getSize();
    for (int i = 0; i < numImages; ++i) {
      comm_->barrier();
      if (i == myImageID) {
        os << "[Image " << myImageID << " of " << numImages << "]" << endl;
        os << " numExports: " << numExports_ << endl;
        os << " selfMessage: " << selfMessage_ << endl;
        os << " numSends_: " << numSends_ << endl;
        os << " imagesTo_: " << toString(imagesTo_) << endl;
        os << " startsTo_: " << toString(startsTo_) << endl;
        os << " lengthsTo_: " << toString(lengthsTo_) << endl;
        os << " maxSendLength_: " << maxSendLength_ << endl;
        os << " indicesTo_: " << toString(indicesTo_) << endl;
        os << " numReceives_: " << numReceives_ << endl;
        os << " totalReceiveLength_: " << totalReceiveLength_ << endl;
        os << " lengthsFrom_: " << toString(lengthsFrom_) << endl;
        os << " imagesFrom_: " << toString(imagesFrom_) << endl;
        os << " indicesFrom_: " << toString(indicesFrom_) << endl;
        os << " startsFrom_: " << toString(startsFrom_) << endl;
      }
    }
  }

  template <typename Ordinal>
  void Distributor<Ordinal>::computeReceives()
  {
    int myImageID = comm_->getRank();
    int numImages = comm_->getSize();
    const Ordinal ZERO = Teuchos::OrdinalTraits<Ordinal>::zero();
    const Ordinal  ONE = Teuchos::OrdinalTraits<Ordinal>::one();

    // to_nodes_from_me[i] == number of messages sent by this node to node i
    // the info in numSends_, imagesTo_, lengthsTo_ concerns the contiguous sends
    // therefore, each node will be listed in imagesTo_ at most once
    {
      std::vector<Ordinal> to_nodes_from_me(numImages, ZERO);
      for (int i = ZERO; i < (numSends_ + (selfMessage_ ? 1 : 0)); ++i) {
#       ifdef TEUCHOS_DEBUG
          SHARED_TEST_FOR_EXCEPTION(to_nodes_from_me[imagesTo_[i]] != 0, std::logic_error,
              "Tpetra::Distributor::createFromSends: logic error. please notify Tpetra team.",*comm_);
#       endif
        to_nodes_from_me[imagesTo_[i]] = ONE;
      }
      // each proc will get back only one item (hence, counts = ones) from the array of globals sums, 
      // namely that entry corresponding to the node, and detailing how many receives it has.
      // this total includes self sends
      std::vector<Ordinal> counts(numImages, 1);
      Teuchos::reduceAllAndScatter<Ordinal>(*comm_,Teuchos::REDUCE_SUM,numImages,&to_nodes_from_me[0],&counts[0],&numReceives_);
    }

    // assign these to length numReceives, with zero entries
    lengthsFrom_.assign(numReceives_, ZERO);
    imagesFrom_.assign(numReceives_, ZERO);

    // FINISH: why do these work? they are blocking sends, and should block until completion, which happens below
    // FINISH: consider switching them to non-blocking
    // NOTE: epetra has both, old (non-blocking) and new (mysterious)

    for (Ordinal i = ZERO; i < (numSends_ + (selfMessage_ ? 1 : 0)); ++i) {
      if (imagesTo_[i] != myImageID ) {
        // send a message to imagesTo_[i], telling him that our pattern sends him lengthsTo_[i] blocks of packets
        Teuchos::send(*comm_,lengthsTo_[i],imagesTo_[i]);
      }
      else {
        // set selfMessage_ to end block of recv arrays
        lengthsFrom_[numReceives_-ONE] = lengthsTo_[i];
        imagesFrom_[numReceives_-ONE] = myImageID;
      }
    }

    //
    for (Ordinal i = ZERO; i < (numReceives_ - (selfMessage_ ? 1 : 0)); ++i) {
      // receive one Ordinal variable from any sender.
      // store the value in lengthsFrom_[i], and store the sender's ImageID in imagesFrom_[i]
      // imagesFrom_[i] = comm_->receive(&lengthsFrom_[i], 1, -1);
      imagesFrom_[i] = Teuchos::receive(*comm_,-1,&lengthsFrom_[i]);
    }
    comm_->barrier();

    sortArrays(imagesFrom_, lengthsFrom_);

    // Compute indicesFrom_
    totalReceiveLength_ = std::accumulate(lengthsFrom_.begin(), lengthsFrom_.end(), ZERO);
    indicesFrom_.clear();
    indicesFrom_.reserve(totalReceiveLength_);
    for (Ordinal i = 0; i < totalReceiveLength_; ++i) {
      indicesFrom_.push_back(i);
    }

    startsFrom_.clear();
    startsFrom_.reserve(numReceives_);
    for (Ordinal i = ZERO, j = ZERO; i < numReceives_; ++i) {
      startsFrom_.push_back(j);
      j += lengthsFrom_[i];
    }

    if (selfMessage_) --numReceives_;

    comm_->barrier();
  }

  // FINISH: walkthrough
  template <typename Ordinal>
  void Distributor<Ordinal>::computeSends(
      const std::vector<Ordinal> & importIDs,
      const std::vector<Ordinal> & importImageIDs,
      std::vector<Ordinal>& exportIDs,
      std::vector<Ordinal>& exportImageIDs)
  {
    int myImageID = comm_->getRank();
    const Ordinal ZERO = Teuchos::OrdinalTraits<Ordinal>::zero();

    Ordinal numImports = importImageIDs.size();
    std::vector<Ordinal> importObjs(2*numImports);
    for (Ordinal i = ZERO; i < numImports; ++i ) {  
      importObjs[2*i]   = importIDs[i];
      importObjs[2*i+1] = myImageID;
    }

    Ordinal numExports;
    Distributor<Ordinal> tempPlan(comm_);
    tempPlan.createFromSends(importImageIDs, numExports);
    exportIDs.resize(numExports);
    exportImageIDs.resize(numExports);

    std::vector<Ordinal> exportObjs;
    tempPlan.doPostsAndWaits(importObjs,2,exportObjs);

    for (int i = 0; i < numExports; ++i) {
      exportIDs[i]      = exportObjs[2*i];
      exportImageIDs[i] = exportObjs[2*i+1];
    }
  }

} // namespace Tpetra

#endif // TPETRA_DISTRIBUTOR_HPP
