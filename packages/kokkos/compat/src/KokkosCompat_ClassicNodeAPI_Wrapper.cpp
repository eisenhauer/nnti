#include <KokkosCompat_ClassicNodeAPI_Wrapper.hpp>

namespace Kokkos {
  namespace Compat {

#ifdef KOKKOS_HAVE_PTHREAD
    template<>
    KokkosDeviceWrapperNode<Kokkos::Threads>::~KokkosDeviceWrapperNode<Kokkos::Threads>() {
      count--;
      if(count==0)
        Threads::finalize();
    }
    template<>
    void KokkosDeviceWrapperNode<Kokkos::Threads>::init(int NumTeams, int NumThreads, int Device) {
      Kokkos::Threads::initialize(NumTeams*NumThreads);
    }
#endif

#ifdef KOKKOS_HAVE_OPENMP
    template<>
    KokkosDeviceWrapperNode<Kokkos::OpenMP>::~KokkosDeviceWrapperNode<Kokkos::OpenMP>() {
      count--;
      if(count==0)
        OpenMP::finalize();
    }
    template<>
    void KokkosDeviceWrapperNode<Kokkos::OpenMP>::init(int NumTeams, int NumThreads, int Device) {
      Kokkos::OpenMP::initialize(NumTeams*NumThreads);
    }
#endif

#ifdef KOKKOS_HAVE_CUDA
    template<>
    KokkosDeviceWrapperNode<Kokkos::Cuda>::~KokkosDeviceWrapperNode<Kokkos::Cuda>() {
      count--;
      if(count==0) {
        Cuda::host_mirror_device_type::finalize();
        Cuda::finalize();
      }
    }
    template<>
    void KokkosDeviceWrapperNode<Kokkos::Cuda>::init(int NumTeams, int NumThreads, int Device) {
      Kokkos::Cuda::host_mirror_device_type::initialize(NumTeams*NumThreads);
      Kokkos::Cuda::SelectDevice select_device(Device);
      Kokkos::Cuda::initialize(select_device);
    }
#endif

#ifdef KOKKOS_HAVE_CUDA
  int KokkosCudaWrapperNode::count = 0;
#endif

#ifdef KOKKOS_HAVE_OPENMP
  int KokkosOpenMPWrapperNode::count = 0;
#endif

#ifdef KOKKOS_HAVE_PTHREAD
  int KokkosThreadsWrapperNode::count = 0;
#endif

  }
}
