/*@HEADER
// ***********************************************************************
//
//       Ifpack2: Tempated Object-Oriented Algebraic Preconditioner Package
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
//@HEADER
*/

#ifndef IFPACK2_CONTAINER_HPP
#define IFPACK2_CONTAINER_HPP

/// \file Ifpack2_Container.hpp
/// \brief Ifpack2::Container class declaration

#include <Ifpack2_ConfigDefs.hpp>
#include <Tpetra_RowMatrix.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_Describable.hpp>
#include <iostream>

namespace Ifpack2 {

/// \class Container
/// \brief Interface for creating and solving a local linear problem.
///
/// This class is mainly useful for BlockRelaxation and other
/// operators that need to solve linear systems with diagonal blocks
/// of a sparse matrix.  Users of BlockRelaxation do not normally need
/// to interact with the Container interface.  However, they do need
/// to specify a specific Container subclass to use.  Implementations
/// of Container specify
/// - the kind of data structure used to store diagonal blocks, and
/// - how linear systems with those diagonal blocks are solved.
///
/// For example, the SparseContainer subclass uses a Tpetra sparse
/// matrix to store each diagonal block, and can use any given Ifpack2
/// Preconditioner subclass to solve linear systems.
///
/// A Container can create, populate, and solve a local linear
/// system. The local linear system matrix, B, is a submatrix of the
/// local components of a distributed matrix, A. The idea of Container
/// is to specify the rows of A that are contained in B, then extract
/// B from A, and compute all it is necessary to solve a linear system
/// in B.  Then, set the initial guess (if necessary) and right-hand
/// side for B, and solve the linear system in B.
///
/// If you are writing a class (comparable to BlockRelaxation) that
/// uses Container, you should use it in the following way:
/// <ol>
/// <li> Create a Container object, specifying the number of rows of B
///      and the indices of the local rows of A that are contained in
///      B.  Both of these come from a Partitioner object.</li>
/// <li> Optionally, set linear solve parameters using setParameters().</li>
/// <li> Initialize the container by calling initialize().</li>
/// <li> Prepare the linear system solver using compute(), passing in
///      the original matrix from which to extract the diagonal
///      block.</li>
/// <li> Solve the linear system using apply().</li>
/// </ol>
/// For an example of Steps 1-5 above, see the implementation of
/// BlockRelaxation::ExtractSubmatrices() in
/// Ifpack2_BlockRelaxation_def.hpp.
///
/// \c MatrixType and \c InverseType may store values of different
/// types, and may have different template parameters (e.g., local or
/// global ordinal types).  You may mix and match so long as implicit
/// conversions are available.  The most obvious use case for this
/// are:
/// - <tt>MatrixType::global_ordinal_type=long long</tt> and
///   <tt>InverseType::global_ordinal_type=short</tt>
/// - <tt>MatrixType::scalar_type=float</tt> and
///   <tt>InverseType::scalar_type=double</tt>
template<class MatrixType, class InverseType>
class Container : public Teuchos::Describable {
public:
  typedef typename MatrixType::scalar_type          MatrixScalar;
  typedef typename MatrixType::local_ordinal_type   MatrixLocalOrdinal;
  typedef typename MatrixType::global_ordinal_type  MatrixGlobalOrdinal;
  typedef typename MatrixType::node_type            MatrixNode;

  typedef typename InverseType::scalar_type         InverseScalar;
  typedef typename InverseType::local_ordinal_type  InverseLocalOrdinal;
  typedef typename InverseType::global_ordinal_type InverseGlobalOrdinal;
  typedef typename InverseType::node_type           InverseNode;

  typedef typename MatrixType::scalar_type          scalar_type;
  typedef typename MatrixType::local_ordinal_type   local_ordinal_type;
  typedef typename MatrixType::global_ordinal_type  global_ordinal_type;
  typedef typename MatrixType::node_type            node_type;

  typedef Tpetra::RowMatrix<scalar_type, local_ordinal_type, global_ordinal_type, node_type> row_matrix_type;

  /// \brief Constructor.
  ///
  /// \param localRows [in] The set of (local) rows assigned to this
  ///   container.  <tt>localRows[i] == j</tt>, where i (from 0 to
  ///   <tt>getNumRows() - 1</tt>) indicates the Container's row, and
  ///   j indicates the local row in the calling process.  Subclasses
  ///   must always pass along these indices to the base class.
  Container (const Teuchos::ArrayView<const MatrixLocalOrdinal>& localRows) :
    localRows_ (localRows.begin (), localRows.end ())
  {}

  //! Destructor.
  virtual ~Container() {};

  //! The number of rows in the diagonal block.
  virtual size_t getNumRows() const = 0;

  //! Local indices of the rows of the input matrix that belong to this block.
  Teuchos::ArrayView<const MatrixLocalOrdinal> getLocalRows () const {
    return localRows_ ();
  }

