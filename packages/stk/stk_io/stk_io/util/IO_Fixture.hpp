#ifndef STK_IO_UTIL_IO_FIXTURE_HPP
#define STK_IO_UTIL_IO_FIXTURE_HPP

#include <stk_util/parallel/Parallel.hpp>

#include <stk_mesh/base/Types.hpp>
#include <stk_mesh/base/BulkData.hpp>

#include <stk_mesh/base/CoordinateSystems.hpp>
#include <stk_mesh/base/MetaData.hpp>

#include <stk_io/IossBridge.hpp>
#include <stk_io/MeshReadWriteUtils.hpp>

#include <stk_util/environment/ReportHandler.hpp>

#include <Teuchos_RCP.hpp>

#include <string>

namespace stk {
namespace io {
namespace util {

/**
 * This purpose of this class is to provide a simple interface for
 * reading/writing an exodus file to/from a "fixture" (a MetaData and BulkData)
 */
class IO_Fixture
{
 public:

  typedef stk::mesh::Field< double, stk::mesh::Cartesian> coord_field_type;

  IO_Fixture(stk::ParallelMachine comm);
  ~IO_Fixture();

  /**
   * Use the mesh defined by the initialized meta and bulk data to create an
   * exodus database. After this has been called, calls to add_timestep_to_output_mesh
   * can be made.
   */
  void create_output_mesh(
    const std::string & base_exodus_filename,
    bool  add_transient = true,
    bool  add_all_fields = false
                          );
  /**
   * Add timestep and write transiant io-fields to exodus file created by last call
   * to create_output_mesh. Assumes that create_output_mesh has
   * been called.
   */
  void add_timestep_to_output_mesh( double time );

  /**
   * Set the input region. Use this if you initialzed meta/bulk data with the
   * setters instead of the initializers.
   */
  void set_input_ioss_region( Teuchos::RCP<Ioss::Region> input_region );

  /**
   * Initialize this fixtures's meta data by reading an input file. Use of this
   * method means you can call initialize_bulk_data to initialize the
   * bulk data of the fixture.
   */
  void initialize_meta_data( const std::string & base_filename,
                             const std::string & type = "exodusii" );

  /**
   * Initialize this fixtures's bulk data by reading an input file. Only call
   * this method if you used initialize_meta_data to initialize this
   * fixture's meta data. The same file will be used to initialize the bulk data
   * as was used to initialize the meta data.
   */
  void initialize_bulk_data();

  stk::mesh::MetaData & meta_data()
  {
    return m_mesh_data.meta_data();
  }

  stk::mesh::BulkData & bulk_data()
  {
    return m_mesh_data.bulk_data();
  }

  stk::io::MeshData & mesh_data()
  {
    return m_mesh_data;
  }

  coord_field_type & get_coordinate_field()
  {
    coord_field_type * coord_field = meta_data().get_field<coord_field_type>("coordinates");
    ThrowRequire( coord_field != NULL);
    return * coord_field;
  }

  Teuchos::RCP<Ioss::Region> input_ioss_region()  { return m_mesh_data.input_io_region(); }
  Teuchos::RCP<Ioss::Region> output_ioss_region() { return m_mesh_data.output_io_region(); }
  void output_ioss_region(Teuchos::RCP<Ioss::Region>);

 private:
  stk::ParallelMachine                       m_comm;
  std::string                                m_mesh_type;
  stk::io::MeshData                          m_mesh_data;

  //disallow copy constructor and assignment operator
  IO_Fixture( const IO_Fixture & );
  IO_Fixture & operator = ( const IO_Fixture & );
};

} // namespace util
} // namespace io
} // namespace stk

#endif //STK_IO_UTIL_IO_FIXTURE_HPP
