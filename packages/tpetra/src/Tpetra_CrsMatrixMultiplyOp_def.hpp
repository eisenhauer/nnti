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

#ifndef TPETRA_CRSMATRIXMULTIPLYOP_DEF_HPP
#define TPETRA_CRSMATRIXMULTIPLYOP_DEF_HPP

#include "Tpetra_CrsMatrix.hpp"

#ifdef DOXYGEN_USE_ONLY
  #include "Tpetra_CrsMatrixMultiplyOp_decl.hpp"
#endif

#ifdef TPETRA_CRSMATRIX_MULTIPLY_DUMP
  #include "Teuchos_VerboseObject.hpp"
#endif

/*! \file Tpetra_CrsMatrixMultiplyOp_def.hpp

    The implementations for the members of Tpetra::CrsMatrixMultiplyOp and related non-member constructors.
 */

namespace Tpetra {

  template <class OpScalar, class MatScalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::CrsMatrixMultiplyOp(const Teuchos::RCP<const CrsMatrix<MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps> > &A)
  : matrix_(A) {
    // we don't require that A is fill complete; we will query for the importer/exporter at apply()-time
#ifdef HAVE_KOKKOSCLASSIC_CUDA_NODE_MEMORY_PROFILING
    importTimer_ = Teuchos::TimeMonitor::getNewTimer( "CrsMatrixMultiplyOp::import" );
    exportTimer_ = Teuchos::TimeMonitor::getNewTimer( "CrsMatrixMultiplyOp::export" );
#endif
  }

  template <class OpScalar, class MatScalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::~CrsMatrixMultiplyOp() {
  }

  template <class OpScalar, class MatScalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::apply(
              const MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> & X_in,
                    MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> &Y_in,
                    Teuchos::ETransp mode, OpScalar alpha, OpScalar beta) const
  {
    TEUCHOS_TEST_FOR_EXCEPTION(!matrix_->isFillComplete(), std::runtime_error,
        Teuchos::typeName(*this) << "::apply(): underlying matrix is not fill-complete.");
    TEUCHOS_TEST_FOR_EXCEPTION(X_in.getNumVectors() != Y_in.getNumVectors(), std::runtime_error,
        Teuchos::typeName(*this) << "::apply(X,Y): X and Y must have the same number of vectors.");
    TEUCHOS_TEST_FOR_EXCEPTION(Teuchos::ScalarTraits<OpScalar>::isComplex && mode == Teuchos::TRANS, std::logic_error,
        Teuchos::typeName(*this) << "::apply() does not currently support transposed multiplications for complex scalar types.");
    if (mode == Teuchos::NO_TRANS) {
      applyNonTranspose(X_in, Y_in, alpha, beta);
    }
    else {
      applyTranspose(X_in, Y_in, alpha, beta);
    }
  }


  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <class OpScalar, class MatScalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::
  gaussSeidel (const MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> &B,
               MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> &X,
               const MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> &D,
               const OpScalar& dampingFactor,
               const ESweepDirection direction,
               const int numSweeps) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    using Teuchos::rcpFromRef;
    typedef OpScalar OS;
    typedef Teuchos::ScalarTraits<OS> STS;
    typedef Map<LocalOrdinal, GlobalOrdinal, Node> map_type;
    typedef Export<LocalOrdinal, GlobalOrdinal, Node> export_type;
    typedef Import<LocalOrdinal, GlobalOrdinal, Node> import_type;
    typedef MultiVector<OS, LocalOrdinal, GlobalOrdinal, Node> OSMV;

    TEUCHOS_TEST_FOR_EXCEPTION(
      numSweeps < 0,
      std::invalid_argument,
      "gaussSeidel: The number of sweeps must be nonnegative, "
      "but you provided numSweeps = " << numSweeps << " < 0.");

