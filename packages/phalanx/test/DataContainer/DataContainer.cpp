#include "Phalanx_ConfigDefs.hpp"
#include "Phalanx.hpp"

#include "Teuchos_RCP.hpp"
#include "Teuchos_ArrayRCP.hpp"
#include "Teuchos_TestForException.hpp"
#include "Teuchos_Array.hpp"
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
    // Start of Data Container Testing
    // *********************************************************************
    {
      cout << "\nStarting Data Container Testing\n";

      
      cout << "\nConstructing DataContainers...";
      DataContainer< double, MyTraits > dc_scalar;
      DataContainer< MyVector<double>, MyTraits > dc_vector;
      DataContainer< MyTensor<double>, MyTraits > dc_tensor;
      cout << "Passed!" << endl;

      cout << "\nTesting allocateField()...";
      RCP<DataLayout> nodes = rcp(new Generic("nodes",4));
      RCP<FieldTag> d_tag = rcp(new Tag<MyVector<double> >("Density", nodes));
      MyTraits::Allocator allocator;
      dc_vector.allocateField(d_tag, 100, allocator);
      cout << "Passed!" << endl;
      
      cout << "\nTesting getFieldData()...";
      ArrayRCP<MyVector<double> > den = dc_vector.getFieldData(*d_tag);
      TEST_FOR_EXCEPTION(den == Teuchos::null, std::logic_error,
			 "Array is null!");
      cout << "Passed!" << endl;

      cout << "\nPrinting DataContainer:\n\n";
      cout << dc_vector << endl;
      cout << "Printing DataContainer...Passed!" << endl;
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
