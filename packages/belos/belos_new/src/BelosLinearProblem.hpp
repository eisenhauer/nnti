
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

#ifndef BELOS_LINEAR_PROBLEM_HPP
#define BELOS_LINEAR_PROBLEM_HPP

/*! \file BelosLinearProblem.hpp
    \brief Class which describes the linear problem to be solved by the iterative solver.
*/

#include "BelosMultiVecTraits.hpp"
#include "BelosOperatorTraits.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_TimeMonitor.hpp"

using Teuchos::RefCountPtr;
using Teuchos::rcp;
using Teuchos::null;
using Teuchos::rcp_const_cast;
using Teuchos::ParameterList;

/*! \class Belos::LinearProblem  
  \brief The Belos::LinearProblem class is a wrapper that encapsulates the 
  general information needed for solving a linear system of equations.  
  The general information is being held as either Belos::Operator or
  Belos::MultiVec objects.
*/

namespace Belos {
  
  template <class ScalarType, class MV, class OP>
  class LinearProblem {
   
  public:
    
    //! @name Constructors/Destructor
    //@{ 
    //!  Default Constructor.
    /*! Creates an empty Belos::LinearProblem instance. The operator A, left-hand-side X
      and right-hand-side B must be set using the setOperator(), setLHS() and setRHS()
      methods respectively.
    */
    LinearProblem(void);
    
    //! Unpreconditioned linear system constructor.
    /*! Creates an unpreconditioned LinearProblem instance with the 
      Belos::Operator (\c A), initial guess (\c X), and right hand side (\c B). 
      Preconditioners can be set using the setLeftPrec() and setRightPrec() methods, and
      scaling can also be set using the setLeftScale() and setRightScale() methods.
    */
    LinearProblem(const RefCountPtr<const OP> &A, 
		  const RefCountPtr<MV> &X, 
		  const RefCountPtr<const MV> &B
		  );
    
    //! Copy Constructor.
    /*! Makes copy of an existing LinearProblem instance.
     */
    LinearProblem(const LinearProblem<ScalarType,MV,OP>& Problem);
    
    //! Destructor.
    /*! Completely deletes a LinearProblem object.  
     */
    virtual ~LinearProblem(void);
    //@}
    
    //! @name Set methods
    //@{ 
    
    //! Set Operator A of linear problem AX = B.
    /*! Sets a pointer to an Operator.  No copy of the operator is made.
     */
    void setOperator(const RefCountPtr<const OP> &A) { A_ = A; };
    
    //! Set left-hand-side X of linear problem AX = B.
    /*! Sets a pointer to a MultiVec.  No copy of the object is made.
     */
    void setLHS(const RefCountPtr<MV> &X);
    
    //! Set right-hand-side B of linear problem AX = B.
    /*! Sets a pointer to a MultiVec.  No copy of the object is made.
     */
    void setRHS(const RefCountPtr<const MV> &B) { B_ = B; };
    
    //! Set left preconditioning operator (\c LP) of linear problem AX = B.
    /*! Sets a pointer to an Operator.  No copy of the operator is made.
     */
    void setLeftPrec(const RefCountPtr<const OP> &LP) {  LP_ = LP; Left_Prec_ = true; };
    
    //! Set right preconditioning operator (\c RP) of linear problem AX = B.
    /*! Sets a pointer to an Operator.  No copy of the operator is made.
     */
    void setRightPrec(const RefCountPtr<const OP> &RP) { RP_ = RP; Right_Prec_ = true; };
    
    //! Set the parameter list for defining the behavior of the linear problem class.
    void setParameterList(const RefCountPtr<ParameterList> &PL) { PL_ = PL; };

    //! Set the blocksize of the linear problem.  This information is used to set up the linear problem for block solvers.
    void setBlockSize(int blocksize) { default_blocksize_ = blocksize; blocksize_ = blocksize; };
    
    //! Inform the linear problem that the solver is finished with the current linear system.
    /*! \note This method is to be <b> only </b> used by the solver to inform the linear problem manager that it's
      finished with this block of linear systems.  The next time the Curr(RHS/LHS)Vec() is called, the next
      linear system will be returned.  Computing the next linear system isn't done in this method in case the 
      blocksize is changed.
    */
    void setCurrLSVec();
    
