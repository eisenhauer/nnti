// @HEADER
// ***********************************************************************
//
//          Tpetra: Templated Linear Algebra Services Package
//                 Copyright (2008) Sandia Corporation
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
// @HEADER

#ifndef TPETRA_KOKKOSREFACTOR_CRSMATRIX_DEF_HPP
#define TPETRA_KOKKOSREFACTOR_CRSMATRIX_DEF_HPP

#ifdef DOXYGEN_USE_ONLY
#  include "Tpetra_KokkosRefactor_CrsMatrix_decl.hpp"
#endif

namespace Tpetra {

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal,
            class DeviceType>
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
            typename KokkosClassic::DefaultKernels<Scalar,
                                                   LocalOrdinal,
                                                   Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  CrsMatrix (const RCP<const map_type> &rowMap,
             size_t maxNumEntriesPerRow,
             ProfileType pftype,
             const RCP<Teuchos::ParameterList>& params) :
    DistObject<char, LocalOrdinal, GlobalOrdinal, node_type> (rowMap),
    storageStatus_ (pftype == StaticProfile ?
                    CRS_MATRIX_STORAGE_1D_UNPACKED :
                    CRS_MATRIX_STORAGE_2D),
    fillComplete_ (false),
    frobNorm_ (-STM::one ())
  {
    try {
      myGraph_ = rcp (new crs_graph_type (rowMap, maxNumEntriesPerRow, pftype, params));
    }
    catch (std::exception& e) {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
        "CrsMatrix constructor: caught exception while allocating CrsGraph "
        "object: " << std::endl << e.what ());
    }
    staticGraph_ = myGraph_;
    resumeFill (params);
    checkInternalState ();
  }

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  CrsMatrix (const Teuchos::RCP<const map_type>& rowMap,
             const Teuchos::ArrayRCP<const size_t>& NumEntriesPerRowToAlloc,
             ProfileType pftype,
             const Teuchos::RCP<Teuchos::ParameterList>& params) :
    DistObject<char, LocalOrdinal, GlobalOrdinal, node_type> (rowMap),
    storageStatus_ (pftype == StaticProfile ?
                    CRS_MATRIX_STORAGE_1D_UNPACKED :
                    CRS_MATRIX_STORAGE_2D),
    fillComplete_ (false),
    frobNorm_ (-STM::one ())
  {
    try {
      myGraph_ = rcp (new Graph (rowMap, NumEntriesPerRowToAlloc, pftype, params));
    }
    catch (std::exception &e) {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
          typeName(*this) << "::CrsMatrix(): caught exception while allocating CrsGraph object: "
          << std::endl << e.what() << std::endl);
    }
    staticGraph_ = myGraph_;
    resumeFill(params);
    checkInternalState();
  }

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  CrsMatrix (const Teuchos::RCP<const map_type>& rowMap,
             const Teuchos::RCP<const map_type>& colMap,
             size_t maxNumEntriesPerRow,
             ProfileType pftype,
             const Teuchos::RCP<Teuchos::ParameterList>& params) :
    DistObject<char, LocalOrdinal, GlobalOrdinal, node_type> (rowMap),
    storageStatus_ (pftype == StaticProfile ?
                    CRS_MATRIX_STORAGE_1D_UNPACKED :
                    CRS_MATRIX_STORAGE_2D),
    fillComplete_ (false),
    frobNorm_ (-STM::one ())
  {
    TEUCHOS_TEST_FOR_EXCEPTION(! staticGraph_.is_null(), std::logic_error,
      "Tpetra::CrsMatrix ctor (row Map, col Map, maxNumEntriesPerRow, ...): "
      "staticGraph_ is not null at the beginning of the constructor.  "
      "Please report this bug to the Tpetra developers.");
    TEUCHOS_TEST_FOR_EXCEPTION(! myGraph_.is_null(), std::logic_error,
      "Tpetra::CrsMatrix ctor (row Map, col Map, maxNumEntriesPerRow, ...): "
      "myGraph_ is not null at the beginning of the constructor.  "
      "Please report this bug to the Tpetra developers.");
    try {
      myGraph_ = rcp (new Graph (rowMap, colMap, maxNumEntriesPerRow, pftype, params));
    }
    catch (std::exception &e) {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
        "CrsMatrix constructor: Caught exception while allocating "
        "CrsGraph object: " << std::endl << e.what ());
    }
    staticGraph_ = myGraph_;
    resumeFill(params);
    checkInternalState();
  }

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  CrsMatrix (const Teuchos::RCP<const map_type>& rowMap,
             const Teuchos::RCP<const map_type>& colMap,
             const Teuchos::ArrayRCP<const size_t>& numEntPerRow,
             ProfileType pftype,
             const RCP<Teuchos::ParameterList>& params) :
    DistObject<char, LocalOrdinal, GlobalOrdinal, node_type> (rowMap),
    storageStatus_ (pftype == StaticProfile ?
                    CRS_MATRIX_STORAGE_1D_UNPACKED :
                    CRS_MATRIX_STORAGE_2D),
    fillComplete_ (false),
    frobNorm_ (-STM::one ())
  {
    try {
      myGraph_ = rcp (new Graph (rowMap, colMap, numEntPerRow, pftype, params));
    }
    catch (std::exception &e) {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
        "CrsMatrix constructor: caught exception while allocating "
        "CrsGraph object: " << std::endl << e.what ());
    }
    staticGraph_ = myGraph_;
    resumeFill (params);
    checkInternalState ();
  }


  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal, class DeviceType>
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  CrsMatrix (const Teuchos::RCP<const CrsGraph<LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>, typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps> >& graph,
             const Teuchos::RCP<Teuchos::ParameterList>& params) :
    DistObject<char, LocalOrdinal,GlobalOrdinal, node_type> (graph->getRowMap ()),
    staticGraph_ (graph),
    storageStatus_ (CRS_MATRIX_STORAGE_1D_PACKED),
    fillComplete_ (false),
    frobNorm_ (-STM::one ())
  {
    const char tfecfFuncName[] = "CrsMatrix(graph)";
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(staticGraph_.is_null (),
      std::runtime_error, ": When calling the CrsMatrix constructor that "
      "accepts a static graph, the pointer to the graph must not be null.");
    // We prohibit the case where the graph is not yet filled.
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC( ! staticGraph_->isFillComplete (),
      std::runtime_error, ": The specified graph is not fill-complete. You "
      "must invoke fillComplete() on the graph before using it to construct a "
      "CrsMatrix.  Note that calling resumeFill() makes the graph not fill-"
      "complete, even if you had previously called fillComplete().  In that "
      "case, you must call fillComplete() on the graph again.");
    // the graph has entries, and the matrix should have entries as well, set to zero. no need or point in lazy allocating in this case.
    // first argument LocalIndices is ignored; the graph is already allocated (local or global, we don't care here)
    allocateValues (LocalIndices, GraphAlreadyAllocated);
    resumeFill(params);
    checkInternalState();
  }

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal,
            class DeviceType>
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  CrsMatrix (const RCP<const map_type>& rowMap,
             const RCP<const map_type>& colMap,
             const t_RowPtrs & rowPointers,
             const t_LocalOrdinal_1D & columnIndices,
             const t_ValuesType & values,
             const RCP<Teuchos::ParameterList>& params) :
    DistObject<char, LocalOrdinal, GlobalOrdinal, node_type> (rowMap),
    storageStatus_ (CRS_MATRIX_STORAGE_1D_PACKED),
    fillComplete_ (false),
    frobNorm_ (-STM::one ())
  {
    try {
      myGraph_ = rcp (new Graph (rowMap, colMap, rowPointers, columnIndices, params));
    }
    catch (std::exception &e) {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
        "CrsMatrix constructor: caught exception while allocating "
        "CrsGraph object: " << std::endl << e.what ());
    }
    staticGraph_ = myGraph_;
    k_values1D_  = values;
    values1D_    = Kokkos::Compat::persistingView (k_values1D_);
    resumeFill (params);
    checkInternalState ();
  }

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  CrsMatrix (const RCP<const map_type>& rowMap,
             const RCP<const map_type>& colMap,
             const ArrayRCP<size_t> & rowPointers,
             const ArrayRCP<LocalOrdinal> & columnIndices,
             const ArrayRCP<Scalar> & values,
             const RCP<Teuchos::ParameterList>& params) :
    DistObject<char, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > (rowMap),
    storageStatus_ (CRS_MATRIX_STORAGE_1D_PACKED),
    fillComplete_ (false),
    frobNorm_ (-STM::one ())
  {
    try {
      myGraph_ = rcp (new Graph (rowMap, colMap, rowPointers,columnIndices,params));
    }
    catch (std::exception &e) {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
        typeName(*this) << "::CrsMatrix(): caught exception while allocating "
        "CrsGraph object: " << std::endl << e.what ());
    }
    staticGraph_ = myGraph_;
    // FIXME (mfh 05 Aug 2014) It should be possible to convince the
    // ArrayRCP to relinquish its allocation, but that might require
    // passing the ArrayRCP in by nonconst reference.
    k_values1D_ = Kokkos::Compat::getKokkosViewDeepCopy<DeviceType> (values ());
    values1D_ = Kokkos::Compat::persistingView (k_values1D_);
    resumeFill (params);
    checkInternalState ();
  }

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  CrsMatrix (const RCP<const map_type>& rowMap,
             const RCP<const map_type>& colMap,
             const k_local_matrix_type& lclMatrix,
             const RCP<Teuchos::ParameterList>& params) :
    DistObject<char, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > (rowMap),
    k_lclMatrix_ (lclMatrix),
    storageStatus_ (CRS_MATRIX_STORAGE_1D_PACKED),
    fillComplete_ (false),
    frobNorm_ (-STM::one ())
  {
    using Teuchos::ArrayRCP;
    using Teuchos::arcp;
    using Teuchos::rcp;
    using Teuchos::RCP;
    const char tfecfFuncName[] = "CrsMatrix(rowMap,colMap,lclMatrix,params)";

    try {
      myGraph_ = rcp (new Graph (rowMap, colMap, lclMatrix.graph, params));
    }
    catch (std::exception &e) {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error,
        typeName(*this) << "::CrsMatrix(): caught exception while allocating "
        "CrsGraph object: " << std::endl << e.what ());
    }
    staticGraph_ = myGraph_;
    computeGlobalConstants();

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      staticGraph_->getLocalGraph ().is_null (), std::logic_error,
      ": The local graph ({my,static}Graph_->getLocalGraph()) is null.  "
      "Please report this bug to the Tpetra developers.");

    k_values1D_ = k_lclMatrix_.values;

    //
    // Set up the local sparse kernels.
    //
    lclMatOps_ = rcp (new sparse_ops_type (getNode ()));
    {
      // For backwards compatibility, set the Kokkos classic pointer
      // to the values, values1D_.
      ArrayRCP<scalar_type> classicValues =
        Kokkos::Compat::persistingView (k_lclMatrix_.values);
      values1D_ = classicValues;

      // For backwards compatibility, create a temporary Kokkos
      // classic "local matrix" and pass it in to the "local mat ops"
      // object.
      RCP<local_matrix_type> classicLocalMatrix =
        rcp (new local_matrix_type (staticGraph_->getLocalGraph (), params));
      classicLocalMatrix->setValues (classicValues);
      lclMatOps_->setGraphAndMatrix (staticGraph_->getLocalGraph (),
                                     classicLocalMatrix);
    }

    // Once we've initialized the sparse kernels, we're done with the
    // local objects.  We may now release them and their memory, since
    // they will persist in the local sparse ops if necessary.  We
    // keep the local graph if the parameters tell us to do so.
    if (myGraph_ != null) {
      bool preserveLocalGraph = true;
      if (params != null) {
        preserveLocalGraph =
          params->get ("Preserve Local Graph", preserveLocalGraph);
      }
      if (! preserveLocalGraph) {
        myGraph_->lclGraph_ = null;
      }
    }
    // Now we're fill complete!
    fillComplete_ = true;

    // Sanity checks at the end.
