/*@HEADER
// ***********************************************************************
// 
//       Tifpack: Tempated Object-Oriented Algebraic Preconditioner Package
//                 Copyright (2009) Sandia Corporation
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
//@HEADER
*/

#ifndef TIFPACK_HEAP_HPP
#define TIFPACK_HEAP_HPP

#include <algorithm>
#include "Teuchos_Array.hpp"

namespace Tifpack {

template<typename Scalar, typename Ordinal>
struct greater_indirect {
  greater_indirect(const Teuchos::Array<Scalar>& vals)
  : m_vals(vals) {}
  ~greater_indirect(){}

  bool operator()(const Ordinal& lhs, const Ordinal& rhs) const
  { return Teuchos::ScalarTraits<Scalar>::magnitude(m_vals[lhs]) >
           Teuchos::ScalarTraits<Scalar>::magnitude(m_vals[rhs]); }

private:
  const Teuchos::Array<Scalar>& m_vals;
};//struct greater_indirect


/** Add idx to heap, don't assume heap occupies entire vector.
*/
template<typename Ordinal, typename SizeType>
void add_to_heap(const Ordinal& idx, Teuchos::Array<Ordinal>& heap, SizeType& heap_len)
{
  if (heap.size() == heap_len) heap.push_back(idx);
  else heap[heap_len] = idx;
  ++heap_len;
  std::push_heap(heap.begin(), heap.begin()+heap_len, std::greater<Ordinal>());
}

/** Add idx to heap, don't assume heap occupies entire vector.
    Also take custom comparator.
*/
template<typename Ordinal, typename SizeType, class Compare>
void add_to_heap(const Ordinal& idx, Teuchos::Array<Ordinal>& heap, SizeType& heap_len, Compare comp)
{
  if (heap.size() == heap_len) heap.push_back(idx);
  else heap[heap_len] = idx;
  ++heap_len;
  std::push_heap(heap.begin(), heap.begin()+heap_len, comp);
}

/** Remove heap root, don't shorten vector but update a heap_len parameter. */
template<typename Ordinal, typename SizeType>
void rm_heap_root(Teuchos::Array<Ordinal>& heap, SizeType& heap_len)
{
  std::pop_heap(heap.begin(), heap.begin()+heap_len, std::greater<Ordinal>());
  --heap_len;
}

/** Remove heap root, with custom comparator, don't assume heap occupies
  entire vector.
*/
template<typename Ordinal, typename SizeType, class Compare>
void rm_heap_root(Teuchos::Array<Ordinal>& heap, SizeType& heap_len, Compare comp)
{
  std::pop_heap(heap.begin(), heap.begin()+heap_len, comp);
  --heap_len;
}

}//namespace Tifpack

#endif