    //! Inform the linear problem that the operator is Hermitian.
    /*! This knowledge may allow the operator to take advantage of the linear problem symmetry.
      However, this should not be set to true if the preconditioner is not Hermitian, or symmetrically
      applied.
    */
    void setHermitian(){ isHermitian_ = true; };
    
    //! Compute the new solution to the linear system given the /c update.
    /*! \note If \c updateLP is true, then the next time GetCurrResVecs is called, a new residual will be computed.  
      This keeps the linear problem from having to recompute the residual vector everytime it's asked for if
      the solution hasn't been updated.  If \c updateLP is false, the new solution is computed without actually 
      updating the linear problem.
    */
    RefCountPtr<MV> updateSolution( const RefCountPtr<MV>& update = Teuchos::null,
				    bool updateLP = false,
                                    ScalarType scale = Teuchos::ScalarTraits<ScalarType>::one() );    

    //! Compute the new solution to the linear system given the /c update without updating the linear problem.
    RefCountPtr<MV> updateSolution( const RefCountPtr<MV>& update = Teuchos::null,
                                    ScalarType scale = Teuchos::ScalarTraits<ScalarType>::one() ) const
    { return const_cast<LinearProblem<ScalarType,MV,OP> *>(this)->updateSolution( update, false, scale ); }

    //@}
    
    //! @name Reset method
    //@{ 
    
    //! Reset the linear problem manager.
    /*! This is useful for solving the linear system with another right-hand side.  
      The internal flags will be set as if the linear system manager was just initialized.
    */
    void reset( const RefCountPtr<MV> &newX = null, const RefCountPtr<const MV> &newB = null );
    //@}
    
    //! @name Accessor methods
    //@{ 
    
    //! Get a pointer to the operator A.
    RefCountPtr<const OP> getOperator() const { return(A_); };
    
    //! Get a pointer to the left-hand side X.
    RefCountPtr<MV> getLHS() const { return(X_); };
    
    //! Get a pointer to the right-hand side B.
    RefCountPtr<const MV> getRHS() const { return(B_); };
    
    //! Get a pointer to the initial residual vector.
    /*! \note This may be the preconditioned residual, if the linear problem is left-preconditioned.
     */
    const MV& getInitResVec();
    
    //! Get a pointer to the current residual vector.
    /*!
      
    \param  CurrSoln  [in] If non-null, then this is the LHS that is used to compute
    the current residual.  If null, then getCurrLHSVec() is used.
    
    Note, the current residual is always computed with respect to getCurrRHSVec().		    
    
    \note <ul>
    <li> This method computes the true residual of the current linear system
    with respect to getCurrRHSVec() and getCurrLHSVec() if CurrSoln==NULL
    or with respect to *CurrSoln if CurrSoln!=NULL.  
    <li> If the solution hasn't been updated in the LinearProblem and
    a current solution has been computed by the solver (like GMRES), it can
    be passed into this method to compute the residual.
    </ul>
    */
    const MV& getCurrResVec( const MV* CurrSoln = 0 );

    //! Get a pointer to the current residual vector.
    /*! This method is called by the solver of any method that is interested in the current linear system
      being solved for.
      <ol>
      <li> If the solution has been updated by the solver, then this vector is current ( see SolutionUpdated() ).
      </ol>
    */
    RefCountPtr<const MV> getCurrResVec() const { return R_; }
    
    //! Get a pointer to the current left-hand side (solution) of the linear system.
    /*! This method is called by the solver or any method that is interested in the current linear system
      being solved for.  
      <ol>
      <li> If the solution has been updated by the solver, then this vector is current ( see SolutionUpdated() ).
      <li> If there is no linear system to solve, this method will return a NULL pointer
      </ol>
    */
    RefCountPtr<MV> getCurrLHSVec();
    
    //! Get a pointer to the current right-hand side of the linear system.
    /*! This method is called by the solver of any method that is interested in the current linear system
      being solved for.  
      <ol>
      <li> If the solution has been updated by the solver, then this vector is current ( see SolutionUpdated() ).
      <li> If there is no linear system to solve, this method will return a NULL pointer
      </ol>
    */	
    RefCountPtr<MV> getCurrRHSVec();
    
