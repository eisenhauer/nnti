// @HEADER
// ***********************************************************************
//
//                 Anasazi: Block Eigensolvers Package
//                 Copyright (2010) Sandia Corporation
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

#ifndef __TSQR_NodeTsqrFactory_hpp
#define __TSQR_NodeTsqrFactory_hpp

#include <Kokkos_ConfigDefs.hpp> // HAVE_KOKKOS_TBB

#ifdef HAVE_KOKKOS_TBB
#  include <Kokkos_TBBNode.hpp>
#  include <TbbTsqr.hpp>
#  include <tbb/task_scheduler_init.h>
#endif // HAVE_KOKKOS_TBB

#include <Kokkos_SerialNode.hpp>
#include <Tsqr_SequentialTsqr.hpp>

#include <Teuchos_ParameterList.hpp>
#include <Teuchos_ParameterListExceptions.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_ScalarTraits.hpp>

#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace TSQR {

  /// \class NodeTsqrFactory
  /// \brief Common interface for intranode TSQR
  template< class Node, class Scalar, class LocalOrdinal >
  class NodeTsqrFactory {
  public:
    typedef Node node_type;
    typedef Teuchos::RCP< node_type > node_ptr;
    // Just a default
    typedef SequentialTsqr< LocalOrdinal, Scalar > node_tsqr_type;

    /// Get default parameters for setting up this implementation of
    /// the intranode part of TSQR.
    static Teuchos::RCP< const Teuchos::ParameterList >
    getDefaultParameters ()
    {
      using Teuchos::ParameterList;
      using Teuchos::RCP;

      // FIXME (mfh 28 Oct 2010) The use of persistent data means that
      // this routine is NOT reentrant.  That means that it is not
      // thread-safe, for example.  One way to make this routine
      // reentrant would be to use a construct like pthread_once().
      static RCP< const ParameterList > defaultParams_;
      if (Teuchos::is_null (defaultParams_))
	{
	  RCP< ParameterList > params = Teuchos::parameterList();
	  defaultParams_ = params;
	}
      return defaultParams_;
    }

    /// \brief Return a pointer to the intranode TSQR implementation
    ///
    /// In a proper implementation, this would teturn a pointer to the
    /// intranode TSQR implementation.  This class method is
    /// implemented with correct behavior for those Kokkos Node types
    /// for which we have an intranode TSQR implementation.
    static Teuchos::RCP< node_tsqr_type >
    makeNodeTsqr (const Teuchos::ParameterList& plist)
    {
      throw std::logic_error("TSQR is not supported on your Kokkos Node type");
    }
  };

  template< class T >
  static void
  getParamValue (const Teuchos::ParameterList& plist,
		 const char name[],
		 T& value, // set to default before calling
		 bool& gotValue)
  {
    // using Teuchos::Exceptions::InvalidParameter;

    // All this try/catch stuff is because the C++ compiler can't
    // deduce the right two-argument get() function (second argument
    // would be the default).
    T retrievedValue;
    try {
      const std::string paramName (name);
      retrievedValue = plist.get< T > (paramName);
      gotValue = true;
    } catch (std::exception&) { // Could be wrong type or wrong name.
      gotValue = false;
    }
    // Only write to the output argument if we got a value out of the
    // parameter list.
    if (gotValue)
      value = retrievedValue;
  }

  static size_t
  getCacheBlockSize (const Teuchos::ParameterList& params, 
		     const Teuchos::RCP< const Teuchos::ParameterList >& defaultParams)
  {
    // We try to guess among some reasonable names.
    // The first in the list is the canonical name.
    const char* possibleNames[] = {"cacheBlockSize",
				   "cache_block_size",
				   "cacheSize",
				   "cache_size"};
    const int numPossibleNames = 4;
    size_t cacheBlockSize = 0;
    bool gotCacheBlockSize = false;
      
    for (int trial = 0; trial < numPossibleNames && ! gotCacheBlockSize; ++trial)
      getParamValue< size_t > (params, possibleNames[trial], 
			       cacheBlockSize, gotCacheBlockSize);
    if (! gotCacheBlockSize)
      {
	// Default parameters had better have the value, so we don't
	// try to catch any exceptions here.
	const std::string canonicalName ("cacheBlockSize");
	cacheBlockSize = defaultParams->get< size_t > (canonicalName);
      }
    return cacheBlockSize;
  }

