/*!
  \file   MeanRatioQualityMetric.cpp
  \brief  

  \author Michael Brewer
  \date   2002-06-9
*/
#include <vector>
#include "MeanRatioQualityMetric.hpp"
#include <math.h>
#include "Vector3D.hpp"
#include "ShapeQualityMetric.hpp"
#include "QualityMetric.hpp"

using namespace Mesquite;

#undef __FUNC__
#define __FUNC__ "MeanRatioQualityMetric::MeanRatioQualityMetric"

MeanRatioQualityMetric::MeanRatioQualityMetric()
{
  MsqError err;
  set_metric_type(ELEMENT_BASED);
  set_element_evaluation_mode(ELEMENT_VERTICES, err); MSQ_CHKERR(err);
  set_negate_flag(1);
  avgMethod=QualityMetric::LINEAR;
  feasible=1;
  set_name("Mean Ratio");
}

bool MeanRatioQualityMetric::evaluate_element(PatchData &pd,
                                              MsqMeshEntity *element,
                                              double &fval,
                                              MsqError &err)
{
  double metric_values[20];
  fval=0.0;
  bool return_flag;
  std::vector<size_t> v_i;
  element->get_vertex_indices(v_i);
  //only 3 temp_vec will be sent to mean ratio calculator, but the
  //additional vector3D may be needed during the calculations
  Vector3D temp_vec[5];
  MsqVertex *vertices=pd.get_vertex_array(err);
  switch(element->get_element_type()){
    case TRIANGLE:
      temp_vec[0]=vertices[v_i[1]]-vertices[v_i[0]];
      temp_vec[2]=vertices[v_i[2]]-vertices[v_i[0]];
        //make relative to equilateral
      temp_vec[1]=((2*temp_vec[2])-temp_vec[0])*MSQ_SQRT_THREE_INV;
      return_flag=mean_ratio_2d(temp_vec,fval,err);
      return return_flag;
      break;
    case QUADRILATERAL:
      temp_vec[0]=vertices[v_i[1]]-vertices[v_i[0]];
      temp_vec[1]=vertices[v_i[3]]-vertices[v_i[0]];
      return_flag=mean_ratio_2d(temp_vec,metric_values[0],err);
      if(!return_flag)
        return false;
      
      temp_vec[0]=vertices[v_i[2]]-vertices[v_i[1]];
      temp_vec[1]=vertices[v_i[0]]-vertices[v_i[1]];
      return_flag=mean_ratio_2d(temp_vec,metric_values[1],err);
      if(!return_flag)
        return false;
      temp_vec[0]=vertices[v_i[3]]-vertices[v_i[2]];
      temp_vec[1]=vertices[v_i[1]]-vertices[v_i[2]];
      return_flag=mean_ratio_2d(temp_vec,metric_values[2],err);
      if(!return_flag)
        return false;
      temp_vec[0]=vertices[v_i[0]]-vertices[v_i[3]];
      temp_vec[1]=vertices[v_i[2]]-vertices[v_i[3]];
      return_flag=mean_ratio_2d(temp_vec,metric_values[3],err);
      if(!return_flag)
        return false;
      fval=average_metrics(metric_values,4,err);
      break;
    case TETRAHEDRON:
      temp_vec[0]=vertices[v_i[1]]-vertices[v_i[0]];
      temp_vec[3]=vertices[v_i[2]]-vertices[v_i[0]];
      temp_vec[4]=vertices[v_i[3]]-vertices[v_i[0]];
        //transform to equilateral tet
      temp_vec[1]=((2*temp_vec[3])-temp_vec[0])*MSQ_SQRT_THREE_INV;
      temp_vec[2]=((3*temp_vec[4])-temp_vec[0]-temp_vec[3])*
        (MSQ_SQRT_THREE_INV*MSQ_SQRT_TWO_INV);
      return_flag=mean_ratio_3d(temp_vec,fval,err);
      break;
    case HEXAHEDRON:
      temp_vec[0]=vertices[v_i[1]]-vertices[v_i[0]];
      temp_vec[1]=vertices[v_i[3]]-vertices[v_i[0]];
      temp_vec[2]=vertices[v_i[4]]-vertices[v_i[0]];
      return_flag=mean_ratio_3d(temp_vec,metric_values[0],err);
      if(!return_flag)
        return false;
      temp_vec[0]=vertices[v_i[2]]-vertices[v_i[1]];
      temp_vec[1]=vertices[v_i[0]]-vertices[v_i[1]];
      temp_vec[2]=vertices[v_i[5]]-vertices[v_i[1]];
      return_flag=mean_ratio_3d(temp_vec,metric_values[1],err);
      if(!return_flag)
        return false;
      temp_vec[0]=vertices[v_i[3]]-vertices[v_i[2]];
      temp_vec[1]=vertices[v_i[1]]-vertices[v_i[2]];
      temp_vec[2]=vertices[v_i[6]]-vertices[v_i[2]];
      return_flag=mean_ratio_3d(temp_vec,metric_values[2],err);
      if(!return_flag)
        return false;
      temp_vec[0]=vertices[v_i[0]]-vertices[v_i[3]];
      temp_vec[1]=vertices[v_i[2]]-vertices[v_i[3]];
      temp_vec[2]=vertices[v_i[7]]-vertices[v_i[3]];
      return_flag=mean_ratio_3d(temp_vec,metric_values[3],err);
      if(!return_flag)
        return false;
      temp_vec[0]=vertices[v_i[7]]-vertices[v_i[4]];
      temp_vec[1]=vertices[v_i[5]]-vertices[v_i[4]];
      temp_vec[2]=vertices[v_i[0]]-vertices[v_i[4]];
      return_flag=mean_ratio_3d(temp_vec,metric_values[4],err);
      if(!return_flag)
        return false;
      temp_vec[0]=vertices[v_i[4]]-vertices[v_i[5]];
      temp_vec[1]=vertices[v_i[6]]-vertices[v_i[5]];
      temp_vec[2]=vertices[v_i[1]]-vertices[v_i[5]];
      return_flag=mean_ratio_3d(temp_vec,metric_values[5],err);
      if(!return_flag)
        return false;
      temp_vec[0]=vertices[v_i[5]]-vertices[v_i[6]];
      temp_vec[1]=vertices[v_i[7]]-vertices[v_i[6]];
      temp_vec[2]=vertices[v_i[2]]-vertices[v_i[6]];
      return_flag=mean_ratio_3d(temp_vec,metric_values[6],err);
      if(!return_flag)
        return false;
      temp_vec[0]=vertices[v_i[6]]-vertices[v_i[7]];
      temp_vec[1]=vertices[v_i[4]]-vertices[v_i[7]];
      temp_vec[2]=vertices[v_i[3]]-vertices[v_i[7]];
      return_flag=mean_ratio_3d(temp_vec,metric_values[7],err);
      if(!return_flag)
        return false;
      fval=average_metrics(metric_values,8,err);
      
      break;
    default:
      fval=0.0;
  }// end switch over element type
  return true;
}

