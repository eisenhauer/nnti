#ifndef stk_io_util_Gmesh_STKmesh_Fixture_hpp
#define stk_io_util_Gmesh_STKmesh_Fixture_hpp

#include <stddef.h>                     // for NULL
#include <stk_io/StkMeshIoBroker.hpp>   // for StkMeshIoBroker
#include <stk_mesh/base/Types.hpp>      // for PartVector
#include <stk_util/parallel/Parallel.hpp>  // for ParallelMachine
#include <string>                       // for string
#include <vector>                       // for vector
namespace stk { namespace mesh { class BulkData; } }
namespace stk { namespace mesh { class MetaData; } }
namespace stk { namespace mesh { struct ConnectivityMap; } }


namespace stk {
namespace io {
namespace util {

/**
 * This class implements a Stk-mesh based fixture that uses a generated
 * mesh as the basis of the fixture.
 */
class Gmesh_STKmesh_Fixture
{
 public:

  /**
   * Construct a fixture. Note that the fixture won't be completed until commit
   * is called; the intent is to give the client a chance to make additional
   * changes to the meta-data.
   *
   * @param comm The comm object for all processors using the fixture
   * @param gmesh_spec The specification for the mesh. See Iogn::GeneratedMesh
   * for documentation on how to specify meshes.
   */
  Gmesh_STKmesh_Fixture(   stk::ParallelMachine comm
                         , const std::string& gmesh_spec
                         , bool use_64bit_int_IO_api=false
                         , stk::mesh::ConnectivityMap * connectivity_map = NULL
                       );

  /**
   * Commits the meta-data of the mesh and populates the bulk-data. Don't call
   * this until you are done modifying the meta-data.
   */
  void commit();

  /**
   * Get the names of all the sideset parts.
   */
  const std::vector<std::string> & getSidesetNames() const
  { return m_sideset_names; }

  /**
   * Get all the sideset parts.
   */
  const stk::mesh::PartVector & getSideParts() const
  { return m_sideset_parts; }

  /**
   * Get a reference to the meta data for the stk-mesh.
   */
  const stk::mesh::MetaData & getMetaData() const
  { return m_mesh_data.meta_data(); }

  stk::mesh::MetaData & getMetaData()
  { return m_mesh_data.meta_data(); }

  /**
   * Get a reference to the bulk data for the stk-mesh.
   */
  const stk::mesh::BulkData & getBulkData() const
  { return m_mesh_data.bulk_data(); }

  stk::mesh::BulkData & getBulkData()
  { return m_mesh_data.bulk_data(); }

 private:
  /**
   * The mesh-data for the mesh. This is a special object that maintains some
   * state between the meta data and bulk data portions of the mesh generation
   * process for use cases.
   */
  stk::io::StkMeshIoBroker m_mesh_data;

  ///> The names of all the side parts
  std::vector<std::string> m_sideset_names;

  ///> Collection of all the side parts
  stk::mesh::PartVector m_sideset_parts;
};

}//namespace util
}//namespace io
}//namespace stk

#endif
