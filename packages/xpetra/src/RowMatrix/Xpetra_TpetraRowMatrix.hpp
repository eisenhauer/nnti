// @HEADER
//
// ***********************************************************************
//
//             Xpetra: A linear algebra interface package
//                  Copyright 2012 Sandia Corporation
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
// Questions? Contact
//                    Jeremie Gaidamour (jngaida@sandia.gov)
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef XPETRA_TPETRAROWMATRIX_HPP
#define XPETRA_TPETRAROWMATRIX_HPP

/* this file is automatically generated - do not edit (see script/tpetra.py) */

#include "Xpetra_TpetraConfigDefs.hpp"

#include "Tpetra_RowMatrix.hpp"

namespace Xpetra {

  template <class Scalar, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType>
  class TpetraRowMatrix
    : public RowMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node>
  {

  public:

    //! @name Destructor Method
    //@{

    //! Destructor.
    virtual ~TpetraRowMatrix() {  }

    //@}

    //! @name Matrix Query Methods
    //@{

    //! Returns the Map that describes the row distribution in this matrix.
    const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getRowMap() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getRowMap"); return mtx_->getRowMap(); }

    //! Returns the Map that describes the column distribution in this matrix.
    const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getColMap() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getColMap"); return mtx_->getColMap(); }

    //! Returns the number of global rows in this matrix.
    global_size_t getGlobalNumRows() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getGlobalNumRows"); return mtx_->getGlobalNumRows(); }

    //! Returns the number of global columns in this matrix.
    global_size_t getGlobalNumCols() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getGlobalNumCols"); return mtx_->getGlobalNumCols(); }

    //! Returns the number of rows owned on the calling node.
    size_t getNodeNumRows() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getNodeNumRows"); return mtx_->getNodeNumRows(); }

    //! Returns the number of columns needed to apply the forward operator on this node, i.e., the number of elements listed in the column map.
    size_t getNodeNumCols() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getNodeNumCols"); return mtx_->getNodeNumCols(); }

    //! Returns the global number of entries in this matrix.
    global_size_t getGlobalNumEntries() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getGlobalNumEntries"); return mtx_->getGlobalNumEntries(); }

    //! Returns the local number of entries in this matrix.
    size_t getNodeNumEntries() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getNodeNumEntries"); return mtx_->getNodeNumEntries(); }

    //! Returns the current number of entries on this node in the specified local row.
    size_t getNumEntriesInLocalRow(LocalOrdinal localRow) const =0 { XPETRA_MONITOR("TpetraRowMatrix::getNumEntriesInLocalRow"); return mtx_->getNumEntriesInLocalRow(localRow); }

    //! Returns the number of global diagonal entries, based on global row/column index comparisons.
    global_size_t getGlobalNumDiags() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getGlobalNumDiags"); return mtx_->getGlobalNumDiags(); }

    //! Returns the number of local diagonal entries, based on global row/column index comparisons.
    size_t getNodeNumDiags() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getNodeNumDiags"); return mtx_->getNodeNumDiags(); }

    //! Returns the maximum number of entries across all rows/columns on all nodes.
    size_t getGlobalMaxNumRowEntries() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getGlobalMaxNumRowEntries"); return mtx_->getGlobalMaxNumRowEntries(); }

    //! Returns the maximum number of entries across all rows/columns on this node.
    size_t getNodeMaxNumRowEntries() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getNodeMaxNumRowEntries"); return mtx_->getNodeMaxNumRowEntries(); }

    //! If matrix indices are in the local range, this function returns true. Otherwise, this function returns false. */.
    bool isLocallyIndexed() const =0 { XPETRA_MONITOR("TpetraRowMatrix::isLocallyIndexed"); return mtx_->isLocallyIndexed(); }

    //! If matrix indices are in the global range, this function returns true. Otherwise, this function returns false. */.
    bool isGloballyIndexed() const =0 { XPETRA_MONITOR("TpetraRowMatrix::isGloballyIndexed"); return mtx_->isGloballyIndexed(); }

    //! Returns true if fillComplete() has been called.
    bool isFillComplete() const =0 { XPETRA_MONITOR("TpetraRowMatrix::isFillComplete"); return mtx_->isFillComplete(); }

    //@}

    //! @name Extraction Methods
    //@{

    //! Extract a list of entries in a specified local row of the graph. Put into storage allocated by calling routine.
    void getLocalRowCopy(LocalOrdinal LocalRow, const Teuchos::ArrayView< LocalOrdinal > &Indices, const Teuchos::ArrayView< Scalar > &Values, size_t &NumEntries) const =0 { XPETRA_MONITOR("TpetraRowMatrix::getLocalRowCopy"); mtx_->getLocalRowCopy(LocalRow, Indices, Values, NumEntries); }

    //! Extract a const, non-persisting view of global indices in a specified row of the matrix.
    void getGlobalRowView(GlobalOrdinal GlobalRow, ArrayView< const GlobalOrdinal > &indices, ArrayView< const Scalar > &values) const =0 { XPETRA_MONITOR("TpetraRowMatrix::getGlobalRowView"); mtx_->getGlobalRowView(GlobalRow, indices, values); }

    //! Extract a const, non-persisting view of local indices in a specified row of the matrix.
    void getLocalRowView(LocalOrdinal LocalRow, ArrayView< const LocalOrdinal > &indices, ArrayView< const Scalar > &values) const =0 { XPETRA_MONITOR("TpetraRowMatrix::getLocalRowView"); mtx_->getLocalRowView(LocalRow, indices, values); }

    //! Get a copy of the diagonal entries owned by this node, with local row indices.
    void getLocalDiagCopy(Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &diag) const =0 { XPETRA_MONITOR("TpetraRowMatrix::getLocalDiagCopy"); mtx_->getLocalDiagCopy(diag); }

    //@}

    //! @name Mathematical Methods
    //@{

    //! Returns the Frobenius norm of the matrix.
    typename ScalarTraits< Scalar >::magnitudeType getFrobeniusNorm() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getFrobeniusNorm"); return mtx_->getFrobeniusNorm(); }

    //@}

    //! @name Pure virtual functions to be overridden by subclasses.
    //@{

    //! Returns the Map associated with the domain of this operator, which must be compatible with X.getMap().
    const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getDomainMap() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getDomainMap"); return mtx_->getDomainMap(); }

    //! Returns the Map associated with the range of this operator, which must be compatible with Y.getMap().
    const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getRangeMap() const =0 { XPETRA_MONITOR("TpetraRowMatrix::getRangeMap"); return mtx_->getRangeMap(); }

    //! Computes the operator-multivector application.
    void apply(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &X, MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &Y, Teuchos::ETransp mode=Teuchos::NO_TRANS, Scalar alpha=Teuchos::ScalarTraits< Scalar >::one(), Scalar beta=Teuchos::ScalarTraits< Scalar >::zero()) const =0 { XPETRA_MONITOR("TpetraRowMatrix::apply"); mtx_->apply(X, Y, mode, alpha, beta); }

    //@}

    //! @name Xpetra specific
    //@{

    //! TpetraCrsMatrix constructor to wrap a Tpetra::CrsMatrix object
    TpetraRowMatrix(const Teuchos::RCP<Tpetra::RowMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > &mtx) : mtx_(mtx) {  }

    //! Get the underlying Tpetra matrix
    RCP<const Tpetra::RowMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > getTpetra_RowMatrix() const { return mtx_; }
    
    //! Get the underlying Tpetra matrix
    RCP<Tpetra::RowMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > getTpetra_RowMatrixNonConst() const { return mtx_; } //TODO: remove
 
   //@}
    
  private:
    
    RCP< Tpetra::RowMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node> > mtx_;

  }; // TpetraRowMatrix class

} // Xpetra namespace

#define XPETRA_TPETRAROWMATRIX_SHORT
#endif // XPETRA_TPETRAROWMATRIX_HPP
