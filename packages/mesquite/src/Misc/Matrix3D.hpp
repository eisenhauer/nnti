// -*- Mode : c++; tab-width: 3; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 3 -*-
//
//    AUTHOR: Thomas Leurent <tleurent@mcs.anl.gov>
//       ORG: Argonne National Laboratory
//    E-MAIL: tleurent@mcs.anl.gov
//
// ORIG-DATE: 18-Dec-02 at 11:08:22
//  LAST-MOD: 10-Oct-03 at 13:33:34 by Thomas Leurent
//
// DESCRIPTION:
// ============
/*! \file Matrix3D.hpp

3*3 Matric class, row-oriented, 0-based [i][j] indexing.

 \author Thomas Leurent
 
*/
// DESCRIP-END.
//



#ifndef Matrix3D_hpp
#define Matrix3D_hpp

#ifdef USE_STD_INCLUDES
#include <iostream>
#include <sstream>
#else
#include <iostream.h>
#include <strstream.h>
#endif

#ifdef USE_C_PREFIX_INCLUDES
#include <cassert>
#include <cstdlib>
#else
#include <assert.h>
#include <stdlib.h>
#endif

#include "Mesquite.hpp"
#include "Vector3D.hpp"

MSQ_USE(ostream);
MSQ_USE(istream);

namespace Mesquite
{

  /*! \class Matrix3D
      \brief 3*3 Matric class, row-oriented, 0-based [i][j] indexing.

      Since the size of the object is fixed at compile time, the Matrix3D
      object is as fast as a double[9] array.
  */
  class Matrix3D 
  {
  protected:
    double v_[9];                  
    double* row_[3];

    // internal helper function to create the array
    // of row pointers

    void initialize()
    {
      row_[0] = v_;
      row_[1] = v_+3;
      row_[2] = v_+6;
    }
   
    void copy(const double*  v)
    { memcpy(v_, v, 9*sizeof(double)); }

    void set(const double& val)
    {
      v_[0]=val;  v_[1]=val;  v_[2]=val;
      v_[3]=val;  v_[4]=val;  v_[5]=val;
      v_[6]=val;  v_[7]=val;  v_[8]=val;
    }

    void set_values(const char *s)
    {
#ifdef USE_STD_INCLUDES
      std::istringstream ins(s);
#else
      istrstream ins(s);
#endif
      ins>>v_[0];  ins>>v_[1];  ins>>v_[2]; 
      ins>>v_[3];  ins>>v_[4];  ins>>v_[5]; 
      ins>>v_[6];  ins>>v_[7];  ins>>v_[8]; 
    }
    
  public:

    operator double**(){ return  row_; }

    size_t size() const { return 9; }

    // constructors
    //! Default constructor sets all entries to 0. 
    Matrix3D()
    {
      initialize();
      zero();
    }
    
    Matrix3D(const Matrix3D &A)
    {
      initialize();
      copy(A.v_);
    }

    //! sets all entries of the matrix to value.
    Matrix3D(const double& value)
    {
      initialize();
      set(value);
    }

    //! sets matrix entries to values in array.
    //! \param v is an array of 9 doubles. 
    Matrix3D(const double* v)
    {
      initialize();
      copy(v);
    }

    //! for test purposes, matrices can be instantiated as
    //! \code Matrix3D A("3 2 1  4 5 6  9 8 7"); \endcode
    Matrix3D(const char *s)
    {
      initialize();
      set_values(s);
    }

    // destructor
    ~Matrix3D() { }

    // assignments
    Matrix3D& operator=(const Matrix3D &A)
    {
      if (v_ == A.v_)
        return *this;
      copy(A.v_);
      return *this;
    }
        
    Matrix3D& operator=(const double& scalar)
    { 
      set(scalar); 
      return *this;
    }

    //! for test purposes, matrices can be assigned as follows
    //! \code A = "3 2 1  4 5 6  9 8 7"; \endcode
    Matrix3D& operator=(const char* s)
    { 
      set_values(s); 
      return *this;
    }

    //! Sets all entries to zero (more efficient than assignement).
    void zero()
    {
      v_[0]=0.;  v_[1]=0.;  v_[2]=0.;
      v_[3]=0.;  v_[4]=0.;  v_[5]=0.;
      v_[6]=0.;  v_[7]=0.;  v_[8]=0.;
//      memset(v_, 0, 9*sizeof(double));
    }
     
    //! Sets column j (0, 1 or 2) to Vector3D c.
    void set_column(int j, const Vector3D c)
    {
      v_[0+j]=c[0];
      v_[3+j]=c[1];
      v_[6+j]=c[2];
    }

