#ifndef __TSQR_TBB_TbbParallelTsqr_hpp
#define __TSQR_TBB_TbbParallelTsqr_hpp

#include <tbb/tbb.h>
#include <TbbTsqr_FactorTask.hpp>
#include <TbbTsqr_ApplyTask.hpp>
#include <TbbTsqr_ExplicitQTask.hpp>
#include <TbbTsqr_CacheBlockTask.hpp>
#include <TbbTsqr_UnCacheBlockTask.hpp>
#include <TbbTsqr_FillWithZerosTask.hpp>
#include <Tsqr_ApplyType.hpp>
#include <limits>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace TSQR {
  namespace TBB {
    
    template< class LocalOrdinal, class Scalar >
    class TbbParallelTsqr {
    private:
      typedef MatView< LocalOrdinal, Scalar > mat_view;
      typedef ConstMatView< LocalOrdinal, Scalar > const_mat_view;
      typedef std::pair< mat_view, mat_view > split_t;
      typedef std::pair< const_mat_view, const_mat_view > const_split_t;
      typedef std::pair< const_mat_view, mat_view > top_blocks_t;
      typedef std::vector< top_blocks_t > array_top_blocks_t;

      template< class MatrixViewType >
      MatrixViewType
      top_block_helper (const size_t P_first,
			const size_t P_last,
			const MatrixViewType& C, 
			const bool contiguous_cache_blocks = false) const
      {
	if (P_first > P_last)
	  throw std::logic_error ("P_first > P_last");
	else if (P_first == P_last)
	  return seq_.top_block (C, contiguous_cache_blocks);
	else
	  {
	    typedef std::pair< MatrixViewType, MatrixViewType > split_type;

	    // Divide [P_first, P_last] into two intervals: [P_first,
	    // P_mid] and [P_mid+1, P_last].  Recurse on the first
	    // interval [P_first, P_mid].
	    const size_t P_mid = (P_first + P_last) / 2;
	    split_type C_split = partitioner_.split (C, P_first, P_mid, P_last,
						     contiguous_cache_blocks);
	    return top_block_helper (P_first, P_mid, C_split.first, 
				     contiguous_cache_blocks);
	  }
      }

    public:
      TbbParallelTsqr (const size_t num_cores = 1,
		       const size_t cache_block_size = 0) :
	seq_ (cache_block_size),
      {
	if (num_cores < 1)
	  ncores_ = 1; // default is no parallelism
	else
	  ncores_ = num_cores;
      }
      
      /// Number of cores to use to solve the problem (i.e., number of
      /// subproblems into which to divide the main problem, to solve
      /// it in parallel).
      size_t ncores() const { return ncores_; }

      /// Cache block size (in bytes) used for the factorization
      size_t cache_block_size() const { return seq_.cache_block_size(); }

      /// Results of SequentialTsqr for each core.
      typedef typename SequentialTsqr< LocalOrdinal, Scalar >::FactorOutput SeqOutput;
      /// Array of ncores "local tau arrays" from parallel TSQR.
      /// (Local Q factors are stored in place.)
      typedef std::vector< std::vector< Scalar > > ParOutput;
      /// factor() returns a pair: the results of SequentialTsqr for
      /// data on each core, and the results of combining the data on
      /// the cores.
      typedef typename std::pair< std::vector< SeqOutput >, ParOutput > FactorOutput;

      FactorOutput
      factor (const LocalOrdinal nrows,
	      const LocalOrdinal ncols, 
	      Scalar A[],
	      const LocalOrdinal lda,
	      Scalar R[],
	      const LocalOrdinal ldr,
	      const bool contiguous_cache_blocks = false)
      {
	using tbb::task;

	mat_view A_view (nrows, ncols, A, lda);
	// A_top will be modified in place by exactly one task, to
	// indicate the partition from which we may extract the R
	// factor after finishing the factorization.
	mat_view A_top;

	std::vector< SeqOutput > seq_output (ncores());
	ParOutput par_output (ncores(), std::vector< Scalar >(ncols));
	if (ncores() < 1)
	  {
	    if (! A_view.empty())
	      throw std::logic_error("Zero subproblems, but A not empty!");
	    else // Return empty results
	      return std::make_pair (seq_output, par_output);
	  }
	
	try {
	  typedef FactorTask< LocalOrdinal, Scalar > factor_task_t;

	  // When the root task completes, A_top will be set to the
	  // topmost partition of A.  We can then extract the R factor
	  // from A_top.
	  factor_task_t& root_task = *new( task::allocate_root() ) 
	    factor_task_t (0, ncores()-1, A_view, &A_top, seq_output, 
			   par_output, seq_, contiguous_cache_blocks);
	  task::spawn_root_and_wait (root_task);
	} catch (tbb::captured_exception& ex) {
	  // TBB can't guarantee on all systems that an exception
	  // thrown in another thread will have its type correctly
	  // propagated to this thread.  If it can't, then it captures
	  // the exception as a tbb:captured_exception, and propagates
	  // it to here.  It may be able to propagate the exception,
	  // though, so be prepared for that.  We deal with the latter
	  // case by allowing the exception to propagate.
	  std::ostringstream os;
	  os << "Intel TBB caught an exception, while computing the QR factor"
	    "ization of a matrix A.  Unfortunately, its type information was "
	    "lost, because the exception was thrown in another thread.  Its "
	    "\"what()\" function returns the following string: " << ex.what();
	  throw std::runtime_error (os.str());
	} 

	// Copy the R factor out of A_top into R.
	seq_.extract_R (A_top.nrows(), A_top.ncols(), A_top.get(), 
			A_top.lda(), R, ldr, contiguous_cache_blocks);

	return std::make_pair (seq_output, par_output);
      }

      void
      apply (const std::string& op,
	     const LocalOrdinal nrows,
	     const LocalOrdinal ncols_Q,
	     const Scalar Q[],
	     const LocalOrdinal ldq,
	     const FactorOutput& factor_output,
	     const LocalOrdinal ncols_C,
	     Scalar C[],
	     const LocalOrdinal ldc,
	     const bool contiguous_cache_blocks = false)
      {
	using tbb::task;

	const ApplyType apply_type (op);
	if (apply_type == ApplyType::ConjugateTranspose && 
	    ScalarTraits< Scalar >::is_complex)
	  throw std::logic_error("Applying Q^H for complex scalar types "
				 "not yet implemented");

	const_mat_view Q_view (nrows, ncols_Q, Q, ldq);
	mat_view C_view (nrows, ncols_C, C, ldc);
	if (! apply_type.transposed())
	  {
	    array_top_blocks_t top_blocks (ncores());
	    build_partition_array (0, ncores()-1, top_blocks, Q_view, 
				   C_view, contiguous_cache_blocks);
	    try {
	      typedef ApplyTask< LocalOrdinal, Scalar > apply_task_t;
	      apply_task_t& root_task = 
		*new( task::allocate_root() )
		apply_task_t (0, ncores()-1, Q_view, C_view, top_blocks,
			      factor_output, seq_, contiguous_cache_blocks);
	      task::spawn_root_and_wait (root_task);
	    } catch (tbb::captured_exception& ex) {
	      std::ostringstream os;
	      os << "Intel TBB caught an exception, while applying a Q factor "
		"computed previously by factor() to the matrix C.  Unfortunate"
		"ly, its type information was lost, because the exception was "
		"thrown in another thread.  Its \"what()\" function returns th"
		"e following string: " << ex.what();
	      throw std::runtime_error (os.str());
	    }
	  }
	else
	  throw std::logic_error ("Applying Q^T and Q^H not implemented");
      }


      void
      explicit_Q (const LocalOrdinal nrows,
		  const LocalOrdinal ncols_Q_in,
		  const Scalar Q_in[],
		  const LocalOrdinal ldq_in,
		  const FactorOutput& factor_output,
		  const LocalOrdinal ncols_Q_out,
		  Scalar Q_out[],
		  const LocalOrdinal ldq_out,
		  const bool contiguous_cache_blocks = false)
      {
	using tbb::task;

	mat_view Q_out_view (nrows, ncols_Q_out, Q_out, ldq_out);
	try {
	  typedef ExplicitQTask< LocalOrdinal, Scalar > explicit_Q_task_t;	  
	  explicit_Q_task_t& root_task = *new( task::allocate_root() )
	    explicit_Q_task_t (0, ncores()-1, Q_out_view, seq_, 
			       contiguous_cache_blocks);
	  task::spawn_root_and_wait (root_task);
	} catch (tbb::captured_exception& ex) {
	  std::ostringstream os;
	  os << "Intel TBB caught an exception, while preparing to compute"
	    " the explicit Q factor from a QR factorization computed previ"
	    "ously by factor().  Unfortunately, its type information was l"
	    "ost, because the exception was thrown in another thread.  Its"
	    " \"what()\" function returns the following string: " 
	     << ex.what();
	  throw std::runtime_error (os.str());
	}
	apply ("N", nrows, ncols_Q_in, Q_in, ldq_in, factor_output,
	       ncols_Q_out, Q_out, ldq_out, contiguous_cache_blocks);
      }

      void
      cache_block (const LocalOrdinal nrows,
		   const LocalOrdinal ncols, 
		   Scalar A_out[],
		   const Scalar A_in[],
		   const LocalOrdinal lda_in) const 
      {
	using tbb::task;

	const_mat_view A_in_view (nrows, ncols, A_in, lda_in);
	// A_out won't have leading dimension lda_in, but that's OK,
	// as long as all the routines are told that A_out is
	// cache-blocked.
	mat_view A_out_view (nrows, ncols, A_out, lda_in);
	try {
	  typedef CacheBlockTask< LocalOrdinal, Scalar > cache_block_task_t;
	  cache_block_task_t& root_task = *new( task::allocate_root() )
	    cache_block_task_t (0, ncores()-1, A_out_view, A_in_view, seq_);
	  task::spawn_root_and_wait (root_task);
	} catch (tbb::captured_exception& ex) {
	  std::ostringstream os;
	  os << "Intel TBB caught an exception, while cache-blocking a mat"
	    "rix.  Unfortunately, its type information was lost, because t"
	    "he exception was thrown in another thread.  Its \"what()\" fu"
	    "nction returns the following string: " << ex.what();
	  throw std::runtime_error (os.str());
	}
      }

      void
      un_cache_block (const LocalOrdinal nrows,
		      const LocalOrdinal ncols,
		      Scalar A_out[],
		      const LocalOrdinal lda_out,		    
		      const Scalar A_in[]) const
      {
	using tbb::task;

	// A_in doesn't have leading dimension lda_out, but that's OK,
	// as long as all the routines are told that A_in is cache-
	// blocked.
	const_mat_view A_in_view (nrows, ncols, A_in, lda_out);
	mat_view A_out_view (nrows, ncols, A_out, lda_out);
	try {
	  typedef UnCacheBlockTask< LocalOrdinal, Scalar > un_cache_block_task_t;
	  un_cache_block_task_t& root_task = *new( task::allocate_root() )
	    un_cache_block_task_t (0, ncores()-1, A_out_view, A_in_view, seq_);
	  task::spawn_root_and_wait (root_task);
	} catch (tbb::captured_exception& ex) {
	  std::ostringstream os;
	  os << "Intel TBB caught an exception, while un-cache-blocking a "
	    "matrix.  Unfortunately, its type information was lost, becaus"
	    "e the exception was thrown in another thread.  Its \"what()\""
	    " function returns the following string: " << ex.what();
	  throw std::runtime_error (os.str());
	}
      }

      template< class MatrixViewType >
      MatrixViewType
      top_block (const MatrixViewType& C, 
		 const bool contiguous_cache_blocks = false) const
      {
	return top_block_helper (0, ncores()-1, C, contiguous_cache_blocks);
      }

      void
      fill_with_zeros (const LocalOrdinal nrows,
		       const LocalOrdinal ncols,
		       Scalar C[],
		       const LocalOrdinal ldc, 
		       const bool contiguous_cache_blocks = false) const
      {
	using tbb::task;
	mat_view C_view (nrows, ncols, C, ldc);

	try {
	  typedef FillWithZerosTask< LocalOrdinal, Scalar > fill_task_t;
	  fill_task_t& root_task = *new( task::allocate_root() )
	    fill_task_t (0, ncores()-1, C_view, seq_, contiguous_cache_blocks);
	  task::spawn_root_and_wait (root_task);
	} catch (tbb::captured_exception& ex) {
	  std::ostringstream os;
	  os << "Intel TBB caught an exception, while un-cache-blocking a "
	    "matrix.  Unfortunately, its type information was lost, becaus"
	    "e the exception was thrown in another thread.  Its \"what()\""
	    " function returns the following string: " << ex.what();
	  throw std::runtime_error (os.str());
	}
      }

    private:
      size_t ncores_;
      TSQR::SequentialTsqr< LocalOrdinal, Scalar > seq_;
      TSQR::Combine< LocalOrdinal, Scalar > combine_;
      Partitioner< LocalOrdinal, Scalar > partitioner_;

      void
      build_partition_array (const size_t P_first,
			     const size_t P_last,
			     array_top_blocks_t& top_blocks,
			     const_mat_view& Q,
			     mat_view& C,
			     const bool contiguous_cache_blocks = false) const
      {
	if (P_first > P_last)
	  return;
	else if (P_first == P_last)
	  {
	    const_mat_view Q_top = seq_.top_block (Q, contiguous_cache_blocks);
	    mat_view C_top = seq_.top_block (C, contiguous_cache_blocks);
	    top_blocks[P_first] = 
	      std::make_pair (const_mat_view (Q_top.ncols(), Q_top.ncols(), Q_top.get(), Q_top.lda()), 
			      mat_view (C_top.ncols(), C_top.ncols(), C_top.get(), C_top.lda()));
	  }
	else
	  {
	    // Recurse on two intervals: [P_first, P_mid] and [P_mid+1, P_last]
	    const size_t P_mid = (P_first + P_last) / 2;
	    const_split_t Q_split = 
	      partitioner_.split (Q, P_first, P_mid, P_last,
				  contiguous_cache_blocks);
	    split_t C_split = 
	      partitioner_.split (C, P_first, P_mid, P_last,
				  contiguous_cache_blocks);
	    build_partition_array (P_first, P_mid, top_blocks, Q_split.first, 
				   C_split.first, contiguous_cache_blocks);
	    build_partition_array (P_mid+1, P_last, top_blocks, Q_split.second, 
				   C_split.second, contiguous_cache_blocks);
	  }
      }
    };
  } // namespace TBB
} // namespace TSQR

#endif // __TSQR_TBB_TbbParallelTsqr_hpp
