/**   ------------------------------------------------------------
 *    Copyright 2001 - 2010 Sandia Corporation.
 *    Under the terms of Contract DE-AC04-94AL85000, there is a
 *    non-exclusive license for use of this work by or on behalf
 *    of the U.S. Government.  Export of this program may require
 *    a license from the United States Government.
 *    ------------------------------------------------------------
 */

#include <stk_util/diag/Env.hpp>
#include <limits.h>                     // for PATH_MAX
#include <stddef.h>                     // for size_t
#include <unistd.h>                     // for getcwd, sleep
#include <cstdlib>                      // for exit, EXIT_FAILURE, NULL
#include <cstring>                      // for strlen, strcpy
#include <iomanip>                      // for operator<<, setw
#include <iostream>                     // for cerr, cout
#include <map>                          // for map<>::mapped_type
#include <sstream>                      // for basic_ostream, operator<<, etc
#include <stk_util/diag/Exception.hpp>  // for RuntimeError
#include <stk_util/diag/Signal.hpp>     // for HUP_received
#include <stk_util/diag/StringUtil.hpp>  // for format_time
#include <stk_util/diag/mpih.hpp>       // for Sub_Communicator
#include <stk_util/environment/EnvData.hpp>  // for EnvData, etc
#include <stk_util/environment/ProductRegistry.hpp>
#include <stk_util/environment/ProgramOptions.hpp>
#include <stk_util/environment/RuntimeMessage.hpp>
#include <stk_util/parallel/ParallelReduce.hpp>  // for all_write_string
#include <string>                       // for string, operator<<, etc
#include "boost/program_options/detail/parsers.hpp"
#include "boost/program_options/errors.hpp"  // for program_options
#include "boost/program_options/variables_map.hpp"  // for variables_map, etc
#include "mpi.h"                        // for MPI_COMM_NULL, MPI_Comm, etc
#include "stk_util/environment/ReportHandler.hpp"  // for ThrowRequire
namespace boost { namespace program_options { class options_description; } }





using namespace std;

namespace sierra {
namespace Env {

