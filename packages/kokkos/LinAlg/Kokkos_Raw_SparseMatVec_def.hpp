//@HEADER
// ************************************************************************
// 
//          Kokkos: Node API and Parallel Node Kernels
//              Copyright (2008) Sandia Corporation
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
//@HEADER

#ifndef __Kokkos_Raw_SparseMatVec_def_hpp
#define __Kokkos_Raw_SparseMatVec_def_hpp

/// \file Kokkos_Raw_SparseMatVec_def.hpp
/// \brief Definitions of "raw" sequential sparse triangular solve routines.
/// \warning This code was generated by the SparseTriSolve.py script.  
///   If you edit this header by hand, your edits will disappear the 
///   next time you run the generator script.

namespace Kokkos {
namespace Raw {

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
matVecCscColMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  const RangeScalar& beta,
  RangeScalar Y[],
  const Ordinal colStrideY,
  const RangeScalar alpha,
  const Ordinal ptr[],
  const Ordinal ind[],
  const MatrixScalar val[],
  const DomainScalar X[],
  const Ordinal colStrideX)
{
  typedef Teuchos::ScalarTraits<RangeScalar> STS;

  // Prescale: Y := beta * Y.
  if (beta == STS::zero()) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      RangeScalar* const Y_j = &Y[j*colStrideY];
      for (Ordinal i = 0; i < numRows; ++i) {
        // Follow the Sparse BLAS convention for beta == 0. 
        Y_j[i] = STS::zero();
      }
    }
  }
  else if (beta != STS::one()) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      RangeScalar* const Y_j = &Y[j*colStrideY];
      for (Ordinal i = 0; i < numRows; ++i) {
        Y_j[i] = beta * Y_j[i];
      }
    }
  }
  Ordinal j = 0;
  if (alpha == STS::zero()) {
    return; // Our work is done!
  }
  const Ordinal nnz = ptr[numCols];
  if (alpha == STS::one()) {
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = val[k];
      const Ordinal i = ind[k];
      while (k >= ptr[j+1]) {
        ++j;
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i + c*colStrideY] = Y[i + c*colStrideY] + A_ij * X[j + c*colStrideX];
      }
    }
  }
  else { // alpha != STS::one()
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = val[k];
      const Ordinal i = ind[k];
      while (k >= ptr[j+1]) {
        ++j;
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i + c*colStrideY] = Y[i + c*colStrideY] + alpha * A_ij * X[j + c*colStrideX];
      }
    }
  }
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
matVecCsrColMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  const RangeScalar& beta,
  RangeScalar Y[],
  const Ordinal colStrideY,
  const RangeScalar alpha,
  const Ordinal ptr[],
  const Ordinal ind[],
  const MatrixScalar val[],
  const DomainScalar X[],
  const Ordinal colStrideX)
{
  typedef Teuchos::ScalarTraits<RangeScalar> STS;

  Ordinal i = 0;
  // Special case for CSR only: Y(0,:) = 0.
  if (beta != STS::zero()) {
    for (Ordinal c = 0; c < numVecs; ++c) {
      Y[c*colStrideY] = beta * Y[c*colStrideY];
    }
  }
  else {
    // Follow the Sparse BLAS convention for beta == 0. 
    for (Ordinal c = 0; c < numVecs; ++c) {
      Y[c*colStrideY] = STS::zero();
    }
  }
  if (alpha == STS::zero()) {
    return; // Our work is done!
  }
  const Ordinal nnz = ptr[numRows];
  if (alpha == STS::one()) {
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = val[k];
      const Ordinal j = ind[k];
      while (k >= ptr[i+1]) {
        ++i;
        // We haven't seen row i before; prescale Y(i,:).
        RangeScalar* const Y_i = &Y[i];
        for (Ordinal c = 0; c < numVecs; ++c) {
          Y_i[c*colStrideY] = beta * Y_i[c*colStrideY];
        }
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i + c*colStrideY] = Y[i + c*colStrideY] + A_ij * X[j + c*colStrideX];
      }
    }
  }
  else { // alpha != STS::one()
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = val[k];
      const Ordinal j = ind[k];
      while (k >= ptr[i+1]) {
        ++i;
        // We haven't seen row i before; prescale Y(i,:).
        RangeScalar* const Y_i = &Y[i];
        for (Ordinal c = 0; c < numVecs; ++c) {
          Y_i[c*colStrideY] = beta * Y_i[c*colStrideY];
        }
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i + c*colStrideY] = Y[i + c*colStrideY] + alpha * A_ij * X[j + c*colStrideX];
      }
    }
  }
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
matVecCscRowMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  const RangeScalar& beta,
  RangeScalar Y[],
  const Ordinal rowStrideY,
  const RangeScalar alpha,
  const Ordinal ptr[],
  const Ordinal ind[],
  const MatrixScalar val[],
  const DomainScalar X[],
  const Ordinal rowStrideX)
{
  typedef Teuchos::ScalarTraits<RangeScalar> STS;

  // Prescale: Y := beta * Y.
  if (beta == STS::zero()) {
    for (Ordinal i = 0; i < numRows; ++i) {
      RangeScalar* const Y_i = &Y[i*rowStrideY];
      for (Ordinal j = 0; j < numVecs; ++j) {
        // Follow the Sparse BLAS convention for beta == 0. 
        Y_i[j] = STS::zero();
      }
    }
  }
  else if (beta != STS::one()) {
    for (Ordinal i = 0; i < numRows; ++i) {
      RangeScalar* const Y_i = &Y[i*rowStrideY];
      for (Ordinal j = 0; j < numVecs; ++j) {
        Y_i[j] = beta * Y_i[j];
      }
    }
  }
  Ordinal j = 0;
  if (alpha == STS::zero()) {
    return; // Our work is done!
  }
  const Ordinal nnz = ptr[numCols];
  if (alpha == STS::one()) {
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = val[k];
      const Ordinal i = ind[k];
      while (k >= ptr[j+1]) {
        ++j;
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i*rowStrideY + c] = Y[i*rowStrideY + c] + A_ij * X[j*rowStrideX + c];
      }
    }
  }
  else { // alpha != STS::one()
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = val[k];
      const Ordinal i = ind[k];
      while (k >= ptr[j+1]) {
        ++j;
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i*rowStrideY + c] = Y[i*rowStrideY + c] + alpha * A_ij * X[j*rowStrideX + c];
      }
    }
  }
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
matVecCsrRowMajor (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  const RangeScalar& beta,
  RangeScalar Y[],
  const Ordinal rowStrideY,
  const RangeScalar alpha,
  const Ordinal ptr[],
  const Ordinal ind[],
  const MatrixScalar val[],
  const DomainScalar X[],
  const Ordinal rowStrideX)
{
  typedef Teuchos::ScalarTraits<RangeScalar> STS;

  Ordinal i = 0;
  // Special case for CSR only: Y(0,:) = 0.
  if (beta != STS::zero()) {
    for (Ordinal c = 0; c < numVecs; ++c) {
      Y[c] = beta * Y[c];
    }
  }
  else {
    // Follow the Sparse BLAS convention for beta == 0. 
    for (Ordinal c = 0; c < numVecs; ++c) {
      Y[c] = STS::zero();
    }
  }
  if (alpha == STS::zero()) {
    return; // Our work is done!
  }
  const Ordinal nnz = ptr[numRows];
  if (alpha == STS::one()) {
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = val[k];
      const Ordinal j = ind[k];
      while (k >= ptr[i+1]) {
        ++i;
        // We haven't seen row i before; prescale Y(i,:).
        RangeScalar* const Y_i = &Y[i*rowStrideY];
        for (Ordinal c = 0; c < numVecs; ++c) {
          Y_i[c*rowStrideY] = beta * Y_i[c*rowStrideY];
        }
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i*rowStrideY + c] = Y[i*rowStrideY + c] + A_ij * X[j*rowStrideX + c];
      }
    }
  }
  else { // alpha != STS::one()
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = val[k];
      const Ordinal j = ind[k];
      while (k >= ptr[i+1]) {
        ++i;
        // We haven't seen row i before; prescale Y(i,:).
        RangeScalar* const Y_i = &Y[i*rowStrideY];
        for (Ordinal c = 0; c < numVecs; ++c) {
          Y_i[c*rowStrideY] = beta * Y_i[c*rowStrideY];
        }
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i*rowStrideY + c] = Y[i*rowStrideY + c] + alpha * A_ij * X[j*rowStrideX + c];
      }
    }
  }
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
matVecCscColMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  const RangeScalar& beta,
  RangeScalar Y[],
  const Ordinal colStrideY,
  const RangeScalar alpha,
  const Ordinal ptr[],
  const Ordinal ind[],
  const MatrixScalar val[],
  const DomainScalar X[],
  const Ordinal colStrideX)
{
  typedef Teuchos::ScalarTraits<RangeScalar> STS;

  // Prescale: Y := beta * Y.
  if (beta == STS::zero()) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      RangeScalar* const Y_j = &Y[j*colStrideY];
      for (Ordinal i = 0; i < numRows; ++i) {
        // Follow the Sparse BLAS convention for beta == 0. 
        Y_j[i] = STS::zero();
      }
    }
  }
  else if (beta != STS::one()) {
    for (Ordinal j = 0; j < numVecs; ++j) {
      RangeScalar* const Y_j = &Y[j*colStrideY];
      for (Ordinal i = 0; i < numRows; ++i) {
        Y_j[i] = beta * Y_j[i];
      }
    }
  }
  Ordinal j = 0;
  if (alpha == STS::zero()) {
    return; // Our work is done!
  }
  const Ordinal nnz = ptr[numCols];
  if (alpha == STS::one()) {
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = Teuchos::ScalarTraits<MatrixScalar>::conjugate (val[k]);
      const Ordinal i = ind[k];
      while (k >= ptr[j+1]) {
        ++j;
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i + c*colStrideY] = Y[i + c*colStrideY] + A_ij * X[j + c*colStrideX];
      }
    }
  }
  else { // alpha != STS::one()
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = Teuchos::ScalarTraits<MatrixScalar>::conjugate (val[k]);
      const Ordinal i = ind[k];
      while (k >= ptr[j+1]) {
        ++j;
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i + c*colStrideY] = Y[i + c*colStrideY] + alpha * A_ij * X[j + c*colStrideX];
      }
    }
  }
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
matVecCsrColMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  const RangeScalar& beta,
  RangeScalar Y[],
  const Ordinal colStrideY,
  const RangeScalar alpha,
  const Ordinal ptr[],
  const Ordinal ind[],
  const MatrixScalar val[],
  const DomainScalar X[],
  const Ordinal colStrideX)
{
  typedef Teuchos::ScalarTraits<RangeScalar> STS;

  Ordinal i = 0;
  // Special case for CSR only: Y(0,:) = 0.
  if (beta != STS::zero()) {
    for (Ordinal c = 0; c < numVecs; ++c) {
      Y[c*colStrideY] = beta * Y[c*colStrideY];
    }
  }
  else {
    // Follow the Sparse BLAS convention for beta == 0. 
    for (Ordinal c = 0; c < numVecs; ++c) {
      Y[c*colStrideY] = STS::zero();
    }
  }
  if (alpha == STS::zero()) {
    return; // Our work is done!
  }
  const Ordinal nnz = ptr[numRows];
  if (alpha == STS::one()) {
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = Teuchos::ScalarTraits<MatrixScalar>::conjugate (val[k]);
      const Ordinal j = ind[k];
      while (k >= ptr[i+1]) {
        ++i;
        // We haven't seen row i before; prescale Y(i,:).
        RangeScalar* const Y_i = &Y[i];
        for (Ordinal c = 0; c < numVecs; ++c) {
          Y_i[c*colStrideY] = beta * Y_i[c*colStrideY];
        }
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i + c*colStrideY] = Y[i + c*colStrideY] + A_ij * X[j + c*colStrideX];
      }
    }
  }
  else { // alpha != STS::one()
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = Teuchos::ScalarTraits<MatrixScalar>::conjugate (val[k]);
      const Ordinal j = ind[k];
      while (k >= ptr[i+1]) {
        ++i;
        // We haven't seen row i before; prescale Y(i,:).
        RangeScalar* const Y_i = &Y[i];
        for (Ordinal c = 0; c < numVecs; ++c) {
          Y_i[c*colStrideY] = beta * Y_i[c*colStrideY];
        }
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i + c*colStrideY] = Y[i + c*colStrideY] + alpha * A_ij * X[j + c*colStrideX];
      }
    }
  }
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
matVecCscRowMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  const RangeScalar& beta,
  RangeScalar Y[],
  const Ordinal rowStrideY,
  const RangeScalar alpha,
  const Ordinal ptr[],
  const Ordinal ind[],
  const MatrixScalar val[],
  const DomainScalar X[],
  const Ordinal rowStrideX)
{
  typedef Teuchos::ScalarTraits<RangeScalar> STS;

  // Prescale: Y := beta * Y.
  if (beta == STS::zero()) {
    for (Ordinal i = 0; i < numRows; ++i) {
      RangeScalar* const Y_i = &Y[i*rowStrideY];
      for (Ordinal j = 0; j < numVecs; ++j) {
        // Follow the Sparse BLAS convention for beta == 0. 
        Y_i[j] = STS::zero();
      }
    }
  }
  else if (beta != STS::one()) {
    for (Ordinal i = 0; i < numRows; ++i) {
      RangeScalar* const Y_i = &Y[i*rowStrideY];
      for (Ordinal j = 0; j < numVecs; ++j) {
        Y_i[j] = beta * Y_i[j];
      }
    }
  }
  Ordinal j = 0;
  if (alpha == STS::zero()) {
    return; // Our work is done!
  }
  const Ordinal nnz = ptr[numCols];
  if (alpha == STS::one()) {
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = Teuchos::ScalarTraits<MatrixScalar>::conjugate (val[k]);
      const Ordinal i = ind[k];
      while (k >= ptr[j+1]) {
        ++j;
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i*rowStrideY + c] = Y[i*rowStrideY + c] + A_ij * X[j*rowStrideX + c];
      }
    }
  }
  else { // alpha != STS::one()
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = Teuchos::ScalarTraits<MatrixScalar>::conjugate (val[k]);
      const Ordinal i = ind[k];
      while (k >= ptr[j+1]) {
        ++j;
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i*rowStrideY + c] = Y[i*rowStrideY + c] + alpha * A_ij * X[j*rowStrideX + c];
      }
    }
  }
}

