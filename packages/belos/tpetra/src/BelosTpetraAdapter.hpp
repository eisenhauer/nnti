//@HEADER
// ************************************************************************
//
//                 Belos: Block Linear Solvers Package
//                  Copyright 2004 Sandia Corporation
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

#ifndef BELOS_TPETRA_ADAPTER_HPP
#define BELOS_TPETRA_ADAPTER_HPP

#include <Kokkos_NodeTrace.hpp>

/*! \file BelosTpetraAdapter.hpp
    \brief Provides several interfaces between Belos virtual classes and Tpetra concrete classes.
*/

// TODO: the assumption is made that the solver, multivector and operator are templated on the same scalar. this will need to be modified.

#include <Tpetra_MultiVector.hpp>
#include <Tpetra_Operator.hpp>
#include <Teuchos_TestForException.hpp>
#include <Teuchos_ScalarTraits.hpp>
#include <Teuchos_Array.hpp>
#include <Teuchos_DefaultSerialComm.hpp>

#include "BelosConfigDefs.hpp"
#include "BelosTypes.hpp"
#include "BelosMultiVecTraits.hpp"
#include "BelosOperatorTraits.hpp"
#include <Kokkos_NodeAPIConfigDefs.hpp>

#ifdef HAVE_BELOS_TSQR
#  include <Tpetra_TsqrAdaptor.hpp>
#endif // HAVE_BELOS_TSQR


namespace Belos {

  ////////////////////////////////////////////////////////////////////
  //
  // Implementation of the Belos::MultiVecTraits for Tpetra::MultiVector.
  //
  ////////////////////////////////////////////////////////////////////

  /*!  \brief Template specialization of Belos::MultiVecTraits class using the Tpetra::MultiVector class.

    This interface will ensure that any Tpetra::MultiVector will be accepted by the Belos
    templated solvers.  */
  template<class Scalar, class LO, class GO, class Node>
  class MultiVecTraits<Scalar, Tpetra::MultiVector<Scalar,LO,GO,Node> >
  {
  public:
#ifdef HAVE_BELOS_TPETRA_TIMERS
    static Teuchos::RCP<Teuchos::Time> mvTimesMatAddMvTimer_, mvTransMvTimer_;
#endif

    static Teuchos::RCP<Tpetra::MultiVector<Scalar,LO,GO,Node> > Clone( const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, const int numvecs )
    { 
      return Teuchos::rcp( new Tpetra::MultiVector<Scalar,LO,GO,Node>(mv.getMap(),numvecs));
    }

    static Teuchos::RCP<Tpetra::MultiVector<Scalar,LO,GO,Node> > CloneCopy( const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv )
    {
      KOKKOS_NODE_TRACE("Belos::MVT::CloneCopy(MV)")
      return Teuchos::rcp( new Tpetra::MultiVector<Scalar,LO,GO,Node>( mv ) ); 
    }

