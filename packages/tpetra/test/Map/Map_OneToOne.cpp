/*
// @HEADER
// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ************************************************************************
// @HEADER
*/

// Some Macro Magic to ensure that if CUDA and KokkosCompat is enabled
// only the .cu version of this file is actually compiled
#include <Tpetra_config.h>
#ifdef HAVE_TPETRA_KOKKOSCOMPAT
#include <KokkosCore_config.h>
#ifdef KOKKOS_USE_CUDA_BUILD
  #define DO_COMPILATION
#else
  #ifndef KOKKOS_HAVE_CUDA
    #define DO_COMPILATION
  #endif
#endif
#else
  #define DO_COMPILATION
#endif

#ifdef DO_COMPILATION

#include "Teuchos_UnitTestHarness.hpp"
#include <Teuchos_Tuple.hpp>
#include "Tpetra_DefaultPlatform.hpp"
#include "Tpetra_Map.hpp"
#include "Tpetra_TieBreak.hpp"

#define NUM_GLOBAL_ELEMENTS 100

using Teuchos::RCP;
using Teuchos::Array;
using Teuchos::ArrayView;
typedef int LO;
typedef int GO;
typedef Tpetra::DefaultPlatform::DefaultPlatformType Platform;
typedef Tpetra::DefaultPlatform::DefaultPlatformType::NodeType Node;
typedef Tpetra::Map<LO, GO, Node> Map;
typedef Tpetra::Directory<LO, GO, Node> Directory;


namespace {

  template <typename LO,typename GO>
  class GotoLowTieBreak : public Tpetra::Details::TieBreak<LO,GO> {
  public:
    std::size_t selectedIndex(GO GID,const std::vector<std::pair<int,LO> > & pid_and_lid) const
    {
      int min = -1; std::size_t index = 0;
      for(std::size_t i=0;i<pid_and_lid.size();i++) {
        if(pid_and_lid[i].first<min) {
          min = pid_and_lid[i].first;
          index = i;
        }
      }

      return index;
    }
  };

  // Create a contiguous Map that is already one-to-one, and test
  // whether createOneToOne returns a Map that is the same.
  TEUCHOS_UNIT_TEST( OneToOne, AlreadyOneToOneContig)
  {
    Platform &platform = Tpetra::DefaultPlatform::getDefaultPlatform();
    RCP<const Teuchos::Comm<int> > comm = platform.getComm();
    RCP<Node> node = platform.getNode();

    RCP<const Map> map = Tpetra::createUniformContigMapWithNode<LO, GO, Node> (NUM_GLOBAL_ELEMENTS, comm, node);

    RCP<const Map> new_map = Tpetra::createOneToOne<LO, GO, Node> (map);

    TEST_ASSERT(map->isSameAs(*new_map));
  }

  // Create a noncontiguous Map that is already one-to-one, and test
  // whether createOneToOne returns a Map that is the same.
  // Use index base 0.
  TEUCHOS_UNIT_TEST( OneToOne, AlreadyOneToOneNonContigIndexBaseZero )
  {
    using Teuchos::Array;
    using Teuchos::outArg;
    using Teuchos::REDUCE_SUM;
    using Teuchos::reduceAll;

    Platform &platform = Tpetra::DefaultPlatform::getDefaultPlatform ();
    RCP<const Teuchos::Comm<int> > comm = platform.getComm ();
    RCP<Node> node = platform.getNode ();
    const int numProcs = comm->getSize ();
    const int myRank = comm->getRank ();
    const GO indexBase = 0;

    // Compute the number of Map elements on my process.
    const GO quotient = static_cast<GO> (NUM_GLOBAL_ELEMENTS) / numProcs;
    const GO remainder = static_cast<GO> (NUM_GLOBAL_ELEMENTS) - quotient * numProcs;
    const GO myNumElts = (myRank < remainder) ? (quotient + 1) : quotient;

    // Verify that we did the above computation correctly.
    GO globalNumElts = 0;
    reduceAll<int, GO> (*comm, REDUCE_SUM, myNumElts, outArg (globalNumElts));
    TEUCHOS_TEST_FOR_EXCEPTION(
      globalNumElts != static_cast<GO> (NUM_GLOBAL_ELEMENTS), std::logic_error,
      "Computed number of Map elements on my process incorrectly.  "
      "This is a bug in the Tpetra test.");

    const GO myEltStart = (myRank < remainder) ?
      (myRank * (quotient + 1)) :
      (remainder * (quotient + 1) + (myRank - remainder) * quotient);
    Array<GO> myElts (myNumElts);
    for (GO k = 0; k < myNumElts; ++k) {
      myElts[k] = indexBase + k + myEltStart;
    }

    RCP<const Map> inputMap =
      Teuchos::rcp (new Map (globalNumElts, myElts (), indexBase, comm, node));
    RCP<const Map> outputMap = Tpetra::createOneToOne<LO, GO, Node> (inputMap);

    TEST_ASSERT(inputMap->isSameAs(*outputMap));
  }


