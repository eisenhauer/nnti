#ifndef __TSQR_TBB_CacheBlockTask_hpp
#define __TSQR_TBB_CacheBlockTask_hpp

#include <tbb/task.h>
#include <TbbTsqr_Partitioner.hpp>
#include <Tsqr_SequentialTsqr.hpp>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace TSQR {
  namespace TBB {
    
    template< class LocalOrdinal, class Scalar >
    class CacheBlockTask : public tbb::task {
    public:
      typedef MatView< LocalOrdinal, Scalar > mat_view;
      typedef ConstMatView< LocalOrdinal, Scalar > const_mat_view;
      typedef std::pair< mat_view, mat_view > split_t;
      typedef std::pair< const_mat_view, const_mat_view > const_split_t;

      CacheBlockTask (const size_t P_first__, 
		      const size_t P_last__,
		      mat_view& A_out,
		      const_mat_view& A_in,
		      const SequentialTsqr< LocalOrdinal, Scalar >& seq) :
	P_first_ (P_first__), 
	P_last_ (P_last__), 
	A_out_ (A_out), 
	A_in_ (A_in), 
	seq_ (seq)
      {}

      tbb::task* execute () {
	using tbb::task;

	if (P_first_ > P_last_ || A_out_.empty() || A_in_.empty())
	  return NULL;
	else if (P_first_ == P_last_)
	  {
	    seq_.cache_block (A_out_.nrows(), A_out_.ncols(), 
			      A_out_.get(), A_in_.get(), A_in_.lda());
	    return NULL;
	  }
	else
	  {
	    // "c": continuation task
	    tbb::empty_task& c = *new( allocate_continuation() ) tbb::empty_task;

	    // Recurse on two intervals: [P_first, P_mid] and [P_mid+1, P_last]
	    const size_t P_mid = (P_first_ + P_last_) / 2;
	    split_t out_split = 
	      partitioner_.split (A_out_, P_first_, P_mid, P_last_, true);
	    const_split_t in_split = 
	      partitioner_.split (A_in_, P_first_, P_mid, P_last_, false);

	    CacheBlockTask& topTask = *new( c.allocate_child() )
	      CacheBlockTask (P_first_, P_mid, out_split.first, 
			      in_split.first, seq_);
	    CacheBlockTask& botTask = *new( c.allocate_child() )
	      CacheBlockTask (P_mid+1, P_last_, out_split.second, 
			      in_split.second, seq_);
	    // Set reference count of parent (in this case, the
	    // continuation task) to 2 (since 2 children -- no
	    // additional task since no waiting).
	    c.set_ref_count (2);
	    c.spawn (botTask);
	    return &topTask; // scheduler bypass optimization
	  }
      }

    private:
      size_t P_first_, P_last_;
      mat_view A_out_;
      const_mat_view A_in_;
      SequentialTsqr< LocalOrdinal, Scalar > seq_;
      Partitioner< LocalOrdinal, Scalar > partitioner_;
    };

  } // namespace TBB
} // namespace TSQR


#endif // __TSQR_TBB_CacheBlockTask_hpp
