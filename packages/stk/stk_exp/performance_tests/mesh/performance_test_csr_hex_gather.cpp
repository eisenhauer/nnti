#ifndef __IBMCPP__
#include <gtest/gtest.h>

#include <sierra/mesh/fixture/hex_fixture.hpp>
#include <sierra/mesh/fixture/csr_mesh_factory.hpp>
#include <sierra/mesh/csr/csr_mesh.hpp>
#include <sierra/mesh/details/selected_buckets.hpp>

#include <performance_tests/mesh/calculate_centroid.hpp>

#include <stk_util/environment/CPUTime.hpp>

#include <iostream>

using namespace sierra::mesh;
using namespace sierra::mesh::details;

namespace {

void gather_hex( const selector & element_select,
                 const fixture::hex_fixture::CoordinateField & coordinates,
                 const csr_mesh & mesh,
                 std::vector<double> & avg_centroid )
{
  std::vector<double> elem_centroid(3,0);
  std::vector<double> elem_node_coords(8*3,0);

  const entity_rank node_rank(0);

  BOOST_FOREACH( bucket_key bucket, get_selected_buckets( element_select, mesh)) {
    BOOST_FOREACH( entity_descriptor elem, mesh.get_entities(bucket)) {
      const size_t num_nodes = mesh.num_relations(elem,node_rank);
      elem_node_coords.resize(num_nodes*3);
      size_t offset = 0;
      BOOST_FOREACH( csr_mesh::relation_descriptor node_relation, mesh.get_relations(elem,node_rank)) {
        entity_descriptor node = mesh.target_entity(node_relation);
        double * node_coords = coordinates[mesh.get_bucket_location(node)];
        elem_node_coords[offset++] = node_coords[0];
        elem_node_coords[offset++] = node_coords[1];
        elem_node_coords[offset++] = node_coords[2];
      }
      performance_tests::calculate_centroid_3d(num_nodes,&elem_node_coords[0],&elem_centroid[0]);

      //add this element-centroid to the avg_centroid vector, and
      //re-zero the element-centroid vector:
      avg_centroid[0] += elem_centroid[0]; elem_centroid[0] = 0;
      avg_centroid[1] += elem_centroid[1]; elem_centroid[1] = 0;
      avg_centroid[2] += elem_centroid[2]; elem_centroid[2] = 0;
    }
  }
}

}

TEST( CSR, gather_centroid_hex_elem)
{
  double start_time = stk::cpu_time();
#ifndef NDEBUG
  unsigned ex=50,  ey=50,  ez=50; // make things smaller in debug so tests don't timeout
#else
  unsigned ex=100, ey=100, ez=100;
#endif
  unsigned num_elems = ex*ey*ez;
  fixture::hex_fixture fixture(ex,ey,ez);
  fixture.generate_mesh();
  double end_time = stk::cpu_time() - start_time;

  std::cout << "hex_fixture: "<<std::endl;
  std::cout << "\tNum Nodes: " << fixture.m_num_nodes<<std::endl;
  std::cout << "\tNum Elements: " << fixture.m_num_elements << std::endl;
  std::cout << "Time to create hex (modifiable) mesh: " << end_time << std::endl;

  start_time = stk::cpu_time();
  boost::shared_ptr<csr_mesh> mesh = csr_mesh_factory::create_from_modifiable(fixture.m_mesh);
  end_time = stk::cpu_time() - start_time;

  std::cout << "Time to convert to CSR mesh: " << end_time << std::endl;

  std::vector<double> avg_centroid(3,0);
  const double tolerance = 1.e-6;
  const double expected = ((double)ex)/2;

  start_time = stk::cpu_time();

  const int num_iters = 100;
  for(int t=0; t<num_iters; ++t) {
    gather_hex(fixture.m_hex_part&!fixture.m_node_part, fixture.m_coordinates, *mesh, avg_centroid);

    for(size_t d=0; d<3u; ++d) {
      EXPECT_LT(std::abs(avg_centroid[d]/num_elems - expected), tolerance);
    }

    avg_centroid[0] = 0;
    avg_centroid[1] = 0;
    avg_centroid[2] = 0;
  }

  end_time = stk::cpu_time();

  double test_time = end_time - start_time;
  std::cout << "Time to compute centroids ("<<num_iters<<" iters): " << test_time << std::endl;
}

#endif
