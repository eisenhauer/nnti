/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2006 Lawrence Livermore National Laboratory.  Under 
    the terms of Contract B545069 with the University of Wisconsin -- 
    Madison, Lawrence Livermore National Laboratory retains certain
    rights in this software.

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

    (2006) kraftche@cae.wisc.edu    

  ***************************************************************** */

#ifndef MSQ_VERTEX_PATCHES_CPP
#define MSQ_VERTEX_PATCHES_CPP

/** \file VertexPatches.cpp
 *  \brief 
 *  \author Jason Kraftcheck
 */
 
#include "VertexPatches.hpp"
#include "MsqError.hpp"
#include "MeshInterface.hpp"
#include "MsqVertex.hpp"

#ifdef MSQ_USE_OLD_STD_HEADERS
# include <algorithm.h>
#else
# include <algorithm>
#endif

namespace Mesquite {

void VertexPatches::get_patch_handles( msq_std::vector<PatchHandle>& patch_handles_out,
                                       MsqError& err )
{
    // get all vertex handles
  get_mesh()->get_all_vertices( patch_handles_out, err ); MSQ_ERRRTN(err);
  
  if (free_vertices_only()) {
      // get fixed flags for vertices
    bool* fixed_flags = new bool[patch_handles_out.size()];
    get_mesh()->vertices_get_fixed_flag( &patch_handles_out[0],
                                         fixed_flags,
                                         patch_handles_out.size(),
                                         err );

      // remove fixed vertices from list
    if (!MSQ_CHKERR(err)) {
      size_t write = 0;
      for (size_t read = 0; read < patch_handles_out.size(); ++read) 
        if (!fixed_flags[read])
          patch_handles_out[write++] = patch_handles_out[read];
      patch_handles_out.resize(write);
    }
    delete [] fixed_flags;
  }
}

void VertexPatches::get_patch( PatchHandle patch_handle,
                               msq_std::vector<Mesh::ElementHandle>& elem_handles_out,
                               msq_std::vector<Mesh::VertexHandle>& free_vertices_out,
                               MsqError& err )
{
  free_vertices_out.clear();
  elem_handles_out.clear();
    
  if (free_vertices_only()) { // check if vertex is culled
    unsigned char byte;
    get_mesh()->vertices_get_byte( &patch_handle, &byte, 1, err );
    if (MSQ_CHKERR(err) || (byte & MsqVertex::MSQ_SOFT_FIXED))
      return;
  }
    
  free_vertices_out.push_back( patch_handle );
  if (!numLayers)  // if no layers of elements, then done.
    return;
    
    // get elements adjacent to free vertex
  get_mesh()->vertices_get_attached_elements( &patch_handle, 
                                              1,
                                              elem_handles_out, 
                                              junk, err );
  if (MSQ_CHKERR(err))
    return;
  
  unsigned remaining = numLayers;
  while (--remaining) { // loop if more than one layer of elements

      // Get vertices adjacent to elements
    free_vertices_out.clear();
    get_mesh()->elements_get_attached_vertices( &elem_handles_out[0],
                                                elem_handles_out.size(),
                                                free_vertices_out,
                                                junk, err );
    if (MSQ_CHKERR(err)) break;
    
      // remove duplicates from vertex list
    msq_std::sort( free_vertices_out.begin(), free_vertices_out.end() );
    free_vertices_out.erase( 
        msq_std::unique( free_vertices_out.begin(), free_vertices_out.end() ),
        free_vertices_out.end() );
  
      // Get elements adjacent to vertices
    elem_handles_out.clear();
    get_mesh()->vertices_get_attached_elements( &free_vertices_out[0], 
                                                free_vertices_out.size(),
                                                elem_handles_out, 
                                                junk, err );
    if (MSQ_CHKERR(err)) break;

      // Remove duplicates from element list
    msq_std::sort( elem_handles_out.begin(), elem_handles_out.end() );
    elem_handles_out.erase( 
        msq_std::unique( elem_handles_out.begin(), elem_handles_out.end() ),
        elem_handles_out.end() );
  }
}

} // namespace Mesquite

#endif
