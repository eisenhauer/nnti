/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2010 Sandia National Laboratories.  Developed at the
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


/** \file MeshUtil.hpp
 *  \author Jason Kraftcheck 
 */

#ifndef MSQ_MESH_UTIL_HPP
#define MSQ_MESH_UTIL_HPP

#include "Mesquite.hpp"

namespace MESQUITE_NS {

class Mesh;
class MsqError;

/**\brief Miscelanions operations performed on an entire \c Mesh
 *        without the conveinience of a \c PatchData.
 */
class MeshUtil 
{
  private:
    Mesh* myMesh;
    
  public:
    MeshUtil( Mesh* mesh ) : myMesh( mesh ) {}
    
    /**\brief Calcluate statistics for mesh edge lengths
     *
     *\param min_out  Minimum edge length in mesh
     *\param avg_out  Algebraic mean of mesh edge lengths
     *\param rms_out  Root mean squared of mesh edge lengths
     *\param max_out  Maximum mesh edge length
     *\param std_dev_out  Standard deviation of mesh edge lengths
     */
    void edge_length_distribution( double& min_out,
                                   double& avg_out,
                                   double& rms_out,
                                   double& max_out,
                                   double& std_dev_out,
                                   MsqError& err );
};


} // namespace MESQUITE_NS

#endif
