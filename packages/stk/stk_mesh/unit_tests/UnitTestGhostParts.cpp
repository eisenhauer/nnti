/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/


#include <stddef.h>                     // for size_t
#include <stdlib.h>                     // for exit
#include <exception>                    // for exception
#include <iostream>                     // for ostringstream, etc
#include <iterator>                     // for distance
#include <map>                          // for _Rb_tree_const_iterator, etc
#include <stdexcept>                    // for logic_error, runtime_error
#include <stk_mesh/base/BulkData.hpp>   // for BulkData, etc
#include <stk_mesh/base/FieldParallel.hpp>  // for communicate_field_data, etc
#include <stk_mesh/base/GetEntities.hpp>  // for count_entities, etc
#include <stk_util/parallel/Parallel.hpp>  // for ParallelMachine, etc
#include <stk_util/parallel/ParallelReduce.hpp>  // for Reduce, ReduceSum, etc
#include <gtest/gtest.h>
#include <string>                       // for string, basic_string, etc
#include <utility>                      // for pair
#include <vector>                       // for vector, etc
#include "mpi.h"                        // for MPI_COMM_WORLD, MPI_Barrier, etc
#include "stk_mesh/base/Bucket.hpp"     // for Bucket, has_superset
#include "stk_mesh/base/Entity.hpp"     // for Entity
#include "stk_mesh/base/EntityKey.hpp"  // for EntityKey
#include "stk_mesh/base/Field.hpp"      // for Field
#include "stk_mesh/base/FieldBase.hpp"  // for field_data, etc
#include "stk_mesh/base/Ghosting.hpp"   // for Ghosting
#include "stk_mesh/base/MetaData.hpp"   // for MetaData, entity_rank_names, etc
#include "stk_mesh/base/Part.hpp"       // for Part
#include "stk_mesh/base/Relation.hpp"
#include "stk_mesh/base/Selector.hpp"   // for Selector, operator|
#include "stk_mesh/base/Types.hpp"      // for EntityProc, EntityVector, etc
#include "stk_topology/topology.hpp"    // for topology, etc
#include "stk_util/util/PairIter.hpp"   // for PairIter
#include "stk_io/StkMeshIoBroker.hpp"

TEST(UnitTestGhostParts, Aura)
{
  stk::ParallelMachine communicator = MPI_COMM_WORLD;

  int numProcs = 1;
  MPI_Comm_size(communicator, &numProcs);
  if (numProcs != 2) {
    return;
  }

  stk::io::StkMeshIoBroker stkMeshIoBroker(communicator);
  const std::string generatedMeshSpecification = "generated:1x1x3";
  stkMeshIoBroker.add_mesh_database(generatedMeshSpecification, stk::io::READ_MESH);
  stkMeshIoBroker.create_input_mesh();
  stkMeshIoBroker.populate_bulk_data();

  stk::mesh::MetaData &stkMeshMetaData = stkMeshIoBroker.meta_data();
  stk::mesh::BulkData &stkMeshBulkData = stkMeshIoBroker.bulk_data();

  std::cerr<<"about to get aura_part..."<<std::endl;
  stk::mesh::Part& aura_part = stkMeshMetaData.aura_part();
  std::cerr<<"...got aura part with name="<<aura_part.name()<<std::endl;
  stk::mesh::Selector aura_selector = aura_part;
  
  stk::mesh::Ghosting& aura_ghosting = stkMeshBulkData.shared_aura();
  EXPECT_EQ(aura_part.mesh_meta_data_ordinal(), stkMeshBulkData.ghosting_part(aura_ghosting).mesh_meta_data_ordinal());

  stk::mesh::Selector not_owned_nor_shared = !stkMeshMetaData.locally_owned_part() & !stkMeshMetaData.globally_shared_part();

  const stk::mesh::BucketVector& not_owned_nor_shared_node_buckets = stkMeshBulkData.get_buckets(stk::topology::NODE_RANK, not_owned_nor_shared);
  size_t expected_num_not_owned_nor_shared_node_buckets = 1;
  EXPECT_EQ(expected_num_not_owned_nor_shared_node_buckets, not_owned_nor_shared_node_buckets.size());

  const stk::mesh::BucketVector& aura_node_buckets = stkMeshBulkData.get_buckets(stk::topology::NODE_RANK, aura_selector);

  EXPECT_EQ(not_owned_nor_shared_node_buckets.size(), aura_node_buckets.size());

  const size_t expected_num_ghost_nodes = 4;
  size_t counted_nodes = 0;
  size_t counted_aura_nodes = 0;
  for(size_t i=0; i<not_owned_nor_shared_node_buckets.size(); ++i)
  {
      counted_nodes += not_owned_nor_shared_node_buckets[i]->size();
      counted_aura_nodes += aura_node_buckets[i]->size();
  }
  EXPECT_EQ(expected_num_ghost_nodes, counted_nodes);
  EXPECT_EQ(expected_num_ghost_nodes, counted_aura_nodes);
}

TEST(UnitTestGhostParts, Custom1)
{
  stk::ParallelMachine communicator = MPI_COMM_WORLD;

  int numProcs = 1;
  MPI_Comm_size(communicator, &numProcs);
  if (numProcs != 2) {
    return;
  }

  stk::io::StkMeshIoBroker stkMeshIoBroker(communicator);
  const std::string generatedMeshSpecification = "generated:1x1x4";
  stkMeshIoBroker.add_mesh_database(generatedMeshSpecification, stk::io::READ_MESH);
  stkMeshIoBroker.create_input_mesh();
  stkMeshIoBroker.populate_bulk_data();

  stk::mesh::BulkData &stkMeshBulkData = stkMeshIoBroker.bulk_data();

  int myProc = stkMeshBulkData.parallel_rank();
  int otherProc = (myProc == 0) ? 1 : 0;

  stkMeshBulkData.modification_begin();

  stk::mesh::Ghosting& custom_ghosting = stkMeshBulkData.create_ghosting("CustomGhosting1");

  std::vector<stk::mesh::EntityProc> elems_to_ghost;

  const stk::mesh::BucketVector& elem_buckets = stkMeshBulkData.buckets(stk::topology::ELEM_RANK);
  for(size_t i=0; i<elem_buckets.size(); ++i) {
    const stk::mesh::Bucket& bucket = *elem_buckets[i];
    for(size_t j=0; j<bucket.size(); ++j) {
      if (stkMeshBulkData.parallel_owner_rank(bucket[j]) == myProc) {
        elems_to_ghost.push_back(std::make_pair(bucket[j], otherProc));
      }
    }
  }

  stkMeshBulkData.change_ghosting(custom_ghosting, elems_to_ghost);

  stkMeshBulkData.modification_end();

  //now each processor should have 2 elements that were received as ghosts of elements from the other proc.
  const size_t expected_num_elems_for_custom_ghosting = 2;

  stk::mesh::Part& custom_ghost_part = stkMeshBulkData.ghosting_part(custom_ghosting);
  stk::mesh::Selector custom_ghost_selector = custom_ghost_part;

  const stk::mesh::BucketVector& custom_ghost_elem_buckets = stkMeshBulkData.get_buckets(stk::topology::ELEM_RANK, custom_ghost_selector);
  size_t counted_elements = 0;
  for(size_t i=0; i<custom_ghost_elem_buckets.size(); ++i) {
    counted_elements += custom_ghost_elem_buckets[i]->size();
  }

  EXPECT_EQ(expected_num_elems_for_custom_ghosting, counted_elements);
}

