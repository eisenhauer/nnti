#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <mpi.h>
#include <stk_util/util/ParameterList.hpp>
#include <stk_io/StkMeshIoBroker.hpp>

namespace
{
  TEST(StkMeshIoBrokerHowTo, writeHeartbeat)
  {

    const std::string file_name = "Heartbeat.txt";
    MPI_Comm communicator = MPI_COMM_WORLD;
    int my_processor = 0;
    MPI_Comm_rank(communicator, &my_processor);

    //-BEGIN
    stk::util::ParameterList params;
    
    {
      // ============================================================
      // INITIALIZATION...
      // Add some params to write and read...
      params.set_param("PI", -3.14159);  // Double 
      params.set_param("Answer", 42);   // Integer

      std::vector<double> my_vector;
      my_vector.push_back(2.78);
      my_vector.push_back(5.30);
      my_vector.push_back(6.21);
      params.set_param("some_doubles", my_vector);   // Vector of doubles
    
      std::vector<int> ages;
      ages.push_back(55);
      ages.push_back(49);
      ages.push_back(21);
      ages.push_back(19);
    
      params.set_param("Ages", ages);   // Vector of integers
    }

    {
      // ============================================================
      // EXAMPLE USAGE...
      // Begin use of stk io heartbeat file...
      stk::io::StkMeshIoBroker stkIo(communicator);

      //+ Define the heartbeat output to be in TEXT format.
      size_t hb = stkIo.add_heartbeat_output(file_name, stk::io::TEXT); /*@\label{io:hb:add_heartbeat_output}*/

      stk::util::ParameterMapType::const_iterator i = params.begin(); /*@\label{io:hb:begin_add}*/
      stk::util::ParameterMapType::const_iterator iend = params.end();
      for (; i != iend; ++i) {
	const std::string paramName = (*i).first;
	stk::util::Parameter &param = params.get_param(paramName);

	//+ Tell heartbeat which variables to output at each step...
	stkIo.add_heartbeat_global(hb, paramName, param.value, param.type);
      }/*@\label{io:hb:end_add}*/

      // Now output the global variables...
      int timestep_count = 1;
      double time = 0.0;
      for (int step=1; step <= timestep_count; step++) {
	//+ Now output the global variables...
	stkIo.process_heartbeat_output(hb, step, time);/*@\label{io:hb:output}*/
      }
    }
    //-END

    if (my_processor == 0) { // Heartbeat is only output on processor 0.
      // ============================================================
      // VERIFICATION:
      // open the heartbeat file...
      std::ifstream heartbeat(file_name.c_str());
      std::string header_line;
      std::string data_line;

      std::string expected_header_line = "        Time	      Ages_1	      Ages_2	      Ages_3	      Ages_4	      Answer	          PI	some_doubles_1	some_doubles_2	some_doubles_3";
      std::string expected_data_line = " 0.00000e+00	          55	          49	          21	          19	          42	-3.14159e+00	 2.78000e+00	 5.30000e+00	 6.21000e+00";

      EXPECT_TRUE(!std::getline(heartbeat, header_line).fail());
      EXPECT_STREQ(header_line.c_str(), expected_header_line.c_str());
      EXPECT_TRUE(!std::getline(heartbeat, data_line).fail());
      EXPECT_STREQ(data_line.c_str(), expected_data_line.c_str());

      // ============================================================
      // CLEANUP:
      unlink(file_name.c_str());
    }
  }
}
