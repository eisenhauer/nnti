/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/


#include <stddef.h>                     // for size_t
#include <stk_mesh/base/BulkData.hpp>   // for BulkData, etc
#include <stk_mesh/base/Entity.hpp>     // for Entity
#include <stk_mesh/base/GetEntities.hpp>  // for count_entities
#include <stk_mesh/base/MetaData.hpp>   // for MetaData
#include <stk_mesh/base/Selector.hpp>   // for Selector, operator|
#include <stk_mesh/fixtures/BoxFixture.hpp>  // for BoxFixture::BOX, etc
#include <gtest/gtest.h>
#include <vector>                       // for vector, etc
#include "stk_mesh/base/Relation.hpp"
#include "stk_mesh/base/Types.hpp"      // for EntityId, etc
#include "stk_topology/topology.hpp"    // for topology, etc
#include "stk_util/parallel/Parallel.hpp"  // for ParallelMachine
#include "stk_util/util/PairIter.hpp"   // for PairIter



using stk::mesh::MetaData;
using stk::mesh::BulkData;
using stk::mesh::Selector;
using stk::mesh::Entity;
using stk::mesh::EntityId;
using stk::mesh::EntityRank;
using stk::mesh::fixtures::BoxFixture;

namespace {

const EntityRank NODE_RANK = stk::topology::NODE_RANK;

class ExposePartition : public BoxFixture
{
 public:
  static void expose_box_partition( int ip , int up , int axis ,
                                    const BOX box ,
                                    BOX p_box[] )
  {
    box_partition(ip, up, axis, box, p_box);
  }
};

}

