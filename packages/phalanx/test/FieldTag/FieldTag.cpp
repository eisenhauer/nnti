// @HEADER
// @HEADER

#include <map>

#include "Phalanx_ConfigDefs.hpp"
#include "Phalanx_FieldTag.hpp"
#include "Phalanx_FieldTag_Tag.hpp"
#include "Phalanx_FieldTag_Comparison.hpp"
#include "Phalanx_DataLayout_Generic.hpp"

#include "Teuchos_RCP.hpp"
#include "Teuchos_TestForException.hpp"
#include "Teuchos_TimeMonitor.hpp"

// From test/Utilities directory
#include "Traits.hpp"

int main(int argc, char *argv[]) 
{
  using namespace std;
  using namespace Teuchos;
  using namespace PHX;
  
  try {
    
    RCP<Time> total_time = TimeMonitor::getNewTimer("Total Run Time");
    TimeMonitor tm(*total_time);

    // *********************************************************************
    // Start of Field Tag Testing
    // *********************************************************************
    {

      // Dummy data layouts (same size different name/type)
      RCP<DataLayout> node4 = 
	rcp(new Generic("Q1_Nodes", 4));
      RCP<DataLayout> quad4 = 
	rcp(new Generic("Q1_QuadPoints", 4));
      
      // Allocate tags with same name but different topology
      RCP<FieldTag> rcp_nodal_density = rcp(new Tag<double>("density", node4));
      RCP<FieldTag> rcp_qp_density = rcp(new Tag<double>("density", quad4));
      RCP<FieldTag> rcp_grad_qp_density = 
	rcp(new Tag<MyVector<double> >("density", quad4));

      // Get references to field tags
      FieldTag& nodal_density = *rcp_nodal_density;
      FieldTag& qp_density = *rcp_qp_density;
      FieldTag& grad_qp_density = *rcp_grad_qp_density;
    
      // test ostream
      cout << "Printing field tags" << endl;
      cout << nodal_density << endl;
      cout << qp_density << endl;
      cout << grad_qp_density << endl;
      cout << endl;
      
      // test operator ==
      cout << "Are nodal and qp fields equal (should be false)? = " 
	   << (nodal_density == qp_density) << endl;
      TEST_FOR_EXCEPTION(nodal_density == qp_density, std::logic_error,
			 "operator==() failed!");
      
      // New constructor that should be same as nodal_density
      RCP<FieldTag> rcp_nodal_density_copy = 
	rcp(new Tag<double>("density", node4));
      FieldTag& nodal_density_copy = *rcp_nodal_density_copy;

      cout << "Are nodal and nodal copy fields equal (should be true)? = " 
	   << (nodal_density == nodal_density_copy) << endl;
      TEST_FOR_EXCEPTION(!(nodal_density == nodal_density_copy), 
			 std::logic_error,
			 "operator==() failed for unique copy comparison!");
      
      cout << "Are scalar and vector fields "
	   << "equal (should be false)? = " 
	   << (qp_density == grad_qp_density) << endl;
      TEST_FOR_EXCEPTION(qp_density == grad_qp_density, 
			 std::logic_error,
			 "operator==() failed for data layout comparison !");
      
      // test operator =
      cout << "Testing operator=()...";
      Tag<double> op_eq("Garbage", node4);
      TEST_FOR_EXCEPTION(op_eq == nodal_density, std::logic_error, 
			 "Comparison failed.  Should be different!");
      op_eq = dynamic_cast< Tag<double>& >(nodal_density);
      TEST_FOR_EXCEPTION(op_eq != nodal_density, std::logic_error, 
			 "operator=() failed.  Tags should be the same!");
      cout << "Passed." << endl;

      // name() accessor
      cout << "Testing name() accessor...";
      TEST_FOR_EXCEPTION( (nodal_density.name() != std::string("density") ), 
			 std::logic_error,
			 "name() accessor failed!");
      cout << "Passed." << endl;
      
      // dataLayout() accessor
      const DataLayout& tmp = *node4;
      cout << "Testing dataLayout() accessor...";
      TEST_FOR_EXCEPTION(nodal_density.dataLayout() != tmp, 
			 std::logic_error,
			 "dataLayout() accessor failed!");
      cout << "Passed." << endl;
      
      
      // clone()
      cout << "Testing clone()...";
      RCP<FieldTag> my_copy = rcp_nodal_density->clone();
      TEST_FOR_EXCEPTION( *my_copy != *rcp_nodal_density , 
			 std::logic_error,
			 "name() accessor failed!");
      cout << "Passed." << endl;
      
      // Comparison for map key operations
      cout << "Testing stl std::map with key of RCP<FieldTag>...";
      
      map<RCP<FieldTag>, int, FTComp> my_map;
      my_map[rcp_nodal_density] = 0;
      my_map[rcp_qp_density] = 1;
      my_map[rcp_grad_qp_density] = 2;
      
      RCP<FieldTag> tmp_rcp_nodal_density = 
	rcp(new Tag<double>("density", node4));
      RCP<FieldTag> tmp_rcp_qp_density = 
	rcp(new Tag<double>("density", quad4));
      RCP<FieldTag> tmp_rcp_grad_qp_density = 
	rcp(new Tag<MyVector<double> >("density", quad4));
      
      // Make sure we can create a field tag and access matching map entry
      TEST_FOR_EXCEPTION(my_map[tmp_rcp_nodal_density] != 0,
			 std::logic_error,
			 "Failed to find correct FieldTag(0)!");
      TEST_FOR_EXCEPTION(my_map[tmp_rcp_qp_density] != 1,
			 std::logic_error,
			 "Failed to find correct FieldTag(1)!");
      TEST_FOR_EXCEPTION(my_map[tmp_rcp_grad_qp_density] != 2,
			 std::logic_error,
			 "Failed to find correct FieldTag(2)!");

      cout << "Passed." << endl;
      
    }

    // *********************************************************************
    // *********************************************************************
    std::cout << "\nTest passed!\n" << std::endl; 
    // *********************************************************************
    // *********************************************************************

  }
  catch (const std::exception& e) {
    std::cout << "************************************************" << endl;
    std::cout << "************************************************" << endl;
    std::cout << "Exception Caught!" << endl;
    std::cout << "Error message is below\n " << e.what() << endl;
    std::cout << "************************************************" << endl;
  }
  catch (...) {
    std::cout << "************************************************" << endl;
    std::cout << "************************************************" << endl;
    std::cout << "Unknown Exception Caught!" << endl;
    std::cout << "************************************************" << endl;
  }

  TimeMonitor::summarize();
    
  return 0;
}
