#include "Tpetra_BlockCrsGraph.hpp"

#ifdef HAVE_TPETRA_EXPLICIT_INSTANTIATION

// #include "Tpetra_ExplicitInstantiationHelpers.hpp"

#include "Tpetra_BlockCrsGraph_def.hpp"

#include <Kokkos_SerialNode.hpp>
#if defined(HAVE_KOKKOS_TBB)
#  include <Kokkos_TBBNode.hpp>
#endif
#if defined(HAVE_KOKKOS_THREADPOOL)
#  include <Kokkos_TPINode.hpp>
#endif
#if defined(HAVE_KOKKOS_THRUST)
#  include <Kokkos_ThrustGPUNode.hpp>
#endif
#if defined(HAVE_KOKKOS_OPENMP)
#  include <Kokkos_OpenMPNode.hpp>
#endif

namespace Tpetra {

  TPETRA_BLOCKCRSGRAPH_INSTANT(int,int,Kokkos::SerialNode)
#if defined(HAVE_KOKKOS_TBB)
  TPETRA_BLOCKCRSGRAPH_INSTANT(int,int,Kokkos::TBBNode)
#endif
#if defined(HAVE_KOKKOS_THREADPOOL)
    TPETRA_BLOCKCRSGRAPH_INSTANT(int,int,Kokkos::TPINode)
#endif
#if defined(HAVE_KOKKOS_THRUST)
    TPETRA_BLOCKCRSGRAPH_INSTANT(int,int,Kokkos::ThrustGPUNode)
#endif
#if defined(HAVE_KOKKOS_OPENMP)
    TPETRA_BLOCKCRSGRAPH_INSTANT(int,int,Kokkos::OpenMPNode)
#endif

} // namespace Tpetra

#endif // HAVE_TPETRA_EXPLICIT_INSTANTIATION