TEST( UnitTestBoxFixture, verifyBoxFixture )
{
  // A unit test to verify the correctness of the BoxFixture fixture.

  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  // Create the box fixture we'll be testing

  // box specifications
  const BoxFixture::BOX root_box = { { 0 , 4 } , { 0 , 5 } , { 0 , 6 } };
  BoxFixture::BOX local_box = { { 0 , 0 } , { 0 , 0 } , { 0 , 0 } };

  BoxFixture fixture(pm);
  MetaData& meta = fixture.fem_meta();
  BulkData& bulk = fixture.bulk_data();

  const EntityRank element_rank = stk::topology::ELEMENT_RANK;

  const int p_rank = bulk.parallel_rank();
  const int p_size = bulk.parallel_size();

  BoxFixture::BOX * const p_box = new BoxFixture::BOX[ p_size ];
  ExposePartition::expose_box_partition( 0, p_size, 2, root_box, &p_box[0] );

  meta.commit();

  bulk.modification_begin();
  fixture.generate_boxes( root_box, local_box );

  const unsigned nx = local_box[0][1] - local_box[0][0] ;
  const unsigned ny = local_box[1][1] - local_box[1][0] ;
  const unsigned nz = local_box[2][1] - local_box[2][0] ;

  const unsigned e_local = nx * ny * nz ;
  const unsigned n_local = ( nx + 1 ) * ( ny + 1 ) * ( nz + 1 );

  const unsigned ngx = root_box[0][1] - root_box[0][0] ;
  const unsigned ngy = root_box[1][1] - root_box[1][0] ;

  std::vector<unsigned> local_count ;

  // Verify that the correct entities are on this process

  for ( int k = local_box[2][0] ; k < local_box[2][1] ; ++k ) {
  for ( int j = local_box[1][0] ; j < local_box[1][1] ; ++j ) {
  for ( int i = local_box[0][0] ; i < local_box[0][1] ; ++i ) {

    const EntityId n0= 1 + (i+0) + (j+0) * (ngx+1) + (k+0) * (ngx+1) * (ngy+1);
    const EntityId n1= 1 + (i+1) + (j+0) * (ngx+1) + (k+0) * (ngx+1) * (ngy+1);
    const EntityId n2= 1 + (i+1) + (j+1) * (ngx+1) + (k+0) * (ngx+1) * (ngy+1);
    const EntityId n3= 1 + (i+0) + (j+1) * (ngx+1) + (k+0) * (ngx+1) * (ngy+1);
    const EntityId n4= 1 + (i+0) + (j+0) * (ngx+1) + (k+1) * (ngx+1) * (ngy+1);
    const EntityId n5= 1 + (i+1) + (j+0) * (ngx+1) + (k+1) * (ngx+1) * (ngy+1);
    const EntityId n6= 1 + (i+1) + (j+1) * (ngx+1) + (k+1) * (ngx+1) * (ngy+1);
    const EntityId n7= 1 + (i+0) + (j+1) * (ngx+1) + (k+1) * (ngx+1) * (ngy+1);

    const EntityId elem_id =  1 + i + j * ngx + k * ngx * ngy;

    std::vector<Entity> nodes(8);
    nodes[0] = bulk.get_entity( NODE_RANK, n0 );
    nodes[1] = bulk.get_entity( NODE_RANK, n1 );
    nodes[2] = bulk.get_entity( NODE_RANK, n2 );
    nodes[3] = bulk.get_entity( NODE_RANK, n3 );
    nodes[4] = bulk.get_entity( NODE_RANK, n4 );
    nodes[5] = bulk.get_entity( NODE_RANK, n5 );
    nodes[6] = bulk.get_entity( NODE_RANK, n6 );
    nodes[7] = bulk.get_entity( NODE_RANK, n7 );

    Entity elem = bulk.get_entity( element_rank, elem_id );

    std::vector<Entity> elems ;
    stk::mesh::get_entities_through_relations(bulk, nodes , elems );
    ASSERT_EQ( elems.size() , size_t(1) );
    ASSERT_TRUE( elems[0] == elem );

    stk::mesh::get_entities_through_relations(bulk, nodes, element_rank, elems);
    ASSERT_EQ( elems.size() , size_t(1) );
    ASSERT_TRUE( elems[0] == elem );

  }
  }
  }

  Selector select_owned( meta.locally_owned_part() );
  Selector select_used = select_owned |
                         meta.globally_shared_part();
  Selector select_all( meta.universal_part() );

  stk::mesh::count_entities( select_used , bulk , local_count );
  ASSERT_EQ( e_local , local_count[3] );
  ASSERT_EQ( 0u , local_count[2] );
  ASSERT_EQ( 0u , local_count[1] );
  ASSERT_EQ( n_local , local_count[0] );

  ASSERT_TRUE(bulk.modification_end());

  // Verify declarations and sharing

  stk::mesh::count_entities( select_used , bulk , local_count );
  ASSERT_EQ( local_count[3] , e_local );
  ASSERT_EQ( local_count[2] , 0u );
  ASSERT_EQ( local_count[1] , 0u );
  ASSERT_EQ( local_count[0] , n_local );

  for ( int k = local_box[2][0] ; k <= local_box[2][1] ; ++k ) {
  for ( int j = local_box[1][0] ; j <= local_box[1][1] ; ++j ) {
  for ( int i = local_box[0][0] ; i <= local_box[0][1] ; ++i ) {
    EntityRank node_type = stk::topology::NODE_RANK;
    EntityId node_id = 1 + i + j * (ngx+1) + k * (ngx+1) * (ngy+1);
    Entity const node = bulk.get_entity( node_type , node_id );
    ASSERT_TRUE( bulk.is_valid(node) );
    // Shared if on a processor boundary.
    const bool shared =
      ( k == local_box[2][0] && k != root_box[2][0] ) ||
      ( k == local_box[2][1] && k != root_box[2][1] ) ||
      ( j == local_box[1][0] && j != root_box[1][0] ) ||
      ( j == local_box[1][1] && j != root_box[1][1] ) ||
      ( i == local_box[0][0] && i != root_box[0][0] ) ||
      ( i == local_box[0][1] && i != root_box[0][1] );
    if (bulk.parallel_size() > 1) {
      ASSERT_EQ( shared , ! bulk.entity_comm_sharing(bulk.entity_key(node)).empty() );
    }
  }
  }
  }

  size_t count_shared_node_pairs = 0 ;
  for ( int p = 0 ; p < p_size ; ++p ) if ( p != p_rank ) {
    for ( int k = p_box[p][2][0] ; k <= p_box[p][2][1] ; ++k )
      if ( local_box[2][0] <= k && k <= local_box[2][1] ) {

        for ( int j = p_box[p][1][0] ; j <= p_box[p][1][1] ; ++j )
          if ( local_box[1][0] <= j && j <= local_box[1][1] ) {

            for ( int i = p_box[p][0][0] ; i <= p_box[p][0][1] ; ++i )
              if ( local_box[0][0] <= i && i <= local_box[0][1] ) {

                EntityRank node_type = stk::topology::NODE_RANK;
                EntityId node_id = 1 + i + j * (ngx+1) + k * (ngx+1) * (ngy+1);
                Entity const node = bulk.get_entity( node_type , node_id );
                ASSERT_TRUE( bulk.is_valid(node) );
                // Must be shared with 'p'
                stk::mesh::PairIterEntityComm iter = bulk.entity_comm_sharing(bulk.entity_key(node));
                for ( ; ! iter.empty() && iter->proc != p ; ++iter );
                ASSERT_TRUE( ! iter.empty() );

                ++count_shared_node_pairs ;
              }
          }
      }
  }

  size_t count_shared_entities = 0 ;
  for (stk::mesh::EntityCommListInfoVector::const_iterator
       i = bulk.comm_list().begin() ;
       i != bulk.comm_list().end() ;
       ++i) {
    const stk::mesh::PairIterEntityComm ec = bulk.entity_comm_sharing(i->key);
    count_shared_entities += ec.size();
  }
  ASSERT_EQ( count_shared_entities , count_shared_node_pairs );

  delete [] p_box;
}
