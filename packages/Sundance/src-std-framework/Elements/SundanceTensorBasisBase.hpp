/* @HEADER@ */
// ************************************************************************
// 
//                              Sundance
//                 Copyright (2005) Sandia Corporation
// 
// Copyright (year first published) Sandia Corporation.  Under the terms 
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
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
// Questions? Contact Kevin Long (krlong@sandia.gov), 
// Sandia National Laboratories, Livermore, California, USA
// 
// ************************************************************************
/* @HEADER@ */

#ifndef SUNDANCE_TENSORBASISBASE_H
#define SUNDANCE_TENSORBASISBASE_H

namespace SundanceStdFwk {

/** 
 *
 */
class TensorBasisBase
{
public:

  /** 
   * \brief Return the tensor order of the basis
   */
  virtual int tensorOrder() const = 0 ;

  /** 
   * \brief Return the dimension of the members of 
   * a vector-valued basis. Return 1 if the basis
   * is scalar-valued. 
   */
  virtual int dim() const = 0 ;

  /** \brief Inform caller as to whether I am a scalar basis. Default
   * implementation returns false. Overridden by ScalarBasis. */
  virtual bool isScalarBasis() const {return false;}

  /** \brief Inform caller as to whether I am in H(div) */
  virtual bool isHDivBasis() const {return false;}

  /** \brief Inform caller as to whether I am in H(curl) */
  virtual bool isHCurlBasis() const {return false;}
};



} // namespace SundanceStdFwk


#endif