  //
  //  Set or get the gemini version, if passed value is not unknown, set the version, either way return the version
  //
  GeminiSCIVersion GetGeminiVersion(GeminiSCIVersion ver) {
    static GeminiSCIVersion GeminiSCIVersionValue = GEMINI_SCI_1;  //This is the default gemini verion
    if(ver != GEMINI_SCI_UNKNOWN) {
      GeminiSCIVersionValue = ver;
    }
    ThrowRequire(GeminiSCIVersionValue != GEMINI_SCI_UNKNOWN);
    return GeminiSCIVersionValue;
  }
    


const std::string &
product_name()
{
  return stk::EnvData::instance().m_productName;
}


const std::string &
executable_file()
{
  return stk::EnvData::instance().m_executablePath;
}


const std::string &
executable_date()
{
  static std::string executable_date;

  if (executable_date.empty())
    executable_date = ProductRegistry::instance().getProductAttribute(stk::EnvData::instance().m_productName, ProductRegistry::BUILD_TIME);

  return executable_date;
}


const std::string &
startup_date()
{
  static std::string startup_date;

  if (startup_date.empty())
    startup_date = format_time(stk::EnvData::instance().m_startTime).c_str();

  return startup_date;
}


double
start_time()
{
  return stk::EnvData::instance().m_startTime;
}


bool
developer_mode()
{
  return !get_param("developer-mode").empty();
}


void setInputFileName(std::string name) {
  stk::EnvData::instance().m_inputFile = name;
}

std::string getInputFileName() {
  return stk::EnvData::instance().m_inputFile;
}

void set_input_file_required(bool value)
{
    stk::EnvData::instance().m_inputFileRequired = value;
}

void set_check_subcycle(bool value)
{
    stk::EnvData::instance().m_checkSubCycle = value;
}

void set_zapotec(bool value)
{
    stk::EnvData::instance().m_isZapotec = value;
}

bool is_zapotec()
{
    return stk::EnvData::instance().m_isZapotec;
}


const std::string &
architecture()
{
  return get_param("architecture");
}


const std::string
working_directory() {
  char cwd[PATH_MAX];
  std::string directory = get_param("directory");
  if (directory[0] != '/' && getcwd(cwd, PATH_MAX) != NULL) {
    directory = cwd;
    directory += '/';
  }
  return directory;
}


std::ostream &
output()
{
  return stk::EnvData::instance().m_output;
}


std::ostream &
outputP0()
{
  return *stk::EnvData::instance().m_outputP0;
}


std::ostream &
outputNull() {
  return stk::EnvData::instance().m_outputNull;
}


const char *
section_separator()
{
  static const char *s_sectionSeparator = "+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----";

  return s_sectionSeparator;
}


const char *
subsection_separator()
{
  static const char *s_subsectionSeparator = "---------------------------------------------------";

  return s_subsectionSeparator;
}


std::string
section_title(
  const std::string &	title)
{
  static size_t s_sectionSeparatorLength = std::strlen(section_separator());

  std::ostringstream strout;

  strout << std::left << std::setw(s_sectionSeparatorLength - 20) << title << std::right << std::setw(20) << format_time(Env::wall_now());
  return strout.str();
}


int parallel_size() {
  return stk::EnvData::instance().m_parallelSize;
}

int parallel_rank() {
  return stk::EnvData::instance().m_parallelRank;
}

MPI_Comm
parallel_comm()
{
  return stk::EnvData::instance().m_parallelComm;
}

MPI_Comm
parallel_world_comm()
{
  return stk::EnvData::instance().m_worldComm;
}

int parallel_lag_master() {
  return stk::EnvData::instance().m_execMap[EXEC_TYPE_LAG].m_master;
}

int parallel_fluid_master() {
  return stk::EnvData::instance().m_execMap[EXEC_TYPE_FLUID].m_master;
}

int peer_group() {
  return stk::EnvData::instance().m_execMap[EXEC_TYPE_PEER].m_master;
}

bool
is_comm_valid()
{
  stk::EnvData &env_data = stk::EnvData::instance();
  if (env_data.m_parallelComm == MPI_COMM_NULL) {
    return false;
  } else {
    return true;
  }
}

void
reset(
  MPI_Comm		new_comm)
{
  stk::EnvData &env_data = stk::EnvData::instance();

  // Destroy old comm
  if (env_data.m_parallelComm != MPI_COMM_NULL) {

    if (new_comm != MPI_COMM_NULL) {
      mpih::Sub_Communicator(env_data.m_parallelComm, new_comm);
    }

    env_data.m_parallelComm = MPI_COMM_NULL ;
    env_data.m_parallelSize = -1;
    env_data.m_parallelRank = -1 ;
  }

  setMpiCommunicator(new_comm);
}

void setMpiCommunicator(MPI_Comm communicator)
{
    stk::EnvData &env_data = stk::EnvData::instance();
    if(communicator != MPI_COMM_NULL)
    {
        env_data.m_parallelComm = communicator;

        if(MPI_Comm_size(env_data.m_parallelComm, &env_data.m_parallelSize) != MPI_SUCCESS
           || MPI_Comm_rank(env_data.m_parallelComm, &env_data.m_parallelRank) != MPI_SUCCESS
           || env_data.m_parallelSize == -1
           || env_data.m_parallelRank == -1)
        {
            throw RuntimeError() << "reset given bad MPI communicator";
        }
    }
}

void
output_flush()
{
  stk::EnvData &env_data = stk::EnvData::instance();

  stk::report_deferred_messages(Env::parallel_comm());

  stk::all_write_string(Env::parallel_comm(), *env_data.m_outputP0, env_data.m_output.str());
  env_data.m_output.str("");
}


void
request_shutdown(bool shutdown)
{
  stk::EnvData::instance().m_shutdownRequested = shutdown;
}


bool
is_shutdown_requested()
{
  int shutdown_requested_in = stk::EnvData::instance().m_shutdownRequested || Env::HUP_received();
  int shutdown_requested = -1;

  MPI_Allreduce(&shutdown_requested_in, &shutdown_requested, 1, MPI_INT, MPI_SUM, Env::parallel_comm());

  return shutdown_requested != 0;
}


void abort() {
  stk::EnvData &env_data = stk::EnvData::instance();

  // Cannot be sure of parallel synchronization status; therefore, no communications can
  // occur.  Grab and dump all pending output buffers to 'std::cerr'.
  std::cerr << std::endl
            << "*** SIERRA ABORT on P" << stk::EnvData::instance().m_parallelRank << " ***"
            << std::endl
            << "*** check " << get_param("output-log")
            << " file for more information ***"
            << std::endl ;

  if (!env_data.m_output.str().empty()) {
    std::cerr << "Buffer contents of deferred output stream on processor " << parallel_rank()
              << std::endl ;
    std::cerr << env_data.m_output.str();
  }

  std::cerr.flush();
  std::cout.flush();

  ::sleep(1);					// Give the other processors a chance at
						// catching up, seems to help hanging problems.
  MPI_Abort(env_data.m_parallelComm, MPI_ERR_OTHER);	// First try to die
  std::exit( EXIT_FAILURE );                    // Second try to die
}


const std::string &
get_param(
  const char * const	option)
{
  if (stk::EnvData::instance().m_vm.count(option)) {
    if (stk::EnvData::instance().m_vm[option].as<std::string>().empty())
      return stk::EnvData::instance().m_onString;
    else
      return stk::EnvData::instance().m_vm[option].as<std::string>();
  }
  else
    return stk::EnvData::instance().m_emptyString;
}


void
set_param(
  const char *          option,
  const std::string &   value) {


  namespace opt = boost::program_options;

  opt::variables_map &vm = stk::get_variables_map();
  opt::options_description &od = stk::get_options_description();

  int argc = 1;
  char *s = std::strcpy(new char[std::strlen(option) + 1], option);

  opt::store(opt::parse_command_line(argc, &s, od), vm);
  opt::notify(vm);

  delete [] s;
}

} // namespace Env
} // namespace sierra
