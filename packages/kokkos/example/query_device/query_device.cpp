/*
//@HEADER
// ************************************************************************
//
//   Kokkos: Manycore Performance-Portable Multidimensional Arrays
//              Copyright (2012) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact  H. Carter Edwards (hcedwar@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#include <iostream>
#include <sstream>

#include <KokkosCore_config.h>

#if defined( KOKKOS_HAVE_MPI )
#include <mpi.h>
#endif

#include <Kokkos_hwloc.hpp>
#include <Kokkos_Threads.hpp>
#include <Kokkos_Cuda.hpp>
#include <Kokkos_OpenMP.hpp>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int main( int argc , char ** argv )
{
  std::ostringstream msg ;

#if defined( KOKKOS_HAVE_MPI )

  MPI_Init( & argc , & argv );

  int mpi_rank = 0 ;

  MPI_Comm_rank( MPI_COMM_WORLD , & mpi_rank );

  msg << "MPI rank(" << mpi_rank << ") " ;

#endif

  msg << "{" << std::endl ;

  if ( Kokkos::hwloc::available() ) {
    const std::pair<unsigned,unsigned> core_top = Kokkos::hwloc::get_core_topology();
    const unsigned                     core_cap = Kokkos::hwloc::get_core_capacity();

    msg << "hwloc( NUMA[" << core_top.first
        << "] x CORE[" << core_top.second
        << "] x HT[" << core_cap << "] )"
        << std::endl ;
  }

#if defined( KOKKOS_HAVE_CUDA )
  Kokkos::Cuda::print_configuration( msg );
#endif

  msg << "}" << std::endl ;

  std::cout << msg.str();

#if defined( KOKKOS_HAVE_MPI )

  MPI_Finalize();

#endif

  return 0 ;
}

