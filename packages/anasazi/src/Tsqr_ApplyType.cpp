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

#include <Tsqr_ApplyType.hpp>
#include <stdexcept>

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

namespace TSQR {
  const ApplyType ApplyType::NoTranspose = ApplyType ("N");
  const ApplyType ApplyType::Transpose = ApplyType ("T");
  const ApplyType ApplyType::ConjugateTranspose = ApplyType ("H");

  bool 
  ApplyType::decide_transposed (const std::string& op) const 
  {
    if (op[0] == 'N' || op[0] == 'n')
      return false;
    else if (op[0] == 'T' || op[0] == 't' || op[0] == 'H' || op[0] == 'h')
      return true;
    else
      throw std::invalid_argument ("Invalid \"op\" argument \"" + op + "\"");
  }

  ApplyType::ApplyType_
  ApplyType::decide_apply_type (const std::string& op) const 
  {
    if (op[0] == 'T' || op[0] == 't')
      return Transpose_;
    else if (op[0] == 'N' || op[0] == 'n')
      return NoTranspose_;
    else if (op[0] == 'H' || op[0] == 'h')
      return ConjugateTranspose_;
    else
      throw std::invalid_argument ("Invalid \"op\" argument \"" + op + "\"");
  }
}