    // Translate from global to local sweep direction.
    // While doing this, validate the input.
    Kokkos::ESweepDirection localDirection;
    if (direction == Forward) {
      localDirection = Kokkos::Forward;
    }
    else if (direction == Backward) {
      localDirection = Kokkos::Backward;
    }
    else if (direction == Symmetric) {
      // We'll control local sweep direction manually.
      localDirection = Kokkos::Forward;
    }
    else {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument,
        "gaussSeidel: The 'direction' enum does not have any of its valid "
        "values: Forward, Backward, or Symmetric.");
    }

    if (numSweeps == 0) {
      return; // Nothing to do.
    }

    // We don't need the Export object because this method assumes
    // that the row, domain, and range Maps are the same.  We do need
    // the Import object, if there is one, though.
    RCP<const import_type> importer = matrix_->getGraph()->getImporter();
    RCP<const export_type> exporter = matrix_->getGraph()->getExporter();
    TEUCHOS_TEST_FOR_EXCEPTION(
      ! exporter.is_null (),
      std::runtime_error,
      "gaussSeidel requires that the row, domain, and range Maps be the same.  "
      "This cannot be the case, because the matrix has a nontrivial Export object.");

#ifdef HAVE_TEUCHOS_DEBUG
    {
      RCP<const map_type> domainMap = matrix_->getDomainMap ();
      RCP<const map_type> rangeMap = matrix_->getRangeMap ();
      RCP<const map_type> rowMap = matrix_->getGraph ()->getRowMap ();

      // The relation 'isSameAs' is transitive.  It's also a
      // collective, so we don't have to do a "shared" test for
      // exception (i.e., a global reduction on the test value).
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! domainMap->isSameAs (*rangeMap) || ! domainMap->isSameAs (*rowMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidel requires that the row, domain, and "
        "range Maps of the matrix be the same.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! X.getMap ()->isSameAs (*domainMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidel requires that the input / output "
        "multivector X be in the domain Map of the matrix.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! B.getMap ()->isSameAs (*rangeMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidel requires that the input multivector B "
        "be in the range Map of the matrix.");
      TEUCHOS_TEST_FOR_EXCEPTION(
        ! D.getMap ()->isSameAs (*rowMap),
        std::runtime_error,
        "Tpetra::CrsMatrix::gaussSeidel requires that the input multivector D "
        "be in the row Map of the matrix.");
    }
