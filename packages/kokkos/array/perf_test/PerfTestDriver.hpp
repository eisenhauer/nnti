/*
//@HEADER
// ************************************************************************
// 
//          Kokkos: Node API and Parallel Node Kernels
//              Copyright (2008) Sandia Corporation
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
//@HEADER
*/

#include <iostream>
#include <string>

#define KOKKOS_MACRO_IMPL_TO_STRING( X ) #X
#define KOKKOS_MACRO_TO_STRING( X )  KOKKOS_MACRO_IMPL_TO_STRING( X )

//------------------------------------------------------------------------

namespace Test {

enum { NUMBER_OF_TRIALS = 5 };

template< class DeviceType >
void run_test_hexgrad( int exp_beg , int exp_end );

template< class DeviceType >
void run_test_gramschmidt( int exp_beg , int exp_end );

template<>
void run_test_hexgrad< KOKKOS_MACRO_DEVICE >( int exp_beg , int exp_end )
{
  std::string label_hexgrad ;
  label_hexgrad.append( "\"HexGrad< double , " );
  label_hexgrad.append( KOKKOS_MACRO_TO_STRING( KOKKOS_MACRO_DEVICE ) );
  label_hexgrad.append( " >\"" );

  for (int i = exp_beg ; i < exp_end ; ++i) {
    double min_seconds = 0.0 ;
    double max_seconds = 0.0 ;
    double avg_seconds = 0.0 ;

    const int parallel_work_length = 1<<i;

    for ( int j = 0 ; j < NUMBER_OF_TRIALS ; ++j ) {
      const double seconds = HexGrad< KOKKOS_MACRO_DEVICE >::test(parallel_work_length) ;

      if ( 0 == j ) {
        min_seconds = seconds ;
        max_seconds = seconds ;
      }
      else {
        if ( seconds < min_seconds ) min_seconds = seconds ;
        if ( seconds > max_seconds ) max_seconds = seconds ;
      }
      avg_seconds += seconds ;
    }
    avg_seconds /= NUMBER_OF_TRIALS ;

    std::cout << label_hexgrad
      << " , " << parallel_work_length
      << " , " << min_seconds
      << " , " << ( min_seconds / parallel_work_length )
      << std::endl ;
  }
}

template<>
void run_test_gramschmidt< KOKKOS_MACRO_DEVICE >( int exp_beg , int exp_end )
{
  std::string label_gramschmidt ;
  label_gramschmidt.append( "\"GramSchmidt< double , " );
  label_gramschmidt.append( KOKKOS_MACRO_TO_STRING( KOKKOS_MACRO_DEVICE ) );
  label_gramschmidt.append( " >\"" );

  for (int i = exp_beg ; i < exp_end ; ++i) {
    double min_seconds = 0.0 ;
    double max_seconds = 0.0 ;
    double avg_seconds = 0.0 ;

    const int parallel_work_length = 1<<i;

    for ( int j = 0 ; j < NUMBER_OF_TRIALS ; ++j ) {
      const double seconds = ModifiedGramSchmidt< double , KOKKOS_MACRO_DEVICE >::test(parallel_work_length, 32 ) ;

      if ( 0 == j ) {
        min_seconds = seconds ;
        max_seconds = seconds ;
      }
      else {
        if ( seconds < min_seconds ) min_seconds = seconds ;
        if ( seconds > max_seconds ) max_seconds = seconds ;
      }
      avg_seconds += seconds ;
    }
    avg_seconds /= NUMBER_OF_TRIALS ;

    std::cout << label_gramschmidt
      << " , " << parallel_work_length
      << " , " << min_seconds
      << " , " << ( min_seconds / parallel_work_length )
      << std::endl ;
  }
}

}

