// @HEADER
// ************************************************************************
//
//                           Intrepid Package
//                 Copyright (2007) Sandia Corporation
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
// Questions? Contact Pavel Bochev (pbboche@sandia.gov) or
//                    Denis Ridzal (dridzal@sandia.gov).
//
// ************************************************************************
// @HEADER

/** \file
\brief Definition file for utility classes providing basic linear algebra functionality.
\author Created by P. Bochev, D. Ridzal, and D. Day.
*/

#include <Teuchos_Array.hpp>


namespace Intrepid {
  //===========================================================================//
  //                                                                           //
  //              Member function definitions of the class Point.              //
  //                                                                           //
  //===========================================================================//
  template<class Scalar>
  Point<Scalar>::Point(const Point<Scalar>& right) {
    data_.resize(right.getDim());
    data_ = right.data_;
    frameKind_ = right.frameKind_;
  }
  
  template<class Scalar>
  Point<Scalar>::Point(const int dim,
                       const EFrame frameKind): frameKind_(frameKind) {
    assert(0 < dim && dim <= INTREPID_MAX_DIMENSION);
    data_.assign(dim, (Scalar)0);
  }

  template<class Scalar>
  Point<Scalar>::Point(const Scalar x,
                       const EFrame frameKind) : frameKind_(frameKind) {
    data_.resize(1);
    data_[0] = x;
  }
  
  template<class Scalar>
  Point<Scalar>::Point(const Scalar x, 
                       const Scalar y, 
                       const EFrame frameKind) : frameKind_(frameKind)  {
    data_.resize(2);
    data_[0] = x;
    data_[1] = y;
  }
  
  template<class Scalar>
  Point<Scalar>::Point(const int x, 
                       const int y, 
                       const EFrame frameKind) : frameKind_(frameKind)  {
    data_.resize(2);
    data_[0] = (Scalar)x;
    data_[1] = (Scalar)y;
  }
  
  template<class Scalar>
  Point<Scalar>::Point(const Scalar x, 
                       const Scalar y, 
                       const Scalar z, 
                       const EFrame frameKind) : frameKind_(frameKind)  {
    data_.resize(3);
    data_[0] = x;
    data_[1] = y;
    data_[2] = z;
  }
  
  template<class Scalar>
  Point<Scalar>::Point(const Scalar* dataPtr, 
                       const int dim, 
                       const EFrame frameKind) : frameKind_(frameKind)  {
    assert(0 < dim && dim <= INTREPID_MAX_DIMENSION);
    data_.assign(dataPtr, dataPtr + dim);
  }
  
  template<class Scalar>
  inline int Point<Scalar>::getDim() const {
    return data_.size();
  }
  
  template<class Scalar>
  inline EFrame Point<Scalar>::getFrameKind() const {
    return frameKind_;
  }
  
  template<class Scalar>
  inline const char* Point<Scalar>::getFrameName() const {
    return FrameNames[frameKind_];
  }

  template<class Scalar>
  inline const Teuchos::Array<Scalar> & Point<Scalar>::getCoordinates() const {
    return data_;
  }
  
  template<class Scalar>
  inline void Point<Scalar>::setCoordinates(const Scalar* dataPtr, 
                                     const int targetDim) {
    assert(this -> getDim() == targetDim);
    for(int dim = 0; dim < targetDim; dim++){
      data_[dim] = dataPtr[dim];
    }
  }
  
  template<class Scalar>
  inline void Point<Scalar>::setFrameKind(const EFrame newFrameKind) {
    frameKind_ = newFrameKind;
  }
  
  
  template<class Scalar>
  Scalar Point<Scalar>::distance(const Point& endPoint) const {
    assert (data_.size() == endPoint.data_.size());
    Scalar temp = (Scalar)0.0;
    for(unsigned int i = 0; i < data_.size(); i++) 
    {
      Scalar diff = data_[i] - endPoint.data_[i];
      temp += diff*diff; 
    }
    return (Scalar)std::sqrt(temp);
  }
  
