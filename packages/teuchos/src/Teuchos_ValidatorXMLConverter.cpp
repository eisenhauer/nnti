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

#include "Teuchos_ValidatorXMLConverter.hpp"

namespace Teuchos{

RCP<ParameterEntryValidator>
ValidatorXMLConverter::fromXMLtoValidator(
  const XMLObject& xmlObj,
  const XMLParameterListReader::ValidatorIDsMap& validatorIDsMap) const
{
  #ifdef HAVE_TEUCHOS_DEBUG
  RCP<const ParameterEntryValidator> dummyValidator = getDummyValidator();
  TEST_FOR_EXCEPTION(
    xmlObj.getRequired(getTypeAttributeName())
    !=
    dummyValidator->getXMLTypeName(), 
    BadValidatorXMLConverterException, 
    "Cannot convert xmlObject " << 
    ". Expected a " << dummyValidator->getXMLTypeName() <<
    " tag but got a " << xmlObj.getRequired(getTypeAttributeName()) << "type");
  #endif
  RCP<ParameterEntryValidator> toReturn = 
    convertXML(xmlObj, validatorIDsMap);
  return toReturn;
}

XMLObject 
ValidatorXMLConverter::fromValidatortoXML(
  const RCP<const ParameterEntryValidator> validator,
  const XMLParameterListWriter::ValidatorSet& validatorSet) const
{
  #ifdef HAVE_TEUCHOS_DEBUG
  RCP<const ParameterEntryValidator> dummyValidator = getDummyValidator();
  TEST_FOR_EXCEPTION(
    validator->getXMLTypeName() 
    !=
    dummyValidator->getXMLTypeName(), 
    BadValidatorXMLConverterException, 
    "Cannot convert Validator " << 
    ". Expected a " << dummyValidator->getXMLTypeName() <<
    " validator but got a " << validator->getXMLTypeName() << "type");
  #endif
  XMLObject toReturn(getValidatorTagName());
  toReturn.addAttribute(getTypeAttributeName(), validator->getXMLTypeName());
  toReturn.addAttribute(getIdAttributeName(),
    ParameterEntryValidator::getValidatorID(validator));
  convertValidator(validator, toReturn, validatorSet);
  return toReturn;
}


}

