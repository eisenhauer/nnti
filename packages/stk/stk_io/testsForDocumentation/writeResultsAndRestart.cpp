#include <gtest/gtest.h>                // for AssertHelper, EXPECT_EQ, etc
#include <stddef.h>                     // for size_t
#include <unistd.h>                     // for unlink
#include <ostream>                      // for basic_ostream::operator<<
#include <stk_io/StkMeshIoBroker.hpp>   // for StkMeshIoBroker
#include <stk_mesh/base/Field.hpp>      // for Field
#include <stk_mesh/base/GetEntities.hpp>  // for get_entities
#include <stk_mesh/base/MetaData.hpp>   // for MetaData, put_field
#include <string>                       // for string
#include <vector>                       // for vector
#include "Ioss_DBUsage.h"               // for DatabaseUsage::READ_MODEL
#include "Ioss_Field.h"                 // for Field, etc
#include "Ioss_IOFactory.h"             // for IOFactory
#include "Ioss_NodeBlock.h"             // for NodeBlock
#include "Ioss_Property.h"              // for Property
#include "Ioss_Region.h"                // for Region, NodeBlockContainer
#include "stk_io/DatabasePurpose.hpp"   // for DatabasePurpose::READ_MESH, etc
#include "stk_mesh/base/Entity.hpp"     // for Entity
#include "stk_mesh/base/FieldBase.hpp"  // for field_data, FieldBase
#include "stk_mesh/base/FieldState.hpp"  // for FieldState::StateN, etc
#include "stk_topology/topology.hpp"    // for topology, etc
namespace Ioss { class DatabaseIO; }
namespace {

