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


/*! \file BelosICGSOrthoManager.hpp
  \brief ICGS implementation of the Belos::OrthoManager class
*/

#ifndef BELOS_ICGS_ORTHOMANAGER_HPP
#define BELOS_ICGS_ORTHOMANAGER_HPP

/*!   \class Belos::ICGSOrthoManager
      \brief An implementation of the Belos::MatOrthoManager that performs orthogonalization
      using multiple steps of classical Gram-Schmidt.
      
      \author Chris Baker, Ulrich Hetmaniuk, Rich Lehoucq, and Heidi Thornquist
*/

// #define ORTHO_DEBUG

#include "BelosConfigDefs.hpp"
#include "BelosMultiVecTraits.hpp"
#include "BelosOperatorTraits.hpp"
#include "BelosMatOrthoManager.hpp"

namespace Belos {

  template<class ScalarType, class MV, class OP>
  class ICGSOrthoManager : public MatOrthoManager<ScalarType,MV,OP> {

  private:
    typedef typename Teuchos::ScalarTraits<ScalarType>::magnitudeType MagnitudeType;
    typedef typename Teuchos::ScalarTraits<MagnitudeType> MGT;
    typedef Teuchos::ScalarTraits<ScalarType>  SCT;
    typedef MultiVecTraits<ScalarType,MV>      MVT;
    typedef OperatorTraits<ScalarType,MV,OP>   OPT;

  public:
    
    //! @name Constructor/Destructor
    //@{ 
    //! Constructor specifying re-orthogonalization tolerance.
    ICGSOrthoManager( Teuchos::RefCountPtr<const OP> Op = Teuchos::null,
		      const int max_ortho_steps = 2,
		      const MagnitudeType blk_tol = 10*MGT::squareroot( MGT::eps() ),
		      const MagnitudeType sing_tol = 10*MGT::eps() )
      : MatOrthoManager<ScalarType,MV,OP>(Op), 
	max_ortho_steps_( max_ortho_steps ),
	blk_tol_( blk_tol ),
	sing_tol_( sing_tol ),
        timerUpdate_( Teuchos::TimeMonitor::getNewTimer("Belos: Ortho (Update)")),
        timerNorm_( Teuchos::TimeMonitor::getNewTimer("Belos: Ortho (Norm)")),
        timerInnerProd_( Teuchos::TimeMonitor::getNewTimer("Belos: Ortho (Inner Product)")) {};

    //! Destructor
    ~ICGSOrthoManager() {};
    //@}


    //! @name Accessor routines
    //@{ 

    //! Set parameter for block re-orthogonalization threshhold.
    void setBlkTol( const MagnitudeType blk_tol ) { blk_tol_ = blk_tol; }

    //! Set parameter for singular block detection.
    void setSingTol( const MagnitudeType sing_tol ) { sing_tol_ = sing_tol; }

    //! Return parameter for block re-orthogonalization threshhold.
    MagnitudeType getBlkTol() const { return blk_tol_; } 

    //! Return parameter for singular block detection.
    MagnitudeType getSingTol() const { return sing_tol_; } 

    //@} 


    //! @name Orthogonalization methods
    //@{ 

    /*! \brief Given a list of (mutually and internally) orthonormal bases \c Q, this method
     * takes a multivector \c X and projects it onto the space orthogonal to the individual <tt>Q[i]</tt>, 
     * optionally returning the coefficients of \c X for the individual <tt>Q[i]</tt>. All of this is done with respect
     * to the inner product innerProd().
     *
     * After calling this routine, \c X will be orthogonal to each of the \c <tt>Q[i]</tt>.
     *
     * The method uses either one or two steps of classical Gram-Schmidt. The algebraically 
     * equivalent projection matrix is \f$P_Q = I - Q Q^H Op\f$, if \c Op is the matrix specified for
     * use in the inner product. Note, this is not an orthogonal projector.
     *
     @param X [in/out] The multivector to be modified.
       On output, \c X will be orthogonal to <tt>Q[i]</tt> with respect to innerProd().

     @param MX [in/out] The image of \c X under the operator \c Op. 
       If \f$ MX != 0\f$: On input, this is expected to be consistent with \c X. On output, this is updated consistent with updates to \c X.
       If \f$ MX == 0\f$ or \f$ Op == 0\f$: \c MX is not referenced.

     @param C [out] The coefficients of \c X in the \c *Q[i], with respect to innerProd(). If <tt>C[i]</tt> is a non-null pointer 
       and \c *C[i] matches the dimensions of \c X and \c *Q[i], then the coefficients computed during the orthogonalization
       routine will be stored in the matrix \c *C[i]. If <tt>C[i]</tt> is a non-null pointer whose size does not match the dimensions of 
       \c X and \c *Q[i], then a std::invalid_argument exception will be thrown. Otherwise, if <tt>C.size() < i</tt> or <tt>C[i]</tt> is a null
       pointer, then the orthogonalization manager will declare storage for the coefficients and the user will not have access to them.

     @param Q [in] A list of multivector bases specifying the subspaces to be orthogonalized against. Each <tt>Q[i]</tt> is assumed to have
     orthonormal columns, and the <tt>Q[i]</tt> are assumed to be mutually orthogonal.
    */
    void project ( MV &X, Teuchos::RefCountPtr<MV> MX, 
                   Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
                   Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q) const;


