/*
// @HEADER
// ***********************************************************************
// 
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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

#include <Teuchos_UnitTestHarness.hpp>
#include <Teuchos_Tuple.hpp>
#include "Tpetra_DefaultPlatform.hpp"
#include "Tpetra_Map.hpp"

using Teuchos::RCP;
using Teuchos::Array;
using Teuchos::tuple;

//// 
TEUCHOS_UNIT_TEST( Map, ProblematicLookup ) 
{
  Teuchos::oblackholestream blackhole;
  RCP<const Teuchos::Comm<int> > comm = Tpetra::DefaultPlatform::getDefaultPlatform().getComm();
  const int MyPid = comm->getRank();
  /**********************************************************************************/
  // Map in question:
  // -----------------------------
  // SRC Map  Processor 0: Global IDs = 0 1 2 3 4 5 6  
  //          Processor 1: Global IDs =                    9 10 11 12 13 14 15
  //
  // Lookup of global IDs 7 8 should return IDNotFound
  // 
  RCP<const Tpetra::Map<int,int> > map;
  if (MyPid == 0) {
    Array<int> gids( tuple<int>(1) );
    map = Tpetra::createNonContigMap<int>( gids().getConst() , comm );
  }
  else {
    Array<int> gids( tuple<int>(3) );
    map = Tpetra::createNonContigMap<int>( gids().getConst() , comm );
  }
  Array<int> nodeIDs( 1 );
  Tpetra::LookupStatus lookup = map->getRemoteIndexList( tuple<int>(2), nodeIDs() );
  TEST_EQUALITY_CONST( map->isDistributed(), true )
  TEST_EQUALITY_CONST( map->isContiguous(), false )
  TEST_EQUALITY_CONST( lookup, Tpetra::IDNotPresent )
  TEST_COMPARE_ARRAYS( nodeIDs(), tuple<int>(-1) );
}
