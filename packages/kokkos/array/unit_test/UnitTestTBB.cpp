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

#include <Kokkos_DeviceHost.hpp>
#include <Kokkos_DeviceHost_ValueView.hpp>
#include <Kokkos_DeviceHost_MultiVectorView.hpp>
#include <Kokkos_DeviceHost_MDArrayView.hpp>
#include <Kokkos_DeviceHost_ParallelFor.hpp>
#include <Kokkos_DeviceHost_ParallelReduce.hpp>

#include <Kokkos_DeviceTBB.hpp>
#include <Kokkos_DeviceTBB_ValueView.hpp>
#include <Kokkos_DeviceTBB_MultiVectorView.hpp>
#include <Kokkos_DeviceTBB_MDArrayView.hpp>
#include <Kokkos_DeviceTBB_ParallelFor.hpp>
#include <Kokkos_DeviceTBB_ParallelReduce.hpp>

//----------------------------------------------------------------------------

#include <Kokkos_DeviceTBB_macros.hpp>

#include <UnitTestDeviceMemoryManagement.hpp>
#include <UnitTestValueView.hpp>
#include <UnitTestMultiVectorView.hpp>
#include <UnitTestMDArrayView.hpp>
#include <UnitTestMDArrayDeepCopy.hpp>
#include <UnitTestMDArrayIndexMap.hpp>
#include <UnitTestReduce.hpp>

#include <Kokkos_DeviceClear_macros.hpp>

namespace Test {

class tbb : public ::testing::Test {
  protected:
    static void SetUpTestCase() {
      //Kokkos::DeviceTBB::initialize( 4 );
    }
    static void TearDownTestCase() {
      //Kokkos::DeviceTBB::finalize();
    }
};



TEST_F( tbb, memory_management_double) {
  UnitTestDeviceMemoryManagement< double, Kokkos::DeviceTBB >();
}

TEST_F( tbb, memory_management_int) {
  UnitTestDeviceMemoryManagement< int, Kokkos::DeviceTBB >();
}

TEST_F( tbb, value_view_double) {
  UnitTestValueView< double, Kokkos::DeviceTBB >();
}

TEST_F( tbb, value_view_int) {
  UnitTestValueView< int, Kokkos::DeviceTBB >();
}

TEST_F( tbb, multi_vector_view_double) {
  UnitTestMultiVectorView< double, Kokkos::DeviceTBB >();
}

TEST_F( tbb, multi_vector_view_int) {
  UnitTestMultiVectorView< int, Kokkos::DeviceTBB >();
}

TEST_F( tbb, mdarray_view_double) {
  UnitTestMDArrayView< double, Kokkos::DeviceTBB >();
}

TEST_F( tbb, mdarray_view_int) {
  UnitTestMDArrayView< int, Kokkos::DeviceTBB >();
}

TEST_F( tbb, mdarray_deep_copy_double) {
  UnitTestMDArrayDeepCopy< double, Kokkos::DeviceTBB >();
}

TEST_F( tbb, mdarray_deep_copy_int) {
  UnitTestMDArrayDeepCopy< int, Kokkos::DeviceTBB >();
}

TEST_F( tbb, mdarray_index_map) {
  UnitTestMDArrayIndexMap< Kokkos::DeviceTBB >();
}

TEST_F( tbb, long_reduce) {
  UnitTestReduce< long ,   Kokkos::DeviceTBB >( 1000000 );
}

TEST_F( tbb, double_reduce) {
  UnitTestReduce< double ,   Kokkos::DeviceTBB >( 1000000 );
}

//TEST_F( tbb, long_multi_reduce) {
//  UnitTestReduceMulti< long , Kokkos::DeviceTBB >( 1000000 , 7 );
//}

} // namespace test

