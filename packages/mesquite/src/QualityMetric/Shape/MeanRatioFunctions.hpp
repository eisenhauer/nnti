// -*- Mode : c++; tab-width: 3; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 3 -*-

/*! \file MeanFunction.hpp

Header that defines generalized Mean Ratio function, gradient, and hessian
evaluations.

\author Todd Munson
\date   2003-06-11
 */

#ifndef MeanRatioFunctions_hpp
#define MeanRatioFunctions_hpp

#include <math.h>
#include "Mesquite.hpp"
#include "Vector3D.hpp"
#include "Matrix3D.hpp"

namespace Mesquite 
{
/*****************************************************************************/
/* Optimal derivative calculations courtesy of Paul Hovland (at least we     */
/* think it is optimal).  The original code provided was modified to         */
/* reduce the number of flops and intermediate variables, and improve the    */
/* locality of reference.                                                    */
/*****************************************************************************/
/* The Hessian calculation is done by blocks.  Only the upper triangular     */
/* blocks are stored.  The results in the data is in the following order:    */
/*    [d1 b1 b2 d2 b3 d3 ]                                                   */
/* The matrices on the diagonal (d1-d3) each contain 10 elements, while the  */
/* off-diagonal elements (b1-b3) each contain 16 elements.                   */
/*****************************************************************************/
/* NOTE: THIS IS NO LONGER ENTIRELY CORRECT!  VARIABLE POWERS MESSES THE     */
/* DESCRIPTION UP!                                                           */
/*                                                                           */
/* The form of the function, gradient, and Hessian is the following:         */
/*   o(x) = a * f(A(x)) * pow(g(A(x)), b)                                    */
/* where A(x) is the matrix generated from:                                  */
/*           [x1-x0 x2-x0 x3-x0]                                             */
/*    A(x) = [y1-y0 y2-y0 y3-y0] * inv(W)                                    */
/*           [z1-z0 z2-z0 z3-z0]                                             */
/* and f() is the squared Frobenius norm of A(x), and g() is the determinant */
/* of A(x).                                                                  */
/*                                                                           */
/* The gradient is calculated as follows:                                    */
/*   alpha := a*pow(g(A(x)),b)                                               */
/*   beta  := a*b*f(A(x))*pow(g(A(x)),b-1)                                   */
/*                                                                           */
/*                                                                           */
/*   do/dx = (alpha * (df/dA) + beta * (dg/dA)) (dA/dx)                      */
/*                                                                           */
/*   Note: this is the optimal ordering for the gradient vector.             */
/*   Distributing (dA/dx) would result in two matrix vector products as      */
/*   opposed to the single matrix vector product in the above formulation.   */
/*                                                                           */
/*   (df/dA)_i = 2*A_i                                                       */
/*   (dg/dA)_i = A_j*A_k - A_l*A_m for some {j,k,l,m}                        */
/*                                                                           */
/*   d^2o/dx^2 = (dA/dx)' * ((d alpha/dA) * (df/dA) +                        */
/*                           (d  beta/dA) * (dg/dA)                          */
/*                                  alpha * (d^2f/dA^2)                      */
/*                                   beta * (d^2g/dA^2)) * (dA/dx)           */
/*                                                                           */
/*   Note: since A(x) is a linear function, there are no terms involving     */
/*   d^2A/dx^2 since this matrix is zero.                                    */
/*                                                                           */
/*   gamma := a*b*pow(g(A(x)),b-1)                                           */
/*   delta := a*b*(b-1)*f(A(x))*pow(g(A(x)),b-2)                             */
/*                                                                           */
/*   d^2o/dx^2 = (dA/dx)' * (gamma*((dg/dA)'*(df/dA) + (df/dA)'*(dg/dA)) +   */
/*                           delta* (dg/dA)'*(dg/dA) +                       */
/*                           alpha*(d^2f/dA^2) +                             */
/*                            beta*(d^2g/dA^2)) * (dA/dx)                    */
/*                                                                           */
/*   Note: (df/dA) and (dg/dA) are row vectors and we only calculate the     */
/*   upper triangular part of the inner matrix.                              */
/*                                                                           */
/*   For regular tetrahedral elements, we have the following:                */
/*                                                                           */
/*           [-1         1        0         0         ]                      */
/*       M = [-sqrt(3)  -sqrt(3)  2*sqrt(3) 0         ]                      */
/*           [-sqrt(6)  -sqrt(6)  -sqrt(6)  3*sqrt(6) ]                      */
/*                                                                           */
/*           [M 0 0]                                                         */
/*   dA/dx = [0 M 0]                                                         */
/*           [0 0 M]                                                         */
/*                                                                           */
/*   I belive the above is close to optimal for the calculation of the       */
/*   Hessian.  Distributing the (dA/dx) results in larger vector which are   */
/*   detrimental when forming the outer product.  The way the method is      */
/*   written, we only calculate a 9x9 symmetric matrix in the outer product. */
/*                                                                           */
/*   In two dimensions, the inner matrix computed has a nice structure and   */
/*   we can eliminate some of the computation in the inner product.  This    */
/*   does not appear to be the case in more than two dimensions.             */
/*****************************************************************************/

/*****************************************************************************/
/* Not all compilers substitute out constants (especially the square root).  */
/* Therefore, they are substituted out manually.  The values below were      */
/* calculated on a solaris machine using long doubles. I believe they are    */
/* accurate.                                                                 */
/*****************************************************************************/

#define isqrt3   5.77350269189625797959429519858e-01        /*  1.0/sqrt(3.0)*/
#define tisqrt3  1.15470053837925159591885903972e+00        /*  2.0/sqrt(3.0)*/
#define isqrt6   4.08248290463863052509822647505e-01        /*  1.0/sqrt(6.0)*/
#define tisqrt6  1.22474487139158915752946794252e+00        /*  3.0/sqrt(6.0)*/

/*****************************************************************************/
/* The following set of functions reference triangular elements to an        */
/* equilateral triangle in the plane defined by the normal.  They are        */
/* used when assessing the quality of a triangular element.  A zero          */
/* return value indicates success, while a nonzero value indicates failure.  */
/*****************************************************************************/

/*****************************************************************************/
/* Function evaluation requires 44 flops.                                    */
/*   Reductions possible when b == 1 or c == 1                               */
/*****************************************************************************/
inline bool m_fcn_2e(double &obj, const Vector3D x[3], const Vector3D &n,
		     const double a, const double b, const double c)
{
  double matr[9], f;
  double g;

  /* Calculate M = [A*inv(W) n] */
  matr[0] = x[1][0] - x[0][0];
  matr[1] = (2.0*x[2][0] - x[1][0] - x[0][0])*isqrt3;
  matr[2] = n[0];

  matr[3] = x[1][1] - x[0][1];
  matr[4] = (2.0*x[2][1] - x[1][1] - x[0][1])*isqrt3;
  matr[5] = n[1];

  matr[6] = x[1][2] - x[0][2];
  matr[7] = (2.0*x[2][2] - x[1][2] - x[0][2])*isqrt3;
  matr[8] = n[2];

  /* Calculate det(M). */
  g = matr[0]*(matr[4]*matr[8] - matr[5]*matr[7]) +
      matr[3]*(matr[2]*matr[7] - matr[1]*matr[8]) +
      matr[6]*(matr[1]*matr[5] - matr[2]*matr[4]);
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] +
      matr[3]*matr[3] + matr[4]*matr[4] +
      matr[6]*matr[6] + matr[7]*matr[7];

  /* Calculate objective function. */
  obj = a * pow(f, b) * pow(g, c);
  return true;
}

/*****************************************************************************/
/* Gradient evaluation requires 88 flops.                                    */
/*   Reductions possible when b == 1 or c == 1                               */
/*****************************************************************************/
inline bool g_fcn_2e(double &obj, Vector3D g_obj[3], 
                     const Vector3D x[3], const Vector3D &n,
		     const double a, const double b, const double c)
{
  double matr[9], f;
  double adj_m[9], g;		// adj_m[2,5,8] not used
  double loc1, loc2, loc3;

  /* Calculate M = [A*inv(W) n] */
  matr[0] = x[1][0] - x[0][0];
  matr[1] = (2.0*x[2][0] - x[1][0] - x[0][0])*isqrt3;
  matr[2] = n[0];

  matr[3] = x[1][1] - x[0][1];
  matr[4] = (2.0*x[2][1] - x[1][1] - x[0][1])*isqrt3;
  matr[5] = n[1];

  matr[6] = x[1][2] - x[0][2];
  matr[7] = (2.0*x[2][2] - x[1][2] - x[0][2])*isqrt3;
  matr[8] = n[2];

  /* Calculate det([n M]). */
  loc1 = matr[4]*matr[8] - matr[5]*matr[7];
  loc2 = matr[2]*matr[7] - matr[1]*matr[8];
  loc3 = matr[1]*matr[5] - matr[2]*matr[4];
  g = matr[0]*loc1 + matr[3]*loc2 + matr[6]*loc3;
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] +
      matr[3]*matr[3] + matr[4]*matr[4] +
      matr[6]*matr[6] + matr[7]*matr[7];

  /* Calculate objective function. */
  obj  = a * pow(f, b) * pow(g, c);

  /* Calculate the derivative of the objective function.    */
  f = b * obj / f;		/* Constant on nabla f      */
  g = c * obj / g;              /* Constant on nable g      */
  f *= 2.0;                     /* Modification for nabla f */

  adj_m[0] = matr[0]*f + loc1*g;
  adj_m[3] = matr[3]*f + loc2*g;
  adj_m[6] = matr[6]*f + loc3*g;

  loc1 = matr[0]*g;
  loc2 = matr[3]*g;
  loc3 = matr[6]*g;

  adj_m[1] = matr[1]*f + loc3*matr[5] - loc2*matr[8];
  adj_m[4] = matr[4]*f + loc1*matr[8] - loc3*matr[2];
  adj_m[7] = matr[7]*f + loc2*matr[2] - loc1*matr[5];

  loc1 = isqrt3*adj_m[1];
  g_obj[0][0] = -adj_m[0] - loc1;
  g_obj[1][0] =  adj_m[0] - loc1;
  g_obj[2][0] = 2.0*loc1;

  loc1 = isqrt3*adj_m[4];
  g_obj[0][1] = -adj_m[3] - loc1;
  g_obj[1][1] =  adj_m[3] - loc1;
  g_obj[2][1] = 2.0*loc1;

  loc1 = isqrt3*adj_m[7];
  g_obj[0][2] = -adj_m[6] - loc1;
  g_obj[1][2] =  adj_m[6] - loc1;
  g_obj[2][2] = 2.0*loc1;
  return true;
}