    // Matrix Operators
    friend bool operator==(const Matrix3D &lhs, const Matrix3D &rhs);
    friend bool operator!=(const Matrix3D &lhs, const Matrix3D &rhs);
    friend const Matrix3D operator+(const Matrix3D &A, const Matrix3D &B);
    friend const Matrix3D operator-(const Matrix3D &A, const Matrix3D &B) ;
    friend const Matrix3D operator*(const Matrix3D &A, const Matrix3D &B);
    friend const Matrix3D mult_element(const Matrix3D &A, const Matrix3D &B);
    friend Matrix3D transpose(const Matrix3D &A);
    friend int matmult(Matrix3D& C, const Matrix3D  &A, const Matrix3D &B);
    friend const Vector3D operator*(const Matrix3D  &A, const Vector3D &x);
    friend const Vector3D operator*(const Vector3D &x, const Matrix3D  &A);
    const Matrix3D operator*(const double &s) const;
    friend const Matrix3D operator*(const double &s, const Matrix3D &A);    
    void operator+=(const Matrix3D &rhs);
    void operator-=(const Matrix3D &rhs);
    void operator*=(const double &s);
    Matrix3D plus_transpose(const Matrix3D &B) const;
    void plus_transpose_equal(const Matrix3D &B);
    Matrix3D& outer_product(const Vector3D &v1, const Vector3D &v2);
    void fill_lower_triangle();

    //! \f$ v = A*x \f$
    friend void eqAx(Vector3D& v, const Matrix3D& A, const Vector3D& x);
    //! \f$ v += A*x \f$
    friend void plusEqAx(Vector3D& v, const Matrix3D& A, const Vector3D& x);
    //! \f$ v += A^T*x \f$
    friend void plusEqTransAx(Vector3D& v, const Matrix3D& A, const Vector3D& x);
     
    //! \f$ B += a*A \f$
    friend void plusEqaA(Matrix3D& B, const double a, const Matrix3D &A);

    size_t num_rows() const { return 3; }
    size_t num_cols() const { return 3; }

    //! returns a pointer to a row.
    inline double* operator[](int i)
    {
//      assert(0<=i); assert(i < 3);
      return row_[i];
    }

    //! returns a pointer to a row.
    inline const double* operator[](int i) const
    {
//      assert(0<=i); assert(i < 3);
      return row_[i];
    }

  };


  /* ***********  I/O  **************/

  inline ostream& operator<<(ostream &s, const Matrix3D &A)
  {
    for (size_t i=0; i<3; ++i)
      {
        for (size_t j=0; j<3; ++j)
          s << A[i][j] << " ";
        s << "\n";
      }
    return s;
  }

  inline istream& operator>>(istream &s, Matrix3D &A)
  {
    for (size_t i=0; i<3; i++)
      for (size_t j=0; j<3; j++)
        {
          s >>  A[i][j];
        }
    return s;
  }

  // *********** matrix operators *******************

  // comparison functions
  inline bool operator==(const Matrix3D &lhs, const Matrix3D &rhs)
  {
    return (memcmp(lhs.v_, rhs.v_, 9*sizeof(double)) == 0);
  }
  inline bool operator!=(const Matrix3D &lhs, const Matrix3D &rhs)
  {
    return (memcmp(lhs.v_, rhs.v_, 9*sizeof(double)) != 0);
  }

  //! \return A+B
  inline const Matrix3D operator+(const Matrix3D &A, 
                            const Matrix3D &B)
  {
    Matrix3D tmp;
    size_t i;
    for (i=0; i<3; ++i) {
      tmp[i][0] = A[i][0] + B[i][0];
      tmp[i][1] = A[i][1] + B[i][1];
      tmp[i][2] = A[i][2] + B[i][2];
    }
    return tmp;
  }

  //! \return A-B
  inline const Matrix3D operator-(const Matrix3D &A, 
                            const Matrix3D &B)
  {
    Matrix3D tmp;
    size_t i;
    for (i=0; i<3; ++i) {
      tmp[i][0] = A[i][0] - B[i][0];
      tmp[i][1] = A[i][1] - B[i][1];
      tmp[i][2] = A[i][2] - B[i][2];
    }
    return tmp;
  }

    //! Multiplies entry by entry. This is NOT a matrix multiplication. 
  inline const Matrix3D mult_element(const Matrix3D &A, 
                               const Matrix3D &B)
  {
    Matrix3D tmp;
    size_t i;
    for (i=0; i<3; ++i) {
      tmp[i][0] = A[i][0] * B[i][0];
      tmp[i][1] = A[i][1] * B[i][1];
      tmp[i][2] = A[i][2] * B[i][2];
    }
    return tmp;
  }

