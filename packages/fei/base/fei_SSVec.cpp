/*--------------------------------------------------------------------*/
/*    Copyright 2005 Sandia Corporation.                              */
/*    Under the terms of Contract DE-AC04-94AL85000, there is a       */
/*    non-exclusive license for use of this work by or on behalf      */
/*    of the U.S. Government.  Export of this program may require     */
/*    a license from the United States Government.                    */
/*--------------------------------------------------------------------*/

#include <fei_macros.hpp>

#include <limits>
#include <cmath>

#include <fei_SSVec.hpp>

#include <feiArray.hpp>
#include <fei_TemplateUtils.hpp>

//----------------------------------------------------------------------------
SSVec::SSVec(int alloc_increment)
  : whichConstructor_(SS_Constr_Default),
    indices_(new feiArray<int>(0, alloc_increment)),
    coefs_(new feiArray<double>(0, alloc_increment))
{
}

//----------------------------------------------------------------------------
SSVec::SSVec(int numEntries, const int* indices, const double* coefs)
  : whichConstructor_(SS_Constr_RawArrays2),
    indices_(new feiArray<int>(numEntries, numEntries, (int*)indices)),
    coefs_(new feiArray<double>(numEntries, numEntries, (double*)coefs))
{
}

//----------------------------------------------------------------------------
SSVec::SSVec(const SSVec& src)
  : whichConstructor_(SS_Constr_Default),
    indices_(new feiArray<int>(0, src.length())),
    coefs_(new feiArray<double>(0, src.length()))
{
  *this = src;
}

//----------------------------------------------------------------------------
SSVec& SSVec::operator=(const SSVec& src)
{
  *indices_ = *(src.indices_);
  *coefs_ = *(src.coefs_);
  whichConstructor_ = SS_Constr_Default;

  return(*this);
}

//----------------------------------------------------------------------------
SSVec::~SSVec()
{
  delete indices_; indices_ = NULL;
  delete coefs_; coefs_ = NULL;
}

//----------------------------------------------------------------------------
void SSVec::logicalClear()
{
  indices_->resize(0);
  coefs_->resize(0);
}

//----------------------------------------------------------------------------
int SSVec::length() const
{
  if (indices_ == NULL) return(0); 
  else return(indices_->length());
}

//----------------------------------------------------------------------------
int SSVec::setInternalData(int numEntries, const int* indices,
			   const double* coefs)
{
  if (numEntries != indices_->length() ) {
    delete indices_;
    delete coefs_;
    indices_ = new feiArray<int>(numEntries, numEntries, (int*)indices);
    coefs_ = new feiArray<double>(numEntries, numEntries, (double*)coefs);
  }
  else {
    indices_->setInternalData(numEntries, numEntries, (int*)indices);
    coefs_->setInternalData(numEntries, numEntries, (double*)coefs);
  }

  return(0);
}

//----------------------------------------------------------------------------
int SSVec::addEntry(int eqn, double coef)
{
  int insertPoint = -1;
  int index = snl_fei::binarySearch(eqn, indices_->dataPtr(),
                                    indices_->length(), insertPoint);
  if (index >= 0) {
    coefs_->dataPtr()[index] += coef;
    return(0);
  }
  else {
    indices_->insert(eqn, insertPoint);
    coefs_->insert(coef, insertPoint);
  }
  return(0);
}

//----------------------------------------------------------------------------
int SSVec::addEntries(int numEntries,
		      const double* coefs,
		      const int* eqns)
{
  double* coefsPtr = coefs_->dataPtr();
  for(int i=0; i<numEntries; ++i) {
    int insertPoint = -1;
    int index = snl_fei::binarySearch(eqns[i], *indices_, insertPoint);
    if (index >= 0) {
      coefsPtr[index] += coefs[i];
    }
    else {
      indices_->insert(eqns[i], insertPoint);
      coefs_->insert(coefs[i], insertPoint);
      coefsPtr = coefs_->dataPtr();
    }
  }
  return(0);
}

//----------------------------------------------------------------------------
int SSVec::addEntries_sortedInput(int numEntries,
				  const double* coefs,
				  const int* eqns,
				  bool storeZeros)
{
  double* coefsPtr = coefs_->dataPtr();

  int* indicesPtr = indices_->dataPtr();
  int indicesLen = indices_->length();
  int start = 0, end = indices_->length()-1;

  double fei_eps = std::numeric_limits<double>::epsilon();

  if (indicesLen < 1) {
    coefs_->reAllocate(numEntries);
    indices_->reAllocate(numEntries);
    for(int i=0; i<numEntries; ++i) {
      if (!storeZeros) {
	if (std::abs(coefs[i]) < fei_eps) continue;
      }
      coefs_->append(coefs[i]);
      indices_->append(eqns[i]);
    }
    coefsPtr = coefs_->dataPtr();
    indicesPtr = indices_->dataPtr();

    return(0);
  }

  for(int i=0; i<numEntries; ++i) {
    if (!storeZeros) {
      if (std::abs(coefs[i]) < fei_eps) continue;
    }
    int insertPoint = -1;
    int index = snl_fei::binarySearch(eqns[i], indicesPtr, indicesLen,
				      start, end, insertPoint);
    if (index >= 0) {
      coefsPtr[index] += coefs[i];
      start = index+1;
    }
    else {
      indices_->insert(eqns[i], insertPoint);
      coefs_->insert(coefs[i], insertPoint);
      coefsPtr = coefs_->dataPtr();
      start = insertPoint+1;
      indicesLen = indices_->length();
      end = indicesLen-1;
      indicesPtr = indices_->dataPtr();
    }
  }
  return(0);
}

//----------------------------------------------------------------------------
int SSVec::putEntry(int eqn, double coef)
{
  int insertPoint = -1;
  int index = snl_fei::binarySearch(eqn, *indices_, insertPoint);
  if (index >= 0) {
    (*coefs_)[index] = coef; return(0);
  }
  else {
    indices_->insert(eqn, insertPoint);
    coefs_->insert(coef, insertPoint);
  }
  return(0);
}

//----------------------------------------------------------------------------
int SSVec::putEntries(int numEntries,
		      const double* coefs,
		      const int* eqns)
{
  double* coefsPtr = coefs_->dataPtr();
  for(int i=0; i<numEntries; ++i) {
    int insertPoint = -1;
    int index = snl_fei::binarySearch(eqns[i], *indices_, insertPoint);
    if (index >= 0) {
      coefsPtr[index] = coefs[i];
    }
    else {
      indices_->insert(eqns[i], insertPoint);
      coefs_->insert(coefs[i], insertPoint);
      coefsPtr = coefs_->dataPtr();
    }
  }
  return(0);
}

//----------------------------------------------------------------------------
void SSVec::writeToStream(FEI_OSTREAM& os)
{
  int* indPtr = indices_->dataPtr();
  double* coefPtr = coefs_->dataPtr();

  os << "   numEntries: " << indices_->length()<<FEI_ENDL;
  for(int i=0; i<indices_->length(); ++i) {
    os << "     "<< indPtr[i] << ": " << coefPtr[i] << FEI_ENDL;
  }
}