  template<class Scalar>
  Scalar Point<Scalar>::norm(ENorm normType) const{
    Scalar temp = (Scalar)0.0;
    switch(normType) {
      case NORM_TWO:
        for(int i = 0; i < this -> getDim(); i++){
          temp += data_[i]*data_[i]; 
        }
          return std::sqrt(temp);
        break;
      case NORM_INF:
        temp = std::abs(data_[0]);
        for(int i = 1; i < this -> getDim(); i++){
          Scalar absData = std::abs(data_[i]);
          if (temp < absData) temp = absData; 
        }
        return temp;
        break;
      case NORM_ONE:
        for(int i = 0; i < this ->getDim(); i++){
          temp += std::abs(data_[i]); 
        }
        return temp;
        break;
      default:
        return 0;
    }
  }
  
  template<class Scalar>
  const Scalar & Point<Scalar>::operator [] (const int coordinateId) const {
    return data_[coordinateId];
  }
  
  template<class Scalar>
  inline Point<Scalar> & Point<Scalar>::operator = (const Point<Scalar>& right)
{
    assert(this != &right);            // check for self-assignment
    assert(getDim()==right.getDim());  // dimensions must match    
    data_ = right.data_;
    frameKind_ = right.frameKind_;      // changes point type of left!
    return *this;
}

template<class Scalar>
inline Point<Scalar> & Point<Scalar>::operator += (const Point<Scalar>& right)
{
  assert(this != &right);  // check for self-assignment
  switch (getDim()) {      // does not change point type of left!
    case 3:
      data_[0] += right.data_[0];
      data_[1] += right.data_[1];
      data_[2] += right.data_[2];
      return *this;
    case 2:
      data_[0] += right.data_[0];
      data_[1] += right.data_[1];
      return *this;
    default: // case 1
      data_[0] += right.data_[0];
      return *this;
  }
}

template<class Scalar>
inline Point<Scalar> & Point<Scalar>::operator ^= (const Point<Scalar>& right)
{                                       // Does not change frameKind_!
  assert(this != &right);               // check for self-assignment
  assert(getDim()==3);                  // cross product only in 3D
  Teuchos::Array<Scalar> tmp(3);
  tmp[0] = data_[1]*right.data_[2]-data_[2]*right.data_[1];
  tmp[1] = data_[2]*right.data_[0]-data_[0]*right.data_[2];
  tmp[2] = data_[0]*right.data_[1]-data_[1]*right.data_[0];
  data_   = tmp;
  return *this;
}

template<class Scalar>
inline Point<Scalar> & Point<Scalar>::operator -= (const Point<Scalar>& right)
{                                         // Does not change frameKind_!
  assert(this != &right);                 // check for self-assignment
  switch (getDim()) {
    case 3:
      data_[0] -= right.data_[0];
      data_[1] -= right.data_[1];
      data_[2] -= right.data_[2];
      return *this;
    case 2:
      data_[0] -= right.data_[0];
      data_[1] -= right.data_[1];
      return *this;
    default: // case 1
      data_[0] -= right.data_[0];
      return *this;
  }
}

template<class Scalar>
inline Point<Scalar> & Point<Scalar>::operator *= (const Scalar left)
{                                         // Does not change frameKind_!
  switch (getDim()) {
    case 3:
      data_[0] *= left;
      data_[1] *= left;
      data_[2] *= left;
      return *this;
    case 2:
      data_[0] *= left;
      data_[1] *= left;
      return *this;
    default: // case 1
      data_[0] *= left;
      return *this;
  }
}
//===========================================================================//
//                                                                           //
//           END of member definitions; START friends and related            //
//                                                                           //
//===========================================================================//

template<class Scalar>
const Point<Scalar> operator ^ (const Point<Scalar>& left, const Point<Scalar>& right) {
  Point<Scalar> result(left);
  result ^= right;
  return result;
}

template<class Scalar>
const Point<Scalar> operator + (const Point<Scalar>& left, const Point<Scalar>& right) {
  Point<Scalar> result(left);
  result += right;
  return result;
}

template<class Scalar>
const Point<Scalar> operator - (const Point<Scalar>& left, const Point<Scalar>& right) {
  Point<Scalar> result(left);
  result -= right;
  return result;
}

template<class Scalar>
const Point<Scalar> operator * (const Scalar left, const Point<Scalar>& right) {
  Point<Scalar> result(right);
  result *= left;
  return result;
}

template<class Scalar>
const Scalar operator * (const Point<Scalar>& left, const Point<Scalar>& right) {
  assert(left.getDim()==right.getDim());
  Scalar result;
  switch (left.getDim()) {
    case 3:
      result = left.getCoordinates()[0]*right.getCoordinates()[0] +
      left.getCoordinates()[1]*right.getCoordinates()[1] +
      left.getCoordinates()[2]*right.getCoordinates()[2];
      return result;
    case 2:
      result = left.getCoordinates()[0]*right.getCoordinates()[0] +
      left.getCoordinates()[1]*right.getCoordinates()[1];
      return result;
    default: // case 1
      result = left.getCoordinates()[0]*right.getCoordinates()[0];
      return result;
  }
}

template<class Scalar>
std::ostream& operator << (std::ostream& os, const Point<Scalar>& point) {
  os.setf(std::ios_base::scientific, std::ios_base::floatfield);
  os.setf(std::ios_base::right);
  os.precision(4);
  
  int dim = point.getDim();
  os << "  " <<  dim << "D "<< point.getFrameName() <<" Point (";
  for (int j=0; j < dim; j++) {
    os << std::setw(12) << point.getCoordinates()[j];
  }
  std::cout << ")";
  return os;
}

// End member, friend, and related function definitions of class Point.

//===========================================================================//
//                                                                           //
//              Member function definitions of the class Matrix.             //
//                                                                           //
//===========================================================================//

template<class Scalar>
Matrix<Scalar>::Matrix(const Matrix<Scalar>& right){
  elements_.assign(right.elements_.begin(), right.elements_.end());
}

template<class Scalar>
Matrix<Scalar>::Matrix(const int dim){
  assert(0 < dim && dim <= INTREPID_MAX_DIMENSION);
  Teuchos::Array<Scalar> tmp(dim);
  elements_.assign(dim,tmp);
}

template<class Scalar>
Matrix<Scalar>::Matrix(const Scalar* dataPtr, const int dim){
  assert(0 < dim && dim <= INTREPID_MAX_DIMENSION);
  Teuchos::Array<Scalar> tmp(dim);
  elements_.assign(dim,tmp);
  for(int i=0; i < dim; ++i){
    for(int j=0; j < dim; ++j){
      elements_[i][j]=*(dataPtr+i*dim+j);
    }
  }
}

template<class Scalar>
void Matrix<Scalar>::setElements(const Scalar* dataPtr, const int dim){
  assert(this -> getDim() == dim);
  for(int i=0; i < dim; ++i){
    for(int j=0; j < dim; ++j){
      elements_[i][j]=*(dataPtr+i*dim+j);
    }
  }
}

template<class Scalar>
int Matrix<Scalar>::getDim() const {
  return elements_.size();
}

template<class Scalar>
Scalar Matrix<Scalar>::getElement(int rowID, int colID) const {
  assert(0 <= rowID && rowID < (int)elements_.size());
  assert(0 <= colID && colID < (int)elements_.size());
  return( elements_[rowID][colID]);
}

template<class Scalar>
Point<Scalar> Matrix<Scalar>::getColumn(int colID) const {
  assert(0<=colID && (unsigned int)colID<elements_.size());
  switch (getDim()) {
    case 3:
      return Point<Scalar>(elements_[0][colID],
                           elements_[1][colID],
                           elements_[2][colID]);
    case 2:
      return Point<Scalar>(elements_[0][colID],
                           elements_[1][colID]);
    default: // = case 1
      return Point<Scalar>(elements_[0][colID]);
  }
}

template<class Scalar>
Point<Scalar> Matrix<Scalar>::getRow(int rowID) const {
  assert(0 <= rowID && rowID < (int)elements_.size());
  switch (getDim()) {
    case 3:
      return Point<Scalar>(elements_[rowID][0],
                           elements_[rowID][1],
                           elements_[rowID][2]);
      break;
    case 2:
      return Point<Scalar>(elements_[rowID][0],
                           elements_[rowID][1]);
      break;
    default: // = case 1
      return Point<Scalar>(elements_[rowID][0]);
  }
}

template<class Scalar>
Matrix<Scalar> Matrix<Scalar>::getTranspose() const {
  Matrix<Scalar> transpose(getDim());
  for(int i=0; i < getDim(); ++i){
    transpose.elements_[i][i]=elements_[i][i];    // Set diagonal elements
    for(int j=i+1; j < getDim(); ++j){
      transpose.elements_[i][j]=elements_[j][i];  // Set off-diagonal elements
      transpose.elements_[j][i]=elements_[i][j];
    }
  }
  return transpose;
}

template<class Scalar>
Matrix<Scalar> Matrix<Scalar>::getInverse() const {
  int dim = getDim();
  int i,j,rowID = 0, colID = 0;
  int rowperm[3]={0,1,2};
  int colperm[3]={0,1,2}; // Complete pivoting
  Scalar emax(0), determinant(0);
  
  if (dim==3) {
    for(i=0; i < 3; ++i){
      for(j=0; j < 3; ++j){
        if( std::abs( elements_[i][j] ) >  emax){
          rowID = i;  colID = j; emax = std::abs( elements_[i][j] );
        }
      }
    }
    if( emax == 0 ) std::cerr <<" Matrix getInverse: Zero matrix\n";
    if( rowID ){
      rowperm[0] = rowID;
      rowperm[rowID] = 0;
    }
    if( colID ){
      colperm[0] = colID;
      colperm[colID] = 0;
    }
    Scalar Inverse[3][3], B[3][3], S[2][2], Bi[3][3]; // B=rowperm elements_ colperm, S=Schur complement(Boo)
    for(i=0; i < 3; ++i){
      for(j=0; j < 3; ++j){
        B[i][j] = elements_[rowperm[i]][colperm[j]];
      }
    }
    B[1][0] /= B[0][0]; B[2][0] /= B[0][0];// B(:,0)/=pivot
      for(i=0; i < 2; ++i){
        for(j=0; j < 2; ++j){
          S[i][j] = B[i+1][j+1] - B[i+1][0] * B[0][j+1]; // S = B -z*y'
        }
      }
      Scalar detS = S[0][0]*S[1][1]- S[0][1]*S[1][0], Si[2][2];
      if( detS == 0 ) std::cerr <<" Matrix getInverse : Singular matrix\n";
      
      Si[0][0] =  S[1][1]/detS;                  Si[0][1] = -S[0][1]/detS;
      Si[1][0] = -S[1][0]/detS;                  Si[1][1] =  S[0][0]/detS;
      
      for(j=0; j<2;j++)
        Bi[0][j+1] = -( B[0][1]*Si[0][j] + B[0][2]* Si[1][j])/B[0][0];
      for(i=0; i<2;i++)
        Bi[i+1][0] = -(Si[i][0]*B[1][0] + Si[i][1]*B[2][0]);
      
      Bi[0][0] =  ((Scalar)1/B[0][0])-Bi[0][1]*B[1][0]-Bi[0][2]*B[2][0]; 
      Bi[1][1] =  Si[0][0]; 
      Bi[1][2] =  Si[0][1];
      Bi[2][1] =  Si[1][0]; 
      Bi[2][2] =  Si[1][1];
      for(i=0; i < 3; ++i){
        for(j=0; j < 3; ++j){
          Inverse[i][j] = Bi[colperm[i]][rowperm[j]]; // return inverse in place
        }
      }
      return Matrix<Scalar>(&Inverse[0][0],dim);
  }
  else if (dim==2) {
    Scalar Inverse[2][2];
    determinant= det();
    Inverse[0][0] =   elements_[1][1]/determinant;
    Inverse[0][1] = - elements_[0][1]/determinant;
    //
    Inverse[1][0] = - elements_[1][0]/determinant;
    Inverse[1][1] =   elements_[0][0]/determinant;
    return Matrix<Scalar>(&Inverse[0][0],dim);
  }
  else { // dim==1
    Scalar Inverse[1][1];
    determinant= det();
    Inverse[0][0] = (Scalar)1 / elements_[0][0];
    return Matrix<Scalar>(&Inverse[0][0],dim);
  }
}

template<class Scalar>
void Matrix<Scalar>::transpose() {
  Scalar temp(0);
  for(int i=0; i < getDim(); ++i){
    for(int j=i+1; j < getDim(); ++j){
      temp=elements_[i][j];
      elements_[i][j]=elements_[j][i];            // Leave diagonal elements alone!
      elements_[j][i]=temp;
    }
  }
}

template<class Scalar>
Scalar Matrix<Scalar>::det() const {
  int i,j,rowID = 0;
  int colID = 0;
  int rowperm[3]={0,1,2};
  int colperm[3]={0,1,2}; // Complete pivoting
  Scalar emax(0), determinant(0);
  switch (getDim()) {
    case 3:
      for(i=0; i < 3; ++i){
        for(j=0; j < 3; ++j){
          if( std::abs( elements_[i][j] ) >  emax){
            rowID = i;  colID = j; emax = std::abs( elements_[i][j] );
          }
        }
      }
      if( emax > 0 ){
        if( rowID ){
          rowperm[0] = rowID;
          rowperm[rowID] = 0;
        }
        if( colID ){
          colperm[0] = colID;
          colperm[colID] = 0;
        }
        Scalar B[3][3], S[2][2]; // B=rowperm elements_ colperm, S=Schur complement(Boo)
        for(i=0; i < 3; ++i){
          for(j=0; j < 3; ++j){
            B[i][j] = elements_[rowperm[i]][colperm[j]];
          }
        }
        B[1][0] /= B[0][0]; B[2][0] /= B[0][0];// B(:,0)/=pivot
          for(i=0; i < 2; ++i){
            for(j=0; j < 2; ++j){
              S[i][j] = B[i+1][j+1] - B[i+1][0] * B[0][j+1]; // S = B -z*y'
            }
          }
          determinant = B[0][0] * (S[0][0] * S[1][1] - S[0][1] * S[1][0]); // det(B)
          if( rowID ) determinant = -determinant;
          if( colID ) determinant = -determinant;
      }
      return(determinant); // vulnerable to underflow and overflow
    case 2:
      return(elements_[0][0]*elements_[1][1]-
             elements_[0][1]*elements_[1][0]);
    default: // case 1
      return(elements_[0][0]);
  }
}

template<class Scalar>
Scalar Matrix<Scalar>::norm(ENorm normType) const {
  int dim = getDim();
  switch(normType) {
    case NORM_ONE: { // std::max column sum of the absolute values
      
      // Set result equal to 1-norm of the first column
      Scalar result = this -> getColumn(0).norm(NORM_ONE);
      
      // If dim > 1 compare 1-norm of first column with 1-norm of second column
      if(dim > 1){
        Scalar temp = getColumn(1).norm(NORM_ONE);
        result = (temp > result) ? temp : result;
      }
      
      // Now result holds the larger of the 1-norms of columns 1 and 2. If dim=3 compare
      // this number with the 1-norm of the 3rd column:
      if(dim == 3) {
        Scalar temp = getColumn(2).norm(NORM_ONE);
        result = (temp > result) ? temp : result; 
      }
      return result;
      break;
    }
    case NORM_INF:{// std::max row sum of absolute values: apply above algorithm to rows
      // Set result equal to 1-norm of the first row
      Scalar result = this -> getRow(0).norm(NORM_ONE);
      
      // If dim > 1 compare 1-norm of first row with 1-norm of second row
      if(dim > 1){
        Scalar temp = getRow(1).norm(NORM_ONE);
        result = (temp > result) ? temp : result;
      }
      
      // Now result holds the larger of the 1-norms of rows 1 and 2. If dim=3 compare
      // this number with the 1-norm of the 3rd row:
      if(dim == 3) {
        Scalar temp = getRow(2).norm(NORM_ONE);
        result = (temp > result) ? temp : result; 
      }
      return result;
      break;
    }
    case NORM_FRO: {                    // square root of sum of all element squares 
      Scalar result = 0;
      for(int i = 0; i < dim; i++){
        for(int j = 0; j < dim; j++){
          result += elements_[i][j]*elements_[i][j];
        }
      }
      return std::sqrt(result);
      break;
    }
    case NORM_TWO:
    default: 
      std::cerr << " Matrix norm not implemented "; exit(1);
      return 0;
  }
}

template<class Scalar>
void Matrix<Scalar>::invert() {
  Matrix<Scalar> tempMatrix = this -> getInverse();
  *this = tempMatrix;
}

template<class Scalar>
inline Matrix<Scalar> & Matrix<Scalar>::operator = (const Matrix<Scalar>& right)
{
  assert(this != &right);            // check for self-assignment
  assert(getDim()==right.getDim());  // dimensions must match
  elements_ = right.elements_;
  return *this;
}

template<class Scalar>
inline Matrix<Scalar> & Matrix<Scalar>::operator += (const Matrix<Scalar>& right)
{
  assert(this != &right);  // check for self-assignment
  switch (getDim()) {      
    case 3:
      elements_[0][0] += right.elements_[0][0];
      elements_[0][1] += right.elements_[0][1];
      elements_[0][2] += right.elements_[0][2];
      //
      elements_[1][0] += right.elements_[1][0];
      elements_[1][1] += right.elements_[1][1];
      elements_[1][2] += right.elements_[1][2];
      //
      elements_[2][0] += right.elements_[2][0];
      elements_[2][1] += right.elements_[2][1];
      elements_[2][2] += right.elements_[2][2];      
      return *this;
    case 2:
      elements_[0][0] += right.elements_[0][0];
      elements_[0][1] += right.elements_[0][1];
      //
      elements_[1][0] += right.elements_[1][0];
      elements_[1][1] += right.elements_[1][1];
      return *this;
    default: // case 1
      elements_[0][0] += right.elements_[0][0];
      return *this;
  }
}

template<class Scalar>
inline Matrix<Scalar> & Matrix<Scalar>::operator -= (const Matrix<Scalar>& right)
{
  assert(this != &right);  // check for self-assignment
  switch (getDim()) {      
    case 3:
      elements_[0][0] -= right.elements_[0][0];
      elements_[0][1] -= right.elements_[0][1];
      elements_[0][2] -= right.elements_[0][2];
      //
      elements_[1][0] -= right.elements_[1][0];
      elements_[1][1] -= right.elements_[1][1];
      elements_[1][2] -= right.elements_[1][2];
      //
      elements_[2][0] -= right.elements_[2][0];
      elements_[2][1] -= right.elements_[2][1];
      elements_[2][2] -= right.elements_[2][2];      
      return *this;
    case 2:
      elements_[0][0] -= right.elements_[0][0];
      elements_[0][1] -= right.elements_[0][1];
      //
      elements_[1][0] -= right.elements_[1][0];
      elements_[1][1] -= right.elements_[1][1];
      return *this;
    default: // case 1
      elements_[0][0] -= right.elements_[0][0];
      return *this;
  }
}

template<class Scalar>
inline Matrix<Scalar> & Matrix<Scalar>::operator *= (const Scalar left)
{
  switch (getDim()) {      
    case 3:
      elements_[0][0] *= left;
      elements_[0][1] *= left;
      elements_[0][2] *= left;
      //
      elements_[1][0] *= left;
      elements_[1][1] *= left;
      elements_[1][2] *= left;
      //
      elements_[2][0] *= left;
      elements_[2][1] *= left;
      elements_[2][2] *= left;      
      return *this;
    case 2:
      elements_[0][0] *= left;
      elements_[0][1] *= left;
      //
      elements_[1][0] *= left;
      elements_[1][1] *= left;
      return *this;
    default: // case 1
      elements_[0][0] *= left;
      return *this;
  }
}
//===========================================================================//
//                                                                           //
//  END of member definitions for class Matrix; START friends and related    //
//                                                                           //
//===========================================================================//

template<class Scalar>
const Matrix<Scalar> operator + (const Matrix<Scalar>& left, const Matrix<Scalar>& right){
  Matrix<Scalar> result(left);
  result += right;
  return result;
}

template<class Scalar>
const Matrix<Scalar> operator - (const Matrix<Scalar>& left, const Matrix<Scalar>& right){
  Matrix<Scalar> result(left);
  result -= right;
  return result;
}

template<class Scalar>
const Matrix<Scalar> operator * (const Scalar left, const Matrix<Scalar>& right) {
  Matrix<Scalar> result(right);
  result *= left;
  return result;
}

template<class Scalar>
const Point<Scalar> operator * (const Matrix<Scalar>& mat, const Point<Scalar>& vec) {
  Scalar Product[3];
  int dim = vec.getDim();
  assert(mat.getDim()==dim);
  for (int i = 0; i < dim; ++i) {
    Product[i] = 0.0;
    for (int j = 0; j < dim; ++j) {
      Product[i] += mat.getElement(i,j)*vec.getCoordinates()[j];
    }
  }
  return Point<Scalar>(Product, dim);
}

template<class Scalar>
const Point<Scalar> operator * ( const Point<Scalar>& vec, const Matrix<Scalar>& mat) {
  Scalar Product[3];
  int dim = vec.getDim();
  assert(mat.getDim() == dim);
  for (int i = 0; i < dim; ++i)
  {
    Product[i] = 0.0;
    for (int j = 0; j < dim; ++j)
      Product[i] += mat.getElement(j,i)*vec.getCoordinates()[j];
  }
  return Point<Scalar>(Product, dim);
}


template<class Scalar>
const Matrix<Scalar> operator * (const Matrix<Scalar>& lmat, const Matrix<Scalar>& rmat) {
  assert(lmat.getDim()==rmat.getDim());
  int dim = lmat.getDim();
  Scalar Product[9];
  for (int i = 0; i < dim; ++i)
  {
    for (int j = 0; j < dim; ++j){
      Product[i*dim+j] = (Scalar)0;
      for (int k = 0; k < dim; ++k){
        Product[i*dim+j] += lmat.getElement(i,k)*rmat.getElement(k,j);
      }
    }
  }
  return Matrix<Scalar>(Product, dim);
}

template<class Scalar>
std::ostream& operator << (std::ostream& os, const Matrix<Scalar>& matrix)
{
  short dim = matrix.getDim();
  os  << "\nMatrix info: ambient dimension = " << dim << "D\n";
  switch(dim){
    case 1:
      os << "               Col 0" << std::endl;
      break;
    case 2:
      os << "               Col 0          Col 1" << std::endl;
      break;
    case 3:
      os << "               Col 0          Col 1          Col 2" << std::endl;
      break;
    default:
      os << "Matrix error: invalid ambient dimension\n";
      exit(1);
  }
  os 	<< std::setprecision(6) << std::setiosflags(std::ios::scientific);
  for(int i = 0; i < dim; ++i){
    os << " Row "<< i<< " ";
    for(int j = 0; j < dim; ++j) {
      os << std::setiosflags(std::ios::right) << std::setw(16) << matrix.getElement(i,j);
    }
    os << std::endl;
  }
  os << std::resetiosflags(std::ios::floatfield);
  return os;
}

// End member, friend, and related function definitions of class Matrix.

} // namespace Intrepid
