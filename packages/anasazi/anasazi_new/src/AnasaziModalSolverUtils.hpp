// @HEADER
// ***********************************************************************
//
//                 Anasazi: Block Eigensolvers Package
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

#ifndef ANASAZI_MODAL_SOLVER_UTILS_HPP
#define ANASAZI_MODAL_SOLVER_UTILS_HPP

/*!     \file AnasaziModalSolverUtils.hpp
        \brief Class which provides internal utilities for the Anasazi modal solvers.
*/

/*!    \class Anasazi::ModalSolverUtils
       \brief Anasazi's templated, static class providing utilities for
       the modal solvers.

       This class provides concrete, templated implementations of utilities necessary
       for the modal solvers (Davidson, LOBPCG, ...).  These utilities include
       sorting, orthogonalization, projecting/solving local eigensystems, and sanity
       checking.  These are internal utilties, so the user should not alter this class.

       \author Ulrich Hetmaniuk, Rich Lehoucq, and Heidi Thornquist
*/

#include "AnasaziMultiVecTraits.hpp"
#include "AnasaziOperatorTraits.hpp"
#include "AnasaziOutputManager.hpp"
#include "Teuchos_BLAS.hpp"

namespace Anasazi {

  template<class STYPE, class MV, class OP>
  class ModalSolverUtils 
  {  
  public:
    
    //@{ \name Constructor/Destructor

    ModalSolverUtils( const Teuchos::RefCountPtr<OutputManager<STYPE> > &om ); 
    
    virtual ~ModalSolverUtils() {};
    
    //@}
    
    //@{ \name Sorting Methods
    
    int sortScalars(int n, STYPE *y, int *perm = 0) const;
    
    int sortScalars_Vectors(int n, STYPE* lambda, STYPE* Q = 0, int ldQ = 0) const;

    //@} 

    //@{ \name Eigensolver Projection Methods

    int massOrthonormalize(MV &X, MV &MX, const OP *M, const MV &Q, int howMany,
				  int type = 0, STYPE *WS = 0, STYPE kappa = 1.5625) const;

    void localProjection(int numRow, int numCol, int length,
				STYPE *U, int ldU, STYPE *MatV, int ldV,
				STYPE *UtMatV, int ldUtMatV, STYPE *work) const;
    
    int directSolver(int, STYPE*, int, STYPE*, int, int&, 
			    STYPE*, int, STYPE*, int, int = 0) const;

    //@}

    //@{ \name Sanity Checking Methods

    STYPE errorOrthogonality(const MV &X, const MV &R, const OP *M = 0) const;
    
    STYPE errorOrthonormality(const MV &X, const OP &M = 0) const;
    
    STYPE errorEquality(const MV &X, const MV &MX, const OP &M = 0) const;
    
    int inputArguments(const int &numEigen, const OP &K, const OP &M, const OP &P,
			      const MV &Q, const int &minSize) const;
    
    //@}

  private:

    // Reference counted pointer to output manager used by eigensolver.
    Teuchos::RefCountPtr<OutputManager<STYPE> > _om;

    //@{ \name Internal Typedefs

    typedef MultiVecTraits<STYPE,MV> MVT;
    typedef OperatorTraits<STYPE,MV,OP> OPT;

    //@}
  };

  //-----------------------------------------------------------------------------
  // 
  //  CONSTRUCTOR
  //
  //-----------------------------------------------------------------------------  

  template<class STYPE, class MV, class OP>
  ModalSolverUtils<STYPE, MV, OP>::ModalSolverUtils( const Teuchos::RefCountPtr<OutputManager<STYPE> > &om ) 
    : _om(om)
  {}

  //-----------------------------------------------------------------------------
  // 
  //  SORTING METHODS
  //
  //-----------------------------------------------------------------------------
  
