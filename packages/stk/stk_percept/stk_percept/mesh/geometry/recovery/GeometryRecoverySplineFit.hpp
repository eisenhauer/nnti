#ifndef GeometryRecoverySplineFit_hpp
#define GeometryRecoverySplineFit_hpp

#include <stk_percept/PerceptMesh.hpp>

#if defined( STK_PERCEPT_HAS_GEOMETRY )
#include <stk_percept/mesh/geometry/kernel/GeometryKernelOpenNURBS.hpp>
#include <stk_percept/mesh/geometry/stk_geom/LocalCubicSplineFit.hpp>
#endif

namespace stk {
  namespace percept {

    /// fit a OpenNURBS spline to each side set found in the mesh, then write a .3dm file with the geometry info
    class GeometryRecoverySplineFit {
    public:
      GeometryRecoverySplineFit(PerceptMesh& eMesh) : m_eMesh(eMesh) {}

#if defined( STK_PERCEPT_HAS_GEOMETRY )
      typedef std::set<stk::mesh::Entity, stk::mesh::EntityLess> SetOfEntities;

      /// after opening a mesh, but before commit, call this to find and create the topology parts on the meta data
      ///   needed to associate geometry with the mesh
      /// usage: see AdaptMain for sample usage
      void fit_geometry_create_parts_meta();

      /// after commit, call this with a filename to create the actual geometry file
      void fit_geometry(const std::string& filename);

    protected:
      stk::mesh::Part& create_clone_part(const std::string& clone_name, const stk::mesh::EntityRank rank_to_clone, bool make_part_io_part=true );
      stk::mesh::Part& clone_part_bulk(const stk::mesh::Part& part, const std::string& clone_name, const stk::mesh::EntityRank rank_to_clone, bool make_part_io_part=true );

      /// returns true if closed found - returns multiple closed loops if they exist
      void get_sorted_curve_node_entities_and_split(stk::mesh::Part& part, std::vector< std::vector<stk::mesh::Entity> >& sorted_nodes_all, std::vector<bool>& isClosed_all);


    private:
      /// returns true if closed found - finds first closed loop if multiple loops - @see get_sorted_curve_node_entities_and_split

      bool get_sorted_curve_node_entities(stk::mesh::Part& part, stk::mesh::Entity edge_first, std::vector<stk::mesh::Entity>& sorted_nodes);

      //int check_for_corners(Vectors2D& Q, double alpha, std::vector<int>& corners);
#endif

    private:
      PerceptMesh& m_eMesh;
      stk::mesh::PartVector m_surface_parts;
      stk::mesh::PartVector m_topo_parts;
    };

  }
}
#endif
