/*
 * Navier2DBlocked_test.cpp
 *
 *  Created on: 16 Aug 2013
 *      Author: wiesner
 */




// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact
//                    Jeremie Gaidamour (jngaida@sandia.gov)
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
/*
 * Navier2D_epetra.cpp
 *
 *  Created on: Mar 26, 2011
 *      Author: wiesner
 */

#include <unistd.h>
#include <iostream>
#include <fstream>

// Teuchos
#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_CommandLineProcessor.hpp>
#include <Teuchos_GlobalMPISession.hpp>
#include <Teuchos_DefaultComm.hpp>
#include <Teuchos_ArrayRCP.hpp>
#include <Teuchos_ScalarTraits.hpp>

// Epetra
#include <EpetraExt_CrsMatrixIn.h>
#include <EpetraExt_VectorIn.h>
#include <EpetraExt_MultiVectorIn.h>

// Xpetra
#include <Xpetra_Map.hpp>
#include <Xpetra_MapFactory.hpp>
#include <Xpetra_Matrix.hpp>
#include <Xpetra_CrsMatrixWrap.hpp>
#include <Xpetra_VectorFactory.hpp>
#include <Xpetra_MultiVectorFactory.hpp>
#include <Xpetra_Parameters.hpp>
#include <Xpetra_MapExtractorFactory.hpp>
#include <Xpetra_BlockedCrsMatrix.hpp>
#include <Xpetra_StridedMapFactory.hpp>
#include <Xpetra_StridedEpetraMap.hpp>

// MueLu
#include "MueLu_ConfigDefs.hpp"
#include "MueLu_Memory.hpp"
#include "MueLu_Hierarchy.hpp"
#include "MueLu_CoupledAggregationFactory.hpp"
#include "MueLu_UncoupledAggregationFactory.hpp"
#include "MueLu_PgPFactory.hpp"
#include "MueLu_GenericRFactory.hpp"
#include "MueLu_SaPFactory.hpp"
#include "MueLu_TransPFactory.hpp"
#include "MueLu_BlockedRAPFactory.hpp"
#include "MueLu_TrilinosSmoother.hpp"
#include "MueLu_Utilities.hpp"
#include "MueLu_Exceptions.hpp"
#include "MueLu_Aggregates.hpp"
#include "MueLu_CoalesceDropFactory.hpp"
#include "MueLu_PreDropFunctionConstVal.hpp"
#include "MueLu_NullspaceFactory.hpp"
#include "MueLu_TentativePFactory.hpp"
#include "MueLu_SmootherFactory.hpp"
#include "MueLu_DirectSolver.hpp"
#include "MueLu_EpetraOperator.hpp"
#include "MueLu_SubBlockAFactory.hpp"
#include "MueLu_BlockedPFactory.hpp"
#include "MueLu_BlockedGaussSeidelSmoother.hpp"
#include "MueLu_SchurComplementFactory.hpp"
#include "MueLu_BraessSarazinSmoother.hpp"

#include "MueLu_CoarseMapFactory.hpp"

#include "MueLu_AmalgamationFactory.hpp"
#include "MueLu_AggregationExportFactory.hpp"

#if defined(HAVE_MPI) && defined(HAVE_MUELU_ZOLTAN) && defined(HAVE_MUELU_ISORROPIA)
#include "MueLu_RepartitionFactory.hpp"
#include "MueLu_RebalanceTransferFactory.hpp"
//#include "MueLu_ZoltanInterface.hpp"
#include "MueLu_IsorropiaInterface.hpp"
//#include "MueLu_RebalanceAcFactory.hpp"
#include "MueLu_RebalanceBlockAcFactory.hpp"
#include "MueLu_RebalanceBlockInterpolationFactory.hpp"
#include "MueLu_RebalanceBlockRestrictionFactory.hpp"
#endif

#include "MueLu_UseDefaultTypes.hpp"
#include "MueLu_UseShortNames.hpp"

#include <Epetra_LinearProblem.h>
#include <AztecOO.h>

