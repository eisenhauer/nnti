
// @HEADER
// ***********************************************************************
// 
//                Amesos: Direct Sparse Solver Package
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

/*!
 * \file Amesos_Umfpack.h
 *
 * \class Amesos_Klu
 *
 * \brief Interface to UMFPACK.
 *
 * \date Last updated on 24-May-05.
 */

#ifndef AMESOS_UMFPACK_H
#define AMESOS_UMFPACK_H

#include "Amesos_ConfigDefs.h"
#include "Amesos_BaseSolver.h"
#include "Amesos_BaseSolver.h"
#include "Amesos_NoCopiable.h"
#include "Amesos_Utils.h"
#include "Amesos_Time.h"
#include "Amesos_Status.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_LinearProblem.h"
#include "Epetra_Time.h"
#include "Epetra_Import.h"
#include "Teuchos_RefCountPtr.hpp"
#ifdef EPETRA_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_Comm.h"
#endif

using namespace Teuchos;

//! Class Amesos_Umfpack:  An object-oriented wrapper for UMFPACK.
/*!  Amesos_Umfpack will solve a linear systems of equations: <TT>A X = B</TT>
   using Epetra objects and the UMFPACK solver library, where
  <TT>A</TT> is an Epetra_RowMatrix and <TT>X</TT> and <TT>B</TT> are 
  Epetra_MultiVector objects.

 
*/
class Amesos_Umfpack: public Amesos_BaseSolver,  Amesos_Time, 
  Amesos_NoCopiable, Amesos_Utils, Amesos_Status { 

public: 

  //@{ \name Constructor methods
  //! Amesos_Umfpack Constructor.
  /*! Creates an Amesos_Umfpack instance, using an Epetra_LinearProblem,
      passing in an already-defined Epetra_LinearProblem object. 

      Note: The operator in LinearProblem must be an
      Epetra_RowMatrix.

  */
  Amesos_Umfpack( const Epetra_LinearProblem& LinearProblem );

  //! Amesos_Umfpack Destructor.
  /*! Completely deletes an Amesos_Umfpack object.  
  */
  ~Amesos_Umfpack(void);

  //@}
  //@{ \name Mathematical functions.

  int SymbolicFactorization();

  int NumericFactorization();

  int Solve();

  //@}
  //@{ \name Additional methods required to support the Epetra_Operator interface.

  const Epetra_LinearProblem *GetProblem() const { return(Problem_); };

  //! Returns true if UMFPACK can handle this matrix shape 
  /*! Returns true if the matrix shape is one that UMFPACK can
    handle. UMFPACK only works with square matrices.  
  */
  bool MatrixShapeOK() const ;

  int SetUseTranspose(bool UseTranspose) {UseTranspose_ = UseTranspose; return(0);};

  bool UseTranspose() const {return(UseTranspose_);};

  const Epetra_Comm & Comm() const {return(GetProblem()->GetOperator()->Comm());};

  //! Returns an estimate of the reciprocal of the condition number 
  /*  Rcond is an estimate of the reciprocal of the condition number of the 
      matrix at the time of the most recent call to NumericFactorization()
      Rcond = min(abs(diag))/max(abs(diag)) see Umfpack documentatoin
      for details.  
   */
  double GetRcond() const ; 

  int SetParameters( Teuchos::ParameterList &ParameterList ) ;

  //! Prints timing information
  void PrintTiming() const;
  
  //! Prints information about the factorization and solution phases.
  void PrintStatus() const;

private:  
  
  //@}
  //@{ \name Utility Methods
  
  //! Returns a pointer to the linear system matrix.
  Epetra_RowMatrix* Matrix()
  {
    return(dynamic_cast<Epetra_RowMatrix *>(Problem_->GetOperator()));
  }

  //! Converts matrix to a serial Epetra_CrsMatrix
  int ConvertToSerial(const bool FirstTime);

  /*
    ConvertToUmfpackCRS - Convert matirx to form expected by Umfpack: Ai, Ap, Aval
    Preconditions:
      numentries_, NumGloalElements_ and SerialMatrix_ must be set.
    Postconditions:
      Ai, Ap, and Aval are resized and populated with a compresses row storage 
      version of the input matrix A.
  */
  int ConvertToUmfpackCRS();     

  /*
    PerformSymbolicFactorization - Call Umfpack to perform symbolic factorization
    Preconditions:
      IsLocal must be set to 1 if the input matrix is entirely stored on process 0
      Ap, Ai and Aval are a compressed row storage version of the input matrix A.
    Postconditions:
      Symbolic points to an UMFPACK internal opaque object containing the
        symbolic factorization and accompanying information.  
      SymbolicFactorizationOK_ = true; 
    Note:  All action is performed on process 0
  */
      
  int PerformSymbolicFactorization(); 

  /*
    PerformNumericFactorization - Call Umfpack to perform numeric factorization
    Preconditions:
      IsLocal must be set 
      Ap, Ai and Aval are a compressed row storage version of the input matrix A.
      Symbolic must be set
    Postconditions:
      Numeric points to an UMFPACK internal opaque object containing the
        numeric factorization and accompanying information.  
      NumericFactorizationOK_ = true; 
    Note:  All action is performed on process 0
  */
  int PerformNumericFactorization(); 

  inline const Epetra_Import& Importer() const
  {
    return(*(ImportToSerial_.get()));
  }

  inline const Epetra_Map& SerialMap() const
  {
    return(*(SerialMap_.get()));
  }

  inline const Epetra_CrsMatrix& SerialCrsMatrix() const
  {
    return(*(SerialCrsMatrixA_.get()));
  }

  inline Epetra_CrsMatrix& SerialCrsMatrix()
  {
    return(*(SerialCrsMatrixA_.get()));
  }

  // @}
  
  //! Umfpack internal opaque object
  void *Symbolic;
  //! Umfpack internal opaque object
  void *Numeric;

  //!  Ap, Ai, Aval form the compressed row storage used by Umfpack
  vector <int> Ap;
  vector <int> Ai;
  vector <double> Aval;

  //! Process number (i.e. Comm().MyPID() )
  int iam;
  //! 1 if Problem_->GetOperator() is stored entirely on process 0
  int IsLocal_;
  //! Number of non-zero entries in Problem_->GetOperator()
  int numentries_;
  //! Number of rows and columns in the Problem_->GetOperator()
  int NumGlobalElements_;

  //! Points to a Serial Map (unused if IsLocal == 1 ) 
  RefCountPtr<Epetra_Map> SerialMap_;
  //! Points to a Serial Copy of A
  /* If IsLocal==1 - Points to the original matrix 
   * If  IsLocal==0 - Points to SerialCrsMatrixA
   */
  Epetra_RowMatrix* SerialMatrix_;

  RefCountPtr<Epetra_CrsMatrix> SerialCrsMatrixA_;

  //! If \c true, solve the problem with the transpose.
  bool UseTranspose_;
  //! Pointer to the linear problem to solve.
  const Epetra_LinearProblem * Problem_;
  //! Reciprocal condition number estimate
  mutable double Rcond_;
  //  True if Rcond_ is the same on all processes
  mutable bool RcondValidOnAllProcs_;
  //! Importer from distributed to serial (all rows on process 0).
  RefCountPtr<Epetra_Import> ImportToSerial_;
  
};  // class Amesos_Umfpack  
#endif /* AMESOS_UMFPACK_H */