    /*! \brief This method calls project(X,Teuchos::null,C,Q); see documentation for that function.
    */
    void project ( MV &X, 
                   Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
                   Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q) const {
      project(X,Teuchos::null,C,Q);
    }


 
    /*! \brief This method takes a multivector \c X and attempts to compute an orthonormal basis for \f$colspan(X)\f$, with respect to innerProd().
     *
     * The method uses classical Gram-Schmidt, so that the coefficient matrix \c B is upper triangular.
     *
     * This routine returns an integer \c rank stating the rank of the computed basis. If \c X does not have full rank and the normalize() routine does 
     * not attempt to augment the subspace, then \c rank may be smaller than the number of columns in \c X. In this case, only the first \c rank columns of 
     * output \c X and first \c rank rows of \c B will be valid.
     *  
     * The method attempts to find a basis with dimension the same as the number of columns in \c X. It does this by augmenting linearly dependant 
     * vectors in \c X with random directions. A finite number of these attempts will be made; therefore, it is possible that the dimension of the 
     * computed basis is less than the number of vectors in \c X.
     *
     @param X [in/out] The multivector to the modified. 
       On output, \c X will have some number of orthonormal columns (with respect to innerProd()).

     @param MX [in/out] The image of \c X under the operator \c Op. 
       If \f$ MX != 0\f$: On input, this is expected to be consistent with \c X. On output, this is updated consistent with updates to \c X.
       If \f$ MX == 0\f$ or \f$ Op == 0\f$: \c MX is not referenced.

     @param B [out] The coefficients of the original \c X with respect to the computed basis. The first rows in \c B
            corresponding to the valid columns in \c X will be upper triangular.

     @return Rank of the basis computed by this method.
    */
    int normalize ( MV &X, Teuchos::RefCountPtr<MV> MX, 
                    Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > B) const;


    /*! \brief This method calls normalize(X,Teuchos::null,B); see documentation for that function.
    */
    int normalize ( MV &X, Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > B ) const {
      return normalize(X,Teuchos::null,B);
    }


    /*! \brief Given a set of bases <tt>Q[i]</tt> and a multivector \c X, this method computes an orthonormal basis for \f$colspan(X) - \sum_i colspan(Q[i])\f$.
     *
     *  This routine returns an integer \c rank stating the rank of the computed basis. If the subspace \f$colspan(X) - \sum_i colspan(Q[i])\f$ does not 
     *  have dimension as large as the number of columns of \c X and the orthogonalization manager doe not attempt to augment the subspace, then \c rank 
     *  may be smaller than the number of columns of \c X. In this case, only the first \c rank columns of output \c X and first \c rank rows of \c B will 
     *  be valid.
     *
     * The method attempts to find a basis with dimension the same as the number of columns in \c X. It does this by augmenting linearly dependant 
     * vectors with random directions. A finite number of these attempts will be made; therefore, it is possible that the dimension of the 
     * computed basis is less than the number of vectors in \c X.
     *
     @param X [in/out] The multivector to the modified. 
       On output, the relevant rows of \c X will be orthogonal to the <tt>Q[i]</tt> and will have orthonormal columns (with respect to innerProd()).

     @param MX [in/out] The image of \c X under the operator \c Op. 
       If \f$ MX != 0\f$: On input, this is expected to be consistent with \c X. On output, this is updated consistent with updates to \c X.
       If \f$ MX == 0\f$ or \f$ Op == 0\f$: \c MX is not referenced.

     @param C [out] The coefficients of the original \c X in the \c *Q[i], with respect to innerProd(). If <tt>C[i]</tt> is a non-null pointer 
       and \c *C[i] matches the dimensions of \c X and \c *Q[i], then the coefficients computed during the orthogonalization
       routine will be stored in the matrix \c *C[i]. If <tt>C[i]</tt> is a non-null pointer whose size does not match the dimensions of 
       \c X and \c *Q[i], then a std::invalid_argument exception will be thrown. Otherwise, if <tt>C.size() < i<\tt> or <tt>C[i]</tt> is a null
       pointer, then the orthogonalization manager will declare storage for the coefficients and the user will not have access to them.

     @param B [out] The coefficients of the original \c X with respect to the computed basis. The first rows in \c B
            corresponding to the valid columns in \c X will be upper triangular.

     @param Q [in] A list of multivector bases specifying the subspaces to be orthogonalized against. Each <tt>Q[i]</tt> is assumed to have
     orthonormal columns, and the <tt>Q[i]</tt> are assumed to be mutually orthogonal.

     @return Rank of the basis computed by this method.
    */
    int projectAndNormalize ( MV &X, Teuchos::RefCountPtr<MV> MX, 
                              Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
                              Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > B, 
                              Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q) const;

    /*! \brief This method calls projectAndNormalize(X,Teuchos::null,C,B,Q); see documentation for that function.
    */
    int projectAndNormalize ( MV &X, 
                              Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
                              Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > B, 
                              Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q ) const {
      return projectAndNormalize(X,Teuchos::null,C,B,Q);
    }

    //@}

    //! @name Error methods
    //@{ 

    /*! \brief This method computes the error in orthonormality of a multivector, measured
     * as the Frobenius norm of the difference <tt>innerProd(X,Y) - I</tt>.
     */
    typename Teuchos::ScalarTraits<ScalarType>::magnitudeType 
    orthonormError(const MV &X) const {
      return orthonormError(X,Teuchos::null);
    }

    /*! \brief This method computes the error in orthonormality of a multivector, measured
     * as the Frobenius norm of the difference <tt>innerProd(X,Y) - I</tt>.
     *  The method has the option of exploiting a caller-provided \c MX.
     */
    typename Teuchos::ScalarTraits<ScalarType>::magnitudeType 
    orthonormError(const MV &X, Teuchos::RefCountPtr<const MV> MX) const;

    /*! \brief This method computes the error in orthogonality of two multivectors, measured
     * as the Frobenius norm of <tt>innerProd(X,Y)</tt>.
     */
    typename Teuchos::ScalarTraits<ScalarType>::magnitudeType 
    orthogError(const MV &X1, const MV &X2) const {
      return orthogError(X1,Teuchos::null,X2);
    }

    /*! \brief This method computes the error in orthogonality of two multivectors, measured
     * as the Frobenius norm of <tt>innerProd(X,Y)</tt>.
     *  The method has the option of exploiting a caller-provided \c MX.
     */
    typename Teuchos::ScalarTraits<ScalarType>::magnitudeType 
    orthogError(const MV &X1, Teuchos::RefCountPtr<const MV> MX1, const MV &X2) const;

    //@}

  private:
    
    //! Parameters for re-orthogonalization.
    int max_ortho_steps_;
    MagnitudeType blk_tol_;
    MagnitudeType sing_tol_;

    Teuchos::RefCountPtr<Teuchos::Time> timerUpdate_, timerNorm_, timerScale_, timerInnerProd_;
  
    //! Routine to find an orthonormal basis for X
    int findBasis(MV &X, Teuchos::RefCountPtr<MV> MX, 
		  Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > C, 
		  bool completeBasis, int howMany = -1 ) const;
    
