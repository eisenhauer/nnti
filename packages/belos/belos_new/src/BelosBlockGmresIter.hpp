// @HEADER
// ***********************************************************************
//
//                 Belos: Block Linear Solvers Package
//                 Copyright (2004) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
// @HEADER

#ifndef BELOS_BLOCK_GMRES_ITER_HPP
#define BELOS_BLOCK_GMRES_ITER_HPP

/*! \file BelosBlockGmresIter.hpp
    \brief Belos concrete class for performing the block GMRES iteration..
*/

#include "BelosConfigDefs.hpp"
#include "BelosTypes.hpp"
#include "BelosIteration.hpp"

#include "BelosLinearProblem.hpp"
#include "BelosOrthoManager.hpp"
#include "BelosOutputManager.hpp"
#include "BelosStatusTest.hpp"
#include "BelosOperatorTraits.hpp"
#include "BelosMultiVecTraits.hpp"

#include "Teuchos_BLAS.hpp"
#include "Teuchos_LAPACK.hpp"
#include "Teuchos_SerialDenseMatrix.hpp"
#include "Teuchos_SerialDenseVector.hpp"
#include "Teuchos_ScalarTraits.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_TimeMonitor.hpp"

/*!	
  \class Belos::BlockGmresIter
  
  \brief This class implements the block GMRES iteration, where a
  block Krylov subspace is constructed.  The QR decomposition of 
  block, upper Hessenberg matrix is performed each iteration to update
  the least squares system and give the current linear system residuals.
 
  \author Teri Barth and Heidi Thornquist
*/

namespace Belos {
  
  //! @name BlockGmresIter Structures 
  //@{ 
  
  /** \brief Structure to contain pointers to BlockGmresIter state variables.
   *
   * This struct is utilized by BlockGmresIter::initialize() and BlockGmresIter::getState().
   */
  template <class ScalarType, class MV>
  struct BlockGmresIterState {
    /*! \brief The current dimension of the reduction.
     *
     * This should always be equal to BlockGmresIter::getCurSubspaceDim()
     */
    int curDim;
    /*! \brief The current Krylov basis. */
    Teuchos::RefCountPtr<const MV> V;
    /*! \brief The current Hessenberg matrix. 
     *
     * The \c curDim by \c curDim leading submatrix of H is the 
     * projection of problem->getOperator() by the first \c curDim vectors in V. 
     */
    Teuchos::RefCountPtr<const Teuchos::SerialDenseMatrix<int,ScalarType> > H;
    /*! \brief The current upper-triangular matrix from the QR reduction of H. */
    Teuchos::RefCountPtr<const Teuchos::SerialDenseMatrix<int,ScalarType> > R;
    /*! \brief The current right-hand side of the least squares system RY = Z. */
    Teuchos::RefCountPtr<const Teuchos::SerialDenseMatrix<int,ScalarType> > Z;
    
    BlockGmresIterState() : curDim(0), V(Teuchos::null),
			    H(Teuchos::null), R(Teuchos::null), 
			    Z(Teuchos::null)
    {}
  };
  
  //! @name BlockGmresIter Exceptions
  //@{ 
  
  /** \brief BlockGmresIterInitFailure is thrown when the BlockGmresIter object is unable to
   * generate an initial iterate in the BlockGmresIter::initialize() routine. 
   *
   * This exception is thrown from the BlockGmresIter::initialize() method, which is
   * called by the user or from the BlockGmresIter::iterate() method if isInitialized()
   * == \c false.
   *
   * In the case that this exception is thrown, 
   * BlockGmresIter::isInitialized() will be \c false and the user will need to provide
   * a new initial iterate to the iteration.
   *
   */
  class BlockGmresIterInitFailure : public BelosError {public:
    BlockGmresIterInitFailure(const std::string& what_arg) : BelosError(what_arg)
    {}};

  /** \brief BlockGmresIterOrthoFailure is thrown when the orthogonalization manager is
   * unable to generate orthonormal columns from the new basis vectors.
   *
   * This exception is thrown from the BlockGmresIter::iterate() method.
   *
   */
  class BlockGmresIterOrthoFailure : public BelosError {public:
    BlockGmresIterOrthoFailure(const std::string& what_arg) : BelosError(what_arg)
    {}};
  
