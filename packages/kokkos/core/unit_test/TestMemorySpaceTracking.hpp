/*
//@HEADER
// ************************************************************************
//
//   Kokkos: Manycore Performance-Portable Multidimensional Arrays
//              Copyright (2012) Sandia Corporation
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
// Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include <iostream>
#include <Kokkos_Core.hpp>

/*--------------------------------------------------------------------------*/

namespace {

template<class Arg1>
class TestMemorySpace {
public:

  typedef typename Arg1::memory_space MemorySpace;
  TestMemorySpace() { run_test(); }

  void run_test()
  {
    Kokkos::View<int* ,Arg1> invalid;
    ASSERT_EQ(0u, invalid.tracker().ref_count() );

    {
      Kokkos::View<int* ,Arg1> a("A",10);

      ASSERT_EQ(1u, a.tracker().ref_count() );

      {
        Kokkos::View<int* ,Arg1> b = a;
        ASSERT_EQ(2u, b.tracker().ref_count() );

        Kokkos::View<int* ,Arg1> D("D",10);
        ASSERT_EQ(1u, D.tracker().ref_count() );

        {
          Kokkos::View<int* ,Arg1> E("E",10);
          ASSERT_EQ(1u, E.tracker().ref_count() );
        }

        ASSERT_EQ(2u, b.tracker().ref_count() );
      }
      ASSERT_EQ(1u, a.tracker().ref_count() );
    }
  }
};

}

/*--------------------------------------------------------------------------*/