/*****************************************************************************/
/* Hessian evaluation requires 316 flops.                                    */
/*   Reductions possible when b == 1 or c == 1                               */
/*****************************************************************************/
inline bool h_fcn_2e(double &obj, Vector3D g_obj[3], Matrix3D h_obj[6],
                     const Vector3D x[3], const Vector3D &n,
		     const double a, const double b, const double c)
{
  double matr[9], f;
  double adj_m[9], g;		// adj_m[2,5,8] not used
  double dg[9];			// dg[2,5,8] not used
  double loc0, loc1, loc2, loc3, loc4;
  double A[12], J_A[6], J_B[9], J_C[9], cross;  // only 2x2 corners used

  /* Calculate M = [A*inv(W) n] */
  matr[0] = x[1][0] - x[0][0];
  matr[1] = (2.0*x[2][0] - x[1][0] - x[0][0])*isqrt3;
  matr[2] = n[0];

  matr[3] = x[1][1] - x[0][1];
  matr[4] = (2.0*x[2][1] - x[1][1] - x[0][1])*isqrt3;
  matr[5] = n[1];

  matr[6] = x[1][2] - x[0][2];
  matr[7] = (2.0*x[2][2] - x[1][2] - x[0][2])*isqrt3;
  matr[8] = n[2];

  /* Calculate det([n M]). */
  dg[0] = matr[4]*matr[8] - matr[5]*matr[7];
  dg[3] = matr[2]*matr[7] - matr[1]*matr[8];
  dg[6] = matr[1]*matr[5] - matr[2]*matr[4];
  g = matr[0]*dg[0] + matr[3]*dg[3] + matr[6]*dg[6];
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] +
      matr[3]*matr[3] + matr[4]*matr[4] +
      matr[6]*matr[6] + matr[7]*matr[7];

  loc3 = f;
  loc4 = g;

  /* Calculate objective function. */
  obj  = a * pow(f, b) * pow(g, c);

  /* Calculate the derivative of the objective function.    */
  f = b * obj / f;		/* Constant on nabla f      */
  g = c * obj / g;              /* Constant on nable g      */
  f *= 2.0;                     /* Modification for nabla f */

  dg[1] = matr[5]*matr[6] - matr[3]*matr[8];
  dg[4] = matr[0]*matr[8] - matr[2]*matr[6];
  dg[7] = matr[2]*matr[3] - matr[0]*matr[5];

  adj_m[0] = matr[0]*f + dg[0]*g;
  adj_m[1] = matr[1]*f + dg[1]*g;
  adj_m[3] = matr[3]*f + dg[3]*g;
  adj_m[4] = matr[4]*f + dg[4]*g;
  adj_m[6] = matr[6]*f + dg[6]*g;
  adj_m[7] = matr[7]*f + dg[7]*g;

  loc1 = isqrt3*adj_m[1];
  g_obj[0][0] = -adj_m[0] - loc1;
  g_obj[1][0] =  adj_m[0] - loc1;
  g_obj[2][0] = 2.0*loc1;

  loc1 = isqrt3*adj_m[4];
  g_obj[0][1] = -adj_m[3] - loc1;
  g_obj[1][1] =  adj_m[3] - loc1;
  g_obj[2][1] = 2.0*loc1;

  loc1 = isqrt3*adj_m[7];
  g_obj[0][2] = -adj_m[6] - loc1;
  g_obj[1][2] =  adj_m[6] - loc1;
  g_obj[2][2] = 2.0*loc1;

  /* Calculate the hessian of the objective.                   */
  loc0 = f;			/* Constant on nabla^2 f       */
  loc1 = g;			/* Constant on nabla^2 g       */
  cross = f * c / loc4;		/* Constant on nabla g nabla f */
  f = f * (b-1) / loc3;		/* Constant on nabla f nabla f */
  g = g * (c-1) / loc4;		/* Constant on nabla g nabla g */
  f *= 2.0;                     /* Modification for nabla f    */

  /* First block of rows */
  loc3 = matr[0]*f + dg[0]*cross;
  loc4 = dg[0]*g + matr[0]*cross;

  J_A[0] = loc0 + loc3*matr[0] + loc4*dg[0];
  J_A[1] = loc3*matr[1] + loc4*dg[1];
  J_B[0] = loc3*matr[3] + loc4*dg[3];
  J_B[1] = loc3*matr[4] + loc4*dg[4];
  J_C[0] = loc3*matr[6] + loc4*dg[6];
  J_C[1] = loc3*matr[7] + loc4*dg[7];

  loc3 = matr[1]*f + dg[1]*cross;
  loc4 = dg[1]*g + matr[1]*cross;

  J_A[3] = loc0 + loc3*matr[1] + loc4*dg[1];
  J_B[3] = loc3*matr[3] + loc4*dg[3];
  J_B[4] = loc3*matr[4] + loc4*dg[4];
  J_C[3] = loc3*matr[6] + loc4*dg[6];
  J_C[4] = loc3*matr[7] + loc4*dg[7];

  /* First diagonal block */
  loc2 = isqrt3*J_A[1];
  A[0] = -J_A[0] - loc2;
  A[1] =  J_A[0] - loc2;

  loc2 = isqrt3*J_A[3];
  A[4] = -J_A[1] - loc2;
  A[5] =  J_A[1] - loc2;
  A[6] = 2.0*loc2;

  loc2 = isqrt3*A[4];
  h_obj[0][0][0] = -A[0] - loc2;
  h_obj[1][0][0] =  A[0] - loc2;
  h_obj[2][0][0] = 2.0*loc2;

  loc2 = isqrt3*A[5];
  h_obj[3][0][0] = A[1] - loc2;
  h_obj[4][0][0] = 2.0*loc2;

  h_obj[5][0][0] = tisqrt3*A[6];

  /* First off-diagonal block */
  loc2 = matr[8]*loc1;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  loc2 = isqrt3*J_B[3];
  A[0] = -J_B[0] - loc2;
  A[1] =  J_B[0] - loc2;
  A[2] = 2.0*loc2;

  loc2 = isqrt3*J_B[4];
  A[4] = -J_B[1] - loc2;
  A[5] =  J_B[1] - loc2;
  A[6] = 2.0*loc2;

  loc2 = isqrt3*A[4];
  h_obj[0][0][1] = -A[0] - loc2;
  h_obj[1][0][1] =  A[0] - loc2;
  h_obj[2][0][1] = 2.0*loc2;

  loc2 = isqrt3*A[5];
  h_obj[1][1][0] = -A[1] - loc2;
  h_obj[3][0][1] =  A[1] - loc2;
  h_obj[4][0][1] = 2.0*loc2;

  loc2 = isqrt3*A[6];
  h_obj[2][1][0] = -A[2] - loc2;
  h_obj[4][1][0] =  A[2] - loc2;
  h_obj[5][0][1] = 2.0*loc2;

  /* Second off-diagonal block */
  loc2 = matr[5]*loc1;
  J_C[1] -= loc2;
  J_C[3] += loc2;

  loc2 = isqrt3*J_C[3];
  A[0] = -J_C[0] - loc2;
  A[1] =  J_C[0] - loc2;
  A[2] = 2.0*loc2;

  loc2 = isqrt3*J_C[4];
  A[4] = -J_C[1] - loc2;
  A[5] =  J_C[1] - loc2;
  A[6] = 2.0*loc2;

  loc2 = isqrt3*A[4];
  h_obj[0][0][2] = -A[0] - loc2;
  h_obj[1][0][2] =  A[0] - loc2;
  h_obj[2][0][2] = 2.0*loc2;

  loc2 = isqrt3*A[5];
  h_obj[1][2][0] = -A[1] - loc2;
  h_obj[3][0][2] =  A[1] - loc2;
  h_obj[4][0][2] = 2.0*loc2;

  loc2 = isqrt3*A[6];
  h_obj[2][2][0] = -A[2] - loc2;
  h_obj[4][2][0] =  A[2] - loc2;
  h_obj[5][0][2] = 2.0*loc2;

  /* Second block of rows */
  loc3 = matr[3]*f + dg[3]*cross;
  loc4 = dg[3]*g + matr[3]*cross;

  J_A[0] = loc0 + loc3*matr[3] + loc4*dg[3];
  J_A[1] = loc3*matr[4] + loc4*dg[4];
  J_B[0] = loc3*matr[6] + loc4*dg[6];
  J_B[1] = loc3*matr[7] + loc4*dg[7];

  loc3 = matr[4]*f + dg[4]*cross;
  loc4 = dg[4]*g + matr[4]*cross;

  J_A[3] = loc0 + loc3*matr[4] + loc4*dg[4];
  J_B[3] = loc3*matr[6] + loc4*dg[6];
  J_B[4] = loc3*matr[7] + loc4*dg[7];

  /* Second diagonal block */
  loc2 = isqrt3*J_A[1];
  A[0] = -J_A[0] - loc2;
  A[1] =  J_A[0] - loc2;

  loc2 = isqrt3*J_A[3];
  A[4] = -J_A[1] - loc2;
  A[5] =  J_A[1] - loc2;
  A[6] = 2.0*loc2;

  loc2 = isqrt3*A[4];
  h_obj[0][1][1] = -A[0] - loc2;
  h_obj[1][1][1] =  A[0] - loc2;
  h_obj[2][1][1] = 2.0*loc2;

  loc2 = isqrt3*A[5];
  h_obj[3][1][1] = A[1] - loc2;
  h_obj[4][1][1] = 2.0*loc2;

  h_obj[5][1][1] = tisqrt3*A[6];

  /* Third off-diagonal block */
  loc2 = matr[2]*loc1;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  loc2 = isqrt3*J_B[3];
  A[0] = -J_B[0] - loc2;
  A[1] =  J_B[0] - loc2;
  A[2] = 2.0*loc2;

  loc2 = isqrt3*J_B[4];
  A[4] = -J_B[1] - loc2;
  A[5] =  J_B[1] - loc2;
  A[6] = 2.0*loc2;

  loc2 = isqrt3*A[4];
  h_obj[0][1][2] = -A[0] - loc2;
  h_obj[1][1][2] =  A[0] - loc2;
  h_obj[2][1][2] = 2.0*loc2;

  loc2 = isqrt3*A[5];
  h_obj[1][2][1] = -A[1] - loc2;
  h_obj[3][1][2] =  A[1] - loc2;
  h_obj[4][1][2] = 2.0*loc2;

  loc2 = isqrt3*A[6];
  h_obj[2][2][1] = -A[2] - loc2;
  h_obj[4][2][1] =  A[2] - loc2;
  h_obj[5][1][2] = 2.0*loc2;

  /* Third block of rows */
  loc3 = matr[6]*f + dg[6]*cross;
  loc4 = dg[6]*g + matr[6]*cross;

  J_A[0] = loc0 + loc3*matr[6] + loc4*dg[6];
  J_A[1] = loc3*matr[7] + loc4*dg[7];

  loc3 = matr[7]*f + dg[7]*cross;
  loc4 = dg[7]*g + matr[7]*cross;

  J_A[3] = loc0 + loc3*matr[7] + loc4*dg[7];

  /* Third diagonal block */
  loc2 = isqrt3*J_A[1];
  A[0] = -J_A[0] - loc2;
  A[1] =  J_A[0] - loc2;

  loc2 = isqrt3*J_A[3];
  A[4] = -J_A[1] - loc2;
  A[5] =  J_A[1] - loc2;
  A[6] = 2.0*loc2;

  loc2 = isqrt3*A[4];
  h_obj[0][2][2] = -A[0] - loc2;
  h_obj[1][2][2] =  A[0] - loc2;
  h_obj[2][2][2] = 2.0*loc2;

  loc2 = isqrt3*A[5];
  h_obj[3][2][2] = A[1] - loc2;
  h_obj[4][2][2] = 2.0*loc2;

  h_obj[5][2][2] = tisqrt3*A[6];

  // completes diagonal blocks.
  h_obj[0].fill_lower_triangle();
  h_obj[3].fill_lower_triangle();
  h_obj[5].fill_lower_triangle();
  return true;
}

