
#include <sstream>

#include <unit_tests/UnitTestMesh.hpp>


namespace stk {
namespace unit_test {

/****************************************************************/

std::vector<std::string>  get_entity_type_names ( int rank )
{
  std::vector<std::string>  ret_val;
  ret_val.push_back ( "Node" );
  if ( rank == 0 ) return ret_val;
  ret_val.push_back ( "Edge" );
  if ( rank == 1 ) return ret_val;
  ret_val.push_back ( "Face" );
  if ( rank == 2 ) return ret_val;
  ret_val.push_back ( "Element" );
  if ( rank == 3 ) return ret_val;
  for ( int i = 3 ; i != rank ; i++ )
  {
    std::stringstream  name;
    name << "Entity rank " << i;
    ret_val.push_back ( name.str() );
  }
  return ret_val;
}


UnitTestMetaData::UnitTestMetaData ()
  : m_meta_data ( get_entity_type_names ( MAX_RANK ) ) 
  , m_test_part ( m_meta_data.declare_part ( "Test Part" ) )
  , m_cell_part ( m_meta_data.declare_part ( "Cell list" , MAX_RANK ) )
  , m_part_A_0 ( m_meta_data.declare_part ( "Part A 0", 0 ) )
  , m_part_A_1 ( m_meta_data.declare_part ( "Part A 1", 1 ) )
  , m_part_A_2 ( m_meta_data.declare_part ( "Part A 2", 2 ) )
  , m_part_A_3 ( m_meta_data.declare_part ( "Part A 3", 3 ) )
  , m_part_A_superset ( m_meta_data.declare_part ( "Part A superset" ) )
  , m_part_B_0 ( m_meta_data.declare_part ( "Part B 0", 0 ) )
  , m_part_B_1 ( m_meta_data.declare_part ( "Part B 1", 1 ) )
  , m_part_B_2 ( m_meta_data.declare_part ( "Part B 2", 2 ) )
  , m_part_B_3 ( m_meta_data.declare_part ( "Part B 3", 3 ) )
  , m_part_B_superset ( m_meta_data.declare_part ( "Part B superset" ) )
{
  m_meta_data.declare_part_subset ( m_part_A_superset , m_part_A_0 );
  m_meta_data.declare_part_subset ( m_part_A_superset , m_part_A_1 );
  m_meta_data.declare_part_subset ( m_part_A_superset , m_part_A_2 );
  m_meta_data.declare_part_subset ( m_part_A_superset , m_part_A_3 );

  m_meta_data.declare_part_subset ( m_part_B_superset , m_part_B_0 );
  m_meta_data.declare_part_subset ( m_part_B_superset , m_part_B_1 );
  m_meta_data.declare_part_subset ( m_part_B_superset , m_part_B_2 );
  m_meta_data.declare_part_subset ( m_part_B_superset , m_part_B_3 );

  m_meta_data.commit ();
}



UnitTestMesh::UnitTestMesh ( stk::ParallelMachine comm , unsigned block_size )
  : UnitTestMetaData ()
  , m_bulk_data ( m_meta_data , comm , block_size )
  , m_previous_state ( stk::mesh::BulkData::MODIFIABLE )
{
  m_comm_rank = stk::parallel_machine_rank( comm );
  m_comm_size = stk::parallel_machine_size( comm );
}


void UnitTestMesh::generate_boxes ( bool aura )
{
  enter_modification();
  const int root_box[3][2] = { { 0,4 } , { 0,5 } , { 0,6 } };
  int local_box[3][2] = { { 0,0 } , { 0,0 } , { 0,0 } };
  priv_generate_boxes( m_bulk_data , aura , root_box , local_box );
  exit_modification();
}



namespace {

/* Recursively split a box into ( up - ip ) sub-boxes */

typedef int BOX[3][2] ;

void box_partition( int ip , int up , int axis , 
                    const BOX box ,
                    BOX p_box[] )
{ 
  const int np = up - ip ;
  if ( 1 == np ) {
    p_box[ip][0][0] = box[0][0] ; p_box[ip][0][1] = box[0][1] ;
    p_box[ip][1][0] = box[1][0] ; p_box[ip][1][1] = box[1][1] ;
    p_box[ip][2][0] = box[2][0] ; p_box[ip][2][1] = box[2][1] ;
  } 
  else {  
    const int n = box[ axis ][1] - box[ axis ][0] ;
    const int np_low = np / 2 ;  /* Rounded down */
    const int np_upp = np - np_low ;
 
    const int n_upp = (int) (((double) n) * ( ((double)np_upp) / ((double)np)));
    const int n_low = n - n_upp ;
    const int next_axis = ( axis + 2 ) % 3 ;
 
    if ( np_low ) { /* P = [ip,ip+np_low) */
      BOX dbox ;
      dbox[0][0] = box[0][0] ; dbox[0][1] = box[0][1] ;
      dbox[1][0] = box[1][0] ; dbox[1][1] = box[1][1] ;
      dbox[2][0] = box[2][0] ; dbox[2][1] = box[2][1] ;
 
      dbox[ axis ][1] = dbox[ axis ][0] + n_low ;
 
      box_partition( ip, ip + np_low, next_axis,
                     (const int (*)[2]) dbox, p_box );
    }
 
    if ( np_upp ) { /* P = [ip+np_low,ip+np_low+np_upp) */
      BOX dbox ;
      dbox[0][0] = box[0][0] ; dbox[0][1] = box[0][1] ;
      dbox[1][0] = box[1][0] ; dbox[1][1] = box[1][1] ;
      dbox[2][0] = box[2][0] ; dbox[2][1] = box[2][1] ;
 
      ip += np_low ;
      dbox[ axis ][0] += n_low ;
      dbox[ axis ][1]  = dbox[ axis ][0] + n_upp ;
 
      box_partition( ip, ip + np_upp, next_axis,
                     (const int (*)[2]) dbox, p_box );
    }
  }
}

}

void UnitTestMesh::priv_generate_boxes(
  stk::mesh::BulkData  & mesh ,
  const bool  generate_aura ,
  const int   root_box[][2] ,
        int   local_box[][2] )
{
  const unsigned p_rank = mesh.parallel_rank();
  const unsigned p_size = mesh.parallel_size();
  const unsigned ngx = root_box[0][1] - root_box[0][0] ;
  const unsigned ngy = root_box[1][1] - root_box[1][0] ;
//  const unsigned ngz = root_box[2][1] - root_box[2][0] ;
/*
  const unsigned e_global = ngx * ngy * ngz ;
  const unsigned n_global = ( ngx + 1 ) * ( ngy + 1 ) * ( ngz + 1 );
*/


  BOX * const p_box = new BOX[ p_size ];

  box_partition( 0 , p_size , 2 , root_box , & p_box[0] );

  local_box[0][0] = p_box[ p_rank ][0][0] ;
  local_box[0][1] = p_box[ p_rank ][0][1] ;
  local_box[1][0] = p_box[ p_rank ][1][0] ;
  local_box[1][1] = p_box[ p_rank ][1][1] ;
  local_box[2][0] = p_box[ p_rank ][2][0] ;
  local_box[2][1] = p_box[ p_rank ][2][1] ;

  //const unsigned nx = local_box[0][1] - local_box[0][0] ;
  //const unsigned ny = local_box[1][1] - local_box[1][0] ;
  //const unsigned nz = local_box[2][1] - local_box[2][0] ;

  //const unsigned e_local = nx * ny * nz ;
  //const unsigned n_local = ( nx + 1 ) * ( ny + 1 ) * ( nz + 1 );

  // Create elements:

  std::vector<unsigned> local_count ;

  const stk::mesh::PartVector no_parts ;

  for ( int k = local_box[2][0] ; k < local_box[2][1] ; ++k ) {
  for ( int j = local_box[1][0] ; j < local_box[1][1] ; ++j ) {
  for ( int i = local_box[0][0] ; i < local_box[0][1] ; ++i ) {
    const stk::mesh::EntityId n0 = 1 + (i+0) + (j+0) * (ngx+1) + (k+0) * (ngx+1) * (ngy+1);
    const stk::mesh::EntityId n1 = 1 + (i+1) + (j+0) * (ngx+1) + (k+0) * (ngx+1) * (ngy+1);
    const stk::mesh::EntityId n2 = 1 + (i+1) + (j+1) * (ngx+1) + (k+0) * (ngx+1) * (ngy+1);
    const stk::mesh::EntityId n3 = 1 + (i+0) + (j+1) * (ngx+1) + (k+0) * (ngx+1) * (ngy+1);
    const stk::mesh::EntityId n4 = 1 + (i+0) + (j+0) * (ngx+1) + (k+1) * (ngx+1) * (ngy+1);
    const stk::mesh::EntityId n5 = 1 + (i+1) + (j+0) * (ngx+1) + (k+1) * (ngx+1) * (ngy+1);
    const stk::mesh::EntityId n6 = 1 + (i+1) + (j+1) * (ngx+1) + (k+1) * (ngx+1) * (ngy+1);
    const stk::mesh::EntityId n7 = 1 + (i+0) + (j+1) * (ngx+1) + (k+1) * (ngx+1) * (ngy+1);

    const stk::mesh::EntityId elem_id =  1 + i + j * ngx + k * ngx * ngy;

    stk::mesh::Entity & node0 = mesh.declare_entity( 0 , n0 , no_parts );
    stk::mesh::Entity & node1 = mesh.declare_entity( 0 , n1 , no_parts );
    stk::mesh::Entity & node2 = mesh.declare_entity( 0 , n2 , no_parts );
    stk::mesh::Entity & node3 = mesh.declare_entity( 0 , n3 , no_parts );
    stk::mesh::Entity & node4 = mesh.declare_entity( 0 , n4 , no_parts );
    stk::mesh::Entity & node5 = mesh.declare_entity( 0 , n5 , no_parts );
    stk::mesh::Entity & node6 = mesh.declare_entity( 0 , n6 , no_parts );
    stk::mesh::Entity & node7 = mesh.declare_entity( 0 , n7 , no_parts );
    stk::mesh::Entity & elem  = mesh.declare_entity( 3 , elem_id , no_parts );

    mesh.declare_relation( elem , node0 , 0 );
    mesh.declare_relation( elem , node1 , 1 );
    mesh.declare_relation( elem , node2 , 2 );
    mesh.declare_relation( elem , node3 , 3 );
    mesh.declare_relation( elem , node4 , 4 );
    mesh.declare_relation( elem , node5 , 5 );
    mesh.declare_relation( elem , node6 , 6 );
    mesh.declare_relation( elem , node7 , 7 );
  }
  }
  }

  // Set up sharing:
  mesh.modification_end();

  delete[] p_box ;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------
// Testing for a simple loop of mesh entities.
// node_key[i] : edge_key[i] : node_key[ ( i + 1 ) % node_key.size() ]
/*
void UnitTestMesh::generate_loop(
  BulkData & mesh ,
  const PartVector      & edge_parts ,
  const bool              generate_aura ,
  const unsigned          nPerProc ,
  std::vector<EntityId> & node_ids ,
  std::vector<EntityId> & edge_ids )
{
  const unsigned p_rank = mesh.parallel_rank();
  const unsigned p_size = mesh.parallel_size();
  const unsigned id_total = nPerProc * p_size ;
  const unsigned id_begin = nPerProc * p_rank ;
  const unsigned id_end   = nPerProc * ( p_rank + 1 );
  const unsigned nLocalNode = nPerProc + ( 1 < p_size ? 1 : 0 );
  const unsigned nLocalEdge = nPerProc ;
  const unsigned n_extra = generate_aura && 1 < p_size ? 2 : 0 ;

  node_ids.resize( id_total );
  edge_ids.resize( id_total );
  std::vector<unsigned> local_count ;

  for ( unsigned i = 0 ; i < id_total ; ++i ) {
    node_ids[i] = i + 1;
    edge_ids[i] = i + 1;
  }

  // Create a loop of edges:
  {
    const PartVector no_parts ;
    PartVector add_parts ;

    if ( ! edge_parts.empty() ) { add_parts.resize(1); }

    for ( unsigned i = id_begin ; i < id_end ; ++i ) {
      const unsigned n0 = i ;
      const unsigned n1 = ( i + 1 ) % id_total ;
      if ( ! edge_parts.empty() ) {
        add_parts[0] = edge_parts[ i % edge_parts.size() ];
      }
      Entity & e_node_0 = mesh.declare_entity( 0 , node_ids[n0] , no_parts );
      Entity & e_node_1 = mesh.declare_entity( 0 , node_ids[n1] , no_parts );
      Entity & e_edge   = mesh.declare_entity( 1 , edge_ids[i] , add_parts );
      mesh.declare_relation( e_edge , e_node_0 , 0 );
      mesh.declare_relation( e_edge , e_node_1 , 1 );
    }
  }

  Selector select_owned( mesh.mesh_meta_data().locally_owned_part() );
  Selector select_used( mesh.mesh_meta_data().locally_used_part() );
  Selector select_all(  mesh.mesh_meta_data().universal_part() );

  count_entities( select_used , mesh , local_count );
  STKUNIT_ASSERT( local_count[stk::mesh::Node] == nLocalNode );
  STKUNIT_ASSERT( local_count[stk::mesh::Edge] == nLocalEdge );

  std::vector<Entity*> all_nodes;
  get_entities(mesh, stk::mesh::Node, all_nodes);

  unsigned num_selected_nodes =
      count_selected_entities( select_used, mesh.buckets(stk::mesh::Node) );
  STKUNIT_ASSERT( num_selected_nodes == local_count[stk::mesh::Node] );

  std::vector<Entity*> universal_nodes;
  get_selected_entities( select_all, mesh.buckets(stk::mesh::Node), universal_nodes );
  STKUNIT_ASSERT( universal_nodes.size() == all_nodes.size() );

  UnitTestBulkData::modification_end( mesh , generate_aura );

  // Verify declarations and sharing two end nodes:

  count_entities( select_used , mesh , local_count );
  STKUNIT_ASSERT( local_count[0] == nLocalNode );
  STKUNIT_ASSERT( local_count[1] == nLocalEdge );

  if ( 1 < p_size ) {
    const std::vector<EntityProc> & shared = mesh.shared_entities();

    STKUNIT_ASSERT_EQUAL( shared.size() , size_t(2) );

    const unsigned n0 = id_end < id_total ? id_begin : 0 ;
    const unsigned n1 = id_end < id_total ? id_end : id_begin ;

    STKUNIT_ASSERT( shared[0].first->identifier() == node_ids[n0] );
    STKUNIT_ASSERT( shared[1].first->identifier() == node_ids[n1] );
    STKUNIT_ASSERT_EQUAL( shared[0].first->sharing().size() , size_t(1) );
    STKUNIT_ASSERT_EQUAL( shared[1].first->sharing().size() , size_t(1) );
  }

  // Test no-op first:

  std::vector<EntityProc> change ;

  STKUNIT_ASSERT( mesh.modification_begin() );
  mesh.change_entity_owner( change );
  UnitTestBulkData::modification_end( mesh , generate_aura );

  count_entities( select_used , mesh , local_count );
  STKUNIT_ASSERT( local_count[0] == nLocalNode );
  STKUNIT_ASSERT( local_count[1] == nLocalEdge );

  count_entities( select_all , mesh , local_count );
  STKUNIT_ASSERT( local_count[0] == nLocalNode + n_extra );
  STKUNIT_ASSERT( local_count[1] == nLocalEdge + n_extra );

  // Make sure that edge->owner_rank() == edge->node[1]->owner_rank() 
  if ( 1 < p_size ) {
    Entity * const e_node_0 = mesh.get_entity( 0 , node_ids[id_begin] );
    if ( p_rank == e_node_0->owner_rank() ) {
      EntityProc entry ;
      entry.first = e_node_0 ;
      entry.second = ( p_rank + p_size - 1 ) % p_size ;
      change.push_back( entry );
    }
    STKUNIT_ASSERT( mesh.modification_begin() );
    mesh.change_entity_owner( change );
    UnitTestBulkData::modification_end( mesh , generate_aura );

    count_entities( select_all , mesh , local_count );
    STKUNIT_ASSERT( local_count[0] == nLocalNode + n_extra );
    STKUNIT_ASSERT( local_count[1] == nLocalEdge + n_extra );

    count_entities( select_used , mesh , local_count );
    STKUNIT_ASSERT( local_count[0] == nLocalNode );
    STKUNIT_ASSERT( local_count[1] == nLocalEdge );

    count_entities( select_owned , mesh , local_count );
    STKUNIT_ASSERT( local_count[0] == nPerProc );
    STKUNIT_ASSERT( local_count[1] == nPerProc );
  }
}
*/


} // namespace unit_test
} // namespace stk



