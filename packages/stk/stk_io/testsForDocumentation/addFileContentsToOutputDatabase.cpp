#include <gtest/gtest.h>
#include <string>
#include <mpi.h>
#include <stk_io/StkMeshIoBroker.hpp>
#include <Ioss_SubSystem.h>

namespace {

TEST(StkMeshIoBrokerHowTo, addFileContentsToOutputDatabase)
{
    std::string filename = "information_records.e";
    MPI_Comm communicator = MPI_COMM_WORLD;

    //-BEGIN
    // ============================================================
    //+ SETUP
    std::string input_file = "application_input_file.i";
    std::string info1("This is the first line of the input file.");
    std::string info2("This is the second line of the input file. "
		      "It is longer than 80 characters, so it should be wrapped.");
    std::string info3("This is the third line of the input file.");
    std::string info4("This is the fourth and last line of the input file.");

    std::string additional_info_record = "This is an info record added explicitly,"
                                         " not from the input file.";
    {
      std::ofstream my_file(input_file.c_str());
      my_file << info1 <<"\n" << info2 <<"\n" << info3 <<"\n" << info4 <<"\n";
    }
    
    {
      // ============================================================
      //+ EXAMPLE
      stk::io::StkMeshIoBroker stkIo(communicator);
      size_t ifh = stkIo.add_mesh_database("9x9x9|shell:xyzXYZ", "generated",
					       stk::io::READ_MESH);
      stkIo.set_active_mesh(ifh);
      stkIo.create_input_mesh();
      stkIo.populate_bulk_data();

      // Output...
      size_t fh = stkIo.create_output_mesh(filename,
					       stk::io::WRITE_RESULTS);
      Ioss::Region *io_reg = stkIo.get_output_io_region(fh).get();

      //+ Add the data from the file "application_input_file.i"
      //+    as information records on this file.
      io_reg->property_add(Ioss::Property("input_file_name",input_file)); /*@\label{io:info:file}*/

      //+ Add the data from the "additional_info_record" vector as
      //+    information records on this file.
      io_reg->add_information_record(additional_info_record); /*@\label{io:info:vector}*/
      
      stkIo.write_output_mesh(fh);
      // ... Verification deleted
      //-END
    }

    // ============================================================
    // VERIFICATION 

    // Verify output mesh contains the data in
    // 'input_file' as information records...  Note that
    // the output mesh will contain all element blocks; however, the
    // non-shell element block will have zero elements.  This is due
    // to the subset_selector subsetting the entities and not the
    // parts...
    Ioss::DatabaseIO *iossDb = Ioss::IOFactory::create("exodus", filename,
						       Ioss::READ_MODEL, communicator);
    Ioss::Region ioRegion(iossDb);

    const std::vector<std::string> &info_records =
      ioRegion.get_information_records();
    // First 2 lines of info records are host information (node name,
    // os version) (2) Next record is the file name of the input file
    // data that follows (1) File contains 4 records; 1 is longer than
    // 80 characters, so it wraps (4+1) Next line is the
    // "additional_info_record" added above (1)
    size_t expected_info_record_count = 2 + (4+1)  + 1 + 1;
    EXPECT_EQ(expected_info_record_count, info_records.size());

    EXPECT_STREQ(input_file.c_str(), info_records[2].c_str());
    
    EXPECT_STREQ(info1.c_str(), info_records[3].c_str());
    EXPECT_STREQ(info2.substr(0,79).c_str(), info_records[4].substr(0,79).c_str());
    EXPECT_STREQ(info2.substr(79).c_str(), info_records[5].c_str());
    EXPECT_STREQ(info3.c_str(), info_records[6].c_str());
    EXPECT_STREQ(info4.c_str(), info_records[7].c_str());
    EXPECT_STREQ(additional_info_record.c_str(), info_records[8].c_str());
    
    unlink(filename.c_str());
    unlink(input_file.c_str());
}

}
//TODO: 
//. Check only on processor 0
