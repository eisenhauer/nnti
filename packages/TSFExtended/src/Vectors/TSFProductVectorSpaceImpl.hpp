/* @HEADER@ */
/* ***********************************************************************
// 
//           TSFExtended: Trilinos Solver Framework Extended
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
// **********************************************************************/
/* @HEADER@ */

#ifndef TSFPRODUCTVECTORSPACEIMPL_HPP
#define TSFPRODUCTVECTORSPACEIMPL_HPP

 //#include "TSFProductVectorSpace.hpp"
 #include "TSFProductVectorDecl.hpp"
#include "WorkspacePack.hpp"
 //#include "TSFCoreProductMultiVectorBase.hpp"

 
using namespace TSFExtended;
using namespace Teuchos;

using std::ostream;


//========================================================================
template <class Scalar>
ProductVectorSpace<Scalar>::
  ProductVectorSpace(const Teuchos::Array<const VectorSpace<Scalar> >
		     &vecSpaces)
    :vecSpaces_(vecSpaces),
     numBlocks_(vecSpaces.size())
  {
    finalize();

    setUpCore();
  }
    



//========================================================================
template <class Scalar>
void ProductVectorSpace<Scalar>::setUpCore()
{
    Teuchos::RefCountPtr<const TSFCore::VectorSpace<Scalar> > 
      coreVecSpaces[numBlocks_];
    for (int i = 0; i < numBlocks_; i++)
      {
	coreVecSpaces[i] = vecSpaces_[i].ptr();
      }
    initialize(numBlocks_, coreVecSpaces);
}






//========================================================================
template <class Scalar>
ProductVectorSpace<Scalar>::
  ProductVectorSpace(const VectorSpace<Scalar>& s1, 
		     const VectorSpace<Scalar>& s2)
{
  vecSpaces_.resize(2);
  vecSpaces_[0] = s1;
  vecSpaces_[1] = s2;
  numBlocks_ = 2;

  finalize();
  setUpCore();
}



//========================================================================
template <class Scalar>
void ProductVectorSpace<Scalar>::finalize()
{
  isInCore_ = true;
  dim_ = 0;
  for (int i = 0; i < numBlocks_; i++)
    {
      if (!vecSpaces_[i].isInCore()) isInCore_ = false;
      dim_ += vecSpaces_[i].dim();
    }
}



//========================================================================
template <class Scalar>
bool ProductVectorSpace<Scalar>::operator==(const VectorSpace<Scalar> &other) 
  const
{
  const ProductVectorSpace<Scalar>* otherPVS = 
    dynamic_cast<const ProductVectorSpace<Scalar>* > (other.ptr());
  if (otherPVS == 0)
    {
      return false;
    }
  for (int i = 0; i < numBlocks_; i++)
    {
      if (vecSpaces_[i] != other->getBlock(i)) 
	{
	  return false;
	} 
    }
  return true;
}



//========================================================================
template <class Scalar>
bool ProductVectorSpace<Scalar>::operator!=(const VectorSpace<Scalar>& other) 
const
{
  return !(operator==(other));
}






//========================================================================
template <class Scalar>
RefCountPtr<TSFCore::Vector<Scalar> >ProductVectorSpace<Scalar>::createMember() const
  {
    cerr << "In ProdVecspace:createMember: numBlocks_ = " << numBlocks_ << endl;
    RefCountPtr<const ProductVectorSpace<Scalar> > me = rcp(this, false);
    VectorSpace<Scalar> vs(me);
    cerr << "In PVSImpl: createMember: about to create PV" << endl;
    Vector<Scalar> v = new ProductVector<Scalar>(vs);
    cerr << "   returning the vector" << endl;
    return v.ptr();
  }



//========================================================================
template <class Scalar>
string ProductVectorSpace<Scalar>::describe(int depth) const
{
  string ret = "";
  for (int i = 0; i < depth; i++)
    {
      ret.append("   ");
    }
  ret.append("ProductVectorSpace of dimension " + toString(dim()) + "\n");
  for (int i = 0; i < numBlocks_; i++)
    {
      ret.append(vecSpaces_[i].describe(depth+1) + "\n");
    }
  return ret;
}






#endif