  template<class STYPE, class MV, class OP>
  int ModalSolverUtils<STYPE, MV, OP>::sortScalars( int n, STYPE *y, int *perm) const 
  {
    // Sort a vector into increasing order of algebraic values
    //
    // Input:
    //
    // n    (integer ) = Size of the array (input)
    // y    (double* ) = Array of length n to be sorted (input/output)
    // perm (integer*) = Array of length n with the permutation (input/output)
    //                   Optional argument
    
    int i, j;
    int igap = n / 2;
    
    if (igap == 0) {
      if ((n > 0) && (perm != 0)) {
	perm[0] = 0;
      }
      return 0;
    }
    
    if (perm) {
      for (i = 0; i < n; ++i)
	perm[i] = i;
    }
    
    while (igap > 0) {
      for (i=igap; i<n; ++i) {
	for (j=i-igap; j>=0; j-=igap) {
	  if (y[j] > y[j+igap]) {
	    double tmpD = y[j];
	    y[j] = y[j+igap];
	    y[j+igap] = tmpD;
	    if (perm) {
	      int tmpI = perm[j];
	      perm[j] = perm[j+igap];
	      perm[j+igap] = tmpI;
	    }
	  }
	  else {
	    break;
	  }
	}
      }
      igap = igap / 2;
    }
    
    return 0;
  }
  
  template<class STYPE, class MV, class OP>
  int ModalSolverUtils<STYPE, MV, OP>::sortScalars_Vectors( int n, STYPE *lambda, STYPE *Q, int ldQ) const
  {
    // This routines sorts the scalars (stored in lambda) in ascending order.
    // The associated vectors (stored in Q) are accordingly ordered.
    // One vector is of length ldQ.
    // Q must be of size ldQ * num.
    
    Teuchos::BLAS<int,STYPE> blas;
    STYPE tmp;
    int info = 0;
    int i, j;
    
    int igap = num / 2;
    
    if ((Q) && (ldQ > 0)) {
      while (igap > 0) {
	for (i=igap; i < num; ++i) {
	  for (j=i-igap; j>=0; j-=igap) {
	    if (lambda[j] > lambda[j+igap]) {
	      // Swap two scalars
	      tmp = lambda[j];
	      lambda[j] = lambda[j+igap];
	      lambda[j+igap] = tmp;
	      // Swap corresponding vectors
	      blas.SWAP( ldQ, Q + j*ldQ, 1, Q + (j+igap)*ldQ, 1 );
	    } 
	    else {
	      break;
	    }
	  }
	}
	igap = igap / 2;
      } // while (igap > 0)
    } // if ((Q) && (ldQ > 0))
    else {
      while (igap > 0) {
	for (i=igap; i < num; ++i) {
	  for (j=i-igap; j>=0; j-=igap) {
	    if (lambda[j] > lambda[j+igap]) {
	      // Swap two scalars
	      tmp = lambda[j];
	      lambda[j] = lambda[j+igap];
	      lambda[j+igap] = tmp;
	    } 
	    else {
	      break;
	    }
	  }
	}
	igap = igap / 2;
      } // while (igap > 0)
    } // if ((Q) && (ldQ > 0))
    
    return info;  
  }
  
  //-----------------------------------------------------------------------------
  // 
  //  EIGENSOLVER PROJECTION METHODS
  //
  //-----------------------------------------------------------------------------
  