  //@}


template<class ScalarType, class MV, class OP>
class BlockGmresIter : virtual public Iteration<ScalarType,MV,OP> {

  public:
    
  //
  // Convenience typedefs
  //
  typedef MultiVecTraits<ScalarType,MV> MVT;
  typedef OperatorTraits<ScalarType,MV,OP> OPT;
  typedef Teuchos::ScalarTraits<ScalarType> SCT;
  typedef typename SCT::magnitudeType MagnitudeType;

  //! @name Constructors/Destructor
  //@{ 

  /*! \brief %BlockGmresIter constructor with eigenproblem, solver utilities, and parameter list of solver options.
   *
   * This constructor takes pointers required by the eigensolver, in addition
   * to a parameter list of options for the eigensolver. These options include the following:
   *   - "Block Size" - an \c int specifying the block size used by the algorithm. This can also be specified using the setBlockSize() method. Default: 1
   *   - "Num Blocks" - an \c int specifying the maximum number of blocks allocated for the solver basis. Default: 25
   *   - "Restart Timers" = a \c bool specifying whether the timers should be restarted each time iterate() is called. Default: false
   *   - "Keep Hessenberg" = a \c bool specifying whether the upper Hessenberg should be stored separately from the least squares system. Default: false
   */
  BlockGmresIter( const Teuchos::RefCountPtr<LinearProblem<ScalarType,MV,OP> > &problem, 
		  const Teuchos::RefCountPtr<OutputManager<ScalarType> > &printer,
		  const Teuchos::RefCountPtr<StatusTest<ScalarType,MV,OP> > &tester,
		  const Teuchos::RefCountPtr<OrthoManager<ScalarType,MV> > &ortho,
		  const Teuchos::RefCountPtr<Teuchos::ParameterList> &params );

  //! Destructor.
  virtual ~BlockGmresIter() {};
  //@}


  //! @name Solver methods
  //@{ 
  
  /*! \brief This method performs block Gmres iterations until the status
   * test indicates the need to stop or an error occurs (in which case, an
   * exception is thrown).
   *
   * iterate() will first determine whether the solver is inintialized; if
   * not, it will call initialize() using default arguments. After
   * initialization, the solver performs block Gmres iterations until the
   * status test evaluates as ::Passed, at which point the method returns to
   * the caller. 
   *
   * The block Gmres iteration proceeds as follows:
   * -# The operator problem->applyOp() is applied to the newest \c blockSize vectors in the Krylov basis.
   * -# The resulting vectors are orthogonalized against the previous basis vectors, and made orthonormal.
   * -# The Hessenberg matrix is updated.
   * -# The least squares system is updated.
   *
   * The status test is queried at the beginning of the iteration.
   *
   * Possible exceptions thrown include the BlockGmresIterOrthoFailure.
   *
   */
  void iterate();

  /*! \brief Initialize the solver to an iterate, providing a complete state.
   *
   * The %BlockGmresIter contains a certain amount of state, consisting of the current 
   * Krylov basis and the associated Hessenberg matrix.
   *
   * initialize() gives the user the opportunity to manually set these,
   * although this must be done with caution, abiding by the rules given
   * below. All notions of orthogonality and orthonormality are derived from
   * the inner product specified by the orthogonalization manager.
   *
   * \post 
   * <li>isInitialized() == \c true (see post-conditions of isInitialize())
   *
   * The user has the option of specifying any component of the state using
   * initialize(). However, these arguments are assumed to match the
   * post-conditions specified under isInitialized(). Any necessary component of the
   * state not given to initialize() will be generated.
   *
   * \note For any pointer in \c newstate which directly points to the multivectors in 
   * the solver, the data is not copied.
   */
  void initialize(BlockGmresIterState<ScalarType,MV> newstate);

  /*! \brief Initialize the solver with the initial vectors from the linear problem
   *  or random data.
   */
  void initialize()
  {
    BlockGmresIterState<ScalarType,MV> empty;
    initialize(empty);
  }
  
  /*! \brief Get the current state of the eigensolver.
   *
   * The data is only valid if isInitialized() == \c true.
   *
   * \returns A BlockKrylovSchurState object containing const pointers to the current
   * solver state.
   */
  BlockGmresIterState<ScalarType,MV> getState() const {
    BlockGmresIterState<ScalarType,MV> state;
    state.curDim = curDim_;
    state.V = V_;
    state.H = H_;
    state.R = R_;
    state.Z = Z_;
    return state;
  }

