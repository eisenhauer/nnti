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

#include "SundanceHomogeneousDOFMap.hpp"
#include "SundanceCellFilter.hpp"
#include "SundanceMaximalCellFilter.hpp"
#include "Teuchos_MPIContainerComm.hpp"
#include "SundanceOut.hpp"
#include "SundanceTabs.hpp"
#include "Teuchos_Time.hpp"
#include "Teuchos_TimeMonitor.hpp"

using namespace SundanceStdFwk;
using namespace SundanceStdFwk::Internal;
using namespace SundanceCore::Internal;
using namespace Teuchos;


static Time& dofLookupTimer() 
{
  static RefCountPtr<Time> rtn 
    = TimeMonitor::getNewTimer("unbatched dof lookup"); 
  return *rtn;
}

static Time& dofBatchLookupTimer() 
{
  static RefCountPtr<Time> rtn 
    = TimeMonitor::getNewTimer("batched dof lookup"); 
  return *rtn;
}

HomogeneousDOFMap::HomogeneousDOFMap(const Mesh& mesh, 
                                     const BasisFamily& basis,
                                     int numFuncs)
  : DOFMapBase(mesh), 
    dim_(mesh.spatialDim()),
    dofs_(mesh.spatialDim()+1),
    maximalDofs_(),
    haveMaximalDofs_(false),
    localNodePtrs_(mesh.spatialDim()+1),
    nNodesPerCell_(mesh.spatialDim()+1),
    totalNNodesPerCell_(mesh.spatialDim()+1, 0),
    numFacets_(mesh.spatialDim()+1),
    basisIsContinuous_(false)
{
  verbosity() = DOFMapBase::classVerbosity();
  
  CellFilter maximalCells = new MaximalCellFilter();
  cellSets().append(maximalCells.getCells(mesh));
  cellDimOnCellSets().append(mesh.spatialDim());

  allocate(mesh, basis, numFuncs);
  initMap();
}


HomogeneousDOFMap::HomogeneousDOFMap(const Mesh& mesh, 
                                     const BasisFamily& basis,
                                     const Array<CellFilter>& subregions,
                                     int numFuncs)
  : DOFMapBase(mesh), 
    dim_(mesh.spatialDim()),
    dofs_(mesh.spatialDim()+1),
    maximalDofs_(),
    haveMaximalDofs_(false),
    localNodePtrs_(mesh.spatialDim()+1),
    nNodesPerCell_(mesh.spatialDim()+1),
    totalNNodesPerCell_(mesh.spatialDim()+1, 0),
    numFacets_(mesh.spatialDim()+1),
    basisIsContinuous_(false)
{
  verbosity() = DOFMapBase::classVerbosity();
  
  for (unsigned int r=0; r<subregions.size(); r++)
    {
      cellSets().append(subregions[r].getCells(mesh));
      cellDimOnCellSets().append(subregions[r].dimension(mesh));
    }

  allocate(mesh, basis, numFuncs);
  initMap();
}