#endif // HAVE_TEUCHOS_DEBUG

    RCP<const map_type> colMap = matrix_->getGraph ()->getColMap ();

    // The Gauss-Seidel / SOR kernel expects multivectors of constant
    // stride.  If either of them is not, we have to duplicate them.
    // Do this before making the column Map view of X, since the view
    // has to be the view of the multivector used in the kernel.
    // Issue a performance warning if we had to do this.  It's really
    // a problem with our kernel, not the user's problem, but it's
    // still a good idea to warn them.

    RCP<OSMV> X_in;
    RCP<OSMV> X_colMap;
    bool copiedInput;
    if (X.isConstantStride()) {
      X_in = rcpFromRef (X);
      copiedInput = false;
    }
    else {
      // Don't just make a copy of X; make a column Map multivector,
      // make a domain Map view of it, and copy X into the domain Map
      // view.
      X_colMap = rcp (new OSMV (colMap, X.getNumVectors ()));
      X_in = X_colMap->offsetViewNonConst (X.getMap (), 0);
      *X_in = X; // deep copy
      copiedInput = true;
      TPETRA_EFFICIENCY_WARNING(
        ! X.isConstantStride (),
        std::runtime_error,
        "gaussSeidel: The current implementation of the Gauss-Seidel kernel "
        "requires that X and B both have constant stride.  Since X does not "
        "have constant stride, we had to make a copy.  This is a limitation of "
        "the current implementation and not your fault, but we still report it "
        "as an efficiency warning for your information.");
    }

    RCP<const OSMV> B_in;
    if (B.isConstantStride()) {
      B_in = rcpFromRef (B);
    }
    else {
      B_in = rcp (new OSMV (B));
      TPETRA_EFFICIENCY_WARNING(
        ! B.isConstantStride (),
        std::runtime_error,
        "gaussSeidel: The current implementation of the Gauss-Seidel kernel "
        "requires that X and B both have constant stride.  Since B does not "
        "have constant stride, we had to make a copy.  This is a limitation of "
        "the current implementation and not your fault, but we still report it "
        "as an efficiency warning for your information.");
    }

    // After this block of code, X_colMap is a view of X, distributed
    // using the column Map of the matrix.
    if (importer.is_null ()) {
      // The domain and column Maps are the same; no need to Import.
      // Don't assign to X_colMap if we've already made it above.
      if (! copiedInput) {
        X_colMap = X_in;
      }
    }
    else {
      // mfh 26 Dec 2012: Alas, I can't currently ask a MultiVector
      // whether it is a view of something else.  This would let me
      // avoid making a new view.  It would also let me check the
      // precondition that X be a domain Map view of a column Map
      // multivector.
      //
      // FIXME (mfh 27 Dec 2012) I can't currently make a view using a
      // Map which is a superset of the OSMV's current Map.  This is
      // because the offsetView* methods check getLocalLength(), which
      // simply asks the OSMV's current Map for the number of elements
      // owned by that process.  It should instead ask the local
      // (Kokkos) OSMV for its number of rows.
      X_colMap = X_in->offsetViewNonConst (colMap, 0);
    }

    for (int sweep = 0; sweep < numSweeps; ++sweep) {
      if (! importer.is_null ()) {
        X_colMap->doImport (*X_in, *importer, INSERT);
      }

      // Do local Gauss-Seidel.
      if (direction != Symmetric) {
        matrix_->template localGaussSeidel<OS,OS> (*B_in, *X_colMap, D,
                                                   dampingFactor, localDirection);
      }
      else { // direction == Symmetric
        matrix_->template localGaussSeidel<OS,OS> (*B_in, *X_colMap, D,
                                                   dampingFactor, Kokkos::Forward);
        // Communicate again before the Backward sweep.
        if (! importer.is_null ()) {
          X_colMap->doImport (*X_in, *importer, INSERT);
        }
        matrix_->template localGaussSeidel<OS,OS> (*B_in, *X_colMap, D,
                                                   dampingFactor, Kokkos::Backward);
      }
    }

    if (copiedInput) { // Copy back from X_in to X.
      X = *X_in;
    }
  }


  template <class OpScalar,
            class MatScalar,
            class LocalOrdinal,
            class GlobalOrdinal,
            class Node,
            class LocalMatOps>
  void
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::
  gaussSeidelCopy (MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> &X,
                   const MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> &B,
                   const MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> &D,
                   const OpScalar& dampingFactor,
                   const ESweepDirection direction,
                   const int numSweeps) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    using Teuchos::rcpFromRef;
    typedef OpScalar OS;
    typedef Teuchos::ScalarTraits<OS> STS;
    typedef Map<LocalOrdinal, GlobalOrdinal, Node> map_type;
    typedef Export<LocalOrdinal, GlobalOrdinal, Node> export_type;
    typedef Import<LocalOrdinal, GlobalOrdinal, Node> import_type;
    typedef MultiVector<OS, LocalOrdinal, GlobalOrdinal, Node> OSMV;

    TEUCHOS_TEST_FOR_EXCEPTION(
      numSweeps < 0,
      std::invalid_argument,
      "gaussSeidelCopy: The number of sweeps must be nonnegative, "
      "but you provided numSweeps = " << numSweeps << " < 0.");

    // Translate from global to local sweep direction.
    // While doing this, validate the input.
    Kokkos::ESweepDirection localDirection;
    if (direction == Forward) {
      localDirection = Kokkos::Forward;
    } else if (direction == Backward) {
      localDirection = Kokkos::Backward;
    } else if (direction == Symmetric) {
      // We'll control local sweep direction manually.
      localDirection = Kokkos::Forward;
    } else {
      TEUCHOS_TEST_FOR_EXCEPTION(true, std::invalid_argument,
        "gaussSeidelCopy: The 'direction' enum does not have any of its "
        "valid values: Forward, Backward, or Symmetric.");
    }

    if (numSweeps == 0) {
      return;
    }

    RCP<const import_type> importer = matrix_->getGraph()->getImporter();
    RCP<const export_type> exporter = matrix_->getGraph()->getExporter();

    RCP<const map_type> domainMap = matrix_->getDomainMap ();
    RCP<const map_type> rangeMap = matrix_->getRangeMap ();
    RCP<const map_type> rowMap = matrix_->getGraph ()->getRowMap ();
    RCP<const map_type> colMap = matrix_->getGraph ()->getColMap ();

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
#endif // HAVE_TEUCHOS_DEBUG

    // Make an overlap multivector X_colMap, and a domain Map view
    // X_domainMap of it.  Both have constant stride by construction.
    RCP<OSMV> X_colMap = rcp (new OSMV (colMap, X.getNumVectors ()));
    RCP<OSMV> X_domainMap = X_colMap->offsetViewNonConst (domainMap, 0);

    // Copy X into X_domainMap (a domain Map view of a column Map
    // multivector X_colMap).  The Gauss-Seidel kernel will work in
    // X_domainMap.  When done, we'll copy back into X.  The copy
    // assumes the the domain and range Maps are the same.
    *X_domainMap = X;

    // The Gauss-Seidel / SOR kernel expects multivectors of constant
    // stride.  X_colMap is by construction, but B might not be.  If
    // it's not, we have to make a copy.
    RCP<const OSMV> B_in;
    if (B.isConstantStride()) {
      B_in = rcpFromRef (B);
    }
    else {
      B_in = rcp (new OSMV (B));
      TPETRA_EFFICIENCY_WARNING(
        ! B.isConstantStride (),
        std::runtime_error,
        "gaussSeidelCopy: The current implementation requires that B have "
        "constant stride.  Since B does not have constant stride, we had to "
        "make a copy.  This is a limitation of the current implementation and "
        "not your fault, but we still report it as an efficiency warning for "
        "your information.");
    }

    for (int sweep = 0; sweep < numSweeps; ++sweep) {
      if (! importer.is_null ()) {
        X_colMap->doImport (*X_domainMap, *importer, INSERT);
      }

      // Do local Gauss-Seidel.
      if (direction != Symmetric) {
        matrix_->template localGaussSeidel<OS,OS> (*B_in, *X_colMap, D,
                                                   dampingFactor, localDirection);
      }
      else { // direction == Symmetric
        matrix_->template localGaussSeidel<OS,OS> (*B_in, *X_colMap, D,
                                                   dampingFactor, Kokkos::Forward);
        // Communicate again before the Backward sweep, if necessary.
        if (! importer.is_null ()) {
          X_colMap->doImport (*X_domainMap, *importer, INSERT);
        }
        matrix_->template localGaussSeidel<OS,OS> (*B_in, *X_colMap, D,
                                                   dampingFactor, Kokkos::Backward);
      }
    }

    // Copy back from X_domainMap to X.
    // This assumes that the domain and range Maps are the same.
    X = *X_domainMap;
  }


  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <class OpScalar, class MatScalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::
  applyNonTranspose (const MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> & X_in,
                     MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> & Y_in,
                     OpScalar alpha,
                     OpScalar beta) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    using Teuchos::rcp_const_cast;
    using Teuchos::rcpFromRef;
    typedef Export<LocalOrdinal,GlobalOrdinal,Node> export_type;
    typedef Import<LocalOrdinal,GlobalOrdinal,Node> import_type;
    typedef Teuchos::ScalarTraits<OpScalar> STS;

    const int myImageID = matrix_->getComm ()->getRank ();

