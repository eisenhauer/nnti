// Copyright (c) 2013, Sandia Corporation.
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
// 
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
// 
//     * Neither the name of Sandia Corporation nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 

#include <stk_mesh/fixtures/TetFixture.hpp>
#include <stk_mesh/fixtures/FixtureNodeSharing.hpp>
#include <stk_mesh/base/Entity.hpp>     // for Entity
#include <stk_mesh/base/FEMHelpers.hpp>  // for declare_element
#include <stk_mesh/base/Types.hpp>      // for EntityId
#include <stk_util/environment/ReportHandler.hpp>  // for ThrowRequireMsg
#include "stk_mesh/base/BulkData.hpp"   // for BulkData
#include "stk_mesh/base/Field.hpp"      // for Field
#include "stk_mesh/base/FieldBase.hpp"  // for field_data
#include "stk_mesh/base/MetaData.hpp"   // for MetaData, put_field
#include "stk_mesh/fixtures/CoordinateMapping.hpp"
#include "stk_util/parallel/Parallel.hpp"  // for ParallelMachine
namespace stk { namespace mesh { struct ConnectivityMap; } }





namespace stk {
namespace mesh {
namespace fixtures {

  TetFixture::TetFixture(   stk::ParallelMachine pm
              , size_t nx
              , size_t ny
              , size_t nz
              , ConnectivityMap const* connectivity_map
            )
  : m_spatial_dimension(3),
    m_nx(nx),
    m_ny(ny),
    m_nz(nz),
    m_meta( m_spatial_dimension ),
    m_bulk_data(  m_meta
                , pm
#ifdef SIERRA_MIGRATION
                , false
#endif
                , connectivity_map
               ),
    m_elem_parts( 1, &m_meta.declare_part_with_topology("tet_part", stk::topology::TET_4) ),
    m_node_parts( 1, &m_meta.declare_part_with_topology("node_part", stk::topology::NODE) ),
    m_coord_field( m_meta.declare_field<CoordFieldType>(stk::topology::NODE_RANK, "Coordinates") )
{

  //put coord-field on all nodes:
  put_field(
    m_coord_field,
    m_meta.universal_part(),
    m_spatial_dimension);

}

void TetFixture::generate_mesh(const CoordinateMapping & coordMap)
{
  std::vector<size_t> hex_range_on_this_processor;

  const size_t p_size = m_bulk_data.parallel_size();
  const size_t p_rank = m_bulk_data.parallel_rank();
  const size_t num_elems = m_nx * m_ny * m_nz ;

  m_nodes_to_procs.clear();
  for (unsigned proc_rank = 0; proc_rank < p_size; ++proc_rank) {
    fill_node_map(proc_rank);
  }

  const size_t beg_elem = ( num_elems * p_rank ) / p_size ;
  const size_t end_elem = ( num_elems * ( p_rank + 1 ) ) / p_size ;

  for ( size_t i = beg_elem; i != end_elem; ++i) {
    hex_range_on_this_processor.push_back(i);
  }

  generate_mesh(hex_range_on_this_processor, coordMap);
}

void TetFixture::node_x_y_z( EntityId entity_id, size_t &x , size_t &y , size_t &z ) const
{
  entity_id -= 1;

  x = entity_id % (m_nx+1);
  entity_id /= (m_nx+1);

  y = entity_id % (m_ny+1);
  entity_id /= (m_ny+1);

  z = entity_id;
}

void TetFixture::hex_x_y_z( EntityId entity_id, size_t &x , size_t &y , size_t &z ) const
{
  x = entity_id % m_nx;
  entity_id /= m_nx;

  y = entity_id % m_ny;
  entity_id /= m_ny;

  z = entity_id;
}

void TetFixture::generate_mesh(std::vector<size_t> & hex_range_on_this_processor, const CoordinateMapping & coordMap)
{
  m_bulk_data.modification_begin();

  {
    int tet_vert[][4] = { {0, 2, 3, 6},
			  {0, 3, 7, 6},
			  {0, 7, 4, 6},
			  {0, 5, 6, 4},
			  {1, 5, 6, 0},
			  {1, 6, 2, 0}};

    // Declare the elements that belong on this process
    std::vector<size_t>::iterator ib = hex_range_on_this_processor.begin();
    const std::vector<size_t>::iterator ie = hex_range_on_this_processor.end();
    for (; ib != ie; ++ib) {
      size_t hex_id = *ib;
      size_t ix = 0, iy = 0, iz = 0;
      hex_x_y_z(hex_id, ix, iy, iz);

      stk::mesh::EntityId elem_node[8] ;
      stk::mesh::EntityId tet_node[4];
      
      elem_node[0] = node_id( ix   , iy   , iz   );
      elem_node[1] = node_id( ix+1 , iy   , iz   );
      elem_node[2] = node_id( ix+1 , iy+1 , iz   );
      elem_node[3] = node_id( ix   , iy+1 , iz   );
      elem_node[4] = node_id( ix   , iy   , iz+1 );
      elem_node[5] = node_id( ix+1 , iy   , iz+1 );
      elem_node[6] = node_id( ix+1 , iy+1 , iz+1 );
      elem_node[7] = node_id( ix   , iy+1 , iz+1 );

      for (size_t tet = 0; tet < 6; tet++) {
	tet_node[0] = elem_node[tet_vert[tet][0]];
	tet_node[1] = elem_node[tet_vert[tet][1]];
	tet_node[2] = elem_node[tet_vert[tet][2]];
	tet_node[3] = elem_node[tet_vert[tet][3]];
	EntityId tet_id = 6*hex_id + tet + 1;
	stk::mesh::declare_element( m_bulk_data, m_elem_parts, tet_id, tet_node);

	for (size_t i = 0; i<4; ++i) {
	  stk::mesh::Entity const node = m_bulk_data.get_entity( stk::topology::NODE_RANK , tet_node[i] );
	  m_bulk_data.change_entity_parts(node, m_node_parts);

	  ThrowRequireMsg( m_bulk_data.is_valid(node),
			   "This process should know about the nodes that make up its element");

	  DoAddNodeSharings(m_bulk_data, m_nodes_to_procs, tet_node[i], node);

	  // Compute and assign coordinates to the node
	  size_t nx = 0, ny = 0, nz = 0;
	  node_x_y_z(tet_node[i], nx, ny, nz);

	  Scalar * data = stk::mesh::field_data( m_coord_field , node );

	  coordMap.getNodeCoordinates(data, nx, ny, nz);
	}
      }
    }
  }
  m_bulk_data.modification_end();
}

void TetFixture::fill_node_map(int p_rank)
{

  std::vector<EntityId> element_ids_on_this_processor;

  const size_t p_size = m_bulk_data.parallel_size();
  const size_t num_elems = m_nx * m_ny * m_nz ;

  const EntityId beg_elem = ( num_elems * p_rank ) / p_size ;
  const EntityId end_elem = ( num_elems * ( p_rank + 1 ) ) / p_size ;

  for ( EntityId i = beg_elem; i != end_elem; ++i) {
    element_ids_on_this_processor.push_back(i);
  }

  {
    int tet_vert[][4] = { {0, 2, 3, 6},
        {0, 3, 7, 6},
        {0, 7, 4, 6},
        {0, 5, 6, 4},
        {1, 5, 6, 0},
        {1, 6, 2, 0}};

    // Declare the elements that belong on this process
    std::vector<EntityId>::iterator ib = element_ids_on_this_processor.begin();
    const std::vector<EntityId>::iterator ie = element_ids_on_this_processor.end();
    for (; ib != ie; ++ib) {
      size_t hex_id = *ib;
      size_t ix = 0, iy = 0, iz = 0;
      hex_x_y_z(hex_id, ix, iy, iz);

      stk::mesh::EntityId elem_node[8] ;
      stk::mesh::EntityId tet_node[4];

      elem_node[0] = node_id( ix   , iy   , iz   );
      elem_node[1] = node_id( ix+1 , iy   , iz   );
      elem_node[2] = node_id( ix+1 , iy+1 , iz   );
      elem_node[3] = node_id( ix   , iy+1 , iz   );
      elem_node[4] = node_id( ix   , iy   , iz+1 );
      elem_node[5] = node_id( ix+1 , iy   , iz+1 );
      elem_node[6] = node_id( ix+1 , iy+1 , iz+1 );
      elem_node[7] = node_id( ix   , iy+1 , iz+1 );

      for (size_t tet = 0; tet < 6; tet++) {
        tet_node[0] = elem_node[tet_vert[tet][0]];
        tet_node[1] = elem_node[tet_vert[tet][1]];
        tet_node[2] = elem_node[tet_vert[tet][2]];
        tet_node[3] = elem_node[tet_vert[tet][3]];

  for (size_t i = 0; i<4; ++i) {
    AddToNodeProcsMMap(m_nodes_to_procs, tet_node[i] , p_rank);
  }
      }
    }
  }
}

} // fixtures
} // mesh
} // stk
