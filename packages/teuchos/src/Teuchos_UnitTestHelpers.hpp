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

#ifndef TEUCHOS_UNIT_TEST_HELPERS_HPP
#define TEUCHOS_UNIT_TEST_HELPERS_HPP


/*! \file Teuchos_UnitTestHelpers.hpp

\brief Macros for helping to create concrete unit tests.
*/


#include "Teuchos_UnitTestBase.hpp"


/** \brief Basic unit test creation macro for non-templated code. */
#define TEUCHOS_UNIT_TEST(TEST_GROUP, TEST_NAME) \
  class TEST_GROUP##_##TEST_NAME##_UnitTest : public Teuchos::UnitTestBase \
	{ \
  public: \
    TEST_GROUP##_##TEST_NAME##_UnitTest() \
      : Teuchos::UnitTestBase( #TEST_GROUP, #TEST_NAME ) \
    {} \
    void runUnitTestImpl( Teuchos::FancyOStream &out, bool &success ) const; \
  }; \
  \
  TEST_GROUP##_##TEST_NAME##_UnitTest \
    instance_##TEST_GROUP##_##TEST_NAME##_UnitTest; \
  \
	void TEST_GROUP##_##TEST_NAME##_UnitTest::runUnitTestImpl( \
    Teuchos::FancyOStream &out, bool &success ) const \


/** \brief Basic unit test creation macro for templated code on one template parameter. */
#define TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL(TEST_GROUP, TEST_NAME, TYPE) \
  template<class TYPE> \
  class TEST_GROUP##_##TEST_NAME##_UnitTest : public Teuchos::UnitTestBase \
	{ \
  public: \
    TEST_GROUP##_##TEST_NAME##_UnitTest(const std::string& typeName) \
      : Teuchos::UnitTestBase( std::string(#TEST_GROUP)+"_"+typeName, #TEST_NAME ) \
    {} \
    void runUnitTestImpl( Teuchos::FancyOStream &out, bool &success ) const; \
  }; \
  \
  template<class TYPE> \
	void TEST_GROUP##_##TEST_NAME##_UnitTest<TYPE>::runUnitTestImpl( \
    Teuchos::FancyOStream &out, bool &success ) const \

/** \brief Tempalte instantiation for a single templated type. */
#define TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT(TEST_GROUP, TEST_NAME, TYPE) \
  \
  template class TEST_GROUP##_##TEST_NAME##_UnitTest<TYPE>; \
  TEST_GROUP##_##TEST_NAME##_UnitTest<TYPE> \
  instance_##TEST_GROUP##_##TYPE##_##TEST_NAME##_UnitTest(#TYPE);


//
// Instantiate groups of types
//

#define TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_FLOAT(TEST_GROUP, TEST_NAME)\
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT(TEST_GROUP, TEST_NAME, float)

#define TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_DOUBLE(TEST_GROUP, TEST_NAME)\
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT(TEST_GROUP, TEST_NAME, double)

#ifdef HAVE_TEUCHOS_COMPLEX
#  define TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_COMPLEX_FLOAT(TEST_GROUP, TEST_NAME)\
     typedef std::complex<float> ComplexFloat; \
     TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT(TEST_GROUP, TEST_NAME, ComplexFloat)
#else
#  define TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_COMPLEX_FLOAT(TEST_GROUP, TEST_NAME)
#endif

#ifdef HAVE_TEUCHOS_COMPLEX
#  define TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_COMPLEX_DOUBLE(TEST_GROUP, TEST_NAME)\
     typedef std::complex<double> ComplexDouble; \
     TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT(TEST_GROUP, TEST_NAME, ComplexDouble)
#else
#  define TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_COMPLEX_DOUBLE(TEST_GROUP, TEST_NAME)
#endif


/** \brief Instantiate a whole group of tests for supported Scalar types */
#define TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_SCALAR_TYPES(TEST_GROUP, TEST_NAME)\
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_FLOAT(TEST_GROUP, TEST_NAME) \
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_DOUBLE(TEST_GROUP, TEST_NAME) \
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_COMPLEX_FLOAT(TEST_GROUP, TEST_NAME) \
  TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_COMPLEX_DOUBLE(TEST_GROUP, TEST_NAME)


#endif  // TEUCHOS_UNIT_TEST_HELPERS_HPP
