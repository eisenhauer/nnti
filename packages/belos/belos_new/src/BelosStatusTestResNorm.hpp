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

#ifndef BELOS_STATUS_TEST_RESNORM_H
#define BELOS_STATUS_TEST_RESNORM_H

/*!
  \file BelosStatusTestResNorm.hpp
  \brief Belos::StatusTest for specifying a residual norm stopping criteria.
*/

#include "BelosStatusTest.hpp"
#include "BelosLinearProblem.hpp"
#include "BelosMultiVecTraits.hpp"

/*! 
  \class Belos::StatusTestResNorm
  \brief An implementation of StatusTest using a family of residual norms.

  StatusTestResNorm is an implementation of StatusTest that allows a user to construct
  one of a family of residual tests for use as a status/convergence test for Belos.  
  The form of the test is
   \f[
   \frac{\|r_i\|}{\sigma_i} \le \tau
   \f]
   where 
   <ul>
   <li> \f$r_i\f$ is the i-th residual vector, implicitly or explicitly computed (determined by enum ResType),
   <li> \f$\|r_i\|\f$ is the i-th residual norm determined by the enum NormType  (1-norm, 2-norm or inf-norm), 
   <li> \f$\sigma_i\f$ is the i-th scale factor that can be passed in as a precomputed number of the templated type, 
   or can be selected from by the enum ScaleType (norm of RHS, norm of initial residual).
   <li> \f$\tau\f$ is the tolerance that is passed in as a number of the templated type to the constructor.  
   The value of \f$\tau\f$ can be reset using the ResetTolerance() method.
   </ul>

*/

namespace Belos {

template <class ScalarType, class MV, class OP>
class StatusTestResNorm: public StatusTest<ScalarType,MV,OP> {

 public:

  // Convenience typedefs
  typedef Teuchos::ScalarTraits<ScalarType> SCT;
  typedef typename SCT::magnitudeType MagnitudeType;
  typedef MultiVecTraits<ScalarType,MV>  MVT;

  //! @name Enums.
  //@{ 
  /*! 
    \brief Select how the residual vector is produced.
  */
  enum ResType {Implicit, /*!< Use the residual vector produced by the iterative solver. */
		Explicit  /*!< Explicitly compute the residual vector r = b - A*x using the 
			    linear problem. */
  };
  /*! 

    \brief Select the scale type.
  */
  enum ScaleType {NormOfRHS,     /*!< Use the norm of the right-hand-side. */
		  NormOfInitRes, /*!< Use the initial residual vector (explicitly computed). */
		  NormOfPrecInitRes, /*!< Use the preconditioned initial residual vector (explicitly computed). */
		  None,          /*!< Use unscaled residual. */
		  UserProvided   /*!< User provides an explicit value that the norm of 
				   the residual will be divided by. */
  };
  //@}

  //! @name Constructors/destructors.
  //@{ 
  //! Constructor
  /*! The constructor takes a single argument specifying the tolerance (\f$\tau\f$).  
    If none of the form definition methods are called, we use \f$\|r\|_2/\|r^{(0)}\|_2 \le \tau\f$ 
    as the stopping criterion, where \f$\|r\|_2\f$ uses the least costly form of the 2-norm of 
    residual available from the iterative method and \f$\|r^{(0)}\|_2\f$ is the corresponding norm 
    of the initial residual.  The least costly form of the 2-norm depends on the chosen iterative 
    method.  Most Krylov methods produce the preconditioned residual vector in a form that would be 
    exact in infinite precision arithmetic.  This vector may be different from the true residual 
    either because left scaling or preconditioning was used, or because round-off error has 
    introduced significant error, or both.

    You can also state the number of vectors that must pass the convergence criteria before the 
    status test passes by using the \c quorum argument.
  */
  StatusTestResNorm( MagnitudeType Tolerance, int quorum = -1, bool showMaxResNormOnly = false );

  //! Destructor
  virtual ~StatusTestResNorm();
  //@}

  //! @name Form and parameter definition methods.
  //@{ 

