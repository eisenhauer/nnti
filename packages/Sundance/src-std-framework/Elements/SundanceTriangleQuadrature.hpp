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

#ifndef SUNDANCE_TRIANGLEQUADRATURE_H
#define SUNDANCE_TRIANGLEQUADRATURE_H

#include "SundanceDefs.hpp"
#include "Teuchos_Array.hpp"

#ifndef DOXYGEN_DEVELOPER_ONLY

namespace SundanceStdFwk
{
  using namespace Teuchos;
  namespace Internal
  {
    /**
     * Get abscissas and weights for Gaussian quadrature on triangles
     */

    class TriangleQuadrature
    {
    public:
      static void getPoints(int order, Array<double>& wgt,
                            Array<double>& x,
                            Array<double>& y);

      static bool test(int p);


    private:

      static void getNonsymmetricPoints(int order, Array<double>& wgt,
                                        Array<double>& x,
                                        Array<double>& y);

      static bool getSymmetricPoints(int order, Array<double>& wgt,
                                     Array<double>& x,
                                     Array<double>& y);

      static void permute(int m, const Array<double>& q,
                          Array<Array<double> >& qPerm);

      static double exact(int a, int b, int c);

      static double fact(int x);

    };
  }
}

#endif  /* DOXYGEN_DEVELOPER_ONLY */

#endif
