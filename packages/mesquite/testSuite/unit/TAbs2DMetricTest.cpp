/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2008 Sandia National Laboratories.  Developed at the
    University of Wisconsin--Madison under SNL contract number
    624796.  The U.S. Government and the University of Wisconsin
    retain certain rights to this software.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License 
    (lgpl.txt) along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    (2010) kraftche@cae.wisc.edu    

  ***************************************************************** */


/** \file TAbs2DMetricTest.cpp
 *  \brief Unit tests TAbs2DMetric base class functionality
 *  \author Jason Kraftcheck 
 */

#include "Mesquite.hpp"
#include "TAbs2DMetric.hpp"
#include "UnitUtil.hpp"
#include "MsqError.hpp"

using namespace Mesquite;

// Test functions implemented in class TAbs2DMetric
class TAbs2DMetricTest : public CppUnit::TestFixture
{
  CPPUNIT_TEST_SUITE( TAbs2DMetricTest );
  CPPUNIT_TEST (test_numerical_gradient);
  CPPUNIT_TEST (test_numerical_hessian);
  CPPUNIT_TEST_SUITE_END(); 
  public:
  void test_numerical_gradient();
  void test_numerical_hessian();
};

CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TAbs2DMetricTest, "Unit" );
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION( TAbs2DMetricTest, "TAbs2DMetricTest" );

// implement metric such that the expected derivatives
// at each location r,c in dm/dA are 2r+c+1
class GradTestMetricAbs2D : public TAbs2DMetric
{
  public:
    std::string get_name() const { return "GradTest"; }
  
    static double grad(int r, int c)
      { return 2*r + c + 1; }
  
    bool evaluate( const MsqMatrix<2,2>& A,
                   const MsqMatrix<2,2>& W,
                   double& result,
                   MsqError&  )
    {
      result = 0;
      for (int r = 0; r < 2; ++r) 
        for (int c = 0; c < 2; ++c)
          result += grad(r,c) * (A(r,c) - W(r,c));
      return true;
    }
};

// implement metric that is the |2A - A^T - W|^2
// such that the Hessian is the constant:
//  _          _
// | 2  0  0  0 |
// | 0 10 -8  0 |
// | 0 -8 10  0 |
// |_0  0  0  2_|
class HessTestMetricAbs2D : public TAbs2DMetric
{
  public:
    std::string get_name() const { return "HessTest"; }

    bool evaluate( const MsqMatrix<2,2>& A,
                   const MsqMatrix<2,2>& W,
                   double& result,
                   MsqError&  )
    {
      result = sqr_Frobenius(2*A - transpose(A) - W);
      return true;
    }
    bool evaluate_with_grad( const MsqMatrix<2,2>& A,
                             const MsqMatrix<2,2>& W,
                             double& result,
                             MsqMatrix<2,2>& wrt_A,
                             MsqError&  )
    {
      result = sqr_Frobenius(2*A - transpose(A) - W);
      wrt_A = 10*A - 8*transpose(A) - 4*W + 2*transpose(W);
      return true;
    }
};

/** Simple target metric for testing second partial derivatives.  
 *  \f$\mu(T) = |T|\f$
 *  \f$\frac{\partial\mu}{\partial T} = \frac{1}{|T|} T \f$
 *  \f$\frac{\partial^{2}\mu}{\partial t_{i,i}^2} = \frac{1}{|T|} - \frac{t_{i,i}^2}{|T|^3}\f$
 *  \f$\frac{\partial^{2}\mu}{\partial t_{i,j} \partial t_{k,l} (i \ne k or j \ne l)} = -\frac{t_{i,j} a_{k,l}}{|T|^3}\f$
 */
/** Simple target metric for testing second partial derivatives.  
 *  \f$\mu(A,W) = |A|\f$
 *  \f$\frac{\partial\mu}{\partial A} = \frac{1}{|A|} A \f$
 *  \f$\frac{\partial^{2}\mu}{\partial a_{i,i}^2} = \frac{1}{|A|} - \frac{a_{i,i}^2}{|A|^3}\f$
 *  \f$\frac{\partial^{2}\mu}{\partial a_{i,j} \partial a_{k,l} (i \ne k or j \ne l)} = -\frac{a_{i,j} a_{k,l}}{|A|^3}\f$
 */
class HessTestMetricAbs2D_2 : public TAbs2DMetric
{
  public:
    std::string get_name() const { return "HessTest2"; }
  