  template<class STYPE, class MV, class OP>
  int ModalSolverUtils<STYPE, MV, OP>::massOrthonormalize(MV &X, MV &MX, const OP *M, const MV &Q, int howMany,
							  int type, STYPE *WS, STYPE kappa) const
  {
    // For the inner product defined by the operator M or the identity (M = 0)
    //   -> Orthogonalize X against Q
    //   -> Orthonormalize X 
    // Modify MX accordingly
    // WS is used as a workspace (size: (# of columns in X)*(# of rows in X))
    //
    // Note that when M is 0, MX is not referenced
    //
    // Parameter variables
    //
    // X  : Vectors to be transformed
    //
    // MX : Image of the block vector X by the mass matrix
    //
    // Q  : Vectors to orthogonalize against
    //
    // howMany : Number of vectors of X to orthogonalized
    //           If this number is smaller than the total number of vectors in X,
    //           then it is assumed that the last "howMany" vectors are not orthogonal
    //           while the other vectors in X are othogonal to Q and orthonormal.
    //
    // type = 0 (default) > Performs both operations
    // type = 1           > Performs Q^T M X = 0
    // type = 2           > Performs X^T M X = I
    //
    // WS   = Working space (default value = 0)
    //
    // kappa= Coefficient determining when to perform a second Gram-Schmidt step
    //        Default value = 1.5625 = (1.25)^2 (as suggested in Parlett's book)
    //
    // Output the status of the computation
    //
    // info =   0 >> Success.
    //
    // info >   0 >> Indicate how many vectors have been tried to avoid rank deficiency for X 
    //
    // info =  -1 >> Failure >> X has zero columns
    //                       >> It happens when # col of X     > # rows of X 
    //                       >> It happens when # col of [Q X] > # rows of X 
    //                       >> It happens when no good random vectors could be found
    
    int i;
    int info = 0;
    
    // Orthogonalize X against Q
    timeProj -= MyWatch.WallTime();
    if (type != 2) {
      
      int xc = howMany;
      int xr = MVT::GetNumberVecs( X );
      int qc = MVT::GetNumberVecs( Q );
      
      std::vector<int> index(howMany);
      for (i=0; i<howMany; i++)
	index[i] = xr - howMany + i;

      Teuchos::RefCountPtr<MV> XX = MVT::CloneView( X, &index[0], howMany );
      
      Teuchos::RefCountPtr<MV> MXX;

      if (M) {
	int mxr = MVT::GetNumberVecs( MX );
	for (i=0; i<howMany; i++)
	  index[i] = mxr - howMany + i;
	MXX = MVT::CloneView( MX, &index[0], howMany );
      } 
      else {
	MXX = MVT::CloneView( X, &index[0], howMany );
      }

    // Perform the Gram-Schmidt transformation for a block of vectors

    // Compute the initial M-norms
    std::vector<STYPE> oldDot( xc );
    MVT::MvDot( *XX, *MXX, &oldDot[0] );

    // Define the product Q^T * (M*X)
    // Multiply Q' with MX
    Teuchos::SerialDenseMatrix<int,STYPE> qTmx( qc, xc );
    MVT::MvTransMv( one, Q, MX, qTmx );

    // Multiply by Q and substract the result in X
    MVT::MvTimesMatAddMv( -1.0, Q, qTmx, 1.0, *XX );

    // Update MX
    if (M) {
      if (qc >= xc) {
        //timeProj_MassMult -= MyWatch.WallTime();
        OPT::Apply( M, *XX, *MXX);
        //timeProj_MassMult += MyWatch.WallTime();
	//numProj_MassMult += xc;
      }
      else {
	Teuchos::RefCountPtr<MV> MQ = MVT::Clone( Q, qc );
        //timeProj_MassMult -= MyWatch.WallTime();
        OPT::Apply( M, Q, *MQ );
        //timeProj_MassMult += MyWatch.WallTime();
        //numProj_MassMult += qc;
	MVT::MvTimesMatAddMv( -1.0, *MQ, qTmx, 1.0, *MXX );
      }  // if (qc >= xc)
    } // if (M)

    // Compute new M-norms
    std::vector<STYPE> newDot(xc);
    MVT::MvDot( *XX, *MXX, &newDot[0] );
    
    int j;
    for (j = 0; j < xc; ++j) {

      if (kappa*newDot[j] < oldDot[j]) {
	
        // Apply another step of classical Gram-Schmidt
        //timeQtMult -= MyWatch.WallTime();
	MVT::MvTransMv( one, Q, *MXX, qTmx );
        //timeQtMult += MyWatch.WallTime();

        //timeQMult -= MyWatch.WallTime();
	MVT::MvTimesMatAddMv( -1.0, Q, qTmx, 1.0, *XX );
        //timeQMult += MyWatch.WallTime();

        // Update MX
        if (M) {
          if (qc >= xc) {
            //timeProj_MassMult -= MyWatch.WallTime();
            OPT::Apply( M, *XX, *MXX);
            //timeProj_MassMult += MyWatch.WallTime();
            //numProj_MassMult += xc;
          }
          else {
	    Teuchos::RefCountPtr<MV> MQ = MVT::Clone( Q, qc );
            //timeProj_MassMult -= MyWatch.WallTime();
            OPT::Apply( M, Q, *MQ);
            //timeProj_MassMult += MyWatch.WallTime();
            //numProj_MassMult += qc;
	    MVT::MvTimesMatAddMv( -1.0, *MQ, qTmx, 1.0, *MXX );
          } // if (qc >= xc)
        } // if (M)
	
        break;
      } // if (kappa*newDot[j] < oldDot[j])
    } // for (j = 0; j < xc; ++j)
    
    } // if (type != 2)
    //timeProj += MyWatch.WallTime();
    
    // Orthonormalize X 
    //  timeNorm -= MyWatch.WallTime();
    if (type != 1) {
      
      int j;
      int xc = MVT::GetNumberVecs( X );
      int xr = MVT::GetVecLength( X );
      int shift = (type == 2) ? 0 : MVT::GetNumberVecs( Q );
      int mxc = (M) ? MVT::GetNumberVecs( MX ) : xc;
      std::vector<int> index(1);
      Teuchos::RefCountPtr<MV> oldMXj;
      
      bool allocated = false;
      if (WS == 0) {
	allocated = true;
	WS = new STYPE[xr];
      }
      
      std::vector<STYPE> oldDot( 1 );
      STYPE *product = new STYPE[2*xc];
      
      STYPE dTmp;
      
      for (j = 0; j < howMany; ++j) {
	
	int numX = xc - howMany + j;
	int numMX = mxc - howMany + j;
	
      // Put zero vectors in X when we are exceeding the space dimension
      if (numX + shift >= globalSize) {
	index[0] = numX;
        Teuchos::RefCountPtr<MV> XXj = MVT::CloneView( X, &index[0], 1 );
        MVT::MvInit( XXj, zero );
        if (M) {
	  index[0] = numMX;
	  Teuchos::RefCountPtr<MV> MXXj = MVT::CloneView( MX, &index[0], 1 );
	  MVT::MvInit( MXXj, zero );
        }
        info = -1;
      }

      int numTrials;
      bool rankDef = true;
      for (numTrials = 0; numTrials < 10; ++numTrials) {

	index[0] = numX;
	Teuchos::RefCountPtr<MV> Xj = MVT::CloneView( X, &index[0], 1 );
	index[0] = numMX;
	Teuchos::RefCountPtr<MV> MXj;
	if (M)
	  MXj = MVT::CloneView( MX, &index[0], 1 );
	else
	  MXj = MVT::CloneView( X, &index[0], 1 );
	//
	// Compute M-norm
	//
	MVT::MvDot( *X, *MX, &oldDot[0] );
	//
	// Save old MXj vector.
	//
	oldMXj = MVT::CloneCopy( *MXj );

        if (numX > 0) {
	  //
          // Apply the first step of Gram-Schmidt
	  //
	  /*	  
          callBLAS.GEMV('T', xr, numX, 1.0, X.Values(), xr, MXj, 0.0, product + xc);
          MyComm.SumAll(product + xc, product, numX);
          callBLAS.GEMV('N', xr, numX, -1.0, X.Values(), xr, product, 1.0, Xj);
          if (M) {
            if (xc == mxc) {
              callBLAS.GEMV('N', xr, numX, -1.0, MXX, xr, product, 1.0, MXj);
            }
            else {
              Epetra_Vector XXj(View, X, numX);
              Epetra_Vector MXXj(View, MX, numMX);
              timeNorm_MassMult -= MyWatch.WallTime();
              M->Apply(XXj, MXXj);
              timeNorm_MassMult += MyWatch.WallTime();
              numNorm_MassMult += 1;
            }
          }

          STYPE dot = 0.0;
          dTmp = callBLAS.DOT(xr, Xj, MXj);
          MyComm.SumAll(&dTmp, &dot, 1);

          if (kappa*dot < oldDot) {
            callBLAS.GEMV('T', xr, numX, 1.0, X.Values(), xr, MXj, 0.0, product + xc);
            MyComm.SumAll(product + xc, product, numX);
            callBLAS.GEMV('N', xr, numX, -1.0, X.Values(), xr, product, 1.0, Xj);
            if (M) {
              if (xc == mxc) {
                callBLAS.GEMV('N', xr, numX, -1.0, MXX, xr, product, 1.0, MXj);
              }
              else {
                Epetra_Vector XXj(View, X, numX);
                Epetra_Vector MXXj(View, MX, numMX);
                timeNorm_MassMult -= MyWatch.WallTime();
                M->Apply(XXj, MXXj);
                timeNorm_MassMult += MyWatch.WallTime();
                numNorm_MassMult += 1;
              }
            }
          } // if (kappa*dot < oldDot)

        } // if (numX > 0)

        STYPE norm = 0.0;
        dTmp = callBLAS.DOT(xr, Xj, oldMXj);
        MyComm.SumAll(&dTmp, &norm, 1);

        if (norm > oldDot*eps*eps) {
          norm = 1.0/sqrt(norm);
          callBLAS.SCAL(xr, norm, Xj);
          if (M)
           callBLAS.SCAL(xr, norm, MXj);
          rankDef = false;
          break;
        }
        else {
          info += 1;
          Epetra_Vector XXj(View, X, numX);
          XXj.Random();
          Epetra_Vector MXXj(View, MX, numMX);
          if (M) {
            timeNorm_MassMult -= MyWatch.WallTime();
            M->Apply(XXj, MXXj);
            timeNorm_MassMult += MyWatch.WallTime();
            numNorm_MassMult += 1;
          }
          if (type == 0)
            massOrthonormalize(XXj, MXXj, M, Q, 1, 1, WS, kappa);
        } // if (norm > oldDot*eps*eps)

      }  // for (numTrials = 0; numTrials < 10; ++numTrials)
  
      if (rankDef == true) {
        Epetra_Vector XXj(View, X, numX);
        XXj.PutScalar(0.0);
        if (M) {
          Epetra_Vector MXXj(View, MX, numMX);
          MXXj.PutScalar(0.0);
        }
        info = -1;
        break;
      }
	  */
	  
	} // for (j = 0; j < howMany; ++j)
	
      }
      } // if (type != 1)
      timeNorm += MyWatch.WallTime();
    }
  return info;
  }
  
