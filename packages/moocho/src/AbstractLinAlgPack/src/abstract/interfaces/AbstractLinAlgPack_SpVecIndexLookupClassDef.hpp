// @HEADER
// ***********************************************************************
// 
// Moocho: Multi-functional Object-Oriented arCHitecture for Optimization
//                  Copyright (2003) Sandia Corporation
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
// Questions? Contact Roscoe A. Bartlett (rabartl@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef SP_VEC_INDEX_LOOKUP_CLASS_DEF_H
#define SP_VEC_INDEX_LOOKUP_CLASS_DEF_H

#include "AbstractLinAlgPack_SpVecIndexLookupClassDecl.hpp"
#include "AbstractLinAlgPack_compare_element_indexes.hpp"

// /////////////////////////////////////////////////////////////////////////////////////
// Definitions of members for SpVecIndexLookup<>

template<class T_Element>
typename AbstractLinAlgPack::SparseVectorUtilityPack::SpVecIndexLookup<T_Element>::poss_type
AbstractLinAlgPack::SparseVectorUtilityPack::SpVecIndexLookup<T_Element>::find_poss(
  index_type index, UpperLower uplow) const
{
  // First look at the cache.  If it matches then use that information, otherwise
  // perform a binary search to find the possition then cache it for latter.

  if(index_cached_) {
    if(index == index_cached_)	// Same as cache so use cache
      return poss_type(adjust_cached_poss(uplow),ele_rel_cached_);
    if(index == index_cached_ + 1 && ele_rel_cached_ == AFTER_ELE
      && uplow == LOWER_ELE)
    {
      // Since poss_cached_ = ( max p s.t. ele_[p].index() < index_cached_ )
      // there are three possibilities here:
      // Either:

      // a) poss_cashed_ == nz_ - 1
      if( poss_cached_ == nz_ - 1 )
        return poss_type( poss_cached_ , AFTER_ELE );	

      // b) ele_[poss_cashed_+1].index() == index
      if( ele_[poss_cached_+1].index() == index )
        return poss_type( poss_cached_+1 , EQUAL_TO_ELE );

      // c) ele_[poss_cashed_+1].index() > index.
      if( ele_[poss_cached_+1].index() > index )
        return poss_type( poss_cached_+1 , BEFORE_ELE );
    }
    if(index == index_cached_ - 1 && ele_rel_cached_ == BEFORE_ELE
      && uplow == UPPER_ELE)
    {
      // Since poss_cached_ = ( max p s.t. ele_[p].index() < index_cached_ )
      // there are three possibilities here:
      // Either:

      // a) poss_cashed_ == 0
      if( poss_cached_ == 0 )
        return poss_type( poss_cached_ , BEFORE_ELE );	
      
      // b) ele_[poss_cashed_-1].index() == index
      if( ele_[poss_cached_+1].index() == index )
        return poss_type( poss_cached_-1 , EQUAL_TO_ELE );

      // c) ele_[poss_cashed_-1].index() < index.
      return poss_type( poss_cached_ - 1, AFTER_ELE);	
    }
  }

  // Perform binary search for the element
  poss_type poss = binary_ele_search(index,uplow);

  // Cache the result if needed.  Don't cache an endpoint
  if(poss.poss != 0 && poss.poss != nz() - 1) {
    index_cached_ = index;
    poss_cached_ = poss.poss;
    ele_rel_cached_ = poss.rel;
  }

  return poss;
}

template<class T_Element>
AbstractLinAlgPack::size_type
AbstractLinAlgPack::SparseVectorUtilityPack::SpVecIndexLookup<T_Element>::find_element(
  index_type index, bool is_sorted ) const
{
  typedef T_Element* itr_t;
  if(is_sorted) {
    const std::pair<itr_t,itr_t> p = std::equal_range( ele(), ele() + nz()
      , index - offset(), compare_element_indexes_less<element_type>() );
    // If p.second - p.first == 1 then the element exits
    if( p.second - p.first == 1 )
      return p.first - ele();	// zero based
    else
      return nz(); // zero based
  }
  else {
    const itr_t itr = std::find_if( ele(), ele() + nz()
      , compare_element_indexes_equal_to<element_type>(index - offset()) );
    return itr - ele();	// zero based
  }
}