/*****************************************************************************/
/* This set of functions reference tetrahedral elements to a regular         */
/* tetrahedron.  The input are the coordinates in the following order:       */
/*      [x1 x2 x3 x4 y1 y2 y3 y4 z1 z2 z3 z4]                                */
/* A zero return value indicates success, while a nonzero value indicates    */
/* failure.                                                                  */
/*****************************************************************************/
/* Not all compilers substitute out constants (especially the square root).  */
/* Therefore, they are substituted out manually.  The values below were      */
/* calculated on a solaris machine using long doubles. I believe they are    */
/* accurate.                                                                 */
/*****************************************************************************/

#define isqrt3   5.77350269189625797959429519858e-01        /*  1.0/sqrt(3.0) */
#define tisqrt3  1.15470053837925159591885903972e+00        /*  2.0/sqrt(3.0) */
#define isqrt6   4.08248290463863052509822647505e-01        /*  1.0/sqrt(6.0) */
#define tisqrt6  1.22474487139158915752946794252e+00        /*  3.0/sqrt(6.0) */
#define a        3.33333333333333333333333333333e-01        /*  1.0/3.0       */
#define b       -6.66666666666666666666666666667e-01        /* -2.0/3.0       */
#define bm1     -1.66666666666666666666666666667e-00        /* -5.0/3.0       */

inline bool m_fcn_3e(double &obj, const Vector3D x[4])
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
  obj = a * f * pow(g, b);
  return true;
}

/*****************************************************************************/
/* Optimal derivative calculation courtesy of Paul Hovland (at least we      */
/* think it is optimal).  The original code provided was modified to         */
/* reduce the number of flops and intermediate variables, and improve the    */
/* locality of reference.                                                    */
/*                                                                           */
/* This requires 130 flops.  The function only requires 61 flops.            */
/*****************************************************************************/

inline bool g_fcn_3e(double &obj, Vector3D g_obj[4], const Vector3D x[4])
{
  double matr[9], f;
  double adj_m[9], g;
  double loc1, loc2, loc3, loc4;

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
  loc4 = a * pow(g, b);
  obj  = f * loc4;

  /* Calculate the derivative of the objective function. */
  f = 2.0*loc4;
  g = b*obj/g;

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
  return 0;
}

/*****************************************************************************/
/* The Hessian calculation is done by blocks.  Only the upper triangular     */
/* blocks are stored.  The results in the data is in the following order:    */
/*    [d1 b1 b2 d2 b3 d3 ]                                                   */
/* The matrices on the diagonal (d1-d3) each contain 10 elements, while the  */
/* off-diagonal elements (b1-b3) each contain 16 elements.                   */
/*                                                                           */
/* The code requires 598 flops.  The gradient evaluation needs 130 flops     */
/* and the function requires 61 flops.                                       */
/*****************************************************************************/
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