// helper routines
  bool SplitMatrix2x2(Teuchos::RCP<const Epetra_CrsMatrix> A,
                      const Epetra_Map& A11rowmap,
                      const Epetra_Map& A22rowmap,
                      Teuchos::RCP<Epetra_CrsMatrix>& A11,
                      Teuchos::RCP<Epetra_CrsMatrix>& A12,
                      Teuchos::RCP<Epetra_CrsMatrix>& A21,
                      Teuchos::RCP<Epetra_CrsMatrix>& A22)
  {
    if (A==Teuchos::null)
      {
        std::cout << "ERROR: SplitMatrix2x2: A==null on entry" << std::endl;
        return false;
      }

    const Epetra_Comm& Comm   = A->Comm();
    const Epetra_Map&  A22map = A22rowmap;
    const Epetra_Map&  A11map = A11rowmap;

    //----------------------------- create a parallel redundant map of A22map
    std::map<int,int> a22gmap;
    {
      std::vector<int> a22global(A22map.NumGlobalElements());
      int count=0;
      for (int proc=0; proc<Comm.NumProc(); ++proc)
        {
          int length = 0;
          if (proc==Comm.MyPID())
            {
              for (int i=0; i<A22map.NumMyElements(); ++i)
                {
                  a22global[count+length] = A22map.GID(i);
                  ++length;
                }
            }
          Comm.Broadcast(&length,1,proc);
          Comm.Broadcast(&a22global[count],length,proc);
          count += length;
        }
      if (count != A22map.NumGlobalElements())
        {
          std::cout << "ERROR SplitMatrix2x2: mismatch in dimensions" << std::endl;
          return false;
        }

      // create the map
      for (int i=0; i<count; ++i)
        a22gmap[a22global[i]] = 1;
      a22global.clear();
    }

    //--------------------------------------------------- create matrix A22
    A22 = Teuchos::rcp(new Epetra_CrsMatrix(Copy,A22map,100));
    {
      std::vector<int>    a22gcindices(100);
      std::vector<double> a22values(100);
      for (int i=0; i<A->NumMyRows(); ++i)
        {
          const int grid = A->GRID(i);
          if (A22map.MyGID(grid)==false)
            continue;
          int     numentries;
          double* values;
          int*    cindices;
          int err = A->ExtractMyRowView(i,numentries,values,cindices);
          if (err)
            {
              std::cout << "ERROR: SplitMatrix2x2: A->ExtractMyRowView returned " << err << std::endl;
              return false;
            }

          if (numentries>(int)a22gcindices.size())
            {
              a22gcindices.resize(numentries);
              a22values.resize(numentries);
            }
          int count=0;
          for (int j=0; j<numentries; ++j)
            {
              const int gcid = A->ColMap().GID(cindices[j]);
              // see whether we have gcid in a22gmap
              std::map<int,int>::iterator curr = a22gmap.find(gcid);
              if (curr==a22gmap.end()) continue;
              //std::cout << gcid << " ";
              a22gcindices[count] = gcid;
              a22values[count]    = values[j];
              ++count;
            }
          //std::cout << std::endl; fflush(stdout);
          // add this filtered row to A22
          err = A22->InsertGlobalValues(grid,count,&a22values[0],&a22gcindices[0]);
          if (err<0)
            {
              std::cout << "ERROR: SplitMatrix2x2: A->InsertGlobalValues returned " << err << std::endl;
              return false;
            }

        } //for (int i=0; i<A->NumMyRows(); ++i)
      a22gcindices.clear();
      a22values.clear();
    }
    A22->FillComplete();
    A22->OptimizeStorage();

    //----------------------------------------------------- create matrix A11
    A11 = Teuchos::rcp(new Epetra_CrsMatrix(Copy,A11map,100));
    {
      std::vector<int>    a11gcindices(100);
      std::vector<double> a11values(100);
      for (int i=0; i<A->NumMyRows(); ++i)
        {
          const int grid = A->GRID(i);
          if (A11map.MyGID(grid)==false) continue;
          int     numentries;
          double* values;
          int*    cindices;
          int err = A->ExtractMyRowView(i,numentries,values,cindices);
          if (err)
            {
              std::cout << "ERROR: SplitMatrix2x2: A->ExtractMyRowView returned " << err << std::endl;
              return false;
            }

          if (numentries>(int)a11gcindices.size())
            {
              a11gcindices.resize(numentries);
              a11values.resize(numentries);
            }
          int count=0;
          for (int j=0; j<numentries; ++j)
            {
              const int gcid = A->ColMap().GID(cindices[j]);
              // see whether we have gcid as part of a22gmap
              std::map<int,int>::iterator curr = a22gmap.find(gcid);
              if (curr!=a22gmap.end()) continue;
              a11gcindices[count] = gcid;
              a11values[count] = values[j];
              ++count;
            }
          err = A11->InsertGlobalValues(grid,count,&a11values[0],&a11gcindices[0]);
          if (err<0)
            {
              std::cout << "ERROR: SplitMatrix2x2: A->InsertGlobalValues returned " << err << std::endl;
              return false;
            }

        } // for (int i=0; i<A->NumMyRows(); ++i)
      a11gcindices.clear();
      a11values.clear();
    }
    A11->FillComplete();
    A11->OptimizeStorage();

    //---------------------------------------------------- create matrix A12
    A12 = Teuchos::rcp(new Epetra_CrsMatrix(Copy,A11map,100));
    {
      std::vector<int>    a12gcindices(100);
      std::vector<double> a12values(100);
      for (int i=0; i<A->NumMyRows(); ++i)
        {
          const int grid = A->GRID(i);
          if (A11map.MyGID(grid)==false) continue;
          int     numentries;
          double* values;
          int*    cindices;
          int err = A->ExtractMyRowView(i,numentries,values,cindices);
          if (err)
            {
              std::cout << "ERROR: SplitMatrix2x2: A->ExtractMyRowView returned " << err << std::endl;
              return false;
            }

          if (numentries>(int)a12gcindices.size())
            {
              a12gcindices.resize(numentries);
              a12values.resize(numentries);
            }
          int count=0;
          for (int j=0; j<numentries; ++j)
            {
              const int gcid = A->ColMap().GID(cindices[j]);
              // see whether we have gcid as part of a22gmap
              std::map<int,int>::iterator curr = a22gmap.find(gcid);
              if (curr==a22gmap.end()) continue;
              a12gcindices[count] = gcid;
              a12values[count] = values[j];
              ++count;
            }
          err = A12->InsertGlobalValues(grid,count,&a12values[0],&a12gcindices[0]);
          if (err<0)
            {
              std::cout << "ERROR: SplitMatrix2x2: A->InsertGlobalValues returned " << err << std::endl;
              return false;
            }

        } // for (int i=0; i<A->NumMyRows(); ++i)
      a12values.clear();
      a12gcindices.clear();
    }
    A12->FillComplete(A22map,A11map);
    A12->OptimizeStorage();

    //----------------------------------------------------------- create A21
    A21 = Teuchos::rcp(new Epetra_CrsMatrix(Copy,A22map,100));
    {
      std::vector<int>    a21gcindices(100);
      std::vector<double> a21values(100);
      for (int i=0; i<A->NumMyRows(); ++i)
        {
          const int grid = A->GRID(i);
          if (A22map.MyGID(grid)==false) continue;
          int     numentries;
          double* values;
          int*    cindices;
          int err = A->ExtractMyRowView(i,numentries,values,cindices);
          if (err)
            {
              std::cout << "ERROR: SplitMatrix2x2: A->ExtractMyRowView returned " << err << std::endl;
              return false;
            }

          if (numentries>(int)a21gcindices.size())
            {
              a21gcindices.resize(numentries);
              a21values.resize(numentries);
            }
          int count=0;
          for (int j=0; j<numentries; ++j)
            {
              const int gcid = A->ColMap().GID(cindices[j]);
              // see whether we have gcid as part of a22gmap
              std::map<int,int>::iterator curr = a22gmap.find(gcid);
              if (curr!=a22gmap.end()) continue;
              a21gcindices[count] = gcid;
              a21values[count] = values[j];
              ++count;
            }
          err = A21->InsertGlobalValues(grid,count,&a21values[0],&a21gcindices[0]);
          if (err<0)
            {
              std::cout << "ERROR: SplitMatrix2x2: A->InsertGlobalValues returned " << err << std::endl;
              return false;
            }

        } // for (int i=0; i<A->NumMyRows(); ++i)
      a21values.clear();
      a21gcindices.clear();
    }
    A21->FillComplete(A11map,A22map);
    A21->OptimizeStorage();

    //-------------------------------------------------------------- tidy up
    a22gmap.clear();
    return true;
  }