void HomogeneousDOFMap::allocate(const Mesh& mesh, 
                                 const BasisFamily& basis,
                                 int numFuncs)
{
  Array<int> fid(numFuncs);
  for (int f=0; f<numFuncs; f++) fid[f] = f;
  funcIDOnCellSets().append(fid);
  
  for (int d=0; d<=dim_; d++)
    {
      /* record the number of facets for each cell type so we're
       * not making a bunch of mesh calls */
      numFacets_[d].resize(d);
      for (int fd=0; fd<d; fd++) numFacets_[d][fd]=mesh.numFacets(d, 0, fd);
      SUNDANCE_OUT(this->verbosity() > VerbMedium, 
                   "num facets for dimension " << d << " is " 
                   << numFacets_[d]);
          
      /* look up the node pointer for this cell and for all of its
       * facets */
      basis.ptr()->getLocalDOFs(mesh.cellType(d), localNodePtrs_[d]);

      SUNDANCE_OUT(this->verbosity() > VerbMedium, 
                   "node ptrs for dimension " << d << " are " 
                   << localNodePtrs_[d]);

      /* with the node pointers in hand, we can work out the number
       * of nodes per cell in this dimension */
      if (localNodePtrs_[d][d].size() > 0) 
        {
          nNodesPerCell_[d] = localNodePtrs_[d][d][0].size();
        }
      else
        {
          nNodesPerCell_[d] = 0;
        }
      SUNDANCE_OUT(this->verbosity() > VerbMedium, 
                   "num nodes for dimension " << d << " is " 
                   << nNodesPerCell_[d]);

      totalNNodesPerCell_[d] = nNodesPerCell_[d];
      for (int dd=0; dd<d; dd++) 
        {
          totalNNodesPerCell_[d] += numFacets_[d][dd]*nNodesPerCell_[dd];
        }

      /* we know from the mesh the number of cells in this dimension */
      if (nNodesPerCell_[d] > 0)
        {
          dofs_[d].resize(mesh.numCells(d));
        }
      else
        {
          dofs_[d].resize(0);
        }

      /* If any nodes are associated with the facets, then we know we have
       * a continuous basis function */
      if (d < dim_ && nNodesPerCell_[d] > 0) basisIsContinuous_ = true;


      /* now that we know the number of nodes per cell for this dimension,
       * we can allocate space for the DOFs in this dimension */
      int numCells = dofs_[d].size();
      for (int c=0; c<numCells; c++)
        {
          dofs_[d][c].resize(funcIDList().size() * nNodesPerCell_[d]);
          /* set everything to uninitializedVal() */
          for (unsigned int i=0; i<dofs_[d][c].size(); i++) 
            {
              dofs_[d][c][i] = uninitializedVal();
            }
        }
    }
}