  // Create a noncontiguous Map that is already one-to-one, and test
  // whether createOneToOne returns a Map that is the same.
  // Use index base 1, just to make sure that it works.
  TEUCHOS_UNIT_TEST( OneToOne, AlreadyOneToOneNonContigIndexBaseOne )
  {
    using Teuchos::Array;
    using Teuchos::outArg;
    using Teuchos::REDUCE_SUM;
    using Teuchos::reduceAll;

    Platform &platform = Tpetra::DefaultPlatform::getDefaultPlatform ();
    RCP<const Teuchos::Comm<int> > comm = platform.getComm ();
    RCP<Node> node = platform.getNode ();
    const int numProcs = comm->getSize ();
    const int myRank = comm->getRank ();
    const GO indexBase = 1;

    // Compute the number of Map elements on my process.
    const GO quotient = static_cast<GO> (NUM_GLOBAL_ELEMENTS) / numProcs;
    const GO remainder = static_cast<GO> (NUM_GLOBAL_ELEMENTS) - quotient * numProcs;
    const GO myNumElts = (myRank < remainder) ? (quotient + 1) : quotient;

    // Verify that we did the above computation correctly.
    GO globalNumElts = 0;
    reduceAll<int, GO> (*comm, REDUCE_SUM, myNumElts, outArg (globalNumElts));
    TEUCHOS_TEST_FOR_EXCEPTION(
      globalNumElts != static_cast<GO> (NUM_GLOBAL_ELEMENTS), std::logic_error,
      "Computed number of Map elements on my process incorrectly.  "
      "This is a bug in the Tpetra test.");

    // Build the list of global indices (GIDs) owned by the calling
    // process.  Add them in reverse order, to ensure that the
    // resulting Map is not contiguous.
    const GO myEltStart = (myRank < remainder) ?
      (myRank * (quotient + 1)) :
      (remainder * (quotient + 1) + (myRank - remainder) * quotient);
    Array<GO> myElts (myNumElts);
    // Iterate from myNumElts to 1, not myNumElts-1 to 0, in case GO
    // is unsigned.  (k >= 0 is always true if k is unsigned, so the
    // loop would never finish.)
    for (GO k = myNumElts; k > 0; --k) {
      myElts[k-1] = indexBase + (k - 1) + myEltStart;
    }

    RCP<const Map> inputMap =
      Teuchos::rcp (new Map (globalNumElts, myElts (), indexBase, comm, node));
    RCP<const Map> outputMap = Tpetra::createOneToOne<LO, GO, Node> (inputMap);

    TEST_ASSERT(inputMap->isSameAs(*outputMap));
  }