  //@}

  
  //! @name Status methods
  //@{ 

  //! \brief Get the current iteration count.
  int getNumIters() const { return iter_; }
  
  //! \brief Reset the iteration count.
  void resetNumIters() { iter_ = 0; }

  //! Get the norms of the residuals native to the solver.
  //! \return A vector of length blockSize containing the native residuals.
  Teuchos::RefCountPtr<const MV> getNativeResiduals( std::vector<MagnitudeType> *norms ) const;

  //! Get the current update to the linear system.
  /*! \note Some solvers, like GMRES, do not compute updates to the solution every iteration.
            This method forces its computation.  Other solvers, like CG, update the solution
            each iteration, so this method will return a zero vector indicating that the linear
            problem contains the current solution.
  */
  Teuchos::RefCountPtr<MV> getCurrentUpdate() const;

  //! Get the dimension of the search subspace used to generate the current eigenvectors and eigenvalues.
  int getCurSubspaceDim() const { 
    if (!initialized_) return 0;
    return curDim_;
  };

  //! Get the maximum dimension allocated for the search subspace.
  int getMaxSubspaceDim() const { return blockSize_*numBlocks_; }

  //@}

  
    //! @name Accessor methods
  //@{ 

  //! Get a constant reference to the eigenvalue problem.
  const LinearProblem<ScalarType,MV,OP>& getProblem() const { return lp_; }

  //! Get the blocksize to be used by the iterative solver in solving this linear problem.
  int getBlockSize() const { return blockSize_; }
  
  //! \brief Set the blocksize.
  void setBlockSize(int blockSize) { setSize( blockSize, numBlocks_ ); }
  
  //! Get the maximum number of blocks used by the iterative solver in solving this linear problem.
  int getNumBlocks() const { return numBlocks_; }
  
  //! \brief Set the maximum number of blocks used by the iterative solver.
  void setNumBlocks(int numBlocks) { setSize( blockSize_, numBlocks ); }
  
  /*! \brief Set the blocksize and number of blocks to be used by the
   * iterative solver in solving this linear problem.
   *
   *  Changing either the block size or the number of blocks will reset the
   *  solver to an uninitialized state.
   */
  void setSize(int blockSize, int numBlocks);

  //! States whether the solver has been initialized or not.
  bool isInitialized() { return initialized_; }

  //@}

    //! @name Output methods
  //@{ 

  //! This method requests that the solver print out its current status to screen.
  void currentStatus(ostream &os);

  //@}

  private:

  //
  // Internal methods
  //
  //! Method for updating QR factorization of upper Hessenberg matrix 
  void UpdateLSQR();

  //! Method for initalizing the state storage needed by block GMRES.
  void setStateSize();
  
  //
  // Classes inputed through constructor that define the eigenproblem to be solved.
  //
  const Teuchos::RefCountPtr<LinearProblem<ScalarType,MV,OP> >    lp_;
  const Teuchos::RefCountPtr<OutputManager<ScalarType> >          om_;
  const Teuchos::RefCountPtr<StatusTest<ScalarType,MV,OP> >       stest_;
  const Teuchos::RefCountPtr<OrthoManager<ScalarType,MV> >        ortho_;

  //
  // Internal timers
  //
  // Restart the timers each time iterate() is called.
  bool restartTimers_;
  Teuchos::RefCountPtr<Teuchos::Time> timerOrtho_;

  //
  // Algorithmic parameters
  //  
  // blockSize_ is the solver block size.
  // It controls the number of vectors added to the basis on each iteration.
  int blockSize_;
  // numBlocks_ is the size of the allocated space for the Krylov basis, in blocks.
  int numBlocks_; 
  
  // Storage for QR factorization of the least squares system.
  Teuchos::SerialDenseVector<int,ScalarType> beta, sn;
  Teuchos::SerialDenseVector<int,MagnitudeType> cs;
  
  // 
  // Current solver state
  //
  // initialized_ specifies that the basis vectors have been initialized and the iterate() routine
  // is capable of running; _initialize is controlled  by the initialize() member method
  // For the implications of the state of initialized_, please see documentation for initialize()
  bool initialized_;

