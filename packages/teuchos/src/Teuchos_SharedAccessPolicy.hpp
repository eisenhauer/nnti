/*
// @HEADER
// ***********************************************************************
//
//                    Teuchos: Common Tools Package
//                 Copyright (2004) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
// @HEADER
*/

#ifndef TEUCHOS_SHARED_ACCESS_POLICY_HPP
#define TEUCHOS_SHARED_ACCESS_POLICY_HPP


#include "Teuchos_ConfigDefs.hpp"


//
// WARNING: This current file is just for iterating on the design of thread
// safety in Teuchos and is not working code yet!
//


namespace Teuchos {


/** \brief Basic portable thread lock primative class.
 *
 * This is a heavy-weight appraoch to lock a thread.
 *
 * This class may or may not be exposed in the Teuchos namespace.  It might
 * just be exposed as a typedef in SharedAccessPolicy.
 */
class ThreadLock {
public:
  /** \brief . */
  ThreadLock();
  /** \brief . */
  ~ThreadLock();
  /** \brief . */
  bool try_lock();
  /** \brief . */
  void lock();
  /** \brief . */
  void unlock();
};


/** \brief Stack-based object for locking a thread.
 *
 * This class is used to lock a function so that no matter how the function is
 * existed the lock will be released.
 *
 * This class may or may not be exposed in the Teuchos namespace.  It might
 * just be exposed as a typedef in SharedAccessPolicy.
 */
template<class T>
class ScopedThreadLock {
public:
  /** \brief . */
  explicit ScopedThreadLock(ThreadLock &lock);
  /** \brief . */
  ~ScopedThreadLock();
};


/** \brief Single policy class defining an approach for sharing an integral
 * object across threads as well as a general heavy-weight locking policy.
 *
 * This policy class provides an primative integral type
 * (atomic_integral_type) and a set of atomic primitives for
 * incrementing/decrementing and/or setting/fetching the object.
 *
 * This policy class also provides typdefs for generic heavier-weight thread
 * locking objects that can be used for more general locking.
 *
 * This class is designed, in purpose, to provide sufficient functionality to
 * make the Teuchos MM classes thread-safe.  However, the will have other uses
 * in Teuchos and other C++ code also.
 *
 * There will likely be a typedef called DefaultSharedAccessPolicy in the
 * Teuchos namespace that will select the default global policy (i.e. the one
 * used by the Teuchos MM classes by default).  Also, there well as concrete
 * implementations NonthreadedSharedAccessPolicy, TbbSharedAccessPolicy,
 * PtheadsSharedAccessPolicy, and others (when support is enabled).
 */
class SharedAccessPolicy {
public:
  /** \brief . */
  typedef ThreadLock lock_type;
  /** \brief . */
  typedef ScopedThreadLock scoped_lock_type;
  /** \brief Supported type for shared integral objects. */
  typedef int atomic_integral_type;
  /** \brief Atomic setting a shared integral object. */
  inline static void atomic_set( atomic_integral_type * p,
    const atomic_integral_type v );
  /** \brief Atomic fetch a shared integral object. */
  inline static const atomic_integral_type
  atomic_fetch( const atomic_integral_type * p );
  /** \brief Atomic increment of a shared integral object. */
  inline static void atomic_increment( atomic_integral_type * p );
  /** \brief Atomic decrement of a shared integral object. */
  inline static void atomic_decrement( atomic_integral_type * p );
  // ToDo: Define some other basic fetch/increment primatives needed for
  // better performance, for example, for Teuchos::RCPNode.
};


} // namespace Teuchos


#endif /* TEUCHOS_SHARED_ACCESS_POLICY_HPP */
