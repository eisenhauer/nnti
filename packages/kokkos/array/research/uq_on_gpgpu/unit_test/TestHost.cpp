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

#include <Kokkos_ProductTensor.hpp>
#include <Kokkos_SymmetricDiagonalSpec.hpp>
#include <Kokkos_BlockCrsMatrix.hpp>
#include <Kokkos_LegendrePolynomial.hpp>
#include <Kokkos_StochasticProductTensor.hpp>


//

#include <Kokkos_Host.hpp>

#include <Kokkos_Host_macros.hpp>
#include <impl/Kokkos_ProductTensor_macros.hpp>
#include <impl/Kokkos_SymmetricDiagonalSpec_macros.hpp>
#include <impl/Kokkos_LegendrePolynomial_macros.hpp>
#include <impl/Kokkos_StochasticProductTensor_macros.hpp>
#include <Kokkos_Clear_macros.hpp>

#include <Host/Kokkos_Host_ProductTensor.hpp>
#include <Host/Kokkos_Host_StochasticProductTensor.hpp>
#include <Host/Kokkos_Host_SymmetricDiagonalSpec.hpp>
#include <Host/Kokkos_Host_BlockCrsMatrix.hpp>

//

#include <TestBlockCrsMatrix.hpp>
#include <TestTensorCrsMatrix.hpp>
#include <TestStochastic.hpp>

int mainHost()
{
  Kokkos::Host::initialize( Kokkos::Host::SetThreadCount(4) );

//  unit_test::test_dense<Kokkos::Host>();
//  unit_test::test_diagonal<Kokkos::Host>();
//  unit_test::test_other<Kokkos::Host>();



  unit_test::test_integration<1>();
  unit_test::test_integration<2>();
  unit_test::test_integration<3>();
  unit_test::test_integration<4>();
  unit_test::test_integration<5>();
  unit_test::test_integration<6>();
  unit_test::test_integration<7>();
  unit_test::test_integration<8>();
  unit_test::test_integration<9>();
  unit_test::test_integration<10>();
  unit_test::test_inner_product_legengre_polynomial<10,Kokkos::Host>();
  unit_test::test_triple_product_legendre_polynomial<4,Kokkos::Host>();

  unit_test::test_product_tensor<Kokkos::Host>( std::vector<int>( 2 , 1 ) );
  unit_test::test_product_tensor<Kokkos::Host>( std::vector<int>( 3 , 2 ) );
  unit_test::test_product_tensor<Kokkos::Host>( std::vector<int>( 5 , 1 ) );

  unit_test::test_block_crs_matrix<Kokkos::Host>( 1 , 2 );
  unit_test::test_block_crs_matrix<Kokkos::Host>( 1 , 5 );
  unit_test::test_block_crs_matrix<Kokkos::Host>( 2 , 1 );
  unit_test::test_block_crs_matrix<Kokkos::Host>( 3 , 1 );

  unit_test_tensor::test_tensor_crs_matrix<Kokkos::Host,long>( 1 , 2 );
  unit_test_tensor::test_tensor_crs_matrix<Kokkos::Host,long>( 1 , 5 );
  unit_test_tensor::test_tensor_crs_matrix<Kokkos::Host,long>( 2 , 1 );
  unit_test_tensor::test_tensor_crs_matrix<Kokkos::Host,long>( 5 , 1 );

  unit_test::test_product_tensor_matrix<double,Kokkos::Host>( std::vector<int>( 3 , 2 ) , 3 );
  unit_test::test_product_tensor_diagonal_matrix<double,Kokkos::Host>( std::vector<int>( 3 , 2 ) , 3 );

  std::cout << "Stress tests:" << std::endl ;

  unit_test::test_block_crs_matrix<Kokkos::Host>( 10 , 8 );
  unit_test::test_block_crs_matrix<Kokkos::Host>( 11 , 8 );
  unit_test::test_block_crs_matrix<Kokkos::Host>( 12 , 10 );
  unit_test::test_block_crs_matrix<Kokkos::Host>( 13 , 10 );
  unit_test_tensor::test_tensor_crs_matrix<Kokkos::Host,long>( 100 , 10 );

  Kokkos::Host::finalize();

  return 0 ;
}