#ifdef HAVE_KOKKOS_TBB
  static int
  getNumCores (const Teuchos::ParameterList& params,
	       const Teuchos::RCP< const Teuchos::ParameterList >& defaultParams)
  {
    // We try to guess among some reasonable names.
    // The first in the list is the canonical name.
    const char* possibleNames[] = {"numCores",
				   "ncores",
				   "numThreads",
				   "nthreads", 
				   "numTasks",
				   "ntasks"};
    const int numPossibleNames = 6;
    int numCores = 1;
    bool gotNumCores = false;

    for (int trial = 0; trial < numPossibleNames && ! gotNumCores; ++trial)
      getParamValue< int > (params, possibleNames[trial], 
			    numCores, gotNumCores);
    if (! gotNumCores)
      {
	// Default parameters had better have the value, so we don't
	// try to catch any exceptions here.
	const std::string canonicalName ("numCores");
	numCores = defaultParams->get< int > (canonicalName);
      }
    return numCores;
  }

  template< class Scalar, class LocalOrdinal >
  class NodeTsqrFactory< Kokkos::TBBNode, Scalar, LocalOrdinal > {
  public:
    typedef Kokkos::TBBNode node_type;
    typedef Teuchos::RCP< node_type > node_ptr;
    typedef TBB::TbbTsqr< LocalOrdinal, Scalar > node_tsqr_type;

    /// Get default parameters for setting up the Intel Threading
    /// Building Blocks - based implementation of the intranode part
    /// of TSQR.  Currently, the only parameters that matter for this
    /// implementation are "cacheBlockSize" and "numCores".  Not
    /// setting "cacheBlockSize" or setting it to zero will ask TSQR
    /// to pick a reasonable default.  Not setting "numCores" will ask
    /// TSQR to pick a reasonable default, namely,
    /// tbb::task_scheduler_init::default_num_threads().  Mild
    /// oversubscription will not decrease performance, but
    /// undersubscription may decrease performance.
    static Teuchos::RCP< const Teuchos::ParameterList >
    getDefaultParameters ()
    {
      using Teuchos::ParameterList;
      using Teuchos::RCP;

      // FIXME (mfh 28 Oct 2010) The use of persistent data means that
      // this routine is NOT reentrant.  That means that it is not
      // thread-safe, for example.  One way to make this routine
      // reentrant would be to use a construct like pthread_once().
      static RCP< const ParameterList > defaultParams_;
      if (Teuchos::is_null (defaultParams_))
	{
	  RCP< ParameterList > params = Teuchos::parameterList();
	  // The TSQR implementation sets a reasonable default value
	  // if you tell it that the cache block size is zero.
	  const size_t defaultCacheBlockSize = 0;
	  params->set ("cacheBlockSize", defaultCacheBlockSize,
		       "Cache block size to use for intranode TSQR.  If "
		       "zero, the intranode TSQR implementation will pick "
		       "a reasonable default.");
	  // TSQR uses a recursive division of the tall skinny matrix
	  // into TBB tasks.  Each task works on a block row.  The TBB
	  // task scheduler ensures that oversubscribing TBB tasks
	  // won't oversubscribe cores, so it's OK if
	  // default_num_threads() is too many.  For example, TBB
	  // might say default_num_threads() is the number of cores on
	  // the node, but the TBB task scheduler might have been
	  // initialized with the number of cores per NUMA region, for
	  // hybrid MPI + TBB parallelism.
	  const int defaultNumCores = 
	    tbb::task_scheduler_init::default_num_threads();
	  params->set ("numCores", defaultNumCores, 
		       "Number of tasks (\"threads\") to use in the intranode "
		       "parallel part of TSQR.  There is little/no performance "
		       "penalty for mild oversubscription, but a potential "
		       "penalty for undersubscription.");
	  defaultParams_ = params;
	}
      return defaultParams_;
    }

    static Teuchos::RCP< node_tsqr_type >
    makeNodeTsqr (const Teuchos::ParameterList& params)
    {
      Teuchos::RCP< const Teuchos::ParameterList > defaultParams = getDefaultParameters ();
      const size_t cacheBlockSize = getCacheBlockSize (params, defaultParams);
      const int numCores = getNumCores (params, defaultParams);
      return Teuchos::rcp (new node_tsqr_type (numCores, cacheBlockSize));
    }
  };
#endif // HAVE_KOKKOS_TBB

  template< class Scalar, class LocalOrdinal >
  class NodeTsqrFactory< Kokkos::SerialNode, Scalar, LocalOrdinal > {
  public:
    typedef Kokkos::SerialNode node_type;
    typedef Teuchos::RCP< node_type > node_ptr;
    typedef SequentialTsqr< LocalOrdinal, Scalar > node_tsqr_type;


    /// Get default parameters for setting up the sequential
    /// implementation of the intranode part of TSQR.  Currently, the
    /// only parameter that matters for this implementation is
    /// "cacheBlockSize" (a size_t).  Not setting that parameter or
    /// setting it to zero will pick a reasonable default.
    static Teuchos::RCP< const Teuchos::ParameterList >
    getDefaultParameters ()
    {
      using Teuchos::ParameterList;
      using Teuchos::RCP;

      // FIXME (mfh 28 Oct 2010) The use of persistent data means that
      // this routine is NOT reentrant.  That means that it is not
      // thread-safe, for example.  One way to make this routine
      // reentrant would be to use a construct like pthread_once().
      static RCP< const ParameterList > defaultParams_;
      if (Teuchos::is_null (defaultParams_))
	{
	  RCP< ParameterList > params = Teuchos::parameterList();
	  // The TSQR implementation sets a reasonable default value
	  // if you tell it that the cache block size is zero.
	  const size_t defaultCacheBlockSize = 0;
	  params->set ("cacheBlockSize", defaultCacheBlockSize,
		       "Cache block size to use for intranode TSQR.  If "
		       "zero, the intranode TSQR implementation will pick "
		       "a reasonable default.");
	  defaultParams_ = params;
	}
      return defaultParams_;
    }

    static Teuchos::RCP< node_tsqr_type >
    makeNodeTsqr (const Teuchos::ParameterList& params)
    {
      Teuchos::RCP< const Teuchos::ParameterList > defaultParams = getDefaultParameters ();
      const size_t cacheBlockSize = getCacheBlockSize (params, defaultParams);
      return Teuchos::rcp (new node_tsqr_type (cacheBlockSize));
    }
  };

} // namespace TSQR

#endif // __TSQR_NodeTsqrFactory_hpp