template<class Ordinal,
         class MatrixScalar,
         class DomainScalar,
         class RangeScalar>
void
matVecCsrRowMajorConj (
  const Ordinal numRows,
  const Ordinal numCols,
  const Ordinal numVecs,
  const RangeScalar& beta,
  RangeScalar Y[],
  const Ordinal rowStrideY,
  const RangeScalar alpha,
  const Ordinal ptr[],
  const Ordinal ind[],
  const MatrixScalar val[],
  const DomainScalar X[],
  const Ordinal rowStrideX)
{
  typedef Teuchos::ScalarTraits<RangeScalar> STS;

  Ordinal i = 0;
  // Special case for CSR only: Y(0,:) = 0.
  if (beta != STS::zero()) {
    for (Ordinal c = 0; c < numVecs; ++c) {
      Y[c] = beta * Y[c];
    }
  }
  else {
    // Follow the Sparse BLAS convention for beta == 0. 
    for (Ordinal c = 0; c < numVecs; ++c) {
      Y[c] = STS::zero();
    }
  }
  if (alpha == STS::zero()) {
    return; // Our work is done!
  }
  const Ordinal nnz = ptr[numRows];
  if (alpha == STS::one()) {
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = Teuchos::ScalarTraits<MatrixScalar>::conjugate (val[k]);
      const Ordinal j = ind[k];
      while (k >= ptr[i+1]) {
        ++i;
        // We haven't seen row i before; prescale Y(i,:).
        RangeScalar* const Y_i = &Y[i*rowStrideY];
        for (Ordinal c = 0; c < numVecs; ++c) {
          Y_i[c*rowStrideY] = beta * Y_i[c*rowStrideY];
        }
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i*rowStrideY + c] = Y[i*rowStrideY + c] + A_ij * X[j*rowStrideX + c];
      }
    }
  }
  else { // alpha != STS::one()
    for (Ordinal k = 0; k < nnz; ++k) {
      const MatrixScalar A_ij = Teuchos::ScalarTraits<MatrixScalar>::conjugate (val[k]);
      const Ordinal j = ind[k];
      while (k >= ptr[i+1]) {
        ++i;
        // We haven't seen row i before; prescale Y(i,:).
        RangeScalar* const Y_i = &Y[i*rowStrideY];
        for (Ordinal c = 0; c < numVecs; ++c) {
          Y_i[c*rowStrideY] = beta * Y_i[c*rowStrideY];
        }
      }
      for (Ordinal c = 0; c < numVecs; ++c) {
        Y[i*rowStrideY + c] = Y[i*rowStrideY + c] + alpha * A_ij * X[j*rowStrideX + c];
      }
    }
  }
}

} // namespace Raw
} // namespace Kokkos

#endif // #ifndef __Kokkos_Raw_SparseMatVec_def_hpp
