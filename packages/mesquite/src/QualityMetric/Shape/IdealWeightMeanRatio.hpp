/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2004 Sandia Corporation and Argonne National
    Laboratory.  Under the terms of Contract DE-AC04-94AL85000 
    with Sandia Corporation, the U.S. Government retains certain 
    rights in this software.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License 
    (lgpl.txt) along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 
    diachin2@llnl.gov, djmelan@sandia.gov, mbrewer@sandia.gov, 
    pknupp@sandia.gov, tleurent@mcs.anl.gov, tmunson@mcs.anl.gov      
   
  ***************************************************************** */
// -*- Mode : c++; tab-width: 3; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 3 -*-

/*! \file IdealWeightMeanRatio.hpp

Header file for the Mesquite::IdealWeightMeanRatio class

\author Michael Brewer
\author Todd Munson
\date   2002-11-11
 */


#ifndef IdealWeightMeanRatio_hpp
#define IdealWeightMeanRatio_hpp

#include "Mesquite.hpp"
#include "MsqError.hpp"
#include "ShapeQualityMetric.hpp"
#include "Vector3D.hpp"
#include "Matrix3D.hpp"
#include "Exponent.hpp"

namespace Mesquite
{
   /*! \class IdealWeightMeanRatio
     \brief Computes the mean ratio quality metric
     of given element.
     
     The metric does not use the sample point functionality or the
     compute_weighted_jacobian.  It evaluates the metric at
     the element vertices, and uses the isotropic ideal element.
     It does require a feasible region, and the metric needs
     to be maximized.
   */
   class IdealWeightMeanRatio : public ShapeQualityMetric
   {
   public:
 
     IdealWeightMeanRatio()
      : a2Con(2.0), 
        b2Con(-1.0), 
        c2Con(1.0),
        a3Con(3.0),
        b3Con(-1.0),
        c3Con(2.0/3.0)
     {
       set_metric_type(ELEMENT_BASED);
       set_negate_flag(-1);
       set_gradient_type(ANALYTICAL_GRADIENT);
       set_hessian_type(ANALYTICAL_HESSIAN);
       avgMethod=QualityMetric::LINEAR;
       feasible=1;
       set_name("Mean Ratio");
     }
     
     //! virtual destructor ensures use of polymorphism during destruction
     virtual ~IdealWeightMeanRatio() {
     }
     
     //! evaluate using mesquite objects 
     bool evaluate_element(PatchData &pd, MsqMeshEntity *element,
                           double &fval, MsqError &err); 
          
     bool compute_element_analytical_gradient(PatchData &pd,
                                               MsqMeshEntity *element,
                                               MsqVertex *free_vtces[], 
                                               Vector3D grad_vec[],
                                               int num_vtx, 
                                               double &metric_value,
                                               MsqError &err);

     bool compute_element_analytical_hessian(PatchData &pd,
                                              MsqMeshEntity *e,
                                              MsqVertex *v[], 
                                              Vector3D g[],
                                              Matrix3D h[],
                                              int nv, 
                                              double &m,
                                              MsqError &err);

    private:
      // arrays used in Hessian computations 
      // We allocate them here, so that one allocation only is done.
      // This gives a big computation speed increase.
      Vector3D mCoords[4]; // Vertex coordinates for the (decomposed) elements
      Vector3D mGradients[32]; // Gradient of metric with respect to the coords
      Matrix3D mHessians[80]; // Hessian of metric with respect to the coords
      double   mMetrics[8]; // Metric values for the (decomposed) elements
      
      const double a2Con;
      const Exponent b2Con;
      const Exponent c2Con;
      
      const double a3Con;
      const Exponent b3Con;
      const Exponent c3Con;

   };
} //namespace


#endif // IdealWeightMeanRatio_hpp


