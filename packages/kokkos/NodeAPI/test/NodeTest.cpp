#include <Teuchos_UnitTestHarness.hpp>
#include <Teuchos_TimeMonitor.hpp>
#include <Teuchos_Time.hpp>

#include "Kokkos_ConfigDefs.hpp"
#include "TestOps.hpp"

#include "Kokkos_SerialNode.hpp"
#ifdef HAVE_KOKKOS_TBB
#include "Kokkos_TBBNode.hpp"
#endif
#ifdef HAVE_KOKKOS_THREADPOOL
#include "Kokkos_TPINode.hpp"
#endif
#ifdef HAVE_KOKKOS_CUDA
#include "Kokkos_CUDANode.hpp"
#endif

namespace {

  using Teuchos::Time;
  using Teuchos::TimeMonitor;
  using Kokkos::SerialNode;

  SerialNode serialnode;

  int N = 100;
  int NumIters = 1000;
  template <class NODE>
  NODE & getNode() {
    TEST_FOR_EXCEPTION(true,std::logic_error,"Node type not defined.");
  }

  template <>
  SerialNode & getNode<SerialNode>() {
    return serialnode;
  }

#ifdef HAVE_KOKKOS_TBB
  using Kokkos::TBBNode;
  int tbb_nT = 0;
  TBBNode tbbnode;
  template <>
  TBBNode & getNode<TBBNode>() {
    return tbbnode;
  }
#endif

#ifdef HAVE_KOKKOS_THREADPOOL
  using Kokkos::TPINode;
  int tpi_nT = 1;
  TPINode tpinode;
  template <>
  TPINode & getNode<TPINode>() {
    return tpinode;
  }
#endif

#ifdef HAVE_KOKKOS_CUDA
  using Kokkos::CUDANode;
  int cuda_dev = 0;
  int cuda_nT  = 64;
  int cuda_nB  = 64;
  int cuda_verb = 0;
  CUDANode cudanode;
  template <>
  CUDANode & getNode<CUDANode>() {
    return cudanode;
  }
#endif

  TEUCHOS_STATIC_SETUP()
  {
    Teuchos::CommandLineProcessor &clp = Teuchos::UnitTestRepository::getCLP();
    clp.addOutputSetupOptions(true);
    clp.setOption("test-size",&N,"Vector length for tests.");
    clp.setOption("num-iters",&NumIters,"Number of iterations in TimeTest.");
    if (N < 2) N = 2;
#ifdef HAVE_KOKKOS_TBB
    {
      clp.setOption("tbb-num-threads",&tbb_nT,"Number of TBB threads: 0 for automatic.");
    }
#endif
#ifdef HAVE_KOKKOS_THREADPOOL
    {
      clp.setOption("tpi-num-threads",&tpi_nT,"Number of TPI threads.");
    }
#endif
#ifdef HAVE_KOKKOS_CUDA
    {
      clp.setOption("cuda-device",&cuda_dev,"CUDA device used for testing.");
      clp.setOption("cuda-num-threads",&cuda_nT,"Number of CUDA threads.");
      clp.setOption("cuda-num-blocks",&cuda_nB,"Number of CUDA blocks.");
      clp.setOption("cuda-verbose",&cuda_verb,"CUDA verbosity level.");
    }
#endif
  }

  //
  // UNIT TESTS
  // 

  TEUCHOS_UNIT_TEST( AAAAA, Init_Nodes )
  {
#ifdef HAVE_KOKKOS_TBB
    out << "Initializing TBB node to " << tbb_nT << " threads." << std::endl;
    tbbnode.init(tbb_nT);
#endif
#ifdef HAVE_KOKKOS_THREADPOOL
    out << "Initializing TPI node to " << tpi_nT << " threads." << std::endl;
    tpinode.init(tpi_nT);
#endif
#ifdef HAVE_KOKKOS_CUDA
    out << "Initializing CUDA device " << cuda_dev 
        << " with " << cuda_nB << " blocks and " << cuda_nT << " threads." << std::endl;
    cudanode.init(cuda_dev,cuda_nB,cuda_nT,cuda_verb);
#endif
    TEST_EQUALITY(0,0);
  }

