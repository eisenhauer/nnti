// $Id$ 
// $Source$ 
// @HEADER
// ***********************************************************************
// 
//                           Sacado Package
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
// Questions? Contact David M. Gay (dmgay@sandia.gov) or Eric T. Phipps
// (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#ifndef FEAPP_QUADRATICSOURCEFUNCTION_HPP
#define FEAPP_QUADRATICSOURCEFUNCTION_HPP

#include "FEApp_AbstractSourceFunction.hpp"

namespace FEApp {

  /*!
   * \brief A quadratic PDE source function
   */
  template <typename ScalarT>
  class QuadraticSourceFunction : 
    public FEApp::AbstractSourceFunction<ScalarT> {
  public:
  
    //! Default constructor
    QuadraticSourceFunction(double factor) : alpha(factor) {};

    //! Destructor
    virtual ~QuadraticSourceFunction() {};

    //! Evaluate source function
    virtual void
    evaluate(const std::vector<ScalarT>& solution,
	     std::vector<ScalarT>& value) const {
      for (unsigned int i=0; i<solution.size(); i++)
	value[i] = alpha*solution[i]*solution[i];
    }

  private:

    //! Private to prohibit copying
    QuadraticSourceFunction(const QuadraticSourceFunction&);

    //! Private to prohibit copying
    QuadraticSourceFunction& operator=(const QuadraticSourceFunction&);

  protected:
  
    //! Factor
    double alpha;

  };

}

#endif // FEAPP_QUADRATICSOURCEFUNCTION_HPP
