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


/** \file SizeFactor.cpp
 *  \brief 
 *  \author Jason Kraftcheck 
 */

#include "Mesquite.hpp"
#include "SizeFactor.hpp"
#include "TargetCalculator.hpp"
#include "TopologyInfo.hpp"
#include "PatchData.hpp"
#include "MsqError.hpp"

namespace MESQUITE_NS {

bool SizeFactor::get_size( PatchData& pd, 
                           size_t element,
                           Sample sample,
                           double& lambda_out,
                           MsqError& err )
{
  int dim = TopologyInfo::dimension( pd.element_by_index(element).get_element_type());
  if (dim == 2) {
    MsqMatrix<3,2> W;
    bool valid = srcTargets->get_2D_target( pd, element, sample, W, err ); 
    if (MSQ_CHKERR(err) || !valid)
      return false;
    lambda_out = factor_size( W );
    return true;
  }
  else {
    MsqMatrix<3,3> W;
    bool valid = srcTargets->get_3D_target( pd, element, sample, W, err ); 
    if (MSQ_CHKERR(err) || !valid)
      return false;
    lambda_out = factor_size( W );
    return true;
  }
}

} // namespace MESQUITE_NS
