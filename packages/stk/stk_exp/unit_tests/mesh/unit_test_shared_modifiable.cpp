#ifndef __IBMCPP__
#include <sierra/mesh/modifiable/modifiable_mesh.hpp>
#include <sierra/mesh/details/cell_topology.hpp>

#include <boost/mpi/communicator.hpp>
#include <boost/range.hpp>

#include <gtest/gtest.h>

using namespace sierra::mesh;
using namespace sierra::mesh::details;

TEST( modifiable_mesh , basic_shared )
{
  boost::mpi::communicator mpi_world;

  //only run this test if num-procs == 2
  if (mpi_world.size() != 2) {
    return;
  }

  modifiable_mesh mesh;

  entity_key shared_entity;

  //create a super-simple mesh with entities 1,2,3 where
  //proc 0 has 1,2 and proc 1 has 2,3 (entity 2 is shared)
  //
  //  1      2      3
  //  *------*------*
  //     P0     P1
  //

  if (mpi_world.rank() == 0) {
    entity_id id1(1);
    mesh.add_entity(entity_property(mesh.node_rank(), id1, mpi_world.rank()));
    entity_id id2(2);
    shared_entity = mesh.add_entity(entity_property(mesh.node_rank(), id2, mpi_world.rank()));
  }
  else {
    entity_id id2(2);
    shared_entity = mesh.add_entity(entity_property(mesh.node_rank(), id2, mpi_world.rank()));
    entity_id id3(3);
    mesh.add_entity(entity_property(mesh.node_rank(), id3, mpi_world.rank()));
  }

  part_key shared_part = mesh.shared_part();

  //put the shared entity into the shared part.
  mesh.change_entity_parts(shared_entity, &shared_part, &shared_part+1);

  //proc 0 is the "owner" of the shared entity
  mesh.set_owner_proc(shared_entity, 0);

  if (mpi_world.rank()==0) {
    int sharing_proc = 1;
    mesh.add_sharing_procs(shared_entity, &sharing_proc, &sharing_proc+1);
  }

  //each proc has 2 entities
  EXPECT_EQ( 2u, mesh.num_entities() );

  modifiable_mesh::sharing_proc_range sharing_procs = mesh.get_sharing_procs(shared_entity);

  if (mpi_world.rank()==0) {
    EXPECT_EQ(1u, boost::distance(sharing_procs));
  }
  if (mpi_world.rank()==1) {
    EXPECT_EQ(0u, boost::distance(sharing_procs));
  }

  //count the entities in the shared part.
  modifiable_mesh::bucket_set buckets = mesh.get_buckets(shared_part);
  size_t num_shared_entities = 0;
  for(modifiable_mesh::bucket_set::iterator it=buckets.begin(), end=buckets.end(); it!=end; ++it) {
    modifiable_mesh::bucket_entity_range entities = mesh.get_entities(*it);
    num_shared_entities += boost::distance(entities);
  }

  EXPECT_EQ(1u, num_shared_entities);
}

TEST( modifiable_mesh , shared_procs )
{
  boost::mpi::communicator mpi_world;

  //only run this test if num-procs == 2
  if (mpi_world.size() != 2) {
    return;
  }

  modifiable_mesh mesh;

  entity_key shared_entity;

  //create a simple mesh containing just the nodes of
  //the following 2D quad mesh:
  //          2
  // 1*-------*------*3
  //  |  P0   |  P0  |
  // 4*------5*------*6
  //  |  P1   |  P1  |
  // 7*-------*------*9
  //          8
  //
  //proc 0 has the top two elements and proc 1
  //has the bottom two elements.
  //Nodes 4,5,6 are shared by both procs, and we
  //arbitrarily choose proc 0 to own nodes 4 and 5,
  //while proc 1 owns node 6.

  part_key shared_part = mesh.shared_part();

  if (mpi_world.rank() == 0) {
    for(size_t i=1; i<=6; ++i) {
      entity_id id(i);
      entity_key key = mesh.add_entity(entity_property(mesh.node_rank(), id, mpi_world.rank()));
      if (i>=4 && i<=6) {
        mesh.change_entity_parts(key, &shared_part, &shared_part+1);
        if (i==6) {
          mesh.set_owner_proc(key, 1);
        }
        else {
          int sharing_proc = 1;
          mesh.add_sharing_procs(key, &sharing_proc, &sharing_proc+1);
        }
      }
    }
  }
  else {
    for(size_t i=4; i<=9; ++i) {
      entity_id id(i);
      entity_key key = mesh.add_entity(entity_property(mesh.node_rank(), id, mpi_world.rank()));
      if (i>=4 && i<=6) {
        mesh.change_entity_parts(key, &shared_part, &shared_part+1);
        if (i!=6) {
          mesh.set_owner_proc(key, 0);
        }
        else {
          int sharing_proc = 0;
          mesh.add_sharing_procs(key, &sharing_proc, &sharing_proc+1);
        }
      }
    }
  }

  //each proc has 6 nodes
  EXPECT_EQ( 6u, mesh.num_entities() );

  //count the entities in the shared part.
  modifiable_mesh::bucket_set buckets = mesh.get_buckets(shared_part);
  size_t num_shared_entities = 0;
  for(modifiable_mesh::bucket_set::iterator it=buckets.begin(), end=buckets.end(); it!=end; ++it) {
    modifiable_mesh::bucket_entity_range entities = mesh.get_entities(*it);
    for(modifiable_mesh::bucket_entity_iterator entity=boost::begin(entities), ent_end=boost::end(entities); entity!=ent_end; ++entity) {
      ++num_shared_entities;
      entity_key ent_key = *entity;
      entity_id id = mesh[ent_key].m_id;
      if (id == entity_id(4) || id == entity_id(5)) {
        EXPECT_EQ(0, mesh.get_owner_proc(ent_key));
      }
      if (id == entity_id(6)) {
        EXPECT_EQ(1, mesh.get_owner_proc(ent_key));
      }

      //for this two-proc test, the number of sharing procs for
      //a shared entity that we own, should be 1.
      if (mesh.get_owner_proc(*entity) == mpi_world.rank()) {
        EXPECT_EQ(1u, boost::distance(mesh.get_sharing_procs(ent_key)));
      }
    }
  }

  EXPECT_EQ(3u, num_shared_entities);
}

#endif
