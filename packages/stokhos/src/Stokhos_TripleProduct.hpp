// $Id$ 
// $Source$ 
// @HEADER
// ***********************************************************************
// 
//                           Stokhos Package
//                 Copyright (2008) Sandia Corporation
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
// Questions? Eric T. Phipps (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#ifndef STOKHOS_TRIPLEPRODUCT_HPP
#define STOKHOS_TRIPLEPRODUCT_HPP

#include <vector>

#include "Teuchos_RCP.hpp"

namespace Stokhos {

  //! 3-tensor that stores C_{ijk} = < \Psi_i \Psi_j \Psi_k >
  template <typename BasisT>
  class TripleProduct {
  public:
    
    typedef BasisT basis_type;
    
    typedef typename BasisT::value_type value_type;
    
    //! Constructor
    TripleProduct(const Teuchos::RCP<BasisT>& basis);
    
    //! Destructor
    ~TripleProduct();
      
    //! Get value (i,j,k)
    const value_type& triple_value(unsigned int i, 
				   unsigned int j, 
				   unsigned int k) const;

    //! Get value (i,j,k')
    const value_type& triple_deriv(unsigned int i, 
				   unsigned int j, 
				   unsigned int k) const;

    //! Get value (i,j')
    const value_type& double_deriv(unsigned int i, 
				   unsigned int j) const;
    
    //! Get norm-squared
    const value_type& norm_squared(unsigned int i) const;
    
    //! Return size
    unsigned int size() const { return l; }
    
    //! Return basis
    const BasisT& getBasis() const { return *basis; }

  protected:
    
    //! Compute values
    void compute();

  private:

    // Prohibit copying
    TripleProduct(const TripleProduct&);

    // Prohibit Assignment
    TripleProduct& operator=(const TripleProduct& b);
    
  protected:

    //! Size of each dimension
    unsigned int l;
    
    //! Basis
    Teuchos::RCP<BasisT> basis;

    //! Cijk data
    std::vector<value_type> Cijk;

    //! Dijk data = Cijk'
    std::vector<value_type> Dijk;

    //! Bij' data
    std::vector<value_type> Bij;

  }; // class Triple Product

} // namespace Stokhos

// Include template definitions
#include "Stokhos_TripleProductImp.hpp"

#endif // STOKHOS_TRIPLEPRODUCT_HPP
