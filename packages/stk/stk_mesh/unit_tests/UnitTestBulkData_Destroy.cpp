/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#include <stk_util/unit_test_support/stk_utest_macros.hpp>

#include <stk_util/parallel/Parallel.hpp>

#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/GetEntities.hpp>
#include <stk_mesh/base/Comm.hpp>

#include <stk_mesh/fixtures/RingFixture.hpp>

#include <unit_tests/UnitTestModificationEndWrapper.hpp>

using stk::mesh::Part;
using stk::mesh::MetaData;
using stk::mesh::BulkData;
using stk::mesh::Entity;
using stk::mesh::Selector;
using stk::mesh::PartVector;
using stk::mesh::EntityId;
using stk::mesh::fixtures::RingFixture;

//----------------------------------------------------------------------
// Testing for mesh entities without relations

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testDestroy_nodes)
{
  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  enum { nPerProc = 10 };
  const unsigned p_rank = stk::parallel_machine_rank( pm );
  const unsigned p_size = stk::parallel_machine_size( pm );
  const unsigned id_total = nPerProc * p_size ;
  const unsigned id_begin = nPerProc * p_rank ;
  const unsigned id_end   = nPerProc * ( p_rank + 1 );

  const int spatial_dimension = 3;
  MetaData meta( spatial_dimension );

  const PartVector no_parts ;

  meta.commit();

  BulkData bulk( meta , pm , 100 );

  // Ids for all entities (all entities have type 0):

  std::vector<EntityId> ids( id_total );

  for ( unsigned i = 0 ; i < id_total ; ++i ) {
    ids[i] = i + 1;
  }

  // Declare just those entities in my range of ids:

  STKUNIT_ASSERT( bulk.modification_begin() );
  for ( unsigned i = id_begin ; i < id_end ; ++i ) {
    bulk.declare_entity( MetaData::NODE_RANK , ids[i] , no_parts );
  }
  STKUNIT_ASSERT( bulk.modification_end() );

  // Verify that I only have entities in my range:

  for ( unsigned i = 0 ; i < id_total ; ++i ) {
    Entity e = bulk.get_entity( MetaData::NODE_RANK , ids[ i ] );
    if ( id_begin <= i && i < id_end ) {
      STKUNIT_ASSERT( e.is_valid() );
      STKUNIT_ASSERT( p_rank == e.owner_rank() );
    }
    else {
      STKUNIT_ASSERT( !e.is_valid() );
    }
  }

  // Delete one entity at a time.

  for ( unsigned i = id_begin ; i < id_end ; ++i ) {
    Entity e = bulk.get_entity( MetaData::NODE_RANK , ids[ i ] );

    STKUNIT_ASSERT( e.is_valid() );

    bulk.modification_begin();
    STKUNIT_ASSERT( bulk.destroy_entity( e ) );
    bulk.modification_end();

    if ( id_begin < i ) {
      STKUNIT_ASSERT( !bulk.get_entity( MetaData::NODE_RANK , ids[ i - 1 ] ).is_valid() );
    }

    e = bulk.get_entity( MetaData::NODE_RANK , ids[ i ] );
    STKUNIT_ASSERT( !e.is_valid() );
  }
}

//----------------------------------------------------------------------

void assert_is_destroyed( const Entity entity )
{
  STKUNIT_ASSERT( !entity.is_valid() || entity.bucket().capacity() == 0 );
}

