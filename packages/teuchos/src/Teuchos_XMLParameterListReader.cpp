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

#include "Teuchos_XMLParameterListReader.hpp"
#include "Teuchos_XMLParameterListWriter.hpp"
#include "Teuchos_TestForException.hpp"
#include "Teuchos_ParameterEntryXMLConverterDB.hpp"
#include "Teuchos_ValidatorXMLConverterDB.hpp"


namespace Teuchos {


XMLParameterListReader::XMLParameterListReader()
{;}


ParameterList XMLParameterListReader::toParameterList(const XMLObject& xml) const
{
  TEST_FOR_EXCEPTION(xml.getTag() != XMLParameterListWriter::getParameterListTagName(), 
    std::runtime_error,
    "XMLParameterListReader expected tag " << XMLParameterListWriter::getParameterListTagName()
    <<", found " << xml.getTag());
  // 2010/07/30: rabartl: Above, use a custom exception type to catch in unit
  // testing!
  IDtoValidatorMap validatorMap;
  ParameterList rtn;
  for(int i =0; i<xml.numChildren(); ++i){
    if(xml.getChild(i).getTag() == XMLParameterListWriter::getValidatorsTagName()){
      convertValidators(xml.getChild(i), validatorMap);
    }
  }

  /*for(int i =0; i<xml.numChildren(); ++i){
    if(xml.getChild(i).getTag() == XMLParameterListWriter::getParameterListTagName()){
      rtn = convertParameterList(xml.getChild(i).getChild(0), validatorMap);
    }
  }*/
  // 2010/07/30: rabartl: Above, try to remove dead code as long as you don't
  // need it anymore.  You can always get it back by looking in the hsitory.


  rtn = convertParameterList(xml, validatorMap);
  return rtn;
}


void XMLParameterListReader::convertValidators(
  const XMLObject& xml, IDtoValidatorMap& validatorMap) const
{
  std::set<const XMLObject*> validatorsWithPrototypes;
  for (int i=0; i<xml.numChildren(); ++i){
    if (xml.getChild(i).hasAttribute(ValidatorXMLConverter::getPrototypeIdAttributeName())){
      validatorsWithPrototypes.insert(&xml.getChild(i));
    }
    else{
      ValidatorXMLConverterDB::getConverter(xml.getChild(i))->fromXMLtoValidator(xml.getChild(i),
        validatorMap);
    }
  }
  // 2010/07/30: rabartl: Use "ANSI" style and multi-lines for long for(..) 
  // statements.  See TCDG 1.0 FSC 9
  for (
    std::set<const XMLObject*>::const_iterator it = validatorsWithPrototypes.begin();
    it!=validatorsWithPrototypes.end();
    ++it)
  {
    ValidatorXMLConverterDB::getConverter(*(*it))->fromXMLtoValidator(*(*it), validatorMap);
  }
}

      
ParameterList XMLParameterListReader::convertParameterList(
  const XMLObject& xml, const IDtoValidatorMap& validatorMap) const
{
  TEST_FOR_EXCEPTION(xml.getTag() != XMLParameterListWriter::getParameterListTagName(), 
    std::runtime_error,
    "XMLParameterListReader expected tag " << XMLParameterListWriter::getParameterListTagName()
    <<", found the tag "
    << xml.getTag());
  // 2010/07/30: rabartl: Above, use a custom exception type to catch in unit
  // testing!

  ParameterList rtn;

  for (int i=0; i<xml.numChildren(); i++) {

      XMLObject child = xml.getChild(i);
      
      // 2010/07/30: rabartl: Make the and/or logic more clear with better formatting.
      TEST_FOR_EXCEPTION(
        (
          child.getTag() != XMLParameterListWriter::getParameterListTagName() 
          &&
          child.getTag() != ParameterEntry::getTagName()
          )
        &&
        child.getTag() != XMLParameterListWriter::getValidatorsTagName(),
        std::runtime_error,
        "XMLParameterListReader expected tag "
        << XMLParameterListWriter::getParameterListTagName() << " or "
        << ParameterEntry::getTagName() << ", but found "
        << child.getTag() << " tag.");
      // 2010/07/30: rabartl: Above, use a custom exception type to catch in unit
      // testing!
      
      if (child.getTag()==XMLParameterListWriter::getParameterListTagName()) {
        TEST_FOR_EXCEPTION(
          !child.hasAttribute(XMLParameterListWriter::getNameAttributeName()), 
          std::runtime_error,
          XMLParameterListWriter::getParameterListTagName() <<" tags must "
          "have a " << XMLParameterListWriter::getNameAttributeName() << " attribute"
          << child.getTag());
        // 2010/07/30: rabartl: Above, use a custom exception type to catch in unit
        // testing!
        
        const std::string& name =
          child.getRequired(XMLParameterListWriter::getNameAttributeName());

        ParameterList sublist = convertParameterList(child, validatorMap);
        sublist.setName(name);

        rtn.set(name, sublist);
      }
      else if (child.getTag() == ParameterEntry::getTagName()) {
        const std::string& name =
          child.getRequired(XMLParameterListWriter::getNameAttributeName());
        RCP<const ParameterEntryXMLConverter> converter =
          ParameterEntryXMLConverterDB::getConverter(child);
        ParameterEntry parameter = converter->fromXMLtoParameterEntry(child);
        if (child.hasAttribute(ValidatorXMLConverter::getIdAttributeName())){
          IDtoValidatorMap::const_iterator result =
            validatorMap.getValidator(
              child.getRequiredInt(ValidatorXMLConverter::getIdAttributeName()));
          if (result != validatorMap.end()) {
            parameter.setValidator(result->second);
          }
          else {
            TEST_FOR_EXCEPTION(true, std::runtime_error,
              "Could not find validator with id: "
              << child.getRequiredInt(ValidatorXMLConverter::getIdAttributeName())
              );
            // 2010/07/30: rabartl: Replace the above with a more specific exception
            // type and catch it in the unit test.
          }
        }  
        rtn.setEntry(name, parameter);
     } 
  }
  return rtn;
}


} // namespace Teuchos

// 2010/07/30: rabartl: Above, use commented end braces for namespaces.  See
// TCDG 1.0 FSC 5