  //! Define form of the residual, its norm and optional weighting vector.
  /*! This method defines the form of \f$\|r\|\f$.  We specify:
    <ul>
    <li> Whether the residual vector should be explicitly computed, or taken from the iterative method.
    <li> The norm to be used on the residual (this may be different than the norm used in 
    DefineScaleForm()).
    </ul>
  */
  int defineResForm( ResType TypeOfResidual, NormType TypeOfNorm);
  
  //! Define form of the scaling, its norm, its optional weighting vector, or, alternatively, define an explicit value.
  /*! This method defines the form of how the residual is scaled (if at all).  It operates in two modes:
    <ol>
    <li> User-provided scaling value:
    <ul> 
    <li> Set argument TypeOfScaling to UserProvided.
    <li> Set ScaleValue to a non-zero value that the residual norm will be divided by.
    <li> TypeOfNorm argument will be ignored.
    <li> Sample use:  Define ScaleValue = \f$\|A\|_{\infty}\f$ where \f$ A \f$ is the matrix 
    of the linear problem.
    </ul>
    
    <li> Use a supported Scaling Form:
    <ul>
    <li> Define TypeOfScaling to be the norm of the right hand side, the initial residual vector, 
    or to none.
    <li> Define norm to be used on the scaling vector (this may be different than the norm used 
    in DefineResForm()).
    </ul>
    </ol>
  */
  int defineScaleForm( ScaleType TypeOfScaling, NormType TypeOfNorm, MagnitudeType ScaleValue = Teuchos::ScalarTraits<MagnitudeType>::one());

  //! Set the value of the tolerance
  /*! We allow the tolerance to be reset for cases where, in the process of testing the residual, 
    we find that the initial tolerance was too tight or too lax.
  */
  int setTolerance(MagnitudeType tolerance) {tolerance_ = tolerance; return(0);}

  //! Sets the number of residuals that must pass the convergence test before Passed is returned.
  //! \note If \c quorum=-1 then all residuals must pass the convergence test before Passed is returned.
  int setQuorum(int quorum) {quorum_ = quorum; return(0);}

  //! Set whether the only maximum residual norm is displayed when the print() method is called
  int setShowMaxResNormOnly(bool showMaxResNormOnly) {showMaxResNormOnly_ = showMaxResNormOnly; return(0);}

  //@}

  //! @name Status methods
  //@{ 
  //! Check convergence status: Passed, Failed, or Undefined.
  /*! This method checks to see if the convergence criteria are met.  
    Depending on how the residual test is constructed this method will return 
    the appropriate status type.

    \return StatusType: Passed, Failed, or Undefined.
  */
  StatusType checkStatus(Iteration<ScalarType,MV,OP>* iSolver);

  //! Return the result of the most recent CheckStatus call.
  StatusType getStatus() const {return(status_);};
  //@}

  //! @name Reset methods
  //@{ 
 
  //! Resets the internal configuration to the initial state.
  void reset();

  //@}

  //! @name Print methods
  //@{ 

  //! Output formatted description of stopping test to output stream.
  void print(ostream& os, int indent = 0) const;

  //! Print message for each status specific to this stopping test.
  void printStatus(ostream& os, StatusType type) const; 
  //@}

  //! @name Methods to access data members.
  //@{ 

  //! Returns the number of residuals that must pass the convergence test before Passed is returned.
  //! \note If \c quorum=-1 then all residuals must pass the convergence test before Passed is returned.
  int getQuorum() { return quorum_; }

  //! Returns the vector containing the indices of the residuals that passed the test.
  std::vector<int> convIndices() { return ind_; }

  //! Returns the value of the tolerance, \f$ \tau \f$, set in the constructor.
  MagnitudeType getTolerance() const {return(tolerance_);};
  
  //! Returns the test value, \f$ \frac{\|r\|}{\sigma} \f$, computed in most recent call to CheckStatus.
  const std::vector<MagnitudeType>* getTestValue() const {return(&testvector_);};

  //! Returns the residual norm value, \f$ \|r\| \f$, computed in most recent call to CheckStatus.
  const std::vector<MagnitudeType>* getResNormValue() const {return(&resvector_);};

