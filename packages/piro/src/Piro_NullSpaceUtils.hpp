// @HEADER
// ************************************************************************
//
//        Piro: Strategy package for embedded analysis capabilitites
//                  Copyright (2013) Sandia Corporation
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
// Questions? Contact Glen Hansen (gahanse@sandia.gov), Sandia
// National Laboratories.
//
// ************************************************************************
// @HEADER

#ifndef PIRO_NULLSPACEUTILS_HPP
#define PIRO_NULLSPACEUTILS_HPP

#include "Teuchos_ParameterList.hpp"
#include "Teuchos_Array.hpp"
#include "Teuchos_RCP.hpp"

namespace Piro {

   // Convenience function to extract Strat PL from Piro PL

   Teuchos::RCP<Teuchos::ParameterList>
     extractStratimikosParams(const Teuchos::RCP<Teuchos::ParameterList> &piroParams);

class MLRigidBodyModes {

public:

   //! Construct RBM object
   MLRigidBodyModes(int numPDEs);

   //! Update the number of PDEs present
   void setNumPDEs(int numPDEs_){ numPDEs = numPDEs_; }

   //! Resize object as mesh changes
   void resize(const int numSpaceDim, const int numNodes);

   //! Set sizes of nullspace etc
   void setParameters(const int numPDEs, const int numElasticityDim, 
          const int numScalar, const int nullSpaceDim);

   //! Set Piro solver parameter list
   void setPiroPL(const Teuchos::RCP<Teuchos::ParameterList>& piroParams);

   //! Access the arrays to store the coordinates
   void getCoordArrays(double **x, double **y, double **z);

   //! Is ML used on this problem?
   bool isMLUsed(){ return mlUsed; }

   //! Pass coordinate arrays to ML
   void informML();

private:

    void Piro_ML_Coord2RBM(int Nnodes, double x[], double y[], double z[], double rbm[], int Ndof, int NscalarDof, int NSdim);

    int numPDEs;
    int numElasticityDim;
    int numScalar;
    int nullSpaceDim;
    int numSpaceDim;
    bool mlUsed;
    Teuchos::RCP<Teuchos::ParameterList> mlList;

    std::vector<double> x;
    std::vector<double> y;
    std::vector<double> z;
    std::vector<double> rr;

};

} // namespace Piro

#endif /* PIRO_NULLSPACEUTILS_HPP */
