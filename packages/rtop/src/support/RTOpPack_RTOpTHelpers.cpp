// @HEADER
// ***********************************************************************
// 
// RTOp: Interfaces and Support Software for Vector Reduction Transformation
//       Operations
//                Copyright (2006) Sandia Corporation
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


#include "RTOpPack_RTOpTHelpers_decl.hpp"


#ifdef RTOPPACK_RTOPT_HELPER_DUMP_OUTPUT
bool RTOpPack::rtop_helpers_dump_all = false;
#endif


#ifdef HAVE_TEUCHOS_EXPLICIT_INSTANTIATION


#include "RTOpPack_RTOpTHelpers_def.hpp"
#include "Teuchos_ExplicitInstantiationHelpers.hpp"


namespace RTOpPack {


TEUCHOS_MACRO_TEMPLATE_INSTANT_SCALAR_TYPES(
  RTOPPACK_RTOPT_HELPERS_INSTANT_SCALAR)


RTOPPACK_RTOPT_HELPERS_DEFAULTREDUCTTARGET_INSTANT(int)


TEUCHOS_MACRO_TEMPLATE_INSTANT_SCALAR_TYPES(
  RTOPPACK_RTOPT_HELPERS_ROPSCALARREDUCTIONBASE_INDEX_INSTANT)


TEUCHOS_MACRO_TEMPLATE_INSTANT_SCALAR_TYPES(
  RTOPPACK_RTOPT_HELPERS_ROPSCALARREDUCTIONBASE_SCALARINDEX_INSTANT)


//TEUCHOS_MACRO_TEMPLATE_INSTANT_SCALAR_TYPES(
//  RTOPPACK_RTOPT_HELPERS_ROPSCALARREDUCTIONBASE_SUBVECTORVIEW_INSTANT)


#ifdef HAVE_TEUCHOS_COMPLEX


RTOPPACK_RTOPT_HELPERS_ROPSCALARREDUCTIONBASE_INSTANT(
  std::complex<float>, float)
RTOPPACK_RTOPT_HELPERS_ROPSCALARREDUCTIONBASE_INSTANT(
  std::complex<double>, double)


#endif // HAVE_TEUCHOS_COMPLEX


} // namespace RTOpPack


#endif // HAVE_TEUCHOS_EXCPLICIT_INSTANTIATION