#ifdef HAVE_TPETRA_DEBUG
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(isFillActive(), std::logic_error,
      ": We're at the end of fillComplete(), but isFillActive() is true.  "
      "Please report this bug to the Tpetra developers.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(! isFillComplete(), std::logic_error,
      ": We're at the end of fillComplete(), but isFillActive() is true.  "
      "Please report this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG
    checkInternalState ();
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  ~CrsMatrix () {}

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  RCP<const Teuchos::Comm<int> >
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getComm () const {
    return getCrsGraph ()->getComm ();
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  Teuchos::RCP<Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getNode () const {
    return getCrsGraph ()->getNode ();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  ProfileType
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getProfileType () const {
    return getCrsGraph ()->getProfileType ();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  isFillComplete () const {
    return fillComplete_;
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  isFillActive () const {
    return ! fillComplete_;
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::isStorageOptimized() const {
    return getCrsGraph()->isStorageOptimized();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::isLocallyIndexed() const {
    return getCrsGraph()->isLocallyIndexed();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::isGloballyIndexed() const {
    return getCrsGraph()->isGloballyIndexed();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::hasColMap() const {
    return getCrsGraph()->hasColMap();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  global_size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getGlobalNumEntries() const {
    return getCrsGraph()->getGlobalNumEntries();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getNodeNumEntries() const {
    return getCrsGraph()->getNodeNumEntries();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  global_size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getGlobalNumRows() const {
    return getCrsGraph()->getGlobalNumRows();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  global_size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getGlobalNumCols() const {
    return getCrsGraph()->getGlobalNumCols();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getNodeNumRows() const {
    return getCrsGraph()->getNodeNumRows();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getNodeNumCols() const {
    return getCrsGraph()->getNodeNumCols();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  global_size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getGlobalNumDiags() const {
    return getCrsGraph()->getGlobalNumDiags();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getNodeNumDiags() const {
    return getCrsGraph()->getNodeNumDiags();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getNumEntriesInGlobalRow(GlobalOrdinal globalRow) const {
    return getCrsGraph()->getNumEntriesInGlobalRow(globalRow);
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getNumEntriesInLocalRow(LocalOrdinal localRow) const {
    return getCrsGraph()->getNumEntriesInLocalRow(localRow);
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getGlobalMaxNumRowEntries() const {
    return getCrsGraph()->getGlobalMaxNumRowEntries();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  size_t CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getNodeMaxNumRowEntries() const {
    return getCrsGraph()->getNodeMaxNumRowEntries();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  GlobalOrdinal CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getIndexBase() const {
    return getRowMap()->getIndexBase();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  RCP<const Map<LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getRowMap() const {
    return getCrsGraph()->getRowMap();
  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  RCP<const Map<LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getColMap() const {
    return getCrsGraph()->getColMap();
  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  RCP<const Map<LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getDomainMap() const {
    return getCrsGraph()->getDomainMap();
  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  RCP<const Map<LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getRangeMap() const {
    return getCrsGraph()->getRangeMap();
  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  RCP<const RowGraph<LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getGraph() const {
    if (staticGraph_ != null) return staticGraph_;
    return myGraph_;
  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  RCP<const CrsGraph<LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps> >
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getCrsGraph() const {
    if (staticGraph_ != null) return staticGraph_;
    return myGraph_;
  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::isLowerTriangular() const {
    return getCrsGraph()->isLowerTriangular();
  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::isUpperTriangular() const {
    return getCrsGraph()->isUpperTriangular();
  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::isStaticGraph() const {
    return (myGraph_ == null);
  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::hasTransposeApply() const {
    return true;
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  bool
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  supportsRowViews () const {
    return true;
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  //                                                                         //
  //                    Internal utility methods                             //
  //                                                                         //
  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal,
            class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  allocateValues (ELocalGlobal lg, GraphAllocationStatus gas)
  {
#ifdef HAVE_TPETRA_DEBUG
    // If the graph indices are already allocated, then gas should be
    // GraphAlreadyAllocated.  Otherwise, gas should be
    // GraphNotYetAllocated.
    if ((gas == GraphAlreadyAllocated) != staticGraph_->indicesAreAllocated()) {
      const std::string err1 ("allocateValues: The caller has asserted that "
                              "the graph is ");
      const std::string err2 ("already allocated, but the static graph says "
                              "that its indices are ");
      const std::string err3 ("already allocated.  Please report this bug to "
                              "the Tpetra developers.");
      TEUCHOS_TEST_FOR_EXCEPTION(gas == GraphAlreadyAllocated && ! staticGraph_->indicesAreAllocated(),
        std::logic_error, err1 << err2 << "not " << err3);
      TEUCHOS_TEST_FOR_EXCEPTION(gas != GraphAlreadyAllocated && staticGraph_->indicesAreAllocated(),
        std::logic_error, err1 << "not " << err2 << err3);
    }

    // If the graph is unallocated, then it had better be a
    // matrix-owned graph.  ("Matrix-owned graph" means that the
    // matrix gets to define the graph structure.  If the CrsMatrix
    // constructor that takes an RCP<const CrsGraph> was used, then
    // the matrix does _not_ own the graph.)
    TEUCHOS_TEST_FOR_EXCEPTION(
      ! staticGraph_->indicesAreAllocated() && myGraph_.is_null(),
      std::logic_error,
      "allocateValues: The static graph says that its indices are not "
      "allocated, but the graph is not owned by the matrix.  Please report "
      "this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG

    if (gas == GraphNotYetAllocated) {
      myGraph_->allocateIndices (lg);
    }

    // Allocate matrix values.
    if (getProfileType() == StaticProfile) {
      // "Static profile" means that the number of matrix entries in
      // each row was fixed at the time the CrsMatrix constructor was
      // called.  This lets us use 1-D storage for the matrix's
      // values.  ("1-D storage" means the same as that used by the
      // three arrays in the classic compressed sparse row format.)

      const size_t lclNumRows = staticGraph_->getNodeNumRows ();
      typename Graph::t_RowPtrs k_ptrs = staticGraph_->k_rowPtrs_;
      TEUCHOS_TEST_FOR_EXCEPTION(
        k_ptrs.dimension_0 () != lclNumRows+1, std::logic_error,
        "Tpetra::CrsMatrix::allocateValues: With StaticProfile, row offsets "
        "array has length " << k_ptrs.dimension_0 () << " != (lclNumRows+1) = "
        << (lclNumRows+1) << ".");
      // FIXME (mfh 08 Aug 2014) This assumes UVM.  We could fix this
      // either by storing the row offsets in the graph as a DualView,
      // or by making a device View of that entry, and copying it back
      // to host.
      const size_t lclTotalNumEntries = k_ptrs(lclNumRows);

      // Allocate array of (packed???) matrix values.
      k_values1D_ = t_ValuesType ("Tpetra::CrsMatrix::val", lclTotalNumEntries);
      values1D_ = Kokkos::Compat::persistingView (k_values1D_);
    }
    else {
      // "Dynamic profile" means the number of matrix entries in each
      // row is not fixed and may expand.  Thus, we store the matrix's
      // values in "2-D storage," meaning an array of arrays.  The
      // outer array has as many inner arrays as there are rows in the
      // matrix, and each inner array stores the values in that row.
      values2D_ = staticGraph_->template allocateValues2D<Scalar>();
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal,
            class DeviceType>
  void
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getAllValues (ArrayRCP<const size_t>& rowPointers,
                ArrayRCP<const LocalOrdinal>& columnIndices,
                ArrayRCP<const Scalar>& values) const
  {
    const char tfecfFuncName[] = "getAllValues";
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      columnIndices.size () != values.size (), std::runtime_error,
      " requires that columnIndices and values are the same size.");

    RCP<const crs_graph_type> relevantGraph = getCrsGraph ();
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      relevantGraph.is_null (), std::runtime_error,
      " requires that getCrsGraph() is not null.");
    try {
      rowPointers = relevantGraph->getNodeRowPtrs ();
      columnIndices = relevantGraph->getNodePackedIndices ();
    }
    catch (std::exception &e) {
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
        true, std::runtime_error,
        ": Caught exception while calling getCrsGraph()->getAllIndices().");
    }
    values = Kokkos::Compat::persistingView (k_values1D_);
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal,
            class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  fillLocalGraphAndMatrix (const Teuchos::RCP<Teuchos::ParameterList>& params)
  {
    using Kokkos::create_mirror_view;
    using Teuchos::arcp_const_cast;
    using Teuchos::ArrayRCP;
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    typedef ArrayRCP<size_t>::size_type size_type;
    typedef typename Graph::t_numRowEntries_ row_entries_type;
    typedef typename Graph::t_RowPtrs row_offsets_type;
    typedef typename Graph::t_LocalOrdinal_1D lclinds_1d_type;
    typedef t_ValuesType values_type;

    // fillComplete() only calls fillLocalGraphAndMatrix() if the
    // matrix owns the graph, which means myGraph_ is not null.
    TEUCHOS_TEST_FOR_EXCEPTION(
      myGraph_.is_null (), std::logic_error, "Tpetra::CrsMatrix::"
      "fillLocalGraphAndMatrix (called from fillComplete or "
      "expertStaticFillComplete): The nonconst graph (myGraph_) is null.  This "
      "means that the matrix has a const (a.k.a. \"static\") graph.  This may "
      "mean that fillComplete or expertStaticFillComplete has a bug, since it "
      "should never call fillLocalGraphAndMatrix in that case.  "
      "Please report this bug to the Tpetra developers.");

    const size_t lclNumRows = this->getNodeNumRows ();

    // This method's goal is to fill in the three arrays (compressed
    // sparse row format) that define the sparse graph's and matrix's
    // structure, and the sparse matrix's values.
    //
    // Use t_RowPtrs and not
    // Graph::LocalStaticCrsGraphType::row_map_type for k_ptrs,
    // because the latter is const and we need to modify k_ptrs here.
    row_offsets_type k_ptrs;
    lclinds_1d_type k_inds;
    values_type k_vals;

    // Get references to the data in myGraph_, so we can modify them
    // as well.  Note that we only call fillLocalGraphAndMatrix() if
    // the matrix owns the graph, which means myGraph_ is not null.
    lclinds_1d_type k_lclInds1D_ = myGraph_->k_lclInds1D_;

    // The number of entries in each locally owned row.  This is a
    // DualView.  2-D storage lives on host and is currently not
    // thread-safe for parallel kernels even on host, so we have to
    // work sequentially with host storage in that case.
    row_entries_type k_numRowEnt = myGraph_->k_numRowEntries_;
    typename row_entries_type::t_host h_numRowEnt = k_numRowEnt.h_view;

    if (getProfileType () == DynamicProfile) {
      // Pack 2-D storage (DynamicProfile) into 1-D packed storage.
      //
      // DynamicProfile means that the matrix's column indices and
      // values are currently stored in a 2-D "unpacked" format, in
      // the arrays-of-arrays myGraph_->lclInds2D_ (for column
      // indices) and values2D_ (for values).  We allocate 1-D storage
      // (k_inds resp. k_vals), and then copy from 2-D storage
      // (lclInds2D_ resp. values2D_) into 1-D storage (k_inds
      // resp. k_vals).
      TEUCHOS_TEST_FOR_EXCEPTION(
        static_cast<size_t> (k_numRowEnt.dimension_0 ()) != lclNumRows,
        std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix (called "
        "from fillComplete or expertStaticFillComplete): For the "
        "DynamicProfile branch, k_numRowEnt has the wrong length.  "
        "k_numRowEnt.dimension_0() = " << k_numRowEnt.dimension_0 ()
        << " != getNodeNumRows() = " << lclNumRows << "");

      // Pack the row offsets into k_ptrs, by doing a sum-scan of
      // the array of valid entry counts per row (h_numRowEnt).
      //
      // Total number of entries in the matrix on the calling
      // process.  We will compute this in the loop below.  It's
      // cheap to compute and useful as a sanity check.
      size_t lclTotalNumEntries = 0;
      // This will be a host view of packed row offsets.
      typename row_offsets_type::HostMirror h_ptrs;
      {
        // Allocate the packed row offsets array.  We use a nonconst
        // temporary (packedRowOffsets) here, because k_ptrs is const.
        // We will assign packedRowOffsets to k_ptrs below.
        row_offsets_type packedRowOffsets ("Tpetra::CrsGraph::ptr",
                                           lclNumRows+1);
        //
        // FIXME hack until we get parallel_scan in kokkos
        //
        h_ptrs = create_mirror_view (packedRowOffsets);
        h_ptrs(0) = 0;
        for (size_type i = 0; i < static_cast<size_type> (lclNumRows); ++i) {
          const size_t numEnt = h_numRowEnt(i);
          lclTotalNumEntries += numEnt;
          h_ptrs(i+1) = h_ptrs(i) + numEnt;
        }
        Kokkos::deep_copy (packedRowOffsets, h_ptrs);
        // packedRowOffsets is modifiable; k_ptrs isn't, so we have to
        // use packedRowOffsets in the loop above and assign here.
        k_ptrs = packedRowOffsets;
      }

      TEUCHOS_TEST_FOR_EXCEPTION(
        static_cast<size_t> (k_ptrs.dimension_0 ()) != lclNumRows + 1,
        std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: In "
        "DynamicProfile branch, after packing k_ptrs, k_ptrs.dimension_0()"
        " = " << k_ptrs.dimension_0 () << " != (lclNumRows+1) = "
        << (lclNumRows+1) << ".");
      TEUCHOS_TEST_FOR_EXCEPTION(
        static_cast<size_t> (h_ptrs.dimension_0 ()) != lclNumRows + 1,
        std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: In "
        "DynamicProfile branch, after packing h_ptrs, h_ptrs.dimension_0()"
        " = " << h_ptrs.dimension_0 () << " != (lclNumRows+1) = "
        << (lclNumRows+1) << ".");
      // FIXME (mfh 08 Aug 2014) This assumes UVM.
      TEUCHOS_TEST_FOR_EXCEPTION(
        k_ptrs(lclNumRows) != lclTotalNumEntries, std::logic_error,
        "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: In DynamicProfile branch, "
        "after packing k_ptrs, k_ptrs(lclNumRows = " << lclNumRows << ") = " <<
        k_ptrs(lclNumRows) << " != total number of entries on the calling "
        "process = " << lclTotalNumEntries << ".");

      // Allocate the arrays of packed column indices and values.
      k_inds = lclinds_1d_type ("Tpetra::CrsGraph::ind", lclTotalNumEntries);
      k_vals = t_ValuesType ("Tpetra::CrsMatrix::val", lclTotalNumEntries);

      // We need host views of the above, since 2-D storage lives on host.
      typename lclinds_1d_type::HostMirror h_inds = create_mirror_view (k_inds);
      typename values_type::HostMirror h_vals = create_mirror_view (k_vals);

      // Pack the column indices and values on the host.
      ArrayRCP<Array<LocalOrdinal> > lclInds2D = myGraph_->lclInds2D_;
      for (size_t row = 0; row < lclNumRows; ++row) {
        const size_t numEnt = h_numRowEnt(row);
        std::copy (lclInds2D[row].begin(),
                   lclInds2D[row].begin() + numEnt,
                   h_inds.ptr_on_device() + h_ptrs(row));
        std::copy (values2D_[row].begin(),
                   values2D_[row].begin() + numEnt,
                   h_vals.ptr_on_device() + h_ptrs(row));
      }
      // Copy the packed column indices and values to the device.
      Kokkos::deep_copy (k_inds, h_inds);
      Kokkos::deep_copy (k_vals, h_vals);

      // Sanity check of packed row offsets.
      if (k_ptrs.dimension_0 () != 0) {
        const size_t numOffsets = static_cast<size_t> (k_ptrs.dimension_0 ());
        TEUCHOS_TEST_FOR_EXCEPTION(
          static_cast<size_t> (k_ptrs(numOffsets-1)) != k_vals.dimension_0 (),
          std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: "
          "In DynamicProfile branch, after packing, k_ptrs(" << (numOffsets-1)
          << ") = " << k_ptrs(numOffsets-1) << " != k_vals.dimension_0() = "
          << k_vals.dimension_0 () << ".");
        TEUCHOS_TEST_FOR_EXCEPTION(
          static_cast<size_t> (k_ptrs(numOffsets-1)) != k_inds.dimension_0 (),
          std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: "
          "In DynamicProfile branch, after packing, k_ptrs(" << (numOffsets-1)
          << ") = " << k_ptrs(numOffsets-1) << " != k_inds.dimension_0() = "
          << k_inds.dimension_0 () << ".");
      }
    }
    else if (getProfileType () == StaticProfile) {
      // StaticProfile means that the matrix's column indices and
      // values are currently stored in a 1-D format, with row offsets
      // in k_rowPtrs_ and local column indices in k_lclInds1D_.

      // StaticProfile also means that the graph's array of row
      // offsets must already be allocated.
      typename Graph::LocalStaticCrsGraphType::row_map_type curRowOffsets =
        myGraph_->k_rowPtrs_;
      TEUCHOS_TEST_FOR_EXCEPTION(
        curRowOffsets.dimension_0 () == 0, std::logic_error,
      "curRowOffsets has size zero, but shouldn't");
      TEUCHOS_TEST_FOR_EXCEPTION(
        curRowOffsets.dimension_0 () != lclNumRows + 1, std::logic_error,
        "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: curRowOffsets has size "
        << curRowOffsets.dimension_0 () << " != lclNumRows + 1 = "
        << (lclNumRows + 1) << ".")
      {
        const size_t numOffsets = curRowOffsets.dimension_0 ();
        // FIXME (mfh 06 Aug 2014) This relies on UVM.
        TEUCHOS_TEST_FOR_EXCEPTION(
          numOffsets != 0 &&
          myGraph_->k_lclInds1D_.dimension_0 () != curRowOffsets(numOffsets - 1),
          std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: "
          "numOffsets = " << numOffsets << " != 0 and "
          "myGraph_->k_lclInds1D_.dimension_0() = "
          << myGraph_->k_lclInds1D_.dimension_0 ()
          << " != curRowOffsets(" << numOffsets << ") = "
          << curRowOffsets(numOffsets - 1) << ".");
      }

      if (myGraph_->nodeNumEntries_ != myGraph_->nodeNumAllocated_) {
        // The matrix's current 1-D storage is "unpacked."  This means
        // the row offsets may differ from what the final row offsets
        // should be.  This could happen, for example, if the user
        // specified StaticProfile in the constructor and set an upper
        // bound on the number of entries per row, but didn't fill all
        // those entries.
        TEUCHOS_TEST_FOR_EXCEPTION(
          static_cast<size_t> (k_numRowEnt.dimension_0 ()) != lclNumRows,
          std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix (called"
          " from fillComplete or expertStaticFillComplete): In StaticProfile "
          "unpacked branch, k_numRowEnt has the wrong length.  "
          "k_numRowEnt.dimension_0() = " << k_numRowEnt.dimension_0 ()
          << " != getNodeNumRows() = " << lclNumRows << ".");

        if (curRowOffsets.dimension_0 () != 0) {
          const size_t numOffsets =
            static_cast<size_t> (curRowOffsets.dimension_0 ());
          TEUCHOS_TEST_FOR_EXCEPTION(
            curRowOffsets(numOffsets-1) != static_cast<size_t> (k_values1D_.dimension_0 ()),
            std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: "
            "In StaticProfile branch, before allocating or packing, "
            "curRowOffsets(" << (numOffsets-1) << ") = "
            << curRowOffsets(numOffsets - 1)
            << " != k_values1D_.dimension_0() = "
            << k_values1D_.dimension_0 () << ".");
          TEUCHOS_TEST_FOR_EXCEPTION(
            static_cast<size_t> (curRowOffsets(numOffsets - 1)) !=
            myGraph_->k_lclInds1D_.dimension_0 (),
            std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: "
            "In StaticProfile branch, before allocating or packing, "
            "curRowOffsets(" << (numOffsets-1) << ") = "
            << curRowOffsets(numOffsets - 1)
            << " != myGraph_->k_lclInds1D_.dimension_0() = "
            << myGraph_->k_lclInds1D_.dimension_0 () << ".");
        }

        // Pack the row offsets into k_ptrs, by doing a sum-scan of
        // the array of valid entry counts per row (h_numRowEnt).

        // Total number of entries in the matrix on the calling
        // process.  We will compute this in the loop below.  It's
        // cheap to compute and useful as a sanity check.
        size_t lclTotalNumEntries = 0;
        // This will be a host view of packed row offsets.
        typename row_offsets_type::HostMirror h_ptrs;
        {
          // Allocate the packed row offsets array.  We use a nonconst
          // temporary (packedRowOffsets) here, because k_ptrs is
          // const.  We will assign packedRowOffsets to k_ptrs below.
          row_offsets_type packedRowOffsets ("Tpetra::CrsGraph::ptr",
                                             lclNumRows+1);
          //
          // FIXME hack until we get parallel_scan in Kokkos
          //
          // Unlike in the 2-D storage case above, we don't need the
          // host view of the packed row offsets array after packing
          // the row offsets.
          h_ptrs = create_mirror_view (packedRowOffsets);
          h_ptrs(0) = 0;
          for (size_type i = 0; i < static_cast<size_type> (lclNumRows); ++i) {
            const size_t numEnt = h_numRowEnt(i);
            lclTotalNumEntries += numEnt;
            h_ptrs(i+1) = h_ptrs(i) + numEnt;
          }
          Kokkos::deep_copy (packedRowOffsets, h_ptrs);
          // packedRowOffsets is modifiable; k_ptrs isn't, so we have
          // to use packedRowOffsets in the loop above and assign here.
          k_ptrs = packedRowOffsets;
        }

        TEUCHOS_TEST_FOR_EXCEPTION(
          static_cast<size_t> (k_ptrs.dimension_0 ()) != lclNumRows + 1,
          std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: For "
          "the StaticProfile unpacked-but-pack branch, after packing k_ptrs, "
          "k_ptrs.dimension_0() = " << k_ptrs.dimension_0 () << " != "
          "lclNumRows+1 = " << (lclNumRows+1) << ".");
        // FIXME (mfh 06 Aug 2014) This assumes UVM.
        TEUCHOS_TEST_FOR_EXCEPTION(
          k_ptrs(lclNumRows) != lclTotalNumEntries, std::logic_error,
          "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: In StaticProfile "
          "unpacked-but-pack branch, after filling k_ptrs, k_ptrs(lclNumRows="
          << lclNumRows << ") = " << k_ptrs(lclNumRows) << " != total number "
          "of entries on the calling process = " << lclTotalNumEntries << ".");

        // Allocate the arrays of packed column indices and values.
        k_inds = lclinds_1d_type ("Tpetra::CrsGraph::ind", lclTotalNumEntries);
        k_vals = t_ValuesType ("Tpetra::CrsMatrix::val", lclTotalNumEntries);

        // curRowOffsets (myGraph_->k_rowPtrs_) (???), k_lclInds1D_,
        // and k_values1D_ are currently unpacked.  Pack them, using
        // the packed row offsets array k_ptrs that we created above.
        //
        // FIXME (mfh 06 Aug 2014) If "Optimize Storage" is false, we
        // need to keep around the unpacked row offsets, column
        // indices, and values arrays.

        // Pack the column indices from unpacked k_lclInds1D_ into
        // packed k_inds.  We will replace k_lclInds1D_ below.
        typedef pack_functor<typename Graph::t_LocalOrdinal_1D,
          typename Graph::LocalStaticCrsGraphType::row_map_type>
          inds_packer_type;
        inds_packer_type indsPacker (k_inds, myGraph_->k_lclInds1D_,
                                     k_ptrs, curRowOffsets);
        Kokkos::parallel_for (lclNumRows, indsPacker);

        // Pack the values from unpacked k_values1D_ into packed
        // k_vals.  We will replace k_values1D_ below.
        typedef pack_functor<t_ValuesType,
          typename Graph::LocalStaticCrsGraphType::row_map_type>
          vals_packer_type;
        vals_packer_type valsPacker (k_vals, this->k_values1D_,
                                     k_ptrs, curRowOffsets);
        Kokkos::parallel_for (lclNumRows, valsPacker);

        TEUCHOS_TEST_FOR_EXCEPTION(
          k_ptrs.dimension_0 () == 0, std::logic_error, "Tpetra::CrsMatrix::"
          "fillLocalGraphAndMatrix: In StaticProfile \"Optimize Storage\" = "
          "true branch, after packing, k_ptrs.dimension_0() = 0.  This "
          "probably means that k_rowPtrs_ was never allocated.");
        if (k_ptrs.dimension_0 () != 0) {
          const size_t numOffsets = static_cast<size_t> (k_ptrs.dimension_0 ());
          TEUCHOS_TEST_FOR_EXCEPTION(
            static_cast<size_t> (k_ptrs(numOffsets - 1)) != k_vals.dimension_0 (),
            std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: "
            "In StaticProfile \"Optimize Storage\"=true branch, after packing, "
            "k_ptrs(" << (numOffsets-1) << ") = " << k_ptrs(numOffsets-1) <<
            " != k_vals.dimension_0() = " << k_vals.dimension_0 () << ".");
          TEUCHOS_TEST_FOR_EXCEPTION(
            static_cast<size_t> (k_ptrs(numOffsets - 1)) != k_inds.dimension_0 (),
            std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: "
            "In StaticProfile \"Optimize Storage\"=true branch, after packing, "
            "k_ptrs(" << (numOffsets-1) << ") = " << k_ptrs(numOffsets-1) <<
            " != k_inds.dimension_0() = " << k_inds.dimension_0 () << ".");
        }
      }
      else { // We don't have to pack, so just set the pointers.
        k_ptrs = myGraph_->k_rowPtrs_;
        k_inds = myGraph_->k_lclInds1D_;
        k_vals = this->k_values1D_;

        TEUCHOS_TEST_FOR_EXCEPTION(
          k_ptrs.dimension_0 () == 0, std::logic_error, "Tpetra::CrsMatrix::"
          "fillLocalGraphAndMatrix: In StaticProfile \"Optimize Storage\" = "
          "false branch, k_ptrs.dimension_0() = 0.  This probably means that "
          "k_rowPtrs_ was never allocated.");
        if (k_ptrs.dimension_0 () != 0) {
          const size_t numOffsets = static_cast<size_t> (k_ptrs.dimension_0 ());
          TEUCHOS_TEST_FOR_EXCEPTION(
            static_cast<size_t> (k_ptrs(numOffsets - 1)) != k_vals.dimension_0 (),
            std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: "
            "In StaticProfile \"Optimize Storage\" = false branch, "
            "k_ptrs(" << (numOffsets-1) << ") = " << k_ptrs(numOffsets - 1)
            << " != k_vals.dimension_0() = " << k_vals.dimension_0 () << ".");
          TEUCHOS_TEST_FOR_EXCEPTION(
            static_cast<size_t> (k_ptrs(numOffsets - 1)) != k_inds.dimension_0 (),
            std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: "
            "In StaticProfile \"Optimize Storage\" = false branch, "
            "k_ptrs(" << (numOffsets-1) << ") = " << k_ptrs(numOffsets - 1)
            << " != k_inds.dimension_0() = " << k_inds.dimension_0 () << ".");
        }
      }
    }

    // Extra sanity checks.
    TEUCHOS_TEST_FOR_EXCEPTION(
      static_cast<size_t> (k_ptrs.dimension_0 ()) != lclNumRows + 1,
      std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: After "
      "packing, k_ptrs.dimension_0() = " << k_ptrs.dimension_0 ()
      << " != lclNumRows+1 = " << (lclNumRows+1) << ".");
    if (k_ptrs.dimension_0 () != 0) {
      const size_t numOffsets = static_cast<size_t> (k_ptrs.dimension_0 ());
      TEUCHOS_TEST_FOR_EXCEPTION(
        static_cast<size_t> (k_ptrs(numOffsets - 1)) != k_vals.dimension_0 (),
        std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: After "
        "packing, k_ptrs(" << (numOffsets-1) << ") = " << k_ptrs(numOffsets-1)
        << " != k_vals.dimension_0() = " << k_vals.dimension_0 () << ".");
      TEUCHOS_TEST_FOR_EXCEPTION(
        static_cast<size_t> (k_ptrs(numOffsets - 1)) != k_inds.dimension_0 (),
        std::logic_error, "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: After "
        "packing, k_ptrs(" << (numOffsets-1) << ") = " << k_ptrs(numOffsets-1)
        << " != k_inds.dimension_0() = " << k_inds.dimension_0 () << ".");
    }

    // May we ditch the old allocations for the packed (and otherwise
    // "optimized") allocations, later in this routine?  Optimize
    // storage if the graph is not static, or if the graph already has
    // optimized storage.
    const bool defaultOptStorage =
      ! isStaticGraph () || staticGraph_->isStorageOptimized ();
    const bool requestOptimizedStorage =
      (! params.is_null () && params->get ("Optimize Storage", defaultOptStorage)) ||
      (params.is_null () && defaultOptStorage);

    // The graph has optimized storage when indices are allocated,
    // myGraph_->k_numRowEntries_ is empty, and there are more than
    // zero rows on this process.  It's impossible for the graph to
    // have dynamic profile (getProfileType() == DynamicProfile) and
    // be optimized (isStorageOptimized()).
    if (requestOptimizedStorage) {
      // Free the old, unpacked, unoptimized allocations.
      // Change the graph from dynamic to static allocation profile

      // Free graph data structures that are only needed for 2-D or
      // unpacked 1-D storage.
      myGraph_->lclInds2D_ = null; // legacy KokkosClassic 2-D storage
      myGraph_->k_numRowEntries_ = row_entries_type ();
      myGraph_->numRowEntries_ = null; // legacy KokkosClassic view of above

      // Free the matrix's 2-D storage.
      this->values2D_ = null;

      // Keep the new 1-D packed allocations.
      myGraph_->k_rowPtrs_ = k_ptrs;
      myGraph_->k_lclInds1D_ = k_inds;
      this->k_values1D_ = k_vals;

      // Set Kokkos classic pointers for backwards compatibility.
      myGraph_->rowPtrs_ =
        arcp_const_cast<size_t> (Kokkos::Compat::persistingView (k_ptrs));
      myGraph_->lclInds1D_ = Kokkos::Compat::persistingView (k_inds);
      this->values1D_ = Kokkos::Compat::persistingView (k_vals);

      // Storage is packed now, so the number of allocated entries is
      // the same as the actual number of entries.
      myGraph_->nodeNumAllocated_ = myGraph_->nodeNumEntries_;
      // The graph is definitely StaticProfile now, whether or not it
      // was before.
      myGraph_->pftype_ = StaticProfile;
    }

    RCP<Teuchos::ParameterList> lclparams;
    if (params.is_null ()) {
      lclparams = Teuchos::parameterList ();
    } else {
      lclparams = Teuchos::sublist (params, "Local Graph");
    }

    // Make the local graph, using the arrays of row offsets and
    // column indices that we built above.  The local graph should be
    // null, but we delete it first so that any memory can be freed
    // before we allocate the new one.
    myGraph_->k_lclGraph_ =
      typename Graph::LocalStaticCrsGraphType (k_inds, k_ptrs);

    // DEBUG ONLY
    // comm.barrier ();
    // cerr << myRank << ": MADE k_lclGraph_" << endl;
    // comm.barrier ();

    // Set up Kokkos classic "local graph" object.
    //
    // FIXME (mfh 06 Aug 2014) It would make more sense for
    // Tpetra::CrsGraph to have a protected method that accepts k_inds
    // and k_ptrs, and creates both k_lclGraph_ (new Kokkos) and
    // lclGraph_ (classic Kokkos).
    myGraph_->lclGraph_ = null;
    {
      const map_type& rowMap = * (getRowMap ());
      myGraph_->lclGraph_ =
        rcp (new local_graph_type (rowMap.getNodeNumElements (),
                                   getColMap ()->getNodeNumElements (),
                                   rowMap.getNode (), lclparams));
    }
    myGraph_->lclGraph_->setStructure (Kokkos::Compat::persistingView (k_ptrs),
                                       Kokkos::Compat::persistingView (k_inds));

    // DEBUG ONLY
    // comm.barrier ();
    // cerr << myRank << ": MADE lclGraph_" << endl;
    // comm.barrier ();

    // Make the local matrix, using the local graph and vals array.
    if (params.is_null ()) {
      lclparams = Teuchos::parameterList ();
    } else {
      lclparams = Teuchos::sublist (params, "Local Matrix");
    }
    TEUCHOS_TEST_FOR_EXCEPTION(
      staticGraph_->getLocalGraph ().is_null (), std::logic_error,
      "Tpetra::CrsMatrix::fillLocalGraphAndMatrix: The local graph "
      "(staticGraph_->getLocalGraph()) is null.  Please report this bug to the "
      "Tpetra developers.");

    // k_lclMatrix_ = k_local_matrix_type ("Tpetra::CrsMatrix::k_lclMatrix_",
    //                                     getNodeNumCols (), k_vals,
    //                                     staticGraph_->getLocalGraph_Kokkos ());
    k_lclMatrix_ = k_local_matrix_type ("Tpetra::CrsMatrix::k_lclMatrix_",
                                        getNodeNumCols (), k_vals,
                                        myGraph_->k_lclGraph_);
    // DEBUG ONLY
    // comm.barrier ();
    // cerr << myRank << ": MADE k_lclMatrix_" << endl;
    // comm.barrier ();

    // Finalize the local graph and matrix together.
    if (params.is_null ()) {
      lclparams = Teuchos::parameterList ();
    } else {
      lclparams = Teuchos::sublist (params, "Local Sparse Ops");
    }
    // Figure out if the matrix has a unit diagonal, and whether it is
    // upper or lower triangular (or neither).
    const Teuchos::EDiag diag = getNodeNumDiags() < lclNumRows ?
      Teuchos::UNIT_DIAG :
      Teuchos::NON_UNIT_DIAG;
    Teuchos::EUplo uplo = Teuchos::UNDEF_TRI;
    if (isUpperTriangular ()) {
      uplo = Teuchos::UPPER_TRI;
    } else if (isLowerTriangular ()) {
      uplo = Teuchos::LOWER_TRI;
    }

    // Set Kokkos classic values array for backwards compatibility.
    ArrayRCP<scalar_type> classicValues =
      Kokkos::Compat::persistingView (k_vals);

    // Set up Kokkos classic "local mat ops" object.
    // RCP<local_matrix_type> classicLocalMatrix =
    //   rcp (new local_matrix_type (staticGraph_->getLocalGraph (), lclparams));
    RCP<local_matrix_type> classicLocalMatrix =
      rcp (new local_matrix_type (myGraph_->getLocalGraph (), lclparams));
    classicLocalMatrix->setValues (classicValues);
    sparse_ops_type::finalizeGraphAndMatrix (uplo, diag,
                                             *myGraph_->getLocalGraphNonConst (),
                                             *classicLocalMatrix,
                                             lclparams);
    // DEBUG ONLY
    // comm.barrier ();
    // cerr << myRank << ": OMG WOAH DONE" << endl;
    // comm.barrier ();
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  fillLocalMatrix (const Teuchos::RCP<Teuchos::ParameterList>& params)
  {
    using Kokkos::create_mirror_view;
    using Teuchos::ArrayRCP;
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    typedef LocalOrdinal LO;
    typedef typename Graph::t_numRowEntries_ row_entries_type;
    typedef typename Graph::LocalStaticCrsGraphType::row_map_type row_map_type;
    typedef typename Graph::t_RowPtrs row_offsets_type;
    typedef ArrayRCP<size_t>::size_type size_type;

    const size_t lclNumRows = getNodeNumRows();
    const map_type& rowMap = * (getRowMap ());
    RCP<node_type> node = rowMap.getNode ();

    // The goals of this routine are first, to allocate and fill
    // packed 1-D storage (see below for an explanation) in the vals
    // array, and second, to give vals to the local matrix and
    // finalize the local matrix.  We only need k_ptrs, the packed 1-D
    // row offsets, within the scope of this routine, since we're only
    // filling the local matrix here (use fillLocalGraphAndMatrix() to
    // fill both the graph and the matrix at the same time).

    // get data from staticGraph_
    ArrayRCP<LO> lclInds1D         = staticGraph_->lclInds1D_;
    ArrayRCP<Array<LO> > lclInds2D = staticGraph_->lclInds2D_;
    ArrayRCP<size_t> numRowEntries = staticGraph_->numRowEntries_;
    size_t nodeNumEntries   = staticGraph_->nodeNumEntries_;
    size_t nodeNumAllocated = staticGraph_->nodeNumAllocated_;
    row_map_type k_rowPtrs_ = staticGraph_->k_lclGraph_.row_map;

    row_map_type k_ptrs; // "packed" row offsets array
    t_ValuesType k_vals; // "packed" values array

    // May we ditch the old allocations for the packed (and otherwise
    // "optimized") allocations, later in this routine?  Request
    // optimized storage by default.
    bool requestOptimizedStorage = true;
    const bool default_OptimizeStorage =
      ! isStaticGraph () || staticGraph_->isStorageOptimized ();
    if (! params.is_null () && ! params->get ("Optimize Storage", default_OptimizeStorage)) {
      requestOptimizedStorage = false;
    }
    // If we're not allowed to change a static graph, then we can't
    // change the storage of the matrix, either.  This means that if
    // the graph's storage isn't already optimized, we can't optimize
    // the matrix's storage either.  Check and give warning, as
    // appropriate.
    if (! staticGraph_->isStorageOptimized () && requestOptimizedStorage) {
      TPETRA_ABUSE_WARNING(true, std::runtime_error,
        "::fillLocalMatrix(): You requested optimized storage by setting the"
        "\"Optimize Storage\" flag to \"true\" in the parameter list, or by virtue"
        "of default behavior. However, the associated CrsGraph was filled separately"
        "and requested not to optimize storage. Therefore, the CrsMatrix cannot"
        "optimize storage.");
      requestOptimizedStorage = false;
    }

    // The number of entries in each locally owned row.  This is a
    // DualView.  2-D storage lives on host and is currently not
    // thread-safe for parallel kernels even on host, so we have to
    // work sequentially with host storage in that case.
    row_entries_type k_numRowEnt = staticGraph_->k_numRowEntries_;
    typename row_entries_type::t_host h_numRowEnt = k_numRowEnt.h_view;

    if (getProfileType() == DynamicProfile) {
      // Pack 2-D storage (DynamicProfile) into 1-D packed storage.
      //
      // DynamicProfile means that the matrix's values are currently
      // stored in a 2-D "unpacked" format, in the array-of-arrays
      // values2D_.  We allocate 1-D storage and then copy from 2-D
      // storage in values2D_ into 1-D storage in k_vals.  Since we're
      // only allocating the local matrix here, not the local graph,
      // we don't need to keep the row offsets array, but we do need
      // it here temporarily in order to convert to 1-D storage.  (The
      // allocStorage() function needs it.)  We'll free ptrs later in
      // this method.
      //
      // FIXME (mfh 08 Aug 2014) If we're in this method, then the
      // graph should already have packed 1-D storage.  Why can't we
      // just use the graph's current row offsets array?

      // Pack the row offsets into k_ptrs, by doing a sum-scan of
      // the array of valid entry counts per row (h_numRowEnt).
      //
      // Total number of entries in the matrix on the calling
      // process.  We will compute this in the loop below.  It's
      // cheap to compute and useful as a sanity check.
      size_t lclTotalNumEntries = 0;
      // This will be a host view of packed row offsets.
      typename row_offsets_type::HostMirror h_ptrs;
      {
        row_offsets_type packedRowOffsets ("Tpetra::CrsGraph::ptr", lclNumRows+1);
        //
        // FIXME hack until we get parallel_scan in Kokkos
        //
        h_ptrs = create_mirror_view (packedRowOffsets);
        h_ptrs(0) = 0;
        for (size_type i = 0; i < static_cast<size_t> (lclNumRows); ++i) {
          const size_t numEnt = h_numRowEnt(i);
          lclTotalNumEntries += numEnt;
          h_ptrs(i+1) = h_ptrs(i) + numEnt;
        }
        Kokkos::deep_copy (packedRowOffsets, h_ptrs);
        k_ptrs = packedRowOffsets;
      }

      TEUCHOS_TEST_FOR_EXCEPTION(
        static_cast<size_t> (k_ptrs.dimension_0 ()) != lclNumRows + 1,
        std::logic_error, "Tpetra::CrsMatrix::fillLocalMatrix: In "
        "DynamicProfile branch, after packing k_ptrs, k_ptrs.dimension_0()"
        " = " << k_ptrs.dimension_0 () << " != (lclNumRows+1) = "
        << (lclNumRows+1) << ".");
      TEUCHOS_TEST_FOR_EXCEPTION(
        static_cast<size_t> (h_ptrs.dimension_0 ()) != lclNumRows + 1,
        std::logic_error, "Tpetra::CrsMatrix::fillLocalMatrix: In "
        "DynamicProfile branch, after packing h_ptrs, h_ptrs.dimension_0()"
        " = " << h_ptrs.dimension_0 () << " != (lclNumRows+1) = "
        << (lclNumRows+1) << ".");
      // FIXME (mfh 08 Aug 2014) This assumes UVM.
      TEUCHOS_TEST_FOR_EXCEPTION(
        k_ptrs(lclNumRows) != lclTotalNumEntries, std::logic_error,
        "Tpetra::CrsMatrix::fillLocalMatrix: In DynamicProfile branch, "
        "after packing k_ptrs, k_ptrs(lclNumRows = " << lclNumRows << ") = " <<
        k_ptrs(lclNumRows) << " != total number of entries on the calling "
        "process = " << lclTotalNumEntries << ".");

      // Allocate the array of packed values.
      k_vals = t_ValuesType ("Tpetra::CrsMatrix::val", lclTotalNumEntries);
      // We need a host view of the above, since 2-D storage lives on host.
      typename t_ValuesType::HostMirror h_vals =
        Kokkos::create_mirror_view (k_vals);
      // Pack the values on the host.
      for (size_t lclRow = 0; lclRow < lclNumRows; ++lclRow) {
        const size_t numEnt = h_numRowEnt(lclRow);
        std::copy (values2D_[lclRow].begin(),
                   values2D_[lclRow].begin() + numEnt,
                   h_vals.ptr_on_device() + h_ptrs(lclRow));
      }
      // Copy the packed values to the device.
      Kokkos::deep_copy (k_vals, h_vals);

      // Sanity check of packed row offsets.
      if (k_ptrs.dimension_0 () != 0) {
        const size_t numOffsets = static_cast<size_t> (k_ptrs.dimension_0 ());
        TEUCHOS_TEST_FOR_EXCEPTION(
          static_cast<size_t> (k_ptrs(numOffsets-1)) != k_vals.dimension_0 (),
          std::logic_error, "Tpetra::CrsMatrix::fillLocalMatrix: "
          "In DynamicProfile branch, after packing, k_ptrs(" << (numOffsets-1)
          << ") = " << k_ptrs(numOffsets-1) << " != k_vals.dimension_0() = "
          << k_vals.dimension_0 () << ".");
      }
    }
    else if (getProfileType () == StaticProfile) {
      // StaticProfile means that the matrix's values are currently
      // stored in a 1-D format.  However, this format is "unpacked";
      // it doesn't necessarily have the same row offsets as indicated
      // by the ptrs array returned by allocRowPtrs.  This could
      // happen, for example, if the user specified StaticProfile in
      // the constructor and fixed the number of matrix entries in
      // each row, but didn't fill all those entries.
      //
      // As above, we don't need to keep the "packed" row offsets
      // array ptrs here, but we do need it here temporarily, so we
      // have to allocate it.  We'll free ptrs later in this method.
      //
      // Note that this routine checks whether storage has already
      // been packed.  This is a common case for solution of nonlinear
      // PDEs using the finite element method, as long as the
      // structure of the sparse matrix does not change between linear
      // solves.
      if (nodeNumEntries != nodeNumAllocated) {
        // We have to pack the 1-D storage, since the user didn't fill
        // up all requested storage.
        typename Graph::t_RowPtrs tmpk_ptrs ("Tpetra::CrsGraph::ptr",
                                             lclNumRows+1);
        // Total number of entries in the matrix on the calling
        // process.  We will compute this in the loop below.  It's
        // cheap to compute and useful as a sanity check.
        size_t lclTotalNumEntries = 0;
        k_ptrs = tmpk_ptrs;
        {
          //
          // FIXME hack until we get parallel_scan in Kokkos
          //
          typename row_offsets_type::HostMirror h_ptrs =
            create_mirror_view (tmpk_ptrs);
          h_ptrs(0) = 0;
          for (size_type i = 0; i < static_cast<size_t> (lclNumRows); ++i) {
            const size_t numEnt = h_numRowEnt(i);
            lclTotalNumEntries += numEnt;
            h_ptrs(i+1) = h_ptrs(i) + numEnt;
          }
          Kokkos::deep_copy (tmpk_ptrs, h_ptrs);
        }

        // Allocate the "packed" values array.
        // It has exactly the right number of entries.
        k_vals = t_ValuesType ("Tpetra::CrsMatrix::val", lclTotalNumEntries);

        // Pack k_values1D_ into k_vals.  We will replace k_values1D_ below.
        typedef pack_functor<t_ValuesType,
          typename Graph::LocalStaticCrsGraphType::row_map_type>
          packer_type;
        packer_type valsPacker (k_vals, k_values1D_, tmpk_ptrs, k_rowPtrs_);
        Kokkos::parallel_for (lclNumRows, valsPacker);
      }
      else { // We don't have to pack, so just set the pointer.
        k_vals = k_values1D_;
      }
    }

    // May we ditch the old allocations for the packed one?
    if (requestOptimizedStorage) {
      // The user requested optimized storage, so we can dump the
      // unpacked 2-D and 1-D storage, and keep the packed storage.
      values2D_ = null;
      k_values1D_ = k_vals;
    }

    // build the matrix, hand over the values
    RCP<ParameterList> lclparams;
    if (params.is_null ()) {
      lclparams = parameterList ();
    }
    else {
      lclparams = sublist (params, "Local Matrix");
    }

    TEUCHOS_TEST_FOR_EXCEPTION(
      staticGraph_->getLocalGraph ().is_null (), std::runtime_error,
      "Tpetra::CrsMatrix::fillLocalMatrix (called by fillComplete with a const "
      "graph): the local graph is null.  This can happen if you constructed "
      "this CrsMatrix B using a const CrsGraph that belongs to another "
      "CrsMatrix A, and A is fill complete.  You can prevent this error by "
      "setting the bool parameter \"Preserve Local Graph\" to true when "
      "calling fillComplete on the original CrsMatrix A.");

    k_lclMatrix_ = k_local_matrix_type ("Tpetra::CrsMatrix::k_lclMatrix_",
                                        getDomainMap ()->getNodeNumElements (),
                                        k_vals,
                                        staticGraph_->getLocalGraph_Kokkos ());

    // Set the legacy values1D_ array.
    ArrayRCP<scalar_type> classicValues =
      Kokkos::Compat::persistingView (k_lclMatrix_.values);
    values1D_ = classicValues;

    // Finalize the local matrix.
    lclparams = params.is_null () ? parameterList () :
      sublist (params, "Local Sparse Ops");
    RCP<local_matrix_type> classicLocalMatrix =
      rcp (new local_matrix_type (staticGraph_->getLocalGraph (), lclparams));
    classicLocalMatrix->setValues (classicValues);
    sparse_ops_type::finalizeMatrix (* (staticGraph_->getLocalGraph ()),
                                     *classicLocalMatrix, lclparams);
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  insertLocalValues (const LocalOrdinal localRow,
                     const Teuchos::ArrayView<const LocalOrdinal>& indices,
                     const Teuchos::ArrayView<const Scalar>& values)
  {
    using Teuchos::Array;
    using Teuchos::ArrayView;
    using Teuchos::as;
    using Teuchos::toString;
    using std::endl;
    const char tfecfFuncName[] = "insertLocalValues";

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(! isFillActive (), std::runtime_error,
      ": Fill is not active.  After calling fillComplete, you must call "
      "resumeFill before you may insert entries into the matrix again.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(isStaticGraph (),  std::runtime_error,
      " cannot insert indices with static graph; use replaceLocalValues() instead.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(myGraph_->isGloballyIndexed(),
      std::runtime_error, ": graph indices are global; use insertGlobalValues().");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(! hasColMap (), std::runtime_error,
      " cannot insert local indices without a column map.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(values.size() != indices.size(),
      std::runtime_error, ": values.size() must equal indices.size().");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! getRowMap()->isNodeLocalElement(localRow), std::runtime_error,
      ": Local row index " << localRow << " does not belong to this process.");

    if (! myGraph_->indicesAreAllocated ()) {
      try {
        allocateValues (LocalIndices, GraphNotYetAllocated);
      }
      catch (std::exception& e) {
        TEUCHOS_TEST_FOR_EXCEPTION(
          true, std::runtime_error, "Tpetra::CrsMatrix::insertLocalValues: "
          "allocateValues(LocalIndices,GraphNotYetAllocated) threw an "
          "exception: " << e.what ());
      }
    }

    const size_t numEntriesToAdd = static_cast<size_t> (indices.size ());
#ifdef HAVE_TPETRA_DEBUG
    // In a debug build, if the matrix has a column Map, test whether
    // any of the given column indices are not in the column Map.
    // Keep track of the invalid column indices so we can tell the
    // user about them.
    if (hasColMap ()) {
      const map_type& colMap = * (getColMap ());
      Array<LocalOrdinal> badColInds;
      bool allInColMap = true;
      for (size_t k = 0; k < numEntriesToAdd; ++k) {
        if (! colMap.isNodeLocalElement (indices[k])) {
          allInColMap = false;
          badColInds.push_back (indices[k]);
        }
      }
      if (! allInColMap) {
        std::ostringstream os;
        os << "Tpetra::CrsMatrix::insertLocalValues: You attempted to insert "
          "entries in owned row " << localRow << ", at the following column "
          "indices: " << toString (indices) << "." << endl;
        os << "Of those, the following indices are not in the column Map on "
          "this process: " << toString (badColInds) << "." << endl << "Since "
          "the matrix has a column Map already, it is invalid to insert "
          "entries at those locations.";
        TEUCHOS_TEST_FOR_EXCEPTION(! allInColMap, std::invalid_argument, os.str ());
      }
    }
#endif // HAVE_TPETRA_DEBUG

#ifdef HAVE_TPETRA_DEBUG
    RowInfo rowInfo;
    try {
      rowInfo = myGraph_->getRowInfo (localRow);
    } catch (std::exception& e) {
      TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::runtime_error, "Tpetra::CrsMatrix::insertLocalValues: "
        "myGraph_->getRowInfo threw an exception: " << e.what ());
    }
#else
    RowInfo rowInfo = myGraph_->getRowInfo (localRow);
#endif // HAVE_TPETRA_DEBUG

    const size_t curNumEntries = rowInfo.numEntries;
    const size_t newNumEntries = curNumEntries + numEntriesToAdd;
    if (newNumEntries > rowInfo.allocSize) {
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
        getProfileType() == StaticProfile, std::runtime_error,
        ": new indices exceed statically allocated graph structure.");

      // Make space for the new matrix entries.
      try {
        rowInfo = myGraph_->template updateLocalAllocAndValues<Scalar> (rowInfo,
                                                                        newNumEntries,
                                                                        values2D_[localRow]);
      } catch (std::exception& e) {
        TEUCHOS_TEST_FOR_EXCEPTION(
          true, std::runtime_error, "Tpetra::CrsMatrix::insertLocalValues: "
          "myGraph_->updateGlobalAllocAndValues threw an exception: "
          << e.what ());
      }
    }
    typename Graph::SLocalGlobalViews indsView;
    indsView.linds = indices;

#ifdef HAVE_TPETRA_DEBUG
    ArrayView<Scalar> valsView;
    try {
      valsView = this->getViewNonConst (rowInfo);
    } catch (std::exception& e) {
      TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::runtime_error, "Tpetra::CrsMatrix::insertLocalValues: "
        "getViewNonConst threw an exception: " << e.what ());
    }
#else
    ArrayView<Scalar> valsView = this->getViewNonConst (rowInfo);
#endif // HAVE_TPETRA_DEBUG

    try {
      myGraph_->template insertIndicesAndValues<Scalar> (rowInfo, indsView,
                                                         valsView, values,
                                                         LocalIndices,
                                                         LocalIndices);
    } catch (std::exception& e) {
      TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::runtime_error, "Tpetra::CrsMatrix::insertLocalValues: "
        "myGraph_->insertIndicesAndValues threw an exception: "
        << e.what ());
    }

#ifdef HAVE_TPETRA_DEBUG
    const size_t chkNewNumEntries = myGraph_->getNumEntriesInLocalRow (localRow);
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      chkNewNumEntries != newNumEntries, std::logic_error,
      ": The row should have " << newNumEntries << " entries after insert, but "
      "instead has " << chkNewNumEntries << ".  Please report this bug to the "
      "Tpetra developers.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(! isLocallyIndexed(), std::logic_error,
      ": At end of insertLocalValues(), this CrsMatrix is not locally indexed.  "
      "Please report this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  insertLocalValuesFiltered (const LocalOrdinal localRow,
                             const Teuchos::ArrayView<const LocalOrdinal>& indices,
                             const Teuchos::ArrayView<const Scalar>& values)
  {
    const char tfecfFuncName[] = "insertLocalValues";
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(! isFillActive (), std::runtime_error,
      " requires that fill is active.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(isStaticGraph (),  std::runtime_error,
      " cannot insert indices with static graph; use replaceLocalValues() instead.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(myGraph_->isGloballyIndexed(),
      std::runtime_error, ": graph indices are global; use insertGlobalValues().");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(! hasColMap (), std::runtime_error,
      " cannot insert local indices without a column map.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(values.size() != indices.size(),
      std::runtime_error, ": values.size() must equal indices.size().");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! getRowMap()->isNodeLocalElement (localRow), std::runtime_error,
      ": Local row index " << localRow << " does not belong to this process.");
    if (! myGraph_->indicesAreAllocated ()) {
      allocateValues (LocalIndices, GraphNotYetAllocated);
    }
    // Use the graph to filter incoming entries whose column indices
    // aren't in the column Map.
    Teuchos::Array<LocalOrdinal> f_inds (indices);
    Teuchos::Array<Scalar> f_vals (values);
    const size_t numFilteredEntries =
      myGraph_->template filterLocalIndicesAndValues<Scalar> (f_inds (),
                                                              f_vals ());
    if (numFilteredEntries > 0) {
      RowInfo rowInfo = myGraph_->getRowInfo (localRow);
      const size_t curNumEntries = rowInfo.numEntries;
      const size_t newNumEntries = curNumEntries + numFilteredEntries;
      if (newNumEntries > rowInfo.allocSize) {
        TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
          getProfileType () == StaticProfile, std::runtime_error,
          ": new indices exceed statically allocated graph structure.  "
          "newNumEntries (" << newNumEntries << " > rowInfo.allocSize ("
          << rowInfo.allocSize << ").");
        // Make space for the new matrix entries.
        rowInfo =
          myGraph_->template updateLocalAllocAndValues<Scalar> (rowInfo,
                                                                newNumEntries,
                                                                values2D_[localRow]);
      }
      typename Graph::SLocalGlobalViews inds_view;
      inds_view.linds = f_inds (0, numFilteredEntries);
      myGraph_->template insertIndicesAndValues<Scalar> (rowInfo, inds_view,
                                                         this->getViewNonConst (rowInfo),
                                                         f_vals, LocalIndices,
                                                         LocalIndices);
#ifdef HAVE_TPETRA_DEBUG
      const size_t chkNewNumEntries = myGraph_->getNumEntriesInLocalRow (localRow);
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(chkNewNumEntries != newNumEntries,
        std::logic_error, ": Internal logic error. Please contact Tpetra team.");
#endif // HAVE_TPETRA_DEBUG
    }
#ifdef HAVE_TPETRA_DEBUG
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(! isLocallyIndexed(), std::logic_error,
      ": At end of insertLocalValues(), this CrsMatrix is not locally indexed.  "
      "Please report this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG
  }


  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  insertGlobalValues (const GlobalOrdinal globalRow,
                      const Teuchos::ArrayView<const GlobalOrdinal>& indices,
                      const Teuchos::ArrayView<const Scalar>& values)
  {
    using Teuchos::Array;
    using Teuchos::ArrayView;
    using Teuchos::as;
    using Teuchos::toString;
    using std::endl;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    typedef typename Teuchos::ArrayView<const GO>::size_type size_type;
    const char tfecfFuncName[] = "insertGlobalValues: ";

#ifdef HAVE_TPETRA_DEBUG
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      values.size() != indices.size(), std::runtime_error,
      "values.size() must equal indices.size().  values.size() = "
      << values.size() << ", but indices.size() = " << indices.size() << ".");
#endif // HAVE_TPETRA_DEBUG

    const LO localRow = getRowMap ()->getLocalElement (globalRow);

    if (localRow == OTL::invalid ()) { // globalRow _not_ owned by calling process
      insertNonownedGlobalValues (globalRow, indices, values);
    }
    else { // globalRow _is_ owned by calling process
      if (this->isStaticGraph ()) {
        // Uh oh!  Not allowed to insert into owned rows in that case.
        std::ostringstream err;
        const int myRank = getRowMap ()->getComm ()->getRank ();
        const int numProcs = getRowMap ()->getComm ()->getSize ();

        err << "The matrix was constructed with a constant (\"static\") graph, "
          "yet the given global row index " << globalRow << " is in the row "
          "Map on the calling process (with rank " << myRank << ", of " <<
          numProcs << " process(es)).  In this case, you may not insert new "
          "entries into rows owned by the calling process.";

        if (! getRowMap ()->isNodeGlobalElement (globalRow)) {
          err << "  Furthermore, GID->LID conversion with the row Map claims that "
            "the global row index is owned on the calling process, yet "
            "getRowMap()->isNodeGlobalElement(globalRow) returns false.  That's"
            " weird!  This might indicate a Map bug.  Please report this to the"
            " Tpetra developers.";
        }
        TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
          this->isStaticGraph (), std::runtime_error, err.str ());
      }

      if (! myGraph_->indicesAreAllocated ()) {
        try {
          allocateValues (GlobalIndices, GraphNotYetAllocated);
        }
        catch (std::exception& e) {
          TEUCHOS_TEST_FOR_EXCEPTION(
            true, std::runtime_error, "Tpetra::CrsMatrix::insertGlobalValues: "
            "allocateValues(GlobalIndices,GraphNotYetAllocated) threw an "
            "exception: " << e.what ());
        }
      }

      const size_type numEntriesToInsert = indices.size ();
      // If the matrix has a column Map, check at this point whether
      // the column indices belong to the column Map.
      //
      // FIXME (mfh 16 May 2013) We may want to consider deferring the
      // test to the CrsGraph method, since it may have to do this
      // anyway.
      if (hasColMap ()) {
        const map_type& colMap = * (getColMap ());
        // In a debug build, keep track of the nonowned ("bad") column
        // indices, so that we can display them in the exception
        // message.  In a release build, just ditch the loop early if
        // we encounter a nonowned column index.
#ifdef HAVE_TPETRA_DEBUG
        Array<GO> badColInds;
#endif // HAVE_TPETRA_DEBUG
        bool allInColMap = true;
        for (size_type k = 0; k < numEntriesToInsert; ++k) {
          if (! colMap.isNodeGlobalElement (indices[k])) {
            allInColMap = false;
#ifdef HAVE_TPETRA_DEBUG
            badColInds.push_back (indices[k]);
#else
            break;
#endif // HAVE_TPETRA_DEBUG
          }
        }
        if (! allInColMap) {
          std::ostringstream os;
          os << "You attempted to insert entries in owned row " << globalRow
             << ", at the following column indices: " << toString (indices)
             << "." << endl;
#ifdef HAVE_TPETRA_DEBUG
          os << "Of those, the following indices are not in the column Map on "
            "this process: " << toString (badColInds) << "." << endl << "Since "
            "the matrix has a column Map already, it is invalid to insert "
            "entries at those locations.";
#else
          os << "At least one of those indices is not in the column Map on this "
            "process." << endl << "It is invalid to insert into columns not in "
            "the column Map on the process that owns the row.";
#endif // HAVE_TPETRA_DEBUG
          TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
            ! allInColMap, std::invalid_argument, os.str ());
        }
      }

      typename Graph::SLocalGlobalViews inds_view;
      ArrayView<const Scalar> vals_view;

      inds_view.ginds = indices;
      vals_view       = values;

#ifdef HAVE_TPETRA_DEBUG
      RowInfo rowInfo;
      try {
        rowInfo = myGraph_->getRowInfo (localRow);
      } catch (std::exception& e) {
        TEUCHOS_TEST_FOR_EXCEPTION(
          true, std::runtime_error, "myGraph_->getRowInfo(localRow=" << localRow
          << ") threw an exception: " << e.what ());
      }
#else
      RowInfo rowInfo = myGraph_->getRowInfo (localRow);
#endif // HAVE_TPETRA_DEBUG

      const size_t curNumEntries = rowInfo.numEntries;
      const size_t newNumEntries =
        curNumEntries + static_cast<size_t> (numEntriesToInsert);
      if (newNumEntries > rowInfo.allocSize) {
        TEUCHOS_TEST_FOR_EXCEPTION(
          getProfileType () == StaticProfile && newNumEntries > rowInfo.allocSize,
          std::runtime_error, "Tpetra::CrsMatrix::insertGlobalValues: new "
          "indices exceed statically allocated graph structure.  curNumEntries"
          " (" << curNumEntries << ") + numEntriesToInsert (" <<
          numEntriesToInsert << ") > allocSize (" << rowInfo.allocSize << ").");

        // Update allocation only as much as necessary
        try {
          rowInfo =
            myGraph_->template updateGlobalAllocAndValues<Scalar> (rowInfo,
                                                                   newNumEntries,
                                                                   values2D_[localRow]);
        } catch (std::exception& e) {
          TEUCHOS_TEST_FOR_EXCEPTION(
            true, std::runtime_error, "myGraph_->updateGlobalAllocAndValues"
            "(...) threw an exception: " << e.what ());
        }
      }
      try {
        if (isGloballyIndexed ()) {
          // lg=GlobalIndices, I=GlobalIndices means the method calls
          // getGlobalViewNonConst() and does direct copying, which
          // should be reasonably fast.
          myGraph_->template insertIndicesAndValues<Scalar> (rowInfo, inds_view,
                                                             this->getViewNonConst (rowInfo),
                                                             values,
                                                             GlobalIndices, GlobalIndices);
        }
        else {
          // lg=GlobalIndices, I=LocalIndices means the method calls
          // the Map's getLocalElement() method once per entry to
          // insert.  This may be slow.
          myGraph_->template insertIndicesAndValues<Scalar> (rowInfo, inds_view,
                                                             this->getViewNonConst (rowInfo),
                                                             values,
                                                             GlobalIndices, LocalIndices);
        }
      }
      catch (std::exception& e) {
        TEUCHOS_TEST_FOR_EXCEPTION(
          true, std::runtime_error, "myGraph_->insertIndicesAndValues(...) "
          "threw an exception: " << e.what ());
      }

#ifdef HAVE_TPETRA_DEBUG
      const size_t chkNewNumEntries = myGraph_->getNumEntriesInLocalRow (localRow);
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(chkNewNumEntries != newNumEntries,
        std::logic_error, ": There should be a total of " << newNumEntries
        << " entries in the row, but the graph now reports " << chkNewNumEntries
        << " entries.  Please report this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG
    }
  }


  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  insertGlobalValuesFiltered (const GlobalOrdinal globalRow,
                              const ArrayView<const GlobalOrdinal>& indices,
                              const ArrayView<const Scalar>& values)
  {
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    const char tfecfFuncName[] = "insertGlobalValuesFiltered";

    // mfh 14 Dec 2012: Defer test for static graph until we know that
    // globalRow is in the row Map.  If it's not in the row Map, it
    // doesn't matter whether or not the graph is static; the data
    // just get stashed for later use by globalAssemble().
    //
    // TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
    //   isStaticGraph(), std::runtime_error,
    //   ": matrix was constructed with static graph. Cannot insert new entries.");
#ifdef HAVE_TPETRA_DEBUG
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      values.size() != indices.size(), std::runtime_error,
      ": values.size() must equal indices.size().  values.size() = "
      << values.size() << ", but indices.size() = " << indices.size() << ".");
#endif // HAVE_TPETRA_DEBUG

    const LO lrow = getRowMap ()->getLocalElement (globalRow);

    if (lrow != Teuchos::OrdinalTraits<LO>::invalid ()) { // globalRow is in our row Map.
      // If the matrix has a static graph, this process is now allowed
      // to insert into rows it owns.
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
        this->isStaticGraph(), std::runtime_error,
        ": The CrsMatrix was constructed with a static graph.  In that case, "
        "it's forbidded to insert new entries into rows owned by the calling process.");
      if (! myGraph_->indicesAreAllocated ()) {
        allocateValues (GlobalIndices, GraphNotYetAllocated);
      }
      typename Graph::SLocalGlobalViews inds_view;
      ArrayView<const Scalar> vals_view;

      // We have to declare these Arrays here rather than in the
      // hasColMap() if branch, so that views to them will remain
      // valid for the whole scope.
      Array<GO> filtered_indices;
      Array<Scalar> filtered_values;
      if (hasColMap ()) { // We have a column Map.
        // Use column Map to filter the indices and corresponding
        // values, so that we only insert entries into columns we own.
        filtered_indices.assign (indices.begin (), indices.end ());
        filtered_values.assign (values.begin (), values.end ());
        const size_t numFilteredEntries =
          myGraph_->template filterGlobalIndicesAndValues<Scalar> (filtered_indices (),
                                                                   filtered_values ());
        inds_view.ginds = filtered_indices (0, numFilteredEntries);
        vals_view       = filtered_values (0, numFilteredEntries);
      }
      else { // we don't have a column Map.
        inds_view.ginds = indices;
        vals_view       = values;
      }
      const size_t numFilteredEntries = vals_view.size ();
      // add the new indices and values
      if (numFilteredEntries > 0) {
        RowInfo rowInfo = myGraph_->getRowInfo(lrow);
        const size_t curNumEntries = rowInfo.numEntries;
        const size_t newNumEntries = curNumEntries + numFilteredEntries;
        if (newNumEntries > rowInfo.allocSize) {
          TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
            getProfileType() == StaticProfile, std::runtime_error,
            ": new indices exceed statically allocated graph structure.");

          // Update allocation only as much as necessary
          rowInfo = myGraph_->template updateGlobalAllocAndValues<Scalar> (rowInfo, newNumEntries,
                                                                           values2D_[lrow]);
        }
        if (isGloballyIndexed ()) {
          // lg=GlobalIndices, I=GlobalIndices means the method calls
          // getGlobalViewNonConst() and does direct copying, which
          // should be reasonably fast.
          myGraph_->template insertIndicesAndValues<Scalar> (rowInfo, inds_view,
                                                             this->getViewNonConst (rowInfo),
                                                             vals_view,
                                                             GlobalIndices, GlobalIndices);
        }
        else {
          // lg=GlobalIndices, I=LocalIndices means the method calls
          // the Map's getLocalElement() method once per entry to
          // insert.  This may be slow.
          myGraph_->template insertIndicesAndValues<Scalar> (rowInfo, inds_view,
                                                             this->getViewNonConst (rowInfo),
                                                             vals_view,
                                                             GlobalIndices, LocalIndices);
        }
#ifdef HAVE_TPETRA_DEBUG
        {
          const size_t chkNewNumEntries = myGraph_->getNumEntriesInLocalRow(lrow);
          TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(chkNewNumEntries != newNumEntries,
            std::logic_error, ": There should be a total of " << newNumEntries
            << " entries in the row, but the graph now reports " << chkNewNumEntries
            << " entries.  Please report this bug to the Tpetra developers.");
        }
#endif // HAVE_TPETRA_DEBUG
      }
    }
    else { // The calling process doesn't own the given row.
      insertNonownedGlobalValues (globalRow, indices, values);
    }
  }


  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal,
           class DeviceType>
  LocalOrdinal
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal, Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  replaceLocalValues (const LocalOrdinal localRow,
                      const ArrayView<const LocalOrdinal> &indices,
                      const ArrayView<const Scalar>& values)
  {
    using Teuchos::Array;
    using Teuchos::ArrayView;
    // project2nd is a binary function that returns its second
    // argument.  This replaces entries in the given row with their
    // corresponding entry of values.
    typedef Tpetra::project2nd<Scalar, Scalar> f_type;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    typedef typename ArrayView<GO>::size_type size_type;

    if (! isFillActive ()) {
      // Fill must be active in order to call this method.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }
    else if (! this->hasColMap ()) {
      // There is no such thing as local column indices without a column Map.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }
    else if (values.size () != indices.size ()) {
      // The sizes of values and indices must match.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }
    const bool isLocalRow = getRowMap ()->isNodeLocalElement (localRow);
    if (! isLocalRow) {
      // The calling process doesn't own the local row, so we can't
      // insert into it.
      return static_cast<LO> (0);
    }

    if (indices.size () == 0) {
      return static_cast<LO> (0);
    }
    else {
      RowInfo rowInfo = staticGraph_->getRowInfo (localRow);
      ArrayView<Scalar> curVals = this->getViewNonConst (rowInfo);
      if (isLocallyIndexed ()) {
        return staticGraph_->template transformLocalValues<Scalar, f_type> (rowInfo, curVals,
                                                                            indices, values,
                                                                            f_type ());
      }
      else if (isGloballyIndexed ()) {
        // Convert the given local indices to global indices.
        //
        // FIXME (mfh 27 Jun 2014) Why can't we ask the graph to do
        // that?  It could do the conversions in place, so that we
        // wouldn't need temporary storage.
        const map_type& colMap = * (this->getColMap ());
        const size_type numInds = indices.size ();

        // mfh 27 Jun 2014: Some of the given local indices might be
        // invalid.  That's OK, though, since the graph ignores them
        // and their corresponding values in transformGlobalValues.
        // Thus, we don't have to count how many indices are valid.
        // We do so just as a sanity check.
        Array<GO> gblInds (numInds);
        size_type numValid = 0; // sanity check count of # valid indices
        for (size_type k = 0; k < numInds; ++k) {
          const GO gid = colMap.getGlobalElement (indices[k]);
          gblInds[k] = gid;
          if (gid != Teuchos::OrdinalTraits<GO>::invalid ()) {
            ++numValid; // sanity check count of # valid indices
          }
        }
        const LO numXformed =
          staticGraph_->template transformGlobalValues<Scalar, f_type> (rowInfo,
                                                                        curVals, // target
                                                                        gblInds,
                                                                        values, // source
                                                                        f_type ());
        if (static_cast<size_type> (numXformed) != numValid) {
          return Teuchos::OrdinalTraits<LO>::invalid ();
        } else {
          return numXformed;
        }
      }
      // NOTE (mfh 26 Jun 2014) In the current version of CrsMatrix,
      // it's possible for a matrix (or graph) to be neither locally
      // nor globally indexed on a process.  This means that the graph
      // or matrix has no entries on that process.  Epetra also works
      // like this.  It's related to lazy allocation (on first
      // insertion, not at graph / matrix construction).  Lazy
      // allocation will go away because it is not thread scalable.
      return static_cast<LO> (0);
    }
  }


  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal, class DeviceType>
  LocalOrdinal
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  replaceGlobalValues (GlobalOrdinal globalRow,
                       const ArrayView<const GlobalOrdinal> &indices,
                       const ArrayView<const Scalar>        &values)
  {
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    using Teuchos::Array;
    using Teuchos::ArrayView;
    typedef typename ArrayView<const GO>::size_type size_type;
    // project2nd is a binary function that returns its second
    // argument.  This replaces entries in the given row with their
    // corresponding entry of values.
    typedef Tpetra::project2nd<Scalar, Scalar> f_type;

    if (! isFillActive ()) {
      // Fill must be active in order to call this method.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }
    else if (values.size () != indices.size ()) {
      // The sizes of values and indices must match.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }

    const LO lrow = this->getRowMap()->getLocalElement (globalRow);
    if (lrow == Teuchos::OrdinalTraits<LO>::invalid ()) {
      // We don't own the row, so we're not allowed to modify its values.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }

    if (staticGraph_.is_null ()) {
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }
    const crs_graph_type& graph = *staticGraph_;
    RowInfo rowInfo = graph.getRowInfo (lrow);
    if (indices.size () == 0) {
      return static_cast<LO> (0);
    }
    else {
      ArrayView<Scalar> curVals = this->getViewNonConst (rowInfo);
      if (isLocallyIndexed ()) {
        // Convert the given global indices to local indices.
        //
        // FIXME (mfh 08 Jul 2014) Why can't we ask the graph to do
        // that?  It could do the conversions in place, so that we
        // wouldn't need temporary storage.
        const map_type& colMap = * (this->getColMap ());
        const size_type numInds = indices.size ();
        Array<LO> lclInds (numInds);
        for (size_type k = 0; k < numInds; ++k) {
          // There is no need to filter out indices not in the
          // column Map.  Those that aren't will be mapped to
          // invalid(), which the graph's transformGlobalValues()
          // will filter out (but not count in its return value).
          lclInds[k] = colMap.getLocalElement (indices[k]);
        }
        return graph.template transformLocalValues<Scalar, f_type> (rowInfo,
                                                                    curVals,
                                                                    lclInds (),
                                                                    values,
                                                                    f_type ());
      }
      else if (isGloballyIndexed ()) {
        return graph.template transformGlobalValues<Scalar, f_type> (rowInfo,
                                                                     curVals,
                                                                     indices,
                                                                     values,
                                                                     f_type ());
      }
      else {
        // If the graph is neither locally nor globally indexed on
        // the calling process, that means that the calling process
        // can't possibly have any entries in the owned row.  Thus,
        // there are no entries to transform, so we return zero.
        return static_cast<LO> (0);
      }
    }
  }


  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal, class DeviceType>
  LocalOrdinal
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  sumIntoGlobalValues (const GlobalOrdinal globalRow,
                       const ArrayView<const GlobalOrdinal> &indices,
                       const ArrayView<const Scalar>        &values)

  {
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    using Teuchos::Array;
    using Teuchos::ArrayView;
    typedef typename ArrayView<const GO>::size_type size_type;
    typedef std::plus<Scalar> f_type;

    if (! isFillActive ()) {
      // Fill must be active in order to call this method.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }
    else if (values.size () != indices.size ()) {
      // The sizes of values and indices must match.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }

    const LO lrow = this->getRowMap()->getLocalElement (globalRow);
    if (lrow == Teuchos::OrdinalTraits<LO>::invalid ()) {
      // globalRow is not in the row Map, so stash the given entries
      // away in a separate data structure.  globalAssemble() (called
      // during fillComplete()) will exchange that data and sum it in
      // using sumIntoGlobalValues().
      this->insertNonownedGlobalValues (globalRow, indices, values);
      // FIXME (mfh 08 Jul 2014) It's not clear what to return here,
      // since we won't know whether the given indices were valid
      // until globalAssemble (called in fillComplete) is called.
      // That's why insertNonownedGlobalValues doesn't return
      // anything.  Just for consistency, I'll return the number of
      // entries that the user gave us.
      return static_cast<LO> (indices.size ());
    }

    if (staticGraph_.is_null ()) {
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }
    const crs_graph_type& graph = *staticGraph_;
    RowInfo rowInfo = graph.getRowInfo (lrow);
    if (indices.size () == 0) {
      return static_cast<LO> (0);
    }
    else {
      ArrayView<Scalar> curVals = this->getViewNonConst (rowInfo);
      if (isLocallyIndexed ()) {
        // Convert the given global indices to local indices.
        //
        // FIXME (mfh 08 Jul 2014) Why can't we ask the graph to do
        // that?  It could do the conversions in place, so that we
        // wouldn't need temporary storage.
        const map_type& colMap = * (this->getColMap ());
        const size_type numInds = indices.size ();
        Array<LO> lclInds (numInds);
        for (size_type k = 0; k < numInds; ++k) {
          // There is no need to filter out indices not in the
          // column Map.  Those that aren't will be mapped to
          // invalid(), which the graph's transformGlobalValues()
          // will filter out (but not count in its return value).
          lclInds[k] = colMap.getLocalElement (indices[k]);
        }
        return graph.template transformLocalValues<Scalar, f_type> (rowInfo,
                                                                    curVals,
                                                                    lclInds (),
                                                                    values,
                                                                    f_type ());
      }
      else if (isGloballyIndexed ()) {
        return graph.template transformGlobalValues<Scalar, f_type> (rowInfo,
                                                                     curVals,
                                                                     indices,
                                                                     values,
                                                                     f_type ());
      }
      else {
        // If the graph is neither locally nor globally indexed on
        // the calling process, that means that the calling process
        // can't possibly have any entries in the owned row.  Thus,
        // there are no entries to transform, so we return zero.
        return static_cast<LO> (0);
      }
    }
  }


  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  LocalOrdinal
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  sumIntoLocalValues (const LocalOrdinal localRow,
                      const ArrayView<const LocalOrdinal>& indices,
                      const ArrayView<const Scalar>& values)
  {
    using Teuchos::Array;
    using Teuchos::ArrayView;
    typedef std::plus<Scalar> f_type;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    typedef typename ArrayView<GO>::size_type size_type;

    if (! isFillActive ()) {
      // Fill must be active in order to call this method.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }
    else if (! this->hasColMap ()) {
      // There is no such thing as local column indices without a column Map.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }
    else if (values.size () != indices.size ()) {
      // The sizes of values and indices must match.
      return Teuchos::OrdinalTraits<LO>::invalid ();
    }
    const bool isLocalRow = getRowMap ()->isNodeLocalElement (localRow);
    if (! isLocalRow) {
      // The calling process doesn't own the local row, so we can't
      // insert into it.
      return static_cast<LO> (0);
    }

    if (indices.size () == 0) {
      return static_cast<LO> (0);
    }
    else {
      RowInfo rowInfo = staticGraph_->getRowInfo (localRow);
      ArrayView<Scalar> curVals = this->getViewNonConst (rowInfo);
      if (isLocallyIndexed ()) {
        return staticGraph_->template transformLocalValues<Scalar, f_type> (rowInfo, curVals,
                                                                            indices, values,
                                                                            f_type ());
      }
      else if (isGloballyIndexed ()) {
        // Convert the given local indices to global indices.
        //
        // FIXME (mfh 27 Jun 2014) Why can't we ask the graph to do
        // that?  It could do the conversions in place, so that we
        // wouldn't need temporary storage.
        const map_type& colMap = * (this->getColMap ());
        const size_type numInds = indices.size ();

        // mfh 27 Jun 2014: Some of the given local indices might be
        // invalid.  That's OK, though, since the graph ignores them
        // and their corresponding values in transformGlobalValues.
        // Thus, we don't have to count how many indices are valid.
        // We do so just as a sanity check.
        Array<GO> gblInds (numInds);
        size_type numValid = 0; // sanity check count of # valid indices
        for (size_type k = 0; k < numInds; ++k) {
          const GO gid = colMap.getGlobalElement (indices[k]);
          gblInds[k] = gid;
          if (gid != Teuchos::OrdinalTraits<GO>::invalid ()) {
            ++numValid; // sanity check count of # valid indices
          }
        }
        const LO numXformed =
          staticGraph_->template transformGlobalValues<Scalar, f_type> (rowInfo,
                                                                        curVals, // target
                                                                        gblInds,
                                                                        values, // source
                                                                        f_type ());
        if (static_cast<size_type> (numXformed) != numValid) {
          return Teuchos::OrdinalTraits<LO>::invalid ();
        } else {
          return numXformed;
        }
      }
      // NOTE (mfh 26 Jun 2014) In the current version of CrsMatrix,
      // it's possible for a matrix (or graph) to be neither locally
      // nor globally indexed on a process.  This means that the graph
      // or matrix has no entries on that process.  Epetra also works
      // like this.  It's related to lazy allocation (on first
      // insertion, not at graph / matrix construction).  Lazy
      // allocation will go away because it is not thread scalable.
      return static_cast<LO> (0);
    }
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  ArrayView<const Scalar>
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getView (RowInfo rowinfo) const
  {
    if (values1D_ != null && rowinfo.allocSize > 0) {
#ifdef HAVE_TPETRA_DEBUG
      TEUCHOS_TEST_FOR_EXCEPTION(
        rowinfo.offset1D + rowinfo.allocSize > values1D_.size (),
        std::range_error, "Tpetra::CrsMatrix::getView: Invalid access "
        "to 1-D storage of values." << std::endl << "rowinfo.offset1D (" <<
        rowinfo.offset1D << ") + rowinfo.allocSize (" << rowinfo.allocSize <<
        ") > values1D_.size() (" << values1D_.size () << ").");
#endif // HAVE_TPETRA_DEBUG
      return values1D_(rowinfo.offset1D,rowinfo.allocSize);
    }
    else if (values2D_ != null) {
      return values2D_[rowinfo.localRow]();
    }
    else {
      return ArrayView<Scalar> ();
    }
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  ArrayView<Scalar>
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getViewNonConst (RowInfo rowinfo)
  {
    if (values1D_ != null && rowinfo.allocSize > 0) {
#ifdef HAVE_TPETRA_DEBUG
      TEUCHOS_TEST_FOR_EXCEPTION(
        rowinfo.offset1D + rowinfo.allocSize > values1D_.size (),
        std::range_error, "Tpetra::CrsMatrix::getViewNonConst: Invalid access "
        "to 1-D storage of values." << std::endl << "rowinfo.offset1D (" <<
        rowinfo.offset1D << ") + rowinfo.allocSize (" << rowinfo.allocSize <<
        ") > values1D_.size() (" << values1D_.size () << ").");
#endif // HAVE_TPETRA_DEBUG
      return values1D_(rowinfo.offset1D,rowinfo.allocSize);
    }
    else if (values2D_ != null) {
      return values2D_[rowinfo.localRow]();
    }
    else {
      return ArrayView<Scalar> ();
    }
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getLocalRowCopy(
                                LocalOrdinal localRow,
                                const ArrayView<LocalOrdinal> &indices,
                                const ArrayView<Scalar>       &values,
                                size_t& numEntries) const
  {
    using Teuchos::ArrayView;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;

    TEUCHOS_TEST_FOR_EXCEPTION(
      isGloballyIndexed () && ! hasColMap (), std::runtime_error,
      "Tpetra::CrsMatrix::getLocalRowCopy: The matrix is globally indexed and "
      "does not have a column Map yet.  That means we don't have local indices "
      "for columns yet, so it doesn't make sense to call this method.  If the "
      "matrix doesn't have a column Map yet, you should call fillComplete on "
      "it first.");
    TEUCHOS_TEST_FOR_EXCEPTION(
      ! staticGraph_->hasRowInfo (), std::runtime_error,
      "Tpetra::CrsMatrix::getLocalRowCopy: The graph's row information was "
      "deleted at fillComplete().");

    if (! this->getRowMap ()->isNodeLocalElement (localRow)) {
      numEntries = 0;
      return;
    }

    const RowInfo rowinfo = staticGraph_->getRowInfo(localRow);
    const size_t theNumEntries = rowinfo.numEntries;

    TEUCHOS_TEST_FOR_EXCEPTION(
      static_cast<size_t> (indices.size ()) < theNumEntries ||
      static_cast<size_t> (values.size ()) < theNumEntries,
      std::runtime_error,
      "Tpetra::CrsMatrix::getLocalRowCopy: The given row " << localRow
      << " has " << theNumEntries << " entries.  One or both of the given "
      "ArrayViews are not long enough to store that many entries.  indices "
      "can store " << indices.size() << " entries and values can store "
      << values.size() << " entries.");

    numEntries = theNumEntries;

    if (staticGraph_->isLocallyIndexed ()) {
      ArrayView<const LO> indrowview = staticGraph_->getLocalView (rowinfo);
      ArrayView<const Scalar> valrowview = getView (rowinfo);
      std::copy (indrowview.begin (), indrowview.begin () + numEntries, indices.begin ());
      std::copy (valrowview.begin (), valrowview.begin () + numEntries,  values.begin ());
    }
    else if (staticGraph_->isGloballyIndexed ()) {
      ArrayView<const GO> indrowview = staticGraph_->getGlobalView (rowinfo);
      ArrayView<const Scalar>        valrowview = getView (rowinfo);
      std::copy (valrowview.begin (), valrowview.begin () + numEntries, values.begin ());

      const map_type& colMap = * (this->getColMap ());
      for (size_t j=0; j < numEntries; ++j) {
        indices[j] = colMap.getLocalElement (indrowview[j]);
      }
    }
    else {
      numEntries = 0;
    }
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getGlobalRowCopy(
                                GlobalOrdinal globalRow,
                                const ArrayView<GlobalOrdinal> &indices,
                                const ArrayView<Scalar>        &values,
                                size_t &numEntries) const
  {
    // Only locally owned rows can be queried, otherwise complain
    const char tfecfFuncName[] = "getGlobalRowCopy()";
    const LocalOrdinal lrow = getRowMap ()->getLocalElement (globalRow);
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      lrow == OTL::invalid(), std::runtime_error,
      ": globalRow=" << globalRow << " does not belong to the calling process "
      << getComm()->getRank() << ".");

    const RowInfo rowinfo = staticGraph_->getRowInfo (lrow);
    numEntries = rowinfo.numEntries;
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      static_cast<size_t> (indices.size ()) < numEntries || static_cast<size_t> (values.size ()) < numEntries,
      std::runtime_error,
      ": size of indices,values must be sufficient to store the specified row.");

    if (staticGraph_->isGloballyIndexed ()) {
      ArrayView<const GlobalOrdinal> indrowview = staticGraph_->getGlobalView(rowinfo);
      ArrayView<const Scalar>        valrowview = getView(rowinfo);
      std::copy( indrowview.begin(), indrowview.begin() + numEntries, indices.begin() );
      std::copy( valrowview.begin(), valrowview.begin() + numEntries,  values.begin() );
    }
    else if (staticGraph_->isLocallyIndexed ()) {
      ArrayView<const LocalOrdinal> indrowview = staticGraph_->getLocalView(rowinfo);
      ArrayView<const Scalar>       valrowview = getView(rowinfo);
      std::copy( valrowview.begin(), valrowview.begin() + numEntries, values.begin() );
      for (size_t j=0; j < numEntries; ++j) {
        indices[j] = getColMap ()->getGlobalElement (indrowview[j]);
      }
    }
    else {
#ifdef HAVE_TPETRA_DEBUG
      // should have fallen in one of the above if indices are allocated
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
        staticGraph_->indicesAreAllocated (), std::logic_error,
        ": Internal logic error. Please contact Tpetra team.");
#endif // HAVE_TPETRA_DEBUG
      numEntries = 0;
    }
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::getLocalRowView(
                                LocalOrdinal localRow,
                                ArrayView<const LocalOrdinal> &indices,
                                ArrayView<const Scalar>       &values) const
  {
    const char tfecfFuncName[] = "getLocalRowView()";
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(isGloballyIndexed() == true, std::runtime_error, ": local indices cannot be provided.");
    indices = null;
    values  = null;
    if (getRowMap ()->isNodeLocalElement (localRow)) {
      const RowInfo rowinfo = staticGraph_->getRowInfo(localRow);
      if (rowinfo.numEntries > 0) {
        indices = staticGraph_->getLocalView(rowinfo);
        indices = indices(0,rowinfo.numEntries);
        values  = getView(rowinfo);
        values  = values(0,rowinfo.numEntries);
      }
    }
#ifdef HAVE_TPETRA_DEBUG
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC( (size_t)indices.size() != getNumEntriesInLocalRow(localRow) || indices.size() != values.size(),
        std::logic_error, ": Violated stated post-conditions. Please contact Tpetra team.");
#endif
    return;
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getGlobalRowView (GlobalOrdinal globalRow,
                    Teuchos::ArrayView<const GlobalOrdinal>& indices,
                    Teuchos::ArrayView<const Scalar>& values) const
  {
    const char tfecfFuncName[] = "getGlobalRowView";

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      isLocallyIndexed (), std::runtime_error,
      ": The matrix is locally indexed, so we cannot return a view of the row "
      "with global column indices.  Use getGlobalRowCopy() instead.");
    indices = null;
    values  = null;
    const LocalOrdinal lrow = getRowMap ()->getLocalElement (globalRow);
    if (lrow != Teuchos::OrdinalTraits<LocalOrdinal>::invalid ()) {
      // getRowInfo() requires a local row index, whether or not
      // storage has been optimized.
      const RowInfo rowinfo = staticGraph_->getRowInfo(lrow);
      if (rowinfo.numEntries > 0) {
        indices = staticGraph_->getGlobalView (rowinfo);
        indices = indices (0, rowinfo.numEntries);
        values  = getView (rowinfo);
        values  = values (0, rowinfo.numEntries);
      }
    }
#ifdef HAVE_TPETRA_DEBUG
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      static_cast<size_t> (indices.size ()) != getNumEntriesInGlobalRow (globalRow) ||
      indices.size () != values.size (),
      std::logic_error,
      ": Violated stated post-conditions. Please contact Tpetra team.");
#endif // HAVE_TPETRA_DEBUG
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  scale (const Scalar& alpha)
  {
    typedef LocalOrdinal LO;
    typedef Kokkos::SparseRowView<k_local_matrix_type> row_view_type;
    typedef typename Teuchos::Array<Scalar>::size_type size_type;
    const char tfecfFuncName[] = "scale";

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! isFillActive (), std::runtime_error,
      ": Fill must be active before you may call this method.");

    const size_t nlrs = staticGraph_->getNodeNumRows ();
    const size_t numAlloc = staticGraph_->getNodeAllocationSize ();
    const size_t numEntries = staticGraph_->getNodeNumEntries ();
    if (! staticGraph_->indicesAreAllocated () || nlrs == 0 ||
        numAlloc == 0 || numEntries == 0) {
      // do nothing
    }
    else {
      if (staticGraph_->getProfileType () == StaticProfile) {
        const LO lclNumRows = k_lclMatrix_.numRows ();
        for (LO lclRow = 0; lclRow < lclNumRows; ++lclRow) {
          row_view_type row_i = k_lclMatrix_.row (lclRow);
          for (LO k = 0; k < row_i.length; ++k) {
            row_i.value (k) *= alpha;
          }
        }
      }
      else if (staticGraph_->getProfileType () == DynamicProfile) {
        for (size_t row = 0; row < nlrs; ++row) {
          const size_type numEnt = getNumEntriesInLocalRow (row);
          Teuchos::ArrayView<Scalar> rowVals = values2D_[row] ();
          for (size_type k = 0; k < numEnt; ++k) {
            rowVals[k] *= alpha;
          }
        }
      }
    }
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::setAllToScalar(const Scalar &alpha)
  {
    const char tfecfFuncName[] = "setAllToScalar()";
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC( isFillActive() == false, std::runtime_error, " requires that fill is active.");
    // replace all values in the matrix
    // it is easiest to replace all allocated values, instead of replacing only the ones with valid entries
    // however, if there are no valid entries, we can short-circuit
    // furthermore, if the values aren't allocated, we can short-circuit (no entry have been inserted so far)
    const size_t     nlrs = staticGraph_->getNodeNumRows(),
                 numAlloc = staticGraph_->getNodeAllocationSize(),
               numEntries = staticGraph_->getNodeNumEntries();
    if (staticGraph_->indicesAreAllocated() == false || numAlloc == 0 || numEntries == 0) {
      // do nothing
    }
    else {
      if (staticGraph_->getProfileType() == StaticProfile) {
        std::fill( values1D_.begin(), values1D_.end(), alpha );
      }
      else if (staticGraph_->getProfileType() == DynamicProfile) {
        for (size_t row=0; row < nlrs; ++row) {
          std::fill( values2D_[row].begin(), values2D_[row].end(), alpha );
        }
      }
    }
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal,
            class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  setAllValues (const t_RowPtrs& rowPointers,
                const t_LocalOrdinal_1D& columnIndices,
                const t_ValuesType& values)
  {
    const char tfecfFuncName[] = "setAllValues";
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      columnIndices.size () != values.size (), std::runtime_error,
      ": columnIndices and values must have the same size.  columnIndices.size() = "
      << columnIndices.size () << " != values.size() = " << values.size () << ".");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      myGraph_.is_null (), std::runtime_error, ": myGraph_ must not be null.");

    try {
      myGraph_->setAllIndices (rowPointers, columnIndices);
    }
    catch (std::exception &e) {
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
        true, std::runtime_error, ": Caught exception while calling myGraph_->"
        "setAllIndices(): " << e.what ());
    }
    k_values1D_ = values;
    values1D_ = Kokkos::Compat::persistingView (k_values1D_);
    checkInternalState();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  setAllValues (const Teuchos::ArrayRCP<size_t>& rowPointers,
                const Teuchos::ArrayRCP<LocalOrdinal>& columnIndices,
                const Teuchos::ArrayRCP<Scalar>& values)
  {
    const char tfecfFuncName[] = "setAllValues";
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      columnIndices.size () != values.size (), std::runtime_error,
      ": columnIndices and values must have the same size.  columnIndices.size() = "
      << columnIndices.size () << " != values.size() = " << values.size () << ".");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      myGraph_.is_null (), std::runtime_error, ": myGraph_ must not be null.");

    try {
      myGraph_->setAllIndices (rowPointers, columnIndices);
    }
    catch (std::exception &e) {
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
        true, std::runtime_error, ": Caught exception while calling myGraph_->"
        "setAllIndices(): " << e.what ());
    }
    k_values1D_ = Kokkos::Compat::getKokkosViewDeepCopy<DeviceType> (values ());
    values1D_ = Kokkos::Compat::persistingView (k_values1D_);
    checkInternalState();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getLocalDiagOffsets (Teuchos::ArrayRCP<size_t>& offsets) const
  {
    using Teuchos::ArrayRCP;
    using Teuchos::ArrayView;
    const char tfecfFuncName[] = "getLocalDiagOffsets";

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! hasColMap (), std::runtime_error,
      ": This method requires that the matrix have a column Map.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      staticGraph_.is_null (), std::runtime_error,
      ": This method requires that the matrix have a graph.");

    const map_type& rowMap = * (this->getRowMap ());
    const map_type& colMap = * (this->getColMap ());

    const size_t myNumRows = getNodeNumRows ();
    if (static_cast<size_t> (offsets.size ()) != myNumRows) {
      offsets.resize (static_cast<size_t> (myNumRows));
    }

#ifdef HAVE_TPETRA_DEBUG
    bool allRowMapDiagEntriesInColMap = true;
    bool allDiagEntriesFound = true;
#endif // HAVE_TPETRA_DEBUG

    for (size_t r = 0; r < myNumRows; ++r) {
      const GlobalOrdinal rgid = rowMap.getGlobalElement (r);
      const LocalOrdinal rlid = colMap.getLocalElement (rgid);

#ifdef HAVE_TPETRA_DEBUG
      if (rlid == Teuchos::OrdinalTraits<LocalOrdinal>::invalid ()) {
        allRowMapDiagEntriesInColMap = false;
      }
#endif // HAVE_TPETRA_DEBUG

      if (rlid != Teuchos::OrdinalTraits<LocalOrdinal>::invalid ()) {
        RowInfo rowinfo = staticGraph_->getRowInfo (r);
        if (rowinfo.numEntries > 0) {
          offsets[r] = staticGraph_->findLocalIndex (rowinfo, rlid);
        }
        else {
          offsets[r] = Teuchos::OrdinalTraits<size_t>::invalid ();
#ifdef HAVE_TPETRA_DEBUG
          allDiagEntriesFound = false;
#endif // HAVE_TPETRA_DEBUG
        }
      }
    }

#ifdef HAVE_TPETRA_DEBUG
    using Teuchos::reduceAll;
    using std::endl;

    const bool localSuccess =
      allRowMapDiagEntriesInColMap && allDiagEntriesFound;
    int localResults[3];
    localResults[0] = allRowMapDiagEntriesInColMap ? 1 : 0;
    localResults[1] = allDiagEntriesFound ? 1 : 0;
    // min-all-reduce will compute least rank of all the processes
    // that didn't succeed.
    localResults[2] =
      ! localSuccess ? getComm ()->getRank () : getComm ()->getSize ();
    int globalResults[3];
    globalResults[0] = 0;
    globalResults[1] = 0;
    globalResults[2] = 0;
    reduceAll<int, int> (* (getComm ()), Teuchos::REDUCE_MIN,
                         3, localResults, globalResults);
    if (globalResults[0] == 0 || globalResults[1] == 0) {
      std::ostringstream os; // build error message
      const bool both =
        globalResults[0] == 0 && globalResults[1] == 0;
      os << ": At least one process (including Process " << globalResults[2]
         << ") had the following issue" << (both ? "s" : "") << ":" << endl;
      if (globalResults[0] == 0) {
        os << "  - The column Map does not contain at least one diagonal entry "
          "of the matrix." << endl;
      }
      if (globalResults[1] == 0) {
        os << "  - There is a row on that / those process(es) that does not "
          "contain a diagonal entry." << endl;
      }
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(true, std::runtime_error, os.str());
    }
#endif // HAVE_TPETRA_DEBUG
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getLocalDiagCopy (Vector<Scalar, LocalOrdinal, GlobalOrdinal, node_type>& dvec) const
  {
    using Teuchos::ArrayRCP;
    using Teuchos::ArrayView;
    const char tfecfFuncName[] = "getLocalDiagCopy";
    typedef Vector<Scalar, LocalOrdinal, GlobalOrdinal, node_type> vec_type;
    typedef typename vec_type::dual_view_type dual_view_type;
    typedef typename device_type::host_mirror_device_type host_device_type;

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! hasColMap (), std::runtime_error,
      ": This method requires that the matrix have a column Map.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      staticGraph_.is_null (), std::runtime_error,
      ": This method requires that the matrix have a graph.");

    const map_type& rowMap = * (this->getRowMap ());
    const map_type& colMap = * (this->getColMap ());

#ifdef HAVE_TPETRA_DEBUG
    // isCompatible() requires an all-reduce, and thus this check
    // should only be done in debug mode.
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! dvec.getMap ()->isCompatible (rowMap), std::runtime_error,
      ": The input Vector's Map must be compatible with the CrsMatrix's row "
      "Map.  You may check this by using Map's isCompatible method: "
      "dvec.getMap ()->isCompatible (A.getRowMap ());");
#endif // HAVE_TPETRA_DEBUG

    // For now, we fill the Vector on the host and sync to device.
    // Later, we may write a parallel kernel that works entirely on
    // device.
    dual_view_type lclVec = dvec.getDualView ();
    typedef typename dual_view_type::t_host host_view_type;
    host_view_type lclVecHost = lclVec.h_view;
    lclVec.template modify<host_device_type> ();

    // 1-D subview of lclVecHost.  All the "typename" stuff ensures
    // that we get the same layout and memory traits as the original
    // 2-D view.
    typedef typename Kokkos::View<scalar_type*,
      typename host_view_type::array_layout, device_type,
      typename host_view_type::memory_traits>::HostMirror
      host_view_1d_type;
    host_view_1d_type lclVecHost1d =
      Kokkos::subview<host_view_1d_type> (lclVecHost, Kokkos::ALL (), 0);

    // Find the diagonal entries and put them in lclVecHost1d.
    const size_t myNumRows = getNodeNumRows ();
    for (size_t r = 0; r < myNumRows; ++r) {
      lclVecHost1d(r) = STS::zero (); // default value if no diag entry
      const GlobalOrdinal rgid = rowMap.getGlobalElement (r);
      const LocalOrdinal rlid = colMap.getLocalElement (rgid);

      if (rlid != Teuchos::OrdinalTraits<LocalOrdinal>::invalid ()) {
        RowInfo rowinfo = staticGraph_->getRowInfo (r);
        if (rowinfo.numEntries > 0) {
          const size_t j = staticGraph_->findLocalIndex (rowinfo, rlid);
          if (j != Teuchos::OrdinalTraits<size_t>::invalid ()) {
            ArrayView<const Scalar> view = this->getView (rowinfo);
            lclVecHost1d(r) = view[j];
          }
        }
      }
    }
    lclVec.template sync<device_type> (); // sync changes back to device
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getLocalDiagCopy (Vector<Scalar, LocalOrdinal, GlobalOrdinal, node_type>& diag,
                    const Teuchos::ArrayView<const size_t>& offsets) const
  {
    using Teuchos::ArrayRCP;
    using Teuchos::ArrayView;
    typedef Vector<Scalar, LocalOrdinal, GlobalOrdinal, node_type> vec_type;
    typedef typename vec_type::dual_view_type dual_view_type;
    typedef typename device_type::host_mirror_device_type host_device_type;

#ifdef HAVE_TPETRA_DEBUG
    const char tfecfFuncName[] = "getLocalDiagCopy";
    const map_type& rowMap = * (this->getRowMap ());
    // isCompatible() requires an all-reduce, and thus this check
    // should only be done in debug mode.
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! diag.getMap ()->isCompatible (rowMap), std::runtime_error,
      ": The input Vector's Map must be compatible with (in the sense of Map::"
      "isCompatible) the CrsMatrix's row Map.");
#endif // HAVE_TPETRA_DEBUG

    // For now, we fill the Vector on the host and sync to device.
    // Later, we may write a parallel kernel that works entirely on
    // device.
    dual_view_type lclVec = diag.getDualView ();
    typedef typename dual_view_type::t_host host_view_type;
    host_view_type lclVecHost = lclVec.h_view;
    lclVec.template modify<host_device_type> ();

    // 1-D subview of lclVecHost.  All the "typename" stuff ensures
    // that we get the same layout and memory traits as the original
    // 2-D view.
    typedef typename Kokkos::View<scalar_type*,
      typename host_view_type::array_layout, device_type,
      typename host_view_type::memory_traits>::HostMirror
      host_view_1d_type;
    host_view_1d_type lclVecHost1d =
      Kokkos::subview<host_view_1d_type> (lclVecHost, Kokkos::ALL (), 0);

    // Find the diagonal entries and put them in lclVecHost1d.
    const size_t myNumRows = getNodeNumRows ();
    for (size_t i = 0; i < myNumRows; ++i) {
      lclVecHost1d(i) = STS::zero (); // default value if no diag entry
      if (offsets[i] != Teuchos::OrdinalTraits<size_t>::invalid ()) {
        ArrayView<const LocalOrdinal> ind;
        ArrayView<const Scalar> val;
        this->getLocalRowView (i, ind, val);
        lclVecHost1d(i) = val[offsets[i]];
      }
    }
    lclVec.template sync<device_type> (); // sync changes back to device
  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::leftScale(
    const Vector<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >& x)
  {
    using Teuchos::ArrayRCP;
    using Teuchos::ArrayView;
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    using Teuchos::rcpFromRef;
    typedef Vector<Scalar, LocalOrdinal, GlobalOrdinal, node_type> vec_type;
    const char tfecfFuncName[] = "leftScale";

    // FIXME (mfh 06 Aug 2014) This doesn't make sense.  The matrix
    // should only be modified when it is not fill complete.
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! isFillComplete (), std::runtime_error,
      ": matrix must be fill complete.");
    RCP<const vec_type> xp;

    if (getRangeMap ()->isSameAs (* (x.getMap ()))){
      // Take from Epetra: If we have a non-trivial exporter, we must
      // import elements that are permuted or are on other processors.
      // (We will use the exporter to perform the import ("reverse
      // mode").)
      if (getCrsGraph ()->getExporter () != null) {
        RCP<vec_type> tempVec = rcp (new vec_type (getRowMap ()));
        tempVec->doImport (x, * (getCrsGraph ()->getExporter ()), INSERT);
        xp = tempVec;
      }
      else {
        xp = rcpFromRef (x);
      }
    }
    else if (getRowMap ()->isSameAs (* (x.getMap ()))) {
      xp = rcpFromRef (x);
    }
    else {
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(true, std::invalid_argument, ": The "
        "input scaling vector x's Map must be the same as either the row Map or "
        "the range Map of the CrsMatrix.");
    }
    ArrayRCP<const Scalar> vectorVals = xp->getData(0);
    ArrayView<Scalar> rowValues = null;

    const size_t lclNumRows = this->getNodeNumRows ();
    for (size_t i = 0; i < lclNumRows; ++i) {
      const RowInfo rowinfo = staticGraph_->getRowInfo (static_cast<LocalOrdinal> (i));
      rowValues = this->getViewNonConst (rowinfo);
      const Scalar scaleValue = vectorVals[i];
      for (size_t j = 0; j < rowinfo.numEntries; ++j) {
        rowValues[j] *= scaleValue;
      }
      rowValues = null;
    }
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::rightScale(
    const Vector<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >& x)
  {
    using Teuchos::ArrayRCP;
    using Teuchos::ArrayView;
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    using Teuchos::rcpFromRef;
    typedef Vector<Scalar, LocalOrdinal, GlobalOrdinal, node_type> vec_type;
    const char tfecfFuncName[] = "rightScale";

    // FIXME (mfh 06 Aug 2014) This doesn't make sense.  The matrix
    // should only be modified when it is not fill complete.
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! isFillComplete (), std::runtime_error, ": matrix must be fill complete.");
    RCP<const vec_type> xp;
    if (getDomainMap ()->isSameAs (* (x.getMap ()))) {
      // Take from Epetra: If we have a non-trivial exporter, we must
      // import elements that are permuted or are on other processors.
      // (We will use the exporter to perform the import.)
      if (getCrsGraph ()->getImporter () != null) {
        RCP<vec_type> tempVec = rcp (new vec_type (getColMap ()));
        tempVec->doImport (x, * (getCrsGraph ()->getImporter ()), INSERT);
        xp = tempVec;
      }
      else {
        xp = rcpFromRef (x);
      }
    }
    else if (getRowMap ()->isSameAs (* (x.getMap ()))) {
      xp = rcpFromRef (x);
    }
    else {
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
        true, std::runtime_error, ": The vector x must be the same as either "
        "the row map or the range map");
    }

    ArrayRCP<const Scalar> vectorVals = xp->getData(0);
    ArrayView<Scalar> rowValues = null;

    const size_t lclNumRows = this->getNodeNumRows ();
    for (size_t i = 0; i < lclNumRows; ++i) {
      const RowInfo rowinfo = staticGraph_->getRowInfo (static_cast<LocalOrdinal> (i));
      rowValues = this->getViewNonConst (rowinfo);
      ArrayView<const LocalOrdinal> colInds;
      getCrsGraph ()->getLocalRowView (static_cast<LocalOrdinal> (i), colInds);
      for (size_t j = 0; j < rowinfo.numEntries; ++j) {
        rowValues[j] *= vectorVals[colInds[j]];
      }
    }
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  typename ScalarTraits<Scalar>::magnitudeType
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getFrobeniusNorm () const
  {
    using Teuchos::as;
    using Teuchos::outArg;
    using Teuchos::REDUCE_SUM;
    using Teuchos::reduceAll;
    typedef typename ArrayRCP<const Scalar>::size_type size_type;

    // FIXME (mfh 05 Aug 2014) Write a thread-parallel kernel for the
    // local part of this computation.  It could make sense to put
    // this operation in the Kokkos::CrsMatrix.

    // check the cache first
    Magnitude frobNorm = frobNorm_;
    if (frobNorm == -STM::one ()) {
      Magnitude mySum = STM::zero ();
      if (getNodeNumEntries() > 0) {
        if (isStorageOptimized ()) {
          // "Optimized" storage is packed storage.  That means we can
          // iterate in one pass through the 1-D values array.
          const size_type numEntries =
            static_cast<size_type> (getNodeNumEntries ());
          for (size_type k = 0; k < numEntries; ++k) {
            // FIXME (mfh 05 Aug 2014) This assumes UVM.
            const Scalar val = k_values1D_(k);
            mySum += STS::real (val) * STS::real (val) +
              STS::imag (val) * STS::imag (val);
          }
        }
        else {
          const size_t numRows = getNodeNumRows ();
          for (size_t r = 0; r < numRows; ++r) {
            RowInfo rowInfo = myGraph_->getRowInfo (r);
            const size_type numEntries =
              static_cast<size_type> (rowInfo.numEntries);
            ArrayView<const Scalar> A_r =
              this->getView (rowInfo).view (0, numEntries);
            for (size_type k = 0; k < numEntries; ++k) {
              const Scalar val = A_r[k];
              mySum += STS::real (val) * STS::real (val) +
                STS::imag (val) * STS::imag (val);
            }
          }
        }
      }
      Magnitude totalSum;
      reduceAll<int, Magnitude> (* (getComm ()), REDUCE_SUM,
                                 mySum, outArg (totalSum));
      frobNorm = STM::squareroot (totalSum);
    }
    if (isFillComplete ()) {
      // Only cache the result if the matrix is fill complete.
      // Otherwise, the values might still change.  resumeFill clears
      // the cache.
      frobNorm_ = frobNorm;
    }
    return frobNorm;
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar,
    LocalOrdinal,
    GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar,
      LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  replaceColMap (const Teuchos::RCP<const map_type>& newColMap)
  {
    const char tfecfFuncName[] = "replaceColMap";
    // FIXME (mfh 06 Aug 2014) What if the graph is locally indexed?
    // Then replacing the column Map might mean that we need to
    // reindex the column indices.
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      myGraph_.is_null (), std::runtime_error,
      ": This method does not work if the matrix has a const graph.  The whole "
      "idea of a const graph is that you are not allowed to change it, but this"
      " method necessarily must modify the graph, since the graph owns the "
      "matrix's column Map.");
    myGraph_->replaceColMap (newColMap);
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar,
    LocalOrdinal,
    GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar,
      LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  replaceDomainMapAndImporter (const Teuchos::RCP<const map_type>& newDomainMap,
                               Teuchos::RCP<const import_type>& newImporter)
  {
    const char tfecfFuncName[] = "replaceDomainMapAndImporter";
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      myGraph_.is_null (), std::runtime_error,
      ": This method does not work if the matrix has a const graph.  The whole "
      "idea of a const graph is that you are not allowed to change it, but this"
      " method necessarily must modify the graph, since the graph owns the "
      "matrix's domain Map and Import objects.");
    myGraph_->replaceDomainMapAndImporter (newDomainMap, newImporter);
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar,
            LocalOrdinal,
            GlobalOrdinal,
            Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
            typename KokkosClassic::DefaultKernels<Scalar,
                                                   LocalOrdinal,
                                                   Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  insertNonownedGlobalValues (const GlobalOrdinal globalRow,
                              const Teuchos::ArrayView<const GlobalOrdinal>& indices,
                              const Teuchos::ArrayView<const Scalar>& values)
  {
    using Teuchos::Array;
    typedef GlobalOrdinal GO;
    typedef typename Array<GO>::size_type size_type;

    const size_type numToInsert = indices.size ();
    // Add the new data to the list of nonlocals.
    // This creates the arrays if they don't exist yet.
    std::pair<Array<GO>, Array<Scalar> >& curRow = nonlocals_[globalRow];
    Array<GO>& curRowInds = curRow.first;
    Array<Scalar>& curRowVals = curRow.second;
    const size_type newCapacity = curRowInds.size () + numToInsert;
    curRowInds.reserve (newCapacity);
    curRowVals.reserve (newCapacity);
    for (size_type k = 0; k < numToInsert; ++k) {
      curRowInds.push_back (indices[k]);
      curRowVals.push_back (values[k]);
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,
            Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
            typename KokkosClassic::DefaultKernels<Scalar, LocalOrdinal, Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  globalAssemble ()
  {
    using Teuchos::arcp;
    using Teuchos::Array;
    using Teuchos::ArrayRCP;
    using Teuchos::ArrayView;
    using Teuchos::CommRequest;
    using Teuchos::gatherAll;
    using Teuchos::isend;
    using Teuchos::ireceive;
    using Teuchos::null;
    using Teuchos::outArg;
    using Teuchos::RCP;
    using Teuchos::rcpFromRef;
    using Teuchos::REDUCE_MAX;
    using Teuchos::reduceAll;
    using Teuchos::SerialDenseMatrix;
    using Teuchos::tuple;
    using Teuchos::waitAll;
    using std::make_pair;
    using std::pair;
    typedef GlobalOrdinal GO;
    typedef typename Array<GO>::size_type size_type;
    // nonlocals_ contains the entries stored by previous calls to
    // insertGlobalValues() for nonowned rows.
    typedef std::map<GO, pair<Array<GO>, Array<Scalar> > > nonlocals_map_type;
    typedef typename nonlocals_map_type::const_iterator nonlocals_iter_type;

    const char tfecfFuncName[] = "globalAssemble";
    const Teuchos::Comm<int>& comm = * (getComm ());
    const int numImages = comm.getSize ();
    const int myImageID = comm.getRank ();

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! isFillActive (), std::runtime_error, ": requires that fill is active.");

    // Determine (via a global all-reduce) if any processes have
    // nonlocal entries to share.  This is necessary even if the
    // matrix has a static graph, because insertGlobalValues allows
    // nonlocal entries in that case.
    size_t MyNonlocals = nonlocals_.size(),
           MaxGlobalNonlocals;
    reduceAll<int, size_t> (comm, REDUCE_MAX, MyNonlocals,
                            outArg (MaxGlobalNonlocals));
    if (MaxGlobalNonlocals == 0) {
      return;  // no entries to share
    }

    // FIXME (mfh 14 Dec 2012) The code below reimplements an Export
    // operation.  It would be better just to use an Export.  See
    // Comment #34 in discussion of Bug 5782.
    //
    // mfh 24 Feb 2014: On the other hand, this is not technically an
    // Export, since the row Map might not necessarily be one-to-one.

    // compute a list of NLRs from nonlocals_ and use it to compute:
    //      IdsAndRows: a vector of (id,row) pairs
    //          NLR2Id: a map from NLR to the Id that owns it
    // globalNeighbors: a global graph of connectivity between images:
    //                  globalNeighbors(i,j) indicates that j sends to i
    //         sendIDs: a list of all images I send to
    //         recvIDs: a list of all images I receive from (constructed later)
    Array<pair<int,GlobalOrdinal> > IdsAndRows;
    std::map<GlobalOrdinal,int> NLR2Id;
    SerialDenseMatrix<int,char> globalNeighbors;
    Array<int> sendIDs, recvIDs;
    {
      // Construct the set of all nonowned rows encountered by this
      // process in insertGlobalValues() or sumIntoGlobalValues().
      std::set<GlobalOrdinal> setOfRows;
      for (nonlocals_iter_type iter = nonlocals_.begin ();
           iter != nonlocals_.end (); ++iter) {
        setOfRows.insert (iter->first);
      }
      // Copy the resulting set of nonowned rows into an Array.
      Array<GlobalOrdinal> NLRs (setOfRows.size ());
      std::copy (setOfRows.begin (), setOfRows.end (), NLRs.begin ());

      // get a list of ImageIDs for the non-local rows (NLRs)
      Array<int> NLRIds (NLRs.size ());
      {
        const LookupStatus stat =
          getRowMap ()->getRemoteIndexList (NLRs (), NLRIds ());
        const int lclerr = (stat == IDNotPresent ? 1 : 0);
        int gblerr;
        reduceAll<int, int> (comm, REDUCE_MAX, lclerr, outArg (gblerr));
        TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
          gblerr, std::runtime_error, ": non-local entries correspond to "
          "invalid rows.");
      }

      // build up a list of neighbors, as well as a map between NLRs and Ids
      // localNeighbors[i] != 0 iff I have data to send to image i
      // put NLRs,Ids into an array of pairs
      IdsAndRows.reserve (NLRs.size ());
      Array<char> localNeighbors (numImages, 0);
      typename Array<GO>::const_iterator nlr;
      typename Array<int>::const_iterator id;
      for (nlr = NLRs.begin (), id = NLRIds.begin ();
           nlr != NLRs.end (); ++nlr, ++id) {
        NLR2Id[*nlr] = *id;
        localNeighbors[*id] = 1;
        IdsAndRows.push_back (make_pair (*id, *nlr));
      }
      for (int j = 0; j < numImages; ++j) {
        if (localNeighbors[j]) {
          sendIDs.push_back (j);
        }
      }
      // sort IdsAndRows, by Ids first, then rows
      std::sort (IdsAndRows.begin (), IdsAndRows.end ());
      // gather from other nodes to form the full graph
      //
      // FIXME (mfh 24 Feb 2014) Ugh, this is awful!!!  It's making a
      // P x P matrix which is the full graph of process connectivity.
      // Neither Export nor Import does this!  It would probably be
      // more efficient to do the following:
      //
      //   1. Form the one-to-one version of the row Map, tgtMap
      //   2. Form the (possibly overlapping) Map srcMap, with the
      //      global row indices which are the keys of nonlocals_ on
      //      each process
      //   3. Construct an Export from srcMap to tgtMap
      //   4. Execute the Export with Tpetra::ADD
      globalNeighbors.shapeUninitialized (numImages, numImages);
      gatherAll (comm, numImages, localNeighbors.getRawPtr (),
                 numImages*numImages, globalNeighbors.values ());
      // globalNeighbors at this point contains (on all images) the
      // connectivity between the images.
      // globalNeighbors(i,j) != 0 means that j sends to i/that i receives from j
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // FIGURE OUT WHO IS SENDING TO WHOM AND HOW MUCH
    // DO THIS IN THE PROCESS OF PACKING ALL OUTGOING DATA ACCORDING TO DESTINATION ID
    //////////////////////////////////////////////////////////////////////////////////////

    // loop over all columns to know from which images I can expect to receive something
    for (int j=0; j<numImages; ++j) {
      if (globalNeighbors (myImageID, j)) {
        recvIDs.push_back (j);
      }
    }
    const size_t numRecvs = recvIDs.size ();

    // we know how many we're sending to already
    // form a contiguous list of all data to be sent
    // track the number of entries for each ID
    Array<Details::CrsIJV<GlobalOrdinal,Scalar> > IJVSendBuffer;
    Array<size_t> sendSizes(sendIDs.size(), 0);
    size_t numSends = 0;
    for (typename Array<pair<int,GlobalOrdinal> >::const_iterator IdAndRow = IdsAndRows.begin();
         IdAndRow != IdsAndRows.end(); ++IdAndRow)
    {
      const int id = IdAndRow->first;
      const GO row = IdAndRow->second;

      // have we advanced to a new send?
      if (sendIDs[numSends] != id) {
        numSends++;
        TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
          sendIDs[numSends] != id, std::logic_error,
          ": internal logic error. Contact Tpetra team.");
      }

      // copy data for row into contiguous storage
      pair<Array<GO>, Array<Scalar> >& nonlocalsRow = nonlocals_[row];
      ArrayView<const GO> nonlocalsRow_colInds = nonlocalsRow.first ();
      ArrayView<const Scalar> nonlocalsRow_values = nonlocalsRow.second ();
      const size_type numNonlocalsRow = nonlocalsRow_colInds.size ();

      for (size_type k = 0; k < numNonlocalsRow; ++k) {
        const Scalar val = nonlocalsRow_values[k];
        const GO col = nonlocalsRow_colInds[k];
        IJVSendBuffer.push_back (Details::CrsIJV<GO, Scalar> (row, col, val));
        sendSizes[numSends]++;
      }
    }
    if (IdsAndRows.size () > 0) {
      numSends++; // one last increment, to make it a count instead of an index
    }
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      Teuchos::as<size_type> (numSends) != sendIDs.size(),
      std::logic_error, ": internal logic error. Contact Tpetra team.");

    // don't need this data anymore
    // clear it before we start allocating a bunch of new memory
    nonlocals_.clear ();

    //////////////////////////////////////////////////////////////////////////////////////
    // TRANSMIT SIZE INFO BETWEEN SENDERS AND RECEIVERS
    //////////////////////////////////////////////////////////////////////////////////////
    // perform non-blocking sends: send sizes to our recipients
    Array<RCP<CommRequest<int> > > sendRequests;
    for (size_t s = 0; s < numSends ; ++s) {
      // we'll fake the memory management, because all communication will be local to this method and the scope of our data
      sendRequests.push_back (isend<int, size_t> (comm, rcpFromRef (sendSizes[s]), sendIDs[s]));
    }
    // perform non-blocking receives: receive sizes from our senders
    Array<RCP<CommRequest<int> > > recvRequests;
    Array<size_t> recvSizes (numRecvs);
    for (size_t r = 0; r < numRecvs; ++r) {
      // we'll fake the memory management, because all communication
      // will be local to this method and the scope of our data
      recvRequests.push_back (ireceive<int, size_t> (comm, rcpFromRef (recvSizes[r]), recvIDs[r]));
    }
    // wait on all
    if (! sendRequests.empty ()) {
      waitAll (comm, sendRequests ());
    }
    if (! recvRequests.empty ()) {
      waitAll (comm, recvRequests ());
    }
    comm.barrier ();
    sendRequests.clear ();
    recvRequests.clear ();

    ////////////////////////////////////////////////////////////////////////////////////
    // NOW SEND/RECEIVE ALL ROW DATA
    ////////////////////////////////////////////////////////////////////////////////////
    // from the size info, build the ArrayViews into IJVSendBuffer
    Array<ArrayView<Details::CrsIJV<GO, Scalar> > > sendBuffers (numSends, null);
    {
      size_t cur = 0;
      for (size_t s=0; s<numSends; ++s) {
        sendBuffers[s] = IJVSendBuffer (cur, sendSizes[s]);
        cur += sendSizes[s];
      }
    }
    // perform non-blocking sends
    for (size_t s = 0; s < numSends; ++s) {
      // we'll fake the memory management, because all communication
      // will be local to this method and the scope of our data
      ArrayRCP<Details::CrsIJV<GO, Scalar> > tmparcp =
        arcp (sendBuffers[s].getRawPtr (), 0, sendBuffers[s].size (), false);
      sendRequests.push_back (isend<int, Details::CrsIJV<GlobalOrdinal,Scalar> > (comm, tmparcp, sendIDs[s]));
    }
    // calculate amount of storage needed for receives
    // setup pointers for the receives as well
    size_t totalRecvSize = std::accumulate (recvSizes.begin (), recvSizes.end (), 0);
    Array<Details::CrsIJV<GO, Scalar> > IJVRecvBuffer (totalRecvSize);
    // from the size info, build the ArrayViews into IJVRecvBuffer
    Array<ArrayView<Details::CrsIJV<GO, Scalar> > > recvBuffers (numRecvs, null);
    {
      size_t cur = 0;
      for (size_t r = 0; r < numRecvs; ++r) {
        recvBuffers[r] = IJVRecvBuffer (cur, recvSizes[r]);
        cur += recvSizes[r];
      }
    }
    // perform non-blocking recvs
    for (size_t r = 0; r < numRecvs ; ++r) {
      // we'll fake the memory management, because all communication
      // will be local to this method and the scope of our data
      ArrayRCP<Details::CrsIJV<GO, Scalar> > tmparcp =
        arcp (recvBuffers[r].getRawPtr (), 0, recvBuffers[r].size (), false);
      recvRequests.push_back (ireceive (comm, tmparcp, recvIDs[r]));
    }
    // perform waits
    if (! sendRequests.empty ()) {
      waitAll (comm, sendRequests ());
    }
    if (! recvRequests.empty ()) {
      waitAll (comm, recvRequests ());
    }
    comm.barrier ();
    sendRequests.clear ();
    recvRequests.clear ();

    ////////////////////////////////////////////////////////////////////////////////////
    // NOW PROCESS THE RECEIVED ROW DATA
    ////////////////////////////////////////////////////////////////////////////////////
    // TODO: instead of adding one entry at a time, add one row at a time.
    //       this requires resorting; they arrived sorted by sending node, so that entries could be non-contiguous if we received
    //       multiple entries for a particular row from different processors.
    //       it also requires restoring the data, which may make it not worth the trouble.

    typedef typename Array<Details::CrsIJV<GO, Scalar> >::const_iterator ijv_iter_type;
    if (this->isStaticGraph ()) {
      for (ijv_iter_type ijv = IJVRecvBuffer.begin ();
           ijv != IJVRecvBuffer.end (); ++ijv) {
        sumIntoGlobalValues (ijv->i, tuple (ijv->j), tuple (ijv->v));
      }
    }
    else { // Dynamic graph; can use insertGlobalValues ()
      for (ijv_iter_type ijv = IJVRecvBuffer.begin ();
           ijv != IJVRecvBuffer.end (); ++ijv) {
        try {
          insertGlobalValues (ijv->i, tuple (ijv->j), tuple (ijv->v));
        }
        catch (std::runtime_error &e) {
          std::ostringstream outmsg;
          outmsg << e.what() << std::endl
                 << "caught in globalAssemble() in " << __FILE__ << ":" << __LINE__
                 << std::endl ;
          TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error, outmsg.str());
        }
      }
    }

    // WHEW! THAT WAS TIRING!
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  resumeFill (const RCP<ParameterList> &params)
  {
    if (! isStaticGraph ()) { // Don't resume fill of a nonowned graph.
      myGraph_->resumeFill (params);
    }
    clearGlobalConstants ();
    lclMatOps_ = null;
    fillComplete_ = false;
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  computeGlobalConstants ()
  {
    // This method doesn't do anything.  The analogous method in
    // CrsGraph does actually compute something.
    //
    // Oddly enough, clearGlobalConstants() clears frobNorm_ (by
    // setting it to -1), but computeGlobalConstants() does _not_
    // compute the Frobenius norm; this is done on demand in
    // getFrobeniusNorm(), and the result is cached there.
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  clearGlobalConstants () {
    // We use -1 to indicate that the Frobenius norm needs to be
    // recomputed, since the values might change between now and the
    // next fillComplete call.
    //
    // Oddly enough, clearGlobalConstants() clears frobNorm_, but
    // computeGlobalConstants() does _not_ compute the Frobenius norm;
    // this is done on demand in getFrobeniusNorm(), and the result is
    // cached there.
    frobNorm_ = -STM::one ();
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  fillComplete (const RCP<ParameterList>& params)
  {
    TEUCHOS_TEST_FOR_EXCEPTION(
      getCrsGraph ().is_null (), std::logic_error, "Tpetra::CrsMatrix::"
      "fillComplete(params): getCrsGraph() returns null.  "
      "This should not happen at this point.  "
      "Please report this bug to the Tpetra developers.");

    if (isStaticGraph () && getCrsGraph ()->isFillComplete ()) {
      fillComplete (getCrsGraph ()->getDomainMap (), getCrsGraph ()->getRangeMap (), params);
    } else {
      fillComplete (getRowMap (), getRowMap (), params);
    }
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  fillComplete (const RCP<const map_type>& domainMap,
                const RCP<const map_type>& rangeMap,
                const RCP<ParameterList>& params)
  {
    using Teuchos::ArrayRCP;
    using Teuchos::RCP;
    using Teuchos::rcp;
    const char tfecfFuncName[] = "fillComplete";

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! isFillActive () || isFillComplete (),
      std::runtime_error, ": Matrix fill state must be active (isFillActive() "
      "must be true) before you may call fillComplete().");
    const int numProcs = getComm ()->getSize ();

    //
    // Read parameters from the input ParameterList.
    //

    // If true, the caller promises that no process did nonlocal
    // changes since the last call to fillComplete.
    bool assertNoNonlocalInserts = false;
    // If true, makeColMap sorts remote GIDs (within each remote
    // process' group).
    bool sortGhosts = true;

    if (! params.is_null ()) {
      assertNoNonlocalInserts = params->get ("No Nonlocal Changes",
                                             assertNoNonlocalInserts);
      if (params->isParameter ("sort column map ghost gids")) {
        sortGhosts = params->get ("sort column map ghost gids", sortGhosts);
      }
      else if (params->isParameter ("Sort column Map ghost GIDs")) {
        sortGhosts = params->get ("Sort column Map ghost GIDs", sortGhosts);
      }
    }
    // We also don't need to do global assembly if there is only one
    // process in the communicator.
    const bool needGlobalAssemble = ! assertNoNonlocalInserts && numProcs > 1;
    // This parameter only matters if this matrix owns its graph.
    if (! myGraph_.is_null ()) {
      myGraph_->sortGhostsAssociatedWithEachProcessor_ = sortGhosts;
    }

    if (! getCrsGraph()->indicesAreAllocated()) {
      if (hasColMap ()) {
        // We have a column Map, so use local indices.
        allocateValues (LocalIndices, GraphNotYetAllocated);
      } else {
        // We don't have a column Map, so use global indices.
        allocateValues (GlobalIndices, GraphNotYetAllocated);
      }
    }
    // Global assemble, if we need to.  This call only costs a single
    // all-reduce if we didn't need global assembly after all.
    if (needGlobalAssemble) {
      globalAssemble ();
    }
    else {
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
        numProcs == 1 && nonlocals_.size() > 0,
        std::runtime_error, ": cannot have nonlocal entries on a serial run.  "
        "An invalid entry (i.e., with row index not in the row Map) must have "
        "been submitted to the CrsMatrix.");
    }

    if (isStaticGraph ()) {
      // FIXME (mfh 18 Jun 2014) This check for correctness of the
      // input Maps incurs a penalty of two all-reduces for the
      // otherwise optimal const graph case.
      //
      // We could turn these (max) 2 all-reduces into (max) 1, by
      // fusing them.  We could do this by adding a "locallySameAs"
      // method to Map, which would return one of four states:
      //
      //   a. Certainly globally the same
      //   b. Certainly globally not the same
      //   c. Locally the same
      //   d. Locally not the same
      //
      // The first two states don't require further communication.
      // The latter two states require an all-reduce to communicate
      // globally, but we only need one all-reduce, since we only need
      // to check whether at least one of the Maps is wrong.
      const bool domainMapsMatch = staticGraph_->getDomainMap ()->isSameAs (*domainMap);
      const bool rangeMapsMatch = staticGraph_->getRangeMap ()->isSameAs (*rangeMap);

      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
        ! domainMapsMatch, std::runtime_error,
        ": The CrsMatrix's domain Map does not match the graph's domain Map.  "
        "The graph cannot be changed because it was given to the CrsMatrix "
        "constructor as const.  You can fix this by passing in the graph's "
        "domain Map and range Map to the matrix's fillComplete call.");

      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
        ! rangeMapsMatch, std::runtime_error,
        ": The CrsMatrix's range Map does not match the graph's range Map.  "
        "The graph cannot be changed because it was given to the CrsMatrix "
        "constructor as const.  You can fix this by passing in the graph's "
        "domain Map and range Map to the matrix's fillComplete call.");
    }
    else {
      // Set the graph's domain and range Maps.  This will clear the
      // Import if the domain Map has changed (is a different
      // pointer), and the Export if the range Map has changed (is a
      // different pointer).
      myGraph_->setDomainRangeMaps (domainMap, rangeMap);

      // Make the graph's column Map, if necessary.
      if (! myGraph_->hasColMap ()) {
        myGraph_->makeColMap ();
      }

      // Make indices local, if necessary.  The method won't do
      // anything if the graph is already locally indexed.
      myGraph_->makeIndicesLocal ();

      if (! myGraph_->isSorted ()) {
        sortEntries ();
      }
      if (! myGraph_->isMerged ()) {
        mergeRedundantEntries ();
      }
      // Make the Import and Export, if they haven't been made already.
      myGraph_->makeImportExport ();
      myGraph_->computeGlobalConstants ();
      myGraph_->fillComplete_ = true;
      myGraph_->checkInternalState ();
    }
    computeGlobalConstants ();
    // fill local objects; will fill and finalize local graph if appropriate
    if (myGraph_.is_null ()) {
      // The matrix does _not_ own the graph, and the graph's
      // structure is already fixed, so just fill the local matrix.
      fillLocalMatrix (params);
    } else {
      // The matrix _does_ own the graph, so fill the local graph at
      // the same time as the local matrix.
      fillLocalGraphAndMatrix (params);
    }
    //
    // Set up the local sparse kernels.
    //
    lclMatOps_ = rcp (new sparse_ops_type (getNode ()));
    // This is where we take the local graph and matrix, and turn them
    // into (possibly optimized) sparse kernels.
    {
      RCP<local_matrix_type> classicLocalMatrix =
        rcp (new local_matrix_type (staticGraph_->getLocalGraph (), params));
      ArrayRCP<scalar_type> classicValues =
        Kokkos::Compat::persistingView (k_lclMatrix_.values);
      classicLocalMatrix->setValues (classicValues);
      lclMatOps_->setGraphAndMatrix (staticGraph_->getLocalGraph (),
                                     classicLocalMatrix);
    }

    // Once we've initialized the sparse kernels, we're done with the
    // local objects.  We may now release them and their memory, since
    // they will persist in the local sparse ops if necessary.  We
    // keep the local graph if the parameters tell us to do so.
    if (myGraph_ != null) {
      bool preserveLocalGraph = true;
      if (params != null) {
        preserveLocalGraph = params->get ("Preserve Local Graph", preserveLocalGraph);
      }
      if (! preserveLocalGraph) {
        myGraph_->lclGraph_ = null;
      }
    }
    fillComplete_ = true; // Now we're fill complete!
    checkInternalState ();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal,
            class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  expertStaticFillComplete (const Teuchos::RCP<const map_type> & domainMap,
                            const Teuchos::RCP<const map_type> & rangeMap,
                            const Teuchos::RCP<const import_type>& importer,
                            const Teuchos::RCP<const export_type>& exporter,
                            const Teuchos::RCP<Teuchos::ParameterList> &params)
  {
    const char tfecfFuncName[] = "expertStaticFillComplete";
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC( ! isFillActive() || isFillComplete(),
      std::runtime_error, ": Matrix fill state must be active (isFillActive() "
      "must be true) before calling fillComplete().");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(myGraph_==Teuchos::null, std::logic_error,": myGraph_ is null.  This is not allowed.");

    // We will presume globalAssemble is not needed, so we do the ESFC on the graph
    myGraph_->expertStaticFillComplete (domainMap, rangeMap, importer, exporter);

    computeGlobalConstants();

    // Fill the local matrix & MatOps
    fillLocalGraphAndMatrix(params);
    lclMatOps_ = rcp (new sparse_ops_type (getNode ()));

    // This is where we take the local graph and matrix, and turn them
    // into (possibly optimized) sparse kernels.
    {
      RCP<local_matrix_type> classicLocalMatrix =
        rcp (new local_matrix_type (staticGraph_->getLocalGraph (), params));
      ArrayRCP<scalar_type> classicValues =
        Kokkos::Compat::persistingView (k_lclMatrix_.values);
      classicLocalMatrix->setValues (classicValues);
      lclMatOps_->setGraphAndMatrix (staticGraph_->getLocalGraph (),
                                     classicLocalMatrix);
    }

    // Once we've initialized the sparse kernels, we're done with the
    // local objects.  We may now release them and their memory, since
    // they will persist in the local sparse ops if necessary.  We
    // keep the local graph if the parameters tell us to do so.
    bool preserveLocalGraph = true;
    if (params != null) {
      preserveLocalGraph = params->get ("Preserve Local Graph", true);
    }
    if (! preserveLocalGraph) {
      myGraph_->lclGraph_ = null;
    }

    // Now we're fill complete!
    fillComplete_ = true;

    // Sanity checks at the end.
#ifdef HAVE_TPETRA_DEBUG
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(isFillActive(), std::logic_error,
      ": We're at the end of fillComplete(), but isFillActive() is true.  "
      "Please report this bug to the Tpetra developers.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(! isFillComplete(), std::logic_error,
      ": We're at the end of fillComplete(), but isFillActive() is true.  "
      "Please report this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG
    checkInternalState();

  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::sortEntries()
  {
    TEUCHOS_TEST_FOR_EXCEPTION(isStaticGraph() == true, std::runtime_error,
        typeName(*this) << "::sortEntries(): cannot sort with static graph.");
    if (myGraph_->isSorted() == false) {
      for (size_t row=0; row < getNodeNumRows(); ++row) {
        RowInfo rowInfo = myGraph_->getRowInfo(row);
        myGraph_->template sortRowIndicesAndValues<Scalar>(rowInfo,this->getViewNonConst(rowInfo));
      }
      // we just sorted every row
      myGraph_->indicesAreSorted_ = true;
    }
  }


  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  mergeRedundantEntries ()
  {
    TEUCHOS_TEST_FOR_EXCEPTION(isStaticGraph() == true, std::runtime_error,
      typeName(*this) << "::mergeRedundantEntries: Cannot merge with static graph.");
    if (! myGraph_->isMerged ()) {
      const size_t nodeNumRows = getNodeNumRows ();
      for (size_t row = 0; row < nodeNumRows; ++row) {
        RowInfo rowInfo = myGraph_->getRowInfo (row);
        Teuchos::ArrayView<Scalar> rowView = (this->getViewNonConst (rowInfo)) ();
        myGraph_->template mergeRowIndicesAndValues<Scalar> (rowInfo, rowView);
      }
      myGraph_->noRedundancies_ = true; // we just merged every row
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  applyNonTranspose (const MultiVector<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > & X_in,
                     MultiVector<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > & Y_in,
                     Scalar alpha,
                     Scalar beta) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    using Teuchos::rcp_const_cast;
    using Teuchos::rcpFromRef;

    // mfh 05 Jun 2014: Special case for alpha == 0.  I added this to
    // fix an Ifpack2 test (RILUKSingleProcessUnitTests), which was
    // failing only for the Kokkos refactor version of Tpetra.  It's a
    // good idea regardless to have the bypass.
    if (alpha == STS::zero ()) {
      if (beta == STS::zero ()) {
        Y_in.putScalar (STS::zero ());
      } else if (beta != STS::one ()) {
        Y_in.scale (beta);
      }
      return;
    }

    // It's possible that X is a view of Y or vice versa.  We don't
    // allow this (apply() requires that X and Y not alias one
    // another), but it's helpful to detect and work around this case.
    // We don't try to to detect the more subtle cases (e.g., one is a
    // subview of the other, but their initial pointers differ).  We
    // only need to do this if this matrix's Import is trivial;
    // otherwise, we don't actually apply the operator from X into Y.

    RCP<const import_type> importer = this->getGraph ()->getImporter ();
    RCP<const export_type> exporter = this->getGraph ()->getExporter ();

    // If beta == 0, then the output MV will be overwritten; none of
    // its entries should be read.  (Sparse BLAS semantics say that we
    // must ignore any Inf or NaN entries in Y_in, if beta is zero.)
    // This matters if we need to do an Export operation; see below.
    const bool Y_is_overwritten = (beta == STS::zero());

    // We treat the case of a replicated MV output specially.
    const bool Y_is_replicated = ! Y_in.isDistributed ();

    // This is part of the special case for replicated MV output.
    // We'll let each process do its thing, but do an all-reduce at
    // the end to sum up the results.  Setting beta=0 on all processes
    // but Proc 0 makes the math work out for the all-reduce.  (This
    // assumes that the replicated data is correctly replicated, so
    // that the data are the same on all processes.)
    if (Y_is_replicated && this->getComm ()->getRank () > 0) {
      beta = STS::zero ();
    }

    // Temporary MV for Import operation.  After the block of code
    // below, this will be an (Imported if necessary) column Map MV
    // ready to give to localMultiply().
    RCP<const MV> X_colMap;
    if (importer.is_null ()) {
      if (! X_in.isConstantStride ()) {
        // Not all sparse mat-vec kernels can handle an input MV with
        // nonconstant stride correctly, so we have to copy it in that
        // case into a constant stride MV.  To make a constant stride
        // copy of X_in, we force creation of the column (== domain)
        // Map MV (if it hasn't already been created, else fetch the
        // cached copy).  This avoids creating a new MV each time.
        RCP<MV> X_colMapNonConst = getColumnMapMultiVector (X_in, true);
        Tpetra::deep_copy (*X_colMapNonConst, X_in);
        X_colMap = rcp_const_cast<const MV> (X_colMapNonConst);
      }
      else {
        // The domain and column Maps are the same, so do the local
        // multiply using the domain Map input MV X_in.
        X_colMap = rcpFromRef (X_in);
      }
    }
    else {
      // We're doing an Import anyway, which will copy the relevant
      // elements of the domain Map MV X_in into a separate column Map
      // MV.  Thus, we don't have to worry whether X_in is constant
      // stride.
      RCP<MV> X_colMapNonConst = getColumnMapMultiVector (X_in);

      // Import from the domain Map MV to the column Map MV.
      X_colMapNonConst->doImport (X_in, *importer, INSERT);
      X_colMap = rcp_const_cast<const MV> (X_colMapNonConst);
    }

    // Temporary MV for Export operation, or for copying a nonconstant
    // stride output MV into a constant stride MV.
    RCP<MV> Y_rowMap = getRowMapMultiVector (Y_in);

    // If we have a nontrivial Export object, we must perform an
    // Export.  In that case, the local multiply result will go into
    // the row Map multivector.  We don't have to make a
    // constant-stride version of Y_in in this case, because we had to
    // make a constant stride Y_rowMap MV and do an Export anyway.
    if (! exporter.is_null ()) {
      this->template localMultiply<Scalar, Scalar> (*X_colMap, *Y_rowMap,
                                                    Teuchos::NO_TRANS,
                                                    alpha, STS::zero ());
      // If we're overwriting the output MV Y_in completely (beta ==
      // 0), then make sure that it is filled with zeros before we do
      // the Export.  Otherwise, the ADD combine mode will use data in
      // Y_in, which is supposed to be zero.
      if (Y_is_overwritten) {
        Y_in.putScalar (STS::zero ());
      }
      else {
        // Scale the output MV by beta, so that the Export sums in the
        // mat-vec contribution: Y_in = beta*Y_in + alpha*A*X_in.
        Y_in.scale (beta);
      }
      // Do the Export operation.
      Y_in.doExport (*Y_rowMap, *exporter, ADD);
    }
    else { // Don't do an Export: row Map and range Map are the same.
      //
      // If Y_in does not have constant stride, or if the column Map
      // MV aliases Y_in, then we can't let the kernel write directly
      // to Y_in.  Instead, we have to use the cached row (== range)
      // Map MV as temporary storage.
      //
      // FIXME (mfh 05 Jun 2014) This test for aliasing only tests if
      // the user passed in the same MultiVector for both X and Y.  It
      // won't detect whether one MultiVector views the other.  We
      // should also check the MultiVectors' raw data pointers.
      if (! Y_in.isConstantStride () || X_colMap.getRawPtr () == &Y_in) {
        // Force creating the MV if it hasn't been created already.
        // This will reuse a previously created cached MV.
        Y_rowMap = getRowMapMultiVector (Y_in, true);

        // If beta == 0, we don't need to copy Y_in into Y_rowMap,
        // since we're overwriting it anyway.
        if (beta != STS::zero ()) {
          Tpetra::deep_copy (*Y_rowMap, Y_in);
        }
        this->template localMultiply<Scalar, Scalar> (*X_colMap,
                                                      *Y_rowMap,
                                                      Teuchos::NO_TRANS,
                                                      alpha, beta);
        Tpetra::deep_copy (Y_in, *Y_rowMap);
      }
      else {
        this->template localMultiply<Scalar, Scalar> (*X_colMap, Y_in,
                                                      Teuchos::NO_TRANS,
                                                      alpha, beta);
      }
    }

    // If the range Map is a locally replicated Map, sum up
    // contributions from each process.  We set beta = 0 on all
    // processes but Proc 0 initially, so this will handle the scaling
    // factor beta correctly.
    if (Y_is_replicated) {
      Y_in.reduce ();
    }
  }

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  applyTranspose (const MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >& X_in,
                  MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >& Y_in,
                  const Teuchos::ETransp mode,
                  Scalar alpha,
                  Scalar beta) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    using Teuchos::rcp_const_cast;
    using Teuchos::rcpFromRef;

    // Take shortcuts for alpha == 0.
    if (alpha == STS::zero ()) {
      // Follow the Sparse BLAS convention by ignoring both the matrix
      // and X_in, in this case.
      if (beta == STS::zero ()) {
        // Follow the Sparse BLAS convention by overwriting any Inf or
        // NaN values in Y_in, in this case.
        Y_in.putScalar (STS::zero ());
      }
      else {
        Y_in.scale (beta);
      }
      return;
    }

    const size_t numVectors = X_in.getNumVectors ();

    // We don't allow X_in and Y_in to alias one another.  It's hard
    // to check this, because advanced users could create views from
    // raw pointers.  However, if X_in and Y_in reference the same
    // object, we will do the user a favor by copying X into new
    // storage (with a warning).  We only need to do this if we have
    // trivial importers; otherwise, we don't actually apply the
    // operator from X into Y.
    RCP<const import_type> importer = this->getGraph ()->getImporter ();
    RCP<const export_type> exporter = this->getGraph ()->getExporter ();
    // access X indirectly, in case we need to create temporary storage
    RCP<const MV> X;

    // some parameters for below
    const bool Y_is_replicated = ! Y_in.isDistributed ();
    const bool Y_is_overwritten = (beta == STS::zero ());
    if (Y_is_replicated && this->getComm ()->getRank () > 0) {
      beta = STS::zero ();
    }

    // The kernels do not allow input or output with nonconstant stride.
    if (! X_in.isConstantStride () && importer.is_null ()) {
      X = rcp (new MV (X_in)); // Constant-stride copy of X_in
    } else {
      X = rcpFromRef (X_in); // Reference to X_in
    }

    // Set up temporary multivectors for Import and/or Export.
    if (importer != null) {
      if (importMV_ != null && importMV_->getNumVectors() != numVectors) {
        importMV_ = null;
      }
      if (importMV_ == null) {
        importMV_ = rcp (new MV (this->getColMap (), numVectors));
      }
    }
    if (exporter != null) {
      if (exportMV_ != null && exportMV_->getNumVectors() != numVectors) {
        exportMV_ = null;
      }
      if (exportMV_ == null) {
        exportMV_ = rcp (new MV (this->getRowMap (), numVectors));
      }
    }

    // If we have a non-trivial exporter, we must import elements that
    // are permuted or are on other processors.
    if (! exporter.is_null ()) {
      exportMV_->doImport (X_in, *exporter, INSERT);
      X = exportMV_; // multiply out of exportMV_
    }

    // If we have a non-trivial importer, we must export elements that
    // are permuted or belong to other processors.  We will compute
    // solution into the to-be-exported MV; get a view.
    if (importer != null) {
      // Do the local computation.
      this->template localMultiply<Scalar, Scalar> (*X, *importMV_, mode, alpha, STS::zero ());
      if (Y_is_overwritten) {
        Y_in.putScalar (STS::zero ());
      } else {
        Y_in.scale (beta);
      }
      Y_in.doExport(*importMV_,*importer,ADD);
    }
    // otherwise, multiply into Y
    else {
      // can't multiply in-situ; can't multiply into non-strided multivector
      //
      // FIXME (mfh 05 Jun 2014) This test for aliasing only tests if
      // the user passed in the same MultiVector for both X and Y.  It
      // won't detect whether one MultiVector views the other.  We
      // should also check the MultiVectors' raw data pointers.
      if (! Y_in.isConstantStride () || X.getRawPtr () == &Y_in) {
        // Make a deep copy of Y_in, into which to write the multiply result.
        MV Y (Y_in, Teuchos::Copy);
        this->template localMultiply<Scalar, Scalar> (*X, Y, mode, alpha, beta);
        Tpetra::deep_copy (Y_in, Y);
      } else {
        this->template localMultiply<Scalar, Scalar> (*X, Y_in, mode, alpha, beta);
      }
    }

    // If the range Map is a locally replicated map, sum the
    // contributions from each process.  (That's why we set beta=0
    // above for all processes but Proc 0.)
    if (Y_is_replicated) {
      Y_in.reduce ();
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  apply (const MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > &X,
         MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > &Y,
         Teuchos::ETransp mode,
         Scalar alpha,
         Scalar beta) const
  {
    TEUCHOS_TEST_FOR_EXCEPTION(
      ! isFillComplete (), std::runtime_error,
      "Tpetra::CrsMatrix::apply(): Cannot call apply() until fillComplete() "
      "has been called.");
    if (mode == Teuchos::NO_TRANS) {
      applyNonTranspose (X, Y, alpha, beta);
    } else {
      applyTranspose (X, Y, mode, alpha, beta);
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  gaussSeidel (const MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > &B,
               MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > &X,
               const MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > &D,
               const Scalar& dampingFactor,
               const ESweepDirection direction,
               const int numSweeps) const
  {
    reorderedGaussSeidel(B,X,D,Teuchos::null,dampingFactor,direction,numSweeps);
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  reorderedGaussSeidel (const MultiVector<Scalar, LocalOrdinal, GlobalOrdinal, node_type>& B,
                        MultiVector<Scalar, LocalOrdinal, GlobalOrdinal, node_type>& X,
                        const MultiVector<Scalar, LocalOrdinal, GlobalOrdinal, node_type>& D,
                        const Teuchos::ArrayView<LocalOrdinal>& rowIndices,
                        const Scalar& dampingFactor,
                        const ESweepDirection direction,
                        const int numSweeps) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    using Teuchos::rcpFromRef;
    using Teuchos::rcp_const_cast;
    typedef Scalar ST;

    TEUCHOS_TEST_FOR_EXCEPTION(
      isFillComplete() == false, std::runtime_error,
      "Tpetra::CrsMatrix::gaussSeidel: cannot call this method until "
      "fillComplete() has been called.");
    TEUCHOS_TEST_FOR_EXCEPTION(
      numSweeps < 0,
      std::invalid_argument,
      "Tpetra::CrsMatrix::gaussSeidel: The number of sweeps must be , "
      "nonnegative but you provided numSweeps = " << numSweeps << " < 0.");

    // Translate from global to local sweep direction.
    // While doing this, validate the input.
    KokkosClassic::ESweepDirection localDirection;
    if (direction == Forward) {
      localDirection = KokkosClassic::Forward;
    }
    else if (direction == Backward) {
      localDirection = KokkosClassic::Backward;
    }
    else if (direction == Symmetric) {
      // We'll control local sweep direction manually.
      localDirection = KokkosClassic::Forward;
    }
    else {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument,
        "Tpetra::CrsMatrix::gaussSeidel: The 'direction' enum does not have "
        "any of its valid values: Forward, Backward, or Symmetric.");
    }

    if (numSweeps == 0) {
      return; // Nothing to do.
    }

    // We don't need the Export object because this method assumes
    // that the row, domain, and range Maps are the same.  We do need
    // the Import object, if there is one, though.
    RCP<const import_type> importer = this->getGraph()->getImporter();
    RCP<const export_type> exporter = this->getGraph()->getExporter();
    TEUCHOS_TEST_FOR_EXCEPTION(
      ! exporter.is_null (), std::runtime_error,
      "Tpetra's gaussSeidel implementation requires that the row, domain, "
      "and range Maps be the same.  This cannot be the case, because the "
      "matrix has a nontrivial Export object.");

    RCP<const map_type> domainMap = this->getDomainMap ();
    RCP<const map_type> rangeMap = this->getRangeMap ();
    RCP<const map_type> rowMap = this->getGraph ()->getRowMap ();
    RCP<const map_type> colMap = this->getGraph ()->getColMap ();

#ifdef HAVE_TEUCHOS_DEBUG
    {
      // The relation 'isSameAs' is transitive.  It's also a
      // collective, so we don't have to do a "shared" test for
      // exception (i.e., a global reduction on the test value).
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! X.getMap ()->isSameAs (*domainMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidel requires that the input "
        "multivector X be in the domain Map of the matrix.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! B.getMap ()->isSameAs (*rangeMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidel requires that the input "
        "B be in the range Map of the matrix.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! D.getMap ()->isSameAs (*rowMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidel requires that the input "
        "D be in the row Map of the matrix.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! rowMap->isSameAs (*rangeMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidel requires that the row Map and the "
        "range Map be the same (in the sense of Tpetra::Map::isSameAs).");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! domainMap->isSameAs (*rangeMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidel requires that the domain Map and "
        "the range Map of the matrix be the same.");
    }
#else
    // Forestall any compiler warnings for unused variables.
    (void) rangeMap;
    (void) rowMap;
#endif // HAVE_TEUCHOS_DEBUG

    // If B is not constant stride, copy it into a constant stride
    // multivector.  We'l handle the right-hand side B first and deal
    // with X right before the sweeps, to improve locality of the
    // first sweep.  (If the problem is small enough, then that will
    // hopefully keep more of the entries of X in cache.  This
    // optimizes for the typical case of a small number of sweeps.)
    RCP<const MV> B_in;
    if (B.isConstantStride()) {
      B_in = rcpFromRef (B);
    }
    else {
      // The range Map and row Map are the same in this case, so we
      // can use the (possibly cached) row Map multivector to store a
      // constant stride copy of B.  We don't have to copy back, since
      // Gauss-Seidel won't modify B.
      RCP<MV> B_in_nonconst = getRowMapMultiVector (B, true);
      *B_in_nonconst = B; // Copy from B into B_in(_nonconst).
      B_in = rcp_const_cast<const MV> (B_in_nonconst);

      TPETRA_EFFICIENCY_WARNING(
        ! B.isConstantStride (),
        std::runtime_error,
        "gaussSeidel: The current implementation of the Gauss-Seidel kernel "
        "requires that X and B both have constant stride.  Since B does not "
        "have constant stride, we had to make a copy.  This is a limitation of "
        "the current implementation and not your fault, but we still report it "
        "as an efficiency warning for your information.");
    }

    // If X is not constant stride, copy it into a constant stride
    // multivector.  Also, make the column Map multivector X_colMap,
    // and its domain Map view X_domainMap.  (X actually must be a
    // domain Map view of a column Map multivector; exploit this, if X
    // has constant stride.)

    RCP<MV> X_domainMap;
    RCP<MV> X_colMap;
    bool copiedInput = false;

    if (importer.is_null ()) { // Domain and column Maps are the same.
      if (X.isConstantStride ()) {
        X_domainMap = rcpFromRef (X);
        X_colMap = X_domainMap;
        copiedInput = false;
      }
      else {
        // Get a temporary column Map multivector, make a domain Map
        // view of it, and copy X into the domain Map view.  We have
        // to copy here because we won't be doing Import operations.
        X_colMap = getColumnMapMultiVector (X, true);
        X_domainMap = X_colMap; // Domain and column Maps are the same.
        deep_copy(*X_domainMap, X); // Copy X into the domain Map view.
        copiedInput = true;
        TPETRA_EFFICIENCY_WARNING(
          ! X.isConstantStride (), std::runtime_error,
          "Tpetra::CrsMatrix::gaussSeidel: The current implementation of the "
          "Gauss-Seidel kernel requires that X and B both have constant "
          "stride.  Since X does not have constant stride, we had to make a "
          "copy.  This is a limitation of the current implementation and not "
          "your fault, but we still report it as an efficiency warning for "
          "your information.");
      }
    }
    else { // We will be doing Import operations in the sweeps.
      if (X.isConstantStride ()) {
        X_domainMap = rcpFromRef (X);
        // This kernel assumes that X is a domain Map view of a column
        // Map multivector.  We will only check if this is valid if
        // the CMake configure Teuchos_ENABLE_DEBUG is ON.
        X_colMap = X_domainMap->offsetViewNonConst (colMap, 0);

        // FIXME (mfh 19 Mar 2013) Do we need to fill the remote
        // entries of X_colMap with zeros?  Do we need to fill all of
        // X_domainMap initially with zeros?  Ifpack
        // (Ifpack_PointRelaxation.cpp, line 906) creates an entirely
        // new MultiVector each time.

        // Do the first Import for the first sweep.  This simplifies
        // the logic in the sweeps.
        X_colMap->doImport (X, *importer, INSERT);
        copiedInput = false;
      }
      else {
        // Get a temporary column Map multivector X_colMap, and make a
        // domain Map view X_domainMap of it.  Instead of copying, we
        // do an Import from X into X_domainMap.  This saves us a
        // copy, since the Import has to copy the data anyway.
        X_colMap = getColumnMapMultiVector (X, true);
        X_domainMap = X_colMap->offsetViewNonConst (domainMap, 0);
        X_colMap->doImport (X, *importer, INSERT);
        copiedInput = true;
        TPETRA_EFFICIENCY_WARNING(
          ! X.isConstantStride (), std::runtime_error,
          "Tpetra::CrsMatrix::gaussSeidel: The current implementation of the "
          "Gauss-Seidel kernel requires that X and B both have constant stride.  "
          "Since X does not have constant stride, we had to make a copy.  "
          "This is a limitation of the current implementation and not your fault, "
          "but we still report it as an efficiency warning for your information.");
      }
    }

    for (int sweep = 0; sweep < numSweeps; ++sweep) {
      if (! importer.is_null () && sweep > 0) {
        // We already did the first Import for the zeroth sweep.
        X_colMap->doImport (*X_domainMap, *importer, INSERT);
      }

      // Do local Gauss-Seidel.
      if (direction != Symmetric) {
        if(rowIndices.is_null())
          this->template localGaussSeidel<ST, ST> (*B_in, *X_colMap, D,
                                                   dampingFactor,
                                                   localDirection);
        else
          this->template reorderedLocalGaussSeidel<ST, ST> (*B_in, *X_colMap, D, rowIndices,
                                                            dampingFactor,
                                                            localDirection);
      } else { // direction == Symmetri
        const bool doImportBetweenDirections = false;
        if(rowIndices.is_null()) {
          this->template localGaussSeidel<ST, ST> (*B_in, *X_colMap, D,
                                                   dampingFactor,
                                                   KokkosClassic::Forward);
          // mfh 18 Mar 2013: Aztec's implementation of "symmetric
          // Gauss-Seidel" does _not_ do an Import between the forward
          // and backward sweeps.  This makes sense, because Aztec
          // considers "symmetric Gauss-Seidel" a subdomain solver.
          if (doImportBetweenDirections) {
            // Communicate again before the Backward sweep.
            if (! importer.is_null ()) {
              X_colMap->doImport (*X_domainMap, *importer, INSERT);
            }
          }
          this->template localGaussSeidel<ST, ST> (*B_in, *X_colMap, D,
                                                   dampingFactor,
                                                   KokkosClassic::Backward);
        }
        else {
          this->template reorderedLocalGaussSeidel<ST, ST> (*B_in, *X_colMap, D, rowIndices,
                                                   dampingFactor,
                                                   KokkosClassic::Forward);
          if (doImportBetweenDirections) {
            // Communicate again before the Backward sweep.
            if (! importer.is_null ()) {
              X_colMap->doImport (*X_domainMap, *importer, INSERT);
            }
          }
          this->template reorderedLocalGaussSeidel<ST, ST> (*B_in, *X_colMap, D, rowIndices,
                                                            dampingFactor,
                                                            KokkosClassic::Backward);
        }
      }
    }

    if (copiedInput) {
      deep_copy(X, *X_domainMap); // Copy back from X_domainMap to X.
    }
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  gaussSeidelCopy (MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,node_type>& X,
                   const MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,node_type>& B,
                   const MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,node_type>& D,
                   const Scalar& dampingFactor,
                   const ESweepDirection direction,
                   const int numSweeps,
                   const bool zeroInitialGuess) const
  {
    reorderedGaussSeidelCopy(X,B,D,Teuchos::null,dampingFactor,direction,numSweeps,zeroInitialGuess);
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  reorderedGaussSeidelCopy (MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,node_type>& X,
                            const MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,node_type>& B,
                            const MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,node_type>& D,
                            const Teuchos::ArrayView<LocalOrdinal>& rowIndices,
                            const Scalar& dampingFactor,
                            const ESweepDirection direction,
                            const int numSweeps,
                            const bool zeroInitialGuess) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    using Teuchos::rcpFromRef;
    using Teuchos::rcp_const_cast;
    typedef Scalar ST;
    TEUCHOS_TEST_FOR_EXCEPTION(
      isFillComplete() == false, std::runtime_error,
      "Tpetra::CrsMatrix::gaussSeidelCopy: cannot call this method until "
      "fillComplete() has been called.");
    TEUCHOS_TEST_FOR_EXCEPTION(
      numSweeps < 0,
      std::invalid_argument,
      "gaussSeidelCopy: The number of sweeps must be nonnegative, "
      "but you provided numSweeps = " << numSweeps << " < 0.");

    // Translate from global to local sweep direction.
    // While doing this, validate the input.
    KokkosClassic::ESweepDirection localDirection;
    if (direction == Forward) {
      localDirection = KokkosClassic::Forward;
    }
    else if (direction == Backward) {
      localDirection = KokkosClassic::Backward;
    }
    else if (direction == Symmetric) {
      // We'll control local sweep direction manually.
      localDirection = KokkosClassic::Forward;
    }
    else {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument,
        "gaussSeidelCopy: The 'direction' enum does not have any of its "
        "valid values: Forward, Backward, or Symmetric.");
    }

    if (numSweeps == 0) {
      return;
    }

    RCP<const import_type> importer = this->getGraph()->getImporter();
    RCP<const export_type> exporter = this->getGraph()->getExporter();
    TEUCHOS_TEST_FOR_EXCEPTION(
      ! exporter.is_null (),
      std::runtime_error,
      "Tpetra's gaussSeidelCopy implementation requires that the row, domain, "
      "and range Maps be the same.  This cannot be the case, because the "
      "matrix has a nontrivial Export object.");

    RCP<const map_type> domainMap = this->getDomainMap ();
    RCP<const map_type> rangeMap = this->getRangeMap ();
    RCP<const map_type> rowMap = this->getGraph ()->getRowMap ();
    RCP<const map_type> colMap = this->getGraph ()->getColMap ();

#ifdef HAVE_TEUCHOS_DEBUG
    {
      // The relation 'isSameAs' is transitive.  It's also a
      // collective, so we don't have to do a "shared" test for
      // exception (i.e., a global reduction on the test value).
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! X.getMap ()->isSameAs (*domainMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidelCopy requires that the input "
        "multivector X be in the domain Map of the matrix.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! B.getMap ()->isSameAs (*rangeMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidelCopy requires that the input "
        "B be in the range Map of the matrix.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! D.getMap ()->isSameAs (*rowMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidelCopy requires that the input "
        "D be in the row Map of the matrix.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! rowMap->isSameAs (*rangeMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidelCopy requires that the row Map and the "
        "range Map be the same (in the sense of Tpetra::Map::isSameAs).");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! domainMap->isSameAs (*rangeMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidelCopy requires that the domain Map and "
        "the range Map of the matrix be the same.");
    }
#else
    // Forestall any compiler warnings for unused variables.
    (void) rangeMap;
    (void) rowMap;
#endif // HAVE_TEUCHOS_DEBUG

    // Fetch a (possibly cached) temporary column Map multivector
    // X_colMap, and a domain Map view X_domainMap of it.  Both have
    // constant stride by construction.  We know that the domain Map
    // must include the column Map, because our Gauss-Seidel kernel
    // requires that the row Map, domain Map, and range Map are all
    // the same, and that each process owns all of its own diagonal
    // entries of the matrix.

    RCP<MV> X_colMap;
    RCP<MV> X_domainMap;
    bool copyBackOutput = false;
    if (importer.is_null ()) {
      if (X.isConstantStride ()) {
        X_colMap = rcpFromRef (X);
        X_domainMap = rcpFromRef (X);
        // Column Map and domain Map are the same, so there are no
        // remote entries.  Thus, if we are not setting the initial
        // guess to zero, we don't have to worry about setting remote
        // entries to zero, even though we are not doing an Import in
        // this case.
        if (zeroInitialGuess) {
          X_colMap->putScalar (STS::zero ());
        }
        // No need to copy back to X at end.
      }
      else { // We must copy X into a constant stride multivector.
        // Just use the cached column Map multivector for that.
        // force=true means fill with zeros, so no need to fill
        // remote entries (not in domain Map) with zeros.
        X_colMap = getColumnMapMultiVector (X, true);
        // X_domainMap is always a domain Map view of the column Map
        // multivector.  In this case, the domain and column Maps are
        // the same, so X_domainMap _is_ X_colMap.
        X_domainMap = X_colMap;
        if (! zeroInitialGuess) { // Don't copy if zero initial guess

          try {
            deep_copy(*X_domainMap , X); // Copy X into constant stride multivector
          } catch (std::exception& e) {
            std::ostringstream os;
            os << "Tpetra::CrsMatrix::reorderedGaussSeidelCopy: "
              "deep_copy(*X_domainMap, X) threw an exception: "
               << e.what () << ".";
            TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error, e.what ());
          }
        }
        copyBackOutput = true; // Don't forget to copy back at end.
        TPETRA_EFFICIENCY_WARNING(
          ! X.isConstantStride (),
          std::runtime_error,
          "gaussSeidelCopy: The current implementation of the Gauss-Seidel "
          "kernel requires that X and B both have constant stride.  Since X "
          "does not have constant stride, we had to make a copy.  This is a "
          "limitation of the current implementation and not your fault, but we "
          "still report it as an efficiency warning for your information.");
      }
    }
    else { // Column Map and domain Map are _not_ the same.
      X_colMap = getColumnMapMultiVector (X);
      X_domainMap = X_colMap->offsetViewNonConst (domainMap, 0);

#ifdef HAVE_TPETRA_DEBUG
      typename MV::dual_view_type X_colMap_view = X_colMap->getDualView ();
      typename MV::dual_view_type X_domainMap_view = X_domainMap->getDualView ();

      if (X_colMap->getLocalLength () != 0 && X_domainMap->getLocalLength ()) {
        TEUCHOS_TEST_FOR_EXCEPTION(
          X_colMap_view.h_view.ptr_on_device () != X_domainMap_view.h_view.ptr_on_device (),
          std::logic_error, "Tpetra::CrsMatrix::gaussSeidelCopy: "
          "Pointer to start of column Map view of X is not equal to pointer to "
          "start of (domain Map view of) X.  This may mean that "
          "Tpetra::MultiVector::offsetViewNonConst is broken.  "
          "Please report this bug to the Tpetra developers.");
      }

      TEUCHOS_TEST_FOR_EXCEPTION(
        X_colMap_view.dimension_0 () < X_domainMap_view.dimension_0 () ||
        X_colMap->getLocalLength () < X_domainMap->getLocalLength (),
        std::logic_error, "Tpetra::CrsMatrix::gaussSeidelCopy: "
        "X_colMap has fewer local rows than X_domainMap.  "
        "X_colMap_view.dimension_0() = " << X_colMap_view.dimension_0 ()
        << ", X_domainMap_view.dimension_0() = "
        << X_domainMap_view.dimension_0 ()
        << ", X_colMap->getLocalLength() = " << X_colMap->getLocalLength ()
        << ", and X_domainMap->getLocalLength() = "
        << X_domainMap->getLocalLength ()
        << ".  This means that Tpetra::MultiVector::offsetViewNonConst "
        "is broken.  Please report this bug to the Tpetra developers.");

      TEUCHOS_TEST_FOR_EXCEPTION(
        X_colMap->getNumVectors () != X_domainMap->getNumVectors (),
        std::logic_error, "Tpetra::CrsMatrix::gaussSeidelCopy: "
        "X_colMap has a different number of columns than X_domainMap.  "
        "X_colMap->getNumVectors() = " << X_colMap->getNumVectors ()
        << " != X_domainMap->getNumVectors() = "
        << X_domainMap->getNumVectors ()
        << ".  This means that Tpetra::MultiVector::offsetViewNonConst "
        "is broken.  Please report this bug to the Tpetra developers.");

      // TEUCHOS_TEST_FOR_EXCEPTION(
      //   X_colMap->getLocalMV ().getStride () !=
      //   X_domainMap->getLocalMV ().getStride (),
      //   std::logic_error,
      //   "Tpetra::CrsMatrix::gaussSeidelCopy: "
      //   "X_colMap has local stride " << X_colMap->getLocalMV ().getStride ()
      //   << ", which does not equal the local stride "
      //   << X_domainMap->getLocalMV ().getStride () << " of X_domainMap.  "
      //   "This means that Tpetra::MultiVector::offsetViewNonConst is broken.  "
      //   "Please report this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG

      if (zeroInitialGuess) {
        // No need for an Import, since we're filling with zeros.
        X_colMap->putScalar (STS::zero ());
      } else {
        // We could just copy X into X_domainMap.  However, that
        // wastes a copy, because the Import also does a copy (plus
        // communication).  Since the typical use case for
        // Gauss-Seidel is a small number of sweeps (2 is typical), we
        // don't want to waste that copy.  Thus, we do the Import
        // here, and skip the first Import in the first sweep.
        // Importing directly from X effects the copy into X_domainMap
        // (which is a view of X_colMap).
        X_colMap->doImport (X, *importer, INSERT);
      }
      copyBackOutput = true; // Don't forget to copy back at end.
    } // if column and domain Maps are (not) the same

    // The Gauss-Seidel / SOR kernel expects multivectors of constant
    // stride.  X_colMap is by construction, but B might not be.  If
    // it's not, we have to make a copy.
    RCP<const MV> B_in;
    if (B.isConstantStride ()) {
      B_in = rcpFromRef (B);
    }
    else {
      // Range Map and row Map are the same in this case, so we can
      // use the cached row Map multivector to store a constant stride
      // copy of B.
      RCP<MV> B_in_nonconst = getRowMapMultiVector (B, true);

      try {
        deep_copy(*B_in_nonconst, B);
      } catch (std::exception& e) {
        std::ostringstream os;
        os << "Tpetra::CrsMatrix::reorderedGaussSeidelCopy: "
          "deep_copy(*B_in_nonconst, B) threw an exception: "
           << e.what () << ".";
        TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error, e.what ());
      }
      B_in = rcp_const_cast<const MV> (B_in_nonconst);

      TPETRA_EFFICIENCY_WARNING(
        ! B.isConstantStride (),
        std::runtime_error,
        "gaussSeidelCopy: The current implementation requires that B have "
        "constant stride.  Since B does not have constant stride, we had to "
        "copy it into a separate constant-stride multivector.  This is a "
        "limitation of the current implementation and not your fault, but we "
        "still report it as an efficiency warning for your information.");
    }

    for (int sweep = 0; sweep < numSweeps; ++sweep) {
      if (! importer.is_null () && sweep > 0) {
        // We already did the first Import for the zeroth sweep above,
        // if it was necessary.
        X_colMap->doImport (*X_domainMap, *importer, INSERT);
      }

      // Do local Gauss-Seidel.
      if (direction != Symmetric) {
        if(rowIndices.is_null())
          this->template localGaussSeidel<ST, ST> (*B_in, *X_colMap, D,
                                                   dampingFactor,
                                                   localDirection);
        else
          this->template reorderedLocalGaussSeidel<ST, ST> (*B_in, *X_colMap, D, rowIndices,
                                                            dampingFactor,
                                                            localDirection);
      } else { // direction == Symmetric
        if(rowIndices.is_null()) {
          this->template localGaussSeidel<ST, ST> (*B_in, *X_colMap, D,
                                                   dampingFactor,
                                                   KokkosClassic::Forward);
          // mfh 18 Mar 2013: Aztec's implementation of "symmetric
          // Gauss-Seidel" does _not_ do an Import between the forward
          // and backward sweeps.  This makes symmetric Gauss-Seidel a
          // symmetric preconditioner if the matrix A is symmetric.  We
          // imitate Aztec's behavior here.
          this->template localGaussSeidel<ST, ST> (*B_in, *X_colMap, D,
                                                   dampingFactor,
                                                   KokkosClassic::Backward);
        }
        else {
          this->template reorderedLocalGaussSeidel<ST, ST> (*B_in, *X_colMap, D, rowIndices,
                                                            dampingFactor,
                                                            KokkosClassic::Forward);
          this->template reorderedLocalGaussSeidel<ST, ST> (*B_in, *X_colMap, D, rowIndices,
                                                            dampingFactor,
                                                            KokkosClassic::Backward);

        }
      }
    }

    if (copyBackOutput) {
      try {
        deep_copy(X , *X_domainMap); // Copy result back into X.
      } catch (std::exception& e) {
        std::ostringstream os;
        os << "Tpetra::CrsMatrix::reorderedGaussSeidelCopy: "
          "deep_copy(X, *X_domainMap) threw an exception: "
           << e.what () << ".";
        TEUCHOS_TEST_FOR_EXCEPTION(true, std::runtime_error, e.what ());
      }
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////
  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  template <class DomainScalar, class RangeScalar>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  localMultiply (const MultiVector<DomainScalar,LocalOrdinal,GlobalOrdinal,node_type>& X,
                 MultiVector<RangeScalar,LocalOrdinal,GlobalOrdinal,node_type>& Y,
                 Teuchos::ETransp mode,
                 RangeScalar alpha,
                 RangeScalar beta) const
  {
    using Teuchos::NO_TRANS;
#ifdef HAVE_TPETRA_DEBUG
    const char tfecfFuncName[] = "localMultiply: ";
#endif // HAVE_TPETRA_DEBUG
    typedef Teuchos::ScalarTraits<RangeScalar> RST;
#ifdef HAVE_TPETRA_DEBUG
    // KokkosClassic::MultiVector<DomainScalar,node_type> lclX = X.getLocalMV ();
    // KokkosClassic::MultiVector<RangeScalar,node_type> lclY = Y.getLocalMV ();

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      X.getNumVectors() != Y.getNumVectors(), std::runtime_error,
      ": X and Y must have the same number of columns (vectors).  ");

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      mode == NO_TRANS && X.getLocalLength () != getColMap ()->getNodeNumElements (),
      std::runtime_error, "NO_TRANS case: X has the wrong number of local rows.  "
      "X.getLocalLength() = " << X.getLocalLength () << " != getColMap()->"
      "getNodeNumElements() = " << getColMap ()->getNodeNumElements () << ".");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      mode == NO_TRANS && Y.getLocalLength () != getRowMap ()->getNodeNumElements (),
      std::runtime_error, "NO_TRANS case: Y has the wrong number of local rows.  "
      "Y.getLocalLength() = " << Y.getLocalLength () << " != getRowMap()->"
      "getNodeNumElements() = " << getRowMap ()->getNodeNumElements () << ".");

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      mode != NO_TRANS && X.getLocalLength () != getRowMap ()->getNodeNumElements (),
      std::runtime_error, "TRANS or CONJ_TRANS case: X has the wrong number of "
      "local rows.  X.getLocalLength() = " << X.getLocalLength () << " != "
      "getRowMap()->getNodeNumElements() = "
      << getRowMap ()->getNodeNumElements () << ".");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      mode != NO_TRANS && Y.getLocalLength () != getColMap ()->getNodeNumElements (),
      std::runtime_error, "TRANS or CONJ_TRANS case: X has the wrong number of "
      "local rows.  Y.getLocalLength() = " << Y.getLocalLength () << " != "
      "getColMap()->getNodeNumElements() = "
      << getColMap ()->getNodeNumElements () << ".");

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! isFillComplete (), std::runtime_error, ": It is incorrect to call this "
      "method unless the matrix is fill complete.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      X.isConstantStride() == false || Y.isConstantStride() == false,
      std::runtime_error, ": X and Y must be constant stride.");
    // If the two pointers are NULL, then they don't alias one
    // another, even though they are equal.
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      X.getDualView ().d_view.ptr_on_device () == Y.getDualView ().d_view.ptr_on_device () &&
      X.getDualView ().d_view.ptr_on_device () != NULL,
      std::runtime_error, ": X and Y may not alias one another.");
#endif
    //
    // Call the matvec
    if (beta == RST::zero()) {
      // Y = alpha*op(M)*X with overwrite semantics

      // FIXME (mfh 27 Mar 2014) What about CONJ_TRANS???
      if (mode != NO_TRANS) {
        Kokkos::MV_MultiplyTranspose (RST::zero (),
                                      Y.template getLocalView<DeviceType> (),
                                      alpha,
                                      k_lclMatrix_,
                                      X.template getLocalView<DeviceType> ());
      }
      else { // mode == NO_TRANS
        Kokkos::MV_Multiply (Y.template getLocalView<DeviceType> (),
                             alpha,
                             k_lclMatrix_,
                             X.template getLocalView<DeviceType> ());
      }
    }
    else {
      // Y = alpha*op(M) + beta*Y

      // FIXME (mfh 27 Mar 2014) What about CONJ_TRANS???
      if(mode != NO_TRANS) {
        Kokkos::MV_MultiplyTranspose (beta,
                                      Y.template getLocalView<DeviceType> (),
                                      alpha,
                                      k_lclMatrix_,
                                      X.template getLocalView<DeviceType> ());
      }
      else {
        Kokkos::MV_Multiply (beta,
                             Y.template getLocalView<DeviceType> (),
                             alpha,
                             k_lclMatrix_,
                             X.template getLocalView<DeviceType> ());
      }
    }
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  template <class DomainScalar, class RangeScalar>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  localGaussSeidel (const MultiVector<DomainScalar,LocalOrdinal,GlobalOrdinal,node_type>& B,
                    MultiVector<RangeScalar,LocalOrdinal,GlobalOrdinal,node_type>& X,
                    const MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,node_type>& D,
                    const RangeScalar& dampingFactor,
                    const KokkosClassic::ESweepDirection direction) const
  {
    KokkosClassic::MultiVector<DomainScalar,node_type> x = X.getLocalMV ();
    KokkosClassic::MultiVector<RangeScalar,node_type> b = B.getLocalMV ();
    KokkosClassic::MultiVector<RangeScalar,node_type> d = D.getLocalMV ();

    lclMatOps_->template gaussSeidel<DomainScalar, RangeScalar> (b, x, d, dampingFactor, direction);
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  template <class DomainScalar, class RangeScalar>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  reorderedLocalGaussSeidel (const MultiVector<DomainScalar,LocalOrdinal,GlobalOrdinal,node_type>& B,
                             MultiVector<RangeScalar,LocalOrdinal,GlobalOrdinal,node_type>& X,
                             const MultiVector<Scalar,LocalOrdinal,GlobalOrdinal,node_type>& D,
                             const Teuchos::ArrayView<LocalOrdinal>& rowIndices,
                             const RangeScalar& dampingFactor,
                             const KokkosClassic::ESweepDirection direction) const
  {
    KokkosClassic::MultiVector<DomainScalar,node_type> x = X.getLocalMV ();
    KokkosClassic::MultiVector<RangeScalar,node_type> b = B.getLocalMV ();
    KokkosClassic::MultiVector<RangeScalar,node_type> d = D.getLocalMV ();

    lclMatOps_->template reorderedGaussSeidel<DomainScalar, RangeScalar> (b, x, d, rowIndices, dampingFactor, direction);
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  template <class DomainScalar, class RangeScalar>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  localSolve (const MultiVector<RangeScalar,LocalOrdinal,GlobalOrdinal,node_type>& Y,
              MultiVector<DomainScalar,LocalOrdinal,GlobalOrdinal,node_type>& X,
              Teuchos::ETransp mode) const
  {
    using Teuchos::NO_TRANS;
#ifdef HAVE_TPETRA_DEBUG
    const char tfecfFuncName[] = "localSolve()";
#endif // HAVE_TPETRA_DEBUG

    KokkosClassic::MultiVector<RangeScalar,node_type> lclY = Y.getLocalMV ();
    KokkosClassic::MultiVector<DomainScalar,node_type> lclX = X.getLocalMV ();
#ifdef HAVE_TPETRA_DEBUG
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(!isFillComplete(),                                              std::runtime_error, " until fillComplete() has been called.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(X.getNumVectors() != Y.getNumVectors(),                         std::runtime_error, ": X and Y must have the same number of vectors.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(X.isConstantStride() == false || Y.isConstantStride() == false, std::runtime_error, ": X and Y must be constant stride.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(isUpperTriangular() == false && isLowerTriangular() == false,   std::runtime_error, ": can only solve() triangular matrices.");
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(STS::isComplex && mode == Teuchos::TRANS,      std::logic_error, " does not currently support transposed solve for complex scalar types.");
#endif
    //
    // Call the solve
    if (mode == Teuchos::NO_TRANS) {
      lclMatOps_->template solve<DomainScalar,RangeScalar>(Teuchos::NO_TRANS, lclY, lclX);
    }
    else {
      lclMatOps_->template solve<DomainScalar,RangeScalar>(Teuchos::CONJ_TRANS, lclY, lclX);
    }
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  template <class T>
  Teuchos::RCP<CrsMatrix<
    T, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  convert () const
  {
    using Teuchos::ArrayRCP;
    using Teuchos::as;
    using Teuchos::RCP;
    using Teuchos::rcp;
    typedef CrsMatrix<
      T, LocalOrdinal, GlobalOrdinal, node_type,
      typename KokkosClassic::DefaultKernels<
        T, LocalOrdinal, node_type>::SparseOps> out_mat_type;
    typedef typename out_mat_type::t_ValuesType out_vals_type;
    typedef typename out_mat_type::k_local_matrix_type out_lcl_mat_type;
    typedef ArrayRCP<size_t>::size_type size_type;
    const char tfecfFuncName[] = "convert";

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      isFillComplete () == false, std::runtime_error,
      ": fill must be complete.");

    // mfh 27 Feb 2014: It seems reasonable that if this matrix has a
    // const graph, then the returned matrix should also.  However, if
    // this matrix does not have a const graph, then neither should
    // the returned matrix.  The code below implements this strategy.

    RCP<out_mat_type> newmat; // the matrix to return

    if (this->isStaticGraph ()) {
      // This matrix has a const graph, so the returned matrix should too.
      newmat = rcp (new out_mat_type (this->getCrsGraph ()));

      // Convert the values from Scalar to T, and stuff them directly
      // into the matrix to return.
      const size_type numVals =
        static_cast<size_type> (this->k_lclMatrix_.values.dimension_0 ());

      // FIXME (mfh 05 Aug 2014) Write a copy kernel (scalar_type and
      // T differ, so we can't use Kokkos::deep_copy).
      //
      // FIXME (mfh 05 Aug 2014) This assumes UVM.
      out_vals_type newVals1D ("Tpetra::CrsMatrix::val", numVals);
      for (size_type k = 0; k < numVals; ++k) {
        newVals1D(k) = static_cast<T> (this->k_values1D_(k));
      }
      newmat->k_lclMatrix_ =
        out_lcl_mat_type ("Tpetra::CrsMatrix::k_lclMatrix_",
                          this->k_lclMatrix_.numCols (), newVals1D,
                          this->k_lclMatrix_.graph);
      newmat->k_values1D_ = newVals1D;
      newmat->values1D_ = Kokkos::Compat::persistingView (newVals1D);
      // Since newmat has a static (const) graph, the graph already
      // has a column Map, and Import and Export objects already exist
      // (if applicable).  Thus, calling fillComplete is cheap.
      newmat->fillComplete (this->getDomainMap (), this->getRangeMap ());
    }
    else {
      // This matrix has a nonconst graph, so the returned matrix
      // should also have a nonconst graph.  However, it's fine for
      // the returned matrix to have static profile.  This will
      // certainly speed up its fillComplete.

      //
      // FIXME (mfh 05 Aug 2014) Instead of the slow stuff below, we
      // should copy the values and existing graph into a new local
      // matrix (lclMatrix), and then use the Tpetra::CrsMatrix
      // constructor that takes (rowMap, colMap, lclMatrix, params).
      //

      // Get this matrix's local data.
      ArrayRCP<const size_t> ptr;
      ArrayRCP<const LocalOrdinal> ind;
      ArrayRCP<const Scalar> oldVal;
      this->getAllValues (ptr, ind, oldVal);

      RCP<const map_type> rowMap = this->getRowMap ();
      RCP<const map_type> colMap = this->getColMap ();

      // Get an array of the number of entries in each (locally owned)
      // row, so that we can make the new matrix with static profile.
      const size_type numLocalRows =
        static_cast<size_type> (rowMap->getNodeNumElements ());
      ArrayRCP<size_t> numEntriesPerRow (numLocalRows);
      for (size_type localRow = 0; localRow < numLocalRows; ++localRow) {
        numEntriesPerRow[localRow] =
          static_cast<size_type> (getNumEntriesInLocalRow (localRow));
      }

      newmat = rcp (new out_mat_type (rowMap, colMap, numEntriesPerRow,
                                      StaticProfile));

      // Convert this matrix's values from Scalar to T.
      const size_type numVals = this->k_lclMatrix_.values.dimension_0 ();
      ArrayRCP<T> newVals1D (numVals);
      // FIXME (mfh 05 Aug 2014) This assumes UVM.
      for (size_type k = 0; k < numVals; ++k) {
        newVals1D[k] = static_cast<T> (this->k_values1D_(k));
      }

      // Give this matrix all of its local data.  We can all this
      // method because newmat was _not_ created with a const graph.
      // The data must be passed in as nonconst, so we have to copy it
      // first.
      ArrayRCP<size_t> newPtr (ptr.size ());
      std::copy (ptr.begin (), ptr.end (), newPtr.begin ());
      ArrayRCP<LocalOrdinal> newInd (ind.size ());
      std::copy (ind.begin (), ind.end (), newInd.begin ());
      newmat->setAllValues (newPtr, newInd, newVals1D);

      // We already have the Import and Export (if applicable) objects
      // from the graph, so we can save a lot of time by passing them
      // in to expertStaticFillComplete.
      RCP<const map_type> domainMap = this->getDomainMap ();
      RCP<const map_type> rangeMap = this->getRangeMap ();
      RCP<const import_type> importer = this->getCrsGraph ()->getImporter ();
      RCP<const export_type> exporter = this->getCrsGraph ()->getExporter ();
      newmat->expertStaticFillComplete (domainMap, rangeMap, importer, exporter);
    }

    return newmat;
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  checkInternalState () const
  {
#ifdef HAVE_TPETRA_DEBUG
    const char tfecfFuncName[] = "checkInternalState: ";
    const char err[] = "Internal state is not consistent.  "
      "Please report this bug to the Tpetra developers.";
    RCP<Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > node = getNode();
    // check the internal state of this data structure
    // this is called by numerous state-changing methods, in a debug build, to ensure that the object
    // always remains in a valid state

    // we must have a static graph
    //
    // a dynamic graph, depending on which constructor was used.
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      staticGraph_.is_null (),
      std::logic_error, err);
    // if active, i have no local sparse ops
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      isFillActive () && ! lclMatOps_.is_null (),
      std::logic_error, err << "  Specifically, the matrix is fill active "
      "(isFillActive() returns true) but its lclMatOps_ object is NOT null.");
    // if filled, i have a local sparse ops
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      isFillComplete() && lclMatOps_.is_null (),
      std::logic_error, err );
    // myGraph == null means that the matrix has a static graph.
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      ! myGraph_.is_null () && myGraph_ != staticGraph_,
      std::logic_error, err);
    // if matrix is fill complete, then graph must be fill complete
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      isFillComplete () && ! staticGraph_->isFillComplete (),
      std::logic_error, err << "  Specifically, the matrix is fill complete, "
      "but its graph is NOT fill complete.");
    // if matrix is storage optimized, it should have a 1D allocation
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      isStorageOptimized () && ! values2D_.is_null (),
      std::logic_error, err);
    // if matrix/graph are static profile, then 2D allocation should not be present
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      getProfileType() == StaticProfile && values2D_ != null,
      std::logic_error, err);
    // if matrix/graph are dynamic profile, then 1D allocation should not be present
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      getProfileType() == DynamicProfile && k_values1D_.dimension_0 () > 0,
      std::logic_error, err);
    // if values are allocated and they are non-zero in number, then
    // one of the allocations should be present
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      staticGraph_->indicesAreAllocated () &&
      staticGraph_->getNodeAllocationSize() > 0 &&
      staticGraph_->getNodeNumRows() > 0
      && values2D_.is_null () &&
      k_values1D_.dimension_0 () == 0,
      std::logic_error, err);
    // we cannot have both a 1D and 2D allocation
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      k_values1D_.dimension_0 () > 0 && values2D_ != null,
      std::logic_error, err << "  Specifically, k_values1D_ is allocated (has "
      "size " << k_values1D_.dimension_0 () << " > 0) and values2D_ is also "
      "allocated.  CrsMatrix is not suppose to have both a 1-D and a 2-D "
      "allocation at the same time.");
#endif
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  std::string
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  description () const
  {
    std::ostringstream os;

    os << "Tpetra::CrsMatrix (Kokkos refactor): {";
    if (this->getObjectLabel () != "") {
      os << "Label: \"" << this->getObjectLabel () << "\", ";
    }
    if (isFillComplete()) {
      os << "isFillComplete: true"
         << ", global dimensions: [" << getGlobalNumRows () << ", "
         << getGlobalNumCols () << "]"
         << ", global number of entries: " << getGlobalNumEntries ()
         << "}";
    }
    else {
      os << "isFillComplete: false"
         << ", global dimensions: [" << getGlobalNumRows () << ", "
         << getGlobalNumCols () << "]}";
    }
    return os.str ();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  describe (Teuchos::FancyOStream &out,
            const Teuchos::EVerbosityLevel verbLevel) const
  {
    using std::endl;
    using std::setw;
    using Teuchos::as;
    using Teuchos::Comm;
    using Teuchos::RCP;
    using Teuchos::TypeNameTraits;
    using Teuchos::VERB_DEFAULT;
    using Teuchos::VERB_NONE;
    using Teuchos::VERB_LOW;
    using Teuchos::VERB_MEDIUM;
    using Teuchos::VERB_HIGH;
    using Teuchos::VERB_EXTREME;

    const Teuchos::EVerbosityLevel vl = (verbLevel == VERB_DEFAULT) ? VERB_LOW : verbLevel;

    if (vl == VERB_NONE) {
      return; // Don't print anything at all
    }
    // By convention, describe() always begins with a tab.
    Teuchos::OSTab tab0 (out);

    RCP<const Comm<int> > comm = this->getComm();
    const int myRank = comm->getRank();
    const int numProcs = comm->getSize();
    size_t width = 1;
    for (size_t dec=10; dec<getGlobalNumRows(); dec *= 10) {
      ++width;
    }
    width = std::max<size_t> (width, as<size_t> (11)) + 2;

    //    none: print nothing
    //     low: print O(1) info from node 0
    //  medium: print O(P) info, num entries per process
    //    high: print O(N) info, num entries per row
    // extreme: print O(NNZ) info: print indices and values
    //
    // for medium and higher, print constituent objects at specified verbLevel
    if (myRank == 0) {
      out << "Tpetra::CrsMatrix (Kokkos refactor):" << endl;
    }
    Teuchos::OSTab tab1 (out);

    if (myRank == 0) {
      if (this->getObjectLabel () != "") {
        out << "Label: \"" << this->getObjectLabel () << "\", ";
      }
      {
        out << "Template parameters:" << endl;
        Teuchos::OSTab tab2 (out);
        out << "Scalar: " << TypeNameTraits<Scalar>::name () << endl
            << "LocalOrdinal: " << TypeNameTraits<LocalOrdinal>::name () << endl
            << "GlobalOrdinal: " << TypeNameTraits<GlobalOrdinal>::name () << endl
            << "Node: " << Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>::name () << endl;
      }
      if (isFillComplete()) {
        out << "isFillComplete: true" << endl
            << "Global dimensions: [" << getGlobalNumRows () << ", "
            << getGlobalNumCols () << "]" << endl
            << "Global number of entries: " << getGlobalNumEntries () << endl
            << "Global number of diagonal entries: " << getGlobalNumDiags ()
            << endl << "Global max number of entries in a row: "
            << getGlobalMaxNumRowEntries () << endl;
      }
      else {
        out << "isFillComplete: false" << endl
            << "Global dimensions: [" << getGlobalNumRows () << ", "
            << getGlobalNumCols () << "]" << endl;
      }
    }

    if (vl < VERB_MEDIUM) {
      return; // all done!
    }

    // Describe the Map Map.
    if (myRank == 0) {
      out << endl << "Row Map:" << endl;
    }
    getRowMap ()->describe (out, vl);

    // Describe the column Map.
    if (myRank == 0) {
      out << "Column Map: ";
    }
    if (getColMap ().is_null ()) {
      if (myRank == 0) {
        out << "null" << endl;
      }
    } else if (getColMap () == getRowMap ()) {
      if (myRank == 0) {
        out << "same as row Map" << endl;
      }
    } else {
      if (myRank == 0) {
        out << endl;
      }
      getColMap ()->describe (out, vl);
    }

    // Describe the domain Map.
    if (myRank == 0) {
      out << "Domain Map: ";
    }
    if (getDomainMap ().is_null ()) {
      if (myRank == 0) {
        out << "null" << endl;
      }
    } else if (getDomainMap () == getRowMap ()) {
      if (myRank == 0) {
        out << "same as row Map" << endl;
      }
    } else if (getDomainMap () == getColMap ()) {
      if (myRank == 0) {
        out << "same as column Map" << endl;
      }
    } else {
      if (myRank == 0) {
        out << endl;
      }
      getColMap ()->describe (out, vl);
    }

    // Describe the range Map.
    if (myRank == 0) {
      out << "Range Map: ";
    }
    if (getRangeMap ().is_null ()) {
      if (myRank == 0) {
        out << "null" << endl;
      }
    } else if (getRangeMap () == getDomainMap ()) {
      if (myRank == 0) {
        out << "same as domain Map" << endl;
      }
    } else if (getRangeMap () == getRowMap ()) {
      if (myRank == 0) {
        out << "same as row Map" << endl;
      }
    } else {
      if (myRank == 0) {
        out << endl;
      }
      getColMap ()->describe (out, vl);
    }

    // O(P) data
    for (int curRank = 0; curRank < numProcs; ++curRank) {
      if (myRank == curRank) {
        out << "Process rank: " << curRank << endl;
        Teuchos::OSTab tab2 (out);
        if (! staticGraph_->indicesAreAllocated ()) {
          out << "Graph indices not allocated" << endl;
        }
        else {
          out << "Number of allocated entries: "
              << staticGraph_->getNodeAllocationSize () << endl;
        }
        out << "Number of entries: " << getNodeNumEntries () << endl;
        if (isFillComplete ()) {
          out << "Number of diagonal entries: " << getNodeNumDiags () << endl;
        }
        out << "Max number of entries per row: " << getNodeMaxNumRowEntries ()
            << endl;
      }
      // Give output time to complete by executing some barriers.
      comm->barrier ();
      comm->barrier ();
      comm->barrier ();
    }

    if (vl < VERB_HIGH) {
      return; // all done!
    }

    // O(N) and O(NNZ) data
    for (int curRank = 0; curRank < numProcs; ++curRank) {
      if (myRank == curRank) {
        out << std::setw(width) << "Proc Rank"
            << std::setw(width) << "Global Row"
            << std::setw(width) << "Num Entries";
        if (vl == VERB_EXTREME) {
          out << std::setw(width) << "(Index,Value)";
        }
        out << endl;
        for (size_t r = 0; r < getNodeNumRows (); ++r) {
          const size_t nE = getNumEntriesInLocalRow(r);
          GlobalOrdinal gid = getRowMap()->getGlobalElement(r);
          out << std::setw(width) << myRank
              << std::setw(width) << gid
              << std::setw(width) << nE;
          if (vl == VERB_EXTREME) {
            if (isGloballyIndexed()) {
              ArrayView<const GlobalOrdinal> rowinds;
              ArrayView<const Scalar> rowvals;
              getGlobalRowView (gid, rowinds, rowvals);
              for (size_t j = 0; j < nE; ++j) {
                out << " (" << rowinds[j]
                    << ", " << rowvals[j]
                    << ") ";
              }
            }
            else if (isLocallyIndexed()) {
              ArrayView<const LocalOrdinal> rowinds;
              ArrayView<const Scalar> rowvals;
              getLocalRowView (r, rowinds, rowvals);
              for (size_t j=0; j < nE; ++j) {
                out << " (" << getColMap()->getGlobalElement(rowinds[j])
                    << ", " << rowvals[j]
                    << ") ";
              }
            } // globally or locally indexed
          } // vl == VERB_EXTREME
          out << endl;
        } // for each row r on this process

        // Print the optimized sparse kernels object, if applicable.
        // That has O(NNZ) data.
        if (vl == VERB_EXTREME && ! lclMatOps_.is_null ()) {
          lclMatOps_->describe (out, vl);
        }
      } // if (myRank == curRank)

      // Give output time to complete
      comm->barrier ();
      comm->barrier ();
      comm->barrier ();
    } // for each process p
  }


  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal, class DeviceType>
  bool
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,
            Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
            typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  checkSizes (const SrcDistObject& source)
  {
    // It's not clear what kind of compatibility checks on sizes can
    // be performed here.  Epetra_CrsGraph doesn't check any sizes for
    // compatibility.

    // Currently, the source object must be a RowMatrix with the same
    // four template parameters as the target CrsMatrix.  We might
    // relax this requirement later.
    typedef RowMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > row_matrix_type;
    const row_matrix_type* srcRowMat =
      dynamic_cast<const row_matrix_type*> (&source);
    return (srcRowMat != NULL);
  }


  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  copyAndPermute (const SrcDistObject& source,
                  size_t numSameIDs,
                  const ArrayView<const LocalOrdinal> &permuteToLIDs,
                  const ArrayView<const LocalOrdinal> &permuteFromLIDs)
  {
    using Teuchos::Array;
    using Teuchos::ArrayView;
    using Teuchos::as;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    typedef node_type NT;
    // Method name string for TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC.
    const char tfecfFuncName[] = "copyAndPermute";

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      permuteToLIDs.size() != permuteFromLIDs.size(),
      std::invalid_argument, ": permuteToLIDs.size() = " << permuteToLIDs.size()
      << "!= permuteFromLIDs.size() = " << permuteFromLIDs.size() << ".");

    // This dynamic cast should succeed, because we've already tested
    // it in checkSizes().
    typedef RowMatrix<Scalar, LO, GO, NT> row_matrix_type;
    const row_matrix_type& srcMat = dynamic_cast<const row_matrix_type&> (source);

    const bool sourceIsLocallyIndexed = srcMat.isLocallyIndexed ();
    //
    // Copy the first numSame row from source to target (this matrix).
    // This involves copying rows corresponding to LIDs [0, numSame-1].
    //
    const map_type& srcRowMap = * (srcMat.getRowMap ());
    Array<GO> rowInds;
    Array<Scalar> rowVals;
    const LO numSameIDs_as_LID = as<LO> (numSameIDs);
    for (LO sourceLID = 0; sourceLID < numSameIDs_as_LID; ++sourceLID) {
      // Global ID for the current row index in the source matrix.
      // The first numSameIDs GIDs in the two input lists are the
      // same, so sourceGID == targetGID in this case.
      const GO sourceGID = srcRowMap.getGlobalElement (sourceLID);
      const GO targetGID = sourceGID;

      // Input views for the combineGlobalValues() call below.
      ArrayView<const GO> rowIndsConstView;
      ArrayView<const Scalar> rowValsConstView;

      if (sourceIsLocallyIndexed) {
        const size_t rowLength = srcMat.getNumEntriesInGlobalRow (sourceGID);
        if (rowLength > as<size_t> (rowInds.size())) {
          rowInds.resize (rowLength);
          rowVals.resize (rowLength);
        }
        // Resizing invalidates an Array's views, so we must make new
        // ones, even if rowLength hasn't changed.
        ArrayView<GO> rowIndsView = rowInds.view (0, rowLength);
        ArrayView<Scalar> rowValsView = rowVals.view (0, rowLength);

        // The source matrix is locally indexed, so we have to get a
        // copy.  Really it's the GIDs that have to be copied (because
        // they have to be converted from LIDs).
        size_t checkRowLength = 0;
        srcMat.getGlobalRowCopy (sourceGID, rowIndsView, rowValsView, checkRowLength);

#ifdef HAVE_TPETRA_DEBUG
        TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(rowLength != checkRowLength,
          std::logic_error, ": For global row index " << sourceGID << ", the source"
          " matrix's getNumEntriesInGlobalRow() method returns a row length of "
          << rowLength << ", but the getGlobalRowCopy() method reports that "
          "the row length is " << checkRowLength << ".  Please report this bug "
          "to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG

        rowIndsConstView = rowIndsView.view (0, rowLength);
        rowValsConstView = rowValsView.view (0, rowLength);
      }
      else { // source matrix is globally indexed.
        srcMat.getGlobalRowView (sourceGID, rowIndsConstView, rowValsConstView);
      }

      // Combine the data into the target matrix.
      if (isStaticGraph()) {
        // Applying a permutation to a matrix with a static graph
        // means REPLACE-ing entries.
        combineGlobalValues (targetGID, rowIndsConstView, rowValsConstView, REPLACE);
      }
      else {
        // Applying a permutation to a matrix with a dynamic graph
        // means INSERT-ing entries.  This has the same effect as
        // ADD, if the target graph already has an entry there.
        combineGlobalValues (targetGID, rowIndsConstView, rowValsConstView, INSERT);
      }
    } // For each of the consecutive source and target IDs that are the same

    //
    // Permute the remaining rows.
    //
    const map_type& tgtRowMap = * (this->getRowMap ());
    const size_t numPermuteToLIDs = as<size_t> (permuteToLIDs.size ());
    for (size_t p = 0; p < numPermuteToLIDs; ++p) {
      const GO sourceGID = srcRowMap.getGlobalElement (permuteFromLIDs[p]);
      const GO targetGID = tgtRowMap.getGlobalElement (permuteToLIDs[p]);

      // Input views for the combineGlobalValues() call below.
      ArrayView<const GO> rowIndsConstView;
      ArrayView<const Scalar> rowValsConstView;

      if (sourceIsLocallyIndexed) {
        const size_t rowLength = srcMat.getNumEntriesInGlobalRow (sourceGID);
        if (rowLength > as<size_t> (rowInds.size ())) {
          rowInds.resize (rowLength);
          rowVals.resize (rowLength);
        }
        // Resizing invalidates an Array's views, so we must make new
        // ones, even if rowLength hasn't changed.
        ArrayView<GO> rowIndsView = rowInds.view (0, rowLength);
        ArrayView<Scalar> rowValsView = rowVals.view (0, rowLength);

        // The source matrix is locally indexed, so we have to get a
        // copy.  Really it's the GIDs that have to be copied (because
        // they have to be converted from LIDs).
        size_t checkRowLength = 0;
        srcMat.getGlobalRowCopy (sourceGID, rowIndsView, rowValsView, checkRowLength);

#ifdef HAVE_TPETRA_DEBUG
        TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(rowLength != checkRowLength,
          std::logic_error, ": For the source matrix's global row index "
          << sourceGID << ", the source matrix's getNumEntriesInGlobalRow() method "
          "returns a row length of " << rowLength << ", but the "
          "getGlobalRowCopy() method reports that the row length is "
          << checkRowLength << ".  Please report this bug to the Tpetra "
          "developers.");
#endif // HAVE_TPETRA_DEBUG

        rowIndsConstView = rowIndsView.view (0, rowLength);
        rowValsConstView = rowValsView.view (0, rowLength);
      }
      else {
        srcMat.getGlobalRowView (sourceGID, rowIndsConstView, rowValsConstView);
      }

      // Combine the data into the target matrix.
      if (isStaticGraph()) {
        this->combineGlobalValues (targetGID, rowIndsConstView,
                                   rowValsConstView, REPLACE);
      }
      else {
        this->combineGlobalValues (targetGID, rowIndsConstView,
                                   rowValsConstView, INSERT);
      }
    } // For each ID to permute
  }


  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  packAndPrepare (const SrcDistObject& source,
                  const Teuchos::ArrayView<const LocalOrdinal>& exportLIDs,
                  Teuchos::Array<char>& exports,
                  const Teuchos::ArrayView<size_t>& numPacketsPerLID,
                  size_t& constantNumPackets,
                  Distributor& distor)
  {
    using Teuchos::Array;
    using Teuchos::ArrayView;
    using Teuchos::as;
    using Teuchos::av_reinterpret_cast;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    //typedef typename ArrayView<const LO>::size_type size_type; // unused
    const char tfecfFuncName[] = "packAndPrepare";

    // Attempt to cast the source object to RowMatrix.  If the cast
    // succeeds, use the source object's pack method to pack its data
    // for communication.  If the source object is really a CrsMatrix,
    // this will pick up the CrsMatrix's more efficient override.  If
    // the RowMatrix cast fails, then the source object doesn't have
    // the right type.
    //
    // FIXME (mfh 30 Jun 2013) We don't even need the RowMatrix to
    // have the same Node type.  Unfortunately, we don't have a way to
    // ask if the RowMatrix is "a RowMatrix with any Node type," since
    // RowMatrix doesn't have a base class.  A hypothetical
    // RowMatrixBase<Scalar, LO, GO> class, which does not currently
    // exist, would satisfy this requirement.
    //
    // Why RowMatrixBase<Scalar, LO, GO>?  The source object's Scalar
    // type doesn't technically need to match the target object's
    // Scalar type, so we could just have RowMatrixBase<LO, GO>.  LO
    // and GO need not be the same, as long as there is no overflow of
    // the indices.  However, checking for index overflow is global
    // and therefore undesirable.
    typedef RowMatrix<Scalar, LO, GO,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > row_matrix_type;
    const row_matrix_type* srcRowMat =
      dynamic_cast<const row_matrix_type*> (&source);
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      srcRowMat == NULL, std::invalid_argument,
      ": The source object of the Import or Export operation is neither a "
      "CrsMatrix (with the same template parameters as the target object), "
      "nor a RowMatrix (with the same first four template parameters as the "
      "target object).");
    srcRowMat->pack (exportLIDs, exports, numPacketsPerLID,
                     constantNumPackets, distor);
  }


  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  pack (const Teuchos::ArrayView<const LocalOrdinal>& exportLIDs,
        Teuchos::Array<char>& exports,
        const Teuchos::ArrayView<size_t>& numPacketsPerLID,
        size_t& constantNumPackets,
        Distributor &distor) const
  {
    using Teuchos::Array;
    using Teuchos::ArrayView;
    using Teuchos::as;
    using Teuchos::av_reinterpret_cast;
    using Teuchos::RCP;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    typedef typename ArrayView<const LO>::size_type size_type;
    const char tfecfFuncName[] = "pack";

    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      exportLIDs.size() != numPacketsPerLID.size(),
      std::invalid_argument, ": exportLIDs.size() = " << exportLIDs.size()
      << "!= numPacketsPerLID.size() = " << numPacketsPerLID.size() << ".");

    // Get a reference to the matrix's row Map.
    const map_type& rowMap = * (this->getRowMap ());

    const bool locallyIndexed = this->isLocallyIndexed ();
    constantNumPackets = 0;

    // Get the GIDs of the rows we want to pack.
    Array<GO> exportGIDs (exportLIDs.size ());
    const size_type numExportGIDs = exportGIDs.size ();
    for (size_type i = 0; i < numExportGIDs; ++i) {
      exportGIDs[i] = rowMap.getGlobalElement (exportLIDs[i]);
    }

    // We say "Packet" is char (really a "byte"), but the actual unit
    // of packing is a (GID, value) pair.  The GID is the column index
    // in that row of the sparse matrix, and the value is the value at
    // that entry of the sparse matrix.  Thus, we have to scale
    // numPacketsPerLID by the number of bytes in a _packed_ (GID,
    // value) pair.  (We pack the GID and value in each pair
    // separately, so the number of bytes in a packed pair is actually
    // sizeof(GO) + sizeof(Scalar).)
    //
    // FIXME (mfh 24 Feb 2013) This code is only correct if
    // sizeof(Scalar) is a meaningful representation of the amount of
    // data in a Scalar instance.  (GO is always a built-in integer
    // type.)
    //
    // Compute the number of packets per export LID, and accumulate
    // the total number of packages.  While doing so, find the max
    // number of entries in each row owned by this process; we will
    // use that to size temporary arrays below.
    const size_t sizeOfOrdValPair = sizeof (GO) + sizeof (Scalar);
    size_t totalNumEntries = 0;
    size_t maxRowLength = 0;
    for (size_type i = 0; i < exportGIDs.size(); ++i) {
      const size_t curNumEntries =
        this->getNumEntriesInGlobalRow (exportGIDs[i]);
      numPacketsPerLID[i] = curNumEntries * sizeOfOrdValPair;
      totalNumEntries += curNumEntries;
      maxRowLength = std::max (curNumEntries, maxRowLength);
    }

    // Pack export data by interleaving rows' indices and values in
    // the following way:
    //
    // [inds_row0 vals_row0 inds_row1 vals_row1 ... ]
    if (totalNumEntries > 0) {
      // exports is an array of char (bytes), so scale the total
      // number of entries by the number of bytes per entry (where
      // "entry" includes both the column index and the value).
      const size_t totalNumBytes = totalNumEntries * sizeOfOrdValPair;
      exports.resize (totalNumBytes);

      // Current position in the 'exports' output array.
      size_t curOffsetInBytes = 0;

      // For each row of the matrix owned by the calling process, pack
      // that row's column indices and values into the exports array.
      // If the matrix is globally indexed, we can use view semantics
      // (getGlobalRowView), which should be faster than copy
      // semantics (getGlobalRowCopy).  Otherwise, we'll have to use
      // copy semantics.
      //
      // FIXME (mfh 28 Jun 2013) This could be made a (shared-memory)
      // parallel kernel, by using the CSR data layout to calculate
      // positions in the output buffer.
      if (locallyIndexed) {
        // Locally indexed matrices always have a column Map.
        const map_type& colMap = * (this->getColMap ());

        // Views of the column LIDs and values in each row.  It's
        // worth creating empty views here, because they aren't
        // returned by getLocalRowView; that method will modify (set)
        // them in place.
        ArrayView<const LO> lidsView;
        ArrayView<const Scalar> valsView;

        // Temporary buffer for a copy of the column indices (as GIDs)
        // in each row.  Import and Export operations to a CrsMatrix
        // target currently expect GIDs, not LIDs.
        //
        // FIXME (mfh 30 Jun 2013) If the source and target have the
        // same column Maps, it would make sense to pack column
        // indices as LIDs instead of GIDs.  Packing them as GIDs is
        // correct, but it's inefficient to convert LIDs to GIDs and
        // then back again on receipt.  Furthermore, GIDs might be
        // larger than LIDs, thus costing more bandwidth.
        Array<GO> gids (as<size_type> (maxRowLength));

        const size_type numExportLIDs = exportLIDs.size ();
        for (size_type i = 0; i < numExportLIDs; ++i) {
          // Get a (locally indexed) view of the current row's data.
          this->getLocalRowView (exportLIDs[i], lidsView, valsView);

          // Convert column indices as LIDs to column indices as GIDs.
          const size_type curNumEntries = lidsView.size ();
          ArrayView<GO> gidsView = gids (0, curNumEntries);
          for (size_type k = 0; k < curNumEntries; ++k) {
            gidsView[k] = colMap.getGlobalElement (lidsView[k]);
          }

          // Get views of the spots in the exports array in which to
          // put the indices resp. values.  The type cast makes the
          // views look like GO resp. Scalar, when the array they are
          // viewing is really an array of char.
          ArrayView<char> gidsViewOutChar =
            exports (curOffsetInBytes,
                     as<size_t> (curNumEntries) * sizeof (GO));
          ArrayView<char> valsViewOutChar =
            exports (curOffsetInBytes + as<size_t> (curNumEntries) * sizeof (GO),
                     as<size_t> (curNumEntries) * sizeof (Scalar));
          ArrayView<GO> gidsViewOut = av_reinterpret_cast<GO> (gidsViewOutChar);
          ArrayView<Scalar> valsViewOut = av_reinterpret_cast<Scalar> (valsViewOutChar);

          // Copy the row's data into the views of the exports array.
          std::copy (gidsView.begin (),
                     gidsView.begin () + as<size_type> (curNumEntries),
                     gidsViewOut.begin ());
          std::copy (valsView.begin (),
                     valsView.begin () + as<size_type> (curNumEntries),
                     valsViewOut.begin ());
          // Keep track of how many bytes we packed.
          curOffsetInBytes += sizeOfOrdValPair * curNumEntries;
        }
      }
      else { // the matrix is globally indexed
        ArrayView<const GO> gidsView;
        ArrayView<const Scalar> valsView;

        const size_type numExportLIDs = exportLIDs.size ();
        for (size_type i = 0; i < numExportLIDs; ++i) {
          // Get a view of the current row's data.
          this->getGlobalRowView (exportGIDs[i], gidsView, valsView);
          const size_t curNumEntries = as<size_t> (gidsView.size ());
          // Get views of the spots in the exports array in which to
          // put the indices resp. values.  See notes and FIXME above.

          ArrayView<char> gidsViewOutChar =
            exports (curOffsetInBytes, curNumEntries * sizeof (GO));
          ArrayView<char> valsViewOutChar =
            exports (curOffsetInBytes + curNumEntries * sizeof (GO),
                     curNumEntries * sizeof (Scalar));
          ArrayView<GO> gidsViewOut = av_reinterpret_cast<GO> (gidsViewOutChar);
          ArrayView<Scalar> valsViewOut = av_reinterpret_cast<Scalar> (valsViewOutChar);

          // Copy the row's data into the views of the exports array.
          std::copy (gidsView.begin (), gidsView.end (), gidsViewOut.begin ());
          std::copy (valsView.begin (), valsView.end (), valsViewOut.begin ());
          // Keep track of how many bytes we packed.
          curOffsetInBytes += sizeOfOrdValPair * curNumEntries;
        }
      }

#ifdef HAVE_TPETRA_DEBUG
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(curOffsetInBytes != totalNumBytes,
        std::logic_error, ": At end of method, the final offset bytes count "
        "curOffsetInBytes=" << curOffsetInBytes << " does not equal the total "
        "number of bytes packed totalNumBytes=" << totalNumBytes << ".  Please "
        "report this bug to the Tpetra developers.");
#endif //  HAVE_TPETRA_DEBUG
    }
  }


  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  combineGlobalValues (const GlobalOrdinal globalRowIndex,
                       const ArrayView<const GlobalOrdinal> columnIndices,
                       const ArrayView<const Scalar> values,
                       const Tpetra::CombineMode combineMode)
  {
    if (isStaticGraph()) {
      // INSERT doesn't make sense for a static graph, since you
      // aren't allowed to change the structure of the graph.
      // However, all the other combine modes work.
      if (combineMode == ADD) {
        sumIntoGlobalValues (globalRowIndex, columnIndices, values);
      }
      else if (combineMode == REPLACE) {
        replaceGlobalValues (globalRowIndex, columnIndices, values);
      }
      else if (combineMode == ABSMAX) {
        using Details::AbsMax;
        AbsMax<Scalar> f;
        this->template transformGlobalValues<AbsMax<Scalar> > (globalRowIndex,
                                                               columnIndices(),
                                                               values(), f);
      }
      else if (combineMode == INSERT) {
        TEUCHOS_TEST_FOR_EXCEPTION(isStaticGraph() && combineMode == INSERT,
          std::invalid_argument, "combineGlobalValues: INSERT combine mode "
          "is not allowed if the matrix has a static graph (i.e., was "
          "constructed with the CrsMatrix constructor that takes a const "
          "CrsGraph pointer).");
      }
      else {
        TEUCHOS_TEST_FOR_EXCEPTION(true, std::logic_error,
          "combineGlobalValues: Invalid combine mode; should never get here!  "
          "Please report this bug to the Tpetra developers.");
      }
    }
    else { // The matrix has a dynamic graph.
      if (combineMode == ADD || combineMode == INSERT) {
        // For a dynamic graph, all incoming column indices are
        // inserted into the target graph.  Duplicate indices will
        // have their values summed.  In this context, ADD and INSERT
        // are equivalent.  We need to call insertGlobalValues()
        // anyway if the column indices don't yet exist in this row,
        // so we just call insertGlobalValues() for both cases.
        insertGlobalValuesFiltered (globalRowIndex, columnIndices, values);
      }
      // FIXME (mfh 14 Mar 2012):
      //
      // Implementing ABSMAX or REPLACE for a dynamic graph would
      // require modifying assembly to attach a possibly different
      // combine mode to each inserted (i, j, A_ij) entry.  For
      // example, consider two different Export operations to the same
      // target CrsMatrix, the first with ABSMAX combine mode and the
      // second with REPLACE.  This isn't a common use case, so we
      // won't mess with it for now.
      else if (combineMode == ABSMAX) {
        TEUCHOS_TEST_FOR_EXCEPTION(! isStaticGraph() && combineMode == ABSMAX,
          std::logic_error, "combineGlobalValues: ABSMAX combine mode when "
          "the matrix has a dynamic graph is not yet implemented.");
      }
      else if (combineMode == REPLACE) {
        TEUCHOS_TEST_FOR_EXCEPTION(! isStaticGraph() && combineMode == REPLACE,
          std::logic_error, "combineGlobalValues: REPLACE combine mode when "
          "the matrix has a dynamic graph is not yet implemented.");
      }
      else {
        TEUCHOS_TEST_FOR_EXCEPTION(true, std::logic_error,
          "combineGlobalValues: Should never get here!  Please report this bug"
          "to the Tpetra developers.");
      }
    }
  }

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal,
           class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  unpackAndCombine (const Teuchos::ArrayView<const LocalOrdinal>& importLIDs,
                    const Teuchos::ArrayView<const char>& imports,
                    const Teuchos::ArrayView<size_t>& numPacketsPerLID,
                    size_t constantNumPackets,
                    Distributor & /* distor */,
                    CombineMode combineMode)
  {
    using Teuchos::ArrayView;
    using Teuchos::av_reinterpret_cast;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    typedef typename ArrayView<const LO>::size_type size_type;
    const char tfecfFuncName[] = "unpackAndCombine";

#ifdef HAVE_TPETRA_DEBUG
    const CombineMode validModes[4] = {ADD, REPLACE, ABSMAX, INSERT};
    const char* validModeNames[4] = {"ADD", "REPLACE", "ABSMAX", "INSERT"};
    const int numValidModes = 4;

    if (std::find (validModes, validModes+numValidModes, combineMode) ==
        validModes+numValidModes) {
      std::ostringstream os;
      os << "unpackAndCombine: Invalid combine mode.  Valid modes are {";
      for (int k = 0; k < numValidModes; ++k) {
        os << validModeNames[k];
        if (k < numValidModes - 1) {
          os << ", ";
        }
      }
      os << "}.";
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(true, std::invalid_argument, os.str());
    }
#endif // HAVE_TPETRA_DEBUG
    TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(
      importLIDs.size() != numPacketsPerLID.size(),
      std::invalid_argument, "importLIDs.size() = " << importLIDs.size()
      << "!= numPacketsPerLID.size() = " << numPacketsPerLID.size() << ".");

    // FIXME (mfh 05 Dec 2012) Here are all the assumptions encoded in
    // the following line of code:
    //
    // 1. The data (index,value) for each element are packed tightly,
    //    with no extra space in between.
    //
    // 2. sizeof(Scalar) says how much data were used to represent a
    //    Scalar in its packed form.
    //
    // 3. All processes and all instances of Scalar use the same
    //    amount of data to represent a Scalar.  (GlobalOrdinal is
    //    typically a built-in integer type, so this is generally true
    //    for GlobalOrdinal.)
    //
    const size_t SizeOfOrdValPair = sizeof (GO) + sizeof (Scalar);
    const size_t totalNumBytes = imports.size (); // * sizeof(char), i.e., 1.
    const size_t totalNumEntries = totalNumBytes / SizeOfOrdValPair;

    if (totalNumEntries > 0) {
      const map_type& rowMap = * (this->getMap ());

      // data packed as follows:
      // [inds_row0 vals_row0 inds_row1 vals_row1 ...]
      ArrayView<const char> avIndsC, avValsC;
      ArrayView<const GO> avInds;
      ArrayView<const Scalar> avVals;

      size_t curOffsetInBytes = 0;
      for (size_type i = 0; i < importLIDs.size (); ++i) {
        const size_t rowSize = numPacketsPerLID[i] / SizeOfOrdValPair;
        // Needs to be in here in case of zero length rows.  If not,
        // the lines following the if statement error out if the row
        // length is zero. KLN 13/06/2011
        //
        // mfh 05 Dec 2012: The problem to which Kurtis refers in the
        // above comment may no longer be an issue, since
        // ArrayView::view() (which implements ArrayView::operator())
        // now allows views of length zero.
        if (rowSize == 0) {
          continue;
        }
        const LO LID = importLIDs[i];
        const GO myGID = rowMap.getGlobalElement (LID);

        // Get views of the import (incoming data) buffers.  Again,
        // this code assumes that sizeof(Scalar) is the number of
        // bytes used by each Scalar.  It also assumes that
        // Teuchos::Comm has correctly deserialized Scalar in place in
        // avValsC.
        avIndsC = imports (curOffsetInBytes, rowSize * sizeof (GO));
        avValsC = imports (curOffsetInBytes + rowSize * sizeof (GO),
                           rowSize * sizeof (Scalar));
        avInds = av_reinterpret_cast<const GO> (avIndsC);
        avVals = av_reinterpret_cast<const Scalar> (avValsC);

        combineGlobalValues (myGID, avInds (), avVals (), combineMode);
        curOffsetInBytes += rowSize * SizeOfOrdValPair;
      }
#ifdef HAVE_TPETRA_DEBUG
      TEUCHOS_TEST_FOR_EXCEPTION_CLASS_FUNC(curOffsetInBytes != totalNumBytes,
        std::logic_error, "After unpacking and combining all the imports, the "
        "final offset in bytes curOffsetInBytes=" << curOffsetInBytes << " != "
        "total number of bytes totalNumBytes=" << totalNumBytes << ".  Please "
        "report this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG
    }
  }

  template<class Scalar,
           class LocalOrdinal,
           class GlobalOrdinal, class DeviceType>
  Teuchos::RCP<MultiVector<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getColumnMapMultiVector (const MV& X_domainMap,
                           const bool force) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;

    TEUCHOS_TEST_FOR_EXCEPTION(
      ! this->hasColMap (), std::runtime_error, "Tpetra::CrsMatrix::getColumn"
      "MapMultiVector: You may only call this method if the matrix has a "
      "column Map.  If the matrix does not yet have a column Map, you should "
      "first call fillComplete (with domain and range Map if necessary).");

    // If the graph is not fill complete, then the Import object (if
    // one should exist) hasn't been constructed yet.
    TEUCHOS_TEST_FOR_EXCEPTION(
      ! this->getGraph ()->isFillComplete (), std::runtime_error, "Tpetra::"
      "CrsMatrix::getColumnMapMultiVector: You may only call this method if "
      "this matrix's graph is fill complete.");

    const size_t numVecs = X_domainMap.getNumVectors ();
    RCP<const import_type> importer = this->getGraph ()->getImporter ();
    RCP<const map_type> colMap = this->getColMap ();

    RCP<MV> X_colMap; // null by default

    // If the Import object is trivial (null), then we don't need a
    // separate column Map multivector.  Just return null in that
    // case.  The caller is responsible for knowing not to use the
    // returned null pointer.
    //
    // If the Import is nontrivial, then we do need a separate
    // column Map multivector for the Import operation.  Check in
    // that case if we have to (re)create the column Map
    // multivector.
    if (! importer.is_null () || force) {
      if (importMV_.is_null () || importMV_->getNumVectors () != numVecs) {
        X_colMap = rcp (new MV (colMap, numVecs));

        // Cache the newly created multivector for later reuse.
        importMV_ = X_colMap;
      }
      else { // Yay, we can reuse the cached multivector!
        X_colMap = importMV_;
        // mfh 09 Jan 2013: We don't have to fill with zeros first,
        // because the Import uses INSERT combine mode, which overwrites
        // existing entries.
        //
        //X_colMap->putScalar (STS::zero ());
      }
    }
    return X_colMap;
  }

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  Teuchos::RCP<MultiVector<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  getRowMapMultiVector (const MultiVector<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >& Y_rangeMap,
                        const bool force) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;

    // If the graph is not fill complete, then the Export object (if
    // one should exist) hasn't been constructed yet.
    TEUCHOS_TEST_FOR_EXCEPTION(
      ! this->getGraph ()->isFillComplete (), std::runtime_error, "Tpetra::"
      "CrsMatrix::getRowMapMultiVector: You may only call this method if this "
      "matrix's graph is fill complete.");

    const size_t numVecs = Y_rangeMap.getNumVectors ();
    RCP<const export_type> exporter = this->getGraph ()->getExporter ();
    // Every version of the constructor takes either a row Map, or a
    // graph (all of whose constructors take a row Map).  Thus, the
    // matrix always has a row Map.
    RCP<const map_type> rowMap = this->getRowMap ();

    RCP<MV> Y_rowMap; // null by default

    // If the Export object is trivial (null), then we don't need a
    // separate row Map multivector.  Just return null in that case.
    // The caller is responsible for knowing not to use the returned
    // null pointer.
    //
    // If the Export is nontrivial, then we do need a separate row
    // Map multivector for the Export operation.  Check in that case
    // if we have to (re)create the row Map multivector.
    if (! exporter.is_null () || force) {
      if (exportMV_.is_null () || exportMV_->getNumVectors () != numVecs) {
        Y_rowMap = rcp (new MV (rowMap, numVecs));

        // Cache the newly created multivector for later reuse.
        exportMV_ = Y_rowMap;
      }
      else { // Yay, we can reuse the cached multivector!
        Y_rowMap = exportMV_;
      }
    }
    return Y_rowMap;
  }

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  removeEmptyProcessesInPlace (const Teuchos::RCP<const Map<LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >& newMap)
  {
    TEUCHOS_TEST_FOR_EXCEPTION(
      myGraph_.is_null (), std::logic_error, "Tpetra::CrsMatrix::"
      "removeEmptyProcessesInPlace: This method does not work when the matrix "
      "was created with a constant graph (that is, when it was created using "
      "the version of its constructor that takes an RCP<const CrsGraph>).  "
      "This is because the matrix is not allowed to modify the graph in that "
      "case, but removing empty processes requires modifying the graph.");
    myGraph_->removeEmptyProcessesInPlace (newMap);
    // Even though CrsMatrix's row Map (as returned by getRowMap())
    // comes from its CrsGraph, CrsMatrix still implements DistObject,
    // so we also have to change the DistObject's Map.
    this->map_ = this->getRowMap ();
    // In the nonconst graph case, staticGraph_ is just a const
    // pointer to myGraph_.  This assignment is probably redundant,
    // but it doesn't hurt.
    staticGraph_ = Teuchos::rcp_const_cast<const Graph> (myGraph_);
  }

  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  Teuchos::RCP<RowMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  add (const Scalar& alpha,
       const RowMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >& A,
       const Scalar& beta,
       const Teuchos::RCP<const Map<LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >& domainMap,
       const Teuchos::RCP<const Map<LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > >& rangeMap,
       const Teuchos::RCP<Teuchos::ParameterList>& params) const
  {
    using Teuchos::Array;
    using Teuchos::ArrayRCP;
    using Teuchos::as;
    using Teuchos::ParameterList;
    using Teuchos::RCP;
    using Teuchos::rcp;
    using Teuchos::rcp_implicit_cast;
    using Teuchos::sublist;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    typedef RowMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> > row_matrix_type;
    typedef CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps> crs_matrix_type;

    const crs_matrix_type& B = *this; // a convenient abbreviation

    // If the user didn't supply a domain or range Map, then try to
    // get one from B first (if it has them), then from A (if it has
    // them).  If we don't have any domain or range Maps, scold the
    // user.
    RCP<const map_type> A_domainMap = A.getDomainMap ();
    RCP<const map_type> A_rangeMap = A.getRangeMap ();
    RCP<const map_type> B_domainMap = B.getDomainMap ();
    RCP<const map_type> B_rangeMap = B.getRangeMap ();

    RCP<const map_type> theDomainMap = domainMap;
    RCP<const map_type> theRangeMap = rangeMap;

    if (domainMap.is_null ()) {
      if (B_domainMap.is_null ()) {
        TEUCHOS_TEST_FOR_EXCEPTION(
          A_domainMap.is_null (), std::invalid_argument,
          "Tpetra::CrsMatrix::add: If neither A nor B have a domain Map, "
          "then you must supply a nonnull domain Map to this method.");
        theDomainMap = A_domainMap;
      } else {
        theDomainMap = B_domainMap;
      }
    }
    if (rangeMap.is_null ()) {
      if (B_rangeMap.is_null ()) {
        TEUCHOS_TEST_FOR_EXCEPTION(
          A_rangeMap.is_null (), std::invalid_argument,
          "Tpetra::CrsMatrix::add: If neither A nor B have a range Map, "
          "then you must supply a nonnull range Map to this method.");
        theRangeMap = A_rangeMap;
      } else {
        theRangeMap = B_rangeMap;
      }
    }

#ifdef HAVE_TPETRA_DEBUG
    // In a debug build, check that A and B have matching domain and
    // range Maps, if they have domain and range Maps at all.  (If
    // they aren't fill complete, then they may not yet have them.)
    if (! A_domainMap.is_null () && ! A_rangeMap.is_null ()) {
      if (! B_domainMap.is_null () && ! B_rangeMap.is_null ()) {
        TEUCHOS_TEST_FOR_EXCEPTION(
          ! B_domainMap->isSameAs (*A_domainMap), std::invalid_argument,
          "Tpetra::CrsMatrix::add: The input RowMatrix A must have a domain Map "
          "which is the same as (isSameAs) this RowMatrix's domain Map.");
        TEUCHOS_TEST_FOR_EXCEPTION(
          ! B_rangeMap->isSameAs (*A_rangeMap), std::invalid_argument,
          "Tpetra::CrsMatrix::add: The input RowMatrix A must have a range Map "
          "which is the same as (isSameAs) this RowMatrix's range Map.");
        TEUCHOS_TEST_FOR_EXCEPTION(
          ! domainMap.is_null () && ! domainMap->isSameAs (*B_domainMap),
          std::invalid_argument,
          "Tpetra::CrsMatrix::add: The input domain Map must be the same as "
          "(isSameAs) this RowMatrix's domain Map.");
        TEUCHOS_TEST_FOR_EXCEPTION(
          ! rangeMap.is_null () && ! rangeMap->isSameAs (*B_rangeMap),
          std::invalid_argument,
          "Tpetra::CrsMatrix::add: The input range Map must be the same as "
          "(isSameAs) this RowMatrix's range Map.");
      }
    }
    else if (! B_domainMap.is_null () && ! B_rangeMap.is_null ()) {
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! domainMap.is_null () && ! domainMap->isSameAs (*B_domainMap),
        std::invalid_argument,
        "Tpetra::CrsMatrix::add: The input domain Map must be the same as "
        "(isSameAs) this RowMatrix's domain Map.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! rangeMap.is_null () && ! rangeMap->isSameAs (*B_rangeMap),
        std::invalid_argument,
        "Tpetra::CrsMatrix::add: The input range Map must be the same as "
        "(isSameAs) this RowMatrix's range Map.");
    }
    else {
      TEUCHOS_TEST_FOR_EXCEPTION(
        domainMap.is_null () || rangeMap.is_null (), std::invalid_argument,
        "Tpetra::CrsMatrix::add: If neither A nor B have a domain and range "
        "Map, then you must supply a nonnull domain and range Map to this "
        "method.");
    }
#endif // HAVE_TPETRA_DEBUG

    // What parameters do we pass to C's constructor?  Do we call
    // fillComplete on C after filling it?  And if so, what parameters
    // do we pass to C's fillComplete call?
    bool callFillComplete = true;
    RCP<ParameterList> constructorSublist;
    RCP<ParameterList> fillCompleteSublist;
    if (! params.is_null ()) {
      callFillComplete = params->get ("Call fillComplete", callFillComplete);
      constructorSublist = sublist (params, "Constructor parameters");
      fillCompleteSublist = sublist (params, "fillComplete parameters");
    }

    RCP<const map_type> A_rowMap = A.getRowMap ();
    RCP<const map_type> B_rowMap = B.getRowMap ();
    RCP<const map_type> C_rowMap = B_rowMap; // see discussion in documentation
    RCP<crs_matrix_type> C; // The result matrix.

    // If A and B's row Maps are the same, we can compute an upper
    // bound on the number of entries in each row of C, before
    // actually computing the sum.  A reasonable upper bound is the
    // sum of the two entry counts in each row.  If we choose this as
    // the actual per-row upper bound, we can use static profile.
    if (A_rowMap->isSameAs (*B_rowMap)) {
      const LO localNumRows = as<LO> (A_rowMap->getNodeNumElements ());
      ArrayRCP<size_t> C_maxNumEntriesPerRow (localNumRows, 0);

      // Get the number of entries in each row of A.
      if (alpha != STS::zero ()) {
        for (LO localRow = 0; localRow < localNumRows; ++localRow) {
          const size_t A_numEntries = A.getNumEntriesInLocalRow (localRow);
          C_maxNumEntriesPerRow[localRow] += A_numEntries;
        }
      }
      // Get the number of entries in each row of B.
      if (beta != STS::zero ()) {
        for (LO localRow = 0; localRow < localNumRows; ++localRow) {
          const size_t B_numEntries = B.getNumEntriesInLocalRow (localRow);
          C_maxNumEntriesPerRow[localRow] += B_numEntries;
        }
      }
      // Construct the result matrix C.
      if (constructorSublist.is_null ()) {
        C = rcp (new crs_matrix_type (C_rowMap, C_maxNumEntriesPerRow,
                                      StaticProfile));
      } else {
        C = rcp (new crs_matrix_type (C_rowMap, C_maxNumEntriesPerRow,
                                      StaticProfile, constructorSublist));
      }
      // Since A and B have the same row Maps, we could add them
      // together all at once and merge values before we call
      // insertGlobalValues.  However, we don't really need to, since
      // we've already allocated enough space in each row of C for C
      // to do the merge itself.
    }
    else { // the row Maps of A and B are not the same
      // Construct the result matrix C.
      if (constructorSublist.is_null ()) {
        C = rcp (new crs_matrix_type (C_rowMap, 0, DynamicProfile));
      } else {
        C = rcp (new crs_matrix_type (C_rowMap, 0, DynamicProfile,
                                      constructorSublist));
      }
    }

#ifdef HAVE_TPETRA_DEBUG
    TEUCHOS_TEST_FOR_EXCEPTION(C.is_null (), std::logic_error,
      "Tpetra::RowMatrix::add: C should not be null at this point.  "
      "Please report this bug to the Tpetra developers.");
#endif // HAVE_TPETRA_DEBUG
    //
    // Compute C = alpha*A + beta*B.
    //
    Array<GO> ind;
    Array<Scalar> val;

    if (alpha != STS::zero ()) {
      const LO A_localNumRows = as<LO> (A_rowMap->getNodeNumElements ());
      for (LO localRow = 0; localRow < A_localNumRows; ++localRow) {
        size_t A_numEntries = A.getNumEntriesInLocalRow (localRow);
        const GO globalRow = A_rowMap->getGlobalElement (localRow);
        if (A_numEntries > as<size_t> (ind.size ())) {
          ind.resize (A_numEntries);
          val.resize (A_numEntries);
        }
        ArrayView<GO> indView = ind (0, A_numEntries);
        ArrayView<Scalar> valView = val (0, A_numEntries);
        A.getGlobalRowCopy (globalRow, indView, valView, A_numEntries);

        if (alpha != STS::one ()) {
          for (size_t k = 0; k < A_numEntries; ++k) {
            valView[k] *= alpha;
          }
        }
        C->insertGlobalValues (globalRow, indView, valView);
      }
    }

    if (beta != STS::zero ()) {
      const LO B_localNumRows = as<LO> (B_rowMap->getNodeNumElements ());
      for (LO localRow = 0; localRow < B_localNumRows; ++localRow) {
        size_t B_numEntries = B.getNumEntriesInLocalRow (localRow);
        const GO globalRow = B_rowMap->getGlobalElement (localRow);
        if (B_numEntries > as<size_t> (ind.size ())) {
          ind.resize (B_numEntries);
          val.resize (B_numEntries);
        }
        ArrayView<GO> indView = ind (0, B_numEntries);
        ArrayView<Scalar> valView = val (0, B_numEntries);
        B.getGlobalRowCopy (globalRow, indView, valView, B_numEntries);

        if (beta != STS::one ()) {
          for (size_t k = 0; k < B_numEntries; ++k) {
            valView[k] *= beta;
          }
        }
        C->insertGlobalValues (globalRow, indView, valView);
      }
    }

    if (callFillComplete) {
      if (fillCompleteSublist.is_null ()) {
        C->fillComplete (theDomainMap, theRangeMap);
      } else {
        C->fillComplete (theDomainMap, theRangeMap, fillCompleteSublist);
      }
    }

    return rcp_implicit_cast<row_matrix_type> (C);
  }


  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<
    Scalar, LocalOrdinal, GlobalOrdinal,
    Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType>,
    typename KokkosClassic::DefaultKernels<
      Scalar, LocalOrdinal,
      Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  transferAndFillComplete (Teuchos::RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, node_type, LocalMatOps> > & destMat,
                           const ::Tpetra::Details::Transfer<LocalOrdinal, GlobalOrdinal, node_type>& rowTransfer,
                           const Teuchos::RCP<const map_type>& domainMap,
                           const Teuchos::RCP<const map_type>& rangeMap,
                           const Teuchos::RCP<Teuchos::ParameterList>& params) const
  {
    using Teuchos::ArrayView;
    using Teuchos::ParameterList;
    using Teuchos::RCP;
    typedef LocalOrdinal LO;
    typedef GlobalOrdinal GO;
    typedef node_type NT;
    typedef CrsMatrix<Scalar, LO, GO, NT, LocalMatOps> this_type;
    typedef Vector<int, LO, GO, NT> IntVectorType;

    // Make sure that the input argument rowTransfer is either an
    // Import or an Export.  Import and Export are the only two
    // subclasses of Transfer that we defined, but users might
    // (unwisely, for now at least) decide to implement their own
    // subclasses.  Exclude this possibility.
    const import_type* xferAsImport = dynamic_cast<const import_type*> (&rowTransfer);
    const export_type* xferAsExport = dynamic_cast<const export_type*> (&rowTransfer);
    TEUCHOS_TEST_FOR_EXCEPTION(
      xferAsImport == NULL && xferAsExport == NULL, std::invalid_argument,
      "Tpetra::CrsMatrix::transferAndFillComplete: The 'rowTransfer' input "
      "argument must be either an Import or an Export, and its template "
      "parameters must match the corresponding template parameters of the "
      "CrsMatrix.");

    // FIXME (mfh 15 May 2014) Wouldn't communication still be needed,
    // if the source Map is not distributed but the target Map is?
    const bool communication_needed = rowTransfer.getSourceMap ()->isDistributed ();

    //
    // Get the caller's parameters
    //

    bool reverseMode = false; // Are we in reverse mode?
    bool restrictComm = false; // Do we need to restrict the communicator?
    RCP<ParameterList> matrixparams; // parameters for the destination matrix
    if (! params.is_null ()) {
      reverseMode = params->get ("Reverse Mode", reverseMode);
      restrictComm = params->get ("Restrict Communicator", restrictComm);
      matrixparams = sublist (params, "CrsMatrix");
    }

    // Get the new domain and range Maps.  We need some of them for
    // error checking, now that we have the reverseMode parameter.
    RCP<const map_type> MyRowMap = reverseMode ?
      rowTransfer.getSourceMap () : rowTransfer.getTargetMap ();
    RCP<const map_type> MyColMap; // create this below
    RCP<const map_type> MyDomainMap = ! domainMap.is_null () ?
      domainMap : getDomainMap ();
    RCP<const map_type> MyRangeMap = ! rangeMap.is_null () ?
      rangeMap : getRangeMap ();
    RCP<const map_type> BaseRowMap = MyRowMap;
    RCP<const map_type> BaseDomainMap = MyDomainMap;

    // If the user gave us a nonnull destMat, then check whether it's
    // "pristine."  That means that it has no entries.
    //
    // FIXME (mfh 15 May 2014) If this is not true on all processes,
    // then this exception test may hang.  It would be better to
    // forward an error flag to the next communication phase.
    if (! destMat.is_null ()) {
      // FIXME (mfh 15 May 2014): The classic Petra idiom for checking
      // whether a graph or matrix has no entries on the calling
      // process, is that it is neither locally nor globally indexed.
      // This may change eventually with the Kokkos refactor version
      // of Tpetra, so it would be better just to check the quantity
      // of interest directly.  Note that with the Kokkos refactor
      // version of Tpetra, asking for the total number of entries in
      // a graph or matrix that is not fill complete might require
      // computation (kernel launch), since it is not thread scalable
      // to update a count every time an entry is inserted.
      const bool NewFlag = ! destMat->getGraph ()->isLocallyIndexed () &&
        ! destMat->getGraph ()->isGloballyIndexed ();
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! NewFlag, std::invalid_argument, "Tpetra::CrsMatrix::"
        "transferAndFillComplete: The input argument 'destMat' is only allowed "
        "to be nonnull, if its graph is empty (neither locally nor globally "
        "indexed).");
      // FIXME (mfh 15 May 2014) At some point, we want to change
      // graphs and matrices so that their DistObject Map
      // (this->getMap()) may differ from their row Map.  This will
      // make redistribution for 2-D distributions more efficient.  I
      // hesitate to change this check, because I'm not sure how much
      // the code here depends on getMap() and getRowMap() being the
      // same.
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! destMat->getRowMap ()->isSameAs (*MyRowMap), std::invalid_argument,
        "Tpetra::CrsMatrix::transferAndFillComplete: The (row) Map of the "
        "input argument 'destMat' is not the same as the (row) Map specified "
        "by the input argument 'rowTransfer'.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! destMat->checkSizes (*this), std::invalid_argument,
        "Tpetra::CrsMatrix::transferAndFillComplete: You provided a nonnull "
        "destination matrix, but checkSizes() indicates that it is not a legal "
        "legal target for redistribution from the source matrix (*this).  This "
        "may mean that they do not have the same dimensions.");
    }

    // If forward mode (the default), then *this's (row) Map must be
    // the same as the source Map of the Transfer.  If reverse mode,
    // then *this's (row) Map must be the same as the target Map of
    // the Transfer.
    //
    // FIXME (mfh 15 May 2014) At some point, we want to change graphs
    // and matrices so that their DistObject Map (this->getMap()) may
    // differ from their row Map.  This will make redistribution for
    // 2-D distributions more efficient.  I hesitate to change this
    // check, because I'm not sure how much the code here depends on
    // getMap() and getRowMap() being the same.
    TEUCHOS_TEST_FOR_EXCEPTION(
      ! (reverseMode || getRowMap ()->isSameAs (*rowTransfer.getSourceMap ())),
      std::invalid_argument, "Tpetra::CrsMatrix::transferAndFillComplete: "
      "rowTransfer->getSourceMap() must match this->getRowMap() in forward mode.");
    TEUCHOS_TEST_FOR_EXCEPTION(
      ! (! reverseMode || getRowMap ()->isSameAs (*rowTransfer.getTargetMap ())),
      std::invalid_argument, "Tpetra::CrsMatrix::transferAndFillComplete: "
      "rowTransfer->getTargetMap() must match this->getRowMap() in reverse mode.");

    // The basic algorithm here is:
    //
    // 1. Call the moral equivalent of "distor.do" to handle the import.
    // 2. Copy all the Imported and Copy/Permuted data into the raw
    //    CrsMatrix / CrsGraphData pointers, still using GIDs.
    // 3. Call an optimized version of MakeColMap that avoids the
    //    Directory lookups (since the importer knows who owns all the
    //    GIDs) AND reindexes to LIDs.
    // 4. Call expertStaticFillComplete()

    // Get information from the Importer
    const size_t NumSameIDs = rowTransfer.getNumSameIDs();
    ArrayView<const LO> ExportLIDs = reverseMode ?
      rowTransfer.getRemoteLIDs () : rowTransfer.getExportLIDs ();
    ArrayView<const LO> RemoteLIDs = reverseMode ?
      rowTransfer.getExportLIDs () : rowTransfer.getRemoteLIDs ();
    ArrayView<const LO> PermuteToLIDs = reverseMode ?
      rowTransfer.getPermuteFromLIDs () : rowTransfer.getPermuteToLIDs ();
    ArrayView<const LO> PermuteFromLIDs = reverseMode ?
      rowTransfer.getPermuteToLIDs () : rowTransfer.getPermuteFromLIDs ();
    Distributor& Distor = rowTransfer.getDistributor ();

    // Owning PIDs
    Teuchos::Array<int> SourcePids;
    Teuchos::Array<int> TargetPids;
    int MyPID = getComm ()->getRank ();

    // Temp variables for sub-communicators
    RCP<const map_type> ReducedRowMap, ReducedColMap,
      ReducedDomainMap, ReducedRangeMap;
    RCP<const Comm<int> > ReducedComm;

    // If the user gave us a null destMat, then construct the new
    // destination matrix.  We will replace its column Map later.
    if (destMat.is_null ()) {
      destMat = rcp (new this_type (MyRowMap, 0, StaticProfile, matrixparams));
    }

    /***************************************************/
    /***** 1) First communicator restriction phase ****/
    /***************************************************/
    if (restrictComm) {
      ReducedRowMap = MyRowMap->removeEmptyProcesses ();
      ReducedComm = ReducedRowMap.is_null () ? Teuchos::null : ReducedRowMap->getComm ();
      destMat->removeEmptyProcessesInPlace (ReducedRowMap);

      ReducedDomainMap = MyRowMap.getRawPtr () == MyDomainMap.getRawPtr () ?
        ReducedRowMap :
        MyDomainMap->replaceCommWithSubset (ReducedComm);
      ReducedRangeMap  = MyRowMap.getRawPtr () == MyRangeMap.getRawPtr () ?
        ReducedRowMap :
        MyRangeMap->replaceCommWithSubset (ReducedComm);

      // Reset the "my" maps
      MyRowMap    = ReducedRowMap;
      MyDomainMap = ReducedDomainMap;
      MyRangeMap  = ReducedRangeMap;

      // Update my PID, if we've restricted the communicator
      if (! ReducedComm.is_null ()) {
        MyPID = ReducedComm->getRank ();
      }
      else {
        MyPID = -2; // For debugging
      }
    }
    else {
      ReducedComm = MyRowMap->getComm ();
    }

    /***************************************************/
    /***** 2) From Tpera::DistObject::doTransfer() ****/
    /***************************************************/

    // Get the owning PIDs
    RCP<const import_type> MyImporter = getGraph ()->getImporter ();

    if (! restrictComm && ! MyImporter.is_null () &&
        BaseDomainMap->isSameAs (*getDomainMap ())) {
      // Same domain map as source matrix
      //
      // NOTE: This won't work for restrictComm (because the Import
      // doesn't know the restricted PIDs), though writing an
      // optimized version for that case would be easy (Import an
      // IntVector of the new PIDs).  Might want to add this later.
      Import_Util::getPids (*MyImporter, SourcePids, false);
    }
    else if (MyImporter.is_null () && BaseDomainMap->isSameAs (*getDomainMap ())) {
      // Matrix has no off-process entries
      SourcePids.resize (getColMap ()->getNodeNumElements ());
      SourcePids.assign (getColMap ()->getNodeNumElements (), MyPID);
    }
    else if (BaseDomainMap->isSameAs (*BaseRowMap) &&
             getDomainMap ()->isSameAs (*getRowMap ())) {
      // We can use the rowTransfer + SourceMatrix's Import to find out who owns what.
      IntVectorType TargetRow_pids (domainMap);
      IntVectorType SourceRow_pids (getRowMap ());
      IntVectorType SourceCol_pids (getColMap ());

      TargetRow_pids.putScalar (MyPID);
      if (! reverseMode && xferAsImport != NULL) {
        SourceRow_pids.doExport (TargetRow_pids, *xferAsImport, INSERT);
      }
      else if (reverseMode && xferAsExport != NULL) {
        SourceRow_pids.doExport (TargetRow_pids, *xferAsExport, INSERT);
      }
      else if (! reverseMode && xferAsExport != NULL) {
        SourceRow_pids.doImport (TargetRow_pids, *xferAsExport, INSERT);
      }
      else if (reverseMode && xferAsImport != NULL) {
        SourceRow_pids.doImport (TargetRow_pids, *xferAsImport, INSERT);
      }
      else {
        TEUCHOS_TEST_FOR_EXCEPTION(
          true, std::logic_error, "Tpetra::CrsMatrix::"
          "transferAndFillComplete: Should never get here!  "
          "Please report this bug to a Tpetra developer.");
      }
      SourceCol_pids.doImport (SourceRow_pids, *MyImporter, INSERT);
      SourcePids.resize (getColMap ()->getNodeNumElements ());
      SourceCol_pids.get1dCopy (SourcePids ());
    }
    else {
      TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::invalid_argument, "Tpetra::CrsMatrix::"
        "transferAndFillComplete: This method only allows either domainMap == "
        "getDomainMap (), or (domainMap == rowTransfer.getTargetMap () and "
        "getDomainMap () == getRowMap ()).");
    }

    // Tpetra-specific stuff
    //
    // FIXME (mfh 15 May 2014) This should work fine if CrsMatrix
    // inherits from DistObject (in which case all arrays that get
    // resized here are Teuchos::Array), but it won't work if
    // CrsMatrix inherits from DistObjectKA (in which case all arrays
    // that get resized here are Kokkos::View).  In the latter case,
    // imports_ and numExportPacketsPerLID_ each have only a device
    // view, but numImportPacketsPerLID_ has a device view and a host
    // view (host_numImportPacketsPerLID_).
    //
    // Currently, CrsMatrix inherits from DistObject, not
    // DistObjectKA, so the code below should be fine for the Kokkos
    // refactor version of CrsMatrix.
    //
    // For this and for all other cases in this function that want to
    // resize the DistObject's communication arrays, it would make
    // sense to give DistObject (and DistObjectKA) methods for
    // resizing that don't expose the details of whether these are
    // Teuchos::Array or Kokkos::View.
    size_t constantNumPackets = destMat->constantNumberOfPackets ();
    if (constantNumPackets == 0) {
      destMat->numExportPacketsPerLID_old_.resize (ExportLIDs.size ());
      destMat->numImportPacketsPerLID_old_.resize (RemoteLIDs.size ());
    }
    else {
      // There are a constant number of packets per element.  We
      // already know (from the number of "remote" (incoming)
      // elements) how many incoming elements we expect, so we can
      // resize the buffer accordingly.
      const size_t rbufLen = RemoteLIDs.size() * constantNumPackets;
      if (static_cast<size_t> (destMat->imports_old_.size ()) != rbufLen) {
        destMat->imports_old_.resize (rbufLen);
      }
    }

    // Pack & Prepare w/ owning PIDs
    //
    // FIXME (mfh 15 May 2014) This should work fine if CrsMatrix
    // inherits from DistObject (in which case all arrays that get
    // passed in here are Teuchos::Array), but it won't work if
    // CrsMatrix inherits from DistObjectKA (in which case all arrays
    // that get passed in here are Kokkos::View).  In the latter case,
    // exports_ and numExportPacketsPerLID_ each have only a device
    // view.
    //
    // Currently, CrsMatrix inherits from DistObject, not
    // DistObjectKA, so the code below should be fine for the Kokkos
    // refactor version of CrsMatrix.
    Import_Util::packAndPrepareWithOwningPIDs (*this, ExportLIDs,
                                               destMat->exports_old_,
                                               destMat->numExportPacketsPerLID_old_ (),
                                               constantNumPackets, Distor,
                                               SourcePids);

    // Do the exchange of remote data.
    //
    // FIXME (mfh 15 May 2014) This should work fine if CrsMatrix
    // inherits from DistObject (in which case all arrays that get
    // passed in here are Teuchos::Array), but it won't work if
    // CrsMatrix inherits from DistObjectKA (in which case all arrays
    // that get passed in here are Kokkos::View).
    //
    // In the latter case, imports_, exports_, and
    // numExportPacketsPerLID_ each have only a device view.
    // numImportPacketsPerLIDs_ is a device view, and also has a host
    // view (host_numImportPacketsPerLID_).
    if (communication_needed) {
      if (reverseMode) {
        if (constantNumPackets == 0) { // variable number of packets per LID
          Distor.doReversePostsAndWaits (destMat->numExportPacketsPerLID_old_ ().getConst (), 1,
                                         destMat->numImportPacketsPerLID_old_ ());
          size_t totalImportPackets = 0;
          for (Array_size_type i = 0; i < destMat->numImportPacketsPerLID_old_.size (); ++i) {
            totalImportPackets += destMat->numImportPacketsPerLID_old_[i];
          }
          destMat->imports_old_.resize (totalImportPackets);
          Distor.doReversePostsAndWaits (destMat->exports_old_ ().getConst (),
                                         destMat->numExportPacketsPerLID_old_ (),
                                         destMat->imports_old_ (),
                                         destMat->numImportPacketsPerLID_old_ ());
        }
        else { // constant number of packets per LID
          Distor.doReversePostsAndWaits (destMat->exports_old_ ().getConst (),
                                         constantNumPackets,
                                         destMat->imports_old_ ());
        }
      }
      else { // forward mode (the default)
        if (constantNumPackets == 0) { // variable number of packets per LID
          Distor.doPostsAndWaits (destMat->numExportPacketsPerLID_old_ ().getConst (), 1,
                                  destMat->numImportPacketsPerLID_old_ ());
          size_t totalImportPackets = 0;
          for (Array_size_type i = 0; i < destMat->numImportPacketsPerLID_old_.size (); ++i) {
            totalImportPackets += destMat->numImportPacketsPerLID_old_[i];
          }
          destMat->imports_old_.resize (totalImportPackets);
          Distor.doPostsAndWaits (destMat->exports_old_ ().getConst (),
                                  destMat->numExportPacketsPerLID_old_ (),
                                  destMat->imports_old_ (),
                                  destMat->numImportPacketsPerLID_old_ ());
        }
        else { // constant number of packets per LID
          Distor.doPostsAndWaits (destMat->exports_old_ ().getConst (),
                                  constantNumPackets,
                                  destMat->imports_old_ ());
        }
      }
    }

    /*********************************************************************/
    /**** 3) Copy all of the Same/Permute/Remote data into CSR_arrays ****/
    /*********************************************************************/

    // FIXME (mfh 15 May 2014) This should work fine if CrsMatrix
    // inherits from DistObject (in which case all arrays that get
    // passed in here are Teuchos::Array), but it won't work if
    // CrsMatrix inherits from DistObjectKA (in which case all arrays
    // that get passed in here are Kokkos::View).
    //
    // In the latter case, imports_ only has a device view.
    // numImportPacketsPerLIDs_ is a device view, and also has a host
    // view (host_numImportPacketsPerLID_).
    size_t mynnz =
      Import_Util::unpackAndCombineWithOwningPIDsCount (*this, RemoteLIDs,
                                                        destMat->imports_old_ (),
                                                        destMat->numImportPacketsPerLID_old_ (),
                                                        constantNumPackets, Distor, INSERT,
                                                        NumSameIDs, PermuteToLIDs,
                                                        PermuteFromLIDs);
    size_t N = BaseRowMap->getNodeNumElements ();

    // Allocations
    ArrayRCP<size_t> CSR_rowptr(N+1);
    ArrayRCP<GO> CSR_colind_GID;
    ArrayRCP<LO> CSR_colind_LID;
    ArrayRCP<Scalar> CSR_vals;
    CSR_colind_GID.resize (mynnz);
    CSR_vals.resize (mynnz);

    // If LO and GO are the same, we can reuse memory when
    // converting the column indices from global to local indices.
    if (typeid (LO) == typeid (GO)) {
      CSR_colind_LID = Teuchos::arcp_reinterpret_cast<LO> (CSR_colind_GID);
    }
    else {
      CSR_colind_LID.resize (mynnz);
    }

    // FIXME (mfh 15 May 2014) This should work fine if CrsMatrix
    // inherits from DistObject (in which case all arrays that get
    // passed in here are Teuchos::Array), but it won't work if
    // CrsMatrix inherits from DistObjectKA (in which case all arrays
    // that get passed in here are Kokkos::View).
    //
    // In the latter case, imports_ only has a device view.
    // numImportPacketsPerLIDs_ is a device view, and also has a host
    // view (host_numImportPacketsPerLID_).
    //
    // FIXME (mfh 15 May 2014) Why can't we abstract this out as an
    // unpackAndCombine method on a "CrsArrays" object?  This passing
    // in a huge list of arrays is icky.  Can't we have a bit of an
    // abstraction?  Implementing a concrete DistObject subclass only
    // takes five methods.
    Import_Util::unpackAndCombineIntoCrsArrays (*this, RemoteLIDs, destMat->imports_old_ (),
                                                destMat->numImportPacketsPerLID_old_ (),
                                                constantNumPackets, Distor, INSERT, NumSameIDs,
                                                PermuteToLIDs, PermuteFromLIDs, N, mynnz, MyPID,
                                                CSR_rowptr (), CSR_colind_GID (), CSR_vals (),
                                                SourcePids (), TargetPids);

    /**************************************************************/
    /**** 4) Call Optimized MakeColMap w/ no Directory Lookups ****/
    /**************************************************************/

    // Call an optimized version of makeColMap that avoids the
    // Directory lookups (since the Import object knows who owns all
    // the GIDs).
    Teuchos::Array<int> RemotePids;
    Import_Util::lowCommunicationMakeColMapAndReindex (CSR_rowptr (),
                                                       CSR_colind_LID (),
                                                       CSR_colind_GID (),
                                                       BaseDomainMap,
                                                       TargetPids, RemotePids,
                                                       MyColMap);

    /*******************************************************/
    /**** 4) Second communicator restriction phase      ****/
    /*******************************************************/
    if (restrictComm) {
      ReducedColMap = (MyRowMap.getRawPtr () == MyColMap.getRawPtr ()) ?
        ReducedRowMap :
        MyColMap->replaceCommWithSubset (ReducedComm);
      MyColMap = ReducedColMap; // Reset the "my" maps
    }

    // Replace the col map
    destMat->replaceColMap (MyColMap);

    // Short circuit if the processor is no longer in the communicator
    //
    // NOTE: Epetra replaces modifies all "removed" processes so they
    // have a dummy (serial) Map that doesn't touch the original
    // communicator.  Duplicating that here might be a good idea.
    if (ReducedComm.is_null ()) {
      return;
    }

    /***************************************************/
    /**** 5) Sort                                   ****/
    /***************************************************/
    Import_Util::sortCrsEntries (CSR_rowptr (),
                                 CSR_colind_LID (),
                                 CSR_vals ());
    if ((! reverseMode && xferAsImport != NULL) ||
        (reverseMode && xferAsExport != NULL)) {
      Import_Util::sortCrsEntries (CSR_rowptr (),
                                   CSR_colind_LID (),
                                   CSR_vals ());
    }
    else if ((! reverseMode && xferAsExport != NULL) ||
             (reverseMode && xferAsImport != NULL)) {
      Import_Util::sortAndMergeCrsEntries (CSR_rowptr (),
                                           CSR_colind_LID (),
                                           CSR_vals ());
      if (CSR_rowptr[N] != mynnz) {
        CSR_colind_LID.resize (CSR_rowptr[N]);
        CSR_vals.resize (CSR_rowptr[N]);
      }
    }
    else {
      TEUCHOS_TEST_FOR_EXCEPTION(
        true, std::logic_error, "Tpetra::CrsMatrix::"
        "transferAndFillComplete: Should never get here!  "
        "Please report this bug to a Tpetra developer.");
    }
    /***************************************************/
    /**** 6) Reset the colmap and the arrays        ****/
    /***************************************************/

    // Call constructor for the new matrix (restricted as needed)
    //
    // NOTE (mfh 15 May 2014) This should work fine for the Kokkos
    // refactor version of CrsMatrix, though it reserves the right to
    // make a deep copy of the arrays.
    destMat->setAllValues (CSR_rowptr, CSR_colind_LID, CSR_vals);

    /***************************************************/
    /**** 7) Build Importer & Call ESFC             ****/
    /***************************************************/
    // Pre-build the importer using the existing PIDs
    RCP<import_type> MyImport = rcp (new import_type (MyDomainMap, MyColMap, RemotePids));
    destMat->expertStaticFillComplete (MyDomainMap, MyRangeMap, MyImport);
  }


  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  importAndFillComplete (Teuchos::RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, node_type, LocalMatOps> >& destMatrix,
                         const import_type& importer,
                         const Teuchos::RCP<const map_type>& domainMap,
                         const Teuchos::RCP<const map_type>& rangeMap,
                         const Teuchos::RCP<Teuchos::ParameterList>& params) const
  {
    transferAndFillComplete (destMatrix, importer, domainMap, rangeMap, params);
  }


  template <class Scalar,
            class LocalOrdinal,
            class GlobalOrdinal, class DeviceType>
  void
  CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> ,  typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Kokkos::Compat::KokkosDeviceWrapperNode<DeviceType> >::SparseOps>::
  exportAndFillComplete (Teuchos::RCP<CrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, node_type, LocalMatOps> >& destMatrix,
                         const export_type& exporter,
                         const Teuchos::RCP<const map_type>& domainMap,
                         const Teuchos::RCP<const map_type>& rangeMap,
                         const Teuchos::RCP<Teuchos::ParameterList>& params) const
  {
    transferAndFillComplete (destMatrix, exporter, domainMap, rangeMap, params);
  }

} // namespace Tpetra

#endif
