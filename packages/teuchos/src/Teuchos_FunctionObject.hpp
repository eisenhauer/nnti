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

#ifndef Teuchos_FUNCTION_OBJECT_H
#define Teuchos_FUNCTION_OBJECT_H

#include "Teuchos_Describable.hpp"


/*! \file Teuchos_FunctionObject.hpp
    \brief An object representation of a function
*/


namespace Teuchos{


/**
 * \brief A function object represents an arbitrary function
 * that takes a single arguement (ArgType) and returns
 * a single value (ReturnType).
 */
template<class ReturnType, class ArgType>
class FunctionObject: public Describable {

public:
  /**
   * \brief Runs the function with the given arguement and returns
   * the return value of the function.
   *
   * @param arguement Arguement to give to the function.
   * @return The return value of the function given the specified
   * arguement.
   */
  virtual ReturnType runFunction(ArgType arguement) const=0;
  
  /** \brief Returns the string to be used for the value of the 
   * type attribute when converting the function to XML.
   */
  virtual std::string getTypeAttributeValue() const=0;

};


} // namespace Teuchos


#endif