inline bool h_fcn_3e(double &obj, Vector3D g_obj[4], double h_obj[78], 
		     const Vector3D x[4])
{
  double matr[9], f;
  double adj_m[9], g;
  double dg[9], loc0, loc1, loc2, loc3, loc4;
  double A[12], J_A[6], J_B[9], J_C[9];

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

  loc4 = g;

  /* Calculate objective function. */
  loc1 = a * pow(g, b);
  obj  = f * loc1;

  /* Calculate the derivative of the objective function. */
  f = 2.0*loc1;
  g = b*obj/g; 

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

  loc0 = g;
  loc1 = f;
  f = f*b/loc4;
  g = g*bm1/loc4;

  /* First block of rows */
  loc2 = matr[0]*f;
  loc3 = dg[0]*f;
  loc4 = dg[0]*g + loc2;

  J_A[0] = loc1 + dg[0]*(loc2 + loc4);
  J_A[1] = loc3*matr[1] + loc4*dg[1];
  J_A[2] = loc3*matr[2] + loc4*dg[2];
  J_B[0] = loc3*matr[3] + loc4*dg[3];
  J_B[1] = loc3*matr[4] + loc4*dg[4];
  J_B[2] = loc3*matr[5] + loc4*dg[5];
  J_C[0] = loc3*matr[6] + loc4*dg[6];
  J_C[1] = loc3*matr[7] + loc4*dg[7];
  J_C[2] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[1]*f;
  loc3 = dg[1]*f;
  loc4 = dg[1]*g + loc2;

  J_A[3] = loc1 + dg[1]*(loc2 + loc4);
  J_A[4] = loc3*matr[2] + loc4*dg[2];
  J_B[3] = loc3*matr[3] + loc4*dg[3];
  J_B[4] = loc3*matr[4] + loc4*dg[4];
  J_B[5] = loc3*matr[5] + loc4*dg[5];
  J_C[3] = loc3*matr[6] + loc4*dg[6];
  J_C[4] = loc3*matr[7] + loc4*dg[7];
  J_C[5] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[2]*f;
  loc3 = dg[2]*f;
  loc4 = dg[2]*g + loc2;

  J_A[5] = loc1 + dg[2]*(loc2 + loc4);
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

  h_obj[0] = -A[0] - loc4;
  h_obj[1] =  A[0] - loc4;
  h_obj[2] = 2.0*loc2 - loc3;
  h_obj[3] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];

  h_obj[4] = A[1] - loc2 - loc3;
  h_obj[5] = 2.0*loc2 - loc3;
  h_obj[6] = 3.0*loc3;

  loc3 = isqrt6*A[10];
  h_obj[7] = tisqrt3*A[6] - loc3;
  h_obj[8] = 3.0*loc3;

  h_obj[9] = tisqrt6*A[11];

  /* First off-diagonal block */
  loc2 = matr[8]*loc0;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  loc2 = matr[7]*loc0;
  J_B[2] -= loc2;
  J_B[6] += loc2;

  loc2 = matr[6]*loc0;
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

  h_obj[10] = -A[0] - loc4;
  h_obj[11] =  A[0] - loc4;
  h_obj[12] = 2.0*loc2 - loc3;
  h_obj[13] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];
  loc4 = loc2 + loc3;

  h_obj[14] = -A[1] - loc4;
  h_obj[15] =  A[1] - loc4;
  h_obj[16] = 2.0*loc2 - loc3;
  h_obj[17] = 3.0*loc3;

  loc2 = isqrt3*A[6];
  loc3 = isqrt6*A[10];
  loc4 = loc2 + loc3;

  h_obj[18] = -A[2] - loc4;
  h_obj[19] =  A[2] - loc4;
  h_obj[20] = 2.0*loc2 - loc3;
  h_obj[21] = 3.0*loc3;

  loc2 = isqrt3*A[7];
  loc3 = isqrt6*A[11];
  loc4 = loc2 + loc3;

  h_obj[22] = -A[3] - loc4;
  h_obj[23] =  A[3] - loc4;
  h_obj[24] = 2.0*loc2 - loc3;
  h_obj[25] = 3.0*loc3;

  /* Second off-diagonal block */
  loc2 = matr[5]*loc0;
  J_C[1] -= loc2;
  J_C[3] += loc2;

  loc2 = matr[4]*loc0;
  J_C[2] += loc2;
  J_C[6] -= loc2;

  loc2 = matr[3]*loc0;
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

  h_obj[26] = -A[0] - loc4;
  h_obj[27] =  A[0] - loc4;
  h_obj[28] = 2.0*loc2 - loc3;
  h_obj[29] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];
  loc4 = loc2 + loc3;

  h_obj[30] = -A[1] - loc4;
  h_obj[31] =  A[1] - loc4;
  h_obj[32] = 2.0*loc2 - loc3;
  h_obj[33] = 3.0*loc3;

  loc2 = isqrt3*A[6];
  loc3 = isqrt6*A[10];
  loc4 = loc2 + loc3;

  h_obj[34] = -A[2] - loc4;
  h_obj[35] =  A[2] - loc4;
  h_obj[36] = 2.0*loc2 - loc3;
  h_obj[37] = 3.0*loc3;

  loc2 = isqrt3*A[7];
  loc3 = isqrt6*A[11];
  loc4 = loc2 + loc3;

  h_obj[38] = -A[3] - loc4;
  h_obj[39] =  A[3] - loc4;
  h_obj[40] = 2.0*loc2 - loc3;
  h_obj[41] = 3.0*loc3;

  /* Second block of rows */
  loc2 = matr[3]*f;
  loc3 = dg[3]*f;
  loc4 = dg[3]*g + loc2;

  J_A[0] = loc1 + dg[3]*(loc2 + loc4);
  J_A[1] = loc3*matr[4] + loc4*dg[4];
  J_A[2] = loc3*matr[5] + loc4*dg[5];
  J_B[0] = loc3*matr[6] + loc4*dg[6];
  J_B[1] = loc3*matr[7] + loc4*dg[7];
  J_B[2] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[4]*f;
  loc3 = dg[4]*f;
  loc4 = dg[4]*g + loc2;

  J_A[3] = loc1 + dg[4]*(loc2 + loc4);
  J_A[4] = loc3*matr[5] + loc4*dg[5];
  J_B[3] = loc3*matr[6] + loc4*dg[6];
  J_B[4] = loc3*matr[7] + loc4*dg[7];
  J_B[5] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[5]*f;
  loc3 = dg[5]*f;
  loc4 = dg[5]*g + loc2;

  J_A[5] = loc1 + dg[5]*(loc2 + loc4);
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

  h_obj[42] = -A[0] - loc4;
  h_obj[43] =  A[0] - loc4;
  h_obj[44] = 2.0*loc2 - loc3;
  h_obj[45] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];

  h_obj[46] = A[1] - loc2 - loc3;
  h_obj[47] = 2.0*loc2 - loc3;
  h_obj[48] = 3.0*loc3;

  loc3 = isqrt6*A[10];
  h_obj[49] = tisqrt3*A[6] - loc3;
  h_obj[50] = 3.0*loc3;

  h_obj[51] = tisqrt6*A[11];

  /* Third off-diagonal block */
  loc2 = matr[2]*loc0;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  loc2 = matr[1]*loc0;
  J_B[2] -= loc2;
  J_B[6] += loc2;

  loc2 = matr[0]*loc0;
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

  h_obj[52] = -A[0] - loc4;
  h_obj[53] =  A[0] - loc4;
  h_obj[54] = 2.0*loc2 - loc3;
  h_obj[55] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];
  loc4 = loc2 + loc3;

  h_obj[56] = -A[1] - loc4;
  h_obj[57] =  A[1] - loc4;
  h_obj[58] = 2.0*loc2 - loc3;
  h_obj[59] = 3.0*loc3;

  loc2 = isqrt3*A[6];
  loc3 = isqrt6*A[10];
  loc4 = loc2 + loc3;

  h_obj[60] = -A[2] - loc4;
  h_obj[61] =  A[2] - loc4;
  h_obj[62] = 2.0*loc2 - loc3;
  h_obj[63] = 3.0*loc3;

  loc2 = isqrt3*A[7];
  loc3 = isqrt6*A[11];
  loc4 = loc2 + loc3;

  h_obj[64] = -A[3] - loc4;
  h_obj[65] =  A[3] - loc4;
  h_obj[66] = 2.0*loc2 - loc3;
  h_obj[67] = 3.0*loc3;

  /* Third block of rows */
  loc2 = matr[6]*f;
  loc3 = dg[6]*f;
  loc4 = dg[6]*g + loc2;

  J_A[0] = loc1 + dg[6]*(loc2 + loc4);
  J_A[1] = loc3*matr[7] + loc4*dg[7];
  J_A[2] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[7]*f;
  loc3 = dg[7]*f;
  loc4 = dg[7]*g + loc2;

  J_A[3] = loc1 + dg[7]*(loc2 + loc4);
  J_A[4] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[8]*f;
  loc4 = dg[8]*g + loc2;

  J_A[5] = loc1 + dg[8]*(loc2 + loc4);

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

  h_obj[68] = -A[0] - loc4;
  h_obj[69] =  A[0] - loc4;
  h_obj[70] = 2.0*loc2 - loc3;
  h_obj[71] = 3.0*loc3;

  loc2 = isqrt3*A[5];
  loc3 = isqrt6*A[9];

  h_obj[72] = A[1] - loc2 - loc3;
  h_obj[73] = 2.0*loc2 - loc3;
  h_obj[74] = 3.0*loc3;

  loc3 = isqrt6*A[10];
  h_obj[75] = tisqrt3*A[6] - loc3;
  h_obj[76] = 3.0*loc3;

  h_obj[77] = tisqrt6*A[11];
  return true;
}