template<class T_Element>
void
AbstractLinAlgPack::SparseVectorUtilityPack::SpVecIndexLookup<T_Element>::validate_state() const
{
  if(ele() && ele()->index() + offset() < 1)
    throw NoSpVecSetException("SpVecIndexLookup<T_Element>::validate_state(): Error, ele()->index() + offset() < 1");
}

template<class T_Element>
AbstractLinAlgPack::size_type
AbstractLinAlgPack::SparseVectorUtilityPack::SpVecIndexLookup<T_Element>::adjust_cached_poss(
  UpperLower uplow) const
{
  if(ele_rel_cached_ == EQUAL_TO_ELE) return poss_cached_;	// nonzero element
  switch(uplow) {
    case LOWER_ELE:
      switch(ele_rel_cached_) {
        case BEFORE_ELE:
          return poss_cached_;
        case AFTER_ELE:
          return poss_cached_ + 1;
      }
    case UPPER_ELE:
      switch(ele_rel_cached_) {
        case BEFORE_ELE:
          return poss_cached_ - 1;
        case AFTER_ELE:
          return poss_cached_;
      }
  }
  return 0;	// you will never get here but the compiler needs it.
}

template<class T_Element>
typename AbstractLinAlgPack::SparseVectorUtilityPack::SpVecIndexLookup<T_Element>::poss_type
AbstractLinAlgPack::SparseVectorUtilityPack::SpVecIndexLookup<T_Element>::binary_ele_search(
  index_type index, UpperLower uplow) const
{

  poss_type poss_returned;

  size_type lower_poss = 0, upper_poss = nz_ - 1;

  // Look at the end points first then perform the binary search
  
  typename T_Element::index_type lower_index = ele()[lower_poss].index() + offset();
  if(index <= lower_index) {	// before or inc. first ele.
    if(index == lower_index)	poss_returned.rel = EQUAL_TO_ELE;
    else						poss_returned.rel = BEFORE_ELE;
    poss_returned.poss = lower_poss;
    return poss_returned;
  }

  typename T_Element::index_type upper_index = ele()[upper_poss].index() + offset();
  if(index >= upper_index) { // after or inc. last ele.
    if(index == upper_index)	poss_returned.rel = EQUAL_TO_ELE;
    else						poss_returned.rel = AFTER_ELE;
    poss_returned.poss = upper_poss;
    return poss_returned;
  }

  // Perform the binary search
  for(;;) {

    if(upper_poss == lower_poss + 1) {
      // This is a zero element that is between these two nonzero elements
      if(uplow == LOWER_ELE) {
        poss_returned.rel = BEFORE_ELE;
        poss_returned.poss = upper_poss;
        return poss_returned;	
      }
      else {
        poss_returned.rel = AFTER_ELE;
        poss_returned.poss = lower_poss;
        return poss_returned;
      }
    }

    // Bisect the region
    size_type mid_poss = (upper_poss - lower_poss) / 2 + lower_poss;
    typename T_Element::index_type mid_index = ele()[mid_poss].index() + offset();

    if(mid_index == index) {	 // The nonzero element exists
      poss_returned.rel = EQUAL_TO_ELE;
      poss_returned.poss = mid_poss;
      return poss_returned;
    }

    // update binary search region
    if(index < mid_index) {
      upper_poss = mid_poss;
      upper_index = mid_index;
    }
    else {
      // mid_index < index
      lower_poss = mid_poss;
      lower_index = mid_index;
    }
  }
}

#endif // SP_VEC_INDEX_LOOKUP_CLASS_DEF_H