    //! Get a pointer to the left preconditioning operator.
    RefCountPtr<const OP> getLeftPrec() const { return(LP_); };
    
    //! Get a pointer to the right preconditioning operator.
    RefCountPtr<const OP> getRightPrec() const { return(RP_); };
    
    //! Get a pointer to the parameter list.
    RefCountPtr<ParameterList> getParameterList() const { return(PL_); };

    //! Get the default blocksize being used by the linear problem.
    int getBlockSize() const { return( default_blocksize_ ); };
    
    //! Get the current blocksize being used by the linear problem.
    /*! This may be different from the default blocksize set for the linear problem in the event
      that the default blocksize doesn't divide evenly into the number of right-hand sides, but
      it should not be more than the default blocksize.
    */
    int getCurrBlockSize() const { return( blocksize_ ); };

    //! Get the current number of linear systems being solved for.
    /*! Since the block size is independent of the number of right-hand sides, 
      it is important to know how many linear systems
      are being solved for when the status is checked.  This is informative for residual
      checks because the entire block of residuals may not be of interest.  Thus, this 
      number can be anywhere between 1 and the blocksize of the linear system.
    */
    int getNumToSolve() const { return( num_to_solve_ ); };
    
    //! Get the 0-based index of the first vector in the current right-hand side block being solved for.
    /*! Since the block size is independent of the number of right-hand sides for
      some solvers (GMRES, CG, etc.), it is important to know which right-hand sides
      are being solved for.  That may mean you need to update the information
      about the norms of your initial residual vector for weighting purposes.  This
      information can keep you from querying the solver for information that rarely
      changes.
    */
    int getRHSIndex() const { return( rhs_index_ ); }
    
    //@}
    
    //! @name State methods
    //@{ 
    
    //! Get the current status of the solution.
    /*! This only means that the current linear system being solved for ( obtained by getCurr<LHS/RHS>Vec() )
      has been updated by the solver.  This will be true every iteration for solvers like CG, but not
      true until restarts for GMRES.
    */
    bool isSolutionUpdated() const { return(solutionUpdated_); }
    
    //! Get the current symmetry of the operator.
    bool isHermitian() const { return(isHermitian_); }
    
    //! Get information on whether the linear system is being preconditioned on the left.
    bool isLeftPrec() const { return(LP_!=Teuchos::null); }

    //! Get information on whether the linear system is being preconditioned on the right.
    bool isRightPrec() const { return(RP_!=Teuchos::null); }
 
    //@}
    
    //! @name Apply / Compute methods
    //@{ 
    
    //! Apply the composite operator of this linear problem to \c x, returning \c y.
    /*! This application is the composition of the left/right preconditioner and operator.
      Most Krylov methods will use this application method within their code.
      
      Precondition:<ul>
      <li><tt>getOperator().get()!=NULL</tt>
      </ul>
    */
    void apply( const MV& x, MV& y ) const;
    
    //! Apply ONLY the operator to \c x, returning \c y.
    /*! This application is only of the linear problem operator, no preconditioners are applied.
      Flexible variants of Krylov methods will use this application method within their code.
      
      Precondition:<ul>
      <li><tt>getOperator().get()!=NULL</tt>
      </ul>
    */
    void applyOp( const MV& x, MV& y ) const;
    
    //! Apply ONLY the left preconditioner to \c x, returning \c y.  
    /*! This application is only of the left preconditioner, which may be required for flexible variants
      of Krylov methods.
      \note This will return Undefined if the left preconditioner is not defined for this operator.
    */  
    void applyLeftPrec( const MV& x, MV& y ) const;
    
    //! Apply ONLY the right preconditioner to \c x, returning \c y.
    /*! This application is only of the right preconditioner, which may be required for flexible variants
      of Krylov methods.
      \note This will return Undefined if the right preconditioner is not defined for this operator.
    */
    void applyRightPrec( const MV& x, MV& y ) const;
    
    //! Compute a residual \c R for this operator given a solution \c X, and right-hand side \c B.
    /*! This method will compute the residual for the current linear system if \c X and \c B are null pointers.
      The result will be returned into R.  Otherwise <tt>R = OP(A)X - B</tt> will be computed and returned.
      \note This residual will be a preconditioned residual if the system has a left preconditioner.
    */
    void computeCurrResVec( MV* R, const MV* X = 0, const MV* B = 0 ) const;