  ////
  TEUCHOS_UNIT_TEST_TEMPLATE_2_DECL( NodeAPI, SumTest, SCALAR, NODE )
  {
    Time tAlloc("Alloc Time"), tInit("Init Op"), tSum("Sum Op"), tFree("Free Time");
    typename NODE::template buffer<SCALAR>::buffer_t x;
    NODE &node = getNode<NODE>();
    SCALAR result;
    {
      TimeMonitor localTimer(tAlloc);
      x = node.template allocBuffer<SCALAR>(N);
    }
    // set x[i] = 1, i=0:N-1
    {
      TimeMonitor localTimer(tInit);
      InitOp<SCALAR,NODE> wdp;
      wdp.x = x;
      node.parallel_for(0,N,wdp);
    }
    // compute sum x[i], i=0:N-1
    {
      TimeMonitor localTimer(tSum);
      SumOp<SCALAR,NODE> wdp;
      wdp.x = x;
      result = node.parallel_reduce(0,N,wdp);
    }
    SCALAR expectedResult = (SCALAR)(N);
    TEST_EQUALITY(result, expectedResult);
    // compute sum x[i], i=1:N-2
    {
      TimeMonitor localTimer(tSum);
      SumOp<SCALAR,NODE> wdp;
      wdp.x = x;
      result = node.parallel_reduce(1,N-1,wdp);
    }
    expectedResult = (SCALAR)(N-2);
    TEST_EQUALITY(result, expectedResult);
    {
      TimeMonitor localTimer(tFree);
      node.template freeBuffer<SCALAR>(x);
    }
    out << "allocBuffer Time: " << tAlloc.totalElapsedTime() << std::endl;
    out << "InitOp Time: " << tInit.totalElapsedTime() << std::endl;
    out << "SumOp Time: " << tSum.totalElapsedTime() << std::endl;
    out << "freeBuffer Time: " << tFree.totalElapsedTime() << std::endl;
  }

  TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( NodeAPI, TimeTest, NODE )
  {
    Time tNoop("Null Op");
    NODE &node = getNode<NODE>();
    NullOp<NODE> noop;
    int red;
    {
      TimeMonitor localTimer(tNoop);
      for (int i=0; i<NumIters; ++i) {
        red = node.parallel_reduce(0,1,noop);
      }
    }
    TEST_EQUALITY_CONST(red,0);
    out << "NullOp Time: " << tNoop.totalElapsedTime() << std::endl;
    out << "    average: " << (int)(tNoop.totalElapsedTime() / (double)(NumIters) * 1000000000.0) << " ns" << std::endl;
  }

  // 
  // INSTANTIATIONS
  //

#define SERIAL_INSTANT(SCALAR) \
    TEUCHOS_UNIT_TEST_TEMPLATE_2_INSTANT( NodeAPI, SumTest, SCALAR, SerialNode )

#ifdef HAVE_KOKKOS_TBB
#define TBB_INSTANT(SCALAR) \
    TEUCHOS_UNIT_TEST_TEMPLATE_2_INSTANT( NodeAPI, SumTest, SCALAR, TBBNode )
#else
#define TBB_INSTANT(SCALAR) 
#endif

#ifdef HAVE_KOKKOS_THREADPOOL
#define TPI_INSTANT(SCALAR) \
    TEUCHOS_UNIT_TEST_TEMPLATE_2_INSTANT( NodeAPI, SumTest, SCALAR, TPINode )
#else
#define TPI_INSTANT(SCALAR) 
#endif

#ifdef HAVE_KOKKOS_CUDA
#define CUDA_INSTANT(SCALAR) \
    TEUCHOS_UNIT_TEST_TEMPLATE_2_INSTANT( NodeAPI, SumTest, SCALAR, CUDANode )
#else
#define CUDA_INSTANT(SCALAR) 
#endif

#define UNIT_TEST_GROUP_SCALAR(SCALAR) \
  SERIAL_INSTANT(SCALAR) \
  TBB_INSTANT(SCALAR) \
  TPI_INSTANT(SCALAR) \
  CUDA_INSTANT(SCALAR)

  UNIT_TEST_GROUP_SCALAR(int)
  UNIT_TEST_GROUP_SCALAR(float)
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT( NodeAPI, TimeTest, SerialNode )
#ifdef HAVE_KOKKOS_TBB
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT( NodeAPI, TimeTest, TBBNode )
#endif
#ifdef HAVE_KOKKOS_THREADPOOL
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT( NodeAPI, TimeTest, TPINode )
#endif
#ifdef HAVE_KOKKOS_CUDA
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT( NodeAPI, TimeTest, CUDANode )
#endif

}