    //! Routine to compute the block orthogonalization
    bool blkOrtho1 ( MV &X, Teuchos::RefCountPtr<MV> MX, 
		     Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
		     Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q) const;

    //! Routine to compute the block orthogonalization
    bool blkOrtho ( MV &X, Teuchos::RefCountPtr<MV> MX, 
		    Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
		    Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q) const;

    int blkOrthoSing ( MV &X, Teuchos::RefCountPtr<MV> MX, 
		       Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
		       Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > B, 
		       Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q) const;    
  };


  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Compute the distance from orthonormality
  template<class ScalarType, class MV, class OP>
  typename Teuchos::ScalarTraits<ScalarType>::magnitudeType 
  ICGSOrthoManager<ScalarType,MV,OP>::orthonormError(const MV &X, Teuchos::RefCountPtr<const MV> MX) const {
    const ScalarType ONE = SCT::one();
    int rank = MVT::GetNumberVecs(X);
    Teuchos::SerialDenseMatrix<int,ScalarType> xTx(rank,rank);
    innerProd(X,X,MX,xTx);
    for (int i=0; i<rank; i++) {
      xTx(i,i) -= ONE;
    }
    return xTx.normFrobenius();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Compute the distance from orthogonality
  template<class ScalarType, class MV, class OP>
  typename Teuchos::ScalarTraits<ScalarType>::magnitudeType 
  ICGSOrthoManager<ScalarType,MV,OP>::orthogError(const MV &X1, Teuchos::RefCountPtr<const MV> MX1, const MV &X2) const {
    int r1 = MVT::GetNumberVecs(X1);
    int r2  = MVT::GetNumberVecs(X2);
    Teuchos::SerialDenseMatrix<int,ScalarType> xTx(r2,r1);
    innerProd(X2,X1,MX1,xTx);
    return xTx.normFrobenius();
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Find an Op-orthonormal basis for span(X) - span(W)
  template<class ScalarType, class MV, class OP>
  int ICGSOrthoManager<ScalarType, MV, OP>::projectAndNormalize(
                                    MV &X, Teuchos::RefCountPtr<MV> MX, 
                                    Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
                                    Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > B, 
                                    Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q ) const {

    ScalarType    ONE  = SCT::one();
    ScalarType    ZERO  = SCT::zero();

    int nq = Q.length();
    int xc = MVT::GetNumberVecs( X );
    int xr = MVT::GetVecLength( X );
    int rank = xc;

    /* if the user doesn't want to store the coefficienets, 
     * allocate some local memory for them 
     */
    if ( B == Teuchos::null ) {
      B = Teuchos::rcp( new Teuchos::SerialDenseMatrix<int,ScalarType>(xc,xc) );
    }

    /******   DO NOT MODIFY *MX IF _hasOp == false   ******/
    if (this->_hasOp) {
      if (MX == Teuchos::null) {
        // we need to allocate space for MX
        MX = MVT::Clone(X,MVT::GetNumberVecs(X));
        OPT::Apply(*(this->_Op),X,*MX);
      }
    }
    else {
      // Op == I  -->  MX = X (ignore it if the user passed it in)
      MX = Teuchos::rcp( &X, false );
    }

    int mxc = MVT::GetNumberVecs( *MX );
    int mxr = MVT::GetVecLength( *MX );

    // short-circuit
    TEST_FOR_EXCEPTION( xc == 0 || xr == 0, std::invalid_argument, "Belos::ICGSOrthoManager::projectAndNormalize(): X must be non-empty" );

    int numbas = 0;
    for (int i=0; i<nq; i++) {
      numbas += MVT::GetNumberVecs( *Q[i] );
    }

    // check size of B
    TEST_FOR_EXCEPTION( B->numRows() != xc || B->numCols() != xc, std::invalid_argument, 
                        "Belos::ICGSOrthoManager::projectAndNormalize(): Size of X must be consistant with size of B" );
    // check size of X and MX
    TEST_FOR_EXCEPTION( xc<0 || xr<0 || mxc<0 || mxr<0, std::invalid_argument, 
                        "Belos::ICGSOrthoManager::projectAndNormalize(): MVT returned negative dimensions for X,MX" );
    // check size of X w.r.t. MX 
    TEST_FOR_EXCEPTION( xc!=mxc || xr!=mxr, std::invalid_argument, 
                        "Belos::ICGSOrthoManager::projectAndNormalize(): Size of X must be consistant with size of MX" );
    // check feasibility
    //TEST_FOR_EXCEPTION( numbas+xc > xr, std::invalid_argument, 
    //                    "Belos::ICGSOrthoManager::projectAndNormalize(): Orthogonality constraints not feasible" );

    // Some flags for checking dependency returns from the internal orthogonalization methods
    bool dep_flg = false;

    // Make a temporary copy of X and MX, just in case a block dependency is detected.
    Teuchos::RefCountPtr<MV> tmpX, tmpMX;
    tmpX = MVT::CloneCopy(X);
    if (this->_hasOp) {
      tmpMX = MVT::CloneCopy(*MX);
    }

    if (xc == 1) {

      // Use the cheaper block orthogonalization.
      // NOTE: Don't check for dependencies because the update has one vector.
      dep_flg = blkOrtho1( X, MX, C, Q );

      // Normalize the new block X
      {
        Teuchos::TimeMonitor normTimer( *timerNorm_ );
        if ( B == Teuchos::null ) {
          B = Teuchos::rcp( new Teuchos::SerialDenseMatrix<int,ScalarType>(xc,xc) );
        }
        std::vector<ScalarType> diag(xc);
        MVT::MvDot( X, *MX, &diag );
        (*B)(0,0) = SCT::squareroot(SCT::magnitude(diag[0]));
        rank = 1; 
        MVT::MvAddMv( ONE/(*B)(0,0), X, ZERO, X, X );
        if (this->_hasOp) {
          // Update MXj.
	  MVT::MvAddMv( ONE/(*B)(0,0), *MX, ZERO, *MX, *MX );
        }
      }

    } 
    else {

      // Use the cheaper block orthogonalization.
      dep_flg = blkOrtho( X, MX, C, Q );

      // If a dependency has been detected in this block, then perform
      // the more expensive single-vector orthogonalization.
      if (dep_flg) {
        rank = blkOrthoSing( *tmpX, tmpMX, C, B, Q );

        // Copy tmpX back into X.
        MVT::MvAddMv( ONE, *tmpX, ZERO, *tmpX, X );
        if (this->_hasOp) {
	  MVT::MvAddMv( ONE, *tmpMX, ZERO, *tmpMX, *MX );
        }
      } 
      else {
        // There is no dependency, so orthonormalize new block X
        rank = findBasis( X, MX, B, false );
        if (rank < xc) {
	  // A dependency was found during orthonormalization of X,
	  // rerun orthogonalization using more expensive single-vector orthogonalization.
	  rank = blkOrthoSing( *tmpX, tmpMX, C, B, Q );

	  // Copy tmpX back into X.
	  MVT::MvAddMv( ONE, *tmpX, ZERO, *tmpX, X );
	  if (this->_hasOp) {
	    MVT::MvAddMv( ONE, *tmpMX, ZERO, *tmpMX, *MX );
	  }
        }    
      }
    } // if (xc == 1) {

    // this should not raise an exception; but our post-conditions oblige us to check
    TEST_FOR_EXCEPTION( rank > xc || rank < 0, std::logic_error, 
                        "Belos::ICGSOrthoManager::projectAndNormalize(): Debug error in rank variable." );

    // Return the rank of X.
    return rank;
  }



  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Find an Op-orthonormal basis for span(X), with rank numvectors(X)
  template<class ScalarType, class MV, class OP>
  int ICGSOrthoManager<ScalarType, MV, OP>::normalize(
                                MV &X, Teuchos::RefCountPtr<MV> MX, 
                                Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > B ) const {
    // call findBasis, with the instruction to try to generate a basis of rank numvecs(X)
    return findBasis(X, MX, B, true);
  }



  //////////////////////////////////////////////////////////////////////////////////////////////////
  template<class ScalarType, class MV, class OP>
  void ICGSOrthoManager<ScalarType, MV, OP>::project(
                          MV &X, Teuchos::RefCountPtr<MV> MX, 
                          Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
                          Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q) const {
    // For the inner product defined by the operator Op or the identity (Op == 0)
    //   -> Orthogonalize X against each Q[i]
    // Modify MX accordingly
    //
    // Note that when Op is 0, MX is not referenced
    //
    // Parameter variables
    //
    // X  : Vectors to be transformed
    //
    // MX : Image of the block vector X by the mass matrix
    //
    // Q  : Bases to orthogonalize against. These are assumed orthonormal, mutually and independently.
    //

    int xc = MVT::GetNumberVecs( X );
    int xr = MVT::GetVecLength( X );
    int nq = Q.length();
    std::vector<int> qcs(nq);
    // short-circuit
    if (nq == 0 || xc == 0 || xr == 0) {
      return;
    }
    int qr = MVT::GetVecLength ( *Q[0] );
    // if we don't have enough C, expand it with null references
    // if we have too many, resize to throw away the latter ones
    // if we have exactly as many as we have Q, this call has no effect
    C.resize(nq);


    /******   DO NOT MODIFY *MX IF _hasOp == false   ******/
    if (this->_hasOp) {
      if (MX == Teuchos::null) {
        // we need to allocate space for MX
        MX = MVT::Clone(X,MVT::GetNumberVecs(X));
        OPT::Apply(*(this->_Op),X,*MX);
      }
    }
    else {
      // Op == I  -->  MX = X (ignore it if the user passed it in)
      MX = Teuchos::rcp( &X, false );
    }
    int mxc = MVT::GetNumberVecs( *MX );
    int mxr = MVT::GetVecLength( *MX );

    // check size of X and Q w.r.t. common sense
    TEST_FOR_EXCEPTION( xc<0 || xr<0 || mxc<0 || mxr<0, std::invalid_argument, 
                        "Belos::ICGSOrthoManager::project(): MVT returned negative dimensions for X,MX" );
    // check size of X w.r.t. MX and Q
    TEST_FOR_EXCEPTION( xc!=mxc || xr!=mxr || xr!=qr, std::invalid_argument, 
                        "Belos::ICGSOrthoManager::project(): Size of X not consistant with MX,Q" );

    // tally up size of all Q and check/allocate C
    int baslen = 0;
    for (int i=0; i<nq; i++) {
      TEST_FOR_EXCEPTION( MVT::GetVecLength( *Q[i] ) != qr, std::invalid_argument, 
                          "Belos::ICGSOrthoManager::project(): Q lengths not mutually consistant" );
      qcs[i] = MVT::GetNumberVecs( *Q[i] );
      TEST_FOR_EXCEPTION( qr < qcs[i], std::invalid_argument, 
                          "Belos::ICGSOrthoManager::project(): Q has less rows than columns" );
      baslen += qcs[i];

      // check size of C[i]
      if ( C[i] == Teuchos::null ) {
        C[i] = Teuchos::rcp( new Teuchos::SerialDenseMatrix<int,ScalarType>(qcs[i],xc) );
      }
      else {
        TEST_FOR_EXCEPTION( C[i]->numRows() != qcs[i] || C[i]->numCols() != xc , std::invalid_argument, 
                           "Belos::ICGSOrthoManager::project(): Size of Q not consistant with size of C" );
      }
    }

    // Use the cheaper block orthogonalization, don't check for rank deficiency.
    blkOrtho( X, MX, C, Q );

  }  

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Find an Op-orthonormal basis for span(X), with the option of extending the subspace so that 
  // the rank is numvectors(X)
  template<class ScalarType, class MV, class OP>
  int ICGSOrthoManager<ScalarType, MV, OP>::findBasis(
						      MV &X, Teuchos::RefCountPtr<MV> MX, 
						      Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > B,
						      bool completeBasis, int howMany ) const {
    // For the inner product defined by the operator Op or the identity (Op == 0)
    //   -> Orthonormalize X 
    // Modify MX accordingly
    //
    // Note that when Op is 0, MX is not referenced
    //
    // Parameter variables
    //
    // X  : Vectors to be orthonormalized
    //
    // MX : Image of the multivector X under the operator Op
    //
    // Op  : Pointer to the operator for the inner product
    //
    //

    Teuchos::TimeMonitor normTimer( *timerNorm_ );

    const ScalarType ONE  = SCT::one();
    const MagnitudeType ZERO = SCT::magnitude(SCT::zero());

    int xc = MVT::GetNumberVecs( X );
    int xr = MVT::GetVecLength( X );

    if (howMany == -1) {
      howMany = xc;
    }

    /*******************************************************
     *  If _hasOp == false, we will not reference MX below *
     *******************************************************/

    // if Op==null, MX == X (via pointer)
    // Otherwise, either the user passed in MX or we will allocated and compute it
    if (this->_hasOp) {
      if (MX == Teuchos::null) {
        // we need to allocate space for MX
        MX = MVT::Clone(X,xc);
        OPT::Apply(*(this->_Op),X,*MX);
      }
    }

    /* if the user doesn't want to store the coefficienets, 
     * allocate some local memory for them 
     */
    if ( B == Teuchos::null ) {
      B = Teuchos::rcp( new Teuchos::SerialDenseMatrix<int,ScalarType>(xc,xc) );
    }

    int mxc = (this->_hasOp) ? MVT::GetNumberVecs( *MX ) : xc;
    int mxr = (this->_hasOp) ? MVT::GetVecLength( *MX )  : xr;

    // check size of C, B
    TEST_FOR_EXCEPTION( xc == 0 || xr == 0, std::invalid_argument, 
                        "Belos::ICGSOrthoManager::findBasis(): X must be non-empty" );
    TEST_FOR_EXCEPTION( B->numRows() != xc || B->numCols() != xc, std::invalid_argument, 
                        "Belos::ICGSOrthoManager::findBasis(): Size of X not consistant with size of B" );
    TEST_FOR_EXCEPTION( xc != mxc || xr != mxr, std::invalid_argument, 
                        "Belos::ICGSOrthoManager::findBasis(): Size of X not consistant with size of MX" );
    TEST_FOR_EXCEPTION( xc > xr, std::invalid_argument, 
                        "Belos::ICGSOrthoManager::findBasis(): Size of X not feasible for normalization" );
    TEST_FOR_EXCEPTION( howMany < 0 || howMany > xc, std::invalid_argument, 
                        "Belos::ICGSOrthoManager::findBasis(): Invalid howMany parameter" );

    /* xstart is which column we are starting the process with, based on howMany
     * columns before xstart are assumed to be Op-orthonormal already
     */
    int xstart = xc - howMany;

    for (int j = xstart; j < xc; j++) {

      // numX is 
      // * number of currently orthonormal columns of X
      // * the index of the current column of X
      int numX = j;
      bool addVec = false;

      // Get a view of the vector currently being worked on.
      std::vector<int> index(1);
      index[0] = numX;
      Teuchos::RefCountPtr<MV> Xj = MVT::CloneView( X, index );
      Teuchos::RefCountPtr<MV> MXj;
      if ((this->_hasOp)) {
        // MXj is a view of the current vector in MX
        MXj = MVT::CloneView( *MX, index );
      }
      else {
        // MXj is a pointer to Xj, and MUST NOT be modified
        MXj = Xj;
      }

      // Get a view of the previous vectors.
      std::vector<int> prev_idx( numX );
      Teuchos::RefCountPtr<const MV> prevX, prevMX;

      if (numX > 0) {
        for (int i=0; i<numX; i++) {
          prev_idx[i] = i;
        }
        prevX = MVT::CloneView( X, prev_idx );
        if (this->_hasOp) {
          prevMX = MVT::CloneView( *MX, prev_idx );
        }
      } 

      // Make storage for these Gram-Schmidt iterations.
      Teuchos::SerialDenseMatrix<int,ScalarType> product(numX, 1);
      std::vector<ScalarType> oldDot( 1 ), newDot( 1 );
      //
      // Save old MXj vector and compute Op-norm
      //
      Teuchos::RefCountPtr<MV> oldMXj = MVT::CloneCopy( *MXj ); 
      MVT::MvDot( *Xj, *MXj, &oldDot );
      // Xj^H Op Xj should be real and positive, by the hermitian positive definiteness of Op
      TEST_FOR_EXCEPTION( SCT::real(oldDot[0]) < ZERO, OrthoError, 
			  "Belos::ICGSOrthoManager::findBasis(): Negative definiteness discovered in inner product" );

      if (numX > 0) {
	
        Teuchos::SerialDenseMatrix<int,ScalarType> P2(numX,1);
	
	for (int i=0; i<max_ortho_steps_; ++i) {
	  
	  // product <- prevX^T MXj
          {
            Teuchos::TimeMonitor innerProdTimer( *timerInnerProd_ );
	    innerProd(*prevX,*Xj,MXj,P2);
          }
	  
	  // Xj <- Xj - prevX prevX^T MXj   
	  //     = Xj - prevX product
          {
            Teuchos::TimeMonitor updateTimer( *timerUpdate_ );
	    MVT::MvTimesMatAddMv( -ONE, *prevX, P2, ONE, *Xj );
          }
	  
	  // Update MXj
	  if (this->_hasOp) {
	    // MXj <- Op*Xj_new
	    //      = Op*(Xj_old - prevX prevX^T MXj)
	    //      = MXj - prevMX product
            Teuchos::TimeMonitor updateTimer( *timerUpdate_ );
	    MVT::MvTimesMatAddMv( -ONE, *prevMX, P2, ONE, *MXj );
	  }
	  
	  // Set coefficients
	  if ( i==0 )
	    product = P2;
	  else
	    product += P2;
        }
	
      } // if (numX > 0)

      // Compute Op-norm with old MXj
      MVT::MvDot( *Xj, *oldMXj, &newDot );

      // Check to see if the new vector is dependent.
      if (completeBasis) {
	//
	// We need a complete basis, so add random vectors if necessary
	//
	if ( SCT::magnitude(newDot[0]) < SCT::magnitude(sing_tol_*oldDot[0]) ) {
	  
	  // Add a random vector and orthogonalize it against previous vectors in block.
	  addVec = true;
#ifdef ORTHO_DEBUG
	  cout << "Belos::ICGSOrthoManager::findBasis() --> Random for column " << numX << endl;
#endif
	  //
	  Teuchos::RefCountPtr<MV> tempXj = MVT::Clone( X, 1 );
	  Teuchos::RefCountPtr<MV> tempMXj;
	  MVT::MvRandom( *tempXj );
	  if (this->_hasOp) {
	    tempMXj = MVT::Clone( X, 1 );
	    OPT::Apply( *(this->_Op), *tempXj, *tempMXj );
	  } 
	  else {
	    tempMXj = tempXj;
	  }
	  MVT::MvDot( *tempXj, *tempMXj, &oldDot );
	  //
	  for (int num_orth=0; num_orth<max_ortho_steps_; num_orth++){
            {
              Teuchos::TimeMonitor innerProdTimer( *timerInnerProd_ );
	      innerProd(*prevX,*tempXj,tempMXj,product);
            }
            {
              Teuchos::TimeMonitor updateTimer( *timerUpdate_ );
	      MVT::MvTimesMatAddMv( -ONE, *prevX, product, ONE, *tempXj );
            }
	    if (this->_hasOp) {
              Teuchos::TimeMonitor updateTimer( *timerUpdate_ );
	      MVT::MvTimesMatAddMv( -ONE, *prevMX, product, ONE, *tempMXj );
	    }
	  }
	  // Compute new Op-norm
	  MVT::MvDot( *tempXj, *tempMXj, &newDot );
	  //
	  if ( SCT::magnitude(newDot[0]) >= SCT::magnitude(oldDot[0]*sing_tol_) ) {
	    // Copy vector into current column of _basisvecs
	    MVT::MvAddMv( ONE, *tempXj, ZERO, *tempXj, *Xj );
	    if (this->_hasOp) {
	      MVT::MvAddMv( ONE, *tempMXj, ZERO, *tempMXj, *MXj );
	    }
	  }
	  else {
	    return numX;
	  } 
	}
      }
      else {
	//
	// We only need to detect dependencies.
	//
	if ( SCT::magnitude(newDot[0]) < SCT::magnitude(oldDot[0]*blk_tol_) ) {
	  return numX;
	}
      }
      
      // If we haven't left this method yet, then we can normalize the new vector Xj.
      // Normalize Xj.
      // Xj <- Xj / sqrt(newDot)
      ScalarType diag = SCT::squareroot(SCT::magnitude(newDot[0]));
      {
        MVT::MvAddMv( ONE/diag, *Xj, ZERO, *Xj, *Xj );
        if (this->_hasOp) {
	  // Update MXj.
	  MVT::MvAddMv( ONE/diag, *MXj, ZERO, *MXj, *MXj );
        }
      }

      // If we've added a random vector, enter a zero in the j'th diagonal element.
      if (addVec) {
	(*B)(j,j) = ZERO;
      }
      else {
	(*B)(j,j) = diag;
      }

      // Save the coefficients, if we are working on the original vector and not a randomly generated one
      if (!addVec) {
	for (int i=0; i<numX; i++) {
	  (*B)(i,j) = product(i,0);
	}
      }

    } // for (j = 0; j < xc; ++j)

    return xc;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Routine to compute the block orthogonalization
  template<class ScalarType, class MV, class OP>
  bool 
  ICGSOrthoManager<ScalarType, MV, OP>::blkOrtho1 ( MV &X, Teuchos::RefCountPtr<MV> MX, 
						    Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
						    Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q) const
  {
    int nq = Q.length();
    int xc = MVT::GetNumberVecs( X );
    const ScalarType ONE  = SCT::one();

    std::vector<int> qcs( nq );
    for (int i=0; i<nq; i++) {
      qcs[i] = MVT::GetNumberVecs( *Q[i] );
    }

    // Perform the Gram-Schmidt transformation for a block of vectors

    Teuchos::Array<Teuchos::RefCountPtr<MV> > MQ(nq);
    // Define the product Q^T * (Op*X)
    for (int i=0; i<nq; i++) {
      // Multiply Q' with MX
      {
        Teuchos::TimeMonitor innerProdTimer( *timerInnerProd_ );
        innerProd(*Q[i],X,MX,*C[i]);
      }
      // Multiply by Q and subtract the result in X
      {
        Teuchos::TimeMonitor updateTimer( *timerUpdate_ );
        MVT::MvTimesMatAddMv( -ONE, *Q[i], *C[i], ONE, X );
      }

      // Update MX, with the least number of applications of Op as possible
      if (this->_hasOp) {
        if (xc <= qcs[i]) {
          OPT::Apply( *(this->_Op), X, *MX);
        }
        else {
          // this will possibly be used again below; don't delete it
          MQ[i] = MVT::Clone( *Q[i], qcs[i] );
          OPT::Apply( *(this->_Op), *Q[i], *MQ[i] );
          MVT::MvTimesMatAddMv( -ONE, *MQ[i], *C[i], ONE, *MX );
        }
      }
    }

    // Do as many steps of classical Gram-Schmidt as required by max_ortho_steps_
    for (int j = 1; j < max_ortho_steps_; ++j) {
      
      for (int i=0; i<nq; i++) {
	Teuchos::SerialDenseMatrix<int,ScalarType> C2(*C[i]);
        
	// Apply another step of classical Gram-Schmidt
	{
          Teuchos::TimeMonitor innerProdTimer( *timerInnerProd_ );
          innerProd(*Q[i],X,MX,C2);
        }
	*C[i] += C2;
        {
          Teuchos::TimeMonitor updateTimer( *timerUpdate_ );
	  MVT::MvTimesMatAddMv( -ONE, *Q[i], C2, ONE, X );
        }
        
	// Update MX, with the least number of applications of Op as possible
	if (this->_hasOp) {
	  if (MQ[i].get()) {
	    // MQ was allocated and computed above; use it
	    MVT::MvTimesMatAddMv( -ONE, *MQ[i], C2, ONE, *MX );
	  }
	  else if (xc <= qcs[i]) {
	    // MQ was not allocated and computed above; it was cheaper to use X before and it still is
	    OPT::Apply( *(this->_Op), X, *MX);
	  }
	}
      } // for (int i=0; i<nq; i++)
    } // for (int j = 0; j < max_ortho_steps; ++j)
  
    return false;
  }

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Routine to compute the block orthogonalization
  template<class ScalarType, class MV, class OP>
  bool 
  ICGSOrthoManager<ScalarType, MV, OP>::blkOrtho ( MV &X, Teuchos::RefCountPtr<MV> MX, 
						   Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
						   Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q) const
  {
    int nq = Q.length();
    int xc = MVT::GetNumberVecs( X );
    bool dep_flg = false;
    const ScalarType ONE  = SCT::one();

    std::vector<int> qcs( nq );
    for (int i=0; i<nq; i++) {
      qcs[i] = MVT::GetNumberVecs( *Q[i] );
    }

    // Perform the Gram-Schmidt transformation for a block of vectors
    
    // Compute the initial Op-norms
    std::vector<ScalarType> oldDot( xc );
    MVT::MvDot( X, *MX, &oldDot );

    Teuchos::Array<Teuchos::RefCountPtr<MV> > MQ(nq);
    // Define the product Q^T * (Op*X)
    for (int i=0; i<nq; i++) {
      // Multiply Q' with MX
      {
        Teuchos::TimeMonitor innerProdTimer( *timerInnerProd_ );
        innerProd(*Q[i],X,MX,*C[i]);
      }
      // Multiply by Q and subtract the result in X
      {
        Teuchos::TimeMonitor updateTimer( *timerUpdate_ );
        MVT::MvTimesMatAddMv( -ONE, *Q[i], *C[i], ONE, X );
      }
      // Update MX, with the least number of applications of Op as possible
      if (this->_hasOp) {
        if (xc <= qcs[i]) {
          OPT::Apply( *(this->_Op), X, *MX);
        }
        else {
          // this will possibly be used again below; don't delete it
          MQ[i] = MVT::Clone( *Q[i], qcs[i] );
          OPT::Apply( *(this->_Op), *Q[i], *MQ[i] );
          MVT::MvTimesMatAddMv( -ONE, *MQ[i], *C[i], ONE, *MX );
        }
      }
    }

    // Do as many steps of classical Gram-Schmidt as required by max_ortho_steps_
    for (int j = 1; j < max_ortho_steps_; ++j) {
      
      for (int i=0; i<nq; i++) {
	Teuchos::SerialDenseMatrix<int,ScalarType> C2(*C[i]);
        
	// Apply another step of classical Gram-Schmidt
        {
          Teuchos::TimeMonitor innerProdTimer( *timerInnerProd_ );
	  innerProd(*Q[i],X,MX,C2);
        }
	*C[i] += C2;
        {
          Teuchos::TimeMonitor updateTimer( *timerUpdate_ );
	  MVT::MvTimesMatAddMv( -ONE, *Q[i], C2, ONE, X );
        }
        
	// Update MX, with the least number of applications of Op as possible
	if (this->_hasOp) {
	  if (MQ[i].get()) {
	    // MQ was allocated and computed above; use it
	    MVT::MvTimesMatAddMv( -ONE, *MQ[i], C2, ONE, *MX );
	  }
	  else if (xc <= qcs[i]) {
	    // MQ was not allocated and computed above; it was cheaper to use X before and it still is
	    OPT::Apply( *(this->_Op), X, *MX);
	  }
	}
      } // for (int i=0; i<nq; i++)
    } // for (int j = 0; j < max_ortho_steps; ++j)
  
    // Compute new Op-norms
    std::vector<ScalarType> newDot(xc);
    MVT::MvDot( X, *MX, &newDot );
 
    // Check to make sure the new block of vectors are not dependent on previous vectors
    for (int i=0; i<xc; i++){
      if (SCT::magnitude(newDot[i]) < SCT::magnitude(oldDot[i] * blk_tol_)) {
	dep_flg = true;
	break;
      }
    } // end for (i=0;...)

    return dep_flg;
  }
  
  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Routine to compute the block orthogonalization using single-vector orthogonalization
  template<class ScalarType, class MV, class OP>
  int
  ICGSOrthoManager<ScalarType, MV, OP>::blkOrthoSing ( MV &X, Teuchos::RefCountPtr<MV> MX, 
						       Teuchos::Array<Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > > C, 
						       Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > B, 
						       Teuchos::Array<Teuchos::RefCountPtr<const MV> > Q) const
  {
    const ScalarType ONE  = SCT::one();
    const ScalarType ZERO  = SCT::zero();
    
    int nq = Q.length();
    int xc = MVT::GetNumberVecs( X );
    std::vector<int> indX( 1 );
    std::vector<ScalarType> oldDot( 1 ), newDot( 1 );

    std::vector<int> qcs( nq );
    for (int i=0; i<nq; i++) {
      qcs[i] = MVT::GetNumberVecs( *Q[i] );
    }

    // Create pointers for the previous vectors of X that have already been orthonormalized.
    Teuchos::RefCountPtr<const MV> lastQ;
    Teuchos::RefCountPtr<MV> Xj, MXj;
    Teuchos::RefCountPtr<Teuchos::SerialDenseMatrix<int,ScalarType> > lastC;

    // Perform the Gram-Schmidt transformation for each vector in the block of vectors.
    for (int j=0; j<xc; j++) {
      
      bool dep_flg = false;
      
      // Get a view of the previously orthogonalized vectors and B, add it to the arrays.
      if (j > 0) {
	std::vector<int> index( j );
	for (int ind=0; ind<j; ind++) {
	  index[ind] = ind;
	}
	lastQ = MVT::CloneView( X, index );

	// Add these views to the Q and C arrays.
	Q.push_back( lastQ );
	C.push_back( B );
	qcs.push_back( MVT::GetNumberVecs( *lastQ ) );
      }
      
      // Get a view of the current vector in X to orthogonalize.
      indX[0] = j;
      Xj = MVT::CloneView( X, indX );
      if (this->_hasOp) {
	MXj = MVT::CloneView( *MX, indX );
      }
      else {
	MXj = Xj;
      }
      
      // Compute the initial Op-norms
      MVT::MvDot( *Xj, *MXj, &oldDot );
      
      Teuchos::Array<Teuchos::RefCountPtr<MV> > MQ(Q.length());
      // Define the product Q^T * (Op*X)
      for (int i=0; i<Q.length(); i++) {

	// Get a view of the current serial dense matrix
	Teuchos::SerialDenseMatrix<int,ScalarType> tempC( Teuchos::View, *C[i], qcs[i], 1, 0, j );

	// Multiply Q' with MX
	innerProd(*Q[i],*Xj,MXj,tempC);
	// Multiply by Q and subtract the result in Xj
	MVT::MvTimesMatAddMv( -ONE, *Q[i], tempC, ONE, *Xj );
	
	// Update MXj, with the least number of applications of Op as possible
	if (this->_hasOp) {
	  if (xc <= qcs[i]) {
	    OPT::Apply( *(this->_Op), *Xj, *MXj);
	  }
	  else {
	    // this will possibly be used again below; don't delete it
	    MQ[i] = MVT::Clone( *Q[i], qcs[i] );
	    OPT::Apply( *(this->_Op), *Q[i], *MQ[i] );
	    MVT::MvTimesMatAddMv( -ONE, *MQ[i], tempC, ONE, *MXj );
	  }
	}
      }
     
      // Do any additional steps of classical Gram-Schmidt orthogonalization 
      for (int num_ortho_steps=1; num_ortho_steps < max_ortho_steps_; ++num_ortho_steps) {
	
	for (int i=0; i<Q.length(); i++) {
	  Teuchos::SerialDenseMatrix<int,ScalarType> tempC( Teuchos::View, *C[i], qcs[i], 1, 0, j );
	  Teuchos::SerialDenseMatrix<int,ScalarType> C2( qcs[i], 1 );
	  
	  // Apply another step of classical Gram-Schmidt
	  innerProd(*Q[i],*Xj,MXj,C2);
	  tempC += C2;
	  MVT::MvTimesMatAddMv( -ONE, *Q[i], C2, ONE, *Xj );
	  
	  // Update MXj, with the least number of applications of Op as possible
	  if (this->_hasOp) {
	    if (MQ[i].get()) {
	      // MQ was allocated and computed above; use it
	      MVT::MvTimesMatAddMv( -ONE, *MQ[i], C2, ONE, *MXj );
	    }
	    else if (xc <= qcs[i]) {
	      // MQ was not allocated and computed above; it was cheaper to use X before and it still is
	      OPT::Apply( *(this->_Op), *Xj, *MXj);
	    }
	  }
	} // for (int i=0; i<Q.length(); i++)
	
      } // for (int num_ortho_steps=1; num_ortho_steps < max_ortho_steps_; ++num_ortho_steps)
      
      // Check for linear dependence.
      if (SCT::magnitude(newDot[0]) < SCT::magnitude(oldDot[0]*sing_tol_)) {
	dep_flg = true;
      }
      
      // Normalize the new vector if it's not dependent
      if (!dep_flg) {
	ScalarType diag = SCT::squareroot(SCT::magnitude(newDot[0]));
	
	MVT::MvAddMv( ONE/diag, *Xj, ZERO, *Xj, *Xj );
	if (this->_hasOp) {
	  // Update MXj.
	  MVT::MvAddMv( ONE/diag, *MXj, ZERO, *MXj, *MXj );
	}
	
	// Enter value on diagonal of B.
	(*B)(j,j) = diag;
      }
      else {
	// Create a random vector and orthogonalize it against all previous columns of Q.
	Teuchos::RefCountPtr<MV> tempXj = MVT::Clone( X, 1 );
	Teuchos::RefCountPtr<MV> tempMXj;
	MVT::MvRandom( *tempXj );
	if (this->_hasOp) {
	  tempMXj = MVT::Clone( X, 1 );
	  OPT::Apply( *(this->_Op), *tempXj, *tempMXj );
	} 
	else {
	  tempMXj = tempXj;
	}
	MVT::MvDot( *tempXj, *tempMXj, &oldDot );
	//
	for (int num_orth=0; num_orth<max_ortho_steps_; num_orth++) {
	  
	  for (int i=0; i<Q.length(); i++) {
	    Teuchos::SerialDenseMatrix<int,ScalarType> product( qcs[i], 1 );
	    
	    // Apply another step of classical Gram-Schmidt
	    innerProd(*Q[i],*tempXj,tempMXj,product);
	    MVT::MvTimesMatAddMv( -ONE, *Q[i], product, ONE, *tempXj );
	    
	    // Update MXj, with the least number of applications of Op as possible
	    if (this->_hasOp) {
	      if (MQ[i].get()) {
		// MQ was allocated and computed above; use it
		MVT::MvTimesMatAddMv( -ONE, *MQ[i], product, ONE, *tempMXj );
	      }
	      else if (xc <= qcs[i]) {
		// MQ was not allocated and computed above; it was cheaper to use X before and it still is
		OPT::Apply( *(this->_Op), *tempXj, *tempMXj);
	      }
	    }
	  } // for (int i=0; i<nq; i++)
	  
	}
	
	// Compute the Op-norms after the correction step.
	MVT::MvDot( *tempXj, *tempMXj, &newDot );
	
	// Copy vector into current column of Xj
	if ( SCT::magnitude(newDot[0]) >= SCT::magnitude(oldDot[0]*sing_tol_) ) {
	  ScalarType diag = SCT::squareroot(SCT::magnitude(newDot[0]));
	  
	  // Enter value on diagonal of B.
	  (*B)(j,j) = ZERO;

	  // Copy vector into current column of _basisvecs
	  MVT::MvAddMv( ONE/diag, *tempXj, ZERO, *tempXj, *Xj );
	  if (this->_hasOp) {
	    MVT::MvAddMv( ONE/diag, *tempMXj, ZERO, *tempMXj, *MXj );
	  }
	}
	else {
	  return j;
	} 
      } // if (!dep_flg)

      // Remove the vectors from array
      if (j > 0) {
	Q.resize( nq );
	C.resize( nq );
	qcs.resize( nq );
      }

    } // for (int j=0; j<xc; j++)

    return xc;
  }

} // namespace Belos

#endif // BELOS_ICGS_ORTHOMANAGER_HPP

