
#include <stddef.h>                     // for size_t
#include <stk_mesh/base/BulkData.hpp>   // for BulkData
#include <stk_mesh/base/Comm.hpp>       // for comm_mesh_counts
#include <stk_mesh/base/CreateAdjacentEntities.hpp>
#include <stk_mesh/base/MetaData.hpp>   // for MetaData
#include <stk_mesh/fixtures/HexFixture.hpp>  // for HexFixture
#include <stk_mesh/fixtures/QuadFixture.hpp>  // for QuadFixture
#include <gtest/gtest.h>
#include <vector>                       // for vector, vector<>::iterator
#include "stk_mesh/base/Bucket.hpp"     // for Bucket
#include "stk_mesh/base/ConnectivityMap.hpp"  // for ConnectivityMap
#include "stk_mesh/base/Types.hpp"      // for BucketVector, PartVector
#include "stk_topology/topology.hpp"    // for topology, topology::rank_t, etc







using stk::mesh::MetaData;

TEST( UnitTestStkMeshSkinning , testCreateAdjacentEntities3x1x1 )
{
  const size_t NX = 3;
  const size_t NY = 1;
  const size_t NZ = 1;

  stk::mesh::fixtures::HexFixture fixture(MPI_COMM_WORLD, NX, NY, NZ, &stk::mesh::ConnectivityMap::classic_stk_mesh());

  fixture.m_meta.commit();
  fixture.generate_mesh();

  {
    std::vector<size_t> counts ;
    stk::mesh::comm_mesh_counts( fixture.m_bulk_data , counts);

    EXPECT_EQ( counts[0] , 16u ); // nodes
    EXPECT_EQ( counts[1] , 0u );  // edges
    EXPECT_EQ( counts[2] , 0u );  // faces
    EXPECT_EQ( counts[3] , 3u ); // elements
  }

  stk::mesh::PartVector empty_add_parts;

  stk::mesh::create_adjacent_entities(fixture.m_bulk_data, empty_add_parts);

  {
    std::vector<size_t> counts ;
    stk::mesh::comm_mesh_counts( fixture.m_bulk_data , counts);

    EXPECT_EQ( counts[0] , 16u );
    EXPECT_EQ( counts[1] , 28u );
    EXPECT_EQ( counts[2] , 16u );
    EXPECT_EQ( counts[3] , 3u );
  }
}

TEST( UnitTestStkMeshSkinning , testCreateAdjacentEntities3x3x3 )
{
  const stk::topology::rank_t elem_rank = stk::topology::ELEMENT_RANK;
  const stk::topology::rank_t face_rank = stk::topology::FACE_RANK;
  const stk::topology::rank_t edge_rank = stk::topology::EDGE_RANK;
  const stk::topology::rank_t node_rank = stk::topology::NODE_RANK;

  const size_t NX = 3;
  const size_t NY = 3;
  const size_t NZ = 3;

  stk::mesh::fixtures::HexFixture fixture(MPI_COMM_WORLD, NX, NY, NZ, &stk::mesh::ConnectivityMap::classic_stk_mesh());

  fixture.m_meta.commit();
  fixture.generate_mesh();

  {
    std::vector<size_t> counts ;
    stk::mesh::comm_mesh_counts( fixture.m_bulk_data , counts);

    EXPECT_EQ( counts[node_rank] , 64u ); // nodes
    EXPECT_EQ( counts[edge_rank] , 0u );  // edges
    EXPECT_EQ( counts[face_rank] , 0u );  // faces
    EXPECT_EQ( counts[elem_rank] , 27u ); // elements
  }

  stk::mesh::PartVector empty_add_parts;

  stk::mesh::create_adjacent_entities(fixture.m_bulk_data, empty_add_parts);

  {
    std::vector<size_t> counts ;
    stk::mesh::comm_mesh_counts( fixture.m_bulk_data , counts);

    EXPECT_EQ( 64u, counts[node_rank] ); // nodes
    EXPECT_EQ( 144u, counts[edge_rank] );  // edges
    EXPECT_EQ( 108u, counts[face_rank] );  // faces
    EXPECT_EQ( 27u, counts[elem_rank]  ); // elements
  }

  stk::mesh::BucketVector  elem_buckets = fixture.m_bulk_data.buckets(elem_rank);
  for ( stk::mesh::BucketVector::iterator b_itr = elem_buckets.begin();
       b_itr != elem_buckets.end();
       ++b_itr
      )
  {
    stk::mesh::Bucket & b = **b_itr;
    for ( size_t i = 0; i< b.size(); ++i) {
      EXPECT_EQ( 6u, b.num_faces(i) );
      EXPECT_EQ( 12u, b.num_edges(i) );
      EXPECT_EQ( 8u,  b.num_nodes(i) );
    }
  }

  stk::mesh::BucketVector  face_buckets = fixture.m_bulk_data.buckets(face_rank);
  for ( stk::mesh::BucketVector::iterator b_itr = face_buckets.begin();
       b_itr != face_buckets.end();
       ++b_itr
      )
  {
    stk::mesh::Bucket & b = **b_itr;
    for ( size_t i = 0; i< b.size(); ++i) {
      EXPECT_EQ( 4u, b.num_edges(i) );
      EXPECT_EQ( 4u, b.num_nodes(i) );
    }
  }

  stk::mesh::BucketVector  edge_buckets = fixture.m_bulk_data.buckets(edge_rank);
  for ( stk::mesh::BucketVector::iterator b_itr = edge_buckets.begin();
       b_itr != edge_buckets.end();
       ++b_itr
      )
  {
    stk::mesh::Bucket & b = **b_itr;
    for ( size_t i = 0; i< b.size(); ++i) {
      EXPECT_EQ( 2u, b.num_nodes(i) );
    }
  }
}