/*****************************************************************************/
/* The following set of functions reference triangular elements to an        */
/* right triangle in the plane defined by the normal.  They are used when    */
/* assessing the quality of a quadrilateral elements.  A zero return value   */
/* indicates success, while a nonzero value indicates failure.               */
/*****************************************************************************/

/*****************************************************************************/
/* Function evaluation -- requires 41 flops.                                 */
/*   Reductions possible when b == 1, c == 1, or d == 1                      */
/*****************************************************************************/
inline bool m_fcn_2i(double &obj, const Vector3D x[3], const Vector3D &n,
		     const double a, const double b, const double c,
		     const Vector3D &d)
{
  double matr[9];
  double f;
  double g;

  /* Calculate M = A*inv(W). */
  matr[0] = d[0]*(x[1][0] - x[0][0]);
  matr[1] = d[1]*(x[2][0] - x[0][0]);
  matr[2] = n[0];

  matr[3] = d[0]*(x[1][1] - x[0][1]);
  matr[4] = d[1]*(x[2][1] - x[0][1]);
  matr[5] = n[1];

  matr[6] = d[0]*(x[1][2] - x[0][2]);
  matr[7] = d[1]*(x[2][2] - x[0][2]);
  matr[8] = n[2];

  /* Calculate det(M). */
  g = matr[0]*(matr[4]*matr[8] - matr[5]*matr[7]) +
      matr[3]*(matr[2]*matr[7] - matr[1]*matr[8]) +
      matr[6]*(matr[1]*matr[5] - matr[2]*matr[4]);
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] +
      matr[3]*matr[3] + matr[4]*matr[4] +
      matr[6]*matr[6] + matr[7]*matr[7];

  /* Calculate objective function. */
  obj = a * pow(f, b) * pow(g, c);
  return true;
}

/*****************************************************************************/
/* Gradient requires 82 flops.                                               */
/*   Reductions possible when b == 1, c == 1, or d == 1                      */
/*****************************************************************************/
inline bool g_fcn_2i(double &obj, Vector3D g_obj[3], 
                     const Vector3D x[3], const Vector3D &n,
		     const double a, const double b, const double c,
		     const Vector3D &d)
{
  double matr[9], f;
  double adj_m[9], g;            // adj_m[2,5,8] not used
  double loc1, loc2, loc3;

  /* Calculate M = [A*inv(W) n] */
  matr[0] = d[0]*(x[1][0] - x[0][0]);
  matr[1] = d[1]*(x[2][0] - x[0][0]);
  matr[2] = n[0];

  matr[3] = d[0]*(x[1][1] - x[0][1]);
  matr[4] = d[1]*(x[2][1] - x[0][1]);
  matr[5] = n[1];

  matr[6] = d[0]*(x[1][2] - x[0][2]);
  matr[7] = d[1]*(x[2][2] - x[0][2]);
  matr[8] = n[2];

  /* Calculate det([n M]). */
  loc1 = matr[4]*matr[8] - matr[5]*matr[7];
  loc2 = matr[2]*matr[7] - matr[1]*matr[8];
  loc3 = matr[1]*matr[5] - matr[2]*matr[4];
  g = matr[0]*loc1 + matr[3]*loc2 + matr[6]*loc3;
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] +
      matr[3]*matr[3] + matr[4]*matr[4] +
      matr[6]*matr[6] + matr[7]*matr[7];

  /* Calculate objective function. */
  obj  = a * pow(f, b) * pow(g, c);

  /* Calculate the derivative of the objective function.    */
  f = b * obj / f;		/* Constant on nabla f      */
  g = c * obj / g;              /* Constant on nable g      */
  f *= 2.0;                     /* Modification for nabla f */

  adj_m[0] = d[0]*(matr[0]*f + loc1*g);
  adj_m[3] = d[0]*(matr[3]*f + loc2*g);
  adj_m[6] = d[0]*(matr[6]*f + loc3*g);

  loc1 = matr[0]*g;
  loc2 = matr[3]*g;
  loc3 = matr[6]*g;

  adj_m[1] = d[1]*(matr[1]*f + loc3*matr[5] - loc2*matr[8]);
  adj_m[4] = d[1]*(matr[4]*f + loc1*matr[8] - loc3*matr[2]);
  adj_m[7] = d[1]*(matr[7]*f + loc2*matr[2] - loc1*matr[5]);

  g_obj[0][0] = -adj_m[0] - adj_m[1];
  g_obj[1][0] =  adj_m[0];
  g_obj[2][0] =  adj_m[1];

  g_obj[0][1] = -adj_m[3] - adj_m[4];
  g_obj[1][1] =  adj_m[3];
  g_obj[2][1] =  adj_m[4];

  g_obj[0][2] = -adj_m[6] - adj_m[7];
  g_obj[1][2] =  adj_m[6];
  g_obj[2][2] =  adj_m[7];
  return true;
}

/*****************************************************************************/
/* Hessian requires 253 flops.                                               */
/*   Reductions possible when b == 1, c == 1, or d == 1                      */
/*****************************************************************************/
inline bool h_fcn_2i(double &obj, Vector3D g_obj[3], Matrix3D h_obj[6],
                     const Vector3D x[3], const Vector3D &n,
		     const double a, const double b, const double c,
		     const Vector3D &d)
{
  double matr[9], f;
  double adj_m[9], g;		// adj_m[2,5,8] not used
  double dg[9];			// dg[2,5,8] not used
  double loc0, loc1, loc2, loc3, loc4;
  double A[12], J_A[6], J_B[9], J_C[9], cross;  // only 2x2 corners used

  const double scale[3] = {
    d[0]*d[0], d[0]*d[1],
               d[1]*d[1]
  };

  /* Calculate M = [A*inv(W) n] */
  matr[0] = d[0]*(x[1][0] - x[0][0]);
  matr[1] = d[1]*(x[2][0] - x[0][0]);
  matr[2] = n[0];

  matr[3] = d[0]*(x[1][1] - x[0][1]);
  matr[4] = d[1]*(x[2][1] - x[0][1]);
  matr[5] = n[1];

  matr[6] = d[0]*(x[1][2] - x[0][2]);
  matr[7] = d[1]*(x[2][2] - x[0][2]);
  matr[8] = n[2];

  /* Calculate det([n M]). */
  dg[0] = matr[4]*matr[8] - matr[5]*matr[7];
  dg[3] = matr[2]*matr[7] - matr[1]*matr[8];
  dg[6] = matr[1]*matr[5] - matr[2]*matr[4];
  g = matr[0]*dg[0] + matr[3]*dg[3] + matr[6]*dg[6];
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] +
      matr[3]*matr[3] + matr[4]*matr[4] +
      matr[6]*matr[6] + matr[7]*matr[7];

  loc3 = f;
  loc4 = g;

  /* Calculate objective function. */
  obj  = a * pow(f, b) * pow(g, c);

  /* Calculate the derivative of the objective function.    */
  f = b * obj / f;		/* Constant on nabla f      */
  g = c * obj / g;              /* Constant on nable g      */
  f *= 2.0;                     /* Modification for nabla f */

  dg[1] = matr[5]*matr[6] - matr[3]*matr[8];
  dg[4] = matr[0]*matr[8] - matr[2]*matr[6];
  dg[7] = matr[2]*matr[3] - matr[0]*matr[5];

  adj_m[0] = d[0]*(matr[0]*f + dg[0]*g);
  adj_m[1] = d[1]*(matr[1]*f + dg[1]*g);
  adj_m[3] = d[0]*(matr[3]*f + dg[3]*g);
  adj_m[4] = d[1]*(matr[4]*f + dg[4]*g);
  adj_m[6] = d[0]*(matr[6]*f + dg[6]*g);
  adj_m[7] = d[1]*(matr[7]*f + dg[7]*g);

  g_obj[0][0] = -adj_m[0] - adj_m[1];
  g_obj[1][0] =  adj_m[0];
  g_obj[2][0] =  adj_m[1];

  g_obj[0][1] = -adj_m[3] - adj_m[4];
  g_obj[1][1] =  adj_m[3];
  g_obj[2][1] =  adj_m[4];

  g_obj[0][2] = -adj_m[6] - adj_m[7];
  g_obj[1][2] =  adj_m[6];
  g_obj[2][2] =  adj_m[7];

  /* Calculate the hessian of the objective.                   */
  loc0 = f;			/* Constant on nabla^2 f       */
  loc1 = g;			/* Constant on nabla^2 g       */
  cross = f * c / loc4;		/* Constant on nabla g nabla f */
  f = f * (b-1) / loc3;		/* Constant on nabla f nabla f */
  g = g * (c-1) / loc4;		/* Constant on nabla g nabla g */
  f *= 2.0;                     /* Modification for nabla f    */

  /* First block of rows */
  loc3 = matr[0]*f + dg[0]*cross;
  loc4 = dg[0]*g + matr[0]*cross;

  J_A[0] = loc0 + loc3*matr[0] + loc4*dg[0];
  J_A[1] = loc3*matr[1] + loc4*dg[1];
  J_B[0] = loc3*matr[3] + loc4*dg[3];
  J_B[1] = loc3*matr[4] + loc4*dg[4];
  J_C[0] = loc3*matr[6] + loc4*dg[6];
  J_C[1] = loc3*matr[7] + loc4*dg[7];

  loc3 = matr[1]*f + dg[1]*cross;
  loc4 = dg[1]*g + matr[1]*cross;

  J_A[3] = loc0 + loc3*matr[1] + loc4*dg[1];
  J_B[3] = loc3*matr[3] + loc4*dg[3];
  J_B[4] = loc3*matr[4] + loc4*dg[4];
  J_C[3] = loc3*matr[6] + loc4*dg[6];
  J_C[4] = loc3*matr[7] + loc4*dg[7];

  /* First diagonal block */
  J_A[0] *= scale[0];
  J_A[1] *= scale[1];
  J_A[3] *= scale[2];

  A[0] = -J_A[0] - J_A[1];
  A[4] = -J_A[1] - J_A[3];

  h_obj[0][0][0] = -A[0] - A[4];
  h_obj[1][0][0] =  A[0];
  h_obj[2][0][0] =  A[4];

  h_obj[3][0][0] = J_A[0];
  h_obj[4][0][0] = J_A[1];

  h_obj[5][0][0] = J_A[3];

  /* First off-diagonal block */
  loc2 = matr[8]*loc1;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  J_B[0] *= scale[0];
  J_B[1] *= scale[1];
  J_B[3] *= scale[1];
  J_B[4] *= scale[2];

  A[0] = -J_B[0] - J_B[3];
  A[4] = -J_B[1] - J_B[4];

  h_obj[0][0][1] = -A[0] - A[4];
  h_obj[1][0][1] =  A[0];
  h_obj[2][0][1] =  A[4];

  h_obj[1][1][0] = -J_B[0] - J_B[1];
  h_obj[3][0][1] =  J_B[0];
  h_obj[4][0][1] =  J_B[1];

  h_obj[2][1][0] = -J_B[3] - J_B[4];
  h_obj[4][1][0] =  J_B[3];
  h_obj[5][0][1] =  J_B[4];

  /* Second off-diagonal block */
  loc2 = matr[5]*loc1;
  J_C[1] -= loc2;
  J_C[3] += loc2;

  J_C[0] *= scale[0];
  J_C[1] *= scale[1];
  J_C[3] *= scale[1];
  J_C[4] *= scale[2];

  A[0] = -J_C[0] - J_C[3];
  A[4] = -J_C[1] - J_C[4];

  h_obj[0][0][2] = -A[0] - A[4];
  h_obj[1][0][2] =  A[0];
  h_obj[2][0][2] =  A[4];

  h_obj[1][2][0] = -J_C[0] - J_C[1];
  h_obj[3][0][2] =  J_C[0];
  h_obj[4][0][2] =  J_C[1];

  h_obj[2][2][0] = -J_C[3] - J_C[4];
  h_obj[4][2][0] =  J_C[3];
  h_obj[5][0][2] =  J_C[4];

  /* Second block of rows */
  loc3 = matr[3]*f + dg[3]*cross;
  loc4 = dg[3]*g + matr[3]*cross;

  J_A[0] = loc0 + loc3*matr[3] + loc4*dg[3];
  J_A[1] = loc3*matr[4] + loc4*dg[4];
  J_B[0] = loc3*matr[6] + loc4*dg[6];
  J_B[1] = loc3*matr[7] + loc4*dg[7];

  loc3 = matr[4]*f + dg[4]*cross;
  loc4 = dg[4]*g + matr[4]*cross;

  J_A[3] = loc0 + loc3*matr[4] + loc4*dg[4];
  J_B[3] = loc3*matr[6] + loc4*dg[6];
  J_B[4] = loc3*matr[7] + loc4*dg[7];

  /* Second diagonal block */
  J_A[0] *= scale[0];
  J_A[1] *= scale[1];
  J_A[3] *= scale[2];

  A[0] = -J_A[0] - J_A[1];
  A[4] = -J_A[1] - J_A[3];

  h_obj[0][1][1] = -A[0] - A[4];
  h_obj[1][1][1] =  A[0];
  h_obj[2][1][1] =  A[4];

  h_obj[3][1][1] = J_A[0];
  h_obj[4][1][1] = J_A[1];

  h_obj[5][1][1] = J_A[3];

  /* Third off-diagonal block */
  loc2 = matr[2]*loc1;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  J_B[0] *= scale[0];
  J_B[1] *= scale[1];
  J_B[3] *= scale[1];
  J_B[4] *= scale[2];

  A[0] = -J_B[0] - J_B[3];
  A[4] = -J_B[1] - J_B[4];

  h_obj[0][1][2] = -A[0] - A[4];
  h_obj[1][1][2] =  A[0];
  h_obj[2][1][2] =  A[4];

  h_obj[1][2][1] = -J_B[0] - J_B[1];
  h_obj[3][1][2] =  J_B[0];
  h_obj[4][1][2] =  J_B[1];

  h_obj[2][2][1] = -J_B[3] - J_B[4];
  h_obj[4][2][1] =  J_B[3];
  h_obj[5][1][2] =  J_B[4];

  /* Third block of rows */
  loc3 = matr[6]*f + dg[6]*cross;
  loc4 = dg[6]*g + matr[6]*cross;

  J_A[0] = loc0 + loc3*matr[6] + loc4*dg[6];
  J_A[1] = loc3*matr[7] + loc4*dg[7];

  loc3 = matr[7]*f + dg[7]*cross;
  loc4 = dg[7]*g + matr[7]*cross;

  J_A[3] = loc0 + loc3*matr[7] + loc4*dg[7];

  /* Third diagonal block */
  J_A[0] *= scale[0];
  J_A[1] *= scale[1];
  J_A[3] *= scale[2];

  A[0] = -J_A[0] - J_A[1];
  A[4] = -J_A[1] - J_A[3];

  h_obj[0][2][2] = -A[0] - A[4];
  h_obj[1][2][2] =  A[0];
  h_obj[2][2][2] =  A[4];

  h_obj[3][2][2] = J_A[0];
  h_obj[4][2][2] = J_A[1];

  h_obj[5][2][2] = J_A[3];

  // completes diagonal blocks.
  h_obj[0].fill_lower_triangle();
  h_obj[3].fill_lower_triangle();
  h_obj[5].fill_lower_triangle();
  return true;
}