    //! Compute a residual \c R for this operator given a solution \c X, and right-hand side \c B.
    /*! This method will compute the residual for the current linear system if \c X and \c B are null pointers.
      The result will be returned into R.  Otherwise <tt>R = OP(A)X - B</tt> will be computed and returned.
      \note This residual will not be preconditioned if the system has a left preconditioner.
    */
    void computeActualResVec( MV* R, const MV* X = 0, const MV* B = 0 ) const;
    
    //@}
    
  private:
    
    //! Private method for populating the next block linear system.
    void setUpBlocks();
    
    //! Operator of linear system. 
    RefCountPtr<const OP> A_;
    
    //! Solution vector of linear system.
    RefCountPtr<MV> X_;
    
    //! Current solution vector of the linear system.
    RefCountPtr<MV> CurX_;
    
    //! Right-hand side of linear system.
    RefCountPtr<const MV> B_;
    
    //! Current right-hand side of the linear system.
    RefCountPtr<MV> CurB_;
    
    //! Current residual of the linear system.
    RefCountPtr<MV> R_;
    
    //! Initial residual of the linear system.
    RefCountPtr<MV> R0_;
    
    //! Left preconditioning operator of linear system
    RefCountPtr<const OP> LP_;  
    
    //! Right preconditioning operator of linear system
    RefCountPtr<const OP> RP_;
    
    //! Parameter list for defining the behavior of the linear problem class
    RefCountPtr<ParameterList> PL_;

    //! Timers
    mutable Teuchos::RefCountPtr<Teuchos::Time> timerOp_, timerPrec_;

    //! Default block size of linear system.
    int default_blocksize_;
    
    //! Current block size of linear system.
    int blocksize_;

    //! Number of linear systems that are currently being solver for ( <= blocksize_ )
    int num_to_solve_;
    
    //! Index of current block of right-hand sides being solver for ( RHS[:, rhs_index_:(rhs_index_+_blocksize)] ).
    int rhs_index_;
    
    //! Booleans to keep track of linear problem attributes/status.
    bool Left_Prec_;
    bool Right_Prec_;
    bool Left_Scale_;
    bool Right_Scale_;
    bool isHermitian_;
    bool solutionUpdated_;    
    bool solutionFinal_;
    bool initresidsComputed_;
    
    typedef MultiVecTraits<ScalarType,MV>  MVT;
    typedef OperatorTraits<ScalarType,MV,OP>  OPT;
  };
  
  //--------------------------------------------
  //  Constructor Implementations
  //--------------------------------------------
  
  template <class ScalarType, class MV, class OP>
  LinearProblem<ScalarType,MV,OP>::LinearProblem(void) : 
    timerOp_(Teuchos::TimeMonitor::getNewTimer("Belos: Operation Op*x")),
    timerPrec_(Teuchos::TimeMonitor::getNewTimer("Belos: Operation Prec*x")),
    default_blocksize_(1),
    blocksize_(1),
    num_to_solve_(0),
    rhs_index_(0),  
    Left_Prec_(false),
    Right_Prec_(false),
    Left_Scale_(false),
    Right_Scale_(false),
    isHermitian_(false),
    solutionUpdated_(false),
    solutionFinal_(true),
    initresidsComputed_(false)
  {
  }
  
  template <class ScalarType, class MV, class OP>
  LinearProblem<ScalarType,MV,OP>::LinearProblem(const RefCountPtr<const OP> &A, 
						 const RefCountPtr<MV> &X, 
						 const RefCountPtr<const MV> &B
						 ) :
    A_(A),
    X_(X),
    B_(B),
    timerOp_(Teuchos::TimeMonitor::getNewTimer("Belos: Operation Op*x")),
    timerPrec_(Teuchos::TimeMonitor::getNewTimer("Belos: Operation Prec*x")),
    default_blocksize_(1),
    blocksize_(1),
    num_to_solve_(1),
    rhs_index_(0),
    Left_Prec_(false),
    Right_Prec_(false),
    Left_Scale_(false),
    Right_Scale_(false),
    isHermitian_(false),
    solutionUpdated_(false),
    solutionFinal_(true),
    initresidsComputed_(false)
  {
    R0_ = MVT::Clone( *X_, MVT::GetNumberVecs( *X_ ) );
  }
  