  //! Returns the scaled norm value, \f$ \sigma \f$.
  const std::vector<MagnitudeType>* getScaledNormValue() const {return(&scalevector_);};

  //@}


  /** @name Misc. */
  //@{

  /** \brief Call to setup initial scaling vector.
   *
   * After this function is called <tt>getScaledNormValue()</tt> can be called
   * to get the scaling vector.
   */
  StatusType firstCallCheckStatusSetup(Iteration<ScalarType,MV,OP>* iSolver);

 protected:

 private:

  //! @name Private data members.
  //@{ 
  
  //! Tolerance used to determine convergence
  MagnitudeType tolerance_;

  //! Number of residuals that must pass the convergence test before Passed is returned.
  int quorum_;

  //! Determines if the entries for all of the residuals are shown or just the max.
  bool showMaxResNormOnly_;
 
  //! Type of residual to use (explicit or implicit)
  ResType restype_;
  
  //! Type of norm to use on residual (OneNorm, TwoNorm, or InfNorm).
  NormType resnormtype_;
  
  //! Type of scaling to use (Norm of RHS, Norm of Initial Residual, None or User provided)
  ScaleType scaletype_;
  
  //! Type of norm to use on the scaling (OneNorm, TwoNorm, or InfNorm)
  NormType scalenormtype_;

  //! Scaling value.
  MagnitudeType scalevalue_;

  //! Scaling vector.
  std::vector<MagnitudeType> scalevector_;
  
  //! Residual norm vector.
  std::vector<MagnitudeType> resvector_;

  //! Test vector = resvector_ / scalevector_
  std::vector<MagnitudeType> testvector_;

  //! Vector containing the indices for the vectors that passed the test.
  std::vector<int> ind_;
  
  //! Status
  StatusType status_;
  
  //! The current blocksize of the linear system being solved.
  int curBlksz_;

  //! The current number of right-hand sides being solved for.
  int curNumRHS_;

  //! The indices of the current number of right-hand sides being solved for.
  std::vector<int> curLSIdx_;

  //! The current number of linear systems that have been loaded into the linear problem.
  int curLSNum_;

  //! The total number of right-hand sides being solved for.
  int numrhs_;

  //! Is this the first time CheckStatus is called?
  bool firstcallCheckStatus_;

  //! Is this the first time DefineResForm is called?
  bool firstcallDefineResForm_;

  //! Is this the first time DefineScaleForm is called?
  bool firstcallDefineScaleForm_;