#ifdef TPETRA_CRSMATRIX_MULTIPLY_DUMP
    RCP<Teuchos::FancyOStream> out = Teuchos::VerboseObjectBase::getDefaultOStream();
    if (myImageID == 0) {
      *out << "Entering CrsMatrixMultiplyOp::applyNonTranspose()" << std::endl
           << "Column Map: " << std::endl;
    }
    *out << matrix_->getColMap() << std::endl;
    if (myImageID == 0) {
      *out << "Initial input: " << std::endl;
    }
    X_in.describe (*out, Teuchos::VERB_EXTREME);
#endif // TPETRA_CRSMATRIX_MULTIPLY_DUMP

    // because of Views, it is difficult to determine if X and Y point to the same data.
    // however, if they reference the exact same object, we will do the user the favor of copying X into new storage (with a warning)
    // we ony need to do this if we have trivial importers; otherwise, we don't actually apply the operator from X into Y
    RCP<const import_type> importer = matrix_->getGraph()->getImporter();
    RCP<const export_type> exporter = matrix_->getGraph()->getExporter();

    // If beta == 0, then the output MV will be overwritten; none of
    // its entries should be read.  (Sparse BLAS semantics say that we
    // must ignore any Inf or NaN entries in Y_in, if beta is zero.)
    // This matters if we need to do an Export operation; see below.
    const bool Y_is_overwritten = (beta == STS::zero());

    // We treat the case of a replicated MV output specially.
    const bool Y_is_replicated = ! Y_in.isDistributed ();

    // This is part of the "hack" for replicated MV output.  We'll let
    // each process do its thing, but do an all-reduce at the end to
    // sum up the results.  Setting beta=0 on all processes but Proc 0
    // makes the math work out for the all-reduce.  (This assumes that
    // the replicated data is correctly replicated, so that the data
    // are the same on all processes.)
    if (Y_is_replicated && myImageID > 0) {
      beta = STS::zero();
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
        *X_colMapNonConst = X_in; // MV assignment just copies the data.
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
      {
#ifdef HAVE_KOKKOSCLASSIC_CUDA_NODE_MEMORY_PROFILING
        Teuchos::TimeMonitor lcltimer (*importTimer_);
#endif
        X_colMapNonConst->doImport (X_in, *importer, INSERT);
      }
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
      matrix_->template localMultiply<OpScalar,OpScalar> (*X_colMap, *Y_rowMap,
                                                          Teuchos::NO_TRANS,
                                                          alpha, STS::zero());
      // If we're overwriting the output MV Y_in completely (beta ==
      // 0), then make sure that it is filled with zeros before we do
      // the Export.  Otherwise, the ADD combine mode will use data in
      // Y_in, which is supposed to be zero.
      if (Y_is_overwritten) {
        Y_in.putScalar (STS::zero());
      }
      else {
        // Scale the output MV by beta, so that the Export sums in the
        // mat-vec contribution: Y_in = beta*Y_in + alpha*A*X_in.
        Y_in.scale (beta);
      }
      // Do the Export operation.
      {
#ifdef HAVE_KOKKOSCLASSIC_CUDA_NODE_MEMORY_PROFILING
        Teuchos::TimeMonitor lcltimer (*exportTimer_);
#endif
        Y_in.doExport (*Y_rowMap, *exporter, ADD);
      }
    }
    else { // Don't do an Export: row Map and range Map are the same.
      //
      // If Y_in does not have constant stride, or if the column Map
      // MV aliases Y_in, then we can't let the kernel write directly
      // to Y_in.  Instead, we have to use the cached row (== range)
      // Map MV as temporary storage.
      if (! Y_in.isConstantStride () || X_colMap.getRawPtr () == &Y_in) {
        // Force creating the MV if it hasn't been created already.
        // This will reuse a previously created cached MV.
        Y_rowMap = getRowMapMultiVector (Y_in, true);

        // If beta == 0, we don't need to copy Y_in into Y_rowMap,
        // since we're overwriting it anyway.
        if (beta != STS::zero ()) {
          *Y_rowMap = Y_in;
        }
        matrix_->template localMultiply<OpScalar,OpScalar> (*X_colMap,
                                                            *Y_rowMap,
                                                            Teuchos::NO_TRANS,
                                                            alpha, beta);
        Y_in = *Y_rowMap; // MV assignment just copies the data.
      }
      else {
        matrix_->template localMultiply<OpScalar,OpScalar> (*X_colMap, Y_in,
                                                            Teuchos::NO_TRANS,
                                                            alpha, beta);
      }
    }
