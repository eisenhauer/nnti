// @HEADER
// ***********************************************************************
// 
//         Optika: A Tool For Developing Parameter Obtaining GUIs
//                Copyright (2009) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, with Sandia Corporation, the 
// U.S. Government retains certain rights in this software.
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
// Questions? Contact Kurtis Nusbaum (klnusbaum@gmail.com) 
// 
// ***********************************************************************
// @HEADER
#include "Optika_ArrayHelperFunctions.hpp"

namespace Optika{


bool doesParameterContainArray(RCP<const ParameterEntry> parameter){
	std::string typeName = parameter->getAny(false).typeName();
	return typeName.find("Teuchos")!=std::string::npos && typeName.find("Array")!=std::string::npos;	
}

QStringList getValues(QString& values){
	values = values.remove("{");
	values = values.remove("}");
	QStringList toReturn = values.split(",");
	for(int i = 0; i < toReturn.size(); ++i){
		if(toReturn[i].at(0) == QChar(' ')){
			toReturn[i] = toReturn[i].remove(0,1);
		}
	}
	return toReturn;
}

QString determineArrayType(RCP<const ParameterEntry> parameter){
	any anyArray = parameter->getAny();
	if(anyArray.type() == typeid(Array<int>)){
		return intId;
	}
	else if(anyArray.type() == typeid(Array<short>)){
		return shortId;
	}
	else if(anyArray.type() == typeid(Array<double>)){
		return doubleId;
	}
	else if(anyArray.type() == typeid(Array<float>)){
		return floatId;
	}
	else if(anyArray.type() == typeid(Array<std::string>)){
		return stringId;
	}
	else{
		return unrecognizedId;		
	}
}

template <>
Array<std::string> fromStringToArray<std::string>(QString arrayString){
	arrayString = arrayString.remove("{");
	arrayString = arrayString.remove("}");
	QStringList tempValues = arrayString.split(",");
	for(int i = 0; i < tempValues.size(); ++i){
		if(tempValues[i].at(0) == QChar(' ')){
			tempValues[i] = tempValues[i].remove(0,1);
		}
	}
	QList<QVariant> values;
	for(int i = 0; i<tempValues.size(); ++i){
		values.append(tempValues[i]);
	}
	Array<std::string> toReturn;
	for(int i = 0; i<values.size(); ++i){
		toReturn.append(values[i].value<QString>().toStdString());	
	}
	return toReturn;

}

QVariant arrayEntryToVariant(
  RCP<const ParameterEntry> arrayEntry, QString type){
	if(type == intId){
    return QVariant::fromValue<Array<int> >(
      getValue<Array<int> >(*arrayEntry));
	}
	else if(type == shortId){
    return QVariant::fromValue<Array<short> >(
      getValue<Array<short> >(*arrayEntry));
	}
	else if(type == doubleId){
    return QVariant::fromValue<Array<double> >(
      getValue<Array<double> >(*arrayEntry));
  }
	else if(type == floatId){
    return QVariant::fromValue<Array<float> >(
      getValue<Array<float> >(*arrayEntry));
  }
	else if(type == stringId){
    return QVariant::fromValue<Array<std::string> >(
      getValue<Array<std::string> >(*arrayEntry));
	}
  return QVariant();
}

QString getArrayType(QString itemType){
  return itemType.section(" ",-1);  
}

bool isArrayEmpty(RCP<const ParameterEntry> arrayEntry, QString type){
	if(type == intId){
    return getValue<Array<int> >(*arrayEntry).size() == 0;
	}
	else if(type == shortId){
    return getValue<Array<short> >(*arrayEntry).size() == 0;
	}
	else if(type == doubleId){
    return getValue<Array<double> >(*arrayEntry).size() == 0;
  }
	else if(type == floatId){
    return getValue<Array<float> >(*arrayEntry).size() == 0;
  }
	else if(type == stringId){
    return getValue<Array<std::string> >(*arrayEntry).size() == 0;
	}
  return true;
}


}
