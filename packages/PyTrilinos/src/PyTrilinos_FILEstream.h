// @HEADER
// ***********************************************************************
//
//              PyTrilinos: Python Interface to Trilinos
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
// USA
// Questions? Contact Bill Spotz (wfspotz@sandia.gov)
//
// ***********************************************************************
// @HEADER

#ifndef PYTRILINOS_FILESTREAM_H
#define PYTRILINOS_FILESTREAM_H

#include <streambuf>
#include <vector>

namespace PyTrilinos
{

class FILEstream : public std::streambuf
{
public:
  explicit FILEstream(FILE        *fptr,
		      std::size_t buff_sz  = 256,
		      std::size_t put_back =   8);

private:
  int_type underflow();
  int_type overflow(int_type ch);
  int_type sync();
  // Copy ctor and assignment not allowed
  FILEstream(const FILEstream &);
  FILEstream &operator=(const FILEstream &);

private:
  FILE *fptr_;
  const std::size_t put_back_;
  std::vector<char> buffer_;
};

}  // Namespace PyTrilinos

#endif