/*****************************************************************************/
/* This set of functions reference tetrahedral elements to a right           */
/* tetrahedron.  The input are the coordinates in the following order:       */
/*      [x1 x2 x3 x4 y1 y2 y3 y4 z1 z2 z3 z4]                                */
/* A zero return value indicates success, while a nonzero value indicates    */
/* failure.                                                                  */
/*****************************************************************************/
/* Not all compilers substitute out constants (especially the square root).  */
/* Therefore, they are substituted out manually.  The values below were      */
/* calculated on a solaris machine using long doubles. I believe they are    */
/* accurate.                                                                 */
/*****************************************************************************/

inline bool m_fcn_3i(double &obj, const Vector3D x[4])
{
  double matr[9], f;
  double g;

  /* Calculate M = A*inv(W). */
  matr[0] = x[1][0] - x[0][0];
  matr[1] = x[2][0] - x[0][0];
  matr[2] = x[3][0] - x[0][0];

  matr[3] = x[1][1] - x[0][1];
  matr[4] = x[2][1] - x[0][1];
  matr[5] = x[3][1] - x[0][1];

  matr[6] = x[1][2] - x[0][2];
  matr[7] = x[2][2] - x[0][2];
  matr[8] = x[3][2] - x[0][2];

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
  obj = a * f * pow(g, b);
  return 0;
}

/*****************************************************************************/
/* Optimal derivative calculation courtesy of Paul Hovland (at least we      */
/* think it is optimal).  The original code provided was modified to         */
/* reduce the number of flops and intermediate variables, and improve the    */
/* locality of reference.                                                    */
/*                                                                           */
/* This requires 94 flops.  The function only requires 43 flops.             */
/*****************************************************************************/