  template <class ScalarType, class MV, class OP>
  LinearProblem<ScalarType,MV,OP>::LinearProblem(const LinearProblem<ScalarType,MV,OP>& Problem) :
    A_(Problem.A_),
    X_(Problem.X_),
    CurX_(Problem.CurX_),
    B_(Problem.B_),
    CurB_(Problem.CurB_),
    R_(Problem.R_),
    R0_(Problem.R0_),
    LP_(Problem.LP_),
    RP_(Problem.RP_),
    PL_(Problem.PL_),
    timerOp_(Problem.timerOp_),
    timerPrec_(Problem.timerPrec_),
    default_blocksize_(Problem.default_blocksize_),
    blocksize_(Problem.blocksize_),
    num_to_solve_(Problem.num_to_solve_),
    rhs_index_(Problem.rhs_index_),
    Left_Prec_(Problem.Left_Prec_),
    Right_Prec_(Problem.Right_Prec_),
    Left_Scale_(Problem.Left_Scale_),
    Right_Scale_(Problem.Right_Scale_),
    isHermitian_(Problem.isHermitian_),
    solutionUpdated_(Problem.solutionUpdated_),
    solutionFinal_(Problem.solutionFinal_),
    initresidsComputed_(Problem.initresidsComputed_)
  {
  }
  
  template <class ScalarType, class MV, class OP>
  LinearProblem<ScalarType,MV,OP>::~LinearProblem(void)
  {}
  
  template <class ScalarType, class MV, class OP>
  void LinearProblem<ScalarType,MV,OP>::setUpBlocks()
  {
    // Compute the new block linear system.
    // ( first clean up old linear system )
    if (CurB_.get()) CurB_ = null;
    if (CurX_.get()) CurX_ = null;
    if (R_.get()) R_ = null;
    //
    // Determine how many linear systems are left to solve for and populate LHS and RHS vector.
    // If the number of linear systems left are less than the current blocksize, then
    // we create a multivector and copy the left over LHS and RHS vectors into them.
    // The rest of the multivector is populated with random vectors (RHS) or zero vectors (LHS).
    //
    num_to_solve_ = MVT::GetNumberVecs(*X_) - rhs_index_;
    //
    // Return the NULL pointer if we don't have any more systems to solve for.
    if ( num_to_solve_ <= 0 ) { return; }  
    //
    int i;
    std::vector<int> index( num_to_solve_ );
    for ( i=0; i<num_to_solve_; i++ ) { index[i] = rhs_index_ + i; }
    //
/*    if ( num_to_solve_ < default_blocksize_ )
      blocksize_ = num_to_solve_;
    else
      blocksize_ = default_blocksize_;
*/
    //
    if ( num_to_solve_ < blocksize_ ) 
      {
	std::vector<int> index2(num_to_solve_);
	for (i=0; i<num_to_solve_; i++) {
	  index2[i] = i;
	}
	//
	// First create multivectors of blocksize and fill the RHS with random vectors LHS with zero vectors.
	CurX_ = MVT::Clone( *X_, blocksize_ );
	MVT::MvInit(*CurX_);
	CurB_ = MVT::Clone( *B_, blocksize_ );
	MVT::MvRandom(*CurB_);
	R_ = MVT::Clone( *X_, blocksize_);
	//
	RefCountPtr<const MV> tptr = MVT::CloneView( *B_, index );
	MVT::SetBlock( *tptr, index2, *CurB_ );
	//
	RefCountPtr<MV> tptr2 = MVT::CloneView( *X_, index );
	MVT::SetBlock( *tptr2, index2, *CurX_ );
      } else { 
	//
	// If the number of linear systems left are more than or equal to the current blocksize, then
	// we create a view into the LHS and RHS.
	//
	num_to_solve_ = blocksize_;
	index.resize( num_to_solve_ );
	for ( i=0; i<num_to_solve_; i++ ) { index[i] = rhs_index_ + i; }
	CurX_ = MVT::CloneView( *X_, index );
	CurB_ = rcp_const_cast<MV>(MVT::CloneView( *B_, index ));
	R_ = MVT::Clone( *X_, num_to_solve_ );
	//
      }
    //
    // Compute the current residual.
    // 
    if (R_.get()) {
      OPT::Apply( *A_, *CurX_, *R_ );
      MVT::MvAddMv( 1.0, *CurB_, -1.0, *R_, *R_ );
      solutionUpdated_ = false;
    }
  }
  
