/* @HEADER@ */
// ************************************************************************
// 
//                              Sundance
//                 Copyright (2005) Sandia Corporation
// 
// Copyright (year first published) Sandia Corporation.  Under the terms 
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
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
// Questions? Contact Kevin Long (krlong@sandia.gov), 
// Sandia National Laboratories, Livermore, California, USA
// 
// ************************************************************************
/* @HEADER@ */

#include "SundanceMeshBase.hpp"
#include "SundanceMesh.hpp"
#include "SundanceExceptions.hpp"
#include "Teuchos_Time.hpp"
#include "Teuchos_TimeMonitor.hpp"

using namespace SundanceStdMesh::Internal;
using namespace SundanceStdMesh;
using namespace Teuchos;
using namespace SundanceUtils;


MeshBase::MeshBase(int dim, const MPIComm& comm) 
  : dim_(dim), 
    comm_(comm),
    reorderer_(Mesh::defaultReorderer().createInstance(this))
{;}



static Time& facetGrabTimer() 
{
  static RefCountPtr<Time> rtn 
    = TimeMonitor::getNewTimer("unbatched facet grabbing"); 
  return *rtn;
}

void MeshBase::getFacetArray(int cellDim, int cellLID, int facetDim, 
                             Array<int>& facetLIDs,
                             Array<int>& facetOrientations) const
{
  TimeMonitor timer(facetGrabTimer());

  int nf = numFacets(cellDim, cellLID, facetDim);
  facetLIDs.resize(nf);
  facetOrientations.resize(nf);
  for (int f=0; f<nf; f++) 
    {
      facetLIDs[f] = facetLID(cellDim, cellLID, facetDim, f, 
                              facetOrientations[f]);
    }
}


void MeshBase::getLabels(int cellDim, const Array<int>& cellLID, 
                         Array<int>& labels) const
{
  labels.resize(cellLID.size());
  for (unsigned int i=0; i<cellLID.size(); i++) labels[i] = label(cellDim, cellLID[i]);
}




