/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/


#include <sstream>
#include <stdexcept>
#include <iostream>

#include <stk_util/unit_test_support/stk_utest_macros.hpp>

#include <stk_util/parallel/Parallel.hpp>

#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/GetEntities.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_mesh/base/FieldData.hpp>
#include <stk_mesh/base/Comm.hpp>
#include <stk_mesh/base/EntityComm.hpp>
#include <stk_mesh/base/Ghosting.hpp>

#include <stk_mesh/fem/TopologyHelpers.hpp>
#include <stk_mesh/fem/TopologicalMetaData.hpp>
#include <stk_mesh/fem/EntityRanks.hpp>

#include <stk_mesh/fixtures/BoxFixture.hpp>
#include <stk_mesh/fixtures/RingFixture.hpp>

#include <unit_tests/UnitTestModificationEndWrapper.hpp>

#include <Shards_BasicTopologies.hpp>

using stk::mesh::Entity;
using stk::mesh::Part;
using stk::mesh::Relation;
using stk::mesh::Selector;
using stk::mesh::EntityId;
using stk::mesh::MetaData;
using stk::mesh::Ghosting;
using stk::mesh::TopologicalMetaData;
using stk::mesh::fixtures::BoxFixture;
using stk::mesh::fixtures::RingFixture;

STKUNIT_UNIT_TEST(UnitTestingOfRelation, testRelation)
{
  // Unit test the Part functionality in isolation:

  stk::ParallelMachine pm = MPI_COMM_WORLD;
  MPI_Barrier ( MPI_COMM_WORLD );

  typedef stk::mesh::Field<double>  ScalarFieldType;
 // static const char method[] = "stk::mesh::UnitTestRelation" ;

  std::vector<std::string> entity_names(10);
  for ( size_t i = 0 ; i < 10 ; ++i ) {
    std::ostringstream name ;
    name << "EntityRank" << i ;
    entity_names[i] = name.str();
  }

  unsigned max_bucket_size = 4;

  BoxFixture fixture1(pm , max_bucket_size, entity_names),
             fixture2(pm , max_bucket_size, entity_names);

  MetaData& meta  = fixture1.meta_data();
  MetaData& meta2 = fixture2.meta_data();
  const int spatial_dimension = 3;
  TopologicalMetaData top( meta, spatial_dimension );
  TopologicalMetaData top2( meta2, spatial_dimension );

  stk::mesh::BulkData& bulk  = fixture1.bulk_data();
  stk::mesh::BulkData& bulk2 = fixture2.bulk_data();

  ScalarFieldType & temperature =
    meta.declare_field < ScalarFieldType > ( "temperature" , 4 );
  ScalarFieldType & volume =
    meta.declare_field < ScalarFieldType > ( "volume" , 4 );
  ScalarFieldType & temperature2 =
    meta2.declare_field < ScalarFieldType > ( "temperature" , 4 );
  ScalarFieldType & volume2 =
    meta2.declare_field < ScalarFieldType > ( "volume" , 4 );

  Part & universal  = meta.universal_part ();
  Part & universal2 = meta2.universal_part ();
  Part & owned      = meta.locally_owned_part ();

  stk::mesh::put_field ( temperature , top.node_rank , universal );
  stk::mesh::put_field ( volume , top.element_rank , universal );
  meta.commit();
  stk::mesh::put_field ( temperature2 , top2.node_rank , universal2 );
  stk::mesh::put_field ( volume2 , top2.element_rank , universal2 );
  meta2.commit();

  bulk.modification_begin();
  bulk2.modification_begin();

  const int root_box[3][2] = { { 0,4 } , { 0,5 } , { 0,6 } };
  int local_box1[3][2] = { { 0,0 } , { 0,0 } , { 0,0 } };
  int local_box2[3][2] = { { 0,0 } , { 0,0 } , { 0,0 } };

  {
    bulk.modification_begin();
    fixture1.generate_boxes(root_box, local_box1);

    const Ghosting & gg = bulk.create_ghosting( std::string("shared") );

    // Test for coverage of comm_procs in EntityComm.cpp
    stk::mesh::EntityVector nodes;
    stk::mesh::get_entities(bulk, top.node_rank, nodes);
    std::vector<unsigned> procs ;
    STKUNIT_ASSERT(!nodes.empty());
    stk::mesh::comm_procs( gg, *nodes.front() , procs );  

    STKUNIT_ASSERT(bulk.modification_end());

    bulk.modification_begin();
    bulk.destroy_all_ghosting();
    STKUNIT_ASSERT(bulk.modification_end());
  }

  {
    bulk2.modification_begin();
    fixture2.generate_boxes(root_box, local_box2);

    bulk2.create_ghosting( std::string("shared") );

    STKUNIT_ASSERT(bulk2.modification_end());

    bulk2.modification_begin();
    bulk2.destroy_all_ghosting();
    STKUNIT_ASSERT(bulk2.modification_end());
  }

  Entity &cell = *(bulk.buckets (3)[0]->begin());
  Entity &node = bulk.buckets (0)[0]-> operator [] ( 0 );
  Entity &nodeb = bulk.buckets (0)[0]-> operator [] ( 2 );

  std::vector<Part *> parts;
  parts.push_back ( &universal );
  parts.push_back ( &owned );
  bulk.modification_begin();
  stk::mesh::EntityId  new_id = bulk.parallel_rank() + 1;
  Entity &edge = bulk.declare_entity ( 1 , new_id , parts );

  Entity &cell2 = *(bulk2.buckets (3)[0]->begin());
  Entity &node2 = *(bulk2.buckets (0)[0]->begin());

  STKUNIT_ASSERT_THROW ( Relation r ( Relation::attribute( 2 , 0 ) , cell ) , std::invalid_argument );

  {
      int ok = 0 ;
    try {

  unsigned id = 10000*(~(0u));

  Relation r (Relation::attribute( 0 , id ), cell );

    }
    catch( const std::exception & x ) {
      ok = 1 ;
      std::cout << "UnitRelation CORRECTLY caught error for : "
                << x.what()
                << std::endl ;
    }

    if ( ! ok ) {
      throw std::runtime_error("UnitTestRelation FAILED to catch error for Relation::attribute");
    }
  }

  STKUNIT_ASSERT_THROW ( bulk.declare_relation ( node , cell , 0 ) , std::runtime_error );
  STKUNIT_ASSERT_THROW ( bulk.declare_relation ( cell , node2 , 0 ) , std::runtime_error );
  STKUNIT_ASSERT_THROW ( bulk.declare_relation ( cell2 , node , 0 ) , std::runtime_error );

  bulk.declare_relation ( edge , node , 1 );
  STKUNIT_ASSERT_THROW ( bulk.declare_relation ( edge , nodeb , 1 ) , std::runtime_error );
  bulk.declare_relation ( edge , nodeb , 2 );

  std::stringstream s;
  s << *edge.relations().first ;

  bulk.modification_end();

  //Testing on in_send_ghost and in_shared in EntityComm.cpp
  enum { nPerProc = 10 };
  const unsigned p_rank = stk::parallel_machine_rank( pm );
  const unsigned p_size = stk::parallel_machine_size( pm );

  const unsigned nLocalEdge = nPerProc ;
  MetaData meta3( TopologicalMetaData::entity_rank_names(spatial_dimension) );

  meta3.commit();

  Selector select_owned( meta3.locally_owned_part() );
  Selector select_used = meta3.locally_owned_part() ;
  Selector select_all(  meta3.universal_part() );

  stk::mesh::PartVector no_parts ;

  std::vector<unsigned> local_count ;

  //------------------------------
  { // No ghosting
    bool aura_flag = false;
    RingFixture mesh2( pm , nPerProc , false /* No edge parts */ );
    mesh2.m_meta_data.commit();

    mesh2.m_bulk_data.modification_begin();
    mesh2.generate_mesh( );
    STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(mesh2.m_bulk_data,
                                                            aura_flag));
    mesh2.m_bulk_data.modification_begin();
    mesh2.fixup_node_ownership( );
    STKUNIT_ASSERT(stk::unit_test::modification_end_wrapper(mesh2.m_bulk_data,
                                                            aura_flag));

    // This process' first element in the loop
    // if a parallel mesh has a shared node

    Entity * edgenew = mesh2.m_bulk_data.get_entity( 1 , mesh2.m_edge_ids[ nLocalEdge * p_rank ] );

    mesh2.m_bulk_data.modification_begin();
    for ( unsigned p = 0 ; p < p_size ; ++p ) if ( p != p_rank ) {
      STKUNIT_ASSERT_EQUAL( in_shared( *edgenew , p ), false );
      STKUNIT_ASSERT_EQUAL( in_send_ghost( *edgenew , p ), false );
    }

    Entity * edgenew2 = mesh2.m_bulk_data.get_entity( 1 , mesh2.m_edge_ids[ nLocalEdge * p_rank ] );
    STKUNIT_ASSERT_EQUAL( in_send_ghost( *edgenew2 , p_rank+100 ), false );

    Entity * node3 = mesh2.m_bulk_data.get_entity( 0 , mesh2.m_node_ids[ nLocalEdge * p_rank ] );
    STKUNIT_ASSERT_EQUAL( in_shared( *node3 , p_rank+100 ), false );
  }

  { //ghosting

  if ( 1 < p_size ) { // With ghosting
    RingFixture mesh3( pm , nPerProc , false /* No edge parts */ );
    mesh3.m_meta_data.commit();

    mesh3.m_bulk_data.modification_begin();
    mesh3.generate_mesh();
    STKUNIT_ASSERT(mesh3.m_bulk_data.modification_end());

    mesh3.m_bulk_data.modification_begin();
    mesh3.fixup_node_ownership();
    STKUNIT_ASSERT(mesh3.m_bulk_data.modification_end());

    const unsigned nNotOwned = nPerProc * p_rank ;

    // The not-owned shared entity:
    Entity * node3 = mesh3.m_bulk_data.get_entity( 0 , mesh3.m_node_ids[ nNotOwned ] );
    Entity * node4 = mesh3.m_bulk_data.get_entity( 0 , mesh3.m_node_ids[ nNotOwned ] );


    EntityId node_edge_ids[2] ;
    node_edge_ids[0] = node3->relations()[0].entity()->identifier();
    node_edge_ids[1] = node3->relations()[1].entity()->identifier();

    mesh3.m_bulk_data.modification_begin();

    for ( unsigned p = 0 ; p < p_size ; ++p ) if ( p != p_rank ) {
      //FIXME for Carol the check below did not pass for -np 3 or 4
      //STKUNIT_ASSERT_EQUAL( in_shared( *node3 , p ), true );
      STKUNIT_ASSERT_EQUAL( in_send_ghost( *node3 , p ), false );
    }

    //not owned and not shared
    Entity * node5 = mesh3.m_bulk_data.get_entity( 0 , mesh3.m_node_ids[ nLocalEdge * p_rank ] );

    node_edge_ids[0] = node5->relations()[0].entity()->identifier();
    node_edge_ids[1] = node5->relations()[1].entity()->identifier();

    STKUNIT_ASSERT_EQUAL( in_shared( *node5 , p_rank+100 ), false );
    STKUNIT_ASSERT_EQUAL( in_send_ghost( *node4 , p_rank+100 ), false );
  }

  }

}
