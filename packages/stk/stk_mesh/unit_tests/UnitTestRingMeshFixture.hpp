#ifndef STK_MESH_UNITTEST_RING_MESH_FIXTURE_HPP
#define STK_MESH_UNITTEST_RING_MESH_FIXTURE_HPP

#include <stk_mesh/base/Types.hpp>
#include <stk_util/parallel/Parallel.hpp>

class RingMeshFixture
{
 public:
  RingMeshFixture(stk::ParallelMachine pm);

  ~RingMeshFixture();

  stk::mesh::MetaData& meta_data() { return *m_meta_data; }
  stk::mesh::BulkData& bulk_data() { return *m_bulk_data; }

  std::vector<stk::mesh::EntityId> m_node_ids, m_edge_ids;

 private:
  void generate_loop(const stk::mesh::PartVector& edge_parts ,
                     const bool              generate_aura ,
                     const unsigned          nPerProc ,
                     std::vector<stk::mesh::EntityId> & node_ids ,
                     std::vector<stk::mesh::EntityId> & edge_ids);

  stk::mesh::MetaData* m_meta_data;
  stk::mesh::BulkData* m_bulk_data;
};

#endif
