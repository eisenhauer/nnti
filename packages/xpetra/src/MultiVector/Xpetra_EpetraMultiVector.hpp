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
#ifndef XPETRA_EPETRAMULTIVECTOR_HPP
#define XPETRA_EPETRAMULTIVECTOR_HPP

/* this file is automatically generated - do not edit (see script/epetra.py) */

#include "Xpetra_EpetraConfigDefs.hpp"

#include "Xpetra_MultiVector.hpp"
#include "Xpetra_Vector.hpp"

#include "Xpetra_EpetraMap.hpp"
#include "Xpetra_EpetraExport.hpp"
#include "Xpetra_Utils.hpp"
#include "Xpetra_EpetraUtils.hpp"

#include <Epetra_MultiVector.h>

namespace Xpetra {

  // TODO: move that elsewhere
  const Epetra_MultiVector & toEpetra(const MultiVector<double,int,int> &);

  Epetra_MultiVector & toEpetra(MultiVector<double, int,int> &);
  //

  // #ifndef DOXYGEN_SHOULD_SKIP_THIS
  //   // forward declaration of EpetraVector, needed to prevent circular inclusions
  //   template<class S, class LO, class GO, class N> class EpetraVector;
  // #endif

  RCP<MultiVector<double, int, int> > toXpetra(RCP<Epetra_MultiVector> vec);

  //  RCP<const MultiVector<double,int, int > > toXpetra(RCP<const Epetra_MultiVector> > vec);



