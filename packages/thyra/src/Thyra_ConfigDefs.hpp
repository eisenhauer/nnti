/*
// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
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
*/

#ifndef _THYRA_CONFIGDEFS_H_
#define _THYRA_CONFIGDEFS_H_

/* Let Teuchos' configure process do all of the work! */
#include <Teuchos_ConfigDefs.hpp>

/*
 * The macros PACKAGE, PACKAGE_NAME, etc, get defined for each package and need to
 * be undef'd here to avoid warnings when this file is included from another package.
 * KL 11/25/02
 */
#ifdef PACKAGE
#  undef PACKAGE
#endif

#ifdef PACKAGE_NAME
#  undef PACKAGE_NAME
#endif

#ifdef PACKAGE_BUGREPORT
#  undef PACKAGE_BUGREPORT
#endif

#ifdef PACKAGE_STRING
#  undef PACKAGE_STRING
#endif

#ifdef PACKAGE_TARNAME
#  undef PACKAGE_TARNAME
#endif

#ifdef PACKAGE_VERSION
#  undef PACKAGE_VERSION
#endif

#ifdef VERSION
#  undef VERSION
#endif

#include <Thyra_Config.h>

#if !defined(HAVE_THYRA_EXPLICIT_INSTANTIATION) && defined(HAVE_TEUCHOS_EXPLICIT_INSTANTIATION)
#  define HAVE_THYRA_EXPLICIT_INSTANTIATION
#endif

#if !defined(ENABLE_THYRA_DEPRECATED_FEATURES) && !defined(DISABLE_THYRA_DEPRECATED_FEATURES) && defined(ENABLE_TRILINOS_DEPRECATED_FEATURES)
#  define ENABLE_THYRA_DEPRECATED_FEATURES
#endif

#endif /*_THYRA_CONFIGDEFS_H_*/
