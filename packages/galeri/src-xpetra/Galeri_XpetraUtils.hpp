// @HEADER
//
// ***********************************************************************
//
//           Galeri: Finite Element and Matrix Generation Package
//                 Copyright (2006) ETHZ/Sandia Corporation
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
  Direct translation of Galeri coordinate generator.
*/
#ifndef GALERI_XPETRAUTILS_HPP
#define GALERI_XPETRAUTILS_HPP

#include "Teuchos_ParameterList.hpp"
#include "Teuchos_Assert.hpp"

#include "Galeri_VectorTraits.hpp"

#include <iostream>

namespace Galeri {
  namespace Xpetra {
  class Utils {

    public:

    template <typename Scalar, typename LocalOrdinal, typename GlobalOrdinal, typename Map, typename MultiVector>
    static Teuchos::RCP<MultiVector>
    CreateCartesianCoordinates(std::string const &coordType, RCP<const Map> const & map, Teuchos::ParameterList& list)
    {
      using Galeri::Xpetra::VectorTraits;

      Teuchos::RCP<MultiVector> coordinates;

      Scalar delta_x, delta_y, delta_z;

      double one = 1.0;
      Scalar lx = list.get<Scalar>("lx", one) * list.get<Scalar>("stretchx", one);
      Scalar ly = list.get<Scalar>("ly", one) * list.get<Scalar>("stretchy", one);
      Scalar lz = list.get<Scalar>("lz", one) * list.get<Scalar>("stretchz", one);

      GlobalOrdinal nx = list.get<GlobalOrdinal>("nx", -1);
      GlobalOrdinal ny = list.get<GlobalOrdinal>("ny", -1);
      GlobalOrdinal nz = list.get<GlobalOrdinal>("nz", -1);

      GlobalOrdinal ix, iy, iz;

      LocalOrdinal NumMyElements = map->getNodeNumElements();
      Teuchos::ArrayView<const GlobalOrdinal> MyGlobalElements = map->getNodeElementList();

      if (coordType == "1D") {
        coordinates = VectorTraits<Map,MultiVector>::Build(map,1,false);
        Teuchos::ArrayRCP<Teuchos::ArrayRCP<Scalar> > Coord(1);
        Coord[0] = coordinates->getDataNonConst(0);

        delta_x = lx / (nx - 1);

        for (LocalOrdinal i = 0; i < NumMyElements; ++i) {
          ix = MyGlobalElements[i];
          Coord[0][i] = delta_x * ix;
        }

      } else if (coordType == "2D") {

        coordinates = VectorTraits<Map,MultiVector>::Build(map,2,false);
        Teuchos::ArrayRCP<Teuchos::ArrayRCP<Scalar> > Coord(2);
        Coord[0] = coordinates->getDataNonConst(0);
        Coord[1] = coordinates->getDataNonConst(1);

        delta_x = lx / (nx - 1);
        delta_y = ly / (ny - 1);

        for (LocalOrdinal i = 0; i < NumMyElements; ++i)
        {
          ix = MyGlobalElements[i] % nx;
          iy = (MyGlobalElements[i] - ix) / nx;

          Coord[0][i] = delta_x * ix;
          Coord[1][i] = delta_y * iy;
        }

      } else if (coordType == "3D") {

        coordinates = VectorTraits<Map,MultiVector>::Build(map,3,false);
        Teuchos::ArrayRCP<Teuchos::ArrayRCP<Scalar> > Coord(3);
        Coord[0] = coordinates->getDataNonConst(0);
        Coord[1] = coordinates->getDataNonConst(1);
        Coord[2] = coordinates->getDataNonConst(2);

        delta_x = lx / (nx - 1);
        delta_y = ly / (ny - 1);
        delta_z = lz / (nz - 1);

        for (LocalOrdinal i = 0; i < NumMyElements; i++)
        {
          GlobalOrdinal ixy = MyGlobalElements[i] % (nx * ny);
          iz = (MyGlobalElements[i] - ixy) / (nx * ny);

          ix = ixy % nx;
          iy = (ixy - ix) / ny;

          Coord[0][i] = delta_x * ix;
          Coord[1][i] = delta_y * iy;
          Coord[2][i] = delta_z * iz;
        }

      } else {

        throw(std::runtime_error("in Galeri::Xpetra::Utils : `coordType' has incorrect value (" + coordType + ")"));

      } //if (coordType == ...

      return coordinates;

    } // CreateCartesianCoordinates()

    template <typename GlobalOrdinal>
    static void getSubdomainData(GlobalOrdinal N, GlobalOrdinal M, GlobalOrdinal i, GlobalOrdinal& n, GlobalOrdinal& shift) {
      GlobalOrdinal start, end;
      GlobalOrdinal xpid = i % M;

      GlobalOrdinal PerProcSmallXDir = (GlobalOrdinal) (((double) N)/((double) M));
      GlobalOrdinal NBigXDir         = N - PerProcSmallXDir*M;

      if (xpid < NBigXDir) start =                                        xpid*(PerProcSmallXDir+1);
      else                 start = (xpid-NBigXDir)*PerProcSmallXDir + NBigXDir*(PerProcSmallXDir+1);
      end = start + PerProcSmallXDir + ((xpid < NBigXDir) ? 1 : 0);

      shift = start;
      n = end - start;
    }

  }; // class Utils
  } // namespace Xpetra
} // namespace Galeri

#endif //ifndef GALERI_XPETRAUTILS_HPP