void HomogeneousDOFMap::initMap()
{

  /* start the DOF count at zero. */
  int nextDOF = 0;


  /* Space in which to keep a list of remote cells needed by each processor
   * for each dimension. The first index is dimension, the second proc, the
   * third cell number. */
  Array<Array<Array<int> > > remoteCells(mesh().spatialDim()+1,
                                         mesh().comm().getNProc());
  
  for (int r=0; r<numCellSets(); r++)
    {
      /* Loop over maximal cells in the order specified by the cell iterator.
       * This might be reordered relative to the mesh. 
       *
       * At each maximal cell, we'll run through the facets and 
       * assign DOFs. That will take somewhat more work, but gives much 
       * better cache locality for the matrix because all the DOFs for
       * each maximal element and its facets are grouped together. */

      CellSet cells = cellSet(r);
      CellIterator iter;
      for (iter=cells.begin(); iter != cells.end(); iter++)
        {
          /* first assign any DOFs associated with the maximal cell */
          int cellLID = *iter;
          int owner;
      
          if (nNodesPerCell_[dim_] > 0)
            {
              /* if the maximal cell is owned by another processor,
               * put it in the list of cells for which we need to request 
               * DOF information from another processor */
              if (isRemote(dim_, cellLID, owner))
                {
                  int dummy=0;
                  int cellGID = mesh().mapLIDToGID(dim_, cellLID);
                  remoteCells[dim_][owner].append(cellGID); 
                  setDOFs(dim_, cellLID, dummy);
                }
              else /* the cell is locally owned, so we can 
                    * set its DOF numbers now */
                {
                  setDOFs(dim_, cellLID, nextDOF);
                }
            }

          /* Now assign any DOFs associated with the facets. */
          /* We can skip this step if the basis is discontinuous at element
           * boundaries, because the facets will own no nodes */
          if (basisIsContinuous_)
            {
              for (int d=0; d<dim_; d++)
                {
                  if (nNodesPerCell_[d] > 0)
                    {
                      int nf = numFacets_[dim_][d];
                      Array<int> facetLID(nf);
                      /* look up the LIDs of the facets */
                      mesh().getFacetArray(dim_, cellLID, d, facetLID);
                      /* for each facet, process its DOFs */
                      for (int f=0; f<nf; f++)
                        {
                          /* if the facet's DOFs have been assigned already,
                           * we're done */
                          if (!hasBeenAssigned(d, facetLID[f]))
                            {
                              /* the facet may be owned by another processor */
                              if (isRemote(d, facetLID[f], owner))
                                {
                                  int dummy=0;
                                  int facetGID = mesh().mapLIDToGID(d, facetLID[f]);
                                  remoteCells[d][owner].append(facetGID);
                                  setDOFs(d, facetLID[f], dummy);
                                }
                              else /* we can assign a DOF locally */
                                {
                                  setDOFs(d, facetLID[f], nextDOF);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
  /* Done with first pass, in which we have assigned DOFs for all
   * local processors. We now have to share DOF information between
   * processors */

  if (mesh().comm().getNProc() > 1)
    {
      for (int d=0; d<=dim_; d++)
        {
          if (nNodesPerCell_[d] > 0)
            {
              computeOffsets(d, nextDOF);
              shareDOFs(d, remoteCells[d]);
            }
        }
    }
  else
    {
      setLowestLocalDOF(0);
      setNumLocalDOFs(nextDOF);
      setTotalNumDOFs(nextDOF);
    }
  
}

void HomogeneousDOFMap::shareDOFs(int cellDim,
                                  const Array<Array<int> >& outgoingCellRequests)
{
  
  Array<Array<int> > incomingCellRequests;
  Array<Array<int> > outgoingDOFs(outgoingCellRequests.size());
  Array<Array<int> > incomingDOFs;

  SUNDANCE_OUT(this->verbosity() > VerbMedium,  
               "p=" << mesh().comm().getRank()
               << "synchronizing DOFs for cells of dimension " << cellDim);
  SUNDANCE_OUT(this->verbosity() > VerbMedium,  
               "p=" << mesh().comm().getRank()
               << " sending cell reqs d=" << cellDim << " GID=" << outgoingCellRequests);

  /* share the cell requests */
  MPIContainerComm<int>::allToAll(outgoingCellRequests, 
                                  incomingCellRequests,
                                  mesh().comm());

  

  SUNDANCE_OUT(this->verbosity() > VerbMedium,  
               "p=" << mesh().comm().getRank()
               << "recvd DOF requests for cells of dimension " << cellDim
               << " GID=" << incomingCellRequests);

  /* get DOF numbers for the first node of every cell that's been 
   * requested by someone else */
  for (int p=0; p<mesh().comm().getNProc(); p++)
    {
      if (p==mesh().comm().getRank()) continue;
      const Array<int>& requestsFromProc = incomingCellRequests[p];
      outgoingDOFs[p].resize(requestsFromProc.size());
      SUNDANCE_OUT(this->verbosity() > VerbHigh,  
                   "p=" << mesh().comm().getRank() << " recv'd from proc=" << p
                   << " reqs for DOFs for cells " << requestsFromProc);
      for (unsigned int c=0; c<requestsFromProc.size(); c++)
        {
          int GID = requestsFromProc[c];
          SUNDANCE_OUT(this->verbosity() > VerbHigh,  
                       "p=" << mesh().comm().getRank() 
                       << " processing cell with d=" << cellDim 
                       << " GID=" << GID);
          int LID = mesh().mapGIDToLID(cellDim, GID);
          SUNDANCE_OUT(this->verbosity() > VerbHigh,  
                       "p=" << mesh().comm().getRank() 
                       << " LID=" << LID << " dofs=" << dofs_[cellDim]);
          outgoingDOFs[p][c] = dofs_[cellDim][LID][0];
          SUNDANCE_OUT(this->verbosity() > VerbHigh,  
                       "p=" << mesh().comm().getRank() 
                       << " done processing cell with GID=" << GID);
        }
    }
 
  

  SUNDANCE_OUT(this->verbosity() > VerbMedium,  
               "p=" << mesh().comm().getRank()
               << "answering DOF requests for cells of dimension " << cellDim);

  /* share the DOF numbers */
  MPIContainerComm<int>::allToAll(outgoingDOFs,
                                  incomingDOFs,
                                  mesh().comm());

  SUNDANCE_OUT(this->verbosity() > VerbMedium,  
               "p=" << mesh().comm().getRank()
               << "communicated DOF answers for cells of dimension " << cellDim);

  
  /* now assign the DOFs from the other procs */
  for (int p=0; p<mesh().comm().getNProc(); p++)
    {
      if (p==mesh().comm().getRank()) continue;
      const Array<int>& dofsFromProc = incomingDOFs[p];
      for (unsigned int c=0; c<dofsFromProc.size(); c++)
        {
          int cellGID = outgoingCellRequests[p][c];
          int cellLID = mesh().mapGIDToLID(cellDim, cellGID);
          int dof = dofsFromProc[c];
          setDOFs(cellDim, cellLID, dof, true);
        }
    }
  
}



void HomogeneousDOFMap::setDOFs(int cellDim, int cellLID, int& nextDOF,
                                bool isRemote)
{
  Array<int>& cellDOFs = dofs_[cellDim][cellLID];
  
  int nn = nNodesPerCell_[cellDim];
  int nf = funcIDList().size();

  for (int i=0; i<nn; i++)
    {
      for (int f=0; f<nf; f++)
        {
          int k = nextDOF++;
          cellDOFs[funcIDList()[f] + nf*i] = k;
          if (isRemote) addGhostIndex(k);
        }
    }
}



void HomogeneousDOFMap::getDOFsForCellBatch(int cellDim, 
                                            const Array<int>& cellLID,
                                            Array<int>& dofs,
                                            unsigned int& nNodes) const 
{
  TimeMonitor timer(dofBatchLookupTimer());

  SUNDANCE_OUT(this->verbosity() > VerbHigh, "getting DOFs for cellDim=" << cellDim
               << " cellLID=" << cellLID);

  if (cellLID.size()==0) return;

  if (cellDim == dim_)
    {
      if (!haveMaximalDofs_) 
        {
          buildMaximalDofTable();
        }


      nNodes = totalNNodesPerCell_[cellDim];
      int nCells = cellLID.size();
      int nTotalCells = mesh().numCells(cellDim);
      int nf = funcIDList().size();
      dofs.resize(totalNNodesPerCell_[cellDim] * cellLID.size() * nf);

      for (unsigned int c=0; c<cellLID.size(); c++)
        {
          for (int fid=0; fid<nf; fid++)
            {
              for (unsigned int n=0; n<nNodes; n++) 
                {
                  dofs[(fid*nCells + c)*nNodes + n] 
                    = maximalDofs_[(fid*nTotalCells + cellLID[c])*nNodes + n];
                }
            }
        }
    }
  else
    {
      int nf = funcIDList().size();
      int nCells = cellLID.size();
      dofs.resize(totalNNodesPerCell_[cellDim] * cellLID.size() * nf);

      SUNDANCE_OUT(this->verbosity() > VerbHigh, "nf=" << nf
                   << " total nNodes=" << totalNNodesPerCell_[cellDim]);
  
      static Array<Array<int> > facetLID(3);
      static Array<int> numFacets(3);

      for (int d=0; d<cellDim; d++) 
        {
          numFacets[d] = mesh().numFacets(cellDim, cellLID[0], d);
          mesh().getFacetLIDs(cellDim, cellLID, d, facetLID[d]);
        }

  
      int nInteriorNodes = localNodePtrs_[cellDim][cellDim][0].size();
  
      nNodes = totalNNodesPerCell_[cellDim];
      for (unsigned int c=0; c<cellLID.size(); c++)
        {
          /* first get the DOFs for the nodes associated with 
           * the cell's interior */
          //      const int* tmpPtr1 = &(localNodePtrs_[cellDim][cellDim][0][0]);
          //      const int* tmpPtr2 = &(dofs_[cellDim][cellLID[c]][0]);
          for (int n=0; n<nInteriorNodes; n++)
            {
              int ptr = localNodePtrs_[cellDim][cellDim][0][n];
              for (int f=0; f<nf; f++)
                {
                  //              dofs[(c*nf + f)*nNodes+ptr] 
                  dofs[(f*nCells + c)*nNodes+ptr] 
                    = dofs_[cellDim][cellLID[c]][f + nf*n];
                }
              //          dofs[c*nNodes + ptr] = tmpPtr2[funcID + nf*n];
            }

          /* now get the DOFs for the nodes on the facets */
          for (int d=0; d<cellDim; d++)
            {
              // mesh().getFacetArray(cellDim, cellLID[c], d, facetLID[d]);

              SUNDANCE_OUT(this->verbosity() > VerbHigh, 
                           "d=" << d << " facets are " << facetLID[d]);
      
              for (int f=0; f<numFacets[d]; f++)
                {
                  int facetID = facetLID[d][c*numFacets[d]+f];
                  SUNDANCE_OUT(this->verbosity() > VerbHigh && localNodePtrs_[cellDim][d][f].size() != 0, 
                               "dofs for all nodes of this facet are: "
                               << dofs_[d][facetLID[d][f]]);
                  for (unsigned int n=0; n<localNodePtrs_[cellDim][d][f].size(); n++)
                    {
                      SUNDANCE_OUT(this->verbosity() > VerbHigh, "n=" << n);
                      int ptr = localNodePtrs_[cellDim][d][f][n];
                      SUNDANCE_OUT(this->verbosity() > VerbHigh, "local ptr=" << ptr);
                      for (int funcID=0; funcID<nf; funcID++)
                        {
                          SUNDANCE_OUT(this->verbosity() > VerbHigh, "found dof=" 
                                       << dofs_[d][facetID][funcID + nf*n]);
                          dofs[(funcID*nCells + c)*nNodes+ptr] 
                            = dofs_[d][facetID][funcID + nf*n];
                        }
                    }
                }
            }
        }
    }
}    

void HomogeneousDOFMap::buildMaximalDofTable() const
{
  Tabs tab;
  int cellDim = dim_;
  int nf = funcIDList().size();
  int nCells = mesh().numCells(dim_);

  SUNDANCE_VERB_MEDIUM(tab << "building dofs for maximal cells");

  SUNDANCE_OUT(this->verbosity() > VerbHigh, "nf=" << nf
               << " total nNodes=" << totalNNodesPerCell_[cellDim]);
  
  static Array<Array<int> > facetLID(3);
  static Array<int> numFacets(3);

  Array<int> cellLID(nCells);

  for (unsigned int c=0; c<cellLID.size(); c++) cellLID[c]=c;
  
  for (int d=0; d<cellDim; d++) 
    {
      numFacets[d] = mesh().numFacets(cellDim, cellLID[0], d);
      mesh().getFacetLIDs(cellDim, cellLID, d, facetLID[d]);
    }

  
  int nInteriorNodes = localNodePtrs_[cellDim][cellDim][0].size();
  

  int nNodes = totalNNodesPerCell_[cellDim];

  maximalDofs_.resize(nCells*nf*nNodes);

  for (int c=0; c<nCells; c++)
    {
      /* first get the DOFs for the nodes associated with 
       * the cell's interior */
      //      const int* tmpPtr1 = &(localNodePtrs_[cellDim][cellDim][0][0]);
      //      const int* tmpPtr2 = &(dofs_[cellDim][cellLID[c]][0]);
      for (int n=0; n<nInteriorNodes; n++)
        {
          int ptr = localNodePtrs_[cellDim][cellDim][0][n];
          for (int f=0; f<nf; f++)
            {
              //              dofs[(c*nf + f)*nNodes+ptr] 
              maximalDofs_[(f*nCells + c)*nNodes+ptr] 
                = dofs_[cellDim][c][f + nf*n];
            }
          //          dofs[c*nNodes + ptr] = tmpPtr2[funcID + nf*n];
        }
      
      /* now get the DOFs for the nodes on the facets */
      for (int d=0; d<cellDim; d++)
        {
          for (int f=0; f<numFacets[d]; f++)
            {
              int facetID = facetLID[d][c*numFacets[d]+f];
              SUNDANCE_OUT(this->verbosity() > VerbHigh && localNodePtrs_[cellDim][d][f].size() != 0,
                           "cellLID=" << cellLID[c] << ", facet dim="
                           << d << ", facetID=" << facetID);
              SUNDANCE_OUT(this->verbosity() > VerbHigh && localNodePtrs_[cellDim][d][f].size() != 0, 
                           "dofs for all nodes of this facet are: "
                           << dofs_[d][facetLID[d][f]]);
              for (unsigned int n=0; n<localNodePtrs_[cellDim][d][f].size(); n++)
                {
                  SUNDANCE_OUT(this->verbosity() > VerbHigh, "n=" << n);
                  int ptr = localNodePtrs_[cellDim][d][f][n];
                  SUNDANCE_OUT(this->verbosity() > VerbHigh, "local ptr=" << ptr);
                  for (int funcID=0; funcID<nf; funcID++)
                    {
                      SUNDANCE_OUT(this->verbosity() > VerbHigh, "found dof=" 
                                   << dofs_[d][facetID][funcID + nf*n])
                        //                    dofs[(c*nf + funcID)*nNodes+ptr] 
                        maximalDofs_[(funcID*nCells + c)*nNodes+ptr] 
                        = dofs_[d][facetID][funcID + nf*n];
                    }
                }
            }
        }
    }
  haveMaximalDofs_ = true;
}





void HomogeneousDOFMap::computeOffsets(int dim, int localCount)
{
  if (verbosity() > VerbMedium)
    {
      comm().synchronize();
      comm().synchronize();
      comm().synchronize();
      comm().synchronize();
    }
  SUNDANCE_OUT(this->verbosity() > VerbMedium, 
               "p=" << mesh().comm().getRank()
               << " sharing offsets for DOF numbering for dim=" << dim);

  SUNDANCE_OUT(this->verbosity() > VerbMedium, 
               "p=" << mesh().comm().getRank()
               << " I have " << localCount << " cells");

  Array<int> dofOffsets;
  int totalDOFCount;
  MPIContainerComm<int>::accumulate(localCount, dofOffsets, totalDOFCount,
                                    mesh().comm());
  int myOffset = dofOffsets[mesh().comm().getRank()];

  SUNDANCE_OUT(this->verbosity() > VerbMedium, 
               "p=" << mesh().comm().getRank()
               << " back from MPI accumulate");

  if (verbosity() > VerbMedium)
    {
      comm().synchronize();
      comm().synchronize();
      comm().synchronize();
      comm().synchronize();
    }

  for (unsigned int c=0; c<dofs_[dim].size(); c++)
    {
      if (hasBeenAssigned(dim, c))
        {
          for (unsigned int n=0; n<dofs_[dim][c].size(); n++) 
            {
              dofs_[dim][c][n] += myOffset;
            }
        }
    }

  setLowestLocalDOF(myOffset);
  setNumLocalDOFs(localCount);
  setTotalNumDOFs(totalDOFCount);

  SUNDANCE_OUT(this->verbosity() > VerbMedium, 
               "p=" << mesh().comm().getRank() 
               << " done sharing offsets for DOF numbering for dim=" << dim);
  if (verbosity() > VerbMedium)
    {
      comm().synchronize();
      comm().synchronize();
      comm().synchronize();
      comm().synchronize();
    }

}                           



void HomogeneousDOFMap::print(ostream& os) const
{
  int myRank = mesh().comm().getRank();

  Tabs tabs;

  os << "DOFS = " << dofs_ << endl;

  for (int p=0; p<mesh().comm().getNProc(); p++)
    {
      if (p == myRank)
        {
          os << tabs << 
            "========= DOFMap on proc p=" << p << " =============" << endl;
          for (int d=dim_; d>=0; d--)
            {
              Tabs tabs1;
              os << tabs1 << "dimension = " << d << endl;
              for (int c=0; c<mesh().numCells(d); c++)
                {
                  Tabs tabs2;
                  os << tabs2 << "Cell LID=" << c << " GID=" 
                     << mesh().mapLIDToGID(d, c) << endl;
                  for (unsigned int f=0; f<funcIDList().size(); f++)
                    {
                      Tabs tabs3;
                      Array<int> dofs;
                      getDOFsForCell(d, c, funcIDList()[f], dofs);
                      os << tabs3 << "f=" << funcIDList()[f] << " " 
                         << dofs << endl;
                    }
                }
            }
        }
      mesh().comm().synchronize();
      mesh().comm().synchronize();
    }
}


