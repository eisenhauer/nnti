#include <iostream>
#include <fstream>

#include "aprepro.h"

// This function is used below in the example showing how an
// application can add its own functions to an aprepro instance.
double succ(double i) {
  return ++i;
}

int main(int argc, char *argv[])
{
  bool readfile = false;

  SEAMS::Aprepro aprepro;
  
  // EXAMPLE: Add a function to aprepro...
  SEAMS::symrec *ptr = aprepro.putsym("succ", SEAMS::Aprepro::FUNCTION, 0);
  ptr->value.fnctptr_d = succ;
  ptr->info = "Return the successor to d";
  ptr->syntax = "succ(d)";
  
  // EXAMPLE: Add a couple variables...
  aprepro.add_variable("Greg", "Is the author of this code", true);  // Make it immutable
  aprepro.add_variable("BirthYear", 1958);
  
  for(int ai = 1; ai < argc; ++ai) {
    std::string arg = argv[ai];
    if (arg == "-i") {
      // Read from cin and echo each line to cout All results will
      // also be stored in Aprepro::parsing_results() stream if needed
      // at end of file.
      aprepro.ap_options.interactive = true;
      bool result = aprepro.parse_stream(std::cin, "standard input");
      if (result) {
	std::cout << "PARSING RESULTS: " << aprepro.parsing_results().str();
      }
    }
    else if (arg[0] == '-') {
      aprepro.set_option(argv[ai]);
    } 
    else {
      // Read and parse a file.  The entire file will be parsed and
      // then the output can be obtained in an std::ostringstream via
      // Aprepro::parsing_results()
      std::fstream infile(argv[ai]);
      if (!infile.good()) {
	std::cerr << "APREPRO: Could not open file: " << argv[ai] << std::endl;
	return 0;
      }

      bool result = aprepro.parse_stream(infile, argv[ai]);
      if (result) {
	std::cout << "PARSING RESULTS: " << aprepro.parsing_results().str();
      }

      readfile = true;
    }
  }

  if (readfile) return 0;
    
  // Read and parse a string's worth of data at a time.
  // Cannot use looping/ifs/... with this method.
  std::string line, tmp;
  while( std::cout << "\nexpression: " &&
   std::getline(std::cin, tmp) &&
   !tmp.empty() ) {

    line += tmp;

    if(*tmp.rbegin() == '\\')
    {
      line.erase(line.length()-1);
      continue;
    }

      line += "\n";
    
    bool result = aprepro.parse_string_interactive(line);

    if (result) {
      std::string res_str = aprepro.parsing_results().str();
      std::cout << "         : " << res_str;


      // Example showing how to get the substitution history for the current line.
      if(aprepro.ap_options.keep_history)
      {
        std::vector<SEAMS::history_data> hist = aprepro.get_history();
        for(size_t i = 0; i < hist.size(); i++)
        {
          SEAMS::history_data curr_history = hist[i];
          std::cout << curr_history.original << " was substituted with " <<
                       curr_history.substitution << " at index " <<
                       curr_history.index << std::endl;
        }

        aprepro.clear_history();
      }
    }

    aprepro.clear_results();

    line.clear();
  }
}