  /// \brief The local index associated with local row i.
  ///
  /// The set of (local) rows assigned to this container is defined by
  /// passing in a set of indices <tt>localRows[i] = j</tt> to the
  /// constructor, where i (from 0 to <tt>getNumRows() - 1</tt>)
  /// indicates the Container's row, and j indicates the local row in
  /// the calling process.  Subclasses must always pass along these
  /// indices to the base class.
  ///
  /// The indices are usually used to reorder the local row index (on
  /// the calling process) of the i-th row in the Container.
  ///
  /// For an example of how to use these indices, see the
  /// implementation of BlockRelaxation::ExtractSubmatrices() in
  /// Ifpack2_BlockRelaxation_def.hpp.
  MatrixLocalOrdinal ID (const size_t i) const {
    return localRows_[i];
  }

  //! Initialize the container, by performing all operations that only require matrix structure.
  virtual void initialize () = 0;

  //! Finalize the linear system matrix and prepare for the application of the inverse.
  virtual void compute (const Teuchos::RCP<const row_matrix_type>& Matrix) = 0;

  //! Set parameters.
  virtual void setParameters (const Teuchos::ParameterList& List) = 0;

  //! Return \c true if the container has been successfully initialized.
  virtual bool isInitialized () const = 0;

  //! Return \c true if the container has been successfully computed.
  virtual bool isComputed () const = 0;

  /// \brief Compute <tt>Y := alpha * M^{-1} X + beta*Y</tt>.
  ///
  /// X is in the domain Map of the original matrix (the argument to
  /// compute()), and Y is in the range Map of the original matrix.
  /// This method only reads resp. modifies the permuted subset of
  /// entries of X resp. Y related to the diagonal block M.  That
  /// permuted subset is defined by the indices passed into the
  /// constructor.
  ///
  /// This method is marked \c const for compatibility with
  /// Tpetra::Operator's method of the same name.  This might require
  /// subclasses to mark some of their instance data as \c mutable.
  virtual void
  apply (const Tpetra::MultiVector<MatrixScalar,MatrixLocalOrdinal,MatrixGlobalOrdinal,MatrixNode>& X,
         Tpetra::MultiVector<MatrixScalar,MatrixLocalOrdinal,MatrixGlobalOrdinal,MatrixNode>& Y,
         Teuchos::ETransp mode = Teuchos::NO_TRANS,
         MatrixScalar alpha = Teuchos::ScalarTraits<MatrixScalar>::one (),
         MatrixScalar beta = Teuchos::ScalarTraits<MatrixScalar>::zero ()) const = 0;

  /// \brief Compute <tt>Y := alpha * diag(D) * M^{-1} (diag(D) * X) + beta*Y</tt>.
  ///
  /// X is in the domain Map of the original matrix (the argument to
  /// compute()), and Y is in the range Map of the original matrix.
  /// This method only reads resp. modifies the permuted subset of
  /// entries of X resp. Y related to the diagonal block M.  That
  /// permuted subset is defined by the indices passed into the
  /// constructor.  The D scaling vector must have the same number of
  /// entries on each process as X and Y, but otherwise need not have
  /// the same Map.  (For example, D could be locally replicated, or
  /// could be a different object on each process with a local (\c
  /// MPI_COMM_SELF) communicator.)
  ///
  /// This method supports overlap techniques, such as those used in
  /// Schwarz methods.
  ///
  /// This method is marked \c const by analogy with apply(), which
  /// itself is marked \c const for compatibility with
  /// Tpetra::Operator's method of the same name.  This might require
  /// subclasses to mark some of their instance data as \c mutable.
  virtual void
  weightedApply (const Tpetra::MultiVector<MatrixScalar,MatrixLocalOrdinal,MatrixGlobalOrdinal,MatrixNode>& X,
                 Tpetra::MultiVector<MatrixScalar,MatrixLocalOrdinal,MatrixGlobalOrdinal,MatrixNode>& Y,
                 const Tpetra::Vector<MatrixScalar,MatrixLocalOrdinal,MatrixGlobalOrdinal,MatrixNode>& D,
                 Teuchos::ETransp mode = Teuchos::NO_TRANS,
                 MatrixScalar alpha = Teuchos::ScalarTraits<MatrixScalar>::one (),
                 MatrixScalar beta = Teuchos::ScalarTraits<MatrixScalar>::zero ()) const = 0;

  //! Print basic information about the container to \c os.
  virtual std::ostream& print (std::ostream& os) const = 0;

private:
  //! Local indices of the rows of the input matrix that belong to this block.
  Teuchos::Array<MatrixLocalOrdinal> localRows_;
};

template <class MatrixType, class InverseType>
inline std::ostream&
operator<< (std::ostream& os, const Ifpack2::Container<MatrixType,InverseType>& obj)
{
  return obj.print (os);
}

}

#endif // IFPACK2_CONTAINER_HPP