/*****************************************************************************/
/* The following set of functions reference tetrahedral elements to a        */
/* regular tetrahedron.  They are used when assessing the quality of a       */
/* tetrahedral element.  A zero return value indicates success, while        */
/* a nonzero value indicates failure.                                        */
/*****************************************************************************/

/*****************************************************************************/
/* Function evaluation requires 62 flops.                                    */
/*   Reductions possible when b == 1 or c == 1                               */
/*****************************************************************************/
inline bool m_fcn_3e(double &obj, const Vector3D x[4],
		     const double a, const double b, const double c)
{
  double matr[9], f;
  double g;

  /* Calculate M = A*inv(W). */
  f       = x[1][0] + x[0][0];
  matr[0] = x[1][0] - x[0][0];
  matr[1] = (2.0*x[2][0] - f)*isqrt3;
  matr[2] = (3.0*x[3][0] - x[2][0] - f)*isqrt6;

  f       = x[1][1] + x[0][1];
  matr[3] = x[1][1] - x[0][1];
  matr[4] = (2.0*x[2][1] - f)*isqrt3;
  matr[5] = (3.0*x[3][1] - x[2][1] - f)*isqrt6;

  f       = x[1][2] + x[0][2];
  matr[6] = x[1][2] - x[0][2];
  matr[7] = (2.0*x[2][2] - f)*isqrt3;
  matr[8] = (3.0*x[3][2] - x[2][2] - f)*isqrt6;

  /* Calculate det(M). */
  g = matr[0]*(matr[4]*matr[8] - matr[5]*matr[7]) +
      matr[1]*(matr[5]*matr[6] - matr[3]*matr[8]) +
      matr[2]*(matr[3]*matr[7] - matr[4]*matr[6]);
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] + matr[2]*matr[2] +
      matr[3]*matr[3] + matr[4]*matr[4] + matr[5]*matr[5] +
      matr[6]*matr[6] + matr[7]*matr[7] + matr[8]*matr[8];

  /* Calculate objective function. */
  obj = a * pow(f, b) * pow(g, c);
  return true;
}

/*****************************************************************************/
/* Gradient evaluation requires 133 flops.                                   */
/*   Reductions possible when b == 1 or c == 1                               */
/*****************************************************************************/
inline bool g_fcn_3e(double &obj, Vector3D g_obj[4], const Vector3D x[4],
		     const double a, const double b, const double c)
{
  double matr[9], f;
  double adj_m[9], g;
  double loc1, loc2, loc3;

  /* Calculate M = A*inv(W). */
  f       = x[1][0] + x[0][0];
  matr[0] = x[1][0] - x[0][0];
  matr[1] = (2.0*x[2][0] - f)*isqrt3;
  matr[2] = (3.0*x[3][0] - x[2][0] - f)*isqrt6;

  f       = x[1][1] + x[0][1];
  matr[3] = x[1][1] - x[0][1];
  matr[4] = (2.0*x[2][1] - f)*isqrt3;
  matr[5] = (3.0*x[3][1] - x[2][1] - f)*isqrt6;

  f       = x[1][2] + x[0][2];
  matr[6] = x[1][2] - x[0][2];
  matr[7] = (2.0*x[2][2] - f)*isqrt3;
  matr[8] = (3.0*x[3][2] - x[2][2] - f)*isqrt6;

  /* Calculate det(M). */
  loc1 = matr[4]*matr[8] - matr[5]*matr[7];
  loc2 = matr[5]*matr[6] - matr[3]*matr[8];
  loc3 = matr[3]*matr[7] - matr[4]*matr[6];
  g = matr[0]*loc1 + matr[1]*loc2 + matr[2]*loc3;
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] + matr[2]*matr[2] +
      matr[3]*matr[3] + matr[4]*matr[4] + matr[5]*matr[5] +
      matr[6]*matr[6] + matr[7]*matr[7] + matr[8]*matr[8];

  /* Calculate objective function. */
  obj  = a * pow(f, b) * pow(g, c);

  /* Calculate the derivative of the objective function.    */
  f = b * obj / f;		/* Constant on nabla f      */
  g = c * obj / g;              /* Constant on nable g      */
  f *= 2.0;                     /* Modification for nabla f */

  adj_m[0] = matr[0]*f + loc1*g;
  adj_m[1] = matr[1]*f + loc2*g;
  adj_m[2] = matr[2]*f + loc3*g;

  loc1 = matr[0]*g;
  loc2 = matr[1]*g;
  loc3 = matr[2]*g;

  adj_m[3] = matr[3]*f + loc3*matr[7] - loc2*matr[8];
  adj_m[4] = matr[4]*f + loc1*matr[8] - loc3*matr[6];
  adj_m[5] = matr[5]*f + loc2*matr[6] - loc1*matr[7];

  adj_m[6] = matr[6]*f + loc2*matr[5] - loc3*matr[4];
  adj_m[7] = matr[7]*f + loc3*matr[3] - loc1*matr[5];
  adj_m[8] = matr[8]*f + loc1*matr[4] - loc2*matr[3];

  loc1 = isqrt3*adj_m[1];
  loc2 = isqrt6*adj_m[2];
  loc3 = loc1 + loc2;
  g_obj[0][0] = -adj_m[0] - loc3;
  g_obj[1][0] = adj_m[0] - loc3;
  g_obj[2][0] = 2.0*loc1 - loc2;
  g_obj[3][0] = 3.0*loc2;

  loc1 = isqrt3*adj_m[4];
  loc2 = isqrt6*adj_m[5];
  loc3 = loc1 + loc2;
  g_obj[0][1] = -adj_m[3] - loc3;
  g_obj[1][1] = adj_m[3] - loc3;
  g_obj[2][1] = 2.0*loc1 - loc2;
  g_obj[3][1] = 3.0*loc2;

  loc1 = isqrt3*adj_m[7];
  loc2 = isqrt6*adj_m[8];
  loc3 = loc1 + loc2;
  g_obj[0][2] = -adj_m[6] - loc3;
  g_obj[1][2] = adj_m[6] - loc3;
  g_obj[2][2] = 2.0*loc1 - loc2;
  g_obj[3][2] = 3.0*loc2;
  return true;
}

