/*
//@HEADER
// ************************************************************************
//
//   KokkosArray: Manycore Performance-Portable Multidimensional Arrays
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

/*--------------------------------------------------------------------------*/
/* KokkosArray interfaces */

#include <KokkosArray_Host.hpp>
#include <KokkosArray_hwloc.hpp>
#include <Host/KokkosArray_Host_Internal.hpp>
#include <impl/KokkosArray_Error.hpp>

/*--------------------------------------------------------------------------*/
/* Standard 'C' libraries */
#include <stdlib.h>

/* Standard 'C++' libraries */
#include <limits>
#include <iostream>
#include <sstream>

/*--------------------------------------------------------------------------*/

namespace KokkosArray {
namespace Impl {
namespace {

class HostWorkerBlock : public HostThreadWorker {
public:

  void execute_on_thread( HostThread & ) const 
  {
    host_thread_lock();
    host_thread_unlock();
  }

  HostWorkerBlock()  {}
  ~HostWorkerBlock() {}
};

const HostWorkerBlock worker_block ;

unsigned host_thread_coordinates[ HostThread::max_thread_count ][ hwloc::depth ];

//----------------------------------------------------------------------------

inline
void thread_mapping( const unsigned thread_rank ,
                     const unsigned gang_count,
                     const unsigned worker_count ,
                     unsigned coordinate[] )
{
  const unsigned gang_rank   = thread_rank / worker_count ;
  const unsigned worker_rank = thread_rank % worker_count ;

  unsigned capacity[ hwloc::depth ];

  hwloc::get_thread_capacity( capacity );

  for ( unsigned i = 0 ; i < hwloc::depth ; ++i ) coordinate[i] = 0 ;

  { // Assign gang to resource:
    const unsigned bin  = gang_count / capacity[0] ;
    const unsigned bin1 = bin + 1 ;
    const unsigned k    = capacity[0] * bin1 - gang_count ;
    const unsigned part = k * bin ;

    if ( gang_rank < part ) {
      coordinate[0] = gang_rank / bin ;
    }
    else {
      coordinate[0] = k + ( gang_rank - part ) / bin1 ;
    }
  }

  { // Assign workers to resources:
    unsigned n = worker_count ;
    unsigned r = worker_rank ;

    for ( unsigned i = 1 ; i < hwloc::depth &&
                           0 < capacity[i]  ; ++i ) {
      // n = k * bin + ( capacity[i] - k ) * ( bin + 1 )
      const unsigned bin  = n / capacity[i] ;
      const unsigned bin1 = bin + 1 ;
      const unsigned k    = capacity[i] * bin1 - n ;
      const unsigned part = k * bin ;

      if ( r < part ) {
        coordinate[i]  = r / bin ;
        r = r - bin * coordinate[i] ;
        n = bin ;
      }
      else {
        const unsigned r1 = r - part ;
        const unsigned c1 = r1 / bin1 ;

        coordinate[i]  = c1 + k ;
        r = r1 - c1 * bin1 ;
        n = bin1 ;
      }
    }
  }
}

} // namespace

//----------------------------------------------------------------------------

void host_barrier( HostThread & thread )
{
  // The 'wait' function repeatedly polls the 'thread' state
  // which may reside in a different NUMA region.
  // Thus the fan is intra-node followed by inter-node
  // to minimize inter-node memory access.

  for ( unsigned i = 0 ; i < thread.fan_count() ; ++i ) {
    // Wait until thread enters the 'Rendezvous' state
    host_thread_wait( & thread.fan(i).m_state , HostThread::ThreadActive );
  }

  if ( thread.rank() ) {
    thread.m_state = HostThread::ThreadRendezvous ;
    host_thread_wait( & thread.m_state , HostThread::ThreadRendezvous );
  }

  for ( unsigned i = thread.fan_count() ; 0 < i ; ) {
    thread.fan(--i).m_state = HostThread::ThreadActive ;
  }
}

//----------------------------------------------------------------------------

/** \brief  This thread waits for each fan-in thread to deactivate.  */
void HostThreadWorker::fanin_deactivation( HostThread & thread ) const
{
  // The 'wait' function repeatedly polls the 'thread' state
  // which may reside in a different NUMA region.
  // Thus the fan is intra-node followed by inter-node
  // to minimize inter-node memory access.

  for ( unsigned i = 0 ; i < thread.fan_count() ; ++i ) {
    host_thread_wait( & thread.fan(i).m_state , HostThread::ThreadActive );
  }
}

//----------------------------------------------------------------------------

void HostInternal::activate_threads()
{
  // Activate threads to call 'm_worker.execute_on_thread'
  for ( unsigned i = m_thread_count ; 1 < i ; ) {
    HostThread::get_thread(--i)->m_state = HostThread::ThreadActive ;
  }
}

inline
void HostInternal::execute( const HostThreadWorker & worker )
{
  verify_inactive("execute(...)");

  // Worker threads are verified to be in the ThreadInactive state.

  m_worker = & worker ;

  activate_threads();

  // Execute on the master thread,
  worker.execute_on_thread( m_master_thread );

  // Wait for fanin/fanout threads to self-deactivate.
  worker.fanin_deactivation( m_master_thread );

  // Worker threads are returned to the ThreadInactive state.
  m_worker = NULL ;
}

void HostThreadWorker::execute() const
{ HostInternal::singleton().execute( *this ); }

void HostInternal::execute_serial( const HostThreadWorker & worker )
{
  verify_inactive("execute_serial(...)");

  // Worker threads are verified to be in the ThreadInactive state.

  m_worker = & worker ;

  for ( unsigned i = m_thread_count ; 1 < i ; ) {
    --i ;
    HostThread & thread = * HostThread::get_thread(i);

    thread.m_state = HostThread::ThreadActive ;
    host_thread_wait( & thread.m_state , HostThread::ThreadActive );
  }

  // Execute on the master thread,
  worker.execute_on_thread( m_master_thread );

  // Worker threads are returned to the ThreadInactive state.
  m_worker = NULL ;
}

void HostThreadWorker::execute_serial() const
{ HostInternal::singleton().execute_serial( *this ); }

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

HostInternal::~HostInternal()
{
  HostThread::clear_thread(0);
}

HostInternal::HostInternal()
  : m_thread_count( 1 )
{
  m_worker = NULL ;

  if ( ! is_master_thread() ) {
    KokkosArray::Impl::throw_runtime_exception( std::string("KokkosArray::Impl::HostInternal FAILED : not initialized on the master thread") );
  }
}

HostInternal & HostInternal::singleton()
{
  static HostInternal self ; return self ;
}

void HostInternal::print_configuration( std::ostream & s ) const
{
  hwloc::print_thread_capacity( s );

  s << std::endl ;

  for ( unsigned r = 0 ; r < m_thread_count ; ++r ) {
    HostThread * const thread = HostThread::get_thread( r );

    s << "  thread[ " << thread->rank() << " : ("
      << thread->gang_rank()
      << "," << thread->worker_rank() << ") ] -> bind{"
      << " " << host_thread_coordinates[r][0]
      << " " << host_thread_coordinates[r][1]
      << " " << host_thread_coordinates[r][2]
      << " }" << std::endl ;
  }
}

//----------------------------------------------------------------------------

inline
void * HostInternal::reduce_scratch() const
{
  return m_master_thread.reduce_data();
}

//----------------------------------------------------------------------------

void HostInternal::finalize()
{
  verify_inactive("finalize()");

  Host::resize_reduce_scratch(0);

  // Release and clear worker threads:

  for ( unsigned r = HostThread::max_thread_count ; 0 < --r ; ) {

    HostThread * thread = HostThread::get_thread( r );

    if ( 0 != thread ) {

      m_master_thread.m_state = HostThread::ThreadInactive ;

      thread->m_state = HostThread::ThreadTerminating ;
      thread = 0 ; // The '*thread' object is now invalid

      // Wait for '*thread' to terminate:
      host_thread_wait( & m_master_thread.m_state , HostThread::ThreadInactive );
    }
  }

  HostThread::clear_thread(0);
}

//----------------------------------------------------------------------------
// Driver for each created thread

void HostInternal::driver( const size_t thread_rank )
{
  // Bind this thread to a unique processing unit
  // with all members of a gang in the same NUMA region.

  if ( hwloc::bind_this_thread( host_thread_coordinates[ thread_rank ] ) ) {

    HostThread this_thread ;

    this_thread.m_state = HostThread::ThreadActive ;

    HostThread::set_thread( thread_rank , & this_thread );

    // Inform master thread that spawning and binding succeeded.

    m_master_thread.m_state = HostThread::ThreadActive ;

    try {
      // Work loop:

      while ( HostThread::ThreadActive == this_thread.m_state ) {

        // Perform the work:
        m_worker->execute_on_thread( this_thread );

        // Wait for fanin threads to self-deactivate:
        m_worker->fanin_deactivation( this_thread );

        // Deactivate this thread:
        this_thread.m_state = HostThread::ThreadInactive ;

        // Wait to be activated or terminated:
        host_thread_wait( & this_thread.m_state , HostThread::ThreadInactive );
      }
    }
    catch( const std::exception & x ) {
      // mfh 29 May 2012: Doesn't calling std::terminate() seem a
      // little violent?  On the other hand, C++ doesn't define how
      // to transport exceptions between threads (until C++11).
      // Since this is a worker thread, it would be hard to tell the
      // master thread what happened.
      std::cerr << "Thread " << thread_rank << " uncaught exception : "
                << x.what() << std::endl ;
      std::terminate();
    }
    catch( ... ) {
      // mfh 29 May 2012: See note above on std::terminate().
      std::cerr << "Thread " << thread_rank << " uncaught exception"
                << std::endl ;
      std::terminate();
    }
  }

  // Notify master thread that this thread has terminated.

  HostThread::clear_thread( thread_rank );

  m_master_thread.m_state = HostThread::ThreadTerminating ;
}

//----------------------------------------------------------------------------

void HostInternal::verify_inactive( const char * const method ) const
{
  if ( NULL != m_worker ) {

    std::ostringstream msg ;
    msg << "KokkosArray::Host::" << method << " FAILED: " ;

    if ( & worker_block == m_worker ) {
      msg << "Device is blocked" ;
    }
    else {
      msg << "Functor is executing." ;
    }
    KokkosArray::Impl::throw_runtime_exception( msg.str() );
  }
}

//----------------------------------------------------------------------------

bool HostInternal::spawn_threads( const unsigned thread_count )
{
  // If the process is bound to a particular node
  // then only use cores belonging to that node.
  // Otherwise use all nodes and all their cores.

  m_worker = & worker_block ;

  bool ok_spawn_threads = true ;

  for ( unsigned rank = thread_count ; ok_spawn_threads && 0 < --rank ; ) {

    m_master_thread.m_state = HostThread::ThreadInactive ;

    // Spawn thread executing the 'driver' function.
    ok_spawn_threads = spawn( rank );

    if ( ok_spawn_threads ) {

      // Thread spawned, wait for thread to activate:
      host_thread_wait( & m_master_thread.m_state , HostThread::ThreadInactive );

      // Check if the thread initialized and bound correctly:
      ok_spawn_threads = HostThread::ThreadActive == m_master_thread.m_state ;

      if ( ok_spawn_threads ) { // Wait for spawned thread to deactivate
        host_thread_wait( & HostThread::get_thread(rank)->m_state , HostThread::ThreadActive );
      }
    }
  }

  // Bind the process thread as thread_rank == 0
  if ( ok_spawn_threads ) {
    HostThread::set_thread( 0 , & m_master_thread );
    ok_spawn_threads = hwloc::bind_this_thread( host_thread_coordinates[0] );
  }

  m_worker = NULL ;

  return ok_spawn_threads ;
}

void HostInternal::initialize( const unsigned gang_count ,
                               const unsigned worker_count )
{
  const unsigned thread_count = gang_count * worker_count ;
  const bool ok_inactive = 1 == m_thread_count &&
                           0 == HostThread::get_thread_count();
  bool ok_spawn_threads = true ;

  if ( ok_inactive && 1 < gang_count * worker_count ) {

    // Define coordinates for pinning threads:

    for ( unsigned r = 0 ; r < thread_count ; ++r ) {
      thread_mapping( r , gang_count , worker_count , host_thread_coordinates[r] );
    }

    // Only try to spawn threads if input is valid.

    m_thread_count = thread_count ;

    ok_spawn_threads = spawn_threads( thread_count );

    if ( ok_spawn_threads ) {
      // All threads spawned, initialize the master-thread fan-in

      for ( unsigned r = 0 ; r < thread_count ; ++r ) {

        HostThread & thread = * HostThread::get_thread(r);

        thread.m_gang_tag    = r / worker_count ;
        thread.m_worker_tag  = r % worker_count ;
      }

      HostThread::set_thread_relationships();
    }
    else {
      finalize();
    }
  }

  if ( ! ok_inactive || ! ok_spawn_threads )
  {
    std::ostringstream msg ;

    msg << "KokkosArray::Host::initialize() FAILED" ;

    if ( ! ok_inactive ) {
      msg << " : Device is already active" ;
    }
    if ( ! ok_spawn_threads ) {
      msg << " : Spawning or cpu-binding the threads" ;
    }

    KokkosArray::Impl::throw_runtime_exception( msg.str() );
  }

  // Initial reduction allocation:
  Host::resize_reduce_scratch( 1024 );
}

//----------------------------------------------------------------------------

} // namespace Impl
} // namespace KokkosArray

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace KokkosArray {

//----------------------------------------------------------------------------

namespace Impl {
namespace {

struct HostThreadResizeReduce : public HostThreadWorker {

  const unsigned reduce_size ;

  HostThreadResizeReduce( unsigned size )
    : reduce_size(size)
    { HostThreadWorker::execute_serial(); }

  void execute_on_thread( HostThread & thread ) const
    { thread.resize_reduce( reduce_size ); }
};

}
} // namespace Impl

void Host::resize_reduce_scratch( unsigned size )
{
  static unsigned m_reduce_size = 0 ;

  const unsigned rem = size % HostSpace::MEMORY_ALIGNMENT ;

  if ( rem ) size += HostSpace::MEMORY_ALIGNMENT - rem ;

  if ( ( 0 == size ) || ( m_reduce_size < size ) ) {

    Impl::HostThreadResizeReduce tmp(size);

    m_reduce_size = size ;
  }
}

void * Host::root_reduce_scratch()
{ return Impl::HostInternal::singleton().reduce_scratch(); }

//----------------------------------------------------------------------------

void Host::finalize()
{ Impl::HostInternal::singleton().finalize(); }

void Host::initialize( const Host::size_type gang_count ,
                       const Host::size_type gang_worker_count )
{
  Impl::HostInternal::singleton().initialize( gang_count , gang_worker_count );
}

void Host::print_configuration( std::ostream & s )
{
  Impl::HostInternal::singleton().Impl::HostInternal::print_configuration(s);
}

Host::size_type Host::detect_gang_capacity()
{
  unsigned capacity[ hwloc::depth ];

  hwloc::get_thread_capacity( capacity );

  return capacity[0] ;
}

Host::size_type Host::detect_gang_worker_capacity()
{
  unsigned capacity[ hwloc::depth ];

  hwloc::get_thread_capacity( capacity );

  return capacity[1] * capacity[2] * capacity[3] ;
}

//----------------------------------------------------------------------------

bool Host::sleep()
{
  Impl::HostInternal & h = Impl::HostInternal::singleton();

  const bool is_ready   = NULL == h.m_worker ;
        bool is_blocked = & Impl::worker_block == h.m_worker ;

  if ( is_ready ) {
    Impl::host_thread_lock();

    h.m_worker = & Impl::worker_block ;

    // Activate threads so that they will proceed from
    // spinning state to being blocked on the mutex.

    h.activate_threads();

    is_blocked = true ;
  }

  return is_blocked ;
}

bool Host::wake()
{
  Impl::HostInternal & h = Impl::HostInternal::singleton();

  const bool is_blocked = & Impl::worker_block == h.m_worker ;
        bool is_ready   = NULL == h.m_worker ;

  if ( is_blocked ) {
    Impl::host_thread_unlock();

    Impl::worker_block.fanin_deactivation( h.m_master_thread );

    h.m_worker = NULL ;

    is_ready = true ;
  }

  return is_ready ;
}

/*--------------------------------------------------------------------------*/

} // namespace KokkosArray

