// $Id$
// $Source$ 
// @HEADER
// ***********************************************************************
// 
//                           Stokhos Package
//                 Copyright (2009) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Eric T. Phipps (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#if defined( __CUDA_ARCH__ )

namespace Stokhos {

  template <typename ordinal_t, typename value_t>
  class DynamicThreadedStorage<ordinal_t, value_t, KokkosArray::Cuda> {
  public:

    static const bool is_static = false;
    static const int static_size = 0;
    static const bool supports_reset = true;

    typedef ordinal_t ordinal_type;
    typedef value_t value_type;
    typedef KokkosArray::Cuda node_type;
    typedef value_type& reference;
    typedef const value_type& const_reference;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef Stokhos::DynArrayTraits<value_type,node_type> ds;

    //! Turn DynamicThreadedStorage into a meta-function class usable with mpl::apply
    template <typename ord_t, typename val_t> 
    struct apply {
      typedef DynamicThreadedStorage<ord_t,val_t,node_type> type;
    };

    //! Constructor
    __device__
    DynamicThreadedStorage(const ordinal_type& sz,
			   const value_type& x = value_type(0.0)) :
      sz_(sz), stride_(num_threads()), total_sz_(sz_*stride_) {
      allocate_coeff_array(coeff_, is_owned_, total_sz_, x);
    }

    //! Constructor
    __device__
    DynamicThreadedStorage(const DynamicThreadedStorage& s) : 
    sz_(s.sz_), stride_(s.stride_), total_sz_(s.total_sz_) {
      allocate_coeff_array(coeff_, is_owned_, total_sz_);
      for (ordinal_type i=0; i<total_sz_; i+=stride_)
	coeff_[i] = s.coeff_[i];
    }

    //! Destructor
    __device__
    ~DynamicThreadedStorage() {
      destroy_coeff_array(coeff_, is_owned_, total_sz_);
    }

    //! Assignment operator
    __device__
    DynamicThreadedStorage& operator=(const DynamicThreadedStorage& s) {
      if (&s != this) { 
	if (s.sz_ != sz_) {
	  destroy_coeff_array(coeff_, is_owned_, total_sz_);
	  sz_ = s.sz_;
	  stride_ = s.stride_;
	  total_sz_ = sz_*stride_;
	  allocate_coeff_array(coeff_, is_owned_, total_sz_);
	  for (ordinal_type i=0; i<total_sz_; i+=stride_)
	    coeff_[i] = s.coeff_[i];
	}
	else {
	  for (ordinal_type i=0; i<total_sz_; i+=stride_)
	    coeff_[i] = s.coeff_[i];
	}
      }
      return *this;
    }

    //! Initialize values to a constant value
    __device__
    void init(const_reference v) { 
      for (ordinal_type i=0; i<total_sz_; i+=stride_)
	coeff_[i] = v;
    }

    //! Initialize values to an array of values
    __device__
    void init(const_pointer v, const ordinal_type& sz = 0) {
      ordinal_type my_sz = stride_*sz;
      if (sz == 0)
	my_sz = total_sz_;
      for (ordinal_type i=0; i<my_sz; i+=stride_)
	coeff_[i] = v[i];
    }

    //! Load values to an array of values
    __device__
    void load(pointer v) {
      for (ordinal_type i=0; i<total_sz_; i+=stride_)
	coeff_[i] = v[i];
    }

    //! Resize to new size (values are preserved)
    __device__
    void resize(const ordinal_type& sz) { 
      if (sz != sz_) {
	value_type *coeff_new;
	bool owned_new;
	ordinal_type total_sz_new = sz*stride_;
	allocate_coeff_array(coeff_new, owned_new, total_sz_new);
	ordinal_type my_tsz = total_sz_;
	if (total_sz_ > total_sz_new)
	  my_tsz = total_sz_new;
	for (ordinal_type i=0; i<my_tsz; i+=stride_)
	  coeff_new[i] = coeff_[i];
	destroy_coeff_array(coeff_, is_owned_, total_sz_);
	coeff_ = coeff_new;
	sz_ = sz;
	total_sz_ = total_sz_new;
	is_owned_ = owned_new;
      }
    }

    //! Reset storage to given array, size, and stride
    __device__
    void shallowReset(pointer v, const ordinal_type& sz, 
		      const ordinal_type& stride, bool owned) { 
      destroy_coeff_array(coeff_, is_owned_, total_sz_);
      coeff_ = v;
      sz_ = sz;
      stride_ = stride;
      total_sz_ = sz_*stride_;
      is_owned_ = owned;
    }

    //! Return size
    __device__
    ordinal_type size() const { return sz_; }

    //! Coefficient access (avoid if possible)
    __device__
    const_reference operator[] (const ordinal_type& i) const {
      return coeff_[i*stride_];
    }

    //! Coefficient access (avoid if possible)
    __device__
    reference operator[] (const ordinal_type& i) {
      return coeff_[i*stride_];
    }

    template <int i>
    __device__
    reference getCoeff() { return coeff_[i]; }

    template <int i>
    __device__
    const_reference getCoeff() const { return coeff_[i]; }

    //! Get coefficients
    __device__
    const_pointer coeff() const { return coeff_; }

    //! Get coefficients
    __device__
    pointer coeff() { return coeff_; }

  protected:

    //! Compute number of threads in each block
    __device__
    ordinal_type num_threads() const { 
      return blockDim.x*blockDim.y*blockDim.z; 
    }

    //! Compute thread index within a block
    __device__
    ordinal_type thread_index() const { 
      return threadIdx.x + (threadIdx.y + threadIdx.z*blockDim.y)*blockDim.x; 
    }

    //! Allocate coefficient array
    __device__
    void allocate_coeff_array(pointer& c, bool& owned, 
			      ordinal_type total_size, 
			      const value_type& x = value_type(0.0)) {

      // Allocate coefficient array on thread 0
      __shared__ pointer ptr;
      ordinal_type tidx = thread_index();
      if (tidx == 0) {
	ptr = ds::get_and_fill(total_size,x);
	owned = true;
      }
      else
	owned = false;
      __syncthreads();

      // Give each thread its portion of the array
      c = ptr + tidx;
    }

    //! Destroy coefficient array
    __device__
    void destroy_coeff_array(pointer c, bool owned, ordinal_type total_size) {
      __syncthreads();
      if (owned)
	ds::destroy_and_release(c, total_size);
    }

  private:

    //! Coefficient values
    pointer coeff_;

    //! Size of array used
    ordinal_type sz_;

    //! Stride of array
    ordinal_type stride_;

    //! Total size of array
    ordinal_type total_sz_;

    //! Do we own the array
    bool is_owned_;

  };

}

#endif
