//@HEADER
// ************************************************************************
//
//                 Belos: Block Linear Solvers Package
//                  Copyright 2010 Sandia Corporation
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

/// \file BelosTsqrOrthoManagerImpl.hpp
/// \brief Orthogonalization manager back end based on TSQR
///

#ifndef __BelosTsqrOrthoManagerImpl_hpp
#define __BelosTsqrOrthoManagerImpl_hpp

#include "BelosConfigDefs.hpp" // HAVE_BELOS_TSQR
#include "BelosMultiVecTraits.hpp"
#include "BelosOrthoManager.hpp" // OrthoError, etc.

#include "Teuchos_LAPACK.hpp"
#include "Teuchos_ParameterList.hpp"
#ifdef BELOS_TEUCHOS_TIME_MONITOR
#  include "Teuchos_TimeMonitor.hpp"
#endif // BELOS_TEUCHOS_TIME_MONITOR

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace Belos {

  /// \class TsqrOrthoError
  /// \brief TsqrOrthoManager(Impl) error
  class TsqrOrthoError : public OrthoError
  {
  public: 
    TsqrOrthoError (const std::string& what_arg) : 
      OrthoError (what_arg) {}
  };

  /// \class TsqrOrthoFault
  /// \brief Orthogonalization fault
  ///
  /// Stewart (SISC 2008) presents a Block Gram-Schmidt (BGS)
  /// algorithm with careful reorthogonalization.  He defines an
  /// "orthogonalization fault" as happening when the second BGS pass
  /// does not succeed.  This is possible in BGS, but not possible in
  /// (non-block) Gram-Schmidt, if you use Stewart's randomization
  /// procedure for the latter.  Stewart gives an algorithm for
  /// recovering from an orthogonalization fault, but the algorithm is
  /// expensive: it involves careful reorthogonalization with
  /// non-block Gram-Schmidt.  If the "throwOnReorthogFault" option is
  /// set, we choose instead to report the orthogonalization fault as
  /// an exception.
  ///
  /// \note This is not a (subclass of) TsqrOrthoError, because the
  ///   latter is a logic or runtime bug, whereas a TsqrOrthoFault is
  ///   a property of the input and admits recovery.
  class TsqrOrthoFault : public OrthoError
  {
  public: 
    TsqrOrthoFault (const std::string& what_arg) : 
      OrthoError (what_arg) {}
  };

  /// \class TsqrOrthoManagerImpl
  /// \brief TSQR-based OrthoManager subclass implementation
  /// 
  /// TsqrOrthoManagerImpl implements the interface defined by
  /// OrthoManager.  It doesn't actually inherit from OrthoManager,
  /// which gives us a bit more freedom when defining the actual
  /// subclass of OrthoManager (TsqrOrthoManager).  
  ///
  /// This class uses a combination of Tall Skinny QR (TSQR) and Block
  /// Gram-Schmidt (BGS) to orthogonalize multivectors.
  ///
  /// The Block Gram-Schmidt procedure used here is inspired by that
  /// of G. W. Stewart ("Block Gram-Schmidt Orthogonalization", SISC
  /// vol 31 #1 pp. 761--775, 2008), except that we use TSQR+SVD
  /// instead of standard Gram-Schmidt with orthogonalization to
  /// handle the current block.  "Orthogonalization faults" may still
  /// happen, but we do not handle them by default.  Rather, we make
  /// one BGS pass, do TSQR+SVD, check the resulting column norms, and
  /// make a second BGS pass (+ TSQR+SVD) if necessary.  If we then
  /// detect an orthogonalization fault, we throw TsqrOrthoFault.
  ///
  /// \note Despite the "Impl" part of the name of this class, we
  ///   don't actually use it for the "pImpl" idiom.
  ///
  template< class Scalar, class MV >
  class TsqrOrthoManagerImpl
  {
  public:
    typedef Scalar scalar_type;
    typedef typename Teuchos::ScalarTraits<Scalar>::magnitudeType magnitude_type;
    typedef MV multivector_type;
    /// \typedef mat_type 
    /// Type of the projection and normalization coefficients
    typedef Teuchos::SerialDenseMatrix<int, Scalar> mat_type;
    typedef Teuchos::RCP<mat_type> mat_ptr;

  private:
    typedef Teuchos::ScalarTraits<Scalar> SCT;
    typedef Teuchos::ScalarTraits<magnitude_type> SCTM;
    typedef MultiVecTraits<Scalar, MV> MVT;
    typedef typename MVT::tsqr_adaptor_type tsqr_adaptor_type;
    typedef Teuchos::RCP<tsqr_adaptor_type> tsqr_adaptor_ptr;

  public:
    /// \brief Get default parameters for TsqrOrthoManagerImpl
    ///
    /// Get a (pointer to a) default list of parameters for
    /// configuring a TsqrOrthoManager or TsqrMatOrthoManager
    /// instance.  The same parameters work for both.
    ///
    /// \note To get nondefault behavior, a good thing to do is to
    ///   make a deep copy of the returned parameter list, and then
    ///   modify individual entries as desired.
    ///
    /// \note TSQR implementation configuration options are stored
    ///   under "TsqrImpl" as an RCP<const ParameterList>.  (Don't call
    ///   sublist() to get them; call get().)
    ///
    /// \warning This method is not reentrant.  It should only be
    ///   called by one thread at a time.
    ///
    static Teuchos::RCP<const Teuchos::ParameterList> getDefaultParameters ();

    /// \brief Get "fast" parameters for TsqrOrthoManagerImpl
    ///
    /// Get a (pointer to a) list of parameters for configuring a
    /// TsqrOrthoManager or TsqrMatOrthoManager instance for maximum
    /// speed, at the cost of accuracy (no block reorthogonalization)
    /// and robustness to rank deficiency (no randomization of the
    /// null space basis).
    ///
    /// \note TSQR implementation configuration options are stored
    ///   under "TsqrImpl" as an RCP<const ParameterList>.  (Don't call
    ///   sublist() to get them; call get().)
    ///
    /// \warning This method is not reentrant.  It should only be
    ///   called by one thread at a time.
    ///
    static Teuchos::RCP<const Teuchos::ParameterList> getFastParameters ();

    /// Constructor
    ///
    /// \param params [in] Configuration parameters, both for this
    ///   orthogonalization manager, and for TSQR itself (as an
    ///   RCP<const ParameterList> under "TsqrImpl").  Call the
    ///   getDefaultParameters() class method for default parameters
    ///   and their documentation, including TSQR implementation
    ///   parameters.
    ///
    /// \param label [in] Label for timers.  This only matters if the
    ///   compile-time option for enabling timers is set.
    ///
    TsqrOrthoManagerImpl (const Teuchos::RCP<const Teuchos::ParameterList>& params,
			  const std::string& label);

    //! Set the label for timers (if timers are enabled)
    void setLabel (const std::string& label) { label_ = label; }

    //! Get the label for timers (if timers are enabled)
    const std::string& getLabel () { return label_; }

    /// \brief Euclidean inner product
    ///
    /// Compute the Euclidean block inner product X^* Y, and store the
    /// result in Z.
    /// 
    /// \param X [in]
    /// \param Y [in]
    /// \param Z [out] On output, \f$X^* Y\f$
    void 
    innerProd (const MV& X, const MV& Y, mat_type& Z) const
    {
      MVT::MvTransMv (SCT::one(), X, Y, Z);
    }

    /// Compute the 2-norm of each column j of X
    ///
    /// \param X [in] Multivector for which to compute column norms
    /// \param normvec [out] On output: normvec[j] is the 2-norm of
    ///   column j of X.  normvec is resized if necessary so that it
    ///   has at least as many entries as there are columns of X.
    ///
    /// \note Performance of this method depends on how MultiVecTraits
    ///   implements column norm computation for the given multivector
    ///   type MV.  It may or may not be the case that a reduction is
    ///   performed for every column of X.  Furthermore, whether or
    ///   not the columns of X are contiguous (as opposed to a view of
    ///   noncontiguous columns) may also affect performance.  The
    ///   computed results should be the same regardless, except
    ///   perhaps for small rounding differences due to a different
    ///   order of operations.
    void
    norm (const MV& X, std::vector<magnitude_type>& normVec) const
    {
      const int numCols = MVT::GetNumberVecs (X);
      // std::vector<T>::size_type is unsigned; int is signed.  Mixed
      // unsigned/signed comparisons trigger compiler warnings.
      if (normVec.size() < static_cast<size_t>(numCols))
	normVec.resize (numCols); // Resize normvec if necessary.
      MVT::MvNorm (X, normVec);
    }

    /// \brief Compute \f$C := Q^* X\f$ and \f$X := X - Q C\f$.
    ///
    /// Project X against the span of the (Euclidean) orthogonal
    /// vectors Q, and store the resulting coefficients in C.
    /// 
    /// \param X [in/out] On input: the vectors to project.
    ///   On output: \f$X := X - Q C\f$ where \f$C := Q^* X\f$.
    /// \param C [out] The projection coefficients \f$C := Q^* X\f$
    /// \param Q [in] The orthogonal basis against which to project
    void 
    project (MV& X, 
	     Teuchos::Array<mat_ptr> C, 
	     Teuchos::ArrayView<Teuchos::RCP<const MV> > Q);

    /// \brief Orthogonalize the columns of X in place
    ///
    /// Orthogonalize the columns of X in place, storing the resulting
    /// coefficients in B.  Return the rank of X.  If X is full rank,
    /// then X*B on output is a QR factorization of X on input.  If X
    /// is not full rank, then the first rank columns of X on output
    /// form a basis for the column space of X (on input).  Additional
    /// options control randomization of the null space basis.
    ///
    /// \param X [in/out]
    /// \param B [out]
    ///
    /// \return Rank of X
    int normalize (MV& X, mat_ptr B);

    /// \brief Normalize X into Q*B, overwriting X
    ///
    /// Normalize X into Q*B, overwriting X with invalid values.
    ///
    /// \param X [in/out] Vector(s) to normalize
    /// \param Q [out] Normalized vector(s)
    /// \param B [out] Normalization coefficients
    ///
    /// \return Rank of X
    ///
    /// \note Q must have at least as many columns as X.  It may have
    ///   more columns than X; those columns are ignored.
    ///
    /// \note We expose this interface to applications because TSQR is
    ///   not able to compute an orthogonal basis in place; it needs
    ///   scratch space.  Applications can exploit this interface to
    ///   avoid excessive copying of vectors when using TSQR for
    ///   orthogonalization.
    int 
    normalizeOutOfPlace (MV& X, MV& Q, mat_ptr B);

    /// Equivalent (in exact arithmetic) to project(X,C,Q) followed by
    /// normalize(X,B).  However, this method performs
    /// reorthogonalization more efficiently and accurately.
    ///
    /// \param X [in/out] The vectors to project against Q and normalize
    /// \param C [out] The projection coefficients 
    /// \param B [out] The normalization coefficients
    /// \param Q [in] The orthogonal basis against which to project
    ///
    /// \return Rank of X after projection
    int 
    projectAndNormalize (MV &X,
			 Teuchos::Array<mat_ptr> C,
			 mat_ptr B,
			 Teuchos::ArrayView<Teuchos::RCP<const MV> > Q);

    /// \brief Project and normalize X_in into X_out; overwrite X_in
    ///
    /// Project X_in against Q, storing projection coefficients in C,
    /// and normalize X_in into X_out, storing normalization
    /// coefficients in B.  On output, X_out has the resulting
    /// orthogonal vectors and X_in is overwritten with invalid values.
    ///
    /// \param X_in [in/out] On input: The vectors to project against
    ///   Q and normalize.  On output: Overwritten with invalid values.
    /// \param X_out [out] The normalized input vectors after 
    ///   projection against Q.
    /// \param C [out] Projection coefficients 
    /// \param B [out] Normalization coefficients
    /// \param Q [in] The orthogonal basis against which to project
    ///
    /// \return Rank of X_in after projection
    ///
    /// \note We expose this interface to applications for the same
    ///   reason that we expose normalizeOutOfPlace().
    int 
    projectAndNormalizeOutOfPlace (MV& X_in, 
				   MV& X_out,
				   Teuchos::Array<mat_ptr> C,
				   mat_ptr B,
				   Teuchos::ArrayView<Teuchos::RCP<const MV> > Q);
    
    /// \brief Return \f$ \| I - X^* \cdot X \|_F \f$
    ///
    /// Return the Frobenius norm of I - X^* X, which is an absolute
    /// measure of the orthogonality of the columns of X.
    magnitude_type 
    orthonormError (const MV &X) const
    {
      const Scalar ONE = SCT::one();
      const int ncols = MVT::GetNumberVecs(X);
      mat_type XTX (ncols, ncols);
      innerProd (X, X, XTX);
      for (int k = 0; k < ncols; ++k)
	XTX(k,k) -= ONE;
      return XTX.normFrobenius();
    }

    //! Return the Frobenius norm of the inner product of X1 and X1
    magnitude_type 
    orthogError (const MV &X1, 
		 const MV &X2) const
    {
      const int ncols_X1 = MVT::GetNumberVecs (X1);
      const int ncols_X2 = MVT::GetNumberVecs (X2);
      mat_type X1_T_X2 (ncols_X1, ncols_X2);
      innerProd (X1, X2, X1_T_X2);
      return X1_T_X2.normFrobenius();
    }

    /// Relative tolerance for triggering a block reorthogonalization.
    /// If any column norm in a block decreases by this amount, then
    /// we reorthogonalize.
    magnitude_type blockReorthogThreshold() const { return blockReorthogThreshold_; }

    /// Relative tolerance for determining (via the SVD) whether a
    /// block is of full numerical rank.
    magnitude_type relativeRankTolerance() const { return relativeRankTolerance_; }

  private:

    //! Configuration parameters
    Teuchos::RCP<const Teuchos::ParameterList> params_;
    //! Label for timers (if timers are used)
    std::string label_;
    //! Interface to TSQR implementation
    tsqr_adaptor_ptr tsqrAdaptor_;
    /// \brief Scratch space for TSQR
    ///
    /// Only allocated if normalize() is called.  We do our best to
    /// avoid allocation and recycle this space whenever possible.
    /// normalizeOutOfPlace() does _not_ allocate Q_, which you can
    /// use to your advantage.
    ///
    /// \warning MultiVecTraits doesn't let you express data
    /// distribution (Epetra_Map or Tpetra::Map) or "range" (as an
    /// operator: domain -> range, as in Thyra).  Thus, we have no way
    /// of guaranteeing that Q_ has the same data distribution as the
    /// input vector X to normalize().  There are only two ways to fix
    /// this: Add a "Map" feature to MultiVecTraits (with at least a
    /// test for compatibility of data distributions), or replace
    /// MultiVecTraits with Thyra.
    Teuchos::RCP<MV> Q_;
    //! Machine precision for Scalar
    magnitude_type eps_;

    /// Whether to fill in null space vectors (after normalization)
    /// with random data.
    bool randomizeNullSpace_;
    //! Whether to reorthogonalize blocks at all.
    bool reorthogonalizeBlocks_;
    /// Whether to throw an exception when an orthogonalization fault
    /// occurs.  Recovery is possible, but expensive.
    bool throwOnReorthogFault_;
    //! Relative reorthogonalization threshold in Block Gram-Schmidt.
    magnitude_type blockReorthogThreshold_;
    //! Relative tolerance for measuring the numerical rank of a matrix.
    magnitude_type relativeRankTolerance_;

#ifdef BELOS_TEUCHOS_TIME_MONITOR
    //! Timer for all orthogonalization operations
    Teuchos::RCP< Teuchos::Time > timerOrtho_;
    //! Timer for projection operations
    Teuchos::RCP< Teuchos::Time > timerProject_;
    //! Timer for normalization operations
    Teuchos::RCP< Teuchos::Time > timerNormalize_;

    /// Instantiate and return a timer with an appropriate label
    ///
    /// \param prefix [in] Prefix for the timer label, e.g., "Belos"
    /// \param timerName [in] Name of the timer, or what the timer
    ///   is timing, e.g., "Projection" or "Normalization"
    ///
    /// \return Smart pointer to a new Teuchos::Time timer object,
    ///   to be used via Teuchos::TimeMonitor
    static Teuchos::RCP< Teuchos::Time >
    makeTimer (const std::string& prefix, 
	       const std::string& timerName)
    {
      const std::string timerLabel = 
	prefix.empty() ? timerName : (prefix + ": " + timerName);
      return Teuchos::TimeMonitor::getNewTimer (timerLabel);
    }
#endif // BELOS_TEUCHOS_TIME_MONITOR

    //! Throw a reorthgonalization fault exception
    void
    raiseReorthogFault (const std::vector<magnitude_type>& normsAfterFirstPass,
			const std::vector<magnitude_type>& normsAfterSecondPass,
			const std::vector<int>& faultIndices);

    /// Try to return a boolean parameter with the given key.  If no
    /// parameter with that key exists, return the value of the
    /// corresponding default parameter (using getDefaultParameters(),
    /// and caching the returned RCP from that function, so the
    /// function is only called once).
    bool 
    getBoolParamWithDefault (const Teuchos::RCP<const Teuchos::ParameterList>& params, 
			     const std::string& key)
    {
      using Teuchos::Exceptions::InvalidParameterName;
      using Teuchos::Exceptions::InvalidParameterType;
      // Note: We don't try to catch an exception when reading from
      // the default parameters, since they key had better be there.
      // (I guess we could catch and rethrow std::logic_error...)
      Teuchos::RCP<const Teuchos::ParameterList> defaultParams = 
	getDefaultParameters();
      if (params.is_null())
	return defaultParams->get< bool >(key);
      else
	{
	  try {
	    // No validation is necessary or even sensible; validating
	    // a boolean value would mean it's not really an option!
	    return params->get< bool >(key);
	  } catch (InvalidParameterName&) {
	    return defaultParams->get< bool >(key);
	  } catch (InvalidParameterType&) {
	    std::ostringstream os;
	    os << "The value of parameter \"" << key << "\" to Tsqr(Mat)"
	      "OrthoManager(Impl) must be a boolean truth value.  Here "
	      "is the documentation for that parameter:" << std::endl
	       << defaultParams->getEntry(key).docString();
	    throw std::invalid_argument (os.str());
	  }
	}
    }

    /// Try to return a nonnegative magnitude_type-valued parameter
    /// with the given key.  If no parameter with that key exists,
    /// return the value of the corresponding default parameter (using
    /// getDefaultParameters(), and caching the returned RCP from that
    /// function, so the function is only called once).
    magnitude_type
    getMagParamWithDefault (const Teuchos::RCP<const Teuchos::ParameterList>& params, 
			    const std::string& key)
    {
      using Teuchos::Exceptions::InvalidParameterName;
      using Teuchos::Exceptions::InvalidParameterType;
      // Note: We don't try to catch an exception when reading from
      // the default parameters, since they key had better be there.
      // (I guess we could catch and rethrow std::logic_error...)
      Teuchos::RCP<const Teuchos::ParameterList> defaultParams = 
	getDefaultParameters();
      if (params.is_null())
	return defaultParams->get< magnitude_type >(key);
      else
	{
	  try {
	    const magnitude_type value = params->get< magnitude_type >(key);
	    if (value >= magnitude_type(0)) // Validate
	      return value;
	    else
	      {
		std::ostringstream os;
		os << "You specified " << key << " = " << value
		   << ", but that parameter must be a nonnegative real "
		   << "floating-point value.  Here is the documentation " 
		   << "for that parameter:" << std::endl
		   << defaultParams->getEntry(key).docString();
		throw std::invalid_argument (os.str());
	      }
	  } catch (InvalidParameterName&) { 
	    return defaultParams->get< magnitude_type >(key);
	  } catch (InvalidParameterType&) {
	    std::ostringstream os;
	    os << "The value of parameter \"" << key << "\" to Tsqr(Mat)OrthoMa"
	      "nager(Impl) must be a nonnegative real floating-point value.  "
	      "Here is the documentation for that parameter:" << std::endl
	       << defaultParams->getEntry(key).docString();
	    throw std::invalid_argument (os.str());
	  }
	}
    }

    //! Set parameters from the given parameter list
    void
    readParams (const Teuchos::RCP<const Teuchos::ParameterList>& params)
    {
      randomizeNullSpace_ = 
	getBoolParamWithDefault (params, "randomizeNullSpace");
      reorthogonalizeBlocks_ = 
	getBoolParamWithDefault (params, "reorthogonalizeBlocks");
      throwOnReorthogFault_ = 
	getBoolParamWithDefault (params, "throwOnReorthogFault");
      blockReorthogThreshold_ = 
	getMagParamWithDefault (params, "blockReorthogThreshold");
      relativeRankTolerance_ = 
	getMagParamWithDefault (params, "relativeRankTolerance");
    }

    /// \brief Initialize the TSQR adaptor and scratch space
    ///
    /// Initialize the TSQR adaptor and scratch space for TSQR.  Both
    /// require a specific MV object, so we have to delay their
    /// initialization until we get an X input (for normalize(), since
    /// only that method uses the TSQR adaptor and the scratch space).
    /// (Hence, "lazy," for delayed initialization.)
    void
    lazyInit (const MV& X)
    {
      using Teuchos::ParameterList;
      using Teuchos::RCP;
      using Teuchos::rcp;

      // TSQR parameters.  They are stored as an RCP<const
      // ParameterList>, so we don't have to worry about the sublist
      // going out of scope.
      RCP<const ParameterList> tsqrParams;
      try {
	tsqrParams = params_->get< RCP<const ParameterList> >("TsqrImpl");
      } catch (Teuchos::Exceptions::InvalidParameter&) {
	RCP<const ParameterList> defaultParams = getDefaultParameters();
	tsqrParams = defaultParams->get< RCP<const ParameterList> >("TsqrImpl");
      }	
      TEST_FOR_EXCEPTION(tsqrParams.is_null(), std::logic_error,
			 "Should never get here!");
      // The TSQR adaptor object requires a specific MV object for
      // initialization.  As long as subsequent MV objects use the
      // same communicator (e.g., the same Teuchos::Comm<int>), we
      // don't need to reinitialize the adaptor.
      //
      // FIXME (mfh 15 Jul 2010) If tsqrAdaptor_ has already been
      // initialized, check to make sure that X has the same Map /
      // communicator as the multivector previously used to initialize
      // tsqrAdaptor_.
      if (tsqrAdaptor_.is_null())
	tsqrAdaptor_ = rcp (new tsqr_adaptor_type (X, tsqrParams));

      // NOTE: We don't try to allocate the temporary workspace Q_
      // here.  This is done on demand in normalize().
    }

    //! Check and set dimensions of X and Q
    void
    checkProjectionDims (int& nrows_X, 
			 int& ncols_X, 
			 int& num_Q_blocks,
			 int& ncols_Q_total,
			 const MV& X, 
			 Teuchos::ArrayView<Teuchos::RCP<const MV> > Q) const
    {
      // First assign to temporary values, so the function won't
      // commit any externally visible side effects unless it will
      // return normally (without throwing an exception).
      int __nrows_X, __ncols_X, __num_Q_blocks, __ncols_Q_total;
      __num_Q_blocks = Q.size();
      __nrows_X = MVT::GetVecLength (X);
      __ncols_X = MVT::GetNumberVecs (X);

      // Compute the total number of columns of all the Q[i] blocks.
      __ncols_Q_total = 0;
      for (int i = 0; i < num_Q_blocks; ++i)
	{
	  const int nrows_Q = MVT::GetVecLength (*Q[i]);
	  TEST_FOR_EXCEPTION( (nrows_Q != __nrows_X), 
			      std::invalid_argument,
			      "Belos::TsqrOrthoManagerImpl::checkProjectionDims(): "
			      "Size of X not consistant with size of Q" );
	  __ncols_Q_total += MVT::GetNumberVecs (*Q[i]);
	}
      // Commit temporary values to the output arguments.
      nrows_X = __nrows_X;
      ncols_X = __ncols_X;
      num_Q_blocks = __num_Q_blocks;
      ncols_Q_total = __ncols_Q_total;
    }

    /// \brief Allocate projection coefficients
    /// 
    /// \param C [out] Array of projection coefficient matrices
    /// \param Q [in] Array of MV against which to project
    /// \param X [in] MV to project against the entries of Q
    /// \param attemptToRecycle [in] Hint whether to check the
    ///   existing entries of C to see if they have already been
    ///   allocated and have the right dimensions.  This function will
    ///   do the right thing regardless, but the hint might improve
    ///   performance by avoiding unnecessary allocations or checks.
    void
    allocateProjectionCoefficients (Teuchos::Array<mat_ptr>& C, 
				    Teuchos::ArrayView<Teuchos::RCP<const MV> > Q,
				    const MV& X,
				    const bool attemptToRecycle = true) const;
    int 
    projectAndNormalizeImpl (MV& X_in, 
			     MV& X_out, // Only written if outOfPlace==false.
			     const bool outOfPlace,
			     Teuchos::Array<mat_ptr> C,
			     mat_ptr B,
			     Teuchos::ArrayView<Teuchos::RCP<const MV> > Q);

    /// \brief One projection pass of X against the Q[i] blocks
    ///
    /// Perform one projection pass (Modified Block Gram-Schmidt) of X
    /// against the Q[i] blocks.  Does not allocate C[i] coefficients,
    /// and does not reorthogonalize.
    void
    rawProject (MV& X, 
		Teuchos::ArrayView<Teuchos::RCP<const MV> > Q,
		Teuchos::Array<mat_ptr> C) const
    {
#ifdef BELOS_TEUCHOS_TIME_MONITOR
      // rawProject() is part of orthogonalization, so we time it with
      // total orthogonalization as well as with projection.  If
      // rawProject() is called from project(), the TimeMonitor won't
      // start timerProject_, because it is already running in
      // project().  Similarly, if rawProject() is called from
      // projectAndNormalize(), the TimeMonitor won't start
      // timerProject_, because it is already running in
      // projectAndNormalize().
      Teuchos::TimeMonitor timerMonitorOrtho(*timerOrtho_);
      Teuchos::TimeMonitor timerMonitorProject(*timerProject_);
#endif // BELOS_TEUCHOS_TIME_MONITOR

      // "Modified Gram-Schmidt" version of Block Gram-Schmidt.
      const int num_Q_blocks = Q.size();
      for (int i = 0; i < num_Q_blocks; ++i)
	{
	  TEST_FOR_EXCEPTION(C[i].is_null(), std::logic_error,
			     "TsqrOrthoManagerImpl::rawProject(): C[" 
			     << i << "] is null");
	  TEST_FOR_EXCEPTION(Q[i].is_null(), std::logic_error,
			     "TsqrOrthoManagerImpl::rawProject(): Q[" 
			     << i << "] is null");
	  mat_type& Ci = *C[i];
	  const MV& Qi = *Q[i];
	  innerProd (Qi, X, Ci);
	  MVT::MvTimesMatAddMv (-SCT::one(), Qi, Ci, SCT::one(), X);
	}
    }

    /// Normalize X into Q*B, overwriting X in the process.
    ///
    /// \warning Q must have _exactly_ as many columns as X.
    ///
    /// \warning B must have been allocated and must have the right
    ///   dimensions (square, with number of rows/columns equal to the
    ///   number of columns in X).
    int 
    rawNormalize (MV& X, MV& Q, mat_type& B)
    {
      int rank;
      try {
	// This call only computes the QR factorization X = Q B.
	// It doesn't compute the rank of X.  That comes from
	// revealRank() below.
	tsqrAdaptor_->factorExplicit (X, Q, B);
	// This call will only modify *B if *B on input is not of full
	// numerical rank.
	rank = tsqrAdaptor_->revealRank (Q, B, relativeRankTolerance_);
      } catch (std::exception& e) {
	throw TsqrOrthoError (e.what()); // Toss the exception up the chain.
      }
      return rank;
    }

    /// Special case of normalize() when X has only one column.
    /// The operation is done in place in X.  We assume that B(0,0)
    /// makes sense, since we're going to assign to it.
    int
    normalizeOne (MV& X, mat_ptr B) const
    {
      std::vector< magnitude_type > theNorm (1, SCTM::zero());
      MVT::MvNorm (X, theNorm);
      if (Teuchos::is_null(B))
	B = Teuchos::rcp (new mat_type (1, 1));
      (*B)(0,0) = theNorm[0];
      if (theNorm[0] == SCTM::zero())
	{
	  // Make a view of the first column of Q, fill it with random
	  // data, and normalize it.  Throw away the resulting norm.
	  if (randomizeNullSpace_)
	    {
	      MVT::MvRandom(X);
	      MVT::MvNorm (X, theNorm);
	      if (theNorm[0] == SCTM::zero())
		throw TsqrOrthoError("normalizeOne(): a supposedly random "
				     "vector has norm zero!");
	      else
		{
		  // FIXME (mfh 09 Nov 2010) I'm assuming that this
		  // operation that converts from a magnitude_type to
		  // a Scalar makes sense.
		  const Scalar alpha = SCT::one() / theNorm[0];
		  MVT::MvScale (X, alpha);
		}
	    }
	  return 0;
	}
      else
	{
	  // FIXME (mfh 09 Nov 2010) I'm assuming that this
	  // operation that converts from a magnitude_type to
	  // a Scalar makes sense.
	  const Scalar alpha = SCT::one() / theNorm[0];
	  MVT::MvScale (X, alpha);
	  return 1;
	}
    }

    /// Normalize X into Q*B, overwriting X in the process.
    ///
    /// \note Q must have at least as many columns as X.  It may have
    /// more columns than X.  This routine doesn't try to allocate
    /// space for Q if it is too small.
    int 
    normalizeImpl (MV& X, MV& Q, mat_ptr B)
    {
      using Teuchos::Range1D;
      using Teuchos::RCP;
      using Teuchos::rcp;

#ifdef BELOS_TEUCHOS_TIME_MONITOR
      Teuchos::TimeMonitor timerMonitorNormalize(*timerNormalize_);
      // If normalize() is called internally -- i.e., called from
      // projectAndNormalize() -- the timer will not be started or 
      // stopped, because it is already running.  TimeMonitor handles
      // recursive invocation by doing nothing.
      Teuchos::TimeMonitor timerMonitorOrtho(*timerOrtho_);
#endif // BELOS_TEUCHOS_TIME_MONITOR

      const int numCols = MVT::GetNumberVecs (X);
      if (numCols == 0)
	return 0; // Fast exit for an empty input matrix
      else if (MVT::GetNumberVecs(Q) < numCols)
	throw std::logic_error("Q has too few columns");

      // TSQR wants a Q with the same number of columns as X, so have
      // it work on a nonconstant view of Q with the same number of
      // columns as X.
      RCP< MV > Q_view = MVT::CloneViewNonConst (Q, Range1D(0, numCols-1));

      // TSQR's rank-revealing part doesn't work unless B is provided.
      // If B is not provided, allocate a temporary B for use in TSQR.
      // If it is provided, adjust dimensions as necessary.
      if (Teuchos::is_null(B))
	B = rcp (new mat_type (numCols, numCols));
      else
	B->shape (numCols, numCols);
      //
      // Compute rank-revealing decomposition (in this case, TSQR of X
      // followed by SVD of the R factor and appropriate updating of
      // the resulting Q factor) of X.  X is modified in place, and
      // Q_view contains the resulting explicit Q factor.  Later, we
      // will copy this back into X.
      //
      // The matrix *B will only be upper triangular if X is of full
      // numerical rank.
      //
      const int rank = rawNormalize (X, *Q_view, *B);

      // Whether we've copied the orthogonalized basis vectors back
      // into X yet.  We keep track of this because if we add in
      // randomized null space basis vectors, we'll be using part of X
      // as scratch space, and won't need to copy back all of Q_view.
      bool copiedVectorsBackYet = false;

      // Now we should copy Q_view back into X, but don't do it yet:
      // if we want to fill the last ncols-rank columns with random
      // data, we should do so in Q_view, because it's fresh in the
      // cache.
      if (randomizeNullSpace_ && rank < numCols)
	{
	  // If X did not have full (numerical rank), augment the last
	  // ncols-rank columns of X with random data.
	  const int nullSpaceNumCols = numCols - rank;
	  // if (ncolsToFill > 0) <- always true if we are here.

	  // ind: Indices of columns of X to fill with random data.
	  std::vector<int> nullSpaceIndices (nullSpaceNumCols);
	  for (int j = 0; j < nullSpaceNumCols; ++j)
	    nullSpaceIndices[j] = j + rank;

	  RCP<MV> Q_null = MVT::CloneViewNonConst (*Q_view, nullSpaceIndices);
	  MVT::MvRandom (*Q_null); // Fill Q_null with random data

	  // Project the random data against the column space basis of
	  // X, to ensure that the random data is orthogonal to it.
	  // We do this using a simple block projection ("Block
	  // Classical Gram-Schmidt").  This is accurate because we've
	  // already orthogonalized the column space basis of X nearly
	  // to machine precision via a QR factorization (TSQR) with
	  // accuracy comparable to Householder QR.
	  RCP<const MV> Q_col;
	  RCP<MV> X_col;
	  {
#ifdef BELOS_TEUCHOS_TIME_MONITOR
	    Teuchos::TimeMonitor timerMonitorProject(*timerProject_);
#endif // BELOS_TEUCHOS_TIME_MONITOR

	    // Make temporary storage for the projection coefficients.
	    // We don't need to keep the coefficients, since the
	    // random vectors are replacing the null space basis of X.
	    // (The null space basis is more or less just biased
	    // noise, and it's better to replace biased noise with
	    // unbiased noise.)
	    if (rank > 0)
	      {
		Q_col = MVT::CloneView (*Q_view, Range1D(0, rank-1));
		X_col = MVT::CloneViewNonConst (X, Range1D(0, rank-1));
		mat_type C_colnull (rank, nullSpaceNumCols);
		// C_colnull := <Q_col, Q_null>
		innerProd (*Q_col, *Q_null, C_colnull);
		// Q_null := Q_null - Q_col * C_colnull
		MVT::MvTimesMatAddMv (-SCT::one(), *Q_col, C_colnull, SCT::one(), *Q_null);
	      }
	  }
	  // Normalize the projected random vectors, so that they are
	  // mutually orthogonal (as well as orthogonal to the column
	  // space basis of X).  
	  Teuchos::RCP< MV > X_null; // Will set this in the "try" block below
	  int nullSpaceBasisRank; // Will set this in the "try" block below
	  try {
	    // We use (part of) X here as scratch space, since
	    // we are going to copy the results from Q_view back into X
	    // anyway.  (That is, don't get confused that X is the
	    // output of TSQR here.  We're just avoiding the allocation
	    // of scratch space.)  
	    X_null = MVT::CloneViewNonConst (X, Range1D(rank, numCols-1));

	    // We don't need to keep the normalization coefficients,
	    // but TSQR needs somewhere to put them.
	    mat_type B_null (nullSpaceNumCols, nullSpaceNumCols);

	    // Remember that we are using X_null for the output of
	    // TSQR here.  We don't need to copy it again since it
	    // will contain the final results, as long as the
	    // randomized null space basis rank is the same as the
	    // number of random columns.  If the random vectors are
	    // not full rank, things get more complicated (see below).
	    nullSpaceBasisRank = rawNormalize (*Q_null, *X_null, B_null);
	  } catch (std::exception& e) {
	    throw TsqrOrthoError (e.what()); // Toss the exception up the chain.
	  }
	  // It's possible, but unlikely, that X_null doesn't have
	  // full rank (after the projection step).  We could
	  // recursively fill in more random vectors until we finally
	  // get a full rank matrix, but instead we just throw an
	  // exception.
	  //
	  // FIXME (mfh 08 Nov 2010) Perhaps we should deal with this
	  // case more elegantly.  Recursion might be one way to solve
	  // it, but be sure to check that the recursion will
	  // terminate.  We could do this by supplying an additional
	  // argument to rawNormalize, which is the null space basis
	  // rank from the previous iteration.  The rank has to
	  // decrease each time, or the recursion may go on forever.
	  if (nullSpaceBasisRank < nullSpaceNumCols)
	    {
	      std::ostringstream os;
	      os << "Random vectors after projection have rank " 
		 << nullSpaceBasisRank << ", but should have rank " 
		 << nullSpaceNumCols;
	      throw TsqrOrthoError(os.str());
	    }
	  // X_null now has the final results in it.  Copy Q_col back
	  // into X_col as well.  The idiom with MVT is to do X_col :=
	  // 1*Q_col + 0*X_col.  The aliasing rules for this operation
	  // aren't stated in the MVT interface, but it's likely no
	  // different than the BLAS' AXPY rules.
	  MVT::MvAddMv (SCT::one(), *Q_col, SCT::zero(), *X_col, *X_col);
	  copiedVectorsBackYet = true;
	}
      // If we didn't fill in a random null space basis, then we still
      // have to copy the normalized basis back from Q_view into X.
      if (! copiedVectorsBackYet)
	MVT::MvAddMv (SCT::one(), *Q_view, SCT::zero(), X, X);

      return rank;
    }
  };


  template<class Scalar, class MV>
  TsqrOrthoManagerImpl<Scalar, MV>::
  TsqrOrthoManagerImpl (const Teuchos::RCP<const Teuchos::ParameterList>& params,
			const std::string& label) :
    params_ (params.is_null() ? getDefaultParameters() : params),
    label_ (label),
    tsqrAdaptor_ (Teuchos::null),   // Initialized on demand
    Q_ (Teuchos::null),             // Scratch space for normalize()
    eps_ (SCT::eps()),              // Machine precision
    randomizeNullSpace_ (true),     // Set later by readParams()
    reorthogonalizeBlocks_ (true),  // Set later by readParams()
    throwOnReorthogFault_ (false),  // Set later by readParams()
    blockReorthogThreshold_ (0),    // Set later by readParams()
    relativeRankTolerance_ (0)      // Set later by readParams()
  {
#ifdef BELOS_TEUCHOS_TIME_MONITOR
    timerOrtho_ = makeTimer (label, "All orthogonalization");
    timerProject_ = makeTimer (label, "Projection");
    timerNormalize_ = makeTimer (label, "Normalization");
#endif // BELOS_TEUCHOS_TIME_MONITOR

    // Extract values for the parameters from the given parameter
    // list.  Use default values if none are provided.  The "TSQR"
    // sublist gets passed along to the underlying TSQR
    // implementation.
    readParams (params_);
  }

  template<class Scalar, class MV>
  void
  TsqrOrthoManagerImpl<Scalar, MV>::project (MV& X, 
					     Teuchos::Array<mat_ptr> C,
					     Teuchos::ArrayView<Teuchos::RCP<const MV> > Q)
  {
#ifdef BELOS_TEUCHOS_TIME_MONITOR
    // project() is part of orthogonalization, so we time it with
    // total orthogonalization as well as with projection.  If
    // project() is called from projectAndNormalize(), the TimeMonitor
    // won't start timerOrtho_, because it is already running in
    // projectAndNormalize().
    Teuchos::TimeMonitor timerMonitorOrtho(*timerOrtho_);
    Teuchos::TimeMonitor timerMonitorProject(*timerProject_);
#endif // BELOS_TEUCHOS_TIME_MONITOR

    // Internal data used by this method require a specific MV object
    // for initialization (e.g., to get a Map / communicator, and to
    // initialize scratch space).  Thus, we delay (hence "lazy")
    // initialization until we get an X.
    lazyInit (X);

    int nrows_X, ncols_X, num_Q_blocks, ncols_Q_total;
    checkProjectionDims (nrows_X, ncols_X, num_Q_blocks, ncols_Q_total, X, Q);
    // Test for quick exit: any dimension of X is zero, or there are
    // zero Q blocks, or the total number of columns of the Q blocks
    // is zero.
    if (nrows_X == 0 || ncols_X == 0 || num_Q_blocks == 0 || ncols_Q_total == 0)
      return;
    
    // If we don't have enough C, expanding it creates null references.
    // If we have too many, resizing just throws away the later ones.
    // If we have exactly as many as we have Q, this call has no effect.
    C.resize (num_Q_blocks);
    for (int k = 0; k < num_Q_blocks; ++k) 
      {
	const int ncols_Q_k = MVT::GetNumberVecs (*Q[k]);
	// Create a new C[k] if necessary; resize the existing C[k]
	// if necessary.
	if (C[k] == Teuchos::null)
	  C[k] = Teuchos::rcp (new mat_type (ncols_Q_k, ncols_X));
	else if (C[k]->numRows() != ncols_Q_k || C[k]->numCols() != ncols_X)
	  C[k]->shape (ncols_Q_k, ncols_X);
	else
	  C[k]->putScalar (Scalar(0));
      }

    // We only use columnNormsBefore if doing block
    // reorthogonalization.
    std::vector<magnitude_type> columnNormsBefore (ncols_X, magnitude_type(0));
    if (reorthogonalizeBlocks_)
      MVT::MvNorm (X, columnNormsBefore);

    // Project (first block orthogonalization step): 
    // C := Q^* X, X := X - Q C.
    rawProject (X, Q, C); 

    // If we are doing block reorthogonalization, reorthogonalize X if
    // necessary.
    if (reorthogonalizeBlocks_)
      {
	std::vector<magnitude_type> columnNormsAfter (ncols_X, magnitude_type(0));
	MVT::MvNorm (X, columnNormsAfter);
	
	const magnitude_type threshold = blockReorthogThreshold_;
	// Reorthogonalize X if any of its column norms decreased by a
	// factor more than the block reorthogonalization threshold.
	// Don't bother trying to subset the columns; that will make
	// the columns noncontiguous and thus hinder BLAS 3
	// optimizations.
	bool reorthogonalize = false;
	for (int j = 0; j < ncols_X; ++j)
	  if (columnNormsAfter[j] < threshold * columnNormsBefore[j])
	    {
	      reorthogonalize = true;
	      break;
	    }
	if (reorthogonalize)
	  {
	    for (int k = 0; k < num_Q_blocks; ++k)
	      {
		const int ncols_Q_k = MVT::GetNumberVecs (*Q[k]);
		// FIXME (mfh 06 Nov 2010)
		//
		// Teuchos::SerialDenseMatrix::reshape() currently
		// allocates memory even if the new dimensions are the
		// same as the old ones.  Thus, we might as well just
		// create a new C2_k for every loop iteration.  It
		// would be better for SerialDenseMatrix to recycle
		// existing storage whenever possible, at least when
		// the new matrix dimensions are the same.
		mat_type C2_k (ncols_Q_k, ncols_X);

		// Redo the projection for this block, and update coefficients.
		innerProd (*Q[k], X, C2_k);
		MVT::MvTimesMatAddMv (Scalar(-1), *Q[k], C2_k, Scalar(1), X);
		*C[k] += C2_k;
	      }
	  }
      }	  
  }


  template<class Scalar, class MV>
  int 
  TsqrOrthoManagerImpl<Scalar, MV>::normalize (MV& X, mat_ptr B)
  {
#ifdef BELOS_TEUCHOS_TIME_MONITOR
    Teuchos::TimeMonitor timerMonitorNormalize(*timerNormalize_);
    // If normalize() is called internally -- i.e., called from
    // projectAndNormalize() -- the timer will not be started or 
    // stopped, because it is already running.  TimeMonitor handles
    // recursive invocation by doing nothing.
    Teuchos::TimeMonitor timerMonitorOrtho(*timerOrtho_);
#endif // BELOS_TEUCHOS_TIME_MONITOR

    // Internal data used by this method require a specific MV
    // object for initialization (in particular, to get a Map /
    // communicator object).  Thus, we delay (hence "lazy")
    // initialization until we get an X.
    lazyInit (X);

    // MVT returns int for this, even though the "local ordinal
    // type" of the MV may be some other type (for example,
    // Tpetra::MultiVector<double, int32_t, int64_t, ...>).
    const int numCols = MVT::GetNumberVecs (X);

    // This special case (for X having only one column) makes
    // TsqrOrthoManagerImpl equivalent to Modified Gram-Schmidt
    // orthogonalization with conditional full reorthogonalization,
    // if all multivector inputs have only one column.  It also
    // avoids allocating Q_ scratch space and copying data when we
    // don't need to invoke TSQR at all.
    if (numCols == 1)
      return normalizeOne (X, B);

    // We use Q_ as scratch space for the normalization, since TSQR
    // requires a scratch multivector (it can't factor in place).
    // Q_ should have the same number of rows as X, and at least as
    // many columns.  If not, we have to reallocate.  We also have
    // to allocate (not "re-") Q_ if we haven't allocated it before.
    // (We can't allocate Q_ until we have some X, so we need a
    // multivector as the "prototype.")
    //
    // FIXME (mfh 07 Nov 2010, 11 Jan 2011) We assume that Q_ has
    // the right number of rows and uses the same Map / communicator
    // as X.  The only way to fix this is to refactor the TSQR
    // implemention so it understands the data layout (i.e., so it
    // uses a Map rather than a communicator).
    //
    // NOTE (mfh 11 Jan 2011) We only increase the number of columsn
    // in Q_, never decrease.  This is OK for typical uses of TSQR,
    // but you might prefer different behavior in some cases.
    if (Q_.is_null() || numCols > MVT::GetNumberVecs(*Q_) ||
	MVT::GetVecLength(X) != MVT::GetVecLength(*Q_))
      Q_ = MVT::Clone (X, numCols);

    // normalizeImpl() wants the second MV argument to have the same
    // number of columns as X.  To ensure this, we pass it a view of
    // Q_ if Q_ has more columns than X.  (This is possible if we
    // previously called normalize() with a different multivector,
    // since we never reallocate Q_ if it has more columns than
    // necessary.)
    if (MVT::GetNumberVecs(*Q_) == numCols)
      return normalizeImpl (X, *Q_, B);
    else
      {
	Teuchos::RCP<MV> Q_view = MVT::CloneViewNonConst (*Q_, Teuchos::Range1D(0, numCols-1));
	return normalizeImpl (X, *Q_view, B);
      }
  }


  template<class Scalar, class MV>
  int 
  TsqrOrthoManagerImpl<Scalar, MV>::
  projectAndNormalize (MV &X,
		       Teuchos::Array<mat_ptr> C,
		       mat_ptr B,
		       Teuchos::ArrayView<Teuchos::RCP<const MV> > Q)
  {
    using Teuchos::Range1D;
    using Teuchos::RCP;
    using Teuchos::rcp;

#ifdef BELOS_TEUCHOS_TIME_MONITOR
    Teuchos::TimeMonitor timerMonitorOrtho(*timerOrtho_);
#endif // BELOS_TEUCHOS_TIME_MONITOR

    // Fetch dimensions of X and Q.
    int nrows_X, ncols_X, num_Q_blocks, ncols_Q_total;
    checkProjectionDims (nrows_X, ncols_X, num_Q_blocks, ncols_Q_total, X, Q);

    // Test for quick exit: any dimension of X is zero.
    if (nrows_X == 0 || ncols_X == 0)
      return 0;

    // If there are zero Q blocks or zero Q columns, just normalize!
    if (num_Q_blocks == 0 || ncols_Q_total == 0)
      return normalize (X, B);
      
    // If we don't have enough C, expanding it creates null references.
    // If we have too many, resizing just throws away the later ones.
    // If we have exactly as many as we have Q, this call has no effect.
    C.resize (num_Q_blocks);
    Teuchos::Array<mat_ptr> newC (num_Q_blocks);
    for (int i = 0; i < num_Q_blocks; ++i) 
      {
	const int ncols_Q_i = MVT::GetNumberVecs (*Q[i]);
	// Create a new C[i] if necessary.
	if (C[i].is_null())
	  C[i] = rcp (new mat_type (ncols_Q_i, ncols_X));
	else if (C[i]->numRows() != ncols_Q_i || C[i]->numCols() != ncols_X)
	  C[i]->shape (ncols_Q_i, ncols_X);
	else
	  C[i]->putScalar (SCT::zero());

	// Create newC[i] as well, with the same dimensions as C[i].
	newC[i] = rcp (new mat_type (ncols_Q_i, ncols_X));
      }

    // If we are doing block reorthogonalization, then compute the
    // column norms of X before projecting for the first time.  This
    // will help us decide whether we need to reorthogonalize X.
    std::vector<magnitude_type> normsBeforeFirstPass (ncols_X, magnitude_type(0));
    if (reorthogonalizeBlocks_)
      MVT::MvNorm (X, normsBeforeFirstPass);

    // First (Modified) Block Gram-Schmidt pass:
    // C := Q^* X
    // X := X - Q C
    rawProject (X, Q, C);

    // Before normalizing the matrix X, be sure to allocate a new B
    // matrix here.  If we let the normalize() routine allocate,
    // that storage will go away at the end of normalize().
    if (B.is_null())
      B = rcp (new mat_type (ncols_X, ncols_X));

    // Rank of X after first projection pass.
    const int firstPassRank = normalize (X, B);
    // Current rank of X
    int rank = firstPassRank;

    // If X (after the projection step) is not full rank and
    // randomizeNullSpace_ is set, normalize() replaced the null
    // space basis with random vectors, and orthogonalized those
    // against the already orthogonalized column space basis of X.
    // However, we still need to make the random vectors orthogonal
    // to the Q blocks.  If the rank of X is r and X has n total
    // columns, we have
    //
    // Original X = X(:, 1:r) B(1:r, :) + \sum_k Q[k] C[k]
    //
    // This is true even if B is not upper triangular (normalize()
    // does not promise that B is upper triangular, if X is not full
    // rank).  Remember that the numerical rank of X is r because
    // (by definition) the norm of B(r+1:n, :) is small.  Thus, we
    // can replace X(:, r+1:n) with whatever we want without
    // changing the above relation.  We just want X(:, r+1:n) to be
    // orthogonal to the Q[k] and to X(:, r+1:n).  The normalize()
    // method has already made X(:, r+1:n) orthogonal to X(:, 1:r),
    // so we only need to project it against the Q[k] blocks.  We
    // don't need to keep the coefficients, since they are
    // multiplied by the "small" matrix B(r+1:n, :).
    //
    // FIXME (mfh 07 Nov 2010) Should combine orthogonalization of
    // the random vectors with the block reorthogonalization of X.
    if (firstPassRank < ncols_X && randomizeNullSpace_)
      {
	// Make a view of the null space basis columns of X.
	const int numNullSpaceCols = ncols_X - firstPassRank;
	const Range1D nullSpaceIndices (firstPassRank, ncols_X - 1);
	RCP<MV> X_null = MVT::CloneViewNonConst (X, nullSpaceIndices);

	// Space for projection coefficients (will be thrown away)
	Teuchos::Array<mat_ptr> C_null (num_Q_blocks);
	for (int k = 0; k < num_Q_blocks; ++k)
	  C_null[k] = rcp (new mat_type (MVT::GetNumberVecs(*Q[k]), numNullSpaceCols));
	// Project the new random vectors against the Q blocks
	rawProject (*X_null, Q, C_null);

	// (Re)normalize the new random vectors after the projection.
	// First, make space for the normalization coefficients (will
	// be thrown away).
	RCP<mat_type> B_null (new mat_type (numNullSpaceCols, numNullSpaceCols));
	// Normalize the random vectors
	const int randomVectorsRank = normalize (*X_null, B_null);

	// While unusual, it is still possible for the random data not
	// to be full rank after projection and normalization.  In
	// that case, we could try another set of random data and
	// recurse as necessary, but instead for now we just raise an
	// exception.
	TEST_FOR_EXCEPTION(randomVectorsRank != numNullSpaceCols, 
			   TsqrOrthoError, 
			   "Belos::TsqrOrthoManagerImpl::projectAndNormalize()"
			   ": After projecting and normalizing the random vect"
			   "ors (used to replace the null space basis vectors "
			   " from normalizing X), they have rank "
			   << randomVectorsRank << ", but should have full "
			   "rank " << numNullSpaceCols << ".");
      }

    // Whether or not X_in was full rank after projection, we still
    // might want to reorthogonalize against Q.
    if (reorthogonalizeBlocks_)
      {
	// We are only interested in the column space basis of X.
	std::vector<magnitude_type> normsAfterFirstPass (firstPassRank, magnitude_type(0));
	std::vector<magnitude_type> normsAfterSecondPass (firstPassRank, magnitude_type(0));

	// Compute post-first-pass (pre-normalization) norms.  We
	// could have done that using MVT::MvNorm() on X, after
	// calling rawProject() and before calling normalize()
	// above.  However, that operation may be expensive.  It is
	// also unnecessary: after calling normalize(), the 2-norm
	// of B(:,j) is the 2-norm of X(:,j) before normalization,
	// in exact arithmetic.
	//
	// NOTE (mfh 06 Nov 2010) This is one way that combining
	// projection and normalization into a single kernel --
	// projectAndNormalize() -- pays off.  In project(), we have
	// to compute column norms of X before and after projection.
	// Here, we get them for free from the normalization
	// coefficients.
	Teuchos::BLAS<int, Scalar> blas;
	for (int j = 0; j < firstPassRank; ++j)
	  {
	    const Scalar* const B_j = &(*B)(0,j);
	    // Teuchos::BLAS returns a magnitude_type result on
	    // Scalar inputs.
	    normsAfterFirstPass[j] = blas.NRM2 (firstPassRank, B_j, 1);
	  }
	// Test whether any of the norms dropped below the
	// reorthogonalization threshold.
	bool reorthogonalize = false;
	for (int j = 0; j < firstPassRank; ++j)
	  // If any column's norm decreased too much, mark this block
	  // for reorthogonalization.  Note that this test will _not_
	  // activate reorthogonalization if a column's norm before
	  // the first project-and-normalize step was zero.  It _will_
	  // activate reorthogonalization if the column's norm before
	  // was not zero, but is zero now.
	  if (normsAfterFirstPass[j] < blockReorthogThreshold() * normsBeforeFirstPass[j])
	    {
	      reorthogonalize = true; 
	      break;
	    }

	// Perform another Block Gram-Schmidt pass if necessary.
	// "Twice is enough" (Kahan's theorem) for a Krylov method,
	// unless (using Stewart's term) there is an
	// "orthogonalization fault" (indicated by reorthogFault).
	//
	// FIXME (mfh 07 Nov 2010) For now, we include the entire
	// block of X, including possible random data (that was
	// already projected and normalized above).  It would make
	// more sense just to process the first firstPassRank
	// columns of X.  However, the resulting reorthogonalization
	// should still be correct regardless.
	bool reorthogFault = false;
	// Indices of X at which there was an orthogonalization fault.
	std::vector<int> faultIndices;
	if (reorthogonalize)
	  {
	    // Block Gram-Schmidt (again).  Delay updating the block
	    // coefficients until we have the new normalization
	    // coefficients, which we need in order to do the update.
	    rawProject (X, Q, newC);
	      
	    Teuchos::RCP< mat_type > B2 (new mat_type (ncols_X, ncols_X));
	    const int secondPassRank = normalize (X, B2);
	    rank = secondPassRank;

	    // (Re)normalize the matrix X.

	    // Update normalization coefficients.  We begin with
	    // copying B, since the BLAS doesn't like aliased
	    // input arguments.
	    mat_type B_copy (Teuchos::Copy, *B, B->numRows(), B->numCols());
	    // B := B2 * B
	    if (0 != B->multiply (Teuchos::NO_TRANS, Teuchos::NO_TRANS, SCT::one(), *B2, B_copy, SCT::zero()))
	      throw std::logic_error ("Should never get here!");
	    
	    // Update the block coefficients from the projection step.
	    for (int k = 0; k < num_Q_blocks; ++k)
	      {
		mat_type C_k_copy (Teuchos::Copy, *C[k], C[k]->numRows(), C[k]->numCols());
		// C[k] := newC[k]*B_copy + C[k] (where B_copy is
		// the original B, before the second call to
		// normalize).
		if (0 != C[k]->multiply (Teuchos::NO_TRANS, Teuchos::NO_TRANS, SCT::one(), *newC[k], B_copy, SCT::one()))
		  throw std::logic_error ("Should never get here!");
	      }
	    
	    // Compute post-second-pass (pre-normalization) norms,
	    // using B2 (the coefficients from the second
	    // normalize() invocation) in the same way as above.
	    for (int j = 0; j < std::min(secondPassRank, firstPassRank); ++j)
	      {
		const Scalar* const B2_j = &(*B2)(0,j);
		// Teuchos::BLAS returns a magnitude_type result on
		// Scalar inputs.
		normsAfterSecondPass[j] = blas.NRM2 (std::min(secondPassRank, firstPassRank), B2_j, 1);
	      }
	    // Test whether any of the norms dropped below the
	    // reorthogonalization threshold.  If so, it's an
	    // orthogonalization fault, which requires expensive
	    // recovery.
	    reorthogFault = false;
	    for (int j = 0; j < std::min(secondPassRank, firstPassRank); ++j)
	      if (normsAfterSecondPass[j] < blockReorthogThreshold() * normsAfterFirstPass[j])
		{
		  reorthogFault = true; 
		  faultIndices.push_back (j);
		}
	  } // if (reorthogonalize) // reorthogonalization pass

	if (reorthogFault)
	  {
	    if (throwOnReorthogFault_)
	      raiseReorthogFault (normsAfterFirstPass, normsAfterSecondPass, faultIndices);
	    else 
	      {
		// FIXME We should slowly reorthogonalize, one
		// vector at a time, the offending vectors of X.
		// Instead, we just let X pass through.
		;
	      }
	  }
      } // if (reorthogonalizeBlocks_)
    return rank;
  }


  template<class Scalar, class MV>
  int 
  TsqrOrthoManagerImpl<Scalar, MV>::
  normalizeOutOfPlace (MV& X, MV& Q, mat_ptr B)
  {
    using Teuchos::Range1D;
    using Teuchos::RCP;
    using Teuchos::rcp;

#ifdef BELOS_TEUCHOS_TIME_MONITOR
    Teuchos::TimeMonitor timerMonitorNormalize(*timerNormalize_);
    // If normalize() is called internally -- i.e., called from
    // projectAndNormalize() -- the timer will not be started or 
    // stopped, because it is already running.  TimeMonitor handles
    // recursive invocation by doing nothing.
    Teuchos::TimeMonitor timerMonitorOrtho(*timerOrtho_);
#endif // BELOS_TEUCHOS_TIME_MONITOR

    const int numCols = MVT::GetNumberVecs (X);
    if (numCols == 0)
      return 0; // Fast exit for an empty input matrix
    TEST_FOR_EXCEPTION(MVT::GetNumberVecs(Q) < numCols, std::invalid_argument, 
		       "Belos::TsqrOrthoManagerImpl::normaliz"
		       "eImplOutOfPlace(...): Q has " << MVT::GetNumberVecs(Q) 
		       << " columns, which is too few, since X has " 
		       << numCols << " columns.");
    // TSQR wants a Q with the same number of columns as X, so have
    // it work on a nonconstant view of Q with the same number of
    // columns as X.
    RCP<MV> Q_view = MVT::CloneViewNonConst (Q, Range1D(0, numCols-1));

    // TSQR's rank-revealing part doesn't work unless B is provided.
    // If B is not provided, allocate a temporary B for use in TSQR.
    // If it is provided, adjust dimensions as necessary.
    if (B.is_null())
      B = rcp (new mat_type (numCols, numCols));
    else
      B->shape (numCols, numCols);
    // Compute rank-revealing decomposition (in this case, TSQR of X
    // followed by SVD of the R factor and appropriate updating of
    // the resulting Q factor) of X.  X is modified in place, and
    // Q_view contains the resulting explicit Q factor.
    //
    // The matrix *B will only be upper triangular if X is of full
    // numerical rank.
    const int rank = rawNormalize (X, *Q_view, *B);

    if (rank == numCols || ! randomizeNullSpace_) // Yay, we're done!
      return rank;
    else
      {
	//
	// If the randomize null space option is enabled, fill in the
	// last numCols-rank columns of Q_view with random vectors.
	//
	const int nullSpaceNumCols = numCols - rank; // Null space dimension
	// The columns of Q_view to fill with random data
	const Range1D nullSpaceIndices (rank, numCols-1);
	RCP<MV> Q_null = MVT::CloneViewNonConst (*Q_view, nullSpaceIndices);
	MVT::MvRandom (*Q_null); // Fill Q_null with random data

	// If the column space has dimension > 0, project the random
	// data against the column space basis of X, to ensure that
	// the random data is orthogonal to it.  We do this using a
	// simple block projection ("Block Classical Gram-Schmidt").
	// This is accurate because we've already orthogonalized the
	// column space basis of X nearly to machine precision via a
	// QR factorization (TSQR) with accuracy comparable to
	// Householder QR.
	if (rank > 0)
	  {
#ifdef BELOS_TEUCHOS_TIME_MONITOR
	    // This is a projection step; it needs to be timed as such.
	    Teuchos::TimeMonitor timerMonitorProject(*timerProject_);
#endif // BELOS_TEUCHOS_TIME_MONITOR
	    // Column space basis (already orthogonalized)
	    RCP<const MV> Q_col = MVT::CloneView (*Q_view, Range1D(0, rank-1));
		
	    // Make temporary storage for the projection
	    // coefficients.  We don't need to keep the
	    // coefficients, since the random vectors are replacing
	    // the null space basis of X.  (The null space basis is
	    // just biased noise, and it's better to replace biased
	    // noise with unbiased noise.)
	    mat_type C_colnull (rank, nullSpaceNumCols);
	    // C_colnull := <Q_col, Q_null>
	    innerProd (*Q_col, *Q_null, C_colnull);
	    // Q_null := Q_null - Q_col * C_colnull
	    MVT::MvTimesMatAddMv (-SCT::one(), *Q_col, C_colnull, SCT::one(), *Q_null);
	  }
	//
	// Normalize the projected random vectors.
	//
	// Rank of the null space candidate basis. 
	// We will (re)set this in the "try" block below.
	int nullSpaceBasisRank = 0; 
	try {
	  // We use (part of) X here as scratch space, since TSQR
	  // can't compute the explicit Q in place.
	  RCP<MV> X_null = MVT::CloneViewNonConst (X, Range1D(rank, numCols-1));
	  // We don't need to keep the normalization coefficients,
	  // but TSQR needs somewhere to put them.
	  mat_type B_null (nullSpaceNumCols, nullSpaceNumCols);
	  // Normalize the null space candidate basis, putting
	  // results in X_null.
	  nullSpaceBasisRank = rawNormalize (*Q_null, *X_null, B_null);
	  // Copy the above results back into Q_null.
	  MVT::Assign (*Q_null, *X_null);
	} catch (std::exception& e) {
	  throw TsqrOrthoError (e.what()); // Toss the exception up the chain.
	}
	// It's possible, but unlikely, that Q_null doesn't have
	// full rank.  We could recursively fill in more random
	// vectors until we finally get a full rank matrix, but
	// instead we just throw an exception.
	//
	// FIXME (mfh 08 Nov 2010, 11 Jan 2011) Perhaps we should
	// deal with this case more elegantly.  Recursion might be
	// one way to solve it, but be sure to check that the
	// recursion will terminate.  We could do this by supplying
	// an additional argument to rawNormalize, which is the null
	// space basis rank from the previous iteration.  The rank
	// has to decrease each time, or the recursion may go on
	// forever.
	TEST_FOR_EXCEPTION(nullSpaceBasisRank < nullSpaceNumCols,
			   TsqrOrthoError,
			   "Random vectors after projection have rank " 
			   << nullSpaceBasisRank << ", but should have rank "
			   << nullSpaceNumCols);
	return rank;
      }
  }



  template<class Scalar, class MV>
  int 
  TsqrOrthoManagerImpl<Scalar, MV>::
  projectAndNormalizeOutOfPlace (MV& X_in, 
				 MV& X_out,
				 Teuchos::Array<mat_ptr> C,
				 mat_ptr B,
				 Teuchos::ArrayView<Teuchos::RCP<const MV> > Q)
  {
    using Teuchos::Range1D;
    using Teuchos::RCP;
    using Teuchos::rcp;

#ifdef BELOS_TEUCHOS_TIME_MONITOR
    Teuchos::TimeMonitor timerMonitorOrtho(*timerOrtho_);
#endif // BELOS_TEUCHOS_TIME_MONITOR

    // Fetch dimensions of X_in and Q.
    int nrows_X, ncols_X, num_Q_blocks, ncols_Q_total;
    checkProjectionDims (nrows_X, ncols_X, num_Q_blocks, ncols_Q_total, X_in, Q);

    // Make sure that X_out has at least as many columns as X_in.
    TEST_FOR_EXCEPTION(MVT::GetNumberVecs(X_out) < ncols_X, 
		       std::invalid_argument, 
		       "Belos::TsqrOrthoManagerImpl::projectAndNormalizeOutOfPlace("
		       "...): X_out has " << MVT::GetNumberVecs(X_out) << " col"
		       "umns, but X_in has " << ncols_X << " columns.");

    // Test for quick exit: any dimension of X is zero.
    if (nrows_X == 0 || ncols_X == 0)
      return 0;

    // If there are zero Q blocks or zero Q columns, just normalize!
    if (num_Q_blocks == 0 || ncols_Q_total == 0)
      return normalizeOutOfPlace (X_in, X_out, B);

    // Make space for projection coefficients (first and second passes)
    allocateProjectionCoefficients (C, Q, X_in, true);
    Teuchos::Array<mat_ptr> newC;
    allocateProjectionCoefficients (newC, Q, X_in, false);

    // If we are doing block reorthogonalization, then compute the
    // column norms of X before projecting for the first time.  This
    // will help us decide whether we need to reorthogonalize X.
    std::vector<magnitude_type> normsBeforeFirstPass (ncols_X, magnitude_type(0));
    if (reorthogonalizeBlocks_)
      MVT::MvNorm (X_in, normsBeforeFirstPass);

    // First (Modified) Block Gram-Schmidt pass:
    // C := Q^* X_in
    // X_in := X_in - Q C
    rawProject (X_in, Q, C);

    // Before normalizing, be sure to allocate a new B matrix here.
    // If we let the normalize() routine allocate, that storage will
    // go away at the end of normalize().
    if (B.is_null())
      B = rcp (new mat_type (ncols_X, ncols_X));

    // Rank of X_in after first projection pass.
    // This overwrites X_in with invalid values.
    const int firstPassRank = normalizeOutOfPlace (X_in, X_out, B);
    // Current rank of X_out (we overwrote X_in above).
    int rank = firstPassRank;

    // If X_in was not full rank after projection, replace its null
    // space basis with random data, and project and normalize the
    // random data.
    if (firstPassRank < ncols_X && randomizeNullSpace_)
      {
	// normalizeOutOfPlace() has already replaced the null space basis
        // with random vectors, and orthogonalized those against the
        // already orthogonalized column space basis of X.  Now we
        // need to project the new random vectors against the Q
        // blocks, and renormalize them.

	// Make a view of the null space basis columns of X_out.
	const int numNullSpaceCols = ncols_X - firstPassRank;
	const Range1D nullSpaceIndices (firstPassRank, ncols_X - 1);
	RCP<MV> X_out_null = MVT::CloneViewNonConst (X_out, nullSpaceIndices);

	// We don't need X_in anymore, so use it for scratch space.
	// Copy X_out_null into the last few columns of X_in
	// (X_in_null) and do projections in there.
	RCP<MV> X_in_null = MVT::CloneViewNonConst (X_in, nullSpaceIndices);
	MVT::Assign (*X_out_null, *X_in_null);

	// Space for projection coefficients (will be thrown away)
	Teuchos::Array<mat_ptr> C_null (num_Q_blocks);
	for (int k = 0; k < num_Q_blocks; ++k)
	  C_null[k] = rcp (new mat_type (MVT::GetNumberVecs(*Q[k]), numNullSpaceCols));
	// Project the new random vectors against the Q blocks
	rawProject (*X_in_null, Q, C_null);

	// (Re)normalize the new random vectors after the projection.
	// First, make space for the normalization coefficients (will
	// be thrown away).
	RCP<mat_type> B_null (new mat_type (numNullSpaceCols, numNullSpaceCols));
	// Normalize the random vectors, putting the results in X_out_null
	const int randomVectorsRank = normalizeOutOfPlace (*X_in_null, *X_out_null, B_null);

	// While unusual, it is still possible for the random data not
	// to be full rank after projection and normalization.  In
	// that case, we could try another set of random data and
	// recurse as necessary, but instead for now we just raise an
	// exception.
	TEST_FOR_EXCEPTION(randomVectorsRank != numNullSpaceCols, 
			   TsqrOrthoError, 
			   "Belos::TsqrOrthoManagerImpl::projectAndNormalize"
			   "OutOfPlace(): After projecting and normalizing the "
			   "random vectors (used to replace the null space "
			   "basis vectors from normalizing X), they have rank "
			   << randomVectorsRank << ", but should have full "
			   "rank " << numNullSpaceCols << ".");
      }

    // Whether or not X_in was full rank after projection, we still
    // might want to reorthogonalize against Q.
    if (reorthogonalizeBlocks_)
      {
	// We are only interested in the column space basis of X_out.
	std::vector<magnitude_type> normsAfterFirstPass (firstPassRank, magnitude_type(0));
	std::vector<magnitude_type> normsAfterSecondPass (firstPassRank, magnitude_type(0));

	// Compute post-first-pass (pre-normalization) norms.  We
	// could have done that using MVT::MvNorm() on X_in after
	// projecting, but before the first normalization.  However,
	// However, that operation may be expensive.  It is also
	// unnecessary: after calling normalize(), the 2-norm of
	// B(:,j) is the 2-norm of X_in(:,j) before normalization, in
	// exact arithmetic.
	//
	// NOTE (mfh 06 Nov 2010) This is one way that combining
	// projection and normalization into a single kernel --
	// projectAndNormalize() -- pays off.  In project(), we have
	// to compute column norms of X before and after projection.
	// Here, we get them for free from the normalization
	// coefficients.
	Teuchos::BLAS<int, Scalar> blas;
	for (int j = 0; j < firstPassRank; ++j)
	  {
	    const Scalar* const B_j = &(*B)(0,j);
	    // Teuchos::BLAS returns a magnitude_type result on
	    // Scalar inputs.
	    normsAfterFirstPass[j] = blas.NRM2 (firstPassRank, B_j, 1);
	  }
	// Test whether any of the norms dropped below the
	// reorthogonalization threshold.
	bool reorthogonalize = false;
	for (int j = 0; j < firstPassRank; ++j)
	  // If any column's norm decreased too much, mark this block
	  // for reorthogonalization.  Note that this test will _not_
	  // activate reorthogonalization if a column's norm before
	  // the first project-and-normalize step was zero.  It _will_
	  // activate reorthogonalization if the column's norm before
	  // was not zero, but is zero now.
	  if (normsAfterFirstPass[j] < blockReorthogThreshold() * normsBeforeFirstPass[j])
	    {
	      reorthogonalize = true; 
	      break;
	    }

	// Perform another Block Gram-Schmidt pass if necessary.
	// "Twice is enough" (Kahan's theorem) for a Krylov method,
	// unless (using Stewart's term) there is an
	// "orthogonalization fault" (indicated by reorthogFault).
	//
	// FIXME (mfh 07 Nov 2010) For now, we include the entire
	// block of X, including possible random data (that was
	// already projected and normalized above).  It might make
	// more sense just to process the first firstPassRank columns
	// of X.  However, the resulting reorthogonalization should
	// still be correct regardless.
	bool reorthogFault = false;
	// Indices of X at which there was an orthogonalization fault.
	std::vector<int> faultIndices;
	if (reorthogonalize)
	  {
	    using Teuchos::Copy;
	    using Teuchos::NO_TRANS;

	    // Copy X_out (results of first pass) into X_in, for
	    // reorthogonalization.
	    MVT::Assign (X_out, X_in);

	    // Block Gram-Schmidt (again).  Delay updating the block
	    // coefficients until we have the new normalization
	    // coefficients, which we need in order to do the update.
	    rawProject (X_in, Q, newC);
	    
	    // Coefficients for (re)normalization of X_in
	    RCP<mat_type> B2 (new mat_type (ncols_X, ncols_X));
	    // Normalize X_in into X_out
	    const int secondPassRank = normalizeOutOfPlace (X_in, X_out, B2);
	    rank = secondPassRank;

	    // Update normalization coefficients.  We begin with
	    // copying B, since the BLAS doesn't like aliased input
	    // arguments.
	    mat_type B_copy (Copy, *B, B->numRows(), B->numCols());
	    // B := B2 * B
	    if (0 != B->multiply (NO_TRANS, NO_TRANS, SCT::one(), *B2, B_copy, SCT::zero()))
	      throw std::logic_error ("Should never get here!");

	    // Update the block coefficients from the projection step.
	    for (int k = 0; k < num_Q_blocks; ++k)
	      {
		mat_type C_k_copy (Copy, *C[k], C[k]->numRows(), C[k]->numCols());
		// C[k] := newC[k]*B_copy + C[k] (where B_copy is
		// the original B, before the second call to
		// normalize).
		if (0 != C[k]->multiply (NO_TRANS, NO_TRANS, SCT::one(), *newC[k], B_copy, SCT::one()))
		  throw std::logic_error ("Should never get here!");
	      }

	    // Compute post-second-pass (pre-normalization) norms,
	    // using B2 (the coefficients from the second normalize()
	    // invocation) in the same way as with B before.
	    for (int j = 0; j < std::min(secondPassRank, firstPassRank); ++j)
	      {
		const Scalar* const B2_j = &(*B2)(0,j);
		// Teuchos::BLAS returns a magnitude_type result on
		// Scalar inputs.
		normsAfterSecondPass[j] = blas.NRM2 (std::min(secondPassRank, firstPassRank), B2_j, 1);
	      }
	    // Test whether any of the norms dropped below the
	    // reorthogonalization threshold.  If so, it's an
	    // orthogonalization fault, which requires expensive
	    // recovery.
	    reorthogFault = false;
	    for (int j = 0; j < std::min(secondPassRank, firstPassRank); ++j)
	      if (normsAfterSecondPass[j] < blockReorthogThreshold() * normsAfterFirstPass[j])
		{
		  reorthogFault = true; 
		  faultIndices.push_back (j);
		}
	  } // if (reorthogonalize) // reorthogonalization pass

	if (reorthogFault)
	  {
	    if (throwOnReorthogFault_)
	      raiseReorthogFault (normsAfterFirstPass, normsAfterSecondPass, faultIndices);
	    else 
	      {
		// FIXME We should slowly reorthogonalize, one
		// vector at a time, the offending vectors of X.
		// Instead, we just let X pass through.
		;
	      }
	  }
      } // if (reorthogonalizeBlocks_)
    return rank;
  }


  template<class Scalar, class MV>
  void
  TsqrOrthoManagerImpl<Scalar, MV>::
  allocateProjectionCoefficients (Teuchos::Array<mat_ptr>& C, 
				  Teuchos::ArrayView<Teuchos::RCP<const MV> > Q,
				  const MV& X,
				  const bool attemptToRecycle) const
  {
    const int num_Q_blocks = Q.size();
    const int ncols_X = MVT::GetNumberVecs (X);
    C.resize (num_Q_blocks);
    if (attemptToRecycle)
      {
	for (int i = 0; i < num_Q_blocks; ++i) 
	  {
	    const int ncols_Qi = MVT::GetNumberVecs (*Q[i]);
	    // Create a new C[i] if necessary, otherwise resize if
	    // necessary, otherwise fill with zeros.
	    if (C[i].is_null())
	      C[i] = rcp (new mat_type (ncols_Qi, ncols_X));
	    else 
	      {
		mat_type& Ci = *C[i];
		if (Ci.numRows() != ncols_Qi || Ci.numCols() != ncols_X)
		  Ci.shape (ncols_Qi, ncols_X);
		else
		  Ci.putScalar (SCT::zero());
	      }
	  }
      }
    else
      {
	for (int i = 0; i < num_Q_blocks; ++i) 
	  {
	    const int ncols_Qi = MVT::GetNumberVecs (*Q[i]);
	    C[i] = rcp (new mat_type (ncols_Qi, ncols_X));
	  }
      }
  }



  template<class Scalar, class MV>
  int 
  TsqrOrthoManagerImpl<Scalar, MV>::
  projectAndNormalizeImpl (MV& X_in, 
			   MV& X_out, // Only written if outOfPlace==false.
			   const bool outOfPlace,
			   Teuchos::Array<mat_ptr> C,
			   mat_ptr B,
			   Teuchos::ArrayView<Teuchos::RCP<const MV> > Q)
  {
    using Teuchos::Range1D;
    using Teuchos::RCP;
    using Teuchos::rcp;

#ifdef BELOS_TEUCHOS_TIME_MONITOR
    Teuchos::TimeMonitor timerMonitorOrtho(*timerOrtho_);
#endif // BELOS_TEUCHOS_TIME_MONITOR

    if (outOfPlace)
      { // Make sure that X_out has at least as many columns as X_in.
	TEST_FOR_EXCEPTION(MVT::GetNumberVecs(X_out) < MVT::GetNumberVecs(X_in),
			   std::invalid_argument, 
			   "Belos::TsqrOrthoManagerImpl::"
			   "projectAndNormalizeOutOfPlace(...):"
			   "X_out has " << MVT::GetNumberVecs(X_out) 
			   << " columns, but X_in has "
			   << MVT::GetNumberVecs(X_in) << " columns.");
      }
    // Fetch dimensions of X_in and Q, and allocate space for first-
    // and second-pass projection coefficients (C resp. C2).
    int nrows_X, ncols_X, num_Q_blocks, ncols_Q_total;
    checkProjectionDims (nrows_X, ncols_X, num_Q_blocks, ncols_Q_total, X_in, Q);

    // Test for quick exit: any dimension of X is zero.
    if (ncols_X == 0)
      return 0;
    // If there are zero Q blocks or zero Q columns, just normalize!
    if (num_Q_blocks == 0 || ncols_Q_total == 0)
      {
	if (outOfPlace)
	  return normalizeOutOfPlace (X_in, X_out, B);
	else
	  return normalize (X_in, B);
      }

    // The typical case is that the entries of C have been allocated
    // before, so we attempt to recycle the allocations.  The call
    // below will be correct regardless.
    allocateProjectionCoefficients (C, Q, X_in, true);

    // If we are doing block reorthogonalization, then compute the
    // column norms of X before projecting for the first time.  This
    // will help us decide whether we need to reorthogonalize X.
    std::vector<magnitude_type> normsBeforeFirstPass (ncols_X, SCTM::zero());
    if (reorthogonalizeBlocks_)
      MVT::MvNorm (X_in, normsBeforeFirstPass);

    // First (Modified) Block Gram-Schmidt pass, in place in X_in
    rawProject (X_in, Q, C);

    // Before normalizing, be sure to allocate a new B matrix here.
    // If we let the normalize() routine allocate, that storage will
    // go away at the end of normalize().  (This is because it passes
    // the RCP by value, not by reference.)
    if (B.is_null())
      B = rcp (new mat_type (ncols_X, ncols_X));

    // Rank of X(_in) after first projection pass.  If outOfPlace,
    // this overwrites X_in with invalid values, and the results go in
    // X_out.  Otherwise, it's done in place in X_in.
    const int firstPassRank = outOfPlace ? 
      normalizeOutOfPlace (X_in, X_out, B) : 
      normalize (X_in, B);
    int rank = firstPassRank; // Current rank of X

    // If X was not full rank after projection and randomizeNullSpace_
    // is true, then normalize(OutOfPlace)() replaced the null space
    // basis of X with random vectors, and orthogonalized them against
    // the column space basis of X.  However, we still need to
    // orthogonalize the random vectors against the Q[i], after which
    // we will need to renormalize them.
    //
    // If outOfPlace, then we need to work in X_out (where
    // normalizeOutOfPlace() wrote the normalized vectors).
    // Otherwise, we need to work in X_in.
    //
    // Note: We don't need to keep the new projection coefficients,
    // since they are multiplied by the "small" part of B
    // corresponding to the null space of the original X.
    if (firstPassRank < ncols_X && randomizeNullSpace_)
      {
	const int numNullSpaceCols = ncols_X - firstPassRank;
	const Range1D nullSpaceIndices (firstPassRank, ncols_X - 1);

	// Space for projection coefficients (will be thrown away)
	Teuchos::Array<mat_ptr> C_null (num_Q_blocks);
	for (int k = 0; k < num_Q_blocks; ++k)
	  {
	    const int numColsQk = MVT::GetNumberVecs(*Q[k]);
	    C_null[k] = rcp (new mat_type (numColsQk, numNullSpaceCols));
	  }
	// Space for normalization coefficients (will be thrown away).
	RCP<mat_type> B_null (new mat_type (numNullSpaceCols, numNullSpaceCols));

	int randomVectorsRank;
	if (outOfPlace)
	  {
	    // View of the null space basis columns of X.
	    // normalizeOutOfPlace() wrote them into X_out.
	    RCP<MV> X_out_null = MVT::CloneViewNonConst (X_out, nullSpaceIndices);
	    // Use X_in for scratch space.  Copy X_out_null into the
	    // last few columns of X_in (X_in_null) and do projections
	    // in there.  (This saves us a copy wen we renormalize
	    // (out of place) back into X_out.)
	    RCP<MV> X_in_null = MVT::CloneViewNonConst (X_in, nullSpaceIndices);
	    MVT::Assign (*X_out_null, *X_in_null);
	    // Project the new random vectors against the Q blocks, and
	    // renormalize the result into X_out_null.  
	    rawProject (*X_in_null, Q, C_null);
	    randomVectorsRank = normalizeOutOfPlace (*X_in_null, *X_out_null, B_null);
	  }
	else
	  {
	    // View of the null space columns of X.  
	    // They live in X_in.
	    RCP<MV> X_null = MVT::CloneViewNonConst (X_in, nullSpaceIndices);
	    // Project the new random vectors against the Q blocks,
	    // and renormalize the result (in place).
	    rawProject (*X_null, Q, C_null);
	    randomVectorsRank = normalize (*X_null, B_null);
	  }
	// While unusual, it is still possible for the random data not
	// to be full rank after projection and normalization.  In
	// that case, we could try another set of random data and
	// recurse as necessary, but instead for now we just raise an
	// exception.
	TEST_FOR_EXCEPTION(randomVectorsRank != numNullSpaceCols, 
			   TsqrOrthoError, 
			   "Belos::TsqrOrthoManagerImpl::projectAndNormalize"
			   "OutOfPlace(): After projecting and normalizing the "
			   "random vectors (used to replace the null space "
			   "basis vectors from normalizing X), they have rank "
			   << randomVectorsRank << ", but should have full "
			   "rank " << numNullSpaceCols << ".");
      }

    // Whether or not X_in was full rank after projection, we still
    // might want to reorthogonalize against Q.
    if (reorthogonalizeBlocks_)
      {
	// We are only interested in the column space basis of X
	// resp. X_out.
	std::vector<magnitude_type> normsAfterFirstPass (firstPassRank, magnitude_type(0));
	std::vector<magnitude_type> normsAfterSecondPass (firstPassRank, magnitude_type(0));

	// Compute post-first-pass (pre-normalization) norms.  We
	// could have done that using MVT::MvNorm() on X_in after
	// projecting, but before the first normalization.  However,
	// that operation may be expensive.  It is also unnecessary:
	// after calling normalize(), the 2-norm of B(:,j) is the
	// 2-norm of X_in(:,j) before normalization, in exact
	// arithmetic.
	//
	// NOTE (mfh 06 Nov 2010) This is one way that combining
	// projection and normalization into a single kernel --
	// projectAndNormalize() -- pays off.  In project(), we have
	// to compute column norms of X before and after projection.
	// Here, we get them for free from the normalization
	// coefficients.
	Teuchos::BLAS<int, Scalar> blas;
	for (int j = 0; j < firstPassRank; ++j)
	  {
	    const Scalar* const B_j = &(*B)(0,j);
	    // Teuchos::BLAS returns a magnitude_type result on
	    // Scalar inputs.
	    normsAfterFirstPass[j] = blas.NRM2 (firstPassRank, B_j, 1);
	  }
	// Test whether any of the norms dropped below the
	// reorthogonalization threshold.
	bool reorthogonalize = false;
	for (int j = 0; j < firstPassRank; ++j)
	  // If any column's norm decreased too much, mark this block
	  // for reorthogonalization.  Note that this test will _not_
	  // activate reorthogonalization if a column's norm before
	  // the first project-and-normalize step was zero.  It _will_
	  // activate reorthogonalization if the column's norm before
	  // was not zero, but is zero now.
	  if (normsAfterFirstPass[j] < blockReorthogThreshold() * normsBeforeFirstPass[j])
	    {
	      reorthogonalize = true; 
	      break;
	    }

	// Perform another Block Gram-Schmidt pass if necessary.
	// "Twice is enough" (Kahan's theorem) for a Krylov method,
	// unless (using Stewart's term) there is an
	// "orthogonalization fault" (indicated by reorthogFault).
	//
	// FIXME (mfh 07 Nov 2010) For now, we include the entire
	// block of X, including possible random data (that was
	// already projected and normalized above).  It might make
	// more sense just to process the first firstPassRank columns
	// of X.  However, the resulting reorthogonalization should
	// still be correct regardless.
	bool reorthogFault = false;
	// Indices of X at which there was an orthogonalization fault.
	std::vector<int> faultIndices;
	if (reorthogonalize)
	  {
	    using Teuchos::Copy;
	    using Teuchos::NO_TRANS;

	    // If we're using out-of-place normalization, copy X_out
	    // (results of first project and normalize pass) back into
	    // X_in, for the second project and normalize pass.
	    if (outOfPlace)
	      MVT::Assign (X_out, X_in);

	    // C2 is only used internally, so we know that we are
	    // allocating fresh and not recycling allocations.
	    Teuchos::Array<mat_ptr> C2;
	    allocateProjectionCoefficients (C2, Q, X_in, false);

	    // Block Gram-Schmidt (again).  Delay updating the block
	    // coefficients until we have the new normalization
	    // coefficients, which we need in order to do the update.
	    rawProject (X_in, Q, C2);
	    
	    // Coefficients for (re)normalization of X_in
	    RCP<mat_type> B2 (new mat_type (ncols_X, ncols_X));

	    // Normalize X_in (into X_out, if working out of place)
	    const int secondPassRank = outOfPlace ? 
	      normalizeOutOfPlace (X_in, X_out, B2) : 
	      normalize (X_in, B2);
	    rank = secondPassRank; // Current rank of X

	    // Update normalization coefficients.  We begin with
	    // copying B, since the BLAS doesn't like aliased input
	    // arguments.
	    mat_type B_copy (Copy, *B, B->numRows(), B->numCols());
	    // B := B2 * B
	    const int err = B->multiply (NO_TRANS, NO_TRANS, SCT::one(), 
					 *B2, B_copy, SCT::zero());
	    TEST_FOR_EXCEPTION(err != 0, std::logic_error, 
			       "Teuchos::SerialDenseMatrix::multiply "
			       "returned err = " << err << " != 0");
	    // Update the block coefficients from the projection step.
	    // We use B_copy for this (a copy of the original B, the
	    // first-pass normalization coefficients).
	    for (int k = 0; k < num_Q_blocks; ++k)
	      {
		mat_type C_k_copy (Copy, *C[k], C[k]->numRows(), C[k]->numCols());

		// C[k] := C2[k]*B_copy + C[k] (where B_copy is a copy
		// of the original B, the first-pass normalization
		// coefficients).
		const int err = C[k]->multiply (NO_TRANS, NO_TRANS, SCT::one(), 
						*C2[k], B_copy, SCT::one());
		TEST_FOR_EXCEPTION(err != 0, std::logic_error, 
				   "Teuchos::SerialDenseMatrix::multiply "
				   "returned err = " << err << " != 0");
	      }
	    // Compute post-second-pass (pre-normalization) norms,
	    // using B2 (the coefficients from the second normalize()
	    // invocation) in the same way as with B before.
	    for (int j = 0; j < rank; ++j)
	      {
		const Scalar* const B2_j = &(*B2)(0,j);
		normsAfterSecondPass[j] = blas.NRM2 (rank, B2_j, 1);
	      }
	    // Test whether any of the norms dropped below the
	    // reorthogonalization threshold.  If so, it's an
	    // orthogonalization fault, which requires expensive
	    // recovery.
	    reorthogFault = false;
	    for (int j = 0; j < rank; ++j)
	      {
		const magnitude_type relativeLowerBound = 
		  blockReorthogThreshold() * normsAfterFirstPass[j];
		if (normsAfterSecondPass[j] < relativeLowerBound)
		  {
		    reorthogFault = true; 
		    faultIndices.push_back (j);
		  }
	      }
	  } // if (reorthogonalize) // reorthogonalization pass

	if (reorthogFault)
	  {
	    if (throwOnReorthogFault_)
	      raiseReorthogFault (normsAfterFirstPass, normsAfterSecondPass, faultIndices);
	    else 
	      {
		// FIXME (mfh 19 Jan 2011)
		//
		// We should slowly reorthogonalize, one vector at a
		// time, the offending vectors of X.
		TEST_FOR_EXCEPTION(true, std::logic_error, 
				   "TsqrOrthoManagerImpl has not yet implemented"
				   " recovery from an orthogonalization fault.");
	      }
	  }
      } // if (reorthogonalizeBlocks_)
    return rank;
  }


  template<class Scalar, class MV>
  void
  TsqrOrthoManagerImpl<Scalar, MV>::
  raiseReorthogFault (const std::vector<magnitude_type>& normsAfterFirstPass,
		      const std::vector<magnitude_type>& normsAfterSecondPass,
		      const std::vector<int>& faultIndices)
  {
    using std::endl;
    typedef std::vector<int>::size_type size_type;
    std::ostringstream os;
		  
    os << "Orthogonalization fault at the following column(s) of X:" << endl;
    os << "Column\tNorm decrease factor" << endl;
    for (size_type k = 0; k < faultIndices.size(); ++k)
      {
	const int index = faultIndices[k];
	const magnitude_type decreaseFactor = 
	  normsAfterSecondPass[index] / normsAfterFirstPass[index];
	os << index << "\t" << decreaseFactor << endl;
      }
    throw TsqrOrthoFault (os.str());
  }

  template<class Scalar, class MV>
  Teuchos::RCP<const Teuchos::ParameterList>
  TsqrOrthoManagerImpl<Scalar, MV>::getDefaultParameters ()
  {
    using Teuchos::ParameterList;
    using Teuchos::RCP;
    typedef Teuchos::ScalarTraits<magnitude_type> SCTM;

    // This part makes this class method non-reentrant.
    static RCP<ParameterList> params;
    if (! params.is_null())
      return params;
    params = Teuchos::parameterList();

    // 
    // TSQR parameters
    //
    params->set ("TsqrImpl", tsqr_adaptor_type::getDefaultParameters(), 
		 "TSQR implementation parameters.");
    // 
    // Orthogonalization parameters
    //
    const bool defaultRandomizeNullSpace = true;
    params->set ("randomizeNullSpace", defaultRandomizeNullSpace, 
		 "Whether to fill in null space vectors with random data.");
    //const bool defaultReorthogonalizeBlocks = false;
    const bool defaultReorthogonalizeBlocks = true;
    params->set ("reorthogonalizeBlocks", defaultReorthogonalizeBlocks,
		 "Whether to do block reorthogonalization.");
    // This parameter corresponds to the "blk_tol_" parameter in
    // Belos' DGKSOrthoManager.  We choose the same default value.
    const magnitude_type defaultBlockReorthogThreshold = 
      magnitude_type(10) * SCTM::squareroot (SCTM::eps());
    params->set ("blockReorthogThreshold", defaultBlockReorthogThreshold, 
		 "If reorthogonalizeBlocks==true, and if the norm of "
		 "any column within a block decreases by this much or "
		 "more after orthogonalization, we reorthogonalize.");
    // This parameter corresponds to the "sing_tol_" parameter in
    // Belos' DGKSOrthoManager.  We choose the same default value.
    const magnitude_type defaultRelativeRankTolerance = 
      magnitude_type(10) * SCTM::eps();
    // If the relative rank tolerance is zero, then we will always
    // declare blocks to be numerically full rank, as long as no
    // singular values are zero.
    params->set ("relativeRankTolerance", defaultRelativeRankTolerance,
		 "Relative tolerance to determine the numerical rank of a "
		 "block when normalizing.");
    const bool defaultThrowOnReorthogFault = true;
    // See Stewart's 2008 paper on block Gram-Schmidt for a
    // definition of "orthogonalization fault."
    params->set ("throwOnReorthogFault", defaultThrowOnReorthogFault,
		 "Whether to throw an exception if an orthogonalization "
		 "fault occurs.  This only matters if reorthogonalization "
		 "is enabled (reorthogonalizeBlocks==true).");
    return params;
  }


  template<class Scalar, class MV>
  Teuchos::RCP<const Teuchos::ParameterList>
  TsqrOrthoManagerImpl<Scalar, MV>::getFastParameters ()
  {
    using Teuchos::ParameterList;
    using Teuchos::RCP;
    using Teuchos::rcp;

    // This part makes this class method non-reentrant.
    static RCP<ParameterList> params;
    if (params.is_null())
      {
	RCP<const ParameterList> defaultParams = getDefaultParameters();
	// Start with a clone of the default parameters
	params = rcp (new ParameterList (*defaultParams));
	
	// Disable reorthogonalization and randomization of the null
	// space basis.  Reorthogonalization tolerances don't matter,
	// since we aren't reorthogonalizing blocks in the fast
	// settings.  We can leave the default values.  Also,
	// (re)orthogonalization faults may only occur with
	// reorthogonalization, so we don't have to worry about the
	// "throwOnReorthogFault" setting.
	const bool randomizeNullSpace = false;
	params->set ("randomizeNullSpace", randomizeNullSpace);      
	const bool reorthogonalizeBlocks = false;
	params->set ("reorthogonalizeBlocks", reorthogonalizeBlocks);
      }
    return params;
  }


} // namespace Belos

#endif // __BelosTsqrOrthoManagerImpl_hpp