  inline Matrix3D transpose(const Matrix3D &A)
  {
    Matrix3D S;
    size_t i;
    for (i=0; i<3; ++i) {
        S[size_t(0)][i] = A[i][0];
        S[size_t(1)][i] = A[i][1];
        S[size_t(2)][i] = A[i][2];
    }
    return S;
  }

  inline void Matrix3D::operator+=(const Matrix3D &rhs)
  {
      v_[0] += rhs.v_[0]; v_[1] += rhs.v_[1]; v_[2] += rhs.v_[2];
      v_[3] += rhs.v_[3]; v_[4] += rhs.v_[4]; v_[5] += rhs.v_[5];
      v_[6] += rhs.v_[6]; v_[7] += rhs.v_[7]; v_[8] += rhs.v_[8];
  }

  inline void Matrix3D::operator-=(const Matrix3D &rhs)
  {
      v_[0] -= rhs.v_[0]; v_[1] -= rhs.v_[1]; v_[2] -= rhs.v_[2];
      v_[3] -= rhs.v_[3]; v_[4] -= rhs.v_[4]; v_[5] -= rhs.v_[5];
      v_[6] -= rhs.v_[6]; v_[7] -= rhs.v_[7]; v_[8] -= rhs.v_[8];
  }

  //! multiplies each entry by the scalar s
  inline void Matrix3D::operator*=(const double &s)
  {
      v_[0] *= s; v_[1] *= s; v_[2] *= s;
      v_[3] *= s; v_[4] *= s; v_[5] *= s;
      v_[6] *= s; v_[7] *= s; v_[8] *= s;
  }

  //! \f$ + B^T  \f$
  inline Matrix3D Matrix3D::plus_transpose(const Matrix3D &B) const
  {
    Matrix3D tmp;

    tmp.v_[0] = v_[0] + B.v_[0];
    tmp.v_[1] = v_[1] + B.v_[3];
    tmp.v_[2] = v_[2] + B.v_[6];
    
    tmp.v_[3] = v_[3] + B.v_[1];
    tmp.v_[4] = v_[4] + B.v_[4];
    tmp.v_[5] = v_[5] + B.v_[7];
    
    tmp.v_[6] = v_[6] + B.v_[2];
    tmp.v_[7] = v_[7] + B.v_[5];
    tmp.v_[8] = v_[8] + B.v_[8];

    return tmp;
  }

  //! \f$ += B^T  \f$
  inline void Matrix3D::plus_transpose_equal(const Matrix3D &B)
  {
    v_[0] += B.v_[0];
    v_[1] += B.v_[3];
    v_[2] += B.v_[6];
    
    v_[3] += B.v_[1];
    v_[4] += B.v_[4];
    v_[5] += B.v_[7];
    
    v_[6] += B.v_[2];
    v_[7] += B.v_[5];
    v_[8] += B.v_[8];
  }

  //! Computes \f$ A = v_1 v_2^T \f$
  inline Matrix3D& Matrix3D::outer_product(const Vector3D  &v1, const Vector3D &v2)
  {
    // remember, matrix entries are v_[0] to v_[8].
    
    // diagonal
    v_[0] = v1[0]*v2[0];
    v_[4] = v1[1]*v2[1];
    v_[8] = v1[2]*v2[2];

    // upper triangular part
    v_[1] = v1[0]*v2[1];
    v_[2] = v1[0]*v2[2];
    v_[5] = v1[1]*v2[2];

    // lower triangular part
    v_[3] = v2[0]*v1[1];
    v_[6] = v2[0]*v1[2];
    v_[7] = v2[1]*v1[2];

    return *this;
  }

  inline void Matrix3D::fill_lower_triangle()
  {
    v_[3] = v_[1];
    v_[6] = v_[2];
    v_[7] = v_[5];
  } 

  //! \return A*B
  inline const Matrix3D operator*(const Matrix3D  &A, 
                            const Matrix3D &B)
  {
    Matrix3D tmp;
    double sum;
    for (size_t i=0; i<3; ++i)
      for (size_t k=0; k<3; ++k)
        {
          sum = 0;
          for (size_t j=0; j<3; j++)
            sum = sum +  A[i][j] * B[j][k];
          tmp[i][k] = sum; 
        }
    return tmp;
  }
   
