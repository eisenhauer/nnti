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

#include <stdio.h>

#include <Kokkos_Core.hpp>

/*--------------------------------------------------------------------------*/

namespace Test {
namespace {

template< class ExecSpace >
struct TestRange {

  typedef int value_type ; ///< typedef required for the parallel_reduce

  typedef Kokkos::View<int*,ExecSpace> view_type ;

  view_type m_flags ;

  struct VerifyInitTag {};
  struct ResetTag {};
  struct VerifyResetTag {};

  TestRange( const size_t N )
    : m_flags( Kokkos::allocate_without_initializing , "flags" , N )
    {}

  static void test_for( const size_t N )
    {
      TestRange functor(N);

      typename view_type::HostMirror host_flags = Kokkos::create_mirror_view( functor.m_flags );

      Kokkos::parallel_for( Kokkos::RangePolicy<ExecSpace>(0,N) , functor );
      Kokkos::parallel_for( Kokkos::RangePolicy<ExecSpace,VerifyInitTag>(0,N) , functor );

      Kokkos::deep_copy( host_flags , functor.m_flags );

      size_t error_count = 0 ;
      for ( size_t i = 0 ; i < N ; ++i ) {
        if ( int(i) != host_flags(i) ) ++error_count ;
      }
      ASSERT_EQ( error_count , size_t(0) );

      Kokkos::parallel_for( Kokkos::RangePolicy<ExecSpace,ResetTag>(0,N) , functor );
      Kokkos::parallel_for( Kokkos::RangePolicy<ExecSpace,VerifyResetTag>(0,N) , functor );

      Kokkos::deep_copy( host_flags , functor.m_flags );

      error_count = 0 ;
      for ( size_t i = 0 ; i < N ; ++i ) {
        if ( int(2*i) != host_flags(i) ) ++error_count ;
      }
      ASSERT_EQ( error_count , size_t(0) );
    }

  KOKKOS_INLINE_FUNCTION
  void operator()( const int i ) const
    { m_flags(i) = i ; }

  KOKKOS_INLINE_FUNCTION
  void operator()( const VerifyInitTag & , const int i ) const
    { if ( i != m_flags(i) ) { printf("TestRange::test_for error at %d != %d\n",i,m_flags(i)); } }

  KOKKOS_INLINE_FUNCTION
  void operator()( const ResetTag & , const int i ) const
    { m_flags(i) = 2 * m_flags(i); }

  KOKKOS_INLINE_FUNCTION
  void operator()( const VerifyResetTag & , const int i ) const
    { if ( 2 * i != m_flags(i) ) { printf("TestRange::test_for error at %d != %d\n",i,m_flags(i)); } }

  //----------------------------------------

  struct OffsetTag {};

  static void test_reduce( const size_t N )
    {
      TestRange functor(N);
      Kokkos::View<int,Kokkos::HostSpace> total("total");

      Kokkos::parallel_for(    Kokkos::RangePolicy<ExecSpace>(0,N) , functor );

      Kokkos::parallel_reduce( Kokkos::RangePolicy<ExecSpace>(0,N) , functor , total );
      // sum( 0 .. N-1 )
      ASSERT_EQ( size_t((N-1)*(N)/2) , size_t(*total) );

      Kokkos::parallel_reduce( Kokkos::RangePolicy<ExecSpace,OffsetTag>(0,N) , functor , total );
      // sum( 1 .. N )
      ASSERT_EQ( size_t((N)*(N+1)/2) , size_t(*total) );
    }

  KOKKOS_INLINE_FUNCTION
  void operator()( const int i , value_type & update ) const
    { update += m_flags(i); }

  KOKKOS_INLINE_FUNCTION
  void operator()( const OffsetTag & , const int i , value_type & update ) const
    { update += 1 + m_flags(i); }

  //----------------------------------------

  static void test_scan( const size_t N )
    {
      TestRange functor(N);
      Kokkos::View<int,Kokkos::HostSpace> total("total");

      Kokkos::parallel_for( Kokkos::RangePolicy<ExecSpace>(0,N) , functor );

      Kokkos::parallel_scan( Kokkos::RangePolicy<ExecSpace,OffsetTag>(0,N) , functor );
    }

  KOKKOS_INLINE_FUNCTION
  void operator()( const OffsetTag & , const int i , value_type & update , bool final ) const
    {
      update += m_flags(i);

      if ( final ) {
        if ( update != (i*(i+1))/2 ) {
          printf("TestRange::test_scan error %d : %d != %d\n",i,(i*(i+1))/2,m_flags(i));
        }
      }
    }
};

} /* namespace */
} /* namespace Test */

/*--------------------------------------------------------------------------*/