  TEST(StkMeshIoBrokerHowTo, writeResultsAndRestart)
  {
    std::string mesh_name = "input_mesh_example.e";
    std::string results_name = "output.results";
    std::string restart_name = "output.restart";
    MPI_Comm communicator = MPI_COMM_WORLD;

    {
      // ============================================================
      //+ INITIALIZATION:
      //+ Create a basic mesh with a hex block, 3 shell blocks, 3 nodesets, and 3 sidesets.
      stk::io::StkMeshIoBroker stkIo(communicator);

      const std::string generatedFileName = "generated:8x8x8|shell:xyz|nodeset:xyz|sideset:XYZ";
      size_t index = stkIo.add_mesh_database(generatedFileName, stk::io::READ_MESH);
      stkIo.set_active_mesh(index);
      stkIo.create_input_mesh();
      stkIo.populate_bulk_data();

      size_t fh = stkIo.create_output_mesh(mesh_name, stk::io::WRITE_RESULTS);
      stkIo.write_output_mesh(fh);
    }

    {
      //-BEGIN
      // ============================================================
      //+ EXAMPLE: 
      //+ Read mesh data from the specified file.
      stk::io::StkMeshIoBroker stkIo(communicator);
      stkIo.add_mesh_database(mesh_name, stk::io::READ_MESH);

      //+ Creates meta data; creates parts 
      stkIo.create_input_mesh();

      //+ Declare a three-state field
      //+ NOTE: Fields must be declared before "populate_bulk_data()" is called
      //+       since it commits the meta data.
      const std::string fieldName = "disp";
      stk::mesh::Field<double> &field = stkIo.meta_data().declare_field<stk::mesh::Field<double> >(stk::topology::NODE_RANK, fieldName, 3);
      stk::mesh::put_field(field, stkIo.meta_data().universal_part());

      //+ commit the meta data and create the bulk data.  
      //+ populate the bulk data with data from the mesh file.
      stkIo.populate_bulk_data();

      // ============================================================
      //+ Create results file. By default, all parts created from the input
      //+ mesh will be written to the results output file.
      size_t results_fh = stkIo.create_output_mesh(results_name, stk::io::WRITE_RESULTS);

      //+ Create restart file. By default, all parts created from the input
      //+ mesh will be written to the results output file.
      size_t restart_fh = stkIo.create_output_mesh(restart_name, stk::io::WRITE_RESTART);

      //+ The field will be output to the results file with the default field name.
      //+ Only the newest state will be output.
      stkIo.add_field(results_fh, field); 

      //+ Output the field to the retart database also.
      //+ The two newest states will be output.
      stkIo.add_field(restart_fh, field); 

      std::vector<stk::mesh::Entity> nodes;
      stk::mesh::get_entities(stkIo.bulk_data(), stk::topology::NODE_RANK, nodes);

      stk::mesh::FieldBase *statedFieldNp1 = field.field_state(stk::mesh::StateNP1);
      stk::mesh::FieldBase *statedFieldN   = field.field_state(stk::mesh::StateN);
      stk::mesh::FieldBase *statedFieldNm1 = field.field_state(stk::mesh::StateNM1);

      // Iterate the application's execute loop five times and output
      // field data each iteration.
      for (int step=0; step < 5; step++) {
	double time = step;

	// Application execution...
	double value = 10.0 * time;
	for(size_t i=0; i<nodes.size(); i++) {
	  double *np1_data = static_cast<double*>(stk::mesh::field_data(*statedFieldNp1, nodes[i]));
	  *np1_data = value;
	  double *n_data   = static_cast<double*>(stk::mesh::field_data(*statedFieldN,   nodes[i]));
	  *n_data   = value + 0.1;
	  double *nm1_data = static_cast<double*>(stk::mesh::field_data(*statedFieldNm1, nodes[i]));
	  *nm1_data = value + 0.2;
	}

	//+ Results output...
	stkIo.begin_output_step(results_fh, time);
	stkIo.write_defined_output_fields(results_fh);
        stkIo.end_output_step(results_fh);

	//+ Restart output...
	stkIo.begin_output_step(restart_fh, time);
	stkIo.write_defined_output_fields(restart_fh);
        stkIo.end_output_step(restart_fh);
      }
      //-END      
    }
    // ============================================================
    //+ VERIFICATION
    {
      Ioss::DatabaseIO *resultsDb = Ioss::IOFactory::create("exodus", results_name,
							    Ioss::READ_MODEL, communicator);
      Ioss::Region results(resultsDb);
      // Should be 5 steps on database...
      EXPECT_EQ(results.get_property("state_count").get_int(), 5);
      // Should be 1 nodal field on database named "disp";
      Ioss::NodeBlock *nb = results.get_node_blocks()[0];
      EXPECT_EQ(1u, nb->field_count(Ioss::Field::TRANSIENT));
      EXPECT_TRUE(nb->field_exists("disp"));

      // Iterate each step and verify that the correct data was written.
      for (size_t step=0; step < 5; step++) {
	double time = step;

	double db_time = results.begin_state(step+1);
	EXPECT_EQ(time, db_time);
      
	std::vector<double> field_data;
	nb->get_field_data("disp", field_data);
	double expected = 10.0 * time;
	for (size_t node = 0; node < field_data.size(); node++) {
	  EXPECT_EQ(field_data[node], expected);
	}
	results.end_state(step+1);
      }
    }

    {
      Ioss::DatabaseIO *restartDb = Ioss::IOFactory::create("exodus", restart_name,
							    Ioss::READ_MODEL, communicator);
      Ioss::Region restart(restartDb);
      // Should be 5 steps on database...
      EXPECT_EQ(restart.get_property("state_count").get_int(), 5);
      // Should be 2 nodal field on database named "disp" and "disp.N";
      Ioss::NodeBlock *nb = restart.get_node_blocks()[0];
      EXPECT_EQ(2u, nb->field_count(Ioss::Field::TRANSIENT));
      EXPECT_TRUE(nb->field_exists("disp"));
      EXPECT_TRUE(nb->field_exists("disp.N"));

      // Iterate each step and verify that the correct data was written.
      for (size_t step=0; step < 5; step++) {
	double time = step;

	double db_time = restart.begin_state(step+1);
	EXPECT_EQ(time, db_time);
      
	std::vector<double> field_data_n;
	std::vector<double> field_data_np1;
	nb->get_field_data("disp", field_data_np1);
	nb->get_field_data("disp.N", field_data_n);
	double expected = 10.0 * time;
	for (size_t node = 0; node < field_data_n.size(); node++) {
	  EXPECT_EQ(field_data_np1[node], expected);
	  EXPECT_EQ(field_data_n[node],   expected+0.1);
	}
	restart.end_state(step+1);
      }
    }
    
    // ============================================================
    // Cleanup
    unlink(mesh_name.c_str());
    unlink(results_name.c_str());
    unlink(restart_name.c_str());
  }
}