  template <class ScalarType, class MV, class OP>
  void LinearProblem<ScalarType,MV,OP>::setLHS(const RefCountPtr<MV> &X)
  {
    X_ = X; 
    R0_ = MVT::Clone( *X_, MVT::GetNumberVecs( *X_ ) ); 
  }
  
  template <class ScalarType, class MV, class OP>
  void LinearProblem<ScalarType,MV,OP>::setCurrLSVec() 
  { 
    int i;
    //
    // We only need to copy the solutions back if the linear systems of
    // interest are less than the block size.
    //
    if (num_to_solve_ < blocksize_) {
      //
      std::vector<int> index( num_to_solve_ );
      //
      RefCountPtr<MV> tptr;
      //
      // Get a view of the current solutions and correction vector.
      //
      for (i=0; i<num_to_solve_; i++) { 
	index[i] = i;	
      }
      tptr = MVT::CloneView( *CurX_, index );
      //
      // Copy the correction vector to the solution vector.
      //
      for (i=0; i<num_to_solve_; i++) { 
	index[i] = rhs_index_ + i; 
      }
      MVT::SetBlock( *tptr, index, *X_ );
    }
    //
    // Get the linear problem ready to determine the next linear system.
    //
    solutionFinal_ = true; 
    rhs_index_ += num_to_solve_; 
  }
  

  template <class ScalarType, class MV, class OP>
  RefCountPtr<MV> LinearProblem<ScalarType,MV,OP>::updateSolution( const RefCountPtr<MV>& update, 
								   bool updateLP,
								   ScalarType scale )
  { 
    RefCountPtr<MV> newSoln;
    if (update != Teuchos::null) {
      if (updateLP == true) {
	if (Right_Prec_) {
	  //
	  // Apply the right preconditioner before computing the current solution.
	  RefCountPtr<MV> TrueUpdate = MVT::Clone( *update, MVT::GetNumberVecs( *update ) );
	  OPT::Apply( *RP_, *update, *TrueUpdate ); 
	  MVT::MvAddMv( 1.0, *CurX_, scale, *TrueUpdate, *CurX_ ); 
	} 
	else {
	  MVT::MvAddMv( 1.0, *CurX_, scale, *update, *CurX_ ); 
	}
	solutionUpdated_ = true; 
	newSoln = CurX_;
      }
      else {
	newSoln = MVT::Clone( *update, MVT::GetNumberVecs( *update ) );
	if (Right_Prec_) {
	  //
	  // Apply the right preconditioner before computing the current solution.
	  RefCountPtr<MV> trueUpdate = MVT::Clone( *update, MVT::GetNumberVecs( *update ) );
	  OPT::Apply( *RP_, *update, *trueUpdate ); 
	  MVT::MvAddMv( 1.0, *CurX_, scale, *trueUpdate, *newSoln ); 
	} 
	else {
	  MVT::MvAddMv( 1.0, *CurX_, scale, *update, *newSoln ); 
	}
      }
    }
    else {
      newSoln = CurX_;
    }
    return newSoln;
  }
  

  template <class ScalarType, class MV, class OP>
  void LinearProblem<ScalarType,MV,OP>::reset( const RefCountPtr<MV> &newX, const RefCountPtr<const MV> &newB )
  {
    solutionUpdated_ = false;
    solutionFinal_ = true;
    initresidsComputed_ = false;
    rhs_index_ = 0;
    
    X_ = newX;
    B_ = newB;
    getInitResVec();
  }
  
