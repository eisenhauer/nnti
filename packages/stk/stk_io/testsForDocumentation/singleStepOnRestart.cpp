#include <gtest/gtest.h>                // for AssertHelper, EXPECT_EQ, etc
#include <stddef.h>                     // for size_t
#include <unistd.h>                     // for unlink
#include <ostream>                      // for basic_ostream::operator<<
#include <stk_io/StkMeshIoBroker.hpp>   // for StkMeshIoBroker
#include <stk_mesh/base/Field.hpp>      // for Field
#include <stk_mesh/base/MetaData.hpp>   // for MetaData, put_field
#include <string>                       // for string
#include "Ioss_DBUsage.h"               // for DatabaseUsage::READ_RESTART
#include "Ioss_DatabaseIO.h"            // for DatabaseIO
#include "Ioss_IOFactory.h"             // for IOFactory
#include "Ioss_Property.h"              // for Property
#include "Ioss_Region.h"                // for Region
#include "Teuchos_RCP.hpp"              // for RCP::operator->
#include "Teuchos_RCPDecl.hpp"          // for RCP
#include "stk_io/DatabasePurpose.hpp"   // for DatabasePurpose::READ_MESH, etc
#include "stk_topology/topology.hpp"    // for topology, etc

namespace {
  TEST(StkMeshIoBrokerHowTo, singleStepOnRestart)
  {
    // ============================================================
    // INITIALIZATION...

    std::string filename = "single_step.restart";
    MPI_Comm comm = MPI_COMM_WORLD;

    stk::io::StkMeshIoBroker stkIo(comm);
    const std::string exodusFileName = "generated:1x1x8";
    stkIo.add_mesh_database(exodusFileName, stk::io::READ_MESH);
    stkIo.create_input_mesh();
      
    stk::mesh::Field<double> &field = stkIo.meta_data().declare_field<stk::mesh::Field<double> >(stk::topology::NODE_RANK, "disp", 3);
    stk::mesh::put_field(field, stkIo.meta_data().universal_part());

    stkIo.populate_bulk_data();

    {
      //-BEGIN
      // ... Setup deleted
      // ============================================================
      // EXAMPLE USAGE...
      // Create a restart file,
      size_t fh = stkIo.create_output_mesh(filename,
					   stk::io::WRITE_RESTART);
      stkIo.add_field(fh, field);

      //+ Set the cycle count to 1.  This will result in a maximum
      //+ of one step on the output database -- when a new step is
      //+ added, it will overwrite the existing step.
      stkIo.get_output_io_region(fh)->get_database()->set_cycle_count(1); /*@\label{io:cycle}*/
	
      // Write multiple steps to the restart file.
      for (size_t step=0; step < 3; step++) {
	double time = step;
	stkIo.begin_output_step(fh, time);
	stkIo.write_defined_output_fields(fh);
	stkIo.end_output_step(fh);
      }

      //+ At this point, there should only be a single state on the
      //+ restart database. The time of this state should be 2.0.
      // ... Verification deleted
      //-END
    }

    {
      // ============================================================
      //  VERIFICATION:
      //+ There should only be a single state on the restart database.
      //+ The time of this state should be 2.0.
      Ioss::DatabaseIO *iossDb =
	Ioss::IOFactory::create("exodus", filename,
				Ioss::READ_RESTART, comm);
      Ioss::Region region(iossDb);

      EXPECT_EQ(region.get_property("state_count").get_int(), 1);
      EXPECT_EQ(region.get_state_time(1), 2.0);
    }
    
    // =============================================================
    // CLEANUP:
    unlink(filename.c_str());
  }
}
