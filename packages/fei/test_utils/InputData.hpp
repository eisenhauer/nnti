#ifndef _InputData_h_
#define _InputData_h_

/*--------------------------------------------------------------------*/
/*    Copyright 2005 Sandia Corporation.                              */
/*    Under the terms of Contract DE-AC04-94AL85000, there is a       */
/*    non-exclusive license for use of this work by or on behalf      */
/*    of the U.S. Government.  Export of this program may require     */
/*    a license from the United States Government.                    */
/*--------------------------------------------------------------------*/

#include <fei_macros.hpp>

#include <feiArray.hpp>

class ElemContribution {
 public:
  ElemContribution(){}
  ElemContribution(const ElemContribution& src)
    {
      rhsContributions = src.rhsContributions;
    }

  ~ElemContribution(){}

  bool operator==(const ElemContribution& rhs)
    {
      if (matrixContributions != rhs.matrixContributions) {
	cout << "matrixContributions don't match." << endl;
	return(false);
      }

      if ( rhsContributions != rhs.rhsContributions ) {
	cout << "rhsContributions don't match." << endl;
	return(false);
      }

      return(true);
    }

  bool operator!=(const ElemContribution& rhs)
    {
      return( !( *this == rhs) );
    }

  feiArray<double> matrixContributions;
  feiArray<double> rhsContributions;
};

class InputData {
 public:
  InputData(){}
  ~InputData()
    {
      for(int i=0; i<elemIDs.length(); i++) delete elemIDs[i];
    }

  feiArray<int> elemBlockIDs;
  feiArray<feiArray<int>*> elemIDs;
  feiArray<feiArray<ElemContribution>*> elemContributions;

  bool operator==(const InputData& rhs)
    {
      if (elemBlockIDs != rhs.elemBlockIDs) {
	cout << "elemBlockIDs don't match." << endl;
	return(false);
      }

      for(int i=0; i<elemIDs.length(); i++) {
	feiArray<ElemContribution>& elems = *(elemContributions[i]);
	feiArray<ElemContribution>& rhsElems = *(rhs.elemContributions[i]);

	for(int j=0; j<elemIDs[i]->length(); j++) {
	  int id1 = (*(elemIDs[i]))[j];
	  int id2 = (*(rhs.elemIDs[i]))[j];

	  if ( id1 != id2 ) {
	    cout << "elemIDs don't match. element-block " << elemBlockIDs[i]
		 << ", elemID in position " << j << " is " << id1
		 << ", doesn't match " << id2 << "." << endl;
	    return(false);
	  }

	  if (elems[j] != rhsElems[j]) {
	    cout << "element-block " << elemBlockIDs[i] << ", elemID " << id1
	      << "'s element-contributions don't match." << endl;
	    return(false);
	  }
	}
      }

      return(true);
    }

  bool operator!=(const InputData& rhs)
    {
      return( !( (*this) == rhs) );
    }

  int addElemID(int elemBlockID, int elemID)
    {
      //add and elemBlockID/elemID pair to the internal arrays if not already
      //present.

      int err, insertPoint = -1;
      int blkInd = elemBlockIDs.binarySearch(elemBlockID, insertPoint);
      if (blkInd < 0) {
	err = elemBlockIDs.insert(elemBlockID, insertPoint);
	err += elemIDs.insert(new feiArray<int>, insertPoint);
	err += elemContributions.insert(new feiArray<ElemContribution>,
					insertPoint);
	if (err != 0) return(err);
	blkInd = insertPoint;
      }

      feiArray<int>& IDs = *(elemIDs[blkInd]);
      feiArray<ElemContribution>& ec = *(elemContributions[blkInd]);

      err = IDs.insertSorted(elemID);      
      if (err == -2) return(err);

      ElemContribution dummy;
      if (err >= 0) err = ec.insert(dummy, err);
      if (err == -2) return(err);

      return(0);
    }

  int addElemMatrix(int elemBlockID, int elemID, feiArray<double>& matrixData)
    {
      int insertPoint = -1;
      int blkInd = elemBlockIDs.binarySearch(elemBlockID, insertPoint);
      if (blkInd < 0) {
	cerr << " addElemMatrix ERROR, elemBlockID " << (int)elemBlockID
	     << " not found" << endl;
	return(-1);
      }

      int elemIdx = elemIDs[blkInd]->binarySearch(elemID);
      if (elemIdx < 0) {
	cerr << "addElemMatrix ERROR, elemID " << (int)elemID << " not found."
	     <<endl;
	return(-1);
      }

      ElemContribution& elemContr = (*(elemContributions[blkInd]))[elemIdx];

      feiArray<double>& elemContrMatrix = elemContr.matrixContributions;
      int len = matrixData.length();
      int oldLen = elemContrMatrix.length();
      if (oldLen < len) {
	elemContrMatrix.resize(len);
	for(int i=oldLen; i<len; i++) elemContrMatrix[i] = 0.0;
      }

      for(int i=0; i<matrixData.length(); i++) {
	elemContrMatrix[i] += matrixData[i];
      }

      return(0);
    }

  int addElemRHS(int elemBlockID, int elemID, feiArray<double>& rhsData)
    {
      int insertPoint = -1;
      int blkInd = elemBlockIDs.binarySearch(elemBlockID, insertPoint);
      if (blkInd < 0) {
	cerr << " addElemRHS ERROR, elemBlockID " << (int)elemBlockID
	     << " not found" << endl;
	return(-1);
      }

      int elemIdx = elemIDs[blkInd]->binarySearch(elemID);
      if (elemIdx < 0) {
	cerr << "addElemRHS ERROR, elemID " << (int)elemID << " not found."<<endl;
	return(-1);
      }

      ElemContribution& elemContr = (*(elemContributions[blkInd]))[elemIdx];

      feiArray<double>& elemContrRHS = elemContr.rhsContributions;
      int len = rhsData.length();
      int oldLen = elemContrRHS.length();
      if (oldLen < len) {
	elemContrRHS.resize(len);
	for(int i=oldLen; i<len; i++) elemContrRHS[i] = 0.0;
      }

      for(int i=0; i<rhsData.length(); i++) {
	elemContrRHS[i] += rhsData[i];
      }

      return(0);
    }
};

#endif // _InputData_h_
