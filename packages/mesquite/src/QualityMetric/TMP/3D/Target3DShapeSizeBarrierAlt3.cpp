/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2009 Sandia National Laboratories.  Developed at the
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
 
    (2009) kraftche@cae.wisc.edu
   
  ***************************************************************** */


/** \file Target3DShapeSizeBarrierAlt3.cpp
 *  \brief 
 *  \author Jason Kraftcheck 
 */

#include "Mesquite.hpp"
#include "Target3DShapeSizeBarrierAlt3.hpp"
#include "TMPDerivs.hpp"

namespace MESQUITE_NS {

std::string Target3DShapeSizeBarrierAlt3::get_name() const
  { return "ShapeSizeBarrier1"; }

bool Target3DShapeSizeBarrierAlt3::evaluate( const MsqMatrix<3,3>& A, 
                                             const MsqMatrix<3,3>& W, 
                                             double& result, 
                                             MsqError&  )
{
  const MsqMatrix<3,3> T = A * inverse(W);
  const double tau = det(T);
  if (invalid_determinant(tau)) { // barrier
    result = 0.0;
    return false;
  }
  
  const double norm = Frobenius(T);
  result = norm*norm*norm / (3*MSQ_SQRT_THREE*tau) - 1 
         + mGamma * (tau + 1/tau - 2);
  return true;
}

bool Target3DShapeSizeBarrierAlt3::evaluate_with_grad( const MsqMatrix<3,3>& A,
                                                       const MsqMatrix<3,3>& W,
                                                       double& result,
                                                       MsqMatrix<3,3>& deriv,
                                                       MsqError& err )
{
  const MsqMatrix<3,3> Winv = inverse(W);
  const MsqMatrix<3,3> T = A * Winv;
  const double tau = det(T);
  if (invalid_determinant(tau)) { // barrier
    result = 0.0;
    return false;
  }
  
  const double norm = Frobenius(T);
  const double f = norm*norm/3.0;
  const double g = norm / (MSQ_SQRT_THREE * tau);
  const double inv_tau = 1.0/tau;
  result = f * g - 1 + mGamma * (tau + inv_tau - 2);
  
  deriv = g*T;
  deriv += (mGamma * (1 - inv_tau*inv_tau) - f*g*inv_tau) * transpose_adj(T);
  deriv = deriv * transpose(Winv);
  
  return true;
}


bool Target3DShapeSizeBarrierAlt3::evaluate_with_hess( const MsqMatrix<3,3>& A,
                                                       const MsqMatrix<3,3>& W,
                                                       double& result,
                                                       MsqMatrix<3,3>& deriv,
                                                       MsqMatrix<3,3> second[6],
                                                       MsqError& err )
{
  const MsqMatrix<3,3> Winv = inverse(W);
  const MsqMatrix<3,3> T = A * Winv;
  const double tau = det(T);
  if (invalid_determinant(tau)) { // barrier
    result = 0.0;
    return false;
  }
  
  const double norm = Frobenius(T);
  const double f = norm*norm/3.0;
  const double h = 1/(MSQ_SQRT_THREE * tau);
  const double g = norm * h;
  const double inv_tau = 1.0/tau;
  result = f * g - 1 + mGamma * (tau + inv_tau - 2);
  
  const double g1 = mGamma * (1 - inv_tau*inv_tau);
  const MsqMatrix<3,3> adjt = transpose_adj(T);
  deriv = g*T;
  deriv += (g1 - f*g*inv_tau) * adjt;
  deriv = deriv * transpose(Winv);
  
  const double inv_norm = 1/norm;
  set_scaled_outer_product( second, h*inv_norm, T );
  pluseq_scaled_I( second, norm * h );
  pluseq_scaled_2nd_deriv_of_det( second, g1 - f*g*inv_tau, T );
  pluseq_scaled_outer_product( second, (f*g + mGamma*inv_tau)*2*inv_tau*inv_tau, adjt );
  pluseq_scaled_sum_outer_product( second, -g*inv_tau, T, adjt );
  second_deriv_wrt_product_factor( second, Winv );
  
  return true;
}

} // namespace Mesquite