inline bool g_fcn_3i(double &obj, Vector3D g_obj[4], const Vector3D x[12])
{
  double matr[9], f;
  double adj_m[9], g;
  double loc1, loc2, loc3, loc4;

  /* Calculate M = A*inv(W). */
  matr[0] = x[1][0] - x[0][0];
  matr[1] = x[2][0] - x[0][0];
  matr[2] = x[3][0] - x[0][0];

  matr[3] = x[1][1] - x[0][1];
  matr[4] = x[2][1] - x[0][1];
  matr[5] = x[3][1] - x[0][1];

  matr[6] = x[1][2] - x[0][2];
  matr[7] = x[2][2] - x[0][2];
  matr[8] = x[3][2] - x[0][2];

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
  loc4 = a * pow(g, b);
  obj  = f * loc4;

  /* Calculate the derivative of the objective function. */
  f = 2.0*loc4;
  g = b*obj/g; 

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
/* The Hessian calculation is done by blocks.  Only the upper triangular     */
/* blocks are stored.  The results in the data is in the following order:    */
/*    [d1 b1 b2 d2 b3 d3 ]                                                   */
/* The matrices on the diagonal (d1-d3) each contain 10 elements, while the  */
/* off-diagonal elements (b1-b3) each contain 16 elements.                   */
/*                                                                           */
/* The code requires 364 flops.  The gradient evaluation needs 94 flops      */
/* and the function requires 43 flops.                                       */
/*****************************************************************************/

inline bool h_fcn_3i(double &obj, Vector3D g_obj[4], double h_obj[78], 
		     const Vector3D x[4])
{
  double matr[9], f;
  double adj_m[9], g;
  double dg[9], loc0, loc1, loc2, loc3, loc4;
  double A[3], J_A[6], J_B[9], J_C[9];

  /* Calculate M = A*inv(W). */
  matr[0] = x[1][0] - x[0][0];
  matr[1] = x[2][0] - x[0][0];
  matr[2] = x[3][0] - x[0][0];

  matr[3] = x[1][1] - x[0][1];
  matr[4] = x[2][1] - x[0][1];
  matr[5] = x[3][1] - x[0][1];

  matr[6] = x[1][2] - x[0][2];
  matr[7] = x[2][2] - x[0][2];
  matr[8] = x[3][2] - x[0][2];

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

  loc4 = g;

  /* Calculate objective function. */
  loc1 = a * pow(g, b);
  obj  = f * loc1;

  /* Calculate the derivative of the objective function. */
  f = 2.0*loc1;
  g = b*obj/g; 

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

  loc0 = g;
  loc1 = f;
  f = f*b/loc4;
  g = g*bm1/loc4;

  /* First block of rows */
  loc2 = matr[0]*f;
  loc3 = dg[0]*f;
  loc4 = dg[0]*g + loc2;

  J_A[0] = loc1 + dg[0]*(loc2 + loc4);
  J_A[1] = loc3*matr[1] + loc4*dg[1];
  J_A[2] = loc3*matr[2] + loc4*dg[2];
  J_B[0] = loc3*matr[3] + loc4*dg[3];
  J_B[1] = loc3*matr[4] + loc4*dg[4];
  J_B[2] = loc3*matr[5] + loc4*dg[5];
  J_C[0] = loc3*matr[6] + loc4*dg[6];
  J_C[1] = loc3*matr[7] + loc4*dg[7];
  J_C[2] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[1]*f;
  loc3 = dg[1]*f;
  loc4 = dg[1]*g + loc2;

  J_A[3] = loc1 + dg[1]*(loc2 + loc4);
  J_A[4] = loc3*matr[2] + loc4*dg[2];
  J_B[3] = loc3*matr[3] + loc4*dg[3];
  J_B[4] = loc3*matr[4] + loc4*dg[4];
  J_B[5] = loc3*matr[5] + loc4*dg[5];
  J_C[3] = loc3*matr[6] + loc4*dg[6];
  J_C[4] = loc3*matr[7] + loc4*dg[7];
  J_C[5] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[2]*f;
  loc3 = dg[2]*f;
  loc4 = dg[2]*g + loc2;

  J_A[5] = loc1 + dg[2]*(loc2 + loc4);
  J_B[6] = loc3*matr[3] + loc4*dg[3];
  J_B[7] = loc3*matr[4] + loc4*dg[4];
  J_B[8] = loc3*matr[5] + loc4*dg[5];
  J_C[6] = loc3*matr[6] + loc4*dg[6];
  J_C[7] = loc3*matr[7] + loc4*dg[7];
  J_C[8] = loc3*matr[8] + loc4*dg[8];

  /* First diagonal block */
  A[0] = -J_A[0] - J_A[1] - J_A[2];
  A[1] = -J_A[1] - J_A[3] - J_A[4];
  A[2] = -J_A[2] - J_A[4] - J_A[5];

  h_obj[0] = -A[0] - A[1] - A[2];
  h_obj[1] =  A[0];
  h_obj[2] =  A[1];
  h_obj[3] =  A[2];

  h_obj[4] = J_A[0];
  h_obj[5] = J_A[1];
  h_obj[6] = J_A[2];

  h_obj[7] = J_A[3];
  h_obj[8] = J_A[4];

  h_obj[9] = J_A[5];

  /* First off-diagonal block */
  loc2 = matr[8]*loc0;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  loc2 = matr[7]*loc0;
  J_B[2] -= loc2;
  J_B[6] += loc2;

  loc2 = matr[6]*loc0;
  J_B[5] += loc2;
  J_B[7] -= loc2;

  A[0] = -J_B[0] - J_B[3] - J_B[6];
  A[1] = -J_B[1] - J_B[4] - J_B[7];
  A[2] = -J_B[2] - J_B[5] - J_B[8];

  h_obj[10] = -A[0] - A[1] - A[2];
  h_obj[11] =  A[0];
  h_obj[12] =  A[1];
  h_obj[13] =  A[2];

  h_obj[14] = -J_B[0] - J_B[1] - J_B[2];
  h_obj[15] =  J_B[0];
  h_obj[16] =  J_B[1];
  h_obj[17] =  J_B[2];

  h_obj[18] = -J_B[3] - J_B[4] - J_B[5];
  h_obj[19] =  J_B[3];
  h_obj[20] =  J_B[4];
  h_obj[21] =  J_B[5];

  h_obj[22] = -J_B[6] - J_B[7] - J_B[8];
  h_obj[23] =  J_B[6];
  h_obj[24] =  J_B[7];
  h_obj[25] =  J_B[8];

  /* Second off-diagonal block */
  loc2 = matr[5]*loc0;
  J_C[1] -= loc2;
  J_C[3] += loc2;

  loc2 = matr[4]*loc0;
  J_C[2] += loc2;
  J_C[6] -= loc2;

  loc2 = matr[3]*loc0;
  J_C[5] -= loc2;
  J_C[7] += loc2;

  A[0] = -J_C[0] - J_C[3] - J_C[6];
  A[1] = -J_C[1] - J_C[4] - J_C[7];
  A[2] = -J_C[2] - J_C[5] - J_C[8];

  h_obj[26] = -A[0] - A[1] - A[2];
  h_obj[27] =  A[0];
  h_obj[28] =  A[1];
  h_obj[29] =  A[2];

  h_obj[30] = -J_C[0] - J_C[1] - J_C[2];
  h_obj[31] =  J_C[0];
  h_obj[32] =  J_C[1];
  h_obj[33] =  J_C[2];

  h_obj[34] = -J_C[3] - J_C[4] - J_C[5];
  h_obj[35] =  J_C[3];
  h_obj[36] =  J_C[4];
  h_obj[37] =  J_C[5];

  h_obj[38] = -J_C[6] - J_C[7] - J_C[8];
  h_obj[39] =  J_C[6];
  h_obj[40] =  J_C[7];
  h_obj[41] =  J_C[8];

  /* Second block of rows */
  loc2 = matr[3]*f;
  loc3 = dg[3]*f;
  loc4 = dg[3]*g + loc2;

  J_A[0] = loc1 + dg[3]*(loc2 + loc4);
  J_A[1] = loc3*matr[4] + loc4*dg[4];
  J_A[2] = loc3*matr[5] + loc4*dg[5];
  J_B[0] = loc3*matr[6] + loc4*dg[6];
  J_B[1] = loc3*matr[7] + loc4*dg[7];
  J_B[2] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[4]*f;
  loc3 = dg[4]*f;
  loc4 = dg[4]*g + loc2;

  J_A[3] = loc1 + dg[4]*(loc2 + loc4);
  J_A[4] = loc3*matr[5] + loc4*dg[5];
  J_B[3] = loc3*matr[6] + loc4*dg[6];
  J_B[4] = loc3*matr[7] + loc4*dg[7];
  J_B[5] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[5]*f;
  loc3 = dg[5]*f;
  loc4 = dg[5]*g + loc2;

  J_A[5] = loc1 + dg[5]*(loc2 + loc4);
  J_B[6] = loc3*matr[6] + loc4*dg[6];
  J_B[7] = loc3*matr[7] + loc4*dg[7];
  J_B[8] = loc3*matr[8] + loc4*dg[8];

  /* Second diagonal block */
  A[0] = -J_A[0] - J_A[1] - J_A[2];
  A[1] = -J_A[1] - J_A[3] - J_A[4];
  A[2] = -J_A[2] - J_A[4] - J_A[5];

  h_obj[42] = -A[0] - A[1] - A[2];
  h_obj[43] =  A[0];
  h_obj[44] =  A[1];
  h_obj[45] =  A[2];

  h_obj[46] = J_A[0];
  h_obj[47] = J_A[1];
  h_obj[48] = J_A[2];

  h_obj[49] = J_A[3];
  h_obj[50] = J_A[4];

  h_obj[51] = J_A[5];

  /* Third off-diagonal block */
  loc2 = matr[2]*loc0;
  J_B[1] += loc2;
  J_B[3] -= loc2;

  loc2 = matr[1]*loc0;
  J_B[2] -= loc2;
  J_B[6] += loc2;

  loc2 = matr[0]*loc0;
  J_B[5] += loc2;
  J_B[7] -= loc2;

  A[0] = -J_B[0] - J_B[3] - J_B[6];
  A[1] = -J_B[1] - J_B[4] - J_B[7];
  A[2] = -J_B[2] - J_B[5] - J_B[8];

  h_obj[52] = -A[0] - A[1] - A[2];
  h_obj[53] =  A[0];
  h_obj[54] =  A[1];
  h_obj[55] =  A[2];

  h_obj[56] = -J_B[0] - J_B[1] - J_B[2];
  h_obj[57] =  J_B[0];
  h_obj[58] =  J_B[1];
  h_obj[59] =  J_B[2];

  h_obj[60] = -J_B[3] - J_B[4] - J_B[5];
  h_obj[61] =  J_B[3];
  h_obj[62] =  J_B[4];
  h_obj[63] =  J_B[5];

  h_obj[64] = -J_B[6] - J_B[7] - J_B[8];
  h_obj[65] =  J_B[6];
  h_obj[66] =  J_B[7];
  h_obj[67] =  J_B[8];

  /* Third block of rows */
  loc2 = matr[6]*f;
  loc3 = dg[6]*f;
  loc4 = dg[6]*g + loc2;

  J_A[0] = loc1 + dg[6]*(loc2 + loc4);
  J_A[1] = loc3*matr[7] + loc4*dg[7];
  J_A[2] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[7]*f;
  loc3 = dg[7]*f;
  loc4 = dg[7]*g + loc2;

  J_A[3] = loc1 + dg[7]*(loc2 + loc4);
  J_A[4] = loc3*matr[8] + loc4*dg[8];

  loc2 = matr[8]*f;
  loc4 = dg[8]*g + loc2;

  J_A[5] = loc1 + dg[8]*(loc2 + loc4);

  /* Third diagonal block */
  A[0] = -J_A[0] - J_A[1] - J_A[2];
  A[1] = -J_A[1] - J_A[3] - J_A[4];
  A[2] = -J_A[2] - J_A[4] - J_A[5];

  h_obj[68] = -A[0] - A[1] - A[2];
  h_obj[69] =  A[0];
  h_obj[70] =  A[1];
  h_obj[71] =  A[2];

  h_obj[72] = J_A[0];
  h_obj[73] = J_A[1];
  h_obj[74] = J_A[2];

  h_obj[75] = J_A[3];
  h_obj[76] = J_A[4];

  h_obj[77] = J_A[5];
  return true;
}

bool MeanRatioQualityMetric::compute_element_analytical_gradient(PatchData &pd,
								 MsqMeshEntity *e,
								 MsqVertex *v[], 
								 Vector3D g[],
								 int nv, 
								 double &m,
								 MsqError &err)
{
  EntityTopology topo = e->get_element_type();

  if (((topo == QUADRILATERAL) || (topo == HEXAHEDRON)) && 
      ((avgMethod == MINIMUM) || (avgMethod != MAXIMUM))) {
    std::cout << "Minimum and maximum not continuously differentiable." << std::endl;
    std::cout << "Element of subdifferential will be returned." << std::endl;
  }

  MsqVertex *vertices = pd.get_vertex_array(err);
  std::vector<size_t> v_i;
  e->get_vertex_indices(v_i);

  Vector3D coords[4];		// Vertex coordinates for the (decomposed) elements
  Vector3D gradients[32];	// Gradient of metric with respect to the coords
  Vector3D grad[8];		// Accumulated gradients (composed merit function)
  double   metrics[8];		// Metric values for the (decomposed) elements
  double   nm, t;

  int i, j;

  m = 0.0;

  switch(topo) {
  case TRIANGLE:
  case QUADRILATERAL:
    std::cout << "Gradients in 2D not implemented yet.\n" << std::endl;
    return false;
    break;

  case TETRAHEDRON:
    coords[0] = vertices[v_i[0]];
    coords[1] = vertices[v_i[1]];
    coords[2] = vertices[v_i[2]];
    coords[3] = vertices[v_i[3]];
    if (!g_fcn_3e(m, gradients, coords)) return false;

    // This is not very efficient, but is one way to select correct gradients
    for (i = 0; i < 4; ++i) {
      for (j = 0; j < nv; ++j) {
	if (vertices + v_i[i] == v[j]) {
	  g[j] = gradients[i];
	}
      }
    }
    break;

  case HEXAHEDRON:
    coords[0] = vertices[v_i[0]];
    coords[1] = vertices[v_i[1]];
    coords[2] = vertices[v_i[3]];
    coords[3] = vertices[v_i[4]];
    if (!g_fcn_3i(metrics[0], gradients, coords)) return false;

    coords[0] = vertices[v_i[1]];
    coords[1] = vertices[v_i[2]];
    coords[2] = vertices[v_i[0]];
    coords[3] = vertices[v_i[5]];
    if (!g_fcn_3i(metrics[1], gradients+4, coords)) return false;

    coords[0] = vertices[v_i[2]];
    coords[1] = vertices[v_i[3]];
    coords[2] = vertices[v_i[1]];
    coords[3] = vertices[v_i[6]];
    if (!g_fcn_3i(metrics[2], gradients+8, coords)) return false;

    coords[0] = vertices[v_i[3]];
    coords[1] = vertices[v_i[0]];
    coords[2] = vertices[v_i[2]];
    coords[3] = vertices[v_i[7]];
    if (!g_fcn_3i(metrics[3], gradients+12, coords)) return false;

    coords[0] = vertices[v_i[4]];
    coords[1] = vertices[v_i[7]];
    coords[2] = vertices[v_i[5]];
    coords[3] = vertices[v_i[0]];
    if (!g_fcn_3i(metrics[4], gradients+16, coords)) return false;

    coords[0] = vertices[v_i[5]];
    coords[1] = vertices[v_i[4]];
    coords[2] = vertices[v_i[6]];
    coords[3] = vertices[v_i[1]];
    if (!g_fcn_3i(metrics[5], gradients+20, coords)) return false;

    coords[0] = vertices[v_i[6]];
    coords[1] = vertices[v_i[5]];
    coords[2] = vertices[v_i[7]];
    coords[3] = vertices[v_i[2]];
    if (!g_fcn_3i(metrics[6], gradients+24, coords)) return false;

    coords[0] = vertices[v_i[7]];
    coords[1] = vertices[v_i[6]];
    coords[2] = vertices[v_i[4]];
    coords[3] = vertices[v_i[3]];
    if (!g_fcn_3i(metrics[7], gradients+28, coords)) return false;

    for (i = 0; i < 8; ++i) {
      grad[i] = 0.0;
    }

    switch(avgMethod) {
    case MINIMUM:
      m = metrics[0];
      for (i = 1; i < 8; ++i) {
	if (metrics[i] < m) m = metrics[i];
      }

      nm = 0;
      if (metrics[0] <= m + MSQ_MIN) {
	grad[0] += gradients[0];
	grad[1] += gradients[1];
	grad[2] += gradients[2];
	grad[3] += gradients[3];
	++nm;
      }

      if (metrics[1] <= m + MSQ_MIN) {
	grad[1] += gradients[4];
	grad[2] += gradients[5];
	grad[0] += gradients[6];
	grad[5] += gradients[7];
	++nm;
      }

      if (metrics[2] <= m + MSQ_MIN) {
	grad[2] += gradients[8];
	grad[3] += gradients[9];
	grad[1] += gradients[10];
	grad[6] += gradients[11];
	++nm;
      }

      if (metrics[3] <= m + MSQ_MIN) {
	grad[3] += gradients[12];
	grad[0] += gradients[13];
	grad[2] += gradients[14];
	grad[7] += gradients[15];
	++nm;
      }

      if (metrics[4] <= m + MSQ_MIN) {
	grad[4] += gradients[16];
	grad[7] += gradients[17];
	grad[5] += gradients[18];
	grad[0] += gradients[19];
	++nm;
      }

      if (metrics[5] <= m + MSQ_MIN) {
	grad[5] += gradients[20];
	grad[4] += gradients[21];
	grad[6] += gradients[22];
	grad[1] += gradients[23];
	++nm;
      }

      if (metrics[6] <= m + MSQ_MIN) {
	grad[6] += gradients[24];
	grad[5] += gradients[25];
	grad[7] += gradients[26];
	grad[2] += gradients[27];
	++nm;
      }

      if (metrics[7] <= m + MSQ_MIN) {
	grad[7] += gradients[28];
	grad[6] += gradients[29];
	grad[4] += gradients[30];
	grad[3] += gradients[31];
	++nm;
      }

      for (i = 0; i < 8; ++i) {
	grad[i] /= nm;
      }
      break;

    case MAXIMUM:
      m = metrics[0];
      for (i = 1; i < 8; ++i) {
	if (metrics[i] > m) m = metrics[i];
      }

      nm = 0;
      if (metrics[0] >= m - MSQ_MIN) {
	grad[0] += gradients[0];
	grad[1] += gradients[1];
	grad[2] += gradients[2];
	grad[3] += gradients[3];
	++nm;
      }

      if (metrics[1] >= m - MSQ_MIN) {
	grad[1] += gradients[4];
	grad[2] += gradients[5];
	grad[0] += gradients[6];
	grad[5] += gradients[7];
	++nm;
      }

      if (metrics[2] >= m - MSQ_MIN) {
	grad[2] += gradients[8];
	grad[3] += gradients[9];
	grad[1] += gradients[10];
	grad[6] += gradients[11];
	++nm;
      }

      if (metrics[3] >= m - MSQ_MIN) {
	grad[3] += gradients[12];
	grad[0] += gradients[13];
	grad[2] += gradients[14];
	grad[7] += gradients[15];
	++nm;
      }

      if (metrics[4] >= m - MSQ_MIN) {
	grad[4] += gradients[16];
	grad[7] += gradients[17];
	grad[5] += gradients[18];
	grad[0] += gradients[19];
	++nm;
      }

      if (metrics[5] >= m - MSQ_MIN) {
	grad[5] += gradients[20];
	grad[4] += gradients[21];
	grad[6] += gradients[22];
	grad[1] += gradients[23];
	++nm;
      }

      if (metrics[6] >= m - MSQ_MIN) {
	grad[6] += gradients[24];
	grad[5] += gradients[25];
	grad[7] += gradients[26];
	grad[2] += gradients[27];
	++nm;
      }

      if (metrics[7] >= m - MSQ_MIN) {
	grad[7] += gradients[28];
	grad[6] += gradients[29];
	grad[4] += gradients[30];
	grad[3] += gradients[31];
	++nm;
      }

      for (i = 0; i < 8; ++i) {
	grad[i] /= nm;
      }
      break;

    case GEOMETRIC:
      m = 0.0;
      for (i = 0; i < 8; ++i) {
	m += log(metrics[i]);
	metrics[i] = 1.0 / metrics[i];
      }
      m = exp(m / 8.0);

      grad[0] += metrics[0]*gradients[0];
      grad[1] += metrics[0]*gradients[1];
      grad[2] += metrics[0]*gradients[2];
      grad[3] += metrics[0]*gradients[3];
      
      grad[1] += metrics[1]*gradients[4];
      grad[2] += metrics[1]*gradients[5];
      grad[0] += metrics[1]*gradients[6];
      grad[5] += metrics[1]*gradients[7];

      grad[2] += metrics[2]*gradients[8];
      grad[3] += metrics[2]*gradients[9];
      grad[1] += metrics[2]*gradients[10];
      grad[6] += metrics[2]*gradients[11];

      grad[3] += metrics[3]*gradients[12];
      grad[0] += metrics[3]*gradients[13];
      grad[2] += metrics[3]*gradients[14];
      grad[7] += metrics[3]*gradients[15];

      grad[4] += metrics[4]*gradients[16];
      grad[7] += metrics[4]*gradients[17];
      grad[5] += metrics[4]*gradients[18];
      grad[0] += metrics[4]*gradients[19];

      grad[5] += metrics[5]*gradients[20];
      grad[4] += metrics[5]*gradients[21];
      grad[6] += metrics[5]*gradients[22];
      grad[1] += metrics[5]*gradients[23];

      grad[6] += metrics[6]*gradients[24];
      grad[5] += metrics[6]*gradients[25];
      grad[7] += metrics[6]*gradients[26];
      grad[2] += metrics[6]*gradients[27];

      grad[7] += metrics[7]*gradients[28];
      grad[6] += metrics[7]*gradients[29];
      grad[4] += metrics[7]*gradients[30];
      grad[3] += metrics[7]*gradients[31];

      nm = m / 8.0;
      for (i = 0; i < 8; ++i) {
	grad[i] *= nm;
      }
      break;

    default:
      switch(avgMethod) {
      case SUM:
      case LINEAR:
	t = 1.0;
	break;

      case RMS:
	t = 2.0;
	break;

      case HARMONIC:
	t = -1.0;
	break;

      case HMS:
	t = -2.0;
	break;
      }

      m = 0;
      for (i = 0; i < 8; ++i) {
	nm = pow(metrics[i], t);
	m += nm;

	metrics[i] = t*nm/metrics[i];
      }
      if (avgMethod = SUM) {
	nm = m;
      }
      else {
	nm = m / 8.0;
      }
      m = pow(nm, 1.0 / t);

      grad[0] += metrics[0]*gradients[0];
      grad[1] += metrics[0]*gradients[1];
      grad[2] += metrics[0]*gradients[2];
      grad[3] += metrics[0]*gradients[3];
      
      grad[1] += metrics[1]*gradients[4];
      grad[2] += metrics[1]*gradients[5];
      grad[0] += metrics[1]*gradients[6];
      grad[5] += metrics[1]*gradients[7];

      grad[2] += metrics[2]*gradients[8];
      grad[3] += metrics[2]*gradients[9];
      grad[1] += metrics[2]*gradients[10];
      grad[6] += metrics[2]*gradients[11];

      grad[3] += metrics[3]*gradients[12];
      grad[0] += metrics[3]*gradients[13];
      grad[2] += metrics[3]*gradients[14];
      grad[7] += metrics[3]*gradients[15];

      grad[4] += metrics[4]*gradients[16];
      grad[7] += metrics[4]*gradients[17];
      grad[5] += metrics[4]*gradients[18];
      grad[0] += metrics[4]*gradients[19];

      grad[5] += metrics[5]*gradients[20];
      grad[4] += metrics[5]*gradients[21];
      grad[6] += metrics[5]*gradients[22];
      grad[1] += metrics[5]*gradients[23];

      grad[6] += metrics[6]*gradients[24];
      grad[5] += metrics[6]*gradients[25];
      grad[7] += metrics[6]*gradients[26];
      grad[2] += metrics[6]*gradients[27];

      grad[7] += metrics[7]*gradients[28];
      grad[6] += metrics[7]*gradients[29];
      grad[4] += metrics[7]*gradients[30];
      grad[3] += metrics[7]*gradients[31];

      if (avgMethod == SUM) {
	nm = m / (nm*t);
      }
      else {
	nm = m / (8.0*nm*t);
      }

      for (i = 0; i < 8; ++i) {
	grad[i] *= nm;
      }
      break;
    }

    // This is not very efficient, but is one way to select correct gradients
    for (i = 0; i < 8; ++i) {
      for (j = 0; j < nv; ++j) {
	if (vertices + v_i[i] == v[j]) {
	  g[j] = grad[i];
	}
      }
    }
    break;

  default:
    m = 0.0;
  } // end switch over element type
  return true;
}