  template <class ScalarType, class MV, class OP>
  const MV& LinearProblem<ScalarType,MV,OP>::getInitResVec() 
  {
    // Compute the initial residual if it hasn't been computed
    // and all the components of the linear system are there.
    // The left preconditioner will be applied if it exists, resulting
    // in a preconditioned residual.
    if (!initresidsComputed_ && A_.get() && X_.get() && B_.get()) 
      {
	if (R0_==Teuchos::null || MVT::GetNumberVecs( *R0_ )!=MVT::GetNumberVecs( *X_ )) {
	  R0_ = MVT::Clone( *X_, MVT::GetNumberVecs( *X_ ) );
	}
	OPT::Apply( *A_, *X_, *R0_ );
	MVT::MvAddMv( 1.0, *B_, -1.0, *R0_, *R0_ );
	initresidsComputed_ = true;
      }
    return (*R0_);
  }
  
  template <class ScalarType, class MV, class OP>
  const MV& LinearProblem<ScalarType,MV,OP>::getCurrResVec( const MV* CurrSoln ) 
  {
    // Compute the residual of the current linear system.
    // This should be used if the solution has been updated.
    // Alternatively, if the current solution has been computed by GMRES
    // this can be passed in and the current residual will be updated using
    // it.
    //
    if (solutionUpdated_) 
      {
	OPT::Apply( *A_, *getCurrLHSVec(), *R_ );
	MVT::MvAddMv( 1.0, *getCurrRHSVec(), -1.0, *R_, *R_ ); 
	solutionUpdated_ = false;
      }
    else if (CurrSoln) 
      {
	OPT::Apply( *A_, *CurrSoln, *R_ );
	MVT::MvAddMv( 1.0, *getCurrRHSVec(), -1.0, *R_, *R_ ); 
      }
    return (*R_);
  }
  
  template <class ScalarType, class MV, class OP>
  RefCountPtr<MV> LinearProblem<ScalarType,MV,OP>::getCurrLHSVec()
  {
    if (solutionFinal_) {
      solutionFinal_ = false;	// make sure we don't populate the current linear system again.
      setUpBlocks();
    }
    return CurX_; 
  }
  
  template <class ScalarType, class MV, class OP>
  RefCountPtr<MV> LinearProblem<ScalarType,MV,OP>::getCurrRHSVec()
  {
    if (solutionFinal_) {
      solutionFinal_ = false;	// make sure we don't populate the current linear system again.
      setUpBlocks();
    }
    return CurB_;
  }
  
  template <class ScalarType, class MV, class OP>
  void LinearProblem<ScalarType,MV,OP>::apply( const MV& x, MV& y ) const
  {
    RefCountPtr<MV> ytemp = MVT::Clone( y, MVT::GetNumberVecs( y ) );
    //
    // No preconditioning.
    // 
    if (!Left_Prec_ && !Right_Prec_){ 
      Teuchos::TimeMonitor OpTimer(*timerOp_);
      OPT::Apply( *A_, x, y );
    }
    //
    // Preconditioning is being done on both sides
    //
    else if( Left_Prec_ && Right_Prec_ ) 
      {
        {
          Teuchos::TimeMonitor PrecTimer(*timerPrec_);
	  OPT::Apply( *RP_, x, y );   
        }
        {
          Teuchos::TimeMonitor OpTimer(*timerOp_);
	  OPT::Apply( *A_, y, *ytemp );
        }
        {
          Teuchos::TimeMonitor PrecTimer(*timerPrec_);
	  OPT::Apply( *LP_, *ytemp, y );
        }
      }
    //
    // Preconditioning is only being done on the left side
    //
    else if( Left_Prec_ ) 
      {
        {
          Teuchos::TimeMonitor PrecTimer(*timerPrec_);
	  OPT::Apply( *A_, x, *ytemp );
        }
        {
          Teuchos::TimeMonitor OpTimer(*timerOp_);
	  OPT::Apply( *LP_, *ytemp, y );
        }
      }
    //
    // Preconditioning is only being done on the right side
    //
    else 
      {
        {
          Teuchos::TimeMonitor PrecTimer(*timerPrec_);
	  OPT::Apply( *RP_, x, *ytemp );
        }
        {
          Teuchos::TimeMonitor OpTimer(*timerOp_);
      	  OPT::Apply( *A_, *ytemp, y );
        }
      }  
  }
  