  //@}

};

template <class ScalarType, class MV, class OP>
StatusTestResNorm<ScalarType,MV,OP>::StatusTestResNorm( MagnitudeType Tolerance, int quorum, bool showMaxResNormOnly )
  : tolerance_(Tolerance),
    quorum_(quorum),
    showMaxResNormOnly_(showMaxResNormOnly),
    restype_(Implicit),
    resnormtype_(TwoNorm),	
    scaletype_(NormOfInitRes),
    scalenormtype_(TwoNorm),
    scalevalue_(1.0),
    status_(Undefined),
    curBlksz_(0),
    curLSNum_(0),
    numrhs_(0),
    firstcallCheckStatus_(true),
    firstcallDefineResForm_(true),
    firstcallDefineScaleForm_(true)
{
  // This constructor will compute the residual ||r_i||/||r0_i|| <= tolerance using the 2-norm of
  // the implicit residual vector.
}

template <class ScalarType, class MV, class OP>
StatusTestResNorm<ScalarType,MV,OP>::~StatusTestResNorm() 
{}

template <class ScalarType, class MV, class OP>
void StatusTestResNorm<ScalarType,MV,OP>::reset() 
{
  status_ = Undefined;
  curBlksz_ = 0;
  curLSNum_ = 0;
  curLSIdx_.resize(0);
  numrhs_ = 0;
  ind_.resize(0);
  firstcallCheckStatus_ = true;
}

template <class ScalarType, class MV, class OP>
int StatusTestResNorm<ScalarType,MV,OP>::defineResForm( ResType TypeOfResidual, NormType TypeOfNorm )
{    
  TEST_FOR_EXCEPTION(firstcallDefineResForm_==false,StatusTestError,
	"StatusTestResNorm::defineResForm(): The residual form has already been defined.");
  firstcallDefineResForm_ = false;
    
  restype_ = TypeOfResidual;
  resnormtype_ = TypeOfNorm;
    
  return(0);
}

template <class ScalarType, class MV, class OP> 
int StatusTestResNorm<ScalarType,MV,OP>::defineScaleForm(ScaleType TypeOfScaling, NormType TypeOfNorm,
                                                         MagnitudeType ScaleValue )
{
  TEST_FOR_EXCEPTION(firstcallDefineScaleForm_==false,StatusTestError,
	"StatusTestResNorm::defineScaleForm(): The scaling type has already been defined.");
  firstcallDefineScaleForm_ = false;
    
  scaletype_ = TypeOfScaling;
  scalenormtype_ = TypeOfNorm;
  scalevalue_ = ScaleValue;
    
  return(0);
}

template <class ScalarType, class MV, class OP>
StatusType StatusTestResNorm<ScalarType,MV,OP>::checkStatus( Iteration<ScalarType,MV,OP>* iSolver )
{
  MagnitudeType zero = Teuchos::ScalarTraits<MagnitudeType>::zero();
  const LinearProblem<ScalarType,MV,OP>& lp = iSolver->getProblem();
  // Compute scaling term (done once for each block that's being solved)
  if (firstcallCheckStatus_) {
    StatusType status = firstCallCheckStatusSetup(iSolver);
    if(status==Failed) {
      status_ = Failed;
      return(status_);
    }
  }
  //
  // This section computes the norm of the residual vector
  //
  if ( curLSNum_ != lp.getLSNumber() ) {
    //
    // We have moved on to the next rhs block
    //
    curLSNum_ = lp.getLSNumber();
    curLSIdx_ = lp.getLSIndex();
    curBlksz_ = (int)curLSIdx_.size();
    int validLS = 0;
    for (int i=0; i<curBlksz_; ++i) {
      if (curLSIdx_[i] > -1 && curLSIdx_[i] < numrhs_) 
	validLS++;
    }
    curNumRHS_ = validLS; 
    //
  } else {
    //
    // We are in the same rhs block, return if we are converged
    //
    if (status_==Passed) { return status_; }
  }
  if (restype_==Implicit) {
    //
    // get the native residual norms from the solver for this block of right-hand sides.
    // If the residual is returned in multivector form, use the resnormtype to compute the residual norms.
    // Otherwise the native residual is assumed to be stored in the resvector_.
    //
    std::vector<MagnitudeType> tmp_resvector( curBlksz_ );
    RefCountPtr<const MV> residMV = iSolver->getNativeResiduals( &tmp_resvector );     
    if ( residMV != Teuchos::null ) { 
      tmp_resvector.resize( MVT::GetNumberVecs( *residMV ) );
      MVT::MvNorm( *residMV, &tmp_resvector, resnormtype_ );    
      typename std::vector<int>::iterator p = curLSIdx_.begin();
      for (int i=0; p<curLSIdx_.end(); ++p, ++i) {
        // Check if this index is valid
        if (*p != -1)  
          resvector_[*p] = tmp_resvector[i]; 
      }
    } else {
      typename std::vector<int>::iterator p = curLSIdx_.begin();
      for (int i=0; p<curLSIdx_.end(); ++p, ++i) {
        // Check if this index is valid
        if (*p != -1)
          resvector_[*p] = tmp_resvector[i];
      }
    }
  }
  else if (restype_==Explicit) {
    //
    // Request the true residual for this block of right-hand sides.
    //
    RefCountPtr<MV> cur_update = iSolver->getCurrentUpdate();
    RefCountPtr<MV> cur_soln = lp.updateSolution( cur_update );
    RefCountPtr<MV> cur_res = MVT::Clone( *cur_soln, MVT::GetNumberVecs( *cur_soln ) );
    lp.computeActualResVec( &*cur_res, &*cur_soln );
    std::vector<MagnitudeType> tmp_resvector( MVT::GetNumberVecs( *cur_res ) );
    MVT::MvNorm( *cur_res, &tmp_resvector, resnormtype_ );
    typename std::vector<int>::iterator p = curLSIdx_.begin();
    for (int i=0; p<curLSIdx_.end(); ++p, ++i) {
      // Check if this index is valid
      if (*p != -1)
        resvector_[*p] = tmp_resvector[i];
    }
  }
  //
  // Compute the new linear system residuals for testing.
  // (if any of them don't meet the tolerance or are NaN, then we exit with that status)
  //
  if ( scalevector_.size() > 0 ) {
    typename std::vector<int>::iterator p = curLSIdx_.begin();
    for (; p<curLSIdx_.end(); ++p) {
      // Check if this index is valid
      if (*p != -1) {     
        // Scale the vector accordingly
        if ( scalevector_[ *p ] != zero ) {
          // Don't intentionally divide by zero.
          testvector_[ *p ] = resvector_[ *p ] / scalevector_[ *p ] / scalevalue_;
        } else {
          testvector_[ *p ] = resvector_[ *p ] / scalevalue_;
        }
      }
    }
  }
  else {
    typename std::vector<int>::iterator p = curLSIdx_.begin();
    for (; p<curLSIdx_.end(); ++p) {
      // Check if this index is valid
      if (*p != -1)     
        testvector_[ *p ] = resvector_[ *p ] / scalevalue_;
    }
  }	

  // Check status of new linear system residuals and see if we have the quorum.
  int have = 0;
  ind_.resize( curLSIdx_.size() );
  typename std::vector<int>::iterator p = curLSIdx_.begin();
  for (; p<curLSIdx_.end(); ++p) {
    // Check if this index is valid
    if (*p != -1) {     
      // Check if any of the residuals are larger than the tolerance.
      if (testvector_[ *p ] > tolerance_) {
        // do nothing.
      } else if (testvector_[ *p ] <= tolerance_) { 
        ind_[have] = *p;
        have++;
      } else {
        // Throw an exception if a NaN is found.
        status_ = Failed;
        TEST_FOR_EXCEPTION(true,StatusTestError,"StatusTestResNorm::checkStatus(): NaN has been detected.");
      }
    }
  } 
  ind_.resize(have);
  int need = (quorum_ == -1) ? curNumRHS_: quorum_;
  status_ = (have >= need) ? Passed : Failed;
  
  // Return the current status
  return status_;
}

template <class ScalarType, class MV, class OP>
void StatusTestResNorm<ScalarType,MV,OP>::print(ostream& os, int indent) const
{
  for (int j = 0; j < indent; j ++)
    os << ' ';
  printStatus(os, status_);
  os << "(";
  os << ((resnormtype_==OneNorm) ? "1-Norm" : (resnormtype_==TwoNorm) ? "2-Norm" : "Inf-Norm");
  os << ((restype_==Explicit) ? " Exp" : " Imp");
  os << " Res Vec) ";
  if (scaletype_!=None)
    os << "/ ";
  if (scaletype_==UserProvided)
    os << " (User Scale)";
  else {
    os << "(";
    os << ((scalenormtype_==OneNorm) ? "1-Norm" : (resnormtype_==TwoNorm) ? "2-Norm" : "Inf-Norm");
    if (scaletype_==NormOfInitRes)
      os << " Res0";
    else if (scaletype_==NormOfPrecInitRes)
      os << " Prec Res0";
    else
      os << " RHS ";
    os << ")";
  }
  if (status_==Undefined)
    os << ", tol = " << tolerance_ << endl;
  else {
    os << endl;
    if(showMaxResNormOnly_) {
      const MagnitudeType maxRelRes = *std::max_element(
        testvector_.begin()+curLSIdx_[0],testvector_.begin()+curLSIdx_[curBlksz_-1]
        );
      for (int j = 0; j < indent + 13; j ++)
        os << ' ';
      os << "max{residual["<<curLSIdx_[0]<<"..."<<curLSIdx_[curBlksz_-1]<<"]} = " << maxRelRes
         << ( maxRelRes <= tolerance_ ? " <= " : " > " ) << tolerance_ << endl;
    }
    else {
      for ( int i=0; i<numrhs_; i++ ) {
        for (int j = 0; j < indent + 13; j ++)
          os << ' ';
        os << "residual [ " << i << " ] = " << testvector_[ i ];
        os << ((testvector_[i]<tolerance_) ? " < " : (testvector_[i]==tolerance_) ? " == " : (testvector_[i]>tolerance_) ? " > " : " "  ) << tolerance_ << endl;
      }
    }
  }
  os << endl;
}

template <class ScalarType, class MV, class OP>
void StatusTestResNorm<ScalarType,MV,OP>::printStatus(ostream& os, StatusType type) const 
{
  os << setiosflags(ios::left) << setw(13) << setfill('.');
  switch (type) {
  case  Passed:
    os << "Converged";
    break;
  case  Failed:
    os << "Unconverged";
    break;
  case  Undefined:
  default:
    os << "**";
    break;
  }
  os << setiosflags(ios::left) << setfill(' ');
    return;
}

template <class ScalarType, class MV, class OP>
StatusType StatusTestResNorm<ScalarType,MV,OP>::firstCallCheckStatusSetup( Iteration<ScalarType,MV,OP>* iSolver )
{
  int i;
  MagnitudeType zero = Teuchos::ScalarTraits<MagnitudeType>::zero();
  MagnitudeType one = Teuchos::ScalarTraits<MagnitudeType>::one();
  const LinearProblem<ScalarType,MV,OP>& lp = iSolver->getProblem();
  // Compute scaling term (done once for each block that's being solved)
  if (firstcallCheckStatus_) {
    //
    // Get some current solver information.
    //
    firstcallCheckStatus_ = false;

    if (scaletype_== NormOfRHS) {
      RefCountPtr<const MV> rhs = lp.getRHS();
      numrhs_ = MVT::GetNumberVecs( *rhs );
      scalevector_.resize( numrhs_ );
      resvector_.resize( numrhs_ ); 
      testvector_.resize( numrhs_ );
      MVT::MvNorm( *rhs, &scalevector_, scalenormtype_ );
    }
    else if (scaletype_==NormOfInitRes) {
      RefCountPtr<const MV> init_res = lp.getInitResVec();
      numrhs_ = MVT::GetNumberVecs( *init_res );
      scalevector_.resize( numrhs_ );
      resvector_.resize( numrhs_ ); 
      testvector_.resize( numrhs_ );
      MVT::MvNorm( *init_res, &scalevector_, scalenormtype_ );
    }
    else if (scaletype_==NormOfPrecInitRes) {
      RefCountPtr<const MV> init_res = lp.getInitResVec();
      numrhs_ = MVT::GetNumberVecs( *init_res );
      scalevector_.resize( numrhs_ );
      resvector_.resize( numrhs_ ); 
      testvector_.resize( numrhs_ );
      if (lp.isLeftPrec()) {
        RefCountPtr<MV> prec_init_res = MVT::Clone( *init_res, numrhs_ );
        lp.applyLeftPrec( *init_res, *prec_init_res );
        MVT::MvNorm( *prec_init_res, &scalevector_, scalenormtype_ );
      }
      else { 
        MVT::MvNorm( *init_res, &scalevector_, scalenormtype_ );
      }
    }

    curLSNum_ = lp.getLSNumber();
    curLSIdx_ = lp.getLSIndex();
    curBlksz_ = (int)curLSIdx_.size();
    int validLS = 0;
    for (i=0; i<curBlksz_; ++i) {
      if (curLSIdx_[i] > -1 && curLSIdx_[i] < numrhs_)
        validLS++;
    }
    curNumRHS_ = validLS;
    //
    // Initialize the testvector.
    for (i=0; i<numrhs_; i++) { testvector_[i] = one; }

    // Return an error if the scaling is zero.
    if (scalevalue_ == zero) {
      return Failed;
    }
  }
  return Undefined;
}

} // end namespace Belos

#endif /* BELOS_STATUS_TEST_RESNORM_H */