  template<class STYPE, class MV, class OP>
  void ModalSolverUtils<STYPE, MV, OP>::localProjection(int numRow, int numCol, int length,
							STYPE *U, int ldU, STYPE *MatV, int ldV,
							STYPE *UtMatV, int ldUtMatV, STYPE *work) const
  {
  }
  
  template<class STYPE, class MV, class OP>
  int ModalSolverUtils<STYPE, MV, OP>::directSolver(int, STYPE*, int, STYPE*, int, int&, 
						    STYPE*, int, STYPE*, int, int) const
  {
  }
  
  //-----------------------------------------------------------------------------
  // 
  //  SANITY CHECKING METHODS
  //
  //-----------------------------------------------------------------------------

  template<class STYPE, class MV, class OP>
  STYPE ModalSolverUtils<STYPE, MV, OP>::errorOrthogonality(const MV &X, const MV &R, 
							    const OP *M) const
  {
  }
  
  template<class STYPE, class MV, class OP>
  STYPE ModalSolverUtils<STYPE, MV, OP>::errorOrthonormality(const MV &X, const OP &M) const
  {
  }
  
  template<class STYPE, class MV, class OP>
  STYPE ModalSolverUtils<STYPE, MV, OP>::errorEquality(const MV &X, const MV &MX, 
						       const OP &M) const
  {
  }    
  
  template<class STYPE, class MV, class OP>
  int ModalSolverUtils<STYPE, MV, OP>::inputArguments(const int &numEigen, const OP &K, 
						      const OP &M, const OP &P,
						      const MV &Q, const int &minSize) const
  {
  }  

} // end namespace Anasazi

#endif // ANASAZI_MODAL_SOLVER_UTILS_HPP