    bool evaluate( const MsqMatrix<2,2>& A, const MsqMatrix<2,2>&, double& result, MsqError& err )
      { result = Frobenius(A); return true; }
    
    bool evaluate_with_grad( const MsqMatrix<2,2>& A, 
                             const MsqMatrix<2,2>&,
                             double& result,
                             MsqMatrix<2,2>& d,
                             MsqError& err )
    {
      result = Frobenius(A);
      d = A / result;
      return true;
    }
    
    bool evaluate_with_hess( const MsqMatrix<2,2>& A, 
                             const MsqMatrix<2,2>&,
                             double& result,
                             MsqMatrix<2,2>& d,
                             MsqMatrix<2,2> d2[3],
                             MsqError& err )
    {
      result = Frobenius(A);
      d = A / result;
      int h = 0;
      for (int r = 0; r < 2; ++r) {
        int i = h;
        for (int c = r; c < 2; ++c)
          d2[h++] = transpose(A.row(r)) * A.row(c) / -(result*result*result);
        d2[i] += MsqMatrix<2,2>(1.0/result);
      }    
      return true;
    }
};

void TAbs2DMetricTest::test_numerical_gradient()
{
  GradTestMetricAbs2D metric;
  HessTestMetricAbs2D_2 metric2;
  const double Avals[] = { 1, 2, 2, 5 };
  const double Bvals[] = { -0.1, -0.15, -0.25, -0.8 };
  const MsqMatrix<2,2> I( 1.0 );
  const MsqMatrix<2,2> A( Avals );
  const MsqMatrix<2,2> B( Bvals );
  
  MsqError err;
  MsqMatrix<2,2> d;
  bool valid;
  double val, gval;
  
  MsqMatrix<2,2> expected;
  for (int r = 0; r < 2; ++r)
    for (int c = 0; c < 2; ++c)
      expected(r,c) = metric.grad(r,c);
  
  valid = metric.evaluate( I, A, val, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_grad( I, A, gval, d, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, gval, 1e-6 );
  ASSERT_MATRICES_EQUAL( expected, d, 1e-6 );
  
  valid = metric.evaluate( A, I, val, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_grad( A, I, gval, d, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, gval, 1e-6 );
  ASSERT_MATRICES_EQUAL( expected, d, 1e-6 );
  
  valid = metric.evaluate( I, B, val, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_grad( I, B, gval, d, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, gval, 1e-6 );
  ASSERT_MATRICES_EQUAL( expected, d, 1e-6 );
  
  valid = metric.evaluate( B, I, val, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_grad( B, I, gval, d, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, gval, 1e-6 );
  ASSERT_MATRICES_EQUAL( expected, d, 1e-6 );
   
  valid = metric.evaluate( A, B, val, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_grad( A, B, gval, d, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, gval, 1e-6 );
  ASSERT_MATRICES_EQUAL( expected, d, 1e-6 );
  
  valid = metric.evaluate( B, A, val, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_grad( B, A, gval, d, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  ASSERT_MATRICES_EQUAL( expected, d, 1e-6 );
  
  MsqMatrix<2,2> da;
  valid = metric2.evaluate_with_grad( A, I, val, da, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric2.TAbs2DMetric::evaluate_with_grad( A, I, gval, d, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, gval, 1e-6 );
  ASSERT_MATRICES_EQUAL( da, d, 1e-6 );
  
  valid = metric2.evaluate_with_grad( B, I, val, da, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric2.TAbs2DMetric::evaluate_with_grad( B, I, gval, d, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, gval, 1e-6 );
  ASSERT_MATRICES_EQUAL( da, d, 1e-6 );
}


void TAbs2DMetricTest::test_numerical_hessian()
{
  HessTestMetricAbs2D metric;
  HessTestMetricAbs2D_2 metric2;
  const double Avals[] = { 1, 2, 2, 5 };
  const double Bvals[] = { -0.1, -0.15, -0.25, -0.8 };
  const MsqMatrix<2,2> I( 1.0 );
  const MsqMatrix<2,2> A( Avals );
  const MsqMatrix<2,2> B( Bvals );
  
  MsqError err;
  MsqMatrix<2,2> g, gh;
  MsqMatrix<2,2> h[6];
  bool valid;
  double val, hval;
  
  const double h_00[] = { 2, 0, 0, 10 };
  const double h_01[] = { 0, 0, -8, 0 };
  const double h_11[] = { 10, 0, 0, 2 };
  MsqMatrix<2,2> h00(h_00), h01(h_01), h11(h_11);
  
  valid = metric.evaluate_with_grad( I, A, val, g, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_hess( I, A, hval, gh, h, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, hval, 1e-6 );
  ASSERT_MATRICES_EQUAL( g, gh, 1e-6 );
  ASSERT_MATRICES_EQUAL( h00, h[0], 1e-6 );
  ASSERT_MATRICES_EQUAL( h01, h[1], 1e-6 );
  ASSERT_MATRICES_EQUAL( h11, h[2], 1e-6 );
  
  valid = metric.evaluate_with_grad( A, I, val, g, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_hess( A, I, hval, gh, h, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, hval, 1e-6 );
  ASSERT_MATRICES_EQUAL( g, gh, 1e-6 );
  ASSERT_MATRICES_EQUAL( h00, h[0], 1e-6 );
  ASSERT_MATRICES_EQUAL( h01, h[1], 1e-6 );
  ASSERT_MATRICES_EQUAL( h11, h[2], 1e-6 );
  
  valid = metric.evaluate_with_grad( I, B, val, g, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_hess( I, B, hval, gh, h, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, hval, 1e-6 );
  ASSERT_MATRICES_EQUAL( g, gh, 1e-6 );
  ASSERT_MATRICES_EQUAL( h00, h[0], 1e-6 );
  ASSERT_MATRICES_EQUAL( h01, h[1], 1e-6 );
  ASSERT_MATRICES_EQUAL( h11, h[2], 1e-6 );
  
  valid = metric.evaluate_with_grad( B, I, val, g, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_hess( B, I, hval, gh, h, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, hval, 1e-6 );
  ASSERT_MATRICES_EQUAL( g, gh, 1e-6 );
  ASSERT_MATRICES_EQUAL( h00, h[0], 1e-6 );
  ASSERT_MATRICES_EQUAL( h01, h[1], 1e-6 );
  ASSERT_MATRICES_EQUAL( h11, h[2], 1e-6 );
  
  valid = metric.evaluate_with_grad( A, B, val, g, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_hess( A, B, hval, gh, h, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, hval, 1e-6 );
  ASSERT_MATRICES_EQUAL( g, gh, 1e-6 );
  ASSERT_MATRICES_EQUAL( h00, h[0], 1e-6 );
  ASSERT_MATRICES_EQUAL( h01, h[1], 1e-6 );
  ASSERT_MATRICES_EQUAL( h11, h[2], 1e-6 );
  
  valid = metric.evaluate_with_grad( B, A, val, g, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric.evaluate_with_hess( B, A, hval, gh, h, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, hval, 1e-6 );
  ASSERT_MATRICES_EQUAL( g, gh, 1e-6 );
  ASSERT_MATRICES_EQUAL( h00, h[0], 1e-6 );
  ASSERT_MATRICES_EQUAL( h01, h[1], 1e-6 );
  ASSERT_MATRICES_EQUAL( h11, h[2], 1e-6 );
  
  MsqMatrix<2,2> ah[6];
  valid = metric2.evaluate_with_hess( A, I, val, g, ah, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric2.TAbs2DMetric::evaluate_with_hess( A, I, hval, gh, h, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, hval, 1e-6 );
  ASSERT_MATRICES_EQUAL( g, gh, 1e-6 );
  ASSERT_MATRICES_EQUAL( ah[0], h[0], 1e-6 );
  ASSERT_MATRICES_EQUAL( ah[1], h[1], 1e-6 );
  ASSERT_MATRICES_EQUAL( ah[2], h[2], 1e-6 );

  valid = metric2.evaluate_with_hess( B, I, val, g, ah, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  valid = metric2.TAbs2DMetric::evaluate_with_hess( B, I, hval, gh, h, err );
  ASSERT_NO_ERROR(err);
  CPPUNIT_ASSERT(valid);
  CPPUNIT_ASSERT_DOUBLES_EQUAL( val, hval, 1e-6 );
  ASSERT_MATRICES_EQUAL( g, gh, 1e-6 );
  ASSERT_MATRICES_EQUAL( ah[0], h[0], 1e-6 );
  ASSERT_MATRICES_EQUAL( ah[1], h[1], 1e-6 );
  ASSERT_MATRICES_EQUAL( ah[2], h[2], 1e-6 );
}