/*****************************************************************************/
/* Hessian evaluation requires 634 flops.                                    */
/*   Reductions possible when b == 1 or c == 1                               */
/*****************************************************************************/
inline bool h_fcn_3e(double &obj, Vector3D g_obj[4], Matrix3D h_obj[10], 
		     const Vector3D x[4],
		     const double a, const double b, const double c)
{
  double matr[9], f;
  double adj_m[9], g;
  double dg[9], loc0, loc1, loc2, loc3, loc4;
  double A[12], J_A[6], J_B[9], J_C[9], cross;

  /* Calculate M = A*inv(W). */
  f       = x[1][0] + x[0][0];
  matr[0] = x[1][0] - x[0][0];
  matr[1] = (2.0*x[2][0] - f)*isqrt3;
  matr[2] = (3.0*x[3][0] - x[2][0] - f)*isqrt6;

  f       = x[1][1] + x[0][1];
  matr[3] = x[1][1] - x[0][1];
  matr[4] = (2.0*x[2][1] - f)*isqrt3;
  matr[5] = (3.0*x[3][1] - x[2][1] - f)*isqrt6;

  f       = x[1][2] + x[0][2];
  matr[6] = x[1][2] - x[0][2];
  matr[7] = (2.0*x[2][2] - f)*isqrt3;
  matr[8] = (3.0*x[3][2] - x[2][2] - f)*isqrt6;

  /* Calculate det(M). */
  dg[0] = matr[4]*matr[8] - matr[5]*matr[7];
  dg[1] = matr[5]*matr[6] - matr[3]*matr[8];
  dg[2] = matr[3]*matr[7] - matr[4]*matr[6];
  g = matr[0]*dg[0] + matr[1]*dg[1] + matr[2]*dg[2];
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] + matr[2]*matr[2] + 
      matr[3]*matr[3] + matr[4]*matr[4] + matr[5]*matr[5] +
      matr[6]*matr[6] + matr[7]*matr[7] + matr[8]*matr[8];

  loc3 = f;
  loc4 = g;

  /* Calculate objective function. */
  obj  = a * pow(f, b) * pow(g, c);

  /* Calculate the derivative of the objective function.    */
  f = b * obj / f;		/* Constant on nabla f      */
  g = c * obj / g;              /* Constant on nable g      */
  f *= 2.0;                     /* Modification for nabla f */

  dg[3] = matr[2]*matr[7] - matr[1]*matr[8];
  dg[4] = matr[0]*matr[8] - matr[2]*matr[6];
  dg[5] = matr[1]*matr[6] - matr[0]*matr[7];
  dg[6] = matr[1]*matr[5] - matr[2]*matr[4];
  dg[7] = matr[2]*matr[3] - matr[0]*matr[5];
  dg[8] = matr[0]*matr[4] - matr[1]*matr[3];

  adj_m[0] = matr[0]*f + dg[0]*g;
  adj_m[1] = matr[1]*f + dg[1]*g;
  adj_m[2] = matr[2]*f + dg[2]*g;
  adj_m[3] = matr[3]*f + dg[3]*g;
  adj_m[4] = matr[4]*f + dg[4]*g;
  adj_m[5] = matr[5]*f + dg[5]*g;
  adj_m[6] = matr[6]*f + dg[6]*g;
  adj_m[7] = matr[7]*f + dg[7]*g;
  adj_m[8] = matr[8]*f + dg[8]*g;

  loc0 = isqrt3*adj_m[1];
  loc1 = isqrt6*adj_m[2];
  loc2 = loc0 + loc1;
  g_obj[0][0] = -adj_m[0] - loc2;
  g_obj[1][0] = adj_m[0] - loc2;
  g_obj[2][0] = 2.0*loc0 - loc1;
  g_obj[3][0] = 3.0*loc1;

  loc0 = isqrt3*adj_m[4];
  loc1 = isqrt6*adj_m[5];
  loc2 = loc0 + loc1;
  g_obj[0][1] = -adj_m[3] - loc2;
  g_obj[1][1] = adj_m[3] - loc2;
  g_obj[2][1] = 2.0*loc0 - loc1;
  g_obj[3][1] = 3.0*loc1;

  loc0 = isqrt3*adj_m[7];
  loc1 = isqrt6*adj_m[8];
  loc2 = loc0 + loc1;
  g_obj[0][2] = -adj_m[6] - loc2;
  g_obj[1][2] = adj_m[6] - loc2;
  g_obj[2][2] = 2.0*loc0 - loc1;
  g_obj[3][2] = 3.0*loc1;

  /* Calculate the hessian of the objective.                   */
  loc0 = f;			/* Constant on nabla^2 f       */
  loc1 = g;			/* Constant on nabla^2 g       */
  cross = f * c / loc4;		/* Constant on nabla g nabla f */
  f = f * (b-1) / loc3;		/* Constant on nabla f nabla f */
  g = g * (c-1) / loc4;		/* Constant on nabla g nabla g */
  f *= 2.0;                     /* Modification for nabla f    */

  /* First block of rows */
  loc3 = matr[0]*f + dg[0]*cross;
  loc4 = dg[0]*g + matr[0]*cross;

  J_A[0] = loc0 + loc3*matr[0] + loc4*dg[0];
  J_A[1] = loc3*matr[1] + loc4*dg[1];
  J_A[2] = loc3*matr[2] + loc4*dg[2];
  J_B[0] = loc3*matr[3] + loc4*dg[3];
  J_B[1] = loc3*matr[4] + loc4*dg[4];
  J_B[2] = loc3*matr[5] + loc4*dg[5];
  J_C[0] = loc3*matr[6] + loc4*dg[6];
  J_C[1] = loc3*matr[7] + loc4*dg[7];
  J_C[2] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[1]*f + dg[1]*cross;
  loc4 = dg[1]*g + matr[1]*cross;

  J_A[3] = loc0 + loc3*matr[1] + loc4*dg[1];
  J_A[4] = loc3*matr[2] + loc4*dg[2];
  J_B[3] = loc3*matr[3] + loc4*dg[3];
  J_B[4] = loc3*matr[4] + loc4*dg[4];
  J_B[5] = loc3*matr[5] + loc4*dg[5];
  J_C[3] = loc3*matr[6] + loc4*dg[6];
  J_C[4] = loc3*matr[7] + loc4*dg[7];
  J_C[5] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[2]*f + dg[2]*cross;
  loc4 = dg[2]*g + matr[2]*cross;

  J_A[5] = loc0 + loc3*matr[2] + loc4*dg[2];
  J_B[6] = loc3*matr[3] + loc4*dg[3];
  J_B[7] = loc3*matr[4] + loc4*dg[4];
  J_B[8] = loc3*matr[5] + loc4*dg[5];
  J_C[6] = loc3*matr[6] + loc4*dg[6];
  J_C[7] = loc3*matr[7] + loc4*dg[7];
  J_C[8] = loc3*matr[8] + loc4*dg[8];

  /* First diagonal block */
  loc2 = isqrt3*J_A[1];
  loc3 = isqrt6*J_A[2];
  loc4 = loc2 + loc3;

  A[0] = -J_A[0] - loc4;
  A[1] =  J_A[0] - loc4;

  loc2 = isqrt3*J_A[3];
  loc3 = isqrt6*J_A[4];
  loc4 = loc2 + loc3;

  A[4] = -J_A[1] - loc4;
  A[5] =  J_A[1] - loc4;
  A[6] = 2.0*loc2 - loc3;

  loc2 = isqrt3*J_A[4];
  loc3 = isqrt6*J_A[5];
  loc4 = loc2 + loc3;

  A[8] = -J_A[2] - loc4;
  A[9] =  J_A[2] - loc4;
  A[10] = 2.0*loc2 - loc3;
  A[11] = 3.0*loc3;

  loc2 = isqrt3*A[4];
  loc3 = isqrt6*A[8];
  loc4 = loc2 + loc3;

  h_obj[0][0][0] = -A[0] - loc4;
  h_obj[1][0][0] =  A[0] - loc4;
  h_obj[2][0][0] = 2.0*loc2 - loc3;
  h_obj[3][0][0] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];

  h_obj[4][0][0] = A[1] - loc2 - loc3;
  h_obj[5][0][0] = 2.0*loc2 - loc3;
  h_obj[6][0][0] = 3.0*loc3;

  loc3 = isqrt6*A[10];
  h_obj[7][0][0] = tisqrt3*A[6] - loc3;
  h_obj[8][0][0] = 3.0*loc3;

  h_obj[9][0][0] = tisqrt6*A[11];

  /* First off-diagonal block */
  loc2 = matr[8]*loc1;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  loc2 = matr[7]*loc1;
  J_B[2] -= loc2;
  J_B[6] += loc2;

  loc2 = matr[6]*loc1;
  J_B[5] += loc2;
  J_B[7] -= loc2;

  loc2 = isqrt3*J_B[3];
  loc3 = isqrt6*J_B[6];
  loc4 = loc2 + loc3;

  A[0] = -J_B[0] - loc4;
  A[1] =  J_B[0] - loc4;
  A[2] = 2.0*loc2 - loc3;
  A[3] = 3.0*loc3;

  loc2 = isqrt3*J_B[4];
  loc3 = isqrt6*J_B[7];
  loc4 = loc2 + loc3;

  A[4] = -J_B[1] - loc4;
  A[5] =  J_B[1] - loc4;
  A[6] = 2.0*loc2 - loc3;
  A[7] = 3.0*loc3;

  loc2 = isqrt3*J_B[5];
  loc3 = isqrt6*J_B[8];
  loc4 = loc2 + loc3;

  A[8] = -J_B[2] - loc4;
  A[9] =  J_B[2] - loc4;
  A[10] = 2.0*loc2 - loc3;
  A[11] = 3.0*loc3;

  loc2 = isqrt3*A[4];
  loc3 = isqrt6*A[8];
  loc4 = loc2 + loc3;

  h_obj[0][0][1] = -A[0] - loc4;
  h_obj[1][0][1] =  A[0] - loc4;
  h_obj[2][0][1] = 2.0*loc2 - loc3;
  h_obj[3][0][1] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];
  loc4 = loc2 + loc3;

  h_obj[1][1][0] = -A[1] - loc4;
  h_obj[4][0][1] =  A[1] - loc4;
  h_obj[5][0][1] = 2.0*loc2 - loc3;
  h_obj[6][0][1] = 3.0*loc3;

  loc2 = isqrt3*A[6];
  loc3 = isqrt6*A[10];
  loc4 = loc2 + loc3;

  h_obj[2][1][0] = -A[2] - loc4;
  h_obj[5][1][0] =  A[2] - loc4;
  h_obj[7][0][1] = 2.0*loc2 - loc3;
  h_obj[8][0][1] = 3.0*loc3;

  loc2 = isqrt3*A[7];
  loc3 = isqrt6*A[11];
  loc4 = loc2 + loc3;

  h_obj[3][1][0] = -A[3] - loc4;
  h_obj[6][1][0] =  A[3] - loc4;
  h_obj[8][1][0] = 2.0*loc2 - loc3;
  h_obj[9][0][1] = 3.0*loc3;

  /* Second off-diagonal block */
  loc2 = matr[5]*loc1;
  J_C[1] -= loc2;
  J_C[3] += loc2;

  loc2 = matr[4]*loc1;
  J_C[2] += loc2;
  J_C[6] -= loc2;

  loc2 = matr[3]*loc1;
  J_C[5] -= loc2;
  J_C[7] += loc2;

  loc2 = isqrt3*J_C[3];
  loc3 = isqrt6*J_C[6];
  loc4 = loc2 + loc3;

  A[0] = -J_C[0] - loc4;
  A[1] =  J_C[0] - loc4;
  A[2] = 2.0*loc2 - loc3;
  A[3] = 3.0*loc3;

  loc2 = isqrt3*J_C[4];
  loc3 = isqrt6*J_C[7];
  loc4 = loc2 + loc3;

  A[4] = -J_C[1] - loc4;
  A[5] =  J_C[1] - loc4;
  A[6] = 2.0*loc2 - loc3;
  A[7] = 3.0*loc3;

  loc2 = isqrt3*J_C[5];
  loc3 = isqrt6*J_C[8];
  loc4 = loc2 + loc3;

  A[8] = -J_C[2] - loc4;
  A[9] =  J_C[2] - loc4;
  A[10] = 2.0*loc2 - loc3;
  A[11] = 3.0*loc3;

  loc2 = isqrt3*A[4];
  loc3 = isqrt6*A[8];
  loc4 = loc2 + loc3;

  h_obj[0][0][2] = -A[0] - loc4;
  h_obj[1][0][2] =  A[0] - loc4;
  h_obj[2][0][2] = 2.0*loc2 - loc3;
  h_obj[3][0][2] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];
  loc4 = loc2 + loc3;

  h_obj[1][2][0] = -A[1] - loc4;
  h_obj[4][0][2] =  A[1] - loc4;
  h_obj[5][0][2] = 2.0*loc2 - loc3;
  h_obj[6][0][2] = 3.0*loc3;

  loc2 = isqrt3*A[6];
  loc3 = isqrt6*A[10];
  loc4 = loc2 + loc3;

  h_obj[2][2][0] = -A[2] - loc4;
  h_obj[5][2][0] =  A[2] - loc4;
  h_obj[7][0][2] = 2.0*loc2 - loc3;
  h_obj[8][0][2] = 3.0*loc3;

  loc2 = isqrt3*A[7];
  loc3 = isqrt6*A[11];
  loc4 = loc2 + loc3;

  h_obj[3][2][0] = -A[3] - loc4;
  h_obj[6][2][0] =  A[3] - loc4;
  h_obj[8][2][0] = 2.0*loc2 - loc3;
  h_obj[9][0][2] = 3.0*loc3;

  /* Second block of rows */
  loc3 = matr[3]*f + dg[3]*cross;
  loc4 = dg[3]*g + matr[3]*cross;

  J_A[0] = loc0 + loc3*matr[3] + loc4*dg[3];
  J_A[1] = loc3*matr[4] + loc4*dg[4];
  J_A[2] = loc3*matr[5] + loc4*dg[5];
  J_B[0] = loc3*matr[6] + loc4*dg[6];
  J_B[1] = loc3*matr[7] + loc4*dg[7];
  J_B[2] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[4]*f + dg[4]*cross;
  loc4 = dg[4]*g + matr[4]*cross;

  J_A[3] = loc0 + loc3*matr[4] + loc4*dg[4];
  J_A[4] = loc3*matr[5] + loc4*dg[5];
  J_B[3] = loc3*matr[6] + loc4*dg[6];
  J_B[4] = loc3*matr[7] + loc4*dg[7];
  J_B[5] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[5]*f + dg[5]*cross;
  loc4 = dg[5]*g + matr[5]*cross;

  J_A[5] = loc0 + loc3*matr[5] + loc4*dg[5];
  J_B[6] = loc3*matr[6] + loc4*dg[6];
  J_B[7] = loc3*matr[7] + loc4*dg[7];
  J_B[8] = loc3*matr[8] + loc4*dg[8];

  /* Second diagonal block */
  loc2 = isqrt3*J_A[1];
  loc3 = isqrt6*J_A[2];
  loc4 = loc2 + loc3;

  A[0] = -J_A[0] - loc4;
  A[1] =  J_A[0] - loc4;

  loc2 = isqrt3*J_A[3];
  loc3 = isqrt6*J_A[4];
  loc4 = loc2 + loc3;

  A[4] = -J_A[1] - loc4;
  A[5] =  J_A[1] - loc4;
  A[6] = 2.0*loc2 - loc3;

  loc2 = isqrt3*J_A[4];
  loc3 = isqrt6*J_A[5];
  loc4 = loc2 + loc3;

  A[8] = -J_A[2] - loc4;
  A[9] =  J_A[2] - loc4;
  A[10] = 2.0*loc2 - loc3;
  A[11] = 3.0*loc3;

  loc2 = isqrt3*A[4];
  loc3 = isqrt6*A[8];
  loc4 = loc2 + loc3;

  h_obj[0][1][1] = -A[0] - loc4;
  h_obj[1][1][1] =  A[0] - loc4;
  h_obj[2][1][1] = 2.0*loc2 - loc3;
  h_obj[3][1][1] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];

  h_obj[4][1][1] = A[1] - loc2 - loc3;
  h_obj[5][1][1] = 2.0*loc2 - loc3;
  h_obj[6][1][1] = 3.0*loc3;

  loc3 = isqrt6*A[10];
  h_obj[7][1][1] = tisqrt3*A[6] - loc3;
  h_obj[8][1][1] = 3.0*loc3;

  h_obj[9][1][1] = tisqrt6*A[11];

  /* Third off-diagonal block */
  loc2 = matr[2]*loc1;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  loc2 = matr[1]*loc1;
  J_B[2] -= loc2;
  J_B[6] += loc2;

  loc2 = matr[0]*loc1;
  J_B[5] += loc2;
  J_B[7] -= loc2;

  loc2 = isqrt3*J_B[3];
  loc3 = isqrt6*J_B[6];
  loc4 = loc2 + loc3;

  A[0] = -J_B[0] - loc4;
  A[1] =  J_B[0] - loc4;
  A[2] = 2.0*loc2 - loc3;
  A[3] = 3.0*loc3;

  loc2 = isqrt3*J_B[4];
  loc3 = isqrt6*J_B[7];
  loc4 = loc2 + loc3;

  A[4] = -J_B[1] - loc4;
  A[5] =  J_B[1] - loc4;
  A[6] = 2.0*loc2 - loc3;
  A[7] = 3.0*loc3;

  loc2 = isqrt3*J_B[5];
  loc3 = isqrt6*J_B[8];
  loc4 = loc2 + loc3;

  A[8] = -J_B[2] - loc4;
  A[9] =  J_B[2] - loc4;
  A[10] = 2.0*loc2 - loc3;
  A[11] = 3.0*loc3;

  loc2 = isqrt3*A[4];
  loc3 = isqrt6*A[8];
  loc4 = loc2 + loc3;

  h_obj[0][1][2] = -A[0] - loc4;
  h_obj[1][1][2] =  A[0] - loc4;
  h_obj[2][1][2] = 2.0*loc2 - loc3;
  h_obj[3][1][2] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];
  loc4 = loc2 + loc3;

  h_obj[1][2][1] = -A[1] - loc4;
  h_obj[4][1][2] =  A[1] - loc4;
  h_obj[5][1][2] = 2.0*loc2 - loc3;
  h_obj[6][1][2] = 3.0*loc3;

  loc2 = isqrt3*A[6];
  loc3 = isqrt6*A[10];
  loc4 = loc2 + loc3;

  h_obj[2][2][1] = -A[2] - loc4;
  h_obj[5][2][1] =  A[2] - loc4;
  h_obj[7][1][2] = 2.0*loc2 - loc3;
  h_obj[8][1][2] = 3.0*loc3;

  loc2 = isqrt3*A[7];
  loc3 = isqrt6*A[11];
  loc4 = loc2 + loc3;

  h_obj[3][2][1] = -A[3] - loc4;
  h_obj[6][2][1] =  A[3] - loc4;
  h_obj[8][2][1] = 2.0*loc2 - loc3;
  h_obj[9][1][2] = 3.0*loc3;

  /* Third block of rows */
  loc3 = matr[6]*f + dg[6]*cross;
  loc4 = dg[6]*g + matr[6]*cross;

  J_A[0] = loc0 + loc3*matr[6] + loc4*dg[6];
  J_A[1] = loc3*matr[7] + loc4*dg[7];
  J_A[2] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[7]*f + dg[7]*cross;
  loc4 = dg[7]*g + matr[7]*cross;

  J_A[3] = loc0 + loc3*matr[7] + loc4*dg[7];
  J_A[4] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[8]*f + dg[8]*cross;
  loc4 = dg[8]*g + matr[8]*cross;

  J_A[5] = loc0 + loc3*matr[8] + loc4*dg[8];

  /* Third diagonal block */
  loc2 = isqrt3*J_A[1];
  loc3 = isqrt6*J_A[2];
  loc4 = loc2 + loc3;

  A[0] = -J_A[0] - loc4;
  A[1] =  J_A[0] - loc4;

  loc2 = isqrt3*J_A[3];
  loc3 = isqrt6*J_A[4];
  loc4 = loc2 + loc3;

  A[4] = -J_A[1] - loc4;
  A[5] =  J_A[1] - loc4;
  A[6] = 2.0*loc2 - loc3;

  loc2 = isqrt3*J_A[4];
  loc3 = isqrt6*J_A[5];
  loc4 = loc2 + loc3;

  A[8] = -J_A[2] - loc4;
  A[9] =  J_A[2] - loc4;
  A[10] = 2.0*loc2 - loc3;
  A[11] = 3.0*loc3;

  loc2 = isqrt3*A[4];
  loc3 = isqrt6*A[8];
  loc4 = loc2 + loc3;

  h_obj[0][2][2] = -A[0] - loc4;
  h_obj[1][2][2] =  A[0] - loc4;
  h_obj[2][2][2] = 2.0*loc2 - loc3;
  h_obj[3][2][2] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];

  h_obj[4][2][2] = A[1] - loc2 - loc3;
  h_obj[5][2][2] = 2.0*loc2 - loc3;
  h_obj[6][2][2] = 3.0*loc3;

  loc3 = isqrt6*A[10];
  h_obj[7][2][2] = tisqrt3*A[6] - loc3;
  h_obj[8][2][2] = 3.0*loc3;

  h_obj[9][2][2] = tisqrt6*A[11];

  // completes diagonal blocks.
  h_obj[0].fill_lower_triangle();
  h_obj[4].fill_lower_triangle();
  h_obj[7].fill_lower_triangle();
  h_obj[9].fill_lower_triangle();
  return true;
}

