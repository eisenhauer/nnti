#include "MueLu_ExplicitInstantiation.hpp"

#include "MueLu_PermutedTransferFactory_def.hpp"

#ifdef HAVE_MUELU_INST_DOUBLE_INT_INT
template class MueLu::PermutedTransferFactory<double, int, int, Kokkos::DefaultNode::DefaultNodeType, Kokkos::DefaultKernels<void, int, Kokkos::DefaultNode::DefaultNodeType>::SparseOps>;
#endif

#ifdef HAVE_MUELU_INST_DOUBLE_INT_LONGLONGINT
# ifdef HAVE_TEUCHOS_LONG_LONG_INT
template class MueLu::PermutedTransferFactory<double, int, long long int, Kokkos::DefaultNode::DefaultNodeType, Kokkos::DefaultKernels<void, int, Kokkos::DefaultNode::DefaultNodeType>::SparseOps>;
# else
# warning To compile MueLu with 'long long int' support, please turn on Teuchos_ENABLE_LONG_LONG_INT
# endif
#endif

#ifdef HAVE_MUELU_INST_COMPLEX_INT_INT
# ifdef HAVE_TEUCHOS_COMPLEX
#include <complex>
template class MueLu::PermutedTransferFactory<std::complex<double>, int, int, Kokkos::DefaultNode::DefaultNodeType, Kokkos::DefaultKernels<void, int, Kokkos::DefaultNode::DefaultNodeType>::SparseOps>;
# else
# warning To compile MueLu with 'complex' support, please turn on Teuchos_ENABLE_COMPLEX
# endif
#endif

#include "Tpetra_Vector_def.hpp"
#include "Tpetra_MultiVector_def.hpp"
#ifdef HAVE_MUELU_INST_DOUBLE_INT_INT
namespace Tpetra {
TPETRA_VECTOR_INSTANT(size_t,int,int,Kokkos::DefaultNode::DefaultNodeType)
TPETRA_MULTIVECTOR_INSTANT(size_t,int,int,Kokkos::DefaultNode::DefaultNodeType)
}
#endif
#ifdef HAVE_MUELU_INST_DOUBLE_INT_LONGLONGINT
# ifdef HAVE_TEUCHOS_LONG_LONG_INT
TPETRA_VECTOR_INSTANT(size_t,int,long long int,Kokkos::DefaultNode::DefaultNodeType);
TPETRA_MULTIVECTOR_INSTANT(size_t,int,long long int,Kokkos::DefaultNode::DefaultNodeType)
#endif
#endif