  // stateStorageInitialized_ specified that the state storage has be initialized to the current
  // blockSize_ and numBlocks_.  This initialization may be postponed if the linear problem was
  // generated without the right-hand side or solution vectors.
  bool stateStorageInitialized_;

  // Current subspace dimension, and number of iterations performed.
  int curDim_, iter_;
  
  // 
  // State Storage
  //
  Teuchos::RefCountPtr<MV> V_;
  //
  // Projected matrices
  // H_ : Projected matrix from the Krylov factorization AV = VH + FE^T
  //
  Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > H_;
  // 
  // QR decomposition of Projected matrices for solving the least squares system HY = B.
  // R_: Upper triangular reduction of H
  // Z_: Q applied to right-hand side of the least squares system
  Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > R_;
  Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > Z_;  
};

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Constructor.
  template<class ScalarType, class MV, class OP>
  BlockGmresIter<ScalarType,MV,OP>::BlockGmresIter(const Teuchos::RefCountPtr<LinearProblem<ScalarType,MV,OP> > &problem, 
						   const Teuchos::RefCountPtr<OutputManager<ScalarType> > &printer,
						   const Teuchos::RefCountPtr<StatusTest<ScalarType,MV,OP> > &tester,
						   const Teuchos::RefCountPtr<OrthoManager<ScalarType,MV> > &ortho,
						   const Teuchos::RefCountPtr<Teuchos::ParameterList> &params ):
    lp_(problem),
    om_(printer),
    stest_(tester),
    ortho_(ortho),
    restartTimers_(false),
    timerOrtho_(Teuchos::TimeMonitor::getNewTimer("Belos: Orthogonalization")),
    blockSize_(0),
    numBlocks_(0),
    initialized_(false),
    stateStorageInitialized_(false),
    curDim_(0),
    iter_(0)
  {
    // Get the maximum number of blocks allowed for this Krylov subspace
    TEST_FOR_EXCEPTION(!params->isParameter("Num Blocks"), std::invalid_argument,
                       "Belos::BlockGmresIter::constructor: mandatory parameter 'Num Blocks' is not specified.");
    int nb = Teuchos::getParameter<int>(*params, "Num Blocks");

    // Set the block size and allocate data
    int bs = params->get("Block Size", 1);
    setSize( bs, nb );
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Set the block size and make necessary adjustments.
  template <class ScalarType, class MV, class OP>
  void BlockGmresIter<ScalarType,MV,OP>::setSize (int blockSize, int numBlocks)
  {
    // This routine only allocates space; it doesn't not perform any computation
    // any change in size will invalidate the state of the solver.

    TEST_FOR_EXCEPTION(numBlocks <= 0 || blockSize <= 0, std::invalid_argument, "Belos::BlockGmresIter::setSize was passed a non-positive argument.");
    if (blockSize == blockSize_ && numBlocks == numBlocks_) {
      // do nothing
      return;
    }

    blockSize_ = blockSize;
    numBlocks_ = numBlocks;

    initialized_ = false;
    curDim_ = 0;

    // Use the current blockSize_ and numBlocks_ to initialized the state storage.    
    setStateSize();
    
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Setup the state storage.
  template <class ScalarType, class MV, class OP>
  void BlockGmresIter<ScalarType,MV,OP>::setStateSize ()
  {
    // Check if there is any multivector to clone from.
    Teuchos::RefCountPtr<const MV> lhsMV = lp_->GetLHS();
    Teuchos::RefCountPtr<const MV> rhsMV = lp_->GetRHS();
    if (lhsMV == Teuchos::null && rhsMV == Teuchos::null) {
      stateStorageInitialized_ = false;
      return;
    }
    else {
      
      //////////////////////////////////
      // blockSize*numBlocks dependent
      //
      int newsd = blockSize_*(numBlocks_+1);
      
      if (blockSize_==1) {
	cs.resize( newsd );
	sn.resize( newsd );
      }
      else {
	beta.resize( newsd );
      }
      
      // Initialize the state storage
      // If the subspace has not be initialized before, generate it using the LHS or RHS from lp_.
      if (V_ == Teuchos::null) {
	// Get the multivector that is not null.
	Teuchos::RefCountPtr<const MV> tmp = ( (rhsMV!=Teuchos::null)? rhsMV: lhsMV );
	TEST_FOR_EXCEPTION(tmp == Teuchos::null,std::invalid_argument,
			   "Belos::BlockGmresIter::setStateSize(): linear problem does not specify multivectors to clone from.");
	V_ = MVT::Clone( *tmp, newsd );
      }
      else {
	// Generate V_ by cloning itself ONLY if more space is needed.
	if (V_->NumVectors() < newsd) {
	  Teuchos::RefCountPtr<const MV> tmp = V_;
	  V_ = MVT::Clone( *tmp, newsd );
	}
      }

      // Generate H_ only if it doesn't exist, otherwise resize it.
      if (H_ == Teuchos::null)
	H_ = Teuchos::rcp( new Teuchos::SerialDenseMatrix<int,ScalarType>( newsd, newsd-blockSize_ ) );	else
	H_->shapeUninitialized( newsd, newsd-blockSize_ );
      
      // TODO:  Insert logic so that Hessenberg matrix can be saved and reduced matrix is stored in R_
      //R_ = Teuchos::rcp( new Teuchos::SerialDenseMatrix<int,ScalarType>( newsd, newsd-blockSize_ ) );
      // Generate Z_ only if it doesn't exist, otherwise resize it.
      if (Z_ == Teuchos::null)
	Z_ = Teuchos::rcp( new Teuchos::SerialDenseMatrix<int,ScalarType>(newsd,blockSize_) );
      else
	Z_->shapeUninitialized( newsd, blockSize_ );
      
      // State storage has now been initialized.
      stateStorageInitialized_ = true;
    }
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Get the current update from this subspace.
  template <class ScalarType, class MV, class OP>
  Teuchos::RefCountPtr<MV> BlockGmresIter<ScalarType,MV,OP>::getCurrentUpdate() const
  {
    //
    // If this is the first iteration of the Arnoldi factorization, 
    // there is no update, so return Teuchos::null. 
    //
    RefCountPtr<MV> currentUpdate = Teuchos::null;
    if (curDim_==0) { 
      return currentUpdate; 
    } else {
      const ScalarType one = Teuchos::ScalarTraits<ScalarType>::one();
      const ScalarType zero = Teuchos::ScalarTraits<ScalarType>::zero();
      int m = curDim_*blockSize_;
      Teuchos::BLAS<int,ScalarType> blas;
      //
      //  Make a view and then copy the RHS of the least squares problem.  DON'T OVERWRITE IT!
      //
      Teuchos::SerialDenseMatrix<int,ScalarType> y( Teuchos::Copy, *Z_, m, blockSize_ );
      //
      //  Solve the least squares problem.
      //
      blas.TRSM( Teuchos::LEFT_SIDE, Teuchos::UPPER_TRI, Teuchos::NO_TRANS,
		 Teuchos::NON_UNIT_DIAG, m, blockSize_, one,  
		 H_->values(), H_->stride(), y.values(), y.stride() );
      //
      //  Compute the current update.
      //
      std::vector<int> index(m);
      for ( int i=0; i<m; i++ ) {   
        index[i] = i;
      }
      RefCountPtr<const MV> Vjp1 = MVT::CloneView( *V_, index );
      MVT::MvTimesMatAddMv( one, *Vjp1, y, zero, *currentUpdate );
    }
    return currentUpdate;
  }


  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Get the native residuals stored in this iteration.  
  // Note:  No residual vector will be returned by Gmres.
  template <class ScalarType, class MV, class OP>
  Teuchos::RefCountPtr<const MV> BlockGmresIter<ScalarType,MV,OP>::getNativeResiduals( std::vector<MagnitudeType> *norms ) const 
  {
    //
    // NOTE: Make sure the incoming vector is the correct size!
    //
    if ( norms && (int)norms->size() < blockSize_ )                         
      norms->resize( blockSize_ );                                          
    
    if (curDim_ != 0) {
      if (norms) {
        Teuchos::BLAS<int,ScalarType> blas;
        for (int j=0; j<blockSize_; j++)
          (*norms)[j] = blas.NRM2( blockSize_, &Z_(curDim_, j), 1);
      }
    }
    return Teuchos::null;
  }

  

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Initialize this iteration object
  template <class ScalarType, class MV, class OP>
  void BlockGmresIter<ScalarType,MV,OP>::initialize(BlockGmresIterState<ScalarType,MV> newstate)
  {
    // Initialize the state storage if it isn't already.
    if (!stateStorageInitialized_) 
      setStateSize();

    TEST_FOR_EXCEPTION(!stateStorageInitialized_,std::invalid_argument,
		       "Belos::BlockGmresIter::initialize(): Cannot initialize state storage!");
    
    // NOTE:  In BlockGmresIter, V and Z are required!!!  
    // inconsitent multivectors widths and lengths will not be tolerated, and
    // will be treated with exceptions.
    //
    std::string errstr("Anasazi::BlockKrylovSchur::initialize(): specified multivectors must have a consistent length and width.");

    if (newstate.V != Teuchos::null && newstate.Z != Teuchos::null) {

      // initialize V_,Z_, and curDim_

      TEST_FOR_EXCEPTION( MVT::GetVecLength(*newstate.V) != MVT::GetVecLength(*V_),
                          std::invalid_argument, errstr );
      TEST_FOR_EXCEPTION( MVT::GetNumberVecs(*newstate.V) < blockSize_,
                          std::invalid_argument, errstr );
      TEST_FOR_EXCEPTION( newstate.curDim > blockSize_*(numBlocks_+1),
                          std::invalid_argument, errstr );

      curDim_ = newstate.curDim;
      int lclDim = MVT::GetNumberVecs(*newstate.V);

      // check size of H
      TEST_FOR_EXCEPTION(newstate.Z->numRows() < curDim_ || newstate.Z->numCols() < blockSize_, std::invalid_argument, errstr);
      
      if (curDim_ == 0 && lclDim > blockSize_) {
        om_->stream(Warnings) << "Belos::BlockGmresIter::initialize(): the solver was initialized with a kernel of " << lclDim << endl
			      << "The block size however is only " << blockSize_ << endl
			      << "The last " << lclDim - blockSize_ << " vectors of the kernel will be overwritten on the first call to iterate()." << endl;
      }


      // copy basis vectors from newstate into V
      if (newstate.V != V_) {
        std::vector<int> nevind(lclDim);
        for (int i=0; i<lclDim; i++) nevind[i] = i;
        MVT::SetBlock(*newstate.V,nevind,*V_);
      }

      // put data into H_, make sure old information is not still hanging around.
      if (newstate.Z != Z_) {
        Teuchos::SerialDenseMatrix<int,ScalarType> newZ(Teuchos::View,*newstate.Z,curDim_+blockSize_,blockSize_);
        Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > lclZ;
        lclZ = Teuchos::rcp( new Teuchos::SerialDenseMatrix<int,ScalarType>(Teuchos::View,*Z_,curDim_+blockSize_,blockSize_) );
        lclZ->assign(newZ);

        // done with local pointers
        lclZ = Teuchos::null;
      }

    }
    else {

      TEST_FOR_EXCEPTION(newstate.V == Teuchos::null,std::invalid_argument,
                         "Belos::BlockGmresIter::initialize(): BlockGmresStateIterState does not have initial kernel V_0.");

      TEST_FOR_EXCEPTION(newstate.Z == Teuchos::null,std::invalid_argument,
                         "Belos::BlockGmresIter::initialize(): BlockGmresStateIterState does not have initial norms Z_0.");
    }

    // the solver is initialized
    initialized_ = true;

    /*
    if (om_->isVerbosity( Debug ) ) {
      // Check almost everything here
      CheckList chk;
      chk.checkV = true;
      chk.checkArn = true;
      chk.checkAux = true;
      om_->print( Debug, accuracyCheck(chk, ": after initialize()") );
    }

    // Print information on current status
    if (om_->isVerbosity(Debug)) {
      currentStatus( om_->stream(Debug) );
    }
    else if (om_->isVerbosity(IterationDetails)) {
      currentStatus( om_->stream(IterationDetails) );
    }
    */
  }


  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Iterate until the status test informs us we should stop.
  template <class ScalarType, class MV, class OP>
  void BlockGmresIter<ScalarType,MV,OP>::iterate()
  {
    //
    // Allocate/initialize data structures
    //
    if (initialized_ == false) {
      initialize();
    }
    
    // Compute the current search dimension. 
    // If the problem is non-Hermitian and the blocksize is one, let the solver use the extra vector.
    int searchDim = blockSize_*numBlocks_;

    ////////////////////////////////////////////////////////////////
    // iterate until the status test tells us to stop.
    //
    // also break if our basis is full
    //
    while (stest_->checkStatus(this) != Passed && curDim_+blockSize_ <= searchDim) {

      iter_++;

      // F can be found at the curDim_ block, but the next block is at curDim_ + blockSize_.
      int lclDim = curDim_ + blockSize_; 

      // Get the current part of the basis.
      std::vector<int> curind(blockSize_);
      for (int i=0; i<blockSize_; i++) { curind[i] = lclDim + i; }
      Teuchos::RefCountPtr<MV> Vnext = MVT::CloneView(*V_,curind);

      // Get a view of the previous vectors
      // this is used for orthogonalization and for computing V^H K H
      for (int i=0; i<blockSize_; i++) { curind[i] = curDim_ + i; }
      Teuchos::RefCountPtr<MV> Vprev = MVT::CloneView(*V_,curind);

      // Compute the next vector in the Krylov basis:  Vnext = Op*Vprev
      lp_->Apply(*Vprev,*Vnext);
      Vprev = Teuchos::null;
      
      // Remove all previous Krylov basis vectors from Vnext
      {
        Teuchos::TimeMonitor lcltimer( *timerOrtho_ );
        
        // Get a view of all the previous vectors
        std::vector<int> prevind(lclDim);
        for (int i=0; i<lclDim; i++) { prevind[i] = i; }
        Vprev = MVT::CloneView(*V_,prevind);
        Teuchos::Array<Teuchos::RefCountPtr<const MV> > AVprev(1, Vprev);
        
        // Get a view of the part of the Hessenberg matrix needed to hold the ortho coeffs.
        Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> >
          subH = Teuchos::rcp( new Teuchos::SerialDenseMatrix<int,ScalarType>
                               ( Teuchos::View,*H_,lclDim,blockSize_,0,curDim_ ) );
        Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > AsubH;
        AsubH.append( subH );
        
        // Get a view of the part of the Hessenberg matrix needed to hold the norm coeffs.
        Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> >
          subR = Teuchos::rcp( new Teuchos::SerialDenseMatrix<int,ScalarType>
                               ( Teuchos::View,*H_,blockSize_,blockSize_,lclDim,curDim_ ) );
        int rank = ortho_->projectAndNormalize(*Vnext,AsubH,subR,AVprev);
        TEST_FOR_EXCEPTION(rank != blockSize_,BlockGmresIterOrthoFailure,
                           "Belos::BlockGmresIter::iterate(): couldn't generate basis of full rank.");
      }
      //
      // V has been extended, and H has been extended. 
      //
      // Update the QR factorization of the upper Hessenberg matrix
      //
      UpdateLSQR();
      //
      // Update basis dim and release all pointers.
      //
      Vnext = Teuchos::null;
      curDim_ += blockSize_;
      //        
      /*      
      // When required, monitor some orthogonalities
      if (om_->isVerbosity( Debug ) ) {
      // Check almost everything here
      CheckList chk;
      chk.checkV = true;
      chk.checkArn = true;
      om_->print( Debug, accuracyCheck(chk, ": after local update") );
      }
      else if (om_->isVerbosity( OrthoDetails ) ) {
        CheckList chk;
        chk.checkV = true;
        om_->print( OrthoDetails, accuracyCheck(chk, ": after local update") );
      }
      
      // Print information on current iteration
      if (om_->isVerbosity(Debug)) {
        currentStatus( om_->stream(Debug) );
      }
      else if (om_->isVerbosity(IterationDetails)) {
        currentStatus( om_->stream(IterationDetails) );
      }
      */
      
    } // end while (statusTest == false)
   
  }

  
  template<class ScalarType, class MV, class OP>
  void BlockGmresIter<ScalarType,MV,OP>::UpdateLSQR()
  {
    int i, j, maxidx;
    ScalarType sigma, mu, vscale, maxelem;
    const ScalarType zero = Teuchos::ScalarTraits<ScalarType>::zero();
    
    Teuchos::LAPACK<int, ScalarType> lapack;
    Teuchos::BLAS<int, ScalarType> blas;
    //
    // Apply previous transformations and compute new transformation to reduce upper-Hessenberg
    // system to upper-triangular form.
    //
    if (blockSize_ == 1) {
      //
      // QR factorization of Least-Squares system with Givens rotations
      //
      for (i=0; i<curDim_; i++) {
	//
	// Apply previous Givens rotations to new column of Hessenberg matrix
	//
	blas.ROT( 1, &(*H_)(i,curDim_), 1, &(*H_)(i+1, curDim_), 1, &cs[i], &sn[i] );
      }
      //
      // Calculate new Givens rotation
      //
      blas.ROTG( &(*H_)(curDim_,curDim_), &(*H_)(curDim_+1,curDim_), &cs[curDim_], &sn[curDim_] );
      (*H_)(curDim_+1,curDim_) = zero;
      //
      // Update RHS w/ new transformation
      //
      blas.ROT( 1, &(*Z_)(curDim_,0), 1, &(*Z_)(curDim_+1,0), 1, &cs[curDim_], &sn[curDim_] );
    }
    else {
      //
      // QR factorization of Least-Squares system with Householder reflectors
      //
      for (j=0; j<blockSize_; j++) {
	//
	// Apply previous Householder reflectors to new block of Hessenberg matrix
	//
	for (i=0; i<curDim_+j; i++) {
	  sigma = blas.DOT( blockSize_, &(*H_)(i+1,i), 1, &(*H_)(i+1,curDim_+j), 1);
	  sigma += (*H_)(i,curDim_+j);
	  sigma *= beta[i];
	  blas.AXPY(blockSize_, -sigma, &(*H_)(i+1,i), 1, &(*H_)(i+1,curDim_+j), 1);
	  (*H_)(i,curDim_+j) -= sigma;
	}
	//
	// Compute new Householder reflector
	//
	maxidx = blas.IAMAX( blockSize_+1, &(*H_)(curDim_+j,curDim_+j), 1 );
	maxelem = (*H_)(curDim_+j+maxidx-1,curDim_+j);
	for (i=0; i<blockSize_+1; i++)
	  (*H_)(curDim_+j+i,curDim_+j) /= maxelem;
	sigma = blas.DOT( blockSize_, &(*H_)(curDim_+j+1,curDim_+j), 1,
			  &(*H_)(curDim_+j+1,curDim_+j), 1 );
	if (sigma == zero) {
	  beta[curDim_ + j] = zero;
	} else {
	  mu = sqrt((*H_)(curDim_+j,curDim_+j)*(*H_)(curDim_+j,curDim_+j)+sigma);
	  if ( Teuchos::ScalarTraits<ScalarType>::real((*H_)(curDim_+j,curDim_+j)) 
	       < Teuchos::ScalarTraits<MagnitudeType>::zero() ) {
	    vscale = (*H_)(curDim_+j,curDim_+j) - mu;
	  } else {
	    vscale = -sigma / ((*H_)(curDim_+j,curDim_+j) + mu);
	  }
	  beta[curDim_+j] = 2.0*vscale*vscale/(sigma + vscale*vscale);
	  (*H_)(curDim_+j,curDim_+j) = maxelem*mu;
	  for (i=0; i<blockSize_; i++)
	    (*H_)(curDim_+j+1+i,curDim_+j) /= vscale;
	}
	//
	// Apply new Householder reflector to rhs
	//
	for (i=0; i<blockSize_; i++) {
	  sigma = blas.DOT( blockSize_, &(*H_)(curDim_+j+1,curDim_+j),
			    1, &(*Z_)(curDim_+j+1,i), 1);
	  sigma += (*Z_)(curDim_+j,i);
	  sigma *= beta[curDim_+j];
	  blas.AXPY(blockSize_, -sigma, &(*H_)(curDim_+j+1,curDim_+j),
		    1, &(*Z_)(curDim_+j+1,i), 1);
	  (*Z_)(curDim_+j,i) -= sigma;
	}
      }
    } // end if (blockSize_ == 1)
  } // end UpdateLSQR()

} // end Belos namespace

#endif /* BELOS_BLOCK_GMRES_ITER_HPP */