  template <class ScalarType, class MV, class OP>
  void LinearProblem<ScalarType,MV,OP>::applyOp( const MV& x, MV& y ) const {
    if (A_.get()) {
      Teuchos::TimeMonitor OpTimer(*timerOp_);
      OPT::Apply( *A_,x, y);   
    }
    else {
      MVT::MvAddMv( Teuchos::ScalarTraits<ScalarType>::one(), x, 
		    Teuchos::ScalarTraits<ScalarType>::zero(), x, y );
    }
  }
  
  template <class ScalarType, class MV, class OP>
  void LinearProblem<ScalarType,MV,OP>::applyLeftPrec( const MV& x, MV& y ) const {
    if (Left_Prec_) {
      Teuchos::TimeMonitor PrecTimer(*timerPrec_);
      return ( OPT::Apply( *LP_,x, y) );
    }
    else {
      MVT::MvAddMv( Teuchos::ScalarTraits<ScalarType>::one(), x, 
		    Teuchos::ScalarTraits<ScalarType>::zero(), x, y );
    }
  }
  
  template <class ScalarType, class MV, class OP>
  void LinearProblem<ScalarType,MV,OP>::applyRightPrec( const MV& x, MV& y ) const {
    if (Right_Prec_) {
      Teuchos::TimeMonitor PrecTimer(*timerPrec_);
      return ( OPT::Apply( *RP_,x, y) );
    }
    else {
      MVT::MvAddMv( Teuchos::ScalarTraits<ScalarType>::one(), x, 
		    Teuchos::ScalarTraits<ScalarType>::zero(), x, y );
    }
  }
  
  template <class ScalarType, class MV, class OP>
  void LinearProblem<ScalarType,MV,OP>::computeCurrResVec( MV* R, const MV* X, const MV* B ) const {

    if (R) {
      if (X && B) // The entries are specified, so compute the residual of Op(A)X = B
	{
	  if (Left_Prec_)
	    {
	      RefCountPtr<MV> R_temp = MVT::Clone( *X, MVT::GetNumberVecs( *X ) );
	      OPT::Apply( *A_, *X, *R_temp );
	      MVT::MvAddMv( -1.0, *R_temp, 1.0, *B, *R_temp );
	      OPT::Apply( *LP_, *R_temp, *R );
	    }
	  else 
	    {
	      OPT::Apply( *A_, *X, *R );
	      MVT::MvAddMv( -1.0, *R, 1.0, *B, *R );
	    }
	}
      else { 
	// The solution and right-hand side may not be specified, check and use which ones exist.
	RefCountPtr<const MV> localB, localX;
	if (B)
	  localB = rcp( B, false );
	else
	  localB = CurB_;
	
	if (X)
	  localX = rcp( X, false );
	else
	  localX = CurX_;
	
	if (Left_Prec_)
	  {
	    RefCountPtr<MV> R_temp = MVT::Clone( *localX, MVT::GetNumberVecs( *localX ) );
	    OPT::Apply( *A_, *localX, *R_temp );
	    MVT::MvAddMv( -1.0, *R_temp, 1.0, *localB, *R_temp );
	    OPT::Apply( *LP_, *R_temp, *R );
	  }
	else 
	  {
	    OPT::Apply( *A_, *localX, *R );
	    MVT::MvAddMv( -1.0, *R, 1.0, *localB, *R );
	  }
      }    
    }
  }
  
  
  template <class ScalarType, class MV, class OP>
  void LinearProblem<ScalarType,MV,OP>::computeActualResVec( MV* R, const MV* X, const MV* B ) const {

    if (R) {
      if (X && B) // The entries are specified, so compute the residual of Op(A)X = B
	{
	  OPT::Apply( *A_, *X, *R );
	  MVT::MvAddMv( -1.0, *R, 1.0, *B, *R );
	}
      else { 
	// The solution and right-hand side may not be specified, check and use which ones exist.
	RefCountPtr<const MV> localB, localX;
	if (B)
	  localB = rcp( B, false );
	else
	  localB = CurB_;
	
	if (X)
	  localX = rcp( X, false );
	else
	  localX = CurX_;
	
	OPT::Apply( *A_, *localX, *R );
	MVT::MvAddMv( -1.0, *R, 1.0, *localB, *R );
      }    
    }
  }
  
} // end Belos namespace

#endif /* BELOS_LINEAR_PROBLEM_HPP */