STKUNIT_UNIT_TEST(UnitTestingOfBulkData, testDestroy_ring)
{
  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier( pm );

  enum { nPerProc = 10 };
  const unsigned p_rank = stk::parallel_machine_rank( pm );
  const unsigned p_size = stk::parallel_machine_size( pm );
  // const unsigned nLocalNode = nPerProc + ( 1 < p_size ? 1 : 0 );
  const unsigned nLocalElement = nPerProc ;

  const int spatial_dimension = 3;
  MetaData meta( spatial_dimension );

  meta.commit();

  Selector select_owned( meta.locally_owned_part() );
  Selector select_used = meta.locally_owned_part() | meta.globally_shared_part();
  Selector select_all(  meta.universal_part() );

  PartVector no_parts ;

  std::vector<unsigned> local_count ;

  //------------------------------
  { // No ghosting
    const bool aura_flag = false ;

    RingFixture mesh( pm , nPerProc , false /* No element parts */ );
    mesh.m_meta_data.commit();
    BulkData& bulk = mesh.m_bulk_data;

    bulk.modification_begin();
    mesh.generate_mesh( );
    STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(bulk,
                                                           false /*no aura*/));

    bulk.modification_begin();
    mesh.fixup_node_ownership();
    STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(bulk,
                                                           false /*no aura*/));

    // This process' first element in the loop
    // if a parallel mesh has a shared node
    Entity element = bulk.get_entity( MetaData::ELEMENT_RANK , mesh.m_element_ids[ nLocalElement * p_rank ] );
    Entity node0 = element.relations()[0].entity();
    Entity node1 = element.relations()[1].entity();

    const size_t node0_elements = node0.relations().size();
    const size_t node1_elements = node1.relations().size();

    STKUNIT_ASSERT( 1 <= node0_elements && node0_elements <= 2 );
    STKUNIT_ASSERT( 1 <= node1_elements && node1_elements <= 2 );

    STKUNIT_ASSERT( node0.relations()[0].entity() == element ||
                    node0.relations()[1].entity() == element );

    STKUNIT_ASSERT( node1.relations()[0].entity() == element ||
                    node1.relations()[1].entity() == element );

    bulk.modification_begin();

    // Destroy the element:
    bool result = bulk.destroy_entity( element );
    element = Entity();
    STKUNIT_ASSERT( true == result );

    // Destroy orphanned node:
    if ( node0.relations().size() == 0 ) {
      STKUNIT_ASSERT( bulk.destroy_entity( node0 ) );
      node0 = Entity();
    }
    if ( node1.relations().size() == 0 ) {
      STKUNIT_ASSERT( bulk.destroy_entity( node1 ) );
      node1 = Entity();
    }
    STKUNIT_ASSERT( stk::unit_test::modification_end_wrapper(bulk, aura_flag) );

    if ( node0.is_valid() ) {
      STKUNIT_ASSERT_EQUAL( node0_elements - 1 , node0.relations().size() );
    }
    if ( node1.is_valid() ) {
      STKUNIT_ASSERT_EQUAL( node1_elements - 1 , node1.relations().size() );
    }
  }
  //------------------------------
  if ( 1 < p_size ) { // With ghosting
    RingFixture mesh( pm , nPerProc , false /* No element parts */ );
    mesh.m_meta_data.commit();
    BulkData& bulk = mesh.m_bulk_data;

    bulk.modification_begin();
    mesh.generate_mesh( );
    STKUNIT_ASSERT( bulk.modification_end() );

    bulk.modification_begin();
    mesh.fixup_node_ownership();
    STKUNIT_ASSERT( bulk.modification_end() );

    const unsigned nNotOwned = nPerProc * p_rank ;

    // The not-owned shared entity:
    Entity node = bulk.get_entity( MetaData::NODE_RANK , mesh.m_node_ids[ nNotOwned ] );

    STKUNIT_ASSERT( node.is_valid() );
    STKUNIT_ASSERT_NE( p_rank , node.owner_rank() );
    STKUNIT_ASSERT_EQUAL( size_t(1) , bulk.entity_comm_sharing(node.key()).size() );
    STKUNIT_ASSERT_EQUAL( size_t(2) , node.relations().size() );

    EntityId node_element_ids[2] ;
    node_element_ids[0] = node.relations()[0].entity().identifier();
    node_element_ids[1] = node.relations()[1].entity().identifier();

    bulk.modification_begin();

    // This process' first node in the loop is shared, destroy it
    // First have to destroy attached elements.
    // One will be owned and the other ghosted

    while ( node.relations().size() ) {
      Entity e = node.relations().back().entity();
      STKUNIT_ASSERT( bulk.destroy_entity( e ) );
    }
    STKUNIT_ASSERT( bulk.destroy_entity( node ) );

    STKUNIT_ASSERT( bulk.modification_end() );

    assert_is_destroyed( bulk.get_entity(MetaData::NODE_RANK, mesh.m_node_ids[nNotOwned] ) );
    assert_is_destroyed( bulk.get_entity(MetaData::ELEMENT_RANK, node_element_ids[0] ) );
    assert_is_destroyed( bulk.get_entity(MetaData::ELEMENT_RANK, node_element_ids[1] ) );

    // assert that no entities are shared or ghosted
    STKUNIT_ASSERT( bulk.comm_list().empty() );
  }
  //------------------------------
  if ( 1 < p_size ) { // With ghosting
    RingFixture mesh( pm , nPerProc , false /* No element parts */ );
    mesh.m_meta_data.commit();
    BulkData& bulk = mesh.m_bulk_data;

    bulk.modification_begin();
    mesh.generate_mesh( );
    STKUNIT_ASSERT( bulk.modification_end() );

    bulk.modification_begin();
    mesh.fixup_node_ownership();
    STKUNIT_ASSERT( bulk.modification_end() );

    // The owned shared entity:
    const unsigned nOwned = ( nPerProc * ( p_rank + 1 ) ) % mesh.m_node_ids.size();
    const unsigned nNotOwned = nPerProc * p_rank ;

    Entity node_owned = bulk.get_entity( MetaData::NODE_RANK , mesh.m_node_ids[ nOwned ] );
    Entity node_not_owned = bulk.get_entity( MetaData::NODE_RANK , mesh.m_node_ids[ nNotOwned ] );

    STKUNIT_ASSERT( node_owned.is_valid() );
    STKUNIT_ASSERT( node_not_owned.is_valid() );
    STKUNIT_ASSERT_NE( p_rank , node_not_owned.owner_rank() );
    STKUNIT_ASSERT_EQUAL( p_rank , node_owned.owner_rank() );
    STKUNIT_ASSERT_EQUAL( 1u , bulk.entity_comm_sharing(node_owned.key()).size() );
    STKUNIT_ASSERT_EQUAL( 1u , bulk.entity_comm_sharing(node_not_owned.key()).size() );
    STKUNIT_ASSERT_EQUAL( 2u , node_owned.relations().size() );

    EntityId node_element_ids[2] ;
    node_element_ids[0] = node_owned.relations()[0].entity().identifier();
    node_element_ids[1] = node_owned.relations()[1].entity().identifier();

    bulk.modification_begin();

    // This process' first node in the loop is shared, destroy it
    // First have to destroy attached elements.
    // One will be owned and the other ghosted

    while ( node_owned.relations().size() ) {
      Entity e = node_owned.relations().back().entity();
      STKUNIT_ASSERT( bulk.destroy_entity( e ) );
    }
    STKUNIT_ASSERT( bulk.destroy_entity( node_owned ) );

    STKUNIT_ASSERT( bulk.modification_end() );

    // Ownership of the other process' owned, shared, and destroyed node
    // has been transferred to this process.

    STKUNIT_ASSERT_EQUAL( p_rank , node_not_owned.owner_rank() );
    assert_is_destroyed( bulk.get_entity(MetaData::NODE_RANK, mesh.m_node_ids[ nOwned ] ) );
    assert_is_destroyed( bulk.get_entity(MetaData::ELEMENT_RANK, node_element_ids[0] ) );
    assert_is_destroyed( bulk.get_entity(MetaData::ELEMENT_RANK, node_element_ids[1] ) );

    // assert that no entities are shared or ghosted
    STKUNIT_ASSERT( bulk.comm_list().empty() );
  }
}

