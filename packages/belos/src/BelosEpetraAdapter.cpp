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


/*! \file BelosEpetraAdapter.cpp
    \brief Implementation of the interfaces between Belos virtual classes and Epetra concrete classes.
*/

#include "BelosEpetraAdapter.hpp"

using namespace Belos;
  
  //-------------------------------------------------------------
  
  //////////////////////////////////////////////////////////////////////
  // Construction/Destruction
  //////////////////////////////////////////////////////////////////////
  
  
EpetraMultiVec::EpetraMultiVec(const Epetra_BlockMap& Map, double * array, 
			       const int numvecs, const int stride)
  : Epetra_MultiVector(Copy, Map, array, stride, numvecs) 
{
}


EpetraMultiVec::EpetraMultiVec(const Epetra_BlockMap& Map, const int numvecs, bool zeroOut)
  : Epetra_MultiVector(Map, numvecs, zeroOut) 
{
}


EpetraMultiVec::EpetraMultiVec(Epetra_DataAccess CV, const Epetra_MultiVector& P_vec, 				
			       const std::vector<int>& index )
  : Epetra_MultiVector(CV, P_vec, &(const_cast<std::vector<int> &>(index))[0], index.size())
{
}


EpetraMultiVec::EpetraMultiVec(const Epetra_MultiVector& P_vec)
  : Epetra_MultiVector(P_vec) 
{
}


EpetraMultiVec::~EpetraMultiVec() 
{
}
//
//  member functions inherited from Belos::MultiVec
//
//
//  Simulating a virtual copy constructor. If we could rely on the co-variance
//  of virtual functions, we could return a pointer to EpetraMultiVec
//  (the derived type) instead of a pointer to the pure virtual base class.
//

MultiVec<double>* EpetraMultiVec::Clone ( const int numvecs ) const
{
  EpetraMultiVec * ptr_apv = new EpetraMultiVec(Map(), numvecs, false);
  return ptr_apv; // safe upcast.
}
//
//  the following is a virtual copy constructor returning
//  a pointer to the pure virtual class. std::vector values are
//  copied.
//

MultiVec<double>* EpetraMultiVec::CloneCopy() const
{
  EpetraMultiVec *ptr_apv = new EpetraMultiVec(*this);
  return ptr_apv; // safe upcast
}


MultiVec<double>* EpetraMultiVec::CloneCopy ( const std::vector<int>& index ) const
{
  EpetraMultiVec * ptr_apv = new EpetraMultiVec(Copy, *this, index);
  return ptr_apv; // safe upcast.
}


MultiVec<double>* EpetraMultiVec::CloneView ( const std::vector<int>& index ) 
{
  EpetraMultiVec * ptr_apv = new EpetraMultiVec(View, *this, index);
  return ptr_apv; // safe upcast.
}
  

void EpetraMultiVec::SetBlock( const MultiVec<double>& A, const std::vector<int>& index ) 
{	
  EpetraMultiVec temp_vec(View, *this, index);
  
  int numvecs = index.size();
  if ( A.GetNumberVecs() != numvecs ) {
    std::vector<int> index2( numvecs );
    for(int i=0; i<numvecs; i++)
      index2[i] = i;
    EpetraMultiVec *tmp_vec = dynamic_cast<EpetraMultiVec *>(&const_cast<MultiVec<double> &>(A)); assert(tmp_vec!=NULL);
    EpetraMultiVec A_vec(View, *tmp_vec, index2);
    temp_vec.MvAddMv( 1.0, A_vec, 0.0, A_vec );
  }
  else {
    temp_vec.MvAddMv( 1.0, A, 0.0, A );
  }
}								

//-------------------------------------------------------------
//
// *this <- alpha * A * B + beta * (*this)
//
//-------------------------------------------------------------