   //! multiplies each entry by the scalar s
  inline const Matrix3D Matrix3D::operator*(const double &s) const
  {
    Matrix3D temp;
    temp[0][0]=v_[0] * s; temp[0][1]=v_[1] * s; temp[0][2]=v_[2] * s;
    temp[1][0]=v_[3] * s; temp[1][1]=v_[4] * s; temp[1][2]=v_[5] * s;
    temp[2][0]=v_[6] * s; temp[2][1]=v_[7] * s; temp[2][2]=v_[8] * s;
    return temp;
  }
     //!friend function to allow for commutatative property of
     //! scalar mulitplication.
   inline const Matrix3D operator*(const double &s, const Matrix3D &A)
   {
     return (A.operator*(s));
   }
   
   
  //! \f$ C = A \times B \f$
  inline int matmult(Matrix3D& C, const Matrix3D  &A, 
                     const Matrix3D &B)
  {
    double sum;
    const double* row_i;
    const double* col_k;
    for (size_t i=0; i<3; ++i)
      for (size_t k=0; k<3; ++k)
        {
          row_i  = &(A[i][0]);
          col_k  = &(B[0][k]);
          sum = 0;
          for (size_t j=0; j<3; ++j)
            {
              sum  += *row_i * *col_k;
              row_i++;
              col_k += 3;
            }
          C[i][k] = sum; 
        }
    return 0;
  }

  /*! \brief Computes \f$ A v \f$ . */
  inline const Vector3D operator*(const Matrix3D  &A, const Vector3D &x)
  {
    Vector3D tmp; // initializes to 0
    for (size_t i=0; i<3; ++i)
      {
        const double* rowi = A[i];
        tmp[i] = rowi[0]*x[0] + rowi[1]*x[1] + rowi[2]*x[2];
      }
    return tmp;
  }

  /*! \brief Computes \f$ v^T A \f$ .
    
      This function implicitly considers the transpose of vector x times
      the matrix A and it is implicit that the returned vector must be
      transposed. */
  inline const Vector3D operator*(const Vector3D &x, const Matrix3D  &A)
  {
    Vector3D res(0., 0., 0.);
    for (size_t i=0; i<3; ++i)
      {
        const double* rowi = A[i];
        for (size_t j=0; j<3; ++j)
          res[j] += rowi[j] * x[i];
      }
    return res;
  }
   
  inline void eqAx(Vector3D& v, const Matrix3D& A, const Vector3D& x)
  {
     v.mCoords[0] = A.v_[0]*x[0] + A.v_[1]*x.mCoords[1] + A.v_[2]*x.mCoords[2];
     v.mCoords[1] = A.v_[3]*x[0] + A.v_[4]*x.mCoords[1] + A.v_[5]*x.mCoords[2];
     v.mCoords[2] = A.v_[6]*x[0] + A.v_[7]*x.mCoords[1] + A.v_[8]*x.mCoords[2];
  }
   
  inline void plusEqAx(Vector3D& v, const Matrix3D& A, const Vector3D& x)
  {
     v.mCoords[0] += A.v_[0]*x[0] + A.v_[1]*x.mCoords[1] + A.v_[2]*x.mCoords[2];
     v.mCoords[1] += A.v_[3]*x[0] + A.v_[4]*x.mCoords[1] + A.v_[5]*x.mCoords[2];
     v.mCoords[2] += A.v_[6]*x[0] + A.v_[7]*x.mCoords[1] + A.v_[8]*x.mCoords[2];
  }
   
  inline void plusEqTransAx(Vector3D& v, const Matrix3D& A, const Vector3D& x)
  {
     v.mCoords[0] += A.v_[0]*x.mCoords[0] + A.v_[3]*x.mCoords[1] + A.v_[6]*x.mCoords[2];
     v.mCoords[1] += A.v_[1]*x.mCoords[0] + A.v_[4]*x.mCoords[1] + A.v_[7]*x.mCoords[2];
     v.mCoords[2] += A.v_[2]*x.mCoords[0] + A.v_[5]*x.mCoords[1] + A.v_[8]*x.mCoords[2];
  }
   
  inline void plusEqaA(Matrix3D& B, const double a, const Matrix3D &A) {
    B.v_[0] += a*A.v_[0]; B.v_[1] += a*A.v_[1]; B.v_[2] += a*A.v_[2]; 
    B.v_[3] += a*A.v_[3]; B.v_[4] += a*A.v_[4]; B.v_[5] += a*A.v_[5];
    B.v_[6] += a*A.v_[6]; B.v_[7] += a*A.v_[7]; B.v_[8] += a*A.v_[8];
  }

} // namespace Mesquite

#endif // Matrix3D_hpp