/*****************************************************************************/
/* The following set of functions reference tetrahedral elements to a        */
/* right tetrahedron.  They are used when assessing the quality of a         */
/* hexahedral element.  A zero return value indicates success, while         */
/* a nonzero value indicates failure.                                        */
/*****************************************************************************/

/*****************************************************************************/
/* Function evaluation requires 53 flops.                                    */
/*   Reductions possible when b == 1, c == 1, or d == 1                      */
/*****************************************************************************/
inline bool m_fcn_3i(double &obj, const Vector3D x[4], 
		     const double a, const double b, const double c,
		     const Vector3D &d)
{
  double matr[9], f;
  double g;

  /* Calculate M = A*inv(W). */
  matr[0] = d[0]*(x[1][0] - x[0][0]);
  matr[1] = d[1]*(x[2][0] - x[0][0]);
  matr[2] = d[2]*(x[3][0] - x[0][0]);

  matr[3] = d[0]*(x[1][1] - x[0][1]);
  matr[4] = d[1]*(x[2][1] - x[0][1]);
  matr[5] = d[2]*(x[3][1] - x[0][1]);

  matr[6] = d[0]*(x[1][2] - x[0][2]);
  matr[7] = d[1]*(x[2][2] - x[0][2]);
  matr[8] = d[2]*(x[3][2] - x[0][2]);

  /* Calculate det(M). */
  g = matr[0]*(matr[4]*matr[8] - matr[5]*matr[7]) +
      matr[1]*(matr[5]*matr[6] - matr[3]*matr[8]) +
      matr[2]*(matr[3]*matr[7] - matr[4]*matr[6]);
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] + matr[2]*matr[2] +
      matr[3]*matr[3] + matr[4]*matr[4] + matr[5]*matr[5] +
      matr[6]*matr[6] + matr[7]*matr[7] + matr[8]*matr[8];

  /* Calculate objective function. */
  obj = a * pow(f, b) * pow(g, c);
  return true;
}

