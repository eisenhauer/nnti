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
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef XPETRA_CRSMATRIX_HPP
#define XPETRA_CRSMATRIX_HPP

/* this file is automatically generated - do not edit (see script/interfaces.py) */

#include <Kokkos_DefaultNode.hpp>
#include <Kokkos_DefaultKernels.hpp>
#include "Xpetra_ConfigDefs.hpp"
#include "Xpetra_RowMatrix.hpp"
#include "Xpetra_DistObject.hpp"
#include "Xpetra_CrsGraph.hpp"
#include "Xpetra_Vector.hpp"

namespace Xpetra {

  template <class Scalar, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = KokkosClassic::DefaultNode::DefaultNodeType, class LocalMatOps = typename KokkosClassic::DefaultKernels<Scalar,LocalOrdinal,Node>::SparseOps>
  class CrsMatrix
    : public RowMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node>, public DistObject<char, LocalOrdinal,GlobalOrdinal,Node>
  {

  public:

    //! @name Constructor/Destructor Methods
    //@{

    //! Destructor.
    virtual ~CrsMatrix() { }

   //@}

    //! @name Insertion/Removal Methods
    //@{

    //! Insert matrix entries, using global IDs.
    virtual void insertGlobalValues(GlobalOrdinal globalRow, const ArrayView< const GlobalOrdinal > &cols, const ArrayView< const Scalar > &vals)= 0;

    //! Insert matrix entries, using local IDs.
    virtual void insertLocalValues(LocalOrdinal localRow, const ArrayView< const LocalOrdinal > &cols, const ArrayView< const Scalar > &vals)= 0;

    //! Replace matrix entries, using global IDs.
    virtual void replaceGlobalValues(GlobalOrdinal globalRow, const ArrayView< const GlobalOrdinal > &cols, const ArrayView< const Scalar > &vals)= 0;

    //! Replace matrix entries, using local IDs.
    virtual void replaceLocalValues(LocalOrdinal localRow, const ArrayView< const LocalOrdinal > &cols, const ArrayView< const Scalar > &vals)= 0;

    //! Set all matrix entries equal to scalarThis.
    virtual void setAllToScalar(const Scalar &alpha)= 0;

    //! Scale the current values of a matrix, this = alpha*this.
    virtual void scale(const Scalar &alpha)= 0;

    //! Allocates and returns ArrayRCPs of the Crs arrays --- This is an Xpetra-only routine.
    //** \warning This is an expert-only routine and should not be called from user code. */
    virtual void allocateAllValues(size_t numNonZeros,ArrayRCP<size_t> & rowptr, ArrayRCP<LocalOrdinal> & colind, ArrayRCP<Scalar> & values)=0;

    //! Sets the 1D pointer arrays of the graph.
    virtual void setAllValues(const ArrayRCP<size_t> & rowptr, const ArrayRCP<LocalOrdinal> & colind, const ArrayRCP<Scalar> & values)=0;

    //! Gets the 1D pointer arrays of the graph.
    virtual void getAllValues(ArrayRCP<const size_t>& rowptr, ArrayRCP<const LocalOrdinal>& colind, ArrayRCP<const Scalar>& values) const = 0;

    //@}

    //! @name Transformational Methods
    //@{

    //!
    virtual void resumeFill(const RCP< ParameterList > &params=null)= 0;

    //! Signal that data entry is complete, specifying domain and range maps.
    virtual void fillComplete(const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &domainMap, const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &rangeMap, const RCP< ParameterList > &params=null)= 0;

    //! Signal that data entry is complete.
    virtual void fillComplete(const RCP< ParameterList > &params=null)= 0;

    //!  Replaces the current domainMap and importer with the user-specified objects.
    virtual void replaceDomainMapAndImporter(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >& newDomainMap, Teuchos::RCP<const Import<LocalOrdinal,GlobalOrdinal,Node> >  & newImporter)=0;

    //! Expert static fill complete
    virtual void expertStaticFillComplete(const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & domainMap,
					  const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> > & rangeMap,
					  const RCP<const Import<LocalOrdinal,GlobalOrdinal,Node> > &importer=Teuchos::null,
					  const RCP<const Export<LocalOrdinal,GlobalOrdinal,Node> > &exporter=Teuchos::null,
					  const RCP<ParameterList> &params=Teuchos::null) = 0;
    //@}

    //! @name Methods implementing RowMatrix
    //@{

    //! Returns the Map that describes the row distribution in this matrix.
    virtual const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getRowMap() const = 0;

    //! Returns the Map that describes the column distribution in this matrix.
    virtual const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getColMap() const = 0;

    //! Returns the CrsGraph associated with this matrix.
    virtual RCP< const CrsGraph< LocalOrdinal, GlobalOrdinal, Node, LocalMatOps > > getCrsGraph() const = 0;

    //! Number of global elements in the row map of this matrix.
    virtual global_size_t getGlobalNumRows() const = 0;

    //! Number of global columns in the matrix.
    virtual global_size_t getGlobalNumCols() const = 0;

    //! Returns the number of matrix rows owned on the calling node.
    virtual size_t getNodeNumRows() const = 0;

    //! Returns the global number of entries in this matrix.
    virtual global_size_t getGlobalNumEntries() const = 0;

    //! Returns the local number of entries in this matrix.
    virtual size_t getNodeNumEntries() const = 0;

    //! Returns the current number of entries on this node in the specified local row.
    virtual size_t getNumEntriesInLocalRow(LocalOrdinal localRow) const = 0;

    //! Returns the number of global diagonal entries, based on global row/column index comparisons.
    virtual global_size_t getGlobalNumDiags() const = 0;

    //! Returns the number of local diagonal entries, based on global row/column index comparisons.
    virtual size_t getNodeNumDiags() const = 0;

    //! Returns the maximum number of entries across all rows/columns on all nodes.
    virtual size_t getGlobalMaxNumRowEntries() const = 0;

    //! Returns the maximum number of entries across all rows/columns on this node.
    virtual size_t getNodeMaxNumRowEntries() const = 0;

    //! If matrix indices are in the local range, this function returns true. Otherwise, this function returns false.
    virtual bool isLocallyIndexed() const = 0;

    //! If matrix indices are in the global range, this function returns true. Otherwise, this function returns false.
    virtual bool isGloballyIndexed() const = 0;

    //! Returns true if the matrix is in compute mode, i.e. if fillComplete() has been called.
    virtual bool isFillComplete() const = 0;

    //! Returns true if the matrix is in edit mode.
    virtual bool isFillActive() const = 0;

    //! Returns the Frobenius norm of the matrix.
    virtual typename ScalarTraits< Scalar >::magnitudeType getFrobeniusNorm() const = 0;

    //! Returns true if getLocalRowView() and getGlobalRowView() are valid for this class.
    virtual bool supportsRowViews() const = 0;

    //! Extract a const, non-persisting view of global indices in a specified row of the matrix.
    virtual void getGlobalRowView(GlobalOrdinal GlobalRow, ArrayView< const GlobalOrdinal > &indices, ArrayView< const Scalar > &values) const = 0;

    //! Extract a list of entries in a specified global row of this matrix. Put into pre-allocated storage.
    virtual void getGlobalRowCopy(GlobalOrdinal GlobalRow, const ArrayView<GlobalOrdinal> &indices, const ArrayView<Scalar> &values, size_t &numEntries) const = 0;

    //! Extract a const, non-persisting view of local indices in a specified row of the matrix.
    virtual void getLocalRowView(LocalOrdinal LocalRow, ArrayView< const LocalOrdinal > &indices, ArrayView< const Scalar > &values) const = 0;

    //! Get a copy of the diagonal entries owned by this node, with local row indices.
    virtual void getLocalDiagCopy(Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &diag) const = 0;

    //! Get offsets of the diagonal entries in the matrix.
    virtual void getLocalDiagOffsets(Teuchos::ArrayRCP<size_t> &offsets) const = 0;

    //! Get a copy of the diagonal entries owned by this node, with local row indices, using row offsets.
    virtual void getLocalDiagCopy(Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &diag, const Teuchos::ArrayView<const size_t> &offsets) const = 0;

    virtual void removeEmptyProcessesInPlace(const RCP<const Map<LocalOrdinal, GlobalOrdinal, Node> >& newMap) = 0;

    //@}

    //! @name Methods implementing Operator
    //@{

    //! Computes the sparse matrix-multivector multiplication.
    virtual void apply(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &X, MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &Y, Teuchos::ETransp mode=Teuchos::NO_TRANS, Scalar alpha=ScalarTraits< Scalar >::one(), Scalar beta=ScalarTraits< Scalar >::zero()) const = 0;

    //! Returns the Map associated with the domain of this operator. This will be null until fillComplete() is called.
    virtual const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getDomainMap() const = 0;

    //!
    virtual const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getRangeMap() const = 0;

    //@}

    //! @name Overridden from Teuchos::Describable
    //@{

    //! A simple one-line description of this object.
    virtual std::string description() const = 0;

    //! Print the object with some verbosity level to an FancyOStream object.
    virtual void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const = 0;

    //@}
    //  Adding these functions by hand, as they're in the skip list.

    //! Returns the number of matrix columns owned on the calling node.
        virtual size_t getNodeNumCols() const = 0;

    //! Extract a list of entries in a specified local row of the matrix. Put into storage allocated by calling routine.
        virtual void getLocalRowCopy(LocalOrdinal LocalRow, const ArrayView< LocalOrdinal > &Indices, const ArrayView< Scalar > &Values, size_t &NumEntries) const = 0;

    //! Does this have an underlying matrix
    virtual bool hasMatrix() const = 0;


  }; // CrsMatrix class

} // Xpetra namespace

#define XPETRA_CRSMATRIX_SHORT
#endif // XPETRA_CRSMATRIX_HPP