  TEUCHOS_UNIT_TEST(OneToOne, LargeOverlap)
  {
    //Creates a map with large overlaps
    Platform &platform = Tpetra::DefaultPlatform::getDefaultPlatform();
    RCP<const Teuchos::Comm<int> > comm = platform.getComm();
    RCP<Node> node = platform.getNode();
    const int myRank = comm->getRank();
    const int numProc = comm->getSize();

    int unit = (NUM_GLOBAL_ELEMENTS/(numProc+1));
    int left_overs = (NUM_GLOBAL_ELEMENTS%(numProc+1));
    int num_loc_elems=0;
    Array<GO> elementList;

    if(myRank<numProc-1)
    {
      elementList = Array<GO>(2*unit);
      num_loc_elems=2*unit;
    }
    else //I'm the last proc and need to take the leftover elements.
    {
      elementList = Array<GO>((2*unit)+(left_overs));
      num_loc_elems=(2*unit)+(left_overs);
    }

    for(int i=(myRank*unit),j=0;i<(myRank*unit)+(2*unit);++i,++j)
    {
      elementList[j]=i;
    }
    //Don't forget to assign leftovers to the last proc!
    if(myRank==numProc-1 && left_overs!= 0)
    {
      for(int i=(myRank*unit)+2*unit, j=2*unit;i<NUM_GLOBAL_ELEMENTS;++i,++j)
      {
        elementList[j]=i;
      }
    }

    RCP<const Map> map = Tpetra::createNonContigMapWithNode<LO,GO>(elementList,comm,node);
    //std::cout<<map->description();
    RCP<const Map> new_map = Tpetra::createOneToOne<LO,GO,Node>(map);
    //std::cout<<new_map->description();
    //Now we need to test if we lost anything.

    Array<int> nodeIDlist(num_loc_elems); //This is unimportant. It's only used in the following function.
    Tpetra::LookupStatus stat=new_map->getRemoteIndexList(elementList (), nodeIDlist ()); //this style from a tpetra test.
    //If we pass this we didn't lose any IDs.
    TEST_EQUALITY_CONST(stat, Tpetra::AllIDsPresent);
    TEST_EQUALITY(new_map->getGlobalNumElements(),NUM_GLOBAL_ELEMENTS);

    //Now we need to make sure they're in the right place. Keep in mind Tpetra
    //Directory gives precidence to the higher numbered proc.

    ArrayView<const GO> my_owned = new_map->getNodeElementList();

    for(int i=(myRank*unit),j=0;i<(myRank*unit)+unit; ++i,++j)
    {
      TEST_EQUALITY(my_owned[j], i);
    }
    //And the last proc should have all that it started with.
    if(myRank==numProc-1)
    {
      for(int i=0;i<num_loc_elems;++i)
      {
        TEST_EQUALITY(elementList[i],my_owned[i]);
      }
    }
  }

  TEUCHOS_UNIT_TEST(OneToOne, AllOnOneProc)
  {
    //Will create a non-contig map with all of the elements on a single
    //processor. Nothing should change.
    Platform &platform = Tpetra::DefaultPlatform::getDefaultPlatform();
    RCP<const Teuchos::Comm<int> > comm = platform.getComm();
    RCP<Node> node = platform.getNode();
    const int myRank = comm->getRank();

    Array<GO> elementList;
    if(myRank==0)
    {
      elementList = Array<GO>(NUM_GLOBAL_ELEMENTS);
      for(int i=0;i<NUM_GLOBAL_ELEMENTS;++i)
      {
        elementList[i]=i;
      }
    }
    else
    {
      elementList = Array<GO>(0);
    }
    RCP<const Map> map = Tpetra::createNonContigMapWithNode<LO,GO>(elementList,comm,node);
    RCP<const Map> new_map = Tpetra::createOneToOne<LO,GO,Node>(map);
    TEST_ASSERT(map->isSameAs(*new_map));
  }

  TEUCHOS_UNIT_TEST(OneToOne, NoIDs)
  {
    //An empty map.
    Platform &platform = Tpetra::DefaultPlatform::getDefaultPlatform();
    RCP<const Teuchos::Comm<int> > comm = platform.getComm();
    RCP<Node> node = platform.getNode();

    Array<GO> elementList (0);

    RCP<const Map> map = Tpetra::createNonContigMapWithNode<LO,GO>(elementList,comm,node);
    RCP<const Map> new_map = Tpetra::createOneToOne<LO,GO,Node>(map);
    TEST_ASSERT(map->isSameAs(*new_map));


  }