/*****************************************************************************/
/* Gradient evaluation requires 115 flops.                                   */
/*   Reductions possible when b == 1, c == 1, or d == 1                      */
/*****************************************************************************/
inline bool g_fcn_3i(double &obj, Vector3D g_obj[4], const Vector3D x[4], 
		     const double a, const double b, const double c,
		     const Vector3D &d)
{
  double matr[9], f;
  double adj_m[9], g;
  double loc1, loc2, loc3;

  /* Calculate M = A*inv(W). */
  matr[0] = d[0]*(x[1][0] - x[0][0]);
  matr[1] = d[1]*(x[2][0] - x[0][0]);
  matr[2] = d[2]*(x[3][0] - x[0][0]);

  matr[3] = d[0]*(x[1][1] - x[0][1]);
  matr[4] = d[1]*(x[2][1] - x[0][1]);
  matr[5] = d[2]*(x[3][1] - x[0][1]);

  matr[6] = d[0]*(x[1][2] - x[0][2]);
  matr[7] = d[1]*(x[2][2] - x[0][2]);
  matr[8] = d[2]*(x[3][2] - x[0][2]);

  /* Calculate det(M). */
  loc1 = matr[4]*matr[8] - matr[5]*matr[7];
  loc2 = matr[5]*matr[6] - matr[3]*matr[8];
  loc3 = matr[3]*matr[7] - matr[4]*matr[6];
  g = matr[0]*loc1 + matr[1]*loc2 + matr[2]*loc3;
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] + matr[2]*matr[2] + 
      matr[3]*matr[3] + matr[4]*matr[4] + matr[5]*matr[5] +
      matr[6]*matr[6] + matr[7]*matr[7] + matr[8]*matr[8];
 
  /* Calculate objective function. */
  obj  = a * pow(f, b) * pow(g, c);

  /* Calculate the derivative of the objective function.    */
  f = b * obj / f;		/* Constant on nabla f      */
  g = c * obj / g;              /* Constant on nable g      */
  f *= 2.0;                     /* Modification for nabla f */

  adj_m[0] = d[0]*(matr[0]*f + loc1*g);
  adj_m[1] = d[1]*(matr[1]*f + loc2*g);
  adj_m[2] = d[2]*(matr[2]*f + loc3*g);

  loc1 = matr[0]*g;
  loc2 = matr[1]*g;
  loc3 = matr[2]*g;

  adj_m[3] = d[0]*(matr[3]*f + loc3*matr[7] - loc2*matr[8]);
  adj_m[4] = d[1]*(matr[4]*f + loc1*matr[8] - loc3*matr[6]);
  adj_m[5] = d[2]*(matr[5]*f + loc2*matr[6] - loc1*matr[7]);

  adj_m[6] = d[0]*(matr[6]*f + loc2*matr[5] - loc3*matr[4]);
  adj_m[7] = d[1]*(matr[7]*f + loc3*matr[3] - loc1*matr[5]);
  adj_m[8] = d[2]*(matr[8]*f + loc1*matr[4] - loc2*matr[3]);

  g_obj[0][0] = -adj_m[0] - adj_m[1] - adj_m[2];
  g_obj[1][0] =  adj_m[0];
  g_obj[2][0] =  adj_m[1];
  g_obj[3][0] =  adj_m[2];

  g_obj[0][1] = -adj_m[3] - adj_m[4] - adj_m[5];
  g_obj[1][1] =  adj_m[3];
  g_obj[2][1] =  adj_m[4];
  g_obj[3][1] =  adj_m[5];

  g_obj[0][2] = -adj_m[6] - adj_m[7] - adj_m[8];
  g_obj[1][2] =  adj_m[6];
  g_obj[2][2] =  adj_m[7];
  g_obj[3][2] =  adj_m[8];
  return true;
}