void EpetraMultiVec::MvTimesMatAddMv ( const double alpha, const MultiVec<double>& A, 
				       const Teuchos::SerialDenseMatrix<int,double>& B, const double beta ) 
{
  Epetra_LocalMap LocalMap(B.numRows(), 0, Map().Comm());
  Epetra_MultiVector B_Pvec(Copy, LocalMap, B.values(), B.stride(), B.numCols());
  
  EpetraMultiVec *A_vec = dynamic_cast<EpetraMultiVec *>(&const_cast<MultiVec<double> &>(A)); assert(A_vec!=NULL);
  
  int info = Multiply( 'N', 'N', alpha, *A_vec, B_Pvec, beta );
  TEST_FOR_EXCEPTION(info!=0, EpetraMultiVecFailure, 
		     "Belos::EpetraMultiVec::MvTimesMatAddMv call to Multiply() returned a nonzero value.");

}

//-------------------------------------------------------------
//
// *this <- alpha * A + beta * B
//
//-------------------------------------------------------------
  
void EpetraMultiVec::MvAddMv ( const double alpha , const MultiVec<double>& A, 
			       const double beta, const MultiVec<double>& B) 
{
  EpetraMultiVec *A_vec = dynamic_cast<EpetraMultiVec *>(&const_cast<MultiVec<double> &>(A)); assert(A_vec!=NULL);
  EpetraMultiVec *B_vec = dynamic_cast<EpetraMultiVec *>(&const_cast<MultiVec<double> &>(B)); assert(B_vec!=NULL);
  
  int info = Update( alpha, *A_vec, beta, *B_vec, 0.0 );
  TEST_FOR_EXCEPTION(info!=0, EpetraMultiVecFailure, 
		     "Belos::EpetraMultiVec::MvAddMv call to Update() returned a nonzero value.");
}

//-------------------------------------------------------------
//
// this[i] = alpha[i] * this[i]
//
//-------------------------------------------------------------
void EpetraMultiVec::MvScale ( const std::vector<double>& alpha )
{
  // Check to make sure the vector is as long as the multivector has columns.
  int numvecs = this->NumVectors();
  assert( (int)alpha.size() == numvecs );
  int ret = 0;
  std::vector<int> tmp_index( 1, 0 );
  for (int i=0; i<numvecs; i++) {
    Epetra_MultiVector temp_vec(View, *this, &tmp_index[0], 1);
    ret = temp_vec.Scale( alpha[i] );
    assert (ret == 0);
    tmp_index[0]++;
  }
}

//-------------------------------------------------------------
//
// dense B <- alpha * A^T * (*this)
//
//-------------------------------------------------------------

void EpetraMultiVec::MvTransMv ( const double alpha, const MultiVec<double>& A,
				 Teuchos::SerialDenseMatrix<int,double>& B) const
{    
  EpetraMultiVec *A_vec = dynamic_cast<EpetraMultiVec *>(&const_cast<MultiVec<double> &>(A));
  
  if (A_vec) {
    Epetra_LocalMap LocalMap(B.numRows(), 0, Map().Comm());
    Epetra_MultiVector B_Pvec(View, LocalMap, B.values(), B.stride(), B.numCols());
    
    int info = B_Pvec.Multiply( 'T', 'N', alpha, *A_vec, *this, 0.0 );
    TEST_FOR_EXCEPTION(info!=0, EpetraMultiVecFailure, 
		       "Belos::EpetraMultiVec::MvTransMv call to Multiply() returned a nonzero value.");
  }
}

//-------------------------------------------------------------
//
// b[i] = A[i]^T * this[i]
// 
//-------------------------------------------------------------

void EpetraMultiVec::MvDot ( const MultiVec<double>& A, std::vector<double>& b ) const
{
  EpetraMultiVec *A_vec = dynamic_cast<EpetraMultiVec *>(&const_cast<MultiVec<double> &>(A)); assert(A_vec!=NULL);
  if (A_vec && ( (int)b.size() >= A_vec->NumVectors() ) ) {
     int info = this->Dot( *A_vec, &b[0] );
     TEST_FOR_EXCEPTION(info!=0, EpetraMultiVecFailure, 
			"Belos::EpetraMultiVec::MvDot call to Dot() returned a nonzero value.");   
  }
}

//-------------------------------------------------------------
//
// alpha[i] = norm of i-th column of (*this)
//
//-------------------------------------------------------------