  TEUCHOS_UNIT_TEST(OneToOne, AllOwnEvery)
  {
    //Every processor starts by owning all of them.
    //After one-to-one, only the last processor should own all of them.

    Platform &platform = Tpetra::DefaultPlatform::getDefaultPlatform();
    RCP<const Teuchos::Comm<int> > comm = platform.getComm();
    RCP<Node> node = platform.getNode();
    const int myRank = comm->getRank();
    const int numProc = comm->getSize();

    Array<GO> elementList (NUM_GLOBAL_ELEMENTS);

    for(int i=0;i<NUM_GLOBAL_ELEMENTS;++i)
    {
      elementList[i]=i;
    }
    RCP<const Map> map = Tpetra::createNonContigMapWithNode<LO,GO>(elementList,comm,node);
    RCP<const Map> new_map = Tpetra::createOneToOne<LO,GO,Node>(map);

    if(myRank<numProc-1)//I shouldn't have any elements.
    {
      TEST_EQUALITY(new_map->getNodeNumElements(),0);
    }
    else//I should have all of them.
    {
      TEST_EQUALITY(new_map->getNodeNumElements(),NUM_GLOBAL_ELEMENTS);
    }
  }

  TEUCHOS_UNIT_TEST(OneToOne, TieBreak)
  {
    //Creates a map with large overlaps
    Platform &platform = Tpetra::DefaultPlatform::getDefaultPlatform();
    RCP<const Teuchos::Comm<int> > comm = platform.getComm();
    RCP<Node> node = platform.getNode();
    const int myRank = comm->getRank();
    const int numProc = comm->getSize();

    int unit = (NUM_GLOBAL_ELEMENTS/(numProc+1));
    int left_overs = (NUM_GLOBAL_ELEMENTS%(numProc+1));
    int num_loc_elems=0;
    Array<GO> elementList;

    if(myRank<numProc-1)
    {
      elementList = Array<GO>(2*unit);
      num_loc_elems=2*unit;
    }
    else //I'm the last proc and need to take the leftover elements.
    {
      elementList = Array<GO>((2*unit)+(left_overs));
      num_loc_elems=(2*unit)+(left_overs);
    }

    for(int i=(myRank*unit),j=0;i<(myRank*unit)+(2*unit);++i,++j)
    {
      elementList[j]=i;
    }
    //Don't forget to assign leftovers to the last proc!
    if(myRank==numProc-1 && left_overs!= 0)
    {
      for(int i=(myRank*unit)+2*unit, j=2*unit;i<NUM_GLOBAL_ELEMENTS;++i,++j)
      {
        elementList[j]=i;
      }
    }

    GotoLowTieBreak<LO,GO> tie_break;
    RCP<const Map> map = Tpetra::createNonContigMapWithNode<LO,GO>(elementList,comm,node);
    RCP<const Map> new_map = Tpetra::createOneToOne<LO,GO,Node>(map,tie_break);
    //Now we need to test if we lost anything.

    Array<int> nodeIDlist(num_loc_elems); //This is unimportant. It's only used in the following function.
    Tpetra::LookupStatus stat = new_map->getRemoteIndexList(elementList (), nodeIDlist ()); //this style from a tpetra test.
    //If we pass this we didn't lose any IDs.
    TEST_EQUALITY_CONST(stat, Tpetra::AllIDsPresent);
    TEST_EQUALITY(new_map->getGlobalNumElements(),NUM_GLOBAL_ELEMENTS);

    //Now we need to make sure they're in the right place. Keep in mind we are
    //overriding the tpetra directory precidence

    ArrayView<const GO> my_owned = new_map->getNodeElementList();

    if(myRank==0) { // zero rank should have everythin
      for(int i=0;i<num_loc_elems;++i) {
        TEST_EQUALITY(elementList[i],my_owned[i]);
      }
    }
    else {
      for(int i=(myRank*unit+unit),j=0;i<(myRank*unit)+2*unit; ++i,++j) {
        TEST_EQUALITY(my_owned[j], i);
      }
    }
  }
}

#endif  //DO_COMPILATION