/*!
 *  2d Navier Stokes example (for Epetra)
 *
 *  using block matrices
 */


int main(int argc, char *argv[]) {
  using Teuchos::RCP;
  using Teuchos::rcp;

  Teuchos::oblackholestream blackhole;
  Teuchos::GlobalMPISession mpiSession(&argc,&argv,&blackhole);
  //
  RCP<const Teuchos::Comm<int> > comm = Teuchos::DefaultComm<int>::getComm();
  RCP<Teuchos::FancyOStream> out = Teuchos::fancyOStream(Teuchos::rcpFromRef(std::cout));
  out->setOutputToRootOnly(0);
  *out << MueLu::MemUtils::PrintMemoryUsage() << std::endl;

  // Timing
  Teuchos::Time myTime("global");
  Teuchos::TimeMonitor MM(myTime);

#ifndef HAVE_TEUCHOS_LONG_LONG_INT
  *out << "Warning: scaling test was not compiled with long long int support" << std::endl;
#endif

  // custom parameters
  LocalOrdinal maxLevels = 3;

  GlobalOrdinal maxCoarseSize=1; //FIXME clp doesn't like long long int

  int globalNumDofs = 8898;  // used for the maps
  int nDofsPerNode = 3;      // used for generating the fine level null-space

  // build strided maps
  // striding information: 2 velocity dofs and 1 pressure dof = 3 dofs per node
  std::vector<size_t> stridingInfo;
  stridingInfo.push_back(2);
  stridingInfo.push_back(1);

  /////////////////////////////////////// build strided maps
  // build strided maps:
  // xstridedfullmap: full map (velocity and pressure dof gids), continous
  // xstridedvelmap: only velocity dof gid maps (i.e. 0,1,3,4,6,7...)
  // xstridedpremap: only pressure dof gid maps (i.e. 2,5,8,...)
  Teuchos::RCP<Xpetra::StridedEpetraMap> xstridedfullmap = Teuchos::rcp_dynamic_cast<Xpetra::StridedEpetraMap>(StridedMapFactory::Build(Xpetra::UseEpetra,globalNumDofs,0,stridingInfo,comm,-1));
  Teuchos::RCP<Xpetra::StridedEpetraMap> xstridedvelmap = Teuchos::rcp_dynamic_cast<Xpetra::StridedEpetraMap>(Xpetra::StridedMapFactory<int,int>::Build(xstridedfullmap,0));
  Teuchos::RCP<Xpetra::StridedEpetraMap> xstridedpremap = Teuchos::rcp_dynamic_cast<Xpetra::StridedEpetraMap>(Xpetra::StridedMapFactory<int,int>::Build(xstridedfullmap,1));

  /////////////////////////////////////// transform Xpetra::Map objects to Epetra
  // this is needed for AztecOO
  const Teuchos::RCP<const Epetra_Map> fullmap = Teuchos::rcpFromRef(xstridedfullmap->getEpetra_Map());
  Teuchos::RCP<const Epetra_Map> velmap = Teuchos::rcpFromRef(xstridedvelmap->getEpetra_Map());
  Teuchos::RCP<const Epetra_Map> premap = Teuchos::rcpFromRef(xstridedpremap->getEpetra_Map());

  /////////////////////////////////////// import problem matrix and RHS from files (-> Epetra)

  // read in problem
  Epetra_CrsMatrix * ptrA = 0;
  Epetra_Vector * ptrf = 0;
  Epetra_MultiVector* ptrNS = 0;

  *out << "Reading matrix market file" << std::endl;

  EpetraExt::MatrixMarketFileToCrsMatrix("A5932_re1000.txt",*fullmap,*fullmap,*fullmap,ptrA);
  EpetraExt::MatrixMarketFileToVector("b5932_re1000.txt",*fullmap,ptrf);
  //EpetraExt::MatrixMarketFileToCrsMatrix("/home/tobias/promotion/trilinos/fc17-dyn/packages/muelu/test/navierstokes/A5932_re1000.txt",*fullmap,*fullmap,*fullmap,ptrA);
  //EpetraExt::MatrixMarketFileToVector("/home/tobias/promotion/trilinos/fc17-dyn/packages/muelu/test/navierstokes/b5932_re1000.txt",*fullmap,ptrf);

  RCP<Epetra_CrsMatrix> epA = Teuchos::rcp(ptrA);
  RCP<Epetra_Vector> epv = Teuchos::rcp(ptrf);
  RCP<Epetra_MultiVector> epNS = Teuchos::rcp(ptrNS);


  /////////////////////////////////////// split system into 2x2 block system

  *out << "Split matrix into 2x2 block matrix" << std::endl;

  // split fullA into A11,..., A22
  Teuchos::RCP<Epetra_CrsMatrix> A11;
  Teuchos::RCP<Epetra_CrsMatrix> A12;
  Teuchos::RCP<Epetra_CrsMatrix> A21;
  Teuchos::RCP<Epetra_CrsMatrix> A22;

  if(SplitMatrix2x2(epA,*velmap,*premap,A11,A12,A21,A22)==false)
    *out << "Problem with splitting matrix"<< std::endl;

  /////////////////////////////////////// transform Epetra objects to Xpetra (needed for MueLu)

  // build Xpetra objects from Epetra_CrsMatrix objects
  Teuchos::RCP<Xpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> > xA11 = Teuchos::rcp(new Xpetra::EpetraCrsMatrix(A11));
  Teuchos::RCP<Xpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> > xA12 = Teuchos::rcp(new Xpetra::EpetraCrsMatrix(A12));
  Teuchos::RCP<Xpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> > xA21 = Teuchos::rcp(new Xpetra::EpetraCrsMatrix(A21));
  Teuchos::RCP<Xpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> > xA22 = Teuchos::rcp(new Xpetra::EpetraCrsMatrix(A22));

  /////////////////////////////////////// generate MapExtractor object

  std::vector<Teuchos::RCP<const Xpetra::Map<LocalOrdinal,GlobalOrdinal,Node> > > xmaps;
  xmaps.push_back(xstridedvelmap);
  xmaps.push_back(xstridedpremap);

  Teuchos::RCP<const Xpetra::MapExtractor<Scalar,LocalOrdinal,GlobalOrdinal,Node> > map_extractor = Xpetra::MapExtractorFactory<Scalar,LocalOrdinal,GlobalOrdinal>::Build(xstridedfullmap,xmaps);

  /////////////////////////////////////// build blocked transfer operator
  // using the map extractor
  Teuchos::RCP<Xpetra::BlockedCrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> > bOp = Teuchos::rcp(new Xpetra::BlockedCrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal>(map_extractor,map_extractor,10));
  bOp->setMatrix(0,0,xA11);
  bOp->setMatrix(0,1,xA12);
  bOp->setMatrix(1,0,xA21);
  bOp->setMatrix(1,1,xA22);

  bOp->fillComplete();

  //////////////////////////////////////////////////// create Hierarchy
  RCP<Hierarchy> H = rcp ( new Hierarchy() );
  H->setDefaultVerbLevel(Teuchos::VERB_HIGH);
  //H->setDefaultVerbLevel(Teuchos::VERB_NONE);
  H->SetMaxCoarseSize(maxCoarseSize);

  //////////////////////////////////////////////////////// finest Level
  RCP<MueLu::Level> Finest = H->GetLevel();
  Finest->setDefaultVerbLevel(Teuchos::VERB_HIGH);
  Finest->Set("A",Teuchos::rcp_dynamic_cast<Matrix>(bOp));


  ////////////////////////////////////////// prepare null space for A11
  RCP<MultiVector> nullspace11 = MultiVectorFactory::Build(xstridedvelmap, 2);  // this is a 2D standard null space

  for (int i=0; i<nDofsPerNode-1; ++i) {
    Teuchos::ArrayRCP<Scalar> nsValues = nullspace11->getDataNonConst(i);
    int numBlocks = nsValues.size() / (nDofsPerNode - 1);
    for (int j=0; j< numBlocks; ++j) {
      nsValues[j*(nDofsPerNode - 1) + i] = 1.0;
    }
  }

  Finest->Set("Nullspace1",nullspace11);

  ////////////////////////////////////////// prepare null space for A22
  RCP<MultiVector> nullspace22 = MultiVectorFactory::Build(xstridedpremap, 1);  // this is a 2D standard null space
  Teuchos::ArrayRCP<Scalar> nsValues22 = nullspace22->getDataNonConst(0);
  for (int j=0; j< nsValues22.size(); ++j) {
    nsValues22[j] = 1.0;
  }

  Finest->Set("Nullspace2",nullspace22);


#if 1

  /////////////////////////////////////////// define rebalanced block AC factory
  // This is the main factory for "A" and defines the input for
  //   - the SubBlockAFactory objects
  //   - the rebalanced block Ac factory
  RCP<RebalanceBlockAcFactory> RebalancedAcFact = rcp(new RebalanceBlockAcFactory());

  /////////////////////////////////////////// define non-rebalanced blocked transfer ops
  RCP<BlockedPFactory> PFact = rcp(new BlockedPFactory()); // use row map index base from bOp
  RCP<GenericRFactory> RFact = rcp(new GenericRFactory());
  RFact->SetFactory("P", PFact);

  // non-rebalanced block coarse matrix factory
  // output is non-rebalanced coarse block matrix Ac
  // used as input for rebalanced block coarse factory RebalancedAcFact
  RCP<Factory> AcFact = rcp(new BlockedRAPFactory());
  AcFact->SetFactory("A", MueLu::NoFactory::getRCP()); /* XXX */ // todo check me
  AcFact->SetFactory("P", PFact);  // use non-rebalanced block prolongator as input
  AcFact->SetFactory("R", RFact);  // use non-rebalanced block restrictor as input

  // define matrix sub-blocks of possibly rebalanced block matrix A
  // These are used as input for
  //   - the sub blocks of the transfer operators
  RCP<SubBlockAFactory> A11Fact = Teuchos::rcp(new SubBlockAFactory(MueLu::NoFactory::getRCP()/*AcFact*/, 0, 0));
  RCP<SubBlockAFactory> A22Fact = Teuchos::rcp(new SubBlockAFactory(MueLu::NoFactory::getRCP()/*AcFact*/, 1, 1));

  /////////////////////////////////////////// define rebalancing factories
  // define sub blocks of the coarse non-rebalanced block matrix Ac
  // input is the block operator generated by AcFact
  RCP<SubBlockAFactory> rebA11Fact = Teuchos::rcp(new SubBlockAFactory(AcFact, 0, 0));
  RCP<SubBlockAFactory> rebA22Fact = Teuchos::rcp(new SubBlockAFactory(AcFact, 1, 1));

  // define rebalancing factory for coarse block matrix A(1,1)
  RCP<MueLu::IsorropiaInterface<LO, GO, NO, LMO> > isoInterface = rcp(new MueLu::IsorropiaInterface<LO, GO, NO, LMO>());
  isoInterface->SetFactory("A", rebA11Fact);

  // Repartitioning (creates "Importer" from "Partition")
  RCP<Factory> RepartitionFact = rcp(new RepartitionFactory());
  {
    Teuchos::ParameterList paramList;
    paramList.set("minRowsPerProcessor", 200);
    paramList.set("nonzeroImbalance", 1.3);
    RepartitionFact->SetParameterList(paramList);
  }
  RepartitionFact->SetFactory("A", rebA11Fact);
  RepartitionFact->SetFactory("Partition", isoInterface);

  ////////////////////////////////////////// build non-rebalanced matrix blocks
  // build factories for transfer operator P(1,1) and R(1,1)
  RCP<AmalgamationFactory> amalgFact11 = rcp(new AmalgamationFactory());
  amalgFact11->SetFactory("A", A11Fact);
  amalgFact11->setDefaultVerbLevel(Teuchos::VERB_EXTREME);

  RCP<CoalesceDropFactory> dropFact11 = rcp(new CoalesceDropFactory());
  dropFact11->SetFactory("A", A11Fact);
  dropFact11->SetFactory("UnAmalgamationInfo", amalgFact11);
  dropFact11->setDefaultVerbLevel(Teuchos::VERB_EXTREME);

  RCP<UncoupledAggregationFactory> UncoupledAggFact11 = rcp(new UncoupledAggregationFactory());
  UncoupledAggFact11->SetFactory("Graph", dropFact11);
  UncoupledAggFact11->SetMinNodesPerAggregate(9);
  UncoupledAggFact11->SetMaxNeighAlreadySelected(2);
  UncoupledAggFact11->SetOrdering(MueLu::AggOptions::NATURAL);

  RCP<CoarseMapFactory> coarseMapFact11 = Teuchos::rcp(new CoarseMapFactory());
  coarseMapFact11->setStridingData(stridingInfo);
  coarseMapFact11->setStridedBlockId(0);

  RCP<TentativePFactory> P11Fact = rcp(new TentativePFactory());
  RCP<TransPFactory> R11Fact = rcp(new TransPFactory());

  Teuchos::RCP<NullspaceFactory> nspFact11 = Teuchos::rcp(new NullspaceFactory("Nullspace1"));
  nspFact11->SetFactory("Nullspace1",P11Fact); // pick "Nullspace1" from Finest level

  //////////////////////////////// define factory manager for (1,1) block
  RCP<FactoryManager> M11 = rcp(new FactoryManager());
  M11->SetFactory("A", A11Fact);  // rebalanced fine-level block operator
  M11->SetFactory("P", P11Fact);  // non-rebalanced transfer operator block P(1,1)
  M11->SetFactory("R", R11Fact);  // non-rebalanced transfer operator block R(1,1)
  M11->SetFactory("Aggregates", UncoupledAggFact11);
  M11->SetFactory("UnAmalgamationInfo", amalgFact11);
  M11->SetFactory("Nullspace", nspFact11); // TODO check me?
  M11->SetFactory("CoarseMap", coarseMapFact11);
  M11->SetIgnoreUserData(true);               // always use data from factories defined in factory manager

  ////////////////////////////////////////// build non-rebalanced matrix blocks
  // build factories for transfer operator P(2,2) and R(2,2)
  RCP<AmalgamationFactory> amalgFact22 = rcp(new AmalgamationFactory());
  RCP<TentativePFactory> P22Fact = rcp(new TentativePFactory());
  RCP<TransPFactory> R22Fact = rcp(new TransPFactory());

  /*XXX*/
  RCP<CoalesceDropFactory> dropFact22 = rcp(new CoalesceDropFactory());
  dropFact22->SetFactory("A", A22Fact);
  dropFact22->SetFactory("UnAmalgamationInfo", amalgFact22);
  dropFact22->setDefaultVerbLevel(Teuchos::VERB_EXTREME);

  /*XXX*/
  RCP<UncoupledAggregationFactory> UncoupledAggFact22 = rcp(new UncoupledAggregationFactory());
  UncoupledAggFact22->SetFactory("Graph", dropFact22);
  UncoupledAggFact22->SetMinNodesPerAggregate(6);
  UncoupledAggFact22->SetMaxNeighAlreadySelected(2);
  UncoupledAggFact22->SetOrdering(MueLu::AggOptions::NATURAL);

  // connect null space and tentative PFactory
  Teuchos::RCP<NullspaceFactory> nspFact22 = Teuchos::rcp(new NullspaceFactory("Nullspace2"));
  nspFact22->SetFactory("Nullspace2", P22Fact); // define null space generated by P22Fact as null space for coarse level (non-rebalanced)

  RCP<CoarseMapFactory> coarseMapFact22 = Teuchos::rcp(new CoarseMapFactory());
  coarseMapFact22->setStridingData(stridingInfo);
  coarseMapFact22->setStridedBlockId(1);

  //////////////////////////////// define factory manager for (2,2) block
  RCP<FactoryManager> M22 = rcp(new FactoryManager());
  M22->SetFactory("A", A22Fact); // rebalanced fine-level block operator
  M22->SetFactory("P", P22Fact); // non-rebalanced transfer operator P(2,2)
  M22->SetFactory("R", R22Fact); // non-rebalanced transfer operator R(2,2)
  M22->SetFactory("Aggregates", UncoupledAggFact22 /* UncoupledAggFact11 *//*XXX*/); // aggregates from block (1,1)
  M22->SetFactory("Nullspace", nspFact22); // TODO check me
  M22->SetFactory("UnAmalgamationInfo", amalgFact22);
  M22->SetFactory("Ptent", P22Fact);
  M22->SetFactory("CoarseMap", coarseMapFact22);
  M22->SetIgnoreUserData(true);

  /////////////////////////////////////////// define rebalanced blocked transfer ops
  //////////////////////////////// define factory manager for (1,1) block
  RCP<FactoryManager> rebM11 = rcp(new FactoryManager());
  rebM11->SetFactory("A", AcFact ); // important: must be a 2x2 block A Factory
  //rebM11->SetFactory("P", PFact); // use non-rebalanced block P operator as input
  //rebM11->SetFactory("R", RFact); // use non-rebalanced block R operator as input
  rebM11->SetFactory("Importer", RepartitionFact);
  rebM11->SetFactory("Nullspace", nspFact11);
  //rebM11->SetIgnoreUserData(true);

  // Reordering of the transfer operators
  RCP<RebalanceBlockInterpolationFactory> RebalancedBlockPFact = rcp(new RebalanceBlockInterpolationFactory());
  RebalancedBlockPFact->SetFactory("P", PFact); // use non-rebalanced block P operator as input
  RebalancedBlockPFact->AddFactoryManager(rebM11);

  RCP<RebalanceBlockRestrictionFactory> RebalancedBlockRFact = rcp(new RebalanceBlockRestrictionFactory());
  //RebalancedBlockRFact->SetParameter("type", Teuchos::ParameterEntry(std::string("Restriction")));
  RebalancedBlockRFact->SetFactory("R", RFact); // non-rebalanced block P operator
  RebalancedBlockRFact->AddFactoryManager(rebM11);

  ///////////////////////////////////////// initialize non-rebalanced block transfer operators
  // output are the non-rebalanced block transfer operators used as input in AcFact to build
  // the non-rebalanced coarse level block matrix Ac
  PFact->AddFactoryManager(M11);  // use non-rebalanced information from sub block factory manager M11
  PFact->AddFactoryManager(M22);  // use non-rebalanced information from sub block factory manager M22

  ///////////////////////////////////////// initialize rebalanced coarse block AC factory
  RebalancedAcFact->SetFactory("A", AcFact);   // use non-rebalanced block operator as input
  RebalancedAcFact->AddFactoryManager(rebM11); // -> TODO add Isorropia interface objects here

  //////////////////////////////////////////////////////////////////////
  // Smoothers

  //Another factory manager for braes sarazin smoother
  //Schur Complement Factory, using the factory to generate AcFact
    SC omega = 1.3;
    RCP<SchurComplementFactory> SFact = Teuchos::rcp(new SchurComplementFactory());
    SFact->SetParameter("omega", Teuchos::ParameterEntry(omega));
    SFact->SetFactory("A", MueLu::NoFactory::getRCP()); // this finally be the rebalanced block operator!

    //Smoother Factory, using SFact as a factory for A
    std::string ifpackSCType;
    Teuchos::ParameterList ifpackSCList;
    ifpackSCList.set("relaxation: sweeps", (LocalOrdinal) 3);
    ifpackSCList.set("relaxation: damping factor", (Scalar) 1.0);
    ifpackSCType = "RELAXATION";
    ifpackSCList.set("relaxation: type", "Gauss-Seidel");
    RCP<SmootherPrototype> smoProtoSC     = rcp( new TrilinosSmoother(ifpackSCType, ifpackSCList, 0, SFact) );
    RCP<SmootherFactory> SmooSCFact = rcp( new SmootherFactory(smoProtoSC) );

    RCP<BraessSarazinSmoother> smootherPrototype     = rcp( new BraessSarazinSmoother(3,omega) );
    smootherPrototype->SetFactory("A",MueLu::NoFactory::getRCP());
  RCP<SmootherFactory>   smootherFact          = rcp( new SmootherFactory(smootherPrototype) );

  RCP<BraessSarazinSmoother> coarseSolverPrototype = rcp( new BraessSarazinSmoother(3,omega) );
  coarseSolverPrototype->SetFactory("A",MueLu::NoFactory::getRCP());
  RCP<SmootherFactory>   coarseSolverFact      = rcp( new SmootherFactory(coarseSolverPrototype, Teuchos::null) );

  RCP<FactoryManager> MB = rcp(new FactoryManager());
  MB->SetFactory("A",     SFact);
  MB->SetFactory("Smoother",    SmooSCFact);
  MB->SetIgnoreUserData(true);               // always use data from factories defined in factory manager
  smootherPrototype->SetFactoryManager(MB);
  coarseSolverPrototype->SetFactoryManager(MB);


  ////////////////////////////////////////// define main factory manager
  FactoryManager M;
  M.SetFactory("A",            RebalancedAcFact);     // rebalance block AC Factory using importer
  M.SetFactory("P",            RebalancedBlockPFact); // rebalance prolongator using non-balanced Ac
  M.SetFactory("R",            RebalancedBlockRFact); // rebalance restrictor and null space using non-balanced Ac
  M.SetFactory("Smoother",     smootherFact);
  M.SetFactory("PreSmoother",     smootherFact);
  M.SetFactory("PostSmoother",     smootherFact);
  M.SetFactory("CoarseSolver", coarseSolverFact);

#else

  /////////////////////////////////////////////// define subblocks of A
  // make A11 block and A22 block available as variable "A" generated
  // by A11Fact and A22Fact
  RCP<SubBlockAFactory> A11Fact = Teuchos::rcp(new SubBlockAFactory(MueLu::NoFactory::getRCP(), 0, 0));
  RCP<SubBlockAFactory> A22Fact = Teuchos::rcp(new SubBlockAFactory(MueLu::NoFactory::getRCP(), 1, 1));

  ///////////////////////////////////////// define CoalesceDropFactory and Aggregation for A11
  // set up amalgamation for A11. Note: we're using a default null space factory (Teuchos::null)
  RCP<AmalgamationFactory> amalgFact11 = rcp(new AmalgamationFactory());
  amalgFact11->SetFactory("A", A11Fact);

  amalgFact11->setDefaultVerbLevel(Teuchos::VERB_EXTREME);
  RCP<CoalesceDropFactory> dropFact11 = rcp(new CoalesceDropFactory());
  dropFact11->SetFactory("A", A11Fact);
  dropFact11->SetFactory("UnAmalgamationInfo", amalgFact11);
  dropFact11->setDefaultVerbLevel(Teuchos::VERB_EXTREME);
  //RCP<CoupledAggregationFactory> CoupledAggFact11 = rcp(new CoupledAggregationFactory());
  RCP<UncoupledAggregationFactory> CoupledAggFact11 = rcp(new UncoupledAggregationFactory());
  CoupledAggFact11->SetFactory("Graph", dropFact11);
  CoupledAggFact11->SetMinNodesPerAggregate(9);
  CoupledAggFact11->SetMaxNeighAlreadySelected(2);
  CoupledAggFact11->SetOrdering(MueLu::AggOptions::NATURAL);
  //CoupledAggFact11->SetPhase3AggCreation(0.5);

  ///////////////////////////////////////// define transfer ops for A11
#if 0
  // use PG-AMG
  RCP<PgPFactory> P11Fact = rcp(new PgPFactory());

  RCP<GenericRFactory> R11Fact = rcp(new GenericRFactory());
  Teuchos::RCP<NullspaceFactory> nspFact11 = Teuchos::rcp(new NullspaceFactory("Nullspace1",P11tentFact));

  Teuchos::RCP<NullspaceFactory> nspFact11 = Teuchos::rcp(new NullspaceFactory("Nullspace1"));

  RCP<CoarseMapFactory> coarseMapFact11 = Teuchos::rcp(new CoarseMapFactory());
  coarseMapFact11->setStridingData(stridingInfo);
  coarseMapFact11->setStridedBlockId(0);

  //////////////////////////////// define factory manager for (1,1) block
  RCP<FactoryManager> M11 = rcp(new FactoryManager());
  M11->SetFactory("A", A11Fact);
  M11->SetFactory("P", P11Fact);
  M11->SetFactory("R", R11Fact);
  M11->SetFactory("Aggregates", CoupledAggFact11);
  M11->SetFactory("UnAmalgamationInfo", amalgFact11);
  M11->SetFactory("Nullspace", nspFact11);
  // M11->SetFactory("Ptent", P11tentFact);
  M11->SetFactory("CoarseMap", coarseMapFact11);
#else
  RCP<TentativePFactory> P11Fact = rcp(new TentativePFactory());

  RCP<TransPFactory> R11Fact = rcp(new TransPFactory());

  Teuchos::RCP<NullspaceFactory> nspFact11 = Teuchos::rcp(new NullspaceFactory("Nullspace1"));
  nspFact11->SetFactory("Nullspace1",P11Fact);

  RCP<CoarseMapFactory> coarseMapFact11 = Teuchos::rcp(new CoarseMapFactory());
  coarseMapFact11->setStridingData(stridingInfo);
  coarseMapFact11->setStridedBlockId(0);

  //////////////////////////////// define factory manager for (1,1) block
  RCP<FactoryManager> M11 = rcp(new FactoryManager());
  M11->SetFactory("A", A11Fact);
  M11->SetFactory("P", P11Fact);
  M11->SetFactory("R", R11Fact);
  M11->SetFactory("Aggregates", CoupledAggFact11);
  M11->SetFactory("UnAmalgamationInfo", amalgFact11);
  M11->SetFactory("Nullspace", nspFact11);
  // M11->SetFactory("Ptent", P11Fact);
  M11->SetFactory("CoarseMap", coarseMapFact11);
#endif
  M11->SetIgnoreUserData(true);               // always use data from factories defined in factory manager

  ////////////////////////////////////////// prepare null space for A22
  RCP<MultiVector> nullspace22 = MultiVectorFactory::Build(xstridedpremap, 1);  // this is a 2D standard null space
  Teuchos::ArrayRCP<Scalar> nsValues22 = nullspace22->getDataNonConst(0);
  for (int j=0; j< nsValues22.size(); ++j) {
    nsValues22[j] = 1.0;
  }

  Finest->Set("Nullspace2",nullspace22);

  ///////////////////////////////////////// define transfer ops for A22
#if 0
  // use PGAMG
  RCP<AmalgamationFactory> amalgFact22 = rcp(new AmalgamationFactory(A22Fact));
  RCP<TentativePFactory> P22tentFact = rcp(new TentativePFactory(CoupledAggFact11, amalgFact22));

  RCP<SaPFactory> P22Fact = rcp(new SaPFactory(P22tentFact));

  //RCP<GenericRFactory> R22Fact = rcp(new GenericRFactory(P22Fact));
  RCP<TransPFactory> R22Fact = rcp(new TransPFactory(P22Fact));

  Teuchos::RCP<NullspaceFactory> nspFact22 = Teuchos::rcp(new NullspaceFactory("Nullspace2",P22tentFact));
  RCP<CoarseMapFactory> coarseMapFact22 = Teuchos::rcp(new CoarseMapFactory(CoupledAggFact11, nspFact22));
  coarseMapFact22->setStridingData(stridingInfo);
  coarseMapFact22->setStridedBlockId(1);

  //////////////////////////////// define factory manager for (2,2) block
  RCP<FactoryManager> M22 = rcp(new FactoryManager());
  M22->SetFactory("A", A22Fact);
  M22->SetFactory("P", P22Fact);
  M22->SetFactory("R", R22Fact);
  M22->SetFactory("Aggregates", AggFact22);
  M22->SetFactory("Nullspace", nspFact22);
  M22->SetFactory("Ptent", P22tentFact);
  M22->SetFactory("CoarseMap", coarseMapFact22);
  M22->SetIgnoreUserData(true);               // always use data from factories defined in factory manager

#else
  // use TentativePFactory
  RCP<AmalgamationFactory> amalgFact22 = rcp(new AmalgamationFactory());
  RCP<TentativePFactory> P22Fact = rcp(new TentativePFactory()); // check me (fed with A22) wrong column GIDS!!!

  RCP<TransPFactory> R22Fact = rcp(new TransPFactory());

  Teuchos::RCP<NullspaceFactory> nspFact22 = Teuchos::rcp(new NullspaceFactory("Nullspace2"));
  nspFact22->SetFactory("Nullspace2", P22Fact);
  RCP<CoarseMapFactory> coarseMapFact22 = Teuchos::rcp(new CoarseMapFactory());
  coarseMapFact22->setStridingData(stridingInfo);
  coarseMapFact22->setStridedBlockId(1);

  //////////////////////////////// define factory manager for (2,2) block
  RCP<FactoryManager> M22 = rcp(new FactoryManager());
  M22->SetFactory("A", A22Fact);
  M22->SetFactory("P", P22Fact);
  M22->SetFactory("R", R22Fact);
  M22->SetFactory("Aggregates", CoupledAggFact11);
  M22->SetFactory("Nullspace", nspFact22);
  M22->SetFactory("UnAmalgamationInfo", amalgFact22); // TODO oops what about that? it was M11 before?
  M22->SetFactory("Ptent", P22Fact);
  M22->SetFactory("CoarseMap", coarseMapFact22);
  M22->SetIgnoreUserData(true);               // always use data from factories defined in factory manager
#endif

  /////////////////////////////////////////// define blocked transfer ops
  RCP<BlockedPFactory> PFact = rcp(new BlockedPFactory()); // use row map index base from bOp
  PFact->AddFactoryManager(M11);
  PFact->AddFactoryManager(M22);

  RCP<GenericRFactory> RFact = rcp(new GenericRFactory());
  RFact->SetFactory("P", PFact);

  RCP<Factory> AcFact = rcp(new BlockedRAPFactory());
  AcFact->SetFactory("P", PFact);
  AcFact->SetFactory("R", RFact);

  ///////////////////////// Experiment

  ////////////////////////////////////// EXPERIMENT
  // create "Partition"
  RCP<SubBlockAFactory> rebA11Fact = Teuchos::rcp(new SubBlockAFactory(AcFact, 0, 0));

  RCP<MueLu::IsorropiaInterface<LO, GO, NO, LMO> > isoInterface = rcp(new MueLu::IsorropiaInterface<LO, GO, NO, LMO>());
  isoInterface->SetFactory("A", rebA11Fact);

  // Repartitioning (creates "Importer" from "Partition")
  RCP<Factory> RepartitionFact = rcp(new RepartitionFactory());
  {
    Teuchos::ParameterList paramList;
    paramList.set("minRowsPerProcessor", 200);
    paramList.set("nonzeroImbalance", 1.3);
    RepartitionFact->SetParameterList(paramList);
  }
  RepartitionFact->SetFactory("A", rebA11Fact);
  RepartitionFact->SetFactory("Partition", isoInterface);

  //////////////////////////////// define factory manager for (1,1) block
  RCP<FactoryManager> rebM11 = rcp(new FactoryManager());
  rebM11->SetFactory("A", rebA11Fact);
  rebM11->SetFactory("Importer", RepartitionFact);
  rebM11->SetFactory("Nullspace", nspFact11);


  RCP<RebalanceBlockAcFactory> RebAcFact = rcp(new RebalanceBlockAcFactory());
  RebAcFact->SetFactory("A", AcFact);
  RebAcFact->AddFactoryManager(rebM11);

  // Reordering of the transfer operators
  /*RCP<RebalanceBlockTransferFactory> RebalancedBlockPFact = rcp(new RebalanceBlockTransferFactory());
  RebalancedBlockPFact->SetParameter("type", Teuchos::ParameterEntry(std::string("Interpolation")));
  RebalancedBlockPFact->SetFactory("P", PFact); // non-rebalanced block P operator
  RebalancedBlockPFact->AddFactoryManager(rebM11);*/
  RCP<RebalanceBlockInterpolationFactory> RebalancedBlockPFact = rcp(new RebalanceBlockInterpolationFactory());
  RebalancedBlockPFact->SetFactory("P", PFact); // non-rebalanced block P operator
  RebalancedBlockPFact->AddFactoryManager(rebM11);

  RCP<RebalanceBlockTransferFactory> RebalancedBlockRFact = rcp(new RebalanceBlockTransferFactory());
  RebalancedBlockRFact->SetParameter("type", Teuchos::ParameterEntry(std::string("Restriction")));
  RebalancedBlockRFact->SetFactory("R", RFact); // non-rebalanced block P operator
  RebalancedBlockRFact->SetFactory("Nullspace", rebM11->GetFactory("Ptent")); // TODO
  RebalancedBlockRFact->AddFactoryManager(rebM11);


  *out << "Creating Braess-Sarazin Smoother" << std::endl;

  //////////////////////////////////////////////////////////////////////
  // Smoothers

  //Another factory manager for braes sarazin smoother
  //Schur Complement Factory, using the factory to generate AcFact
  SC omega = 1.3;
    RCP<SchurComplementFactory> SFact = Teuchos::rcp(new SchurComplementFactory());
    SFact->SetParameter("omega", Teuchos::ParameterEntry(omega));
    SFact->SetFactory("A", MueLu::NoFactory::getRCP());

    //Smoother Factory, using SFact as a factory for A
    std::string ifpackSCType;
    Teuchos::ParameterList ifpackSCList;
    ifpackSCList.set("relaxation: sweeps", (LocalOrdinal) 3);
    ifpackSCList.set("relaxation: damping factor", (Scalar) 1.0);
    ifpackSCType = "RELAXATION";
    ifpackSCList.set("relaxation: type", "Gauss-Seidel");
    RCP<SmootherPrototype> smoProtoSC     = rcp( new TrilinosSmoother(ifpackSCType, ifpackSCList, 0, SFact) );
    RCP<SmootherFactory> SmooSCFact = rcp( new SmootherFactory(smoProtoSC) );

    RCP<BraessSarazinSmoother> smootherPrototype     = rcp( new BraessSarazinSmoother(3,omega) );

  RCP<SmootherFactory>   smootherFact          = rcp( new SmootherFactory(smootherPrototype) );

  RCP<BraessSarazinSmoother> coarseSolverPrototype = rcp( new BraessSarazinSmoother(3,omega) );

  RCP<SmootherFactory>   coarseSolverFact      = rcp( new SmootherFactory(coarseSolverPrototype, Teuchos::null) );

  RCP<FactoryManager> MB = rcp(new FactoryManager());
  MB->SetFactory("A",     SFact);
  MB->SetFactory("Smoother",    SmooSCFact);
  MB->SetIgnoreUserData(true);               // always use data from factories defined in factory manager
  smootherPrototype->SetFactoryManager(MB);
  coarseSolverPrototype->SetFactoryManager(MB);



  // main factory manager
  FactoryManager M;
  M.SetFactory("A",            RebAcFact); // TODO switch back
  M.SetFactory("P",            RebalancedBlockPFact);     // TODO switch back
  M.SetFactory("R",            RebalancedBlockRFact);     // TODO switch back
  M.SetFactory("Smoother",     smootherFact); // TODO fix me
  M.SetFactory("PreSmoother",     smootherFact); // TODO fix me
  M.SetFactory("PostSmoother",     smootherFact); // TODO fix me
  M.SetFactory("CoarseSolver", coarseSolverFact);
  //////////////////////////////////// setup multigrid
#endif


  H->Setup(M,0,maxLevels);

  *out << std::endl;
  *out << "print content of multigrid levels:" << std::endl;

  Finest->print(*out);

  RCP<Level> coarseLevel = H->GetLevel(1);
  coarseLevel->print(*out);

  RCP<Level> coarseLevel2 = H->GetLevel(2);
  coarseLevel2->print(*out);

  RCP<MultiVector> xLsg = MultiVectorFactory::Build(xstridedfullmap,1);

  // Use AMG directly as an iterative method
#if 0
  {
    xLsg->putScalar( (SC) 0.0);

    // Epetra_Vector -> Xpetra::Vector
    RCP<Vector> xRhs = Teuchos::rcp(new Xpetra::EpetraVector(epv));

    // calculate initial (absolute) residual
    Teuchos::Array<ST::magnitudeType> norms(1);
    xRhs->norm2(norms);
    *out << "||x_0|| = " << norms[0] << std::endl;

    // apply ten multigrid iterations
    H->Iterate(*xRhs,100,*xLsg);


    // calculate and print residual
    RCP<MultiVector> xTmp = MultiVectorFactory::Build(xstridedfullmap,1);
    bOp->apply(*xLsg,*xTmp,Teuchos::NO_TRANS,(SC)1.0,(SC)0.0);
    xRhs->update((SC)-1.0,*xTmp,(SC)1.0);
    xRhs->norm2(norms);
    *out << "||x|| = " << norms[0] << std::endl;
  }
#endif

  //
  // Solve Ax = b using AMG as a preconditioner in AztecOO
  //
  {
    RCP<Epetra_Vector> X = rcp(new Epetra_Vector(epv->Map()));
    X->PutScalar(0.0);
    Epetra_LinearProblem epetraProblem(epA.get(), X.get(), epv.get());

    AztecOO aztecSolver(epetraProblem);
    aztecSolver.SetAztecOption(AZ_solver, AZ_gmres);

    MueLu::EpetraOperator aztecPrec(H);
    aztecSolver.SetPrecOperator(&aztecPrec);

    int maxIts = 50;
    double tol = 1e-8;

    aztecSolver.Iterate(maxIts, tol);
  }

   return EXIT_SUCCESS;
}
