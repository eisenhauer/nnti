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
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef MUELU_MHDRAPFACTORY_DEF_HPP
#define MUELU_MHDRAPFACTORY_DEF_HPP

#include <sstream>

#include <Xpetra_Map.hpp>
#include <Xpetra_MapFactory.hpp>
#include <Xpetra_Matrix.hpp>
#include <Xpetra_CrsMatrixWrap.hpp>
#include <Xpetra_Vector.hpp>
#include <Xpetra_VectorFactory.hpp>

#include "MueLu_MHDRAPFactory_decl.hpp"
#include "MueLu_Utilities.hpp"
#include "MueLu_Monitor.hpp"

namespace MueLu {

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  MHDRAPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::MHDRAPFactory()
    : implicitTranspose_(true) { }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void MHDRAPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::DeclareInput(Level &fineLevel, Level &coarseLevel) const {

    if (implicitTranspose_ == false)
      {
        Input(coarseLevel, "R" );
        Input(coarseLevel, "RV");
        Input(coarseLevel, "RP");
        Input(coarseLevel, "RM");
      }

    Input(fineLevel, "A"  );
    Input(fineLevel, "A00");
    Input(fineLevel, "A01");
    Input(fineLevel, "A02");
    Input(fineLevel, "A10");
    Input(fineLevel, "A11");
    Input(fineLevel, "A12");
    Input(fineLevel, "A20");
    Input(fineLevel, "A21");
    Input(fineLevel, "A22");

    Input(coarseLevel, "P" );
    Input(coarseLevel, "PV");
    Input(coarseLevel, "PP");
    Input(coarseLevel, "PM");

  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void MHDRAPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Build(Level &fineLevel, Level &coarseLevel) const { // FIXME make fineLevel const
    {
      FactoryMonitor m(*this, "Computing Ac", coarseLevel);

      //
      // Inputs: A, P
      //

      //DEBUG
      //Teuchos::FancyOStream fout(Teuchos::rcpFromRef(std::cout));
      //coarseLevel.print(fout,Teuchos::VERB_HIGH);


      RCP<Matrix> A   = Get< RCP<Matrix> >(fineLevel, "A"  );
      RCP<Matrix> A00 = Get< RCP<Matrix> >(fineLevel, "A00");
      RCP<Matrix> A01 = Get< RCP<Matrix> >(fineLevel, "A01");
      RCP<Matrix> A02 = Get< RCP<Matrix> >(fineLevel, "A02");
      RCP<Matrix> A10 = Get< RCP<Matrix> >(fineLevel, "A10");
      RCP<Matrix> A11 = Get< RCP<Matrix> >(fineLevel, "A11");
      RCP<Matrix> A12 = Get< RCP<Matrix> >(fineLevel, "A12");
      RCP<Matrix> A20 = Get< RCP<Matrix> >(fineLevel, "A20");
      RCP<Matrix> A21 = Get< RCP<Matrix> >(fineLevel, "A21");
      RCP<Matrix> A22 = Get< RCP<Matrix> >(fineLevel, "A22");

      RCP<Matrix> P  = Get< RCP<Matrix> >(coarseLevel, "P" );
      RCP<Matrix> PV = Get< RCP<Matrix> >(coarseLevel, "PV");
      RCP<Matrix> PP = Get< RCP<Matrix> >(coarseLevel, "PP");
      RCP<Matrix> PM = Get< RCP<Matrix> >(coarseLevel, "PM");

      //
      // Build Ac = RAP
      //

      RCP<Matrix> AP;
      RCP<Matrix> AP00;
      RCP<Matrix> AP01;
      RCP<Matrix> AP02;
      RCP<Matrix> AP10;
      RCP<Matrix> AP11;
      RCP<Matrix> AP12;
      RCP<Matrix> AP20;
      RCP<Matrix> AP21;
      RCP<Matrix> AP22;

      {
        SubFactoryMonitor subM(*this, "MxM: A x P", coarseLevel);

        AP   = Utils::Multiply(*A  , false, *P , false);
        AP00 = Utils::Multiply(*A00, false, *PV, false);
        AP01 = Utils::Multiply(*A01, false, *PP, false);
        AP02 = Utils::Multiply(*A02, false, *PM, false);
        AP10 = Utils::Multiply(*A10, false, *PV, false);
        AP11 = Utils::Multiply(*A11, false, *PP, false);
        AP12 = Utils::Multiply(*A12, false, *PM, false);
        AP20 = Utils::Multiply(*A20, false, *PV, false);
        AP21 = Utils::Multiply(*A21, false, *PP, false);
        AP22 = Utils::Multiply(*A22, false, *PM, false);
      }

      RCP<Matrix> Ac;
      RCP<Matrix> Ac00;
      RCP<Matrix> Ac01;
      RCP<Matrix> Ac02;
      RCP<Matrix> Ac10;
      RCP<Matrix> Ac11;
      RCP<Matrix> Ac12;
      RCP<Matrix> Ac20;
      RCP<Matrix> Ac21;
      RCP<Matrix> Ac22;

      if (implicitTranspose_)
        {
          SubFactoryMonitor m2(*this, "MxM: P' x (AP) (implicit)", coarseLevel);

          Ac   = Utils::Multiply(*P , true, *AP  , false);
          Ac00 = Utils::Multiply(*PV, true, *AP00, false);
          Ac01 = Utils::Multiply(*PV, true, *AP01, false);
          Ac02 = Utils::Multiply(*PV, true, *AP02, false);
          Ac10 = Utils::Multiply(*PP, true, *AP10, false);
          Ac11 = Utils::Multiply(*PP, true, *AP11, false);
          Ac12 = Utils::Multiply(*PP, true, *AP12, false);
          Ac20 = Utils::Multiply(*PM, true, *AP20, false);
          Ac21 = Utils::Multiply(*PM, true, *AP21, false);
          Ac22 = Utils::Multiply(*PM, true, *AP22, false);

        }
      else
        {

          SubFactoryMonitor m2(*this, "MxM: R x (AP) (explicit)", coarseLevel);

          RCP<Matrix> R  = Get< RCP<Matrix> >(coarseLevel, "R" );
          RCP<Matrix> RV = Get< RCP<Matrix> >(coarseLevel, "RV");
          RCP<Matrix> RP = Get< RCP<Matrix> >(coarseLevel, "RP");
          RCP<Matrix> RM = Get< RCP<Matrix> >(coarseLevel, "RM");

          Ac   = Utils::Multiply(*R , false, *AP  , false);
          Ac00 = Utils::Multiply(*RV, false, *AP00, false);
          Ac01 = Utils::Multiply(*RV, false, *AP01, false);
          Ac02 = Utils::Multiply(*RV, false, *AP02, false);
          Ac10 = Utils::Multiply(*RP, false, *AP10, false);
          Ac11 = Utils::Multiply(*RP, false, *AP11, false);
          Ac12 = Utils::Multiply(*RP, false, *AP12, false);
          Ac20 = Utils::Multiply(*RM, false, *AP20, false);
          Ac21 = Utils::Multiply(*RM, false, *AP21, false);
          Ac22 = Utils::Multiply(*RM, false, *AP22, false);

        }
      // FINISHED MAKING COARSE BLOCKS

      Set(coarseLevel, "A"  , Ac  );
      Set(coarseLevel, "A00", Ac00);
      Set(coarseLevel, "A01", Ac01);
      Set(coarseLevel, "A02", Ac02);
      Set(coarseLevel, "A10", Ac10);
      Set(coarseLevel, "A11", Ac11);
      Set(coarseLevel, "A12", Ac12);
      Set(coarseLevel, "A20", Ac20);
      Set(coarseLevel, "A21", Ac21);
      Set(coarseLevel, "A22", Ac22);


    }


  }


  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  std::string MHDRAPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::PerfUtils::PrintMatrixInfo(const Matrix & Ac, const std::string & msgTag) {
    std::stringstream ss(std::stringstream::out);
    ss << msgTag
       << " # global rows = "      << Ac.getGlobalNumRows()
       << ", estim. global nnz = " << Ac.getGlobalNumEntries()
       << std::endl;
    return ss.str();
  }

  template <class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  std::string MHDRAPFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::PrintLoadBalancingInfo(const Matrix & Ac, const std::string & msgTag) {
    std::stringstream ss(std::stringstream::out);

    // TODO: provide a option to skip this (to avoid global communication)
      // TODO: skip if nproc == 1

    //nonzero imbalance
    size_t numMyNnz  = Ac.getNodeNumEntries();
    GO maxNnz, minNnz;
    RCP<const Teuchos::Comm<int> > comm = Ac.getRowMap()->getComm();
    maxAll(comm,(GO)numMyNnz,maxNnz);
    //min nnz over all proc (disallow any processors with 0 nnz)
    minAll(comm, (GO)((numMyNnz > 0) ? numMyNnz : maxNnz), minNnz);
    double imbalance = ((double) maxNnz) / minNnz;

    size_t numMyRows = Ac.getNodeNumRows();
    //Check whether Ac is spread over more than one process.
    GO numActiveProcesses=0;
    sumAll(comm, (GO)((numMyRows > 0) ? 1 : 0), numActiveProcesses);

    //min, max, and avg # rows per proc
    GO minNumRows, maxNumRows;
    double avgNumRows;
    maxAll(comm, (GO)numMyRows, maxNumRows);
    minAll(comm, (GO)((numMyRows > 0) ? numMyRows : maxNumRows), minNumRows);
    assert(numActiveProcesses > 0);
    avgNumRows = Ac.getGlobalNumRows() / numActiveProcesses;

    ss << msgTag << " # processes with rows = " << numActiveProcesses << std::endl;
    ss << msgTag << " min # rows per proc = " << minNumRows << ", max # rows per proc = " << maxNumRows << ", avg # rows per proc = " << avgNumRows << std::endl;
    ss << msgTag << " nonzero imbalance = " << imbalance << std::endl;

    return ss.str();
  }


} //namespace MueLu

#define MUELU_MHDRAPFACTORY_SHORT
#endif // MUELU_MHDRAPFACTORY_DEF_HPP