#ifdef TPETRA_CRSMATRIX_MULTIPLY_DUMP
    if (myImageID == 0) {
      *out << "Result Y_in after localMultiply and Export:" << std::endl;
    }
    Y_in.describe (*out, Teuchos::VERB_EXTREME);
#endif // TPETRA_CRSMATRIX_MULTIPLY_DUMP

    // If the range Map is a locally replicated Map, sum up
    // contributions from each process.  We set beta = 0 on all
    // processes but Proc 0 initially, so this will handle the scaling
    // factor beta correctly.
    if (Y_is_replicated) {
      Y_in.reduce ();
#ifdef TPETRA_CRSMATRIX_MULTIPLY_DUMP
      if (myImageID == 0) {
        *out << "Result Y_in after reduce:" << std::endl;
      }
      Y_in.describe (*out, Teuchos::VERB_EXTREME);
#endif // TPETRA_CRSMATRIX_MULTIPLY_DUMP
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  template <class OpScalar, class MatScalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::applyTranspose(
               const MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> & X_in,
                     MultiVector<OpScalar,LocalOrdinal,GlobalOrdinal,Node> & Y_in,
               OpScalar alpha, OpScalar beta) const
  {
    typedef Teuchos::ScalarTraits<OpScalar> ST;
    using Teuchos::null;

    int myImageID = Teuchos::rank(*matrix_->getComm());
#ifdef TPETRA_CRSMATRIX_MULTIPLY_DUMP
    Teuchos::RCP<Teuchos::FancyOStream> out = Teuchos::VerboseObjectBase::getDefaultOStream();
    if (myImageID == 0) {
      *out << "Entering CrsMatrixMultiplyOp::applyTranspose()" << std::endl
                << "Column Map: " << std::endl;
    }
    *out << matrix_->getColMap() << std::endl;
    if (myImageID == 0) {
      *out << "Initial input: " << std::endl;
    }
    X_in.describe(*out,Teuchos::VERB_EXTREME);
#endif

    const size_t numVectors = X_in.getNumVectors();
    // because of Views, it is difficult to determine if X and Y point to the same data.
    // however, if they reference the exact same object, we will do the user the favor of copying X into new storage (with a warning)
    // we ony need to do this if we have trivial importers; otherwise, we don't actually apply the operator from X into Y
    Teuchos::RCP<const Import<LocalOrdinal,GlobalOrdinal,Node> > importer = matrix_->getGraph()->getImporter();
    Teuchos::RCP<const Export<LocalOrdinal,GlobalOrdinal,Node> > exporter = matrix_->getGraph()->getExporter();
    // access X indirectly, in case we need to create temporary storage
    Teuchos::RCP<const MV> X;

    // some parameters for below
    const bool Y_is_replicated = !Y_in.isDistributed(),
               Y_is_overwritten = (beta == ST::zero());
    if (Y_is_replicated && myImageID > 0) {
      beta = ST::zero();
    }

    // currently, cannot multiply from multivector of non-constant stride
    if (X_in.isConstantStride() == false && importer==null) {
      // generate a strided copy of X_in
      X = Teuchos::rcp(new MV(X_in));
#ifdef TPETRA_CRSMATRIX_MULTIPLY_DUMP
      if (myImageID == 0) *out << "X is not constant stride, duplicating X results in a strided copy" << std::endl;
      X->describe(*out,Teuchos::VERB_EXTREME);
#endif
    }
    else {
      // just temporary, so this non-owning RCP is okay
      X = Teuchos::rcp(&X_in, false);
    }

    // set up import/export temporary multivectors
    if (importer != null) {
      if (importMV_ != null && importMV_->getNumVectors() != numVectors) importMV_ = null;
      if (importMV_ == null) {
        importMV_ = Teuchos::rcp( new MV(matrix_->getColMap(),numVectors) );
      }
    }
    if (exporter != null) {
      if (exportMV_ != null && exportMV_->getNumVectors() != numVectors) exportMV_ = null;
      if (exportMV_ == null) {
        exportMV_ = Teuchos::rcp( new MV(matrix_->getRowMap(),numVectors) );
      }
    }

    // If we have a non-trivial exporter, we must import elements that are permuted or are on other processors
    if (exporter != null) {
      {
#ifdef HAVE_KOKKOSCLASSIC_CUDA_NODE_MEMORY_PROFILING
        Teuchos::TimeMonitor lcltimer(*importTimer_);
#endif
        exportMV_->doImport(X_in,*exporter,INSERT);
      }
      // multiply out of exportMV_
      X = exportMV_;
#ifdef TPETRA_CRSMATRIX_MULTIPLY_DUMP
      if (myImageID == 0) {
        *out << "Performed import of X using exporter..." << std::endl;
      }
      X->describe(*out,Teuchos::VERB_EXTREME);
#endif
    }


    // If we have a non-trivial importer, we must export elements that are permuted or belong to other processors
    // We will compute solution into the to-be-exported MV; get a view
    if (importer != null) {
      // Do actual computation
      matrix_->template localMultiply<OpScalar,OpScalar>(*X, *importMV_, Teuchos::CONJ_TRANS, alpha, ST::zero());
#ifdef TPETRA_CRSMATRIX_MULTIPLY_DUMP
      if (myImageID == 0) *out << "Import vector after localMultiply()..." << std::endl;
      importMV_->describe(*out,Teuchos::VERB_EXTREME);
#endif
      if (Y_is_overwritten) Y_in.putScalar(ST::zero());
      else                  Y_in.scale(beta);
      //
      {
#ifdef HAVE_KOKKOSCLASSIC_CUDA_NODE_MEMORY_PROFILING
        Teuchos::TimeMonitor lcltimer(*importTimer_);
#endif
        Y_in.doExport(*importMV_,*importer,ADD);
      }
    }
    // otherwise, multiply into Y
    else {
      // can't multiply in-situ; can't multiply into non-strided multivector
      if (Y_in.isConstantStride() == false || X.getRawPtr() == &Y_in) {
        // generate a strided copy of Y
        MV Y(Y_in);
        matrix_->template localMultiply<OpScalar,OpScalar>(*X, Y, Teuchos::CONJ_TRANS, alpha, beta);
        Y_in = Y;
      }
      else {
        matrix_->template localMultiply<OpScalar,OpScalar>(*X, Y_in, Teuchos::CONJ_TRANS, alpha, beta);
      }
    }
#ifdef TPETRA_CRSMATRIX_MULTIPLY_DUMP
    if (myImageID == 0) *out << "Y_in vector after local multiply/export..." << std::endl;
    Y_in.describe(*out,Teuchos::VERB_EXTREME);
#endif
    // Handle case of rangemap being a local replicated map: in this case, sum contributions from each processor
    if (Y_is_replicated) {
      Y_in.reduce();
#ifdef TPETRA_CRSMATRIX_MULTIPLY_DUMP
      if (myImageID == 0) *out << "Output vector is local; result after reduce()..." << std::endl;
      Y_in.describe(*out,Teuchos::VERB_EXTREME);
#endif
    }
  }

  template <class OpScalar, class MatScalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  bool
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::hasTransposeApply() const {
    return true;
  }

  template <class OpScalar, class MatScalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  const Teuchos::RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > &
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::getDomainMap() const {
    return matrix_->getDomainMap();
  }

  template <class OpScalar, class MatScalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  const Teuchos::RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > &
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::getRangeMap() const {
    return matrix_->getRangeMap();
  }

  template <class OpScalar,
            class MatScalar,
            class LocalOrdinal,
            class GlobalOrdinal,
            class Node,
            class LocalMatOps>
  Teuchos::RCP<MultiVector<OpScalar, LocalOrdinal, GlobalOrdinal, Node> >
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::
  getColumnMapMultiVector (const MultiVector<OpScalar, LocalOrdinal, GlobalOrdinal, Node>& X_domainMap,
                           const bool force) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    typedef Import<LocalOrdinal,GlobalOrdinal,Node> import_type;
    typedef Map<LocalOrdinal,GlobalOrdinal,Node> map_type;

    const size_t numVecs = X_domainMap.getNumVectors ();
    RCP<const import_type> importer = matrix_->getGraph ()->getImporter ();
    RCP<const map_type> colMap = matrix_->getColMap ();

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


  template <class OpScalar,
            class MatScalar,
            class LocalOrdinal,
            class GlobalOrdinal,
            class Node,
            class LocalMatOps>
  Teuchos::RCP<MultiVector<OpScalar, LocalOrdinal, GlobalOrdinal, Node> >
  CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::
  getRowMapMultiVector (const MultiVector<OpScalar, LocalOrdinal, GlobalOrdinal, Node>& Y_rangeMap,
                        const bool force) const
  {
    using Teuchos::null;
    using Teuchos::RCP;
    using Teuchos::rcp;
    typedef Export<LocalOrdinal,GlobalOrdinal,Node> export_type;
    typedef Map<LocalOrdinal,GlobalOrdinal,Node> map_type;

    const size_t numVecs = Y_rangeMap.getNumVectors ();
    RCP<const export_type> exporter = matrix_->getGraph ()->getExporter ();
    RCP<const map_type> rowMap = matrix_->getRowMap ();

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

} // namespace Tpetra

template <class OpScalar, class MatScalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
Teuchos::RCP< Tpetra::CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps> >
Tpetra::createCrsMatrixMultiplyOp(const Teuchos::RCP<const Tpetra::CrsMatrix<MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps> > &A) {
  return Teuchos::rcp(new Tpetra::CrsMatrixMultiplyOp<OpScalar,MatScalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>(A) );
}

//
// Explicit instantiation macro
//
// Must be expanded from within the Tpetra namespace!
//

//! Explicit instantiation macro supporting the CrsMatrixMultiplyOp class. Instantiates the class, the non-member constructor, and the necessary CrsMatrix::multiply() member.
#define TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(OPSCALAR,MATSCALAR,LO,GO,NODE) \
  \
  template class CrsMatrixMultiplyOp< OPSCALAR , MATSCALAR , LO , GO , NODE >; \
  \
  template Teuchos::RCP< Tpetra::CrsMatrixMultiplyOp<OPSCALAR,MATSCALAR,LO,GO,NODE> > \
  createCrsMatrixMultiplyOp(const Teuchos::RCP<const Tpetra::CrsMatrix<MATSCALAR,LO,GO,NODE> > &A); \
  \
  template void CrsMatrix<MATSCALAR,LO,GO,NODE>::localMultiply<OPSCALAR,OPSCALAR>( \
        const MultiVector<OPSCALAR,LO,GO,NODE> &X, \
              MultiVector<OPSCALAR,LO,GO,NODE> &Y, \
              Teuchos::ETransp mode,               \
              OPSCALAR alpha, OPSCALAR beta        \
              ) const; \
  template void CrsMatrix<MATSCALAR,LO,GO,NODE>::localGaussSeidel<OPSCALAR,OPSCALAR>( \
        const MultiVector<OPSCALAR,LO,GO,NODE> &B, \
              MultiVector<OPSCALAR,LO,GO,NODE> &X, \
        const MultiVector<OPSCALAR,LO,GO,NODE> &D, \
        const OPSCALAR& alpha, \
        const Kokkos::ESweepDirection direction) const;

#define TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT_SINGLE(SCALAR,LO,GO,NODE) \
        TPETRA_CRSMATRIX_MULTIPLYOP_INSTANT(SCALAR,SCALAR,LO,GO,NODE)

#endif // TPETRA_CRSMATRIXMULTIPLYOP_DEF_HPP