  class EpetraMultiVector
    : public virtual MultiVector<double, int, int>
  {

    typedef double Scalar;
    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::DefaultNode::DefaultNodeType Node;

  public:

    //! @name Constructor/Destructor Methods
    //@{

    //! Basic MultiVector constuctor.
    EpetraMultiVector(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &map, size_t NumVectors, bool zeroOut=true)
      : vec_(Teuchos::rcp(new Epetra_MultiVector(toEpetra(map), NumVectors, zeroOut))) { }

    //! MultiVector copy constructor.
    EpetraMultiVector(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &source)
      : vec_(Teuchos::rcp(new Epetra_MultiVector(toEpetra(source)))) { }

    //! Set multi-vector values from array of pointers using Teuchos memory management classes. (copy).
    EpetraMultiVector(const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > &map, const Teuchos::ArrayView< const Teuchos::ArrayView< const Scalar > > &ArrayOfPtrs, size_t NumVectors);
    
    //! Build from an Epetra_MultiVector
    EpetraMultiVector(RCP<Epetra_MultiVector> &vec):vec_(vec){ }

    //! MultiVector destructor.
    virtual ~EpetraMultiVector() { }

    //@}

    //! @name Post-construction modification routines
    //@{

    //! Replace value, using global (row) index.
    void replaceGlobalValue(GlobalOrdinal globalRow, size_t vectorIndex, const Scalar &value) { XPETRA_MONITOR("EpetraMultiVector::replaceGlobalValue"); vec_->ReplaceGlobalValue(globalRow, vectorIndex, value); }

    //! Add value to existing value, using global (row) index.
    void sumIntoGlobalValue(GlobalOrdinal globalRow, size_t vectorIndex, const Scalar &value) { XPETRA_MONITOR("EpetraMultiVector::sumIntoGlobalValue"); vec_->SumIntoGlobalValue(globalRow, vectorIndex, value); }

    //! Replace value, using local (row) index.
    void replaceLocalValue(LocalOrdinal myRow, size_t vectorIndex, const Scalar &value) { XPETRA_MONITOR("EpetraMultiVector::replaceLocalValue"); vec_->ReplaceMyValue(myRow, vectorIndex, value); }

    //! Add value to existing value, using local (row) index.
    void sumIntoLocalValue(LocalOrdinal myRow, size_t vectorIndex, const Scalar &value) { XPETRA_MONITOR("EpetraMultiVector::sumIntoLocalValue"); vec_->SumIntoMyValue(myRow, vectorIndex, value); }

    //! Set all values in the multivector with the given value.
    void putScalar(const Scalar &value) { XPETRA_MONITOR("EpetraMultiVector::putScalar"); vec_->PutScalar(value); }

    //@}

    //! @name Data Copy and View get methods
    //@{

    //! Return a Vector which is a const view of column j.
    Teuchos::RCP< const Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > > getVector(size_t j) const;

    //! Return a Vector which is a nonconst view of column j.
    Teuchos::RCP< Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > > getVectorNonConst(size_t j);

    //! Const view of the local values in a particular vector of this multivector.
    Teuchos::ArrayRCP< const Scalar > getData(size_t j) const;

    //! View of the local values in a particular vector of this multivector.
    Teuchos::ArrayRCP< Scalar > getDataNonConst(size_t j);

    //@}

    //! @name Mathematical methods
    //@{

    //! Compute dot product of each corresponding pair of vectors, dots[i] = this[i].dot(A[i]).
    void dot(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A, const Teuchos::ArrayView< Scalar > &dots) const;

    //! Put element-wise absolute values of input Multi-vector in target: A = abs(this).
    void abs(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A) { XPETRA_MONITOR("EpetraMultiVector::abs"); vec_->Abs(toEpetra(A)); }

    //! Put element-wise reciprocal values of input Multi-vector in target, this(i,j) = 1/A(i,j).
    void reciprocal(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A) { XPETRA_MONITOR("EpetraMultiVector::reciprocal"); vec_->Reciprocal(toEpetra(A)); }

    //! Scale the current values of a multi-vector, this = alpha*this.
    void scale(const Scalar &alpha) { XPETRA_MONITOR("EpetraMultiVector::scale"); vec_->Scale(alpha); }

    //! Update multi-vector values with scaled values of A, this = beta*this + alpha*A.
    void update(const Scalar &alpha, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A, const Scalar &beta) { XPETRA_MONITOR("EpetraMultiVector::update"); vec_->Update(alpha, toEpetra(A), beta); }

    //! Update multi-vector with scaled values of A and B, this = gamma*this + alpha*A + beta*B.
    void update(const Scalar &alpha, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A, const Scalar &beta, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &B, const Scalar &gamma) { XPETRA_MONITOR("EpetraMultiVector::update"); vec_->Update(alpha, toEpetra(A), beta, toEpetra(B), gamma); }

    //! Compute 1-norm of each vector in multi-vector.
    void norm1(const Teuchos::ArrayView< Teuchos::ScalarTraits< Scalar >::magnitudeType > &norms) const;

    //!
    void norm2(const Teuchos::ArrayView< Teuchos::ScalarTraits< Scalar >::magnitudeType > &norms) const;

    //! Compute Inf-norm of each vector in multi-vector.
    void normInf(const Teuchos::ArrayView< Teuchos::ScalarTraits< Scalar >::magnitudeType > &norms) const;

    //!
    void normWeighted(const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &weights, const Teuchos::ArrayView< Teuchos::ScalarTraits< Scalar >::magnitudeType > &norms) const;

    //! Compute mean (average) value of each vector in multi-vector. The outcome of this routine is undefined for non-floating point scalar types (e.g., int).
    void meanValue(const Teuchos::ArrayView< Scalar > &means) const;

    //! Matrix-matrix multiplication: this = beta*this + alpha*op(A)*op(B).
    void multiply(Teuchos::ETransp transA, Teuchos::ETransp transB, const Scalar &alpha, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &B, const Scalar &beta) { XPETRA_MONITOR("EpetraMultiVector::multiply"); vec_->Multiply(toEpetra(transA), toEpetra(transB), alpha, toEpetra(A), toEpetra(B), beta); }

    //! Element-wise multiply of a Vector A with a MultiVector B.
    void elementWiseMultiply(Scalar scalarAB, const Vector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &A, const MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node > &B, Scalar scalarThis) { XPETRA_MONITOR("EpetraMultiVector::elementWiseMultiply"); vec_->Multiply(scalarAB, toEpetra(A), toEpetra(B), scalarThis); }

    //@}

    //! @name Attribute access functions
    //@{

    //! Number of columns in the multivector.
    size_t getNumVectors() const { XPETRA_MONITOR("EpetraMultiVector::getNumVectors"); return vec_->NumVectors(); }

    //! Local number of rows on the calling process.
    size_t getLocalLength() const { XPETRA_MONITOR("EpetraMultiVector::getLocalLength"); return vec_->MyLength(); }

    //! Global number of rows in the multivector.
    global_size_t getGlobalLength() const { XPETRA_MONITOR("EpetraMultiVector::getGlobalLength"); return vec_->GlobalLength(); }

    //@}

    //! @name Overridden from Teuchos::Describable
    //@{

    //! A simple one-line description of this object.
    std::string description() const;

    //! Print the object with the given verbosity level to a FancyOStream.
    void describe(Teuchos::FancyOStream &out, const Teuchos::EVerbosityLevel verbLevel=Teuchos::Describable::verbLevel_default) const;

    //@}

    //! Set multi-vector values to random numbers.
    void randomize(bool bUseXpetraImplementation = false) {
        XPETRA_MONITOR("EpetraMultiVector::randomize");

        if(bUseXpetraImplementation)
            Xpetra::MultiVector< Scalar, LocalOrdinal, GlobalOrdinal, Node >::Xpetra_randomize();
        else
            vec_->Random();
    }

    //! Implements DistObject interface
    //{@

    //! Access function for the Tpetra::Map this DistObject was constructed with.
    const Teuchos::RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > > getMap() const { XPETRA_MONITOR("EpetraMultiVector::getMap"); return toXpetra(vec_->Map()); }

    //! Import.
    void doImport(const DistObject<Scalar, LocalOrdinal, GlobalOrdinal, Node> &source, const Import< LocalOrdinal, GlobalOrdinal, Node > &importer, CombineMode CM);

    //! Export.
    void doExport(const DistObject<Scalar, LocalOrdinal, GlobalOrdinal, Node> &dest, const Import< LocalOrdinal, GlobalOrdinal, Node >& importer, CombineMode CM);

    //! Import (using an Exporter).
    void doImport(const DistObject<Scalar, LocalOrdinal, GlobalOrdinal, Node> &source, const Export< LocalOrdinal, GlobalOrdinal, Node >& exporter, CombineMode CM);

    //! Export (using an Importer).
    void doExport(const DistObject<Scalar, LocalOrdinal, GlobalOrdinal, Node> &dest, const Export< LocalOrdinal, GlobalOrdinal, Node >& exporter, CombineMode CM);

    //! Replace the map
    void replaceMap(const RCP<const Map<LocalOrdinal,GlobalOrdinal,Node> >& map);

    //@}

    //! @name Xpetra specific
    //@{

    //! EpetraMultiVector constructor to wrap a Epetra_MultiVector object
    EpetraMultiVector(const RCP<Epetra_MultiVector> &vec) : vec_(vec) { } //TODO removed const

    //! Get the underlying Epetra multivector
    RCP<Epetra_MultiVector> getEpetra_MultiVector() const { return vec_; }

    //! Set seed for Random function.
    void setSeed(unsigned int seed) {
      XPETRA_MONITOR("EpetraMultiVector::seedrandom");

      Teuchos::ScalarTraits< Scalar >::seedrandom(seed);
      vec_->SetSeed(seed);
    }

    //@}

  private:

    RCP< Epetra_MultiVector > vec_;

  }; // EpetraMultiVector class

} // Xpetra namespace

#endif // XPETRA_EPETRAMULTIVECTOR_HPP