TEST( UnitTestStkMeshSkinning , testCreateAdjacentEntities3x3 )
{
  const stk::topology::rank_t elem_rank = stk::topology::ELEMENT_RANK;
  const stk::topology::rank_t edge_rank = stk::topology::EDGE_RANK;
  const stk::topology::rank_t node_rank = stk::topology::NODE_RANK;

  const size_t NX = 3;
  const size_t NY = 3;

  stk::mesh::fixtures::QuadFixture fixture(MPI_COMM_WORLD, NX, NY);

  // skip this test unless using standard connectivity
  if (&stk::mesh::ConnectivityMap::default_map() != &stk::mesh::ConnectivityMap::classic_stk_mesh()) {
    return;
  }

  fixture.m_meta.commit();
  fixture.generate_mesh();

  {
    std::vector<size_t> counts ;
    stk::mesh::comm_mesh_counts( fixture.m_bulk_data , counts);

    EXPECT_EQ( counts[node_rank] , 16u ); // nodes
    EXPECT_EQ( counts[edge_rank] , 0u );  // edges
    EXPECT_EQ( counts[elem_rank] , 9u ); // elements
  }

  stk::mesh::PartVector empty_add_parts;

  stk::mesh::create_adjacent_entities(fixture.m_bulk_data, empty_add_parts);

  {
    std::vector<size_t> counts ;
    stk::mesh::comm_mesh_counts( fixture.m_bulk_data , counts);

    EXPECT_EQ( 16u, counts[node_rank] ); // nodes
    EXPECT_EQ( 24u, counts[edge_rank] );  // edges
    EXPECT_EQ( 9u, counts[elem_rank]  ); // elements
  }

  stk::mesh::BucketVector  elem_buckets = fixture.m_bulk_data.buckets(elem_rank);
  for ( stk::mesh::BucketVector::iterator b_itr = elem_buckets.begin();
       b_itr != elem_buckets.end();
       ++b_itr
      )
  {
    stk::mesh::Bucket & b = **b_itr;
    for ( size_t i = 0; i< b.size(); ++i) {
      EXPECT_EQ( 4u, b.num_edges(i) );
      EXPECT_EQ( 4u, b.num_nodes(i) );
    }
  }

  stk::mesh::BucketVector  edge_buckets = fixture.m_bulk_data.buckets(edge_rank);
  for ( stk::mesh::BucketVector::iterator b_itr = edge_buckets.begin();
       b_itr != edge_buckets.end();
       ++b_itr
      )
  {
    stk::mesh::Bucket & b = **b_itr;
    for ( size_t i = 0; i< b.size(); ++i) {
      EXPECT_EQ( 2u, b.num_nodes(i) );
    }
  }
}