/*****************************************************************************/
/* Hessian evaluation requires 469 flops.                                    */
/*   Reductions possible when b == 1, c == 1, or d == 1                      */
/*****************************************************************************/
inline int h_fcn_3i(double &obj, Vector3D g_obj[4], Matrix3D h_obj[10], 
		    const Vector3D x[4], 
		    const double a, const double b, const double c,
		    const Vector3D &d)
{
  double matr[9], f;
  double adj_m[9], g;
  double dg[9], loc0, loc1, loc2, loc3, loc4;
  double A[3], J_A[6], J_B[9], J_C[9], cross;

  const double scale[6] = {
    d[0]*d[0], d[0]*d[1], d[0]*d[2],
    d[1]*d[1], d[1]*d[2],
    d[2]*d[2]
  };

  /* Calculate M = A*inv(W). */
  matr[0] = d[0]*(x[1][0] - x[0][0]);
  matr[1] = d[1]*(x[2][0] - x[0][0]);
  matr[2] = d[2]*(x[3][0] - x[0][0]);

  matr[3] = d[0]*(x[1][1] - x[0][1]);
  matr[4] = d[1]*(x[2][1] - x[0][1]);
  matr[5] = d[2]*(x[3][1] - x[0][1]);

  matr[6] = d[0]*(x[1][2] - x[0][2]);
  matr[7] = d[1]*(x[2][2] - x[0][2]);
  matr[8] = d[2]*(x[3][2] - x[0][2]);

  /* Calculate det(M). */
  dg[0] = matr[4]*matr[8] - matr[5]*matr[7];
  dg[1] = matr[5]*matr[6] - matr[3]*matr[8];
  dg[2] = matr[3]*matr[7] - matr[4]*matr[6];
  g = matr[0]*dg[0] + matr[1]*dg[1] + matr[2]*dg[2];
  if (g < MSQ_MIN) { obj = g; return false; }

  /* Calculate norm(M). */
  f = matr[0]*matr[0] + matr[1]*matr[1] + matr[2]*matr[2] + 
    matr[3]*matr[3] + matr[4]*matr[4] + matr[5]*matr[5] +
    matr[6]*matr[6] + matr[7]*matr[7] + matr[8]*matr[8];

  loc3 = f;
  loc4 = g;

  /* Calculate objective function. */
  obj  = a * pow(f, b) * pow(g, c);

  /* Calculate the derivative of the objective function.    */
  f = b * obj / f;		/* Constant on nabla f      */
  g = c * obj / g;              /* Constant on nable g      */
  f *= 2.0;                     /* Modification for nabla f */

  dg[3] = matr[2]*matr[7] - matr[1]*matr[8];
  dg[4] = matr[0]*matr[8] - matr[2]*matr[6];
  dg[5] = matr[1]*matr[6] - matr[0]*matr[7];
  dg[6] = matr[1]*matr[5] - matr[2]*matr[4];
  dg[7] = matr[2]*matr[3] - matr[0]*matr[5];
  dg[8] = matr[0]*matr[4] - matr[1]*matr[3];

  adj_m[0] = d[0]*(matr[0]*f + dg[0]*g);
  adj_m[1] = d[1]*(matr[1]*f + dg[1]*g);
  adj_m[2] = d[2]*(matr[2]*f + dg[2]*g);
  adj_m[3] = d[0]*(matr[3]*f + dg[3]*g);
  adj_m[4] = d[1]*(matr[4]*f + dg[4]*g);
  adj_m[5] = d[2]*(matr[5]*f + dg[5]*g);
  adj_m[6] = d[0]*(matr[6]*f + dg[6]*g);
  adj_m[7] = d[1]*(matr[7]*f + dg[7]*g);
  adj_m[8] = d[2]*(matr[8]*f + dg[8]*g);

  g_obj[0][0] = -adj_m[0] - adj_m[1] - adj_m[2];
  g_obj[1][0] =  adj_m[0];
  g_obj[2][0] =  adj_m[1];
  g_obj[3][0] =  adj_m[2];

  g_obj[0][1] = -adj_m[3] - adj_m[4] - adj_m[5];
  g_obj[1][1] =  adj_m[3];
  g_obj[2][1] =  adj_m[4];
  g_obj[3][1] =  adj_m[5];

  g_obj[0][2] = -adj_m[6] - adj_m[7] - adj_m[8];
  g_obj[1][2] =  adj_m[6];
  g_obj[2][2] =  adj_m[7];
  g_obj[3][2] =  adj_m[8];

  /* Calculate the hessian of the objective.                   */
  loc0 = f;			/* Constant on nabla^2 f       */
  loc1 = g;			/* Constant on nabla^2 g       */
  cross = f * c / loc4;		/* Constant on nabla g nabla f */
  f = f * (b-1) / loc3;		/* Constant on nabla f nabla f */
  g = g * (c-1) / loc4;		/* Constant on nabla g nabla g */
  f *= 2.0;                     /* Modification for nabla f    */

  /* First block of rows */
  loc3 = matr[0]*f + dg[0]*cross;
  loc4 = dg[0]*g + matr[0]*cross;

  J_A[0] = loc0 + loc3*matr[0] + loc4*dg[0];
  J_A[1] = loc3*matr[1] + loc4*dg[1];
  J_A[2] = loc3*matr[2] + loc4*dg[2];
  J_B[0] = loc3*matr[3] + loc4*dg[3];
  J_B[1] = loc3*matr[4] + loc4*dg[4];
  J_B[2] = loc3*matr[5] + loc4*dg[5];
  J_C[0] = loc3*matr[6] + loc4*dg[6];
  J_C[1] = loc3*matr[7] + loc4*dg[7];
  J_C[2] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[1]*f + dg[1]*cross;
  loc4 = dg[1]*g + matr[1]*cross;

  J_A[3] = loc0 + loc3*matr[1] + loc4*dg[1];
  J_A[4] = loc3*matr[2] + loc4*dg[2];
  J_B[3] = loc3*matr[3] + loc4*dg[3];
  J_B[4] = loc3*matr[4] + loc4*dg[4];
  J_B[5] = loc3*matr[5] + loc4*dg[5];
  J_C[3] = loc3*matr[6] + loc4*dg[6];
  J_C[4] = loc3*matr[7] + loc4*dg[7];
  J_C[5] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[2]*f + dg[2]*cross;
  loc4 = dg[2]*g + matr[2]*cross;

  J_A[5] = loc0 + loc3*matr[2] + loc4*dg[2];
  J_B[6] = loc3*matr[3] + loc4*dg[3];
  J_B[7] = loc3*matr[4] + loc4*dg[4];
  J_B[8] = loc3*matr[5] + loc4*dg[5];
  J_C[6] = loc3*matr[6] + loc4*dg[6];
  J_C[7] = loc3*matr[7] + loc4*dg[7];
  J_C[8] = loc3*matr[8] + loc4*dg[8];

  /* First diagonal block */
  J_A[0] *= scale[0];
  J_A[1] *= scale[1];
  J_A[2] *= scale[2];
  J_A[3] *= scale[3];
  J_A[4] *= scale[4];
  J_A[5] *= scale[5];

  A[0] = -J_A[0] - J_A[1] - J_A[2];
  A[1] = -J_A[1] - J_A[3] - J_A[4];
  A[2] = -J_A[2] - J_A[4] - J_A[5];

  h_obj[0][0][0] = -A[0] - A[1] - A[2];
  h_obj[1][0][0] =  A[0];
  h_obj[2][0][0] =  A[1];
  h_obj[3][0][0] =  A[2];

  h_obj[4][0][0] = J_A[0];
  h_obj[5][0][0] = J_A[1];
  h_obj[6][0][0] = J_A[2];

  h_obj[7][0][0] = J_A[3];
  h_obj[8][0][0] = J_A[4];

  h_obj[9][0][0] = J_A[5];

  /* First off-diagonal block */
  loc2 = matr[8]*loc1;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  loc2 = matr[7]*loc1;
  J_B[2] -= loc2;
  J_B[6] += loc2;

  loc2 = matr[6]*loc1;
  J_B[5] += loc2;
  J_B[7] -= loc2;

  J_B[0] *= scale[0];
  J_B[1] *= scale[1];
  J_B[2] *= scale[2];
  J_B[3] *= scale[1];
  J_B[4] *= scale[3];
  J_B[5] *= scale[4];
  J_B[6] *= scale[2];
  J_B[7] *= scale[4];
  J_B[8] *= scale[5];

  A[0] = -J_B[0] - J_B[3] - J_B[6];
  A[1] = -J_B[1] - J_B[4] - J_B[7];
  A[2] = -J_B[2] - J_B[5] - J_B[8];

  h_obj[0][0][1] = -A[0] - A[1] - A[2];
  h_obj[1][0][1] =  A[0];
  h_obj[2][0][1] =  A[1];
  h_obj[3][0][1] =  A[2];

  h_obj[1][1][0] = -J_B[0] - J_B[1] - J_B[2];
  h_obj[4][0][1] =  J_B[0];
  h_obj[5][0][1] =  J_B[1];
  h_obj[6][0][1] =  J_B[2];

  h_obj[2][1][0] = -J_B[3] - J_B[4] - J_B[5];
  h_obj[5][1][0] =  J_B[3];
  h_obj[7][0][1] =  J_B[4];
  h_obj[8][0][1] =  J_B[5];

  h_obj[3][1][0] = -J_B[6] - J_B[7] - J_B[8];
  h_obj[6][1][0] =  J_B[6];
  h_obj[8][1][0] =  J_B[7];
  h_obj[9][0][1] =  J_B[8];

  /* Second off-diagonal block */
  loc2 = matr[5]*loc1;
  J_C[1] -= loc2;
  J_C[3] += loc2;

  loc2 = matr[4]*loc1;
  J_C[2] += loc2;
  J_C[6] -= loc2;

  loc2 = matr[3]*loc1;
  J_C[5] -= loc2;
  J_C[7] += loc2;

  J_C[0] *= scale[0];
  J_C[1] *= scale[1];
  J_C[2] *= scale[2];
  J_C[3] *= scale[1];
  J_C[4] *= scale[3];
  J_C[5] *= scale[4];
  J_C[6] *= scale[2];
  J_C[7] *= scale[4];
  J_C[8] *= scale[5];

  A[0] = -J_C[0] - J_C[3] - J_C[6];
  A[1] = -J_C[1] - J_C[4] - J_C[7];
  A[2] = -J_C[2] - J_C[5] - J_C[8];

  h_obj[0][0][2] = -A[0] - A[1] - A[2];
  h_obj[1][0][2] =  A[0];
  h_obj[2][0][2] =  A[1];
  h_obj[3][0][2] =  A[2];

  h_obj[1][2][0] = -J_C[0] - J_C[1] - J_C[2];
  h_obj[4][0][2] =  J_C[0];
  h_obj[5][0][2] =  J_C[1];
  h_obj[6][0][2] =  J_C[2];

  h_obj[2][2][0] = -J_C[3] - J_C[4] - J_C[5];
  h_obj[5][2][0] =  J_C[3];
  h_obj[7][0][2] =  J_C[4];
  h_obj[8][0][2] =  J_C[5];

  h_obj[3][2][0] = -J_C[6] - J_C[7] - J_C[8];
  h_obj[6][2][0] =  J_C[6];
  h_obj[8][2][0] =  J_C[7];
  h_obj[9][0][2] =  J_C[8];

  /* Second block of rows */
  loc3 = matr[3]*f + dg[3]*cross;
  loc4 = dg[3]*g + matr[3]*cross;

  J_A[0] = loc0 + loc3*matr[3] + loc4*dg[3];
  J_A[1] = loc3*matr[4] + loc4*dg[4];
  J_A[2] = loc3*matr[5] + loc4*dg[5];
  J_B[0] = loc3*matr[6] + loc4*dg[6];
  J_B[1] = loc3*matr[7] + loc4*dg[7];
  J_B[2] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[4]*f + dg[4]*cross;
  loc4 = dg[4]*g + matr[4]*cross;

  J_A[3] = loc0 + loc3*matr[4] + loc4*dg[4];
  J_A[4] = loc3*matr[5] + loc4*dg[5];
  J_B[3] = loc3*matr[6] + loc4*dg[6];
  J_B[4] = loc3*matr[7] + loc4*dg[7];
  J_B[5] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[5]*f + dg[5]*cross;
  loc4 = dg[5]*g + matr[5]*cross;

  J_A[5] = loc0 + loc3*matr[5] + loc4*dg[5];
  J_B[6] = loc3*matr[6] + loc4*dg[6];
  J_B[7] = loc3*matr[7] + loc4*dg[7];
  J_B[8] = loc3*matr[8] + loc4*dg[8];

  /* Second diagonal block */
  J_A[0] *= scale[0];
  J_A[1] *= scale[1];
  J_A[2] *= scale[2];
  J_A[3] *= scale[3];
  J_A[4] *= scale[4];
  J_A[5] *= scale[5];

  A[0] = -J_A[0] - J_A[1] - J_A[2];
  A[1] = -J_A[1] - J_A[3] - J_A[4];
  A[2] = -J_A[2] - J_A[4] - J_A[5];

  h_obj[0][1][1] = -A[0] - A[1] - A[2];
  h_obj[1][1][1] =  A[0];
  h_obj[2][1][1] =  A[1];
  h_obj[3][1][1] =  A[2];

  h_obj[4][1][1] = J_A[0];
  h_obj[5][1][1] = J_A[1];
  h_obj[6][1][1] = J_A[2];

  h_obj[7][1][1] = J_A[3];
  h_obj[8][1][1] = J_A[4];

  h_obj[9][1][1] = J_A[5];

  /* Third off-diagonal block */
  loc2 = matr[2]*loc1;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  loc2 = matr[1]*loc1;
  J_B[2] -= loc2;
  J_B[6] += loc2;

  loc2 = matr[0]*loc1;
  J_B[5] += loc2;
  J_B[7] -= loc2;

  J_B[0] *= scale[0];
  J_B[1] *= scale[1];
  J_B[2] *= scale[2];
  J_B[3] *= scale[1];
  J_B[4] *= scale[3];
  J_B[5] *= scale[4];
  J_B[6] *= scale[2];
  J_B[7] *= scale[4];
  J_B[8] *= scale[5];

  A[0] = -J_B[0] - J_B[3] - J_B[6];
  A[1] = -J_B[1] - J_B[4] - J_B[7];
  A[2] = -J_B[2] - J_B[5] - J_B[8];

  h_obj[0][1][2] = -A[0] - A[1] - A[2];
  h_obj[1][1][2] =  A[0];
  h_obj[2][1][2] =  A[1];
  h_obj[3][1][2] =  A[2];

  h_obj[1][2][1] = -J_B[0] - J_B[1] - J_B[2];
  h_obj[4][1][2] =  J_B[0];
  h_obj[5][1][2] =  J_B[1];
  h_obj[6][1][2] =  J_B[2];

  h_obj[2][2][1] = -J_B[3] - J_B[4] - J_B[5];
  h_obj[5][2][1] =  J_B[3];
  h_obj[7][1][2] =  J_B[4];
  h_obj[8][1][2] =  J_B[5];

  h_obj[3][2][1] = -J_B[6] - J_B[7] - J_B[8];
  h_obj[6][2][1] =  J_B[6];
  h_obj[8][2][1] =  J_B[7];
  h_obj[9][1][2] =  J_B[8];

  /* Third block of rows */
  loc3 = matr[6]*f + dg[6]*cross;
  loc4 = dg[6]*g + matr[6]*cross;

  J_A[0] = loc0 + loc3*matr[6] + loc4*dg[6];
  J_A[1] = loc3*matr[7] + loc4*dg[7];
  J_A[2] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[7]*f + dg[7]*cross;
  loc4 = dg[7]*g + matr[7]*cross;

  J_A[3] = loc0 + loc3*matr[7] + loc4*dg[7];
  J_A[4] = loc3*matr[8] + loc4*dg[8];

  loc3 = matr[8]*f + dg[8]*cross;
  loc4 = dg[8]*g + matr[8]*cross;

  J_A[5] = loc0 + loc3*matr[8] + loc4*dg[8];

  /* Third diagonal block */
  J_A[0] *= scale[0];
  J_A[1] *= scale[1];
  J_A[2] *= scale[2];
  J_A[3] *= scale[3];
  J_A[4] *= scale[4];
  J_A[5] *= scale[5];

  A[0] = -J_A[0] - J_A[1] - J_A[2];
  A[1] = -J_A[1] - J_A[3] - J_A[4];
  A[2] = -J_A[2] - J_A[4] - J_A[5];

  h_obj[0][2][2] = -A[0] - A[1] - A[2];
  h_obj[1][2][2] =  A[0];
  h_obj[2][2][2] =  A[1];
  h_obj[3][2][2] =  A[2];

  h_obj[4][2][2] = J_A[0];
  h_obj[5][2][2] = J_A[1];
  h_obj[6][2][2] = J_A[2];

  h_obj[7][2][2] = J_A[3];
  h_obj[8][2][2] = J_A[4];

  h_obj[9][2][2] = J_A[5];

  // completes diagonal blocks.
  h_obj[0].fill_lower_triangle();
  h_obj[4].fill_lower_triangle();
  h_obj[7].fill_lower_triangle();
  h_obj[9].fill_lower_triangle();
  return true;
}

}

#endif