    static Teuchos::RCP<Tpetra::MultiVector<Scalar,LO,GO,Node> > CloneCopy( const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, const std::vector<int>& index )
    { 
      KOKKOS_NODE_TRACE("Belos::MVT::CloneCopy(MV,ind)")
      TEST_FOR_EXCEPTION(index.size() == 0,std::runtime_error,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::CloneCopy(mv,index): numvecs must be greater than zero.");
#ifdef HAVE_TPETRA_DEBUG
      TEST_FOR_EXCEPTION( *std::min_element(index.begin(),index.end()) < 0, std::runtime_error,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::CloneCopy(mv,index): indices must be >= zero.");
      TEST_FOR_EXCEPTION( (size_t)*std::max_element(index.begin(),index.end()) >= mv.getNumVectors(), std::runtime_error,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::CloneCopy(mv,index): indices must be < mv.getNumVectors().");
#endif
      for (typename std::vector<int>::size_type j=1; j<index.size(); ++j) {
        if (index[j] != index[j-1]+1) {
          // not contiguous; short circuit
          Teuchos::Array<size_t> stinds(index.begin(), index.end());
          return mv.subCopy(stinds);
        }
      }
      // contiguous
      return mv.subCopy(Teuchos::Range1D(index.front(),index.back()));
    }

    static Teuchos::RCP<Tpetra::MultiVector<Scalar,LO,GO,Node> > 
    CloneCopy (const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, 
	       const Teuchos::Range1D& index)
    { 
      KOKKOS_NODE_TRACE("Belos::MVT::CloneCopy(MV,ind)")
      const bool validRange = index.size() > 0 && 
	index.lbound() >= 0 && 
	index.ubound() < mv.getNumVectors();
      if (! validRange)
	{
	  std::ostringstream os;
	  os << "Belos::MultiVecTraits<Scalar, Tpetra::MultiVector<...> >::"
	    "CloneCopy(mv,index=[" << index.lbound() << ", " << index.ubound() 
	     << "]): ";
	  TEST_FOR_EXCEPTION(index.size() == 0, std::invalid_argument,
			     os.str() << "Empty index range is not allowed.");
	  TEST_FOR_EXCEPTION(index.lbound() < 0, std::invalid_argument,
			     os.str() << "Index range includes negative "
			     "index/ices, which is not allowed.");
	  // Range1D bounds are signed; size_t is unsigned.
	  TEST_FOR_EXCEPTION((size_t) index.ubound() >= mv.getNumVectors(), 
			     std::invalid_argument, 
			     os.str() << "Index range exceeds number of vectors " 
			     << mv.getNumVectors() << " in the input multivector.");
	  TEST_FOR_EXCEPTION(true, std::logic_error, 
			     os.str() << "Should never get here!");
	}
      return mv.subCopy (index);
    }


    static Teuchos::RCP<Tpetra::MultiVector<Scalar,LO,GO,Node> > CloneViewNonConst( Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, const std::vector<int>& index )
    {
      TEST_FOR_EXCEPTION(index.size() == 0,std::invalid_argument,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::CloneView(mv,index): numvecs must be greater than zero.");
#ifdef HAVE_TPETRA_DEBUG
      TEST_FOR_EXCEPTION( *std::min_element(index.begin(),index.end()) < 0, std::invalid_argument,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::CloneView(mv,index): indices must be >= zero.");
      TEST_FOR_EXCEPTION( (size_t)*std::max_element(index.begin(),index.end()) >= mv.getNumVectors(), std::invalid_argument,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::CloneView(mv,index): indices must be < mv.getNumVectors().");
#endif
      for (typename std::vector<int>::size_type j=1; j<index.size(); ++j) {
        if (index[j] != index[j-1]+1) {
          // not contiguous; short circuit
          Teuchos::Array<size_t> stinds(index.begin(), index.end());
          return mv.subViewNonConst(stinds);
        }
      }
      // contiguous
      return mv.subViewNonConst(Teuchos::Range1D(index.front(),index.back()));
    }


    static Teuchos::RCP<Tpetra::MultiVector<Scalar,LO,GO,Node> > 
    CloneViewNonConst (Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, 
		       const Teuchos::Range1D& index)
    {
      const bool validRange = index.size() > 0 && 
	index.lbound() >= 0 && 
	index.ubound() < mv.getNumVectors();
      if (! validRange)
	{
	  std::ostringstream os;
	  os << "Belos::MultiVecTraits<Scalar, Tpetra::MultiVector<...> >::"
	    "CloneViewNonConst(mv,index=[" << index.lbound() << ", " 
	     << index.ubound() << "]): ";
	  TEST_FOR_EXCEPTION(index.size() == 0, std::invalid_argument,
			     os.str() << "Empty index range is not allowed.");
	  TEST_FOR_EXCEPTION(index.lbound() < 0, std::invalid_argument,
			     os.str() << "Index range includes negative "
			     "index/ices, which is not allowed.");
	  // Range1D bounds are signed; size_t is unsigned.
	  TEST_FOR_EXCEPTION((size_t) index.ubound() >= mv.getNumVectors(), 
			     std::invalid_argument, 
			     os.str() << "Index range exceeds number of vectors " 
			     << mv.getNumVectors() << " in the input multivector.");
	  TEST_FOR_EXCEPTION(true, std::logic_error, 
			     os.str() << "Should never get here!");
	}
      return mv.subViewNonConst (index);
    }


    static Teuchos::RCP<const Tpetra::MultiVector<Scalar,LO,GO,Node> > CloneView(const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, const std::vector<int>& index )
    {
      TEST_FOR_EXCEPTION(index.size() == 0,std::invalid_argument,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::CloneView(mv,index): numvecs must be greater than zero.");
#ifdef HAVE_TPETRA_DEBUG
      TEST_FOR_EXCEPTION( *std::min_element(index.begin(),index.end()) < 0, std::invalid_argument,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::CloneView(mv,index): indices must be >= zero.");
      TEST_FOR_EXCEPTION( (size_t)*std::max_element(index.begin(),index.end()) >= mv.getNumVectors(), std::invalid_argument,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::CloneView(mv,index): indices must be < mv.getNumVectors().");
#endif
      for (typename std::vector<int>::size_type j=1; j<index.size(); ++j) {
        if (index[j] != index[j-1]+1) {
          // not contiguous; short circuit
          Teuchos::Array<size_t> stinds(index.begin(), index.end());
          return mv.subView(stinds);
        }
      }
      // contiguous
      return mv.subView(Teuchos::Range1D(index.front(),index.back()));
    }

    static Teuchos::RCP<const Tpetra::MultiVector<Scalar,LO,GO,Node> > 
    CloneView (const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, 
	       const Teuchos::Range1D& index)
    {
      const bool validRange = index.size() > 0 && 
	index.lbound() >= 0 && 
	index.ubound() < mv.getNumVectors();
      if (! validRange)
	{
	  std::ostringstream os;
	  os << "Belos::MultiVecTraits<Scalar, Tpetra::MultiVector<...> >::"
	    "CloneView(mv,index=[" << index.lbound() << ", " 
	     << index.ubound() << "]): ";
	  TEST_FOR_EXCEPTION(index.size() == 0, std::invalid_argument,
			     os.str() << "Empty index range is not allowed.");
	  TEST_FOR_EXCEPTION(index.lbound() < 0, std::invalid_argument,
			     os.str() << "Index range includes negative "
			     "index/ices, which is not allowed.");
	  // Range1D bounds are signed; size_t is unsigned.
	  TEST_FOR_EXCEPTION((size_t) index.ubound() >= mv.getNumVectors(), 
			     std::invalid_argument, 
			     os.str() << "Index range exceeds number of vectors " 
			     << mv.getNumVectors() << " in the input multivector.");
	  TEST_FOR_EXCEPTION(true, std::logic_error, 
			     os.str() << "Should never get here!");
	}
      return mv.subView (index);
    }

    static int GetVecLength( const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv )
    { return mv.getGlobalLength(); }

    static int GetNumberVecs( const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv )
    { return mv.getNumVectors(); }

    static bool HasConstantStride( const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv )
    { return mv.isConstantStride(); }

    static void MvTimesMatAddMv( Scalar alpha, const Tpetra::MultiVector<Scalar,LO,GO,Node>& A, 
                                 const Teuchos::SerialDenseMatrix<int,Scalar>& B, 
                                 Scalar beta, Tpetra::MultiVector<Scalar,LO,GO,Node>& mv )
    {
      KOKKOS_NODE_TRACE("Belos::MVT::MvTimesMatAddMv()")
#ifdef HAVE_BELOS_TPETRA_TIMERS
      Teuchos::TimeMonitor lcltimer(*mvTimesMatAddMvTimer_);
#endif
      // create local map
      Teuchos::SerialComm<int> scomm;
      Tpetra::Map<LO,GO,Node> LocalMap(B.numRows(), 0, Teuchos::rcpFromRef< const Teuchos::Comm<int> >(scomm), Tpetra::LocallyReplicated, A.getMap()->getNode());
      // encapsulate Teuchos::SerialDenseMatrix data in ArrayView
      Teuchos::ArrayView<const Scalar> Bvalues(B.values(),B.stride()*B.numCols());
      // create locally replicated MultiVector with a copy of this data
      Tpetra::MultiVector<Scalar,LO,GO,Node> B_mv(Teuchos::rcpFromRef(LocalMap),Bvalues,B.stride(),B.numCols());
      // multiply
      mv.multiply(Teuchos::NO_TRANS, Teuchos::NO_TRANS, alpha, A, B_mv, beta);
    }

    static void MvAddMv( Scalar alpha, const Tpetra::MultiVector<Scalar,LO,GO,Node>& A, Scalar beta, const Tpetra::MultiVector<Scalar,LO,GO,Node>& B, Tpetra::MultiVector<Scalar,LO,GO,Node>& mv )
    {
      mv.update(alpha,A,beta,B,Teuchos::ScalarTraits<Scalar>::zero());
    }

    static void MvScale ( Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, Scalar alpha )
    { mv.scale(alpha); }

    static void MvScale ( Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, const std::vector<Scalar>& alphas )
    { mv.scale(alphas); }

    static void MvTransMv( Scalar alpha, const Tpetra::MultiVector<Scalar,LO,GO,Node>& A, const Tpetra::MultiVector<Scalar,LO,GO,Node>& B, Teuchos::SerialDenseMatrix<int,Scalar>& C)
    { 
      KOKKOS_NODE_TRACE("Belos::MVT::MvTransMv()")
#ifdef HAVE_BELOS_TPETRA_TIMERS
      Teuchos::TimeMonitor lcltimer(*mvTransMvTimer_);
#endif
      // form alpha * A^H * B, then copy into SDM
      // we will create a multivector C_mv from a a local map
      // this map has a serial comm, the purpose being to short-circuit the MultiVector::reduce() call at the end of MultiVector::multiply()
      // otherwise, the reduced multivector data would be copied back to the GPU, only to turn around and have to get it back here.
      // this saves us a round trip for this data.
      const int numRowsC = C.numRows(),
                numColsC = C.numCols(),
                strideC  = C.stride();
      Teuchos::SerialComm<int> scomm;
      // create local map with serial comm
      Tpetra::Map<LO,GO,Node> LocalMap(numRowsC, 0, Teuchos::rcpFromRef< const Teuchos::Comm<int> >(scomm), Tpetra::LocallyReplicated, A.getMap()->getNode());
      // create local multivector to hold the result
      const bool INIT_TO_ZERO = true;
      Tpetra::MultiVector<Scalar,LO,GO,Node> C_mv(Teuchos::rcpFromRef(LocalMap),numColsC, INIT_TO_ZERO);
      // multiply result into local multivector
      C_mv.multiply(Teuchos::CONJ_TRANS,Teuchos::NO_TRANS,alpha,A,B,Teuchos::ScalarTraits<Scalar>::zero());
      // get comm
      Teuchos::RCP< const Teuchos::Comm<int> > pcomm = A.getMap()->getComm();
      // create arrayview encapsulating the Teuchos::SerialDenseMatrix
      Teuchos::ArrayView<Scalar> C_view(C.values(),strideC*numColsC);
      if (pcomm->getSize() == 1) {
        // no accumulation to do; simply extract the multivector data into C
        // extract a copy of the result into the array view (and therefore, the SerialDenseMatrix)
        C_mv.get1dCopy(C_view,strideC);
      }  
      else {
        // get a const host view of the data in C_mv
        Teuchos::ArrayRCP<const Scalar> C_mv_view = C_mv.get1dView();
        if (strideC == numRowsC) {
          // sumall into C
          Teuchos::reduceAll<int,Scalar>(*pcomm,Teuchos::REDUCE_SUM,numColsC*numRowsC,C_mv_view.getRawPtr(),C_view.getRawPtr());
        }
        else {
          // sumall into temp, copy into C
          Teuchos::Array<Scalar> destBuff(numColsC*numRowsC);
          Teuchos::reduceAll<int,Scalar>(*pcomm,Teuchos::REDUCE_SUM,numColsC*numRowsC,C_mv_view.getRawPtr(),destBuff.getRawPtr());
          for (int j=0; j < numColsC; ++j) {
            for (int i=0; i < numRowsC; ++i) {
              C_view[strideC*j+i] = destBuff[numRowsC*j+i];
            }
          }
        }
      }
    }

    static void MvDot( const Tpetra::MultiVector<Scalar,LO,GO,Node>& A, const Tpetra::MultiVector<Scalar,LO,GO,Node>& B, std::vector<Scalar> &dots)
    {
      TEST_FOR_EXCEPTION(A.getNumVectors() != B.getNumVectors(),std::invalid_argument,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::MvDot(A,B,dots): A and B must have the same number of vectors.");
#ifdef HAVE_TPETRA_DEBUG
      TEST_FOR_EXCEPTION(dots.size() < (typename std::vector<int>::size_type)A.getNumVectors(),std::invalid_argument,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::MvDot(A,B,dots): dots must have room for all dot products.");
#endif
      Teuchos::ArrayView<Scalar> av(dots);
      A.dot(B,av(0,A.getNumVectors()));
    }

    static void MvNorm(const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, std::vector<typename Teuchos::ScalarTraits<Scalar>::magnitudeType> &normvec, NormType type=TwoNorm)
    { 
#ifdef HAVE_TPETRA_DEBUG
      TEST_FOR_EXCEPTION(normvec.size() < (typename std::vector<int>::size_type)mv.getNumVectors(),std::invalid_argument,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::MvNorm(mv,normvec): normvec must have room for all norms.");
#endif
      Teuchos::ArrayView<typename Teuchos::ScalarTraits<Scalar>::magnitudeType> av(normvec);
      switch (type) {
        case OneNorm:
          mv.norm1(av(0,mv.getNumVectors()));
          break;
        case TwoNorm:
          mv.norm2(av(0,mv.getNumVectors()));
          break;
        case InfNorm:
          mv.normInf(av(0,mv.getNumVectors()));
          break;
      }
    }

    static void SetBlock( const Tpetra::MultiVector<Scalar,LO,GO,Node>& A, const std::vector<int>& index, Tpetra::MultiVector<Scalar,LO,GO,Node>& mv )
    {
      KOKKOS_NODE_TRACE("Belos::MVT::SetBlock()")
#ifdef HAVE_TPETRA_DEBUG
      TEST_FOR_EXCEPTION((typename std::vector<int>::size_type)A.getNumVectors() < index.size(),std::invalid_argument,
          "Belos::MultiVecTraits<Scalar,Tpetra::MultiVector>::SetBlock(A,index,mv): index must be the same size as A.");
#endif
      Teuchos::RCP<Tpetra::MultiVector<Scalar,LO,GO,Node> > mvsub = CloneViewNonConst(mv,index);
      if ((typename std::vector<int>::size_type)A.getNumVectors() > index.size()) {
        Teuchos::RCP<const Tpetra::MultiVector<Scalar,LO,GO,Node> > Asub = A.subView(Teuchos::Range1D(0,index.size()-1));
        (*mvsub) = (*Asub);
      }
      else {
        (*mvsub) = A;
      }
      mvsub = Teuchos::null;
    }

    static void
    SetBlock (const Tpetra::MultiVector<Scalar,LO,GO,Node>& A, 
	      const Teuchos::Range1D& index, 
	      Tpetra::MultiVector<Scalar,LO,GO,Node>& mv)
    {
      KOKKOS_NODE_TRACE("Belos::MVT::SetBlock()")

      // Range1D bounds are signed; size_t is unsigned.
      // Assignment of Tpetra::MultiVector is a deep copy.

      // Tpetra::MultiVector::getNumVectors() returns size_t.  It's
      // fair to assume that the number of vectors won't overflow int,
      // since the typical use case of multivectors involves few
      // columns, but it's friendly to check just in case.
      const size_t maxInt = static_cast<size_t> (Teuchos::OrdinalTraits<int>::max());
      const bool overflow = maxInt < A.getNumVectors() && maxInt < mv.getNumVectors();
      if (overflow)
	{
	  std::ostringstream os;
	  os <<	"Belos::MultiVecTraits<Scalar, Tpetra::MultiVector<Scalar, ..."
	    "> >::SetBlock(A, index=[" << index.lbound() << ", " 
	     << index.ubound() << "], mv): ";
	  TEST_FOR_EXCEPTION(maxInt < A.getNumVectors(), std::range_error,
			     os.str() << "Number of columns in the input multi"
			     "vector 'A' (a size_t) overflows int.");
	  TEST_FOR_EXCEPTION(maxInt < mv.getNumVectors(), std::range_error,
			     os.str() << "Number of columns in the output multi"
			     "vector 'mv' (a size_t) overflows int.");
	}
      // We've already validated the static casts above.
      const int numColsA = static_cast<int> (A.getNumVectors());
      const int numColsMv = static_cast<int> (mv.getNumVectors());
      // 'index' indexes into mv; it's the index set of the target.
      const bool validIndex = index.lbound() >= 0 && index.ubound() < numColsMv;
      // We can't take more columns out of A than A has.
      const bool validSource = index.size() <= numColsA;

      if (! validIndex || ! validSource)
	{
	  std::ostringstream os;
	  os <<	"Belos::MultiVecTraits<Scalar, Tpetra::MultiVector<Scalar, ..."
	    "> >::SetBlock(A, index=[" << index.lbound() << ", " 
	     << index.ubound() << "], mv): ";
	  TEST_FOR_EXCEPTION(index.lbound() < 0, std::invalid_argument,
			     os.str() << "Range lower bound must be nonnegative.");
	  TEST_FOR_EXCEPTION(index.ubound() >= numColsMv, std::invalid_argument,
			     os.str() << "Range upper bound must be less than "
			     "the number of columns " << numColsA << " in the "
			     "'mv' output argument.");
	  TEST_FOR_EXCEPTION(index.size() > numColsA, std::invalid_argument,
			     os.str() << "Range must have no more elements than"
			     " the number of columns " << numColsA << " in the "
			     "'A' input argument.");
	  TEST_FOR_EXCEPTION(true, std::logic_error, "Should never get here!");
	}
      typedef Teuchos::RCP<Tpetra::MultiVector<Scalar,LO,GO,Node> > MV_ptr;
      typedef Teuchos::RCP<const Tpetra::MultiVector<Scalar,LO,GO,Node> > const_MV_ptr;

      // View of the relevant column(s) of the target multivector mv.
      // We avoid view creation overhead by only creating a view if
      // the index range is different than [0, (# columns in mv) - 1].
      MV_ptr mv_view;
      if (index.lbound() == 0 && index.ubound()+1 == numColsMv)
	mv_view = Teuchos::rcpFromRef (mv); // Non-const, non-owning RCP
      else
	mv_view = CloneViewNonConst (mv, index);

      // View of the relevant column(s) of the source multivector A.
      // If A has fewer columns than mv_view, then create a view of
      // the first index.size() columns of A.
      const_MV_ptr A_view;
      if (index.size() == numColsA)
	A_view = Teuchos::rcpFromRef (A); // Const, non-owning RCP
      else
	A_view = CloneView (A, Teuchos::Range1D(0, index.size()-1));

      // Assignment of Tpetra::MultiVector objects via operator=()
      // assumes that both arguments have compatible Maps.  If
      // HAVE_TPETRA_DEBUG is defined at compile time, operator=()
      // will throw an std::runtime_error if the Maps are
      // incompatible.
      *mv_view = *A_view; 
    }

    static void MvRandom( Tpetra::MultiVector<Scalar,LO,GO,Node>& mv )
    { 
      KOKKOS_NODE_TRACE("Belos::MVT::randomize()")
      mv.randomize(); 
    }

    static void MvInit( Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, Scalar alpha = Teuchos::ScalarTraits<Scalar>::zero() )
    { mv.putScalar(alpha); }

    static void MvPrint( const Tpetra::MultiVector<Scalar,LO,GO,Node>& mv, std::ostream& os )
    { mv.print(os); }

#ifdef HAVE_BELOS_TSQR
    /// \typedef tsqr_adaptor_type
    /// \brief TsqrAdaptor specialization for Tpetra::MultiVector
    ///
    typedef Tpetra::TsqrAdaptor< Tpetra::MultiVector< Scalar, LO, GO, Node > > tsqr_adaptor_type;
#endif // HAVE_BELOS_TSQR
  };        

  ////////////////////////////////////////////////////////////////////
  //
  // Implementation of the Belos::OperatorTraits for Tpetra::Operator.
  //
  ////////////////////////////////////////////////////////////////////

  template <class Scalar, class LO, class GO, class Node> 
  class OperatorTraits < Scalar, Tpetra::MultiVector<Scalar,LO,GO,Node>, Tpetra::Operator<Scalar,LO,GO,Node> >
  {
  public:
    static void Apply ( const Tpetra::Operator<Scalar,LO,GO,Node> & Op, 
                        const Tpetra::MultiVector<Scalar,LO,GO,Node> & X,
                              Tpetra::MultiVector<Scalar,LO,GO,Node> & Y,
                              ETrans trans=NOTRANS )
    { 
      switch (trans) {
        case NOTRANS:
          Op.apply(X,Y,Teuchos::NO_TRANS);
          break;
        case TRANS:
          Op.apply(X,Y,Teuchos::TRANS);
          break;
        case CONJTRANS:
          Op.apply(X,Y,Teuchos::CONJ_TRANS);
          break;
      }
    }
  };

} // end of Belos namespace 

#endif 
// end of file BELOS_TPETRA_ADAPTER_HPP
