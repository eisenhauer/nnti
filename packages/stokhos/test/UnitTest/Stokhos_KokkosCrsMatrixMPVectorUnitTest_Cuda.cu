// @HEADER
// ***********************************************************************
//
//                           Stokhos Package
//                 Copyright (2009) Sandia Corporation
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
// Questions? Contact Eric T. Phipps (etphipp@sandia.gov).
//
// ***********************************************************************
// @HEADER

#include "Teuchos_UnitTestHarness.hpp"
#include "Teuchos_UnitTestRepository.hpp"
#include "Teuchos_GlobalMPISession.hpp"

#include "Stokhos_KokkosCrsMatrixMPVectorUnitTest.hpp"
#include "Kokkos_CrsMatrix_MP_Vector_Cuda.hpp"

// Instantiate test for Cuda device
using Kokkos::Cuda;
CRSMATRIX_MP_VECTOR_TESTS_SCALAR_ORDINAL_DEVICE( double, int, Cuda )

template <typename Ordinal, typename Scalar, typename MultiplyOp,
          Ordinal NumPerThread, Ordinal ThreadsPerVector>
bool test_cuda_static_fixed_embedded_vector(Ordinal num_blocks,
                                            Ordinal num_vec_threads,
                                            Ordinal num_row_threads,
                                            Teuchos::FancyOStream& out) {
  typedef Kokkos::Cuda Device;

  const Ordinal VectorSize = NumPerThread * ThreadsPerVector;
  typedef Stokhos::StaticFixedStorage<Ordinal,Scalar,VectorSize,Device> Storage;
  typedef Sacado::MP::Vector<Storage> Vector;

  const Ordinal nGrid = 5;
  Kokkos::DeviceConfig dev_config(num_blocks, num_vec_threads, num_row_threads);

  bool success = test_embedded_vector<Vector>(
    nGrid, VectorSize, dev_config, MultiplyOp(), out);

  return success;
}

// Test default configuration
TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(
  Kokkos_CrsMatrix_MP, Multiply_Default, Ordinal, Scalar, MultiplyOp )
{
  const Ordinal NumPerThread = 1;
  const Ordinal ThreadsPerVector = 16;

  const Ordinal num_blocks = 0;
  const Ordinal num_vec_threads = 0;
  const Ordinal num_row_threads = 0;

  success =
    test_cuda_static_fixed_embedded_vector<Ordinal,Scalar,MultiplyOp,NumPerThread,ThreadsPerVector>(num_blocks, num_vec_threads, num_row_threads, out);
}

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(
  Kokkos_CrsMatrix_MP, Multiply_1, Ordinal, Scalar, MultiplyOp )
{
  const Ordinal NumPerThread = 1;
  const Ordinal ThreadsPerVector = 16;

  const Ordinal num_blocks = 10;
  const Ordinal num_vec_threads = ThreadsPerVector;
  const Ordinal num_row_threads = 4;

  success =
    test_cuda_static_fixed_embedded_vector<Ordinal,Scalar,MultiplyOp,NumPerThread,ThreadsPerVector>(num_blocks, num_vec_threads, num_row_threads, out);
}

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(
  Kokkos_CrsMatrix_MP, Multiply_2, Ordinal, Scalar, MultiplyOp )
{
  const Ordinal NumPerThread = 2;
  const Ordinal ThreadsPerVector = 16;

  const Ordinal num_blocks = 10;
  const Ordinal num_vec_threads = ThreadsPerVector;
  const Ordinal num_row_threads = 4;

  success =
    test_cuda_static_fixed_embedded_vector<Ordinal,Scalar,MultiplyOp,NumPerThread,ThreadsPerVector>(num_blocks, num_vec_threads, num_row_threads, out);
}

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(
  Kokkos_CrsMatrix_MP, Multiply_3, Ordinal, Scalar, MultiplyOp )
{
  const Ordinal NumPerThread = 3;
  const Ordinal ThreadsPerVector = 16;

  const Ordinal num_blocks = 10;
  const Ordinal num_vec_threads = ThreadsPerVector;
  const Ordinal num_row_threads = 4;

  success =
    test_cuda_static_fixed_embedded_vector<Ordinal,Scalar,MultiplyOp,NumPerThread,ThreadsPerVector>(num_blocks, num_vec_threads, num_row_threads, out);
}

TEUCHOS_UNIT_TEST_TEMPLATE_3_DECL(
  Kokkos_CrsMatrix_MP, Multiply_4, Ordinal, Scalar, MultiplyOp )
{
  const Ordinal NumPerThread = 4;
  const Ordinal ThreadsPerVector = 16;

  const Ordinal num_blocks = 10;
  const Ordinal num_vec_threads = ThreadsPerVector;
  const Ordinal num_row_threads = 4;

  success =
    test_cuda_static_fixed_embedded_vector<Ordinal,Scalar,MultiplyOp,NumPerThread,ThreadsPerVector>(num_blocks, num_vec_threads, num_row_threads, out);
}

#define CRS_MATRIX_MP_VECTOR_MULTIPLY_TESTS_ORDINAL_SCALAR_OP( ORDINAL, SCALAR, OP ) \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT(                                 \
    Kokkos_CrsMatrix_MP, Multiply_Default,  ORDINAL, SCALAR, OP )       \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT(                                 \
    Kokkos_CrsMatrix_MP, Multiply_1,  ORDINAL, SCALAR, OP )             \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT(                                 \
    Kokkos_CrsMatrix_MP, Multiply_2,  ORDINAL, SCALAR, OP )             \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT(                                 \
    Kokkos_CrsMatrix_MP, Multiply_3,  ORDINAL, SCALAR, OP )             \
  TEUCHOS_UNIT_TEST_TEMPLATE_3_INSTANT(                                 \
    Kokkos_CrsMatrix_MP, Multiply_4,  ORDINAL, SCALAR, OP )

CRS_MATRIX_MP_VECTOR_MULTIPLY_TESTS_ORDINAL_SCALAR_OP(int, double, DefaultMultiply)
CRS_MATRIX_MP_VECTOR_MULTIPLY_TESTS_ORDINAL_SCALAR_OP(int, double, EnsembleMultiply)
CRS_MATRIX_MP_VECTOR_MULTIPLY_TESTS_ORDINAL_SCALAR_OP(int, double, KokkosMultiply)

int main( int argc, char* argv[] ) {
  Teuchos::GlobalMPISession mpiSession(&argc, &argv);

  // Initialize Cuda
  Kokkos::Cuda::host_mirror_device_type::initialize();
  Kokkos::Cuda::initialize(Kokkos::Cuda::SelectDevice(0));
  Kokkos::Cuda::print_configuration(std::cout);

  // Run tests
  int ret = Teuchos::UnitTestRepository::runUnitTestsFromMain(argc, argv);

  // Finish up
  Kokkos::Cuda::host_mirror_device_type::finalize();
  Kokkos::Cuda::finalize();

  return ret;
}
