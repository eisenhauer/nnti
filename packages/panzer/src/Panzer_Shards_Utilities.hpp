// @HEADER
// ***********************************************************************
//
//           Panzer: A partial differential equation assembly
//       engine for strongly coupled complex multiphysics systems
//                 Copyright (2011) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Roger P. Pawlowski (rppawlo@sandia.gov) and
// Eric C. Cyr (eccyr@sandia.gov)
// ***********************************************************************
// @HEADER

#ifndef PANZER_SHARDS_UTILITIES
#define PANZER_SHARDS_UTILITIES

#include <iostream>
#include <vector>
#include <list>
#include "Teuchos_Assert.hpp"
#include "Shards_CellTopology.hpp"

namespace panzer {
  
  template<typename ArrayCellGIDs, typename ArraySideGIDs>
    unsigned 
    getLocalSideIndexFromGlobalNodeList(const ArrayCellGIDs& cellGIDs, 
					const ArraySideGIDs& sideGIDs,
					const shards::CellTopology& cell)
  {
    unsigned cell_dim = cell.getDimension();
    //TEUCHOS_TEST_FOR_EXCEPTION(!cell.getSubcellHomogeneity(cell_dim - 1),
    //	       std::runtime_error, "Sides are not homogeneous!");
    
    unsigned local_side;
    bool found_local_side = false;
    unsigned side = 0;  
    while ( (side < cell.getSideCount()) && (!found_local_side) ) {
      
      const shards::CellTopology 
	side_topo(cell.getCellTopologyData(cell.getDimension()-1, side));
      
      unsigned num_side_nodes = 
	cell.getCellTopologyData()->side[side].topology->node_count;
 

      std::list<unsigned> tmp_side_gid_list;
      for (unsigned node = 0; node < num_side_nodes; ++node)
	tmp_side_gid_list.push_back(cellGIDs[cell.getNodeMap(cell_dim - 1, 
							     side, node)]);
     
      bool side_matches = true;
      unsigned node = 0;
      while ( side_matches && (node < num_side_nodes) ) {

	std::list<unsigned>::iterator search = 
	  std::find(tmp_side_gid_list.begin(), tmp_side_gid_list.end(),
		    sideGIDs[node]);
	
	if (search == tmp_side_gid_list.end())
	  side_matches = false;
	  
	++node;
      }
      
      if (side_matches) {
	found_local_side = true;
	local_side = side;
      }
      
      ++side;
    }
    
    TEUCHOS_TEST_FOR_EXCEPTION(!found_local_side, std::runtime_error,
		       "Failed to find side!");
    
    return local_side;
  }

  /** This function takes a list of of GIDs defined on
    * a cell and returns the local subcell Shards index for
    * a user specified subcell dimension.
    *
    * \param[in] cellGIDs Node GIDs for the cell
    * \param[in] subcellGIDs Node GIDs for the subcell to be found
    * \param[in] cell Shards cell topology used to find local index
    * \param[in] subcell_dim Dimension of subcell to be found
    *
    * \returns Local Shards index for the subcell described by subcellGIDs
    */
  template<typename ArrayCellGIDs, typename ArraySideGIDs>
    unsigned
    getLocalSubcellIndexFromGlobalNodeList(const ArrayCellGIDs& cellGIDs,
                                           const ArraySideGIDs& subcellGIDs,
                                           const shards::CellTopology& cell,unsigned subcell_dim)
  {
    unsigned local_subcell;
    bool found_local_subcell = false;
    unsigned subcell = 0;
    while ( (subcell < cell.getSubcellCount(subcell_dim)) && (!found_local_subcell) ) {
  
      unsigned num_subcell_nodes =
  	cell.getCellTopologyData()->subcell[subcell_dim][subcell].topology->node_count;
  
      std::list<unsigned> tmp_subcell_gid_list;
      for (unsigned node = 0; node < num_subcell_nodes; ++node)
        tmp_subcell_gid_list.push_back(cellGIDs[cell.getNodeMap(subcell_dim,
                                                             subcell, node)]);
  
      bool subcell_matches = true;
      unsigned node = 0;
      while ( subcell_matches && (node < num_subcell_nodes) ) {
  
        std::list<unsigned>::iterator search =
          std::find(tmp_subcell_gid_list.begin(), tmp_subcell_gid_list.end(),
                    subcellGIDs[node]);
  
        if (search == tmp_subcell_gid_list.end())
          subcell_matches = false;
  
        ++node;
      }
  
      if (subcell_matches) {
        found_local_subcell = true;
        local_subcell = subcell;
      }
  
      ++subcell;
    }
  
    TEUCHOS_TEST_FOR_EXCEPTION(!found_local_subcell, std::runtime_error,
                       "Failed to find subcell!");
  
    return local_subcell;
  }
  
  /** This function takes a vector of lists of GIDs defined on
    * a cell and returns a the local Shards subcell numbering to 
    * indexes into the vector. (Essentially will return a 
    * function defining (Shards #ing -> Local Elmt #ing).
    *
    * \param[in] cellGIDs Node GIDs for the cell
    * \param[in] subcellGIDs Node GIDs for the subcell to be found
    *                        for each subcell of a particular dimension
    * \param[in] cell Shards cell topology used to find local index
    * \param[in] subcell_dim Dimension of subcell to be found
    * \param[in,out] subcellMap Vector mapping the shards local
    *                           numbering to the user specified
    *                           local numbering.
    *
    * \notes To use the mapping "function" to find the GIDs of the
    *        subcell associated with a Shards indexed subcell
    *        simply do: <code>subcellGIDs[subcellMap[shardsIndex]]</code>
    */
  template<typename ArrayCellGIDs, typename ArraySubcellGIDs>
    void getLocalSubcellMapFromGlobalNodeLists(const ArrayCellGIDs& cellGIDs,
                                               const std::vector<ArraySubcellGIDs> & subcellGIDs,
                                               const shards::CellTopology& cell,unsigned subcell_dim,
                                               std::vector<unsigned> & subcellMap)
  {
     subcellMap.resize(subcellGIDs.size());
  
     // loop over subcell node indices searching for local subcell index
     unsigned index = 0;
     typename std::vector<ArraySubcellGIDs>::const_iterator subcellIter;
     for(subcellIter=subcellGIDs.begin();subcellIter!=subcellGIDs.end();++subcellIter) {
        unsigned localSubcell = getLocalSubcellIndexFromGlobalNodeList(cellGIDs,*subcellIter,cell,subcell_dim);
  
        // build vector mapping current index to local subcell index
        subcellMap[localSubcell] = index;
  
        index++;
     }
  }
  
}

#endif
