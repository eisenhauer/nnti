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

#include <string>
#include <stdexcept>

#include <KokkosCore_config.h>
#include <Kokkos_Threads.hpp>

/*--------------------------------------------------------------------------*/

#if defined( KOKKOS_HAVE_PTHREAD )

/* Standard 'C' Linux libraries */

#include <pthread.h>
#include <sched.h>
#include <errno.h>

/*--------------------------------------------------------------------------*/

namespace Kokkos {
namespace Impl {

//----------------------------------------------------------------------------

namespace {

pthread_mutex_t host_internal_pthread_mutex = PTHREAD_MUTEX_INITIALIZER ;

// Pthreads compatible driver:

void * internal_pthread_driver( void * )
{
  ThreadsExec::driver();

  return NULL ;
}

} // namespace

//----------------------------------------------------------------------------
// Spawn a thread

bool ThreadsExec::spawn()
{
  bool result = false ;

  pthread_attr_t attr ;

  if ( 0 == pthread_attr_init( & attr ) ||
       0 == pthread_attr_setscope(       & attr, PTHREAD_SCOPE_SYSTEM ) ||
       0 == pthread_attr_setdetachstate( & attr, PTHREAD_CREATE_DETACHED ) ) {

    pthread_t pt ;

    result = 0 == pthread_create( & pt, & attr, internal_pthread_driver, 0 );
  }

  pthread_attr_destroy( & attr );

  return result ;
}

//----------------------------------------------------------------------------

bool ThreadsExec::is_process()
{
  static const pthread_t master_pid = pthread_self();

  return pthread_equal( master_pid , pthread_self() );
}

void ThreadsExec::global_lock()
{
  pthread_mutex_lock( & host_internal_pthread_mutex );
}

void ThreadsExec::global_unlock()
{
  pthread_mutex_unlock( & host_internal_pthread_mutex );
}

//----------------------------------------------------------------------------

namespace {

template< unsigned N > inline void noop_cycle();

template<> inline void noop_cycle<0>() {}
template< unsigned N > inline void noop_cycle()
{
#if !defined ( KOKKOS_DISABLE_ASM ) && \
    ( defined( __GNUC__ ) || \
      defined( __GNUG__ ) || \
      defined( __INTEL_COMPILER__ ) )

  asm volatile("nop");
  noop_cycle<N-1>();

#else
  sched_yield();
#endif
}

}

void ThreadsExec::wait( volatile int & flag , const int value )
{
  // Issue 'NCycle' no-op operations between checks for the flag to change value.
  enum { NCycle = 1 };
  while ( value == flag ) { noop_cycle< NCycle >(); }
}

void ThreadsExec::wait_yield( volatile int & flag , const int value )
{
  while ( value == flag ) { sched_yield(); }
}

} // namespace Impl
} // namespace Kokkos

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#elif defined( KOKKOS_HAVE_WINTHREAD )

/* Windows libraries */
#include <windows.h>
#include <process.h>

//----------------------------------------------------------------------------
// Driver for each created pthread

namespace Kokkos {
namespace Impl {
namespace {

unsigned WINAPI internal_winthread_driver( void * arg )
{
  ThreadsExec::driver();

  return 0 ;
}

class ThreadLockWindows {
private:
  CRITICAL_SECTION  m_handle ;

  ~ThreadLockWindows()
  { DeleteCriticalSection( & m_handle ); }

  ThreadLockWindows();
  { InitializeCriticalSection( & m_handle ); }

  ThreadLockWindows( const ThreadLockWindows & );
  ThreadLockWindows & operator = ( const ThreadLockWindows & );

public:

  static ThreadLockWindows & singleton();

  void lock()
  { EnterCriticalSection( & m_handle ); }

  void unlock()
  { LeaveCriticalSection( & m_handle ); }
};

ThreadLockWindows & ThreadLockWindows::singleton()
{ static ThreadLockWindows self ; return self ; }

} // namespace <>
} // namespace Kokkos
} // namespace Impl

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace Kokkos {
namespace Impl {

// Spawn this thread

bool ThreadsExec::spawn()
{
  unsigned Win32ThreadID = 0 ;

  HANDLE handle =
    _beginthreadex(0,0,internal_winthread_driver,0,0, & Win32ThreadID );

  return ! handle ;
}

bool ThreadsExec::is_process() { return true ; }

void ThreadsExec::global_lock()
{ ThreadLockWindows::singleton().lock(); }

void ThreadsExec::global_unlock()
{ ThreadLockWindows::singleton().unlock(); }

void ThreadsExec::wait( volatile int & flag , const int value )
{
  while ( value == flag ) { Sleep(0); }
}

void ThreadsExec::wait_yield( volatile int & flag , const int value ) {}
{
  while ( value == flag ) { Sleep(0); }
}

} // namespace Impl
} // namespace Kokkos

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#else /* NO Threads */

namespace Kokkos {
namespace Impl {

bool ThreadsExec::spawn()
{
  std::string msg("Kokkos::Threads ERROR : Attempting to spawn threads without configuring with a threading library.  Try configuring with KOKKOS_HAVE_PTHREAD");
  throw std::runtime_error( msg );

  return false ;
}

bool ThreadsExec::is_process() { return true ; }
void ThreadsExec::global_lock() {}
void ThreadsExec::global_unlock() {}
void ThreadsExec::wait( volatile int & , const int ) {}
void ThreadsExec::wait_yield( volatile int & , const int ) {}

} // namespace Impl
} // namespace Kokkos

#endif /* End thread model */