void EpetraMultiVec::MvNorm ( std::vector<double>& normvec, NormType norm_type ) const {
  if ((int)normvec.size() >= GetNumberVecs()) {
    int info = 0;
    switch( norm_type ) {
    case ( OneNorm ) :
      info = Norm1(&normvec[0]);
      break;
    case ( TwoNorm ) :
      info = Norm2(&normvec[0]);
      break;
    case ( InfNorm ) :	
      info = NormInf(&normvec[0]);
      break;
    default:
      break;
    }
    TEST_FOR_EXCEPTION(info!=0, EpetraMultiVecFailure, 
		       "Belos::EpetraMultiVec::MvNorm call to Norm() returned a nonzero value.");
  }
}

///////////////////////////////////////////////////////////////
//
// implementation of the BelosEpetraOp class.
//
////////////////////////////////////////////////////////////////////
//
// BelosOperator constructors
//

EpetraOp::EpetraOp( const Teuchos::RCP<Epetra_Operator> &Op ) 
  : Epetra_Op(Op)
{
}

//
// BelosOperator applications
//
void EpetraOp::Apply ( const MultiVec<double>& x, 
                       MultiVec<double>& y, ETrans trans ) const {
  int info=0;
  MultiVec<double> & temp_x = const_cast<MultiVec<double> &>(x);
  Epetra_MultiVector* vec_x = dynamic_cast<Epetra_MultiVector* >(&temp_x);
  Epetra_MultiVector* vec_y = dynamic_cast<Epetra_MultiVector* >(&y);
  //
  TEST_FOR_EXCEPTION( vec_x==NULL || vec_y==NULL, EpetraOpFailure, 
		      "Belos::EpetraOp::Apply, x and/or y cannot be dynamic cast to an Epetra_MultiVector.");
  //
  // Set the operator to apply the transpose if that is requested.
  //
  if ( trans ) { 
    info = Epetra_Op->SetUseTranspose( true );
  }
  info = Epetra_Op->Apply( *vec_x, *vec_y );
  
  if ( trans ) { 
    info = Epetra_Op->SetUseTranspose( false );
  }
  
  TEST_FOR_EXCEPTION(info!=0, EpetraOpFailure, 
		     "Belos::EpetraOp::Apply call to Apply() returned a nonzero value."); 

}

///////////////////////////////////////////////////////////////
//
// implementation of the BelosEpetraPrecOp class.
//
////////////////////////////////////////////////////////////////////
//
// BelosOperator constructors
//

EpetraPrecOp::EpetraPrecOp( const Teuchos::RCP<Epetra_Operator> &Op ) 
  : Epetra_Op(Op)
{
}

//
// BelosOperator applications
//
void EpetraPrecOp::Apply ( const MultiVec<double>& x, 
			   MultiVec<double>& y, ETrans trans ) const {
  int info=0;
  MultiVec<double> & temp_x = const_cast<MultiVec<double> &>(x);
  Epetra_MultiVector* vec_x = dynamic_cast<Epetra_MultiVector* >(&temp_x);
  Epetra_MultiVector* vec_y = dynamic_cast<Epetra_MultiVector* >(&y);
  //
  TEST_FOR_EXCEPTION( vec_x==NULL || vec_y==NULL, EpetraOpFailure, 
		      "Belos::EpetraOp::Apply, x and/or y cannot be dynamic cast to an Epetra_MultiVector.");
  //
  // Set the operator to apply the transpose if that is requested.
  //
  if ( trans ) { 
    info = Epetra_Op->SetUseTranspose( true );
  }
  info = Epetra_Op->ApplyInverse( *vec_x, *vec_y );
  
  if ( trans ) { 
    info = Epetra_Op->SetUseTranspose( false );
  }
  
  TEST_FOR_EXCEPTION(info!=0, EpetraOpFailure, 
		     "Belos::EpetraOp::Apply call to Apply() returned a nonzero value."); 
}

int EpetraPrecOp::Apply ( const Epetra_MultiVector &X, Epetra_MultiVector &Y ) const 
{
  //
  // This operator applies Y = A^{-1}*X
  //
  int info = Epetra_Op->ApplyInverse( X, Y );
  return info;
}

int EpetraPrecOp::ApplyInverse( const Epetra_MultiVector &X, Epetra_MultiVector &Y ) const
{
  //
  // This operator applies Y = A*X
  //
  int info = Epetra_Op->Apply( X, Y );
  return info;
}

