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
/*!
  \file   IdealWeightInverseMeanRatio.cpp
  \brief  

  \author Michael Brewer
  \author Todd Munson
  \author Thomas Leurent

  \date   2002-06-9
*/
#include "IdealWeightInverseMeanRatio.hpp"
#include "MeanRatioFunctions.hpp"
#include "MsqTimer.hpp"
#include "MsqDebug.hpp"

#ifdef MSQ_USE_OLD_STD_HEADERS
#  include <vector.h>
#else
#  include <vector>
   using std::vector;
#endif

#include <math.h>

namespace Mesquite {

IdealWeightInverseMeanRatio::IdealWeightInverseMeanRatio( MsqError& err, double pow_dbl )
{
  set_metric_type(ELEMENT_BASED);

  set_gradient_type(ANALYTICAL_GRADIENT);
  set_hessian_type(ANALYTICAL_HESSIAN);
  avgMethod=QualityMetric::LINEAR;
  feasible=1;
  set_name("Inverse Mean Ratio");

  set_metric_power(pow_dbl, err);  MSQ_ERRRTN(err);
}

       //! Sets the power value in the metric computation.
void IdealWeightInverseMeanRatio::set_metric_power(double pow_dbl, MsqError& err)
{
  if(fabs(pow_dbl)<=MSQ_MIN){
    MSQ_SETERR(err)(MsqError::INVALID_ARG);
    return;
  }
  if(pow_dbl<0)
    set_negate_flag(-1);
  else
    set_negate_flag(1);
  a2Con=::pow(.5,pow_dbl);
  b2Con=pow_dbl;
  c2Con=-pow_dbl;
  a3Con=::pow(1.0/3.0,pow_dbl);
  b3Con=pow_dbl;
  c3Con=-2.0*pow_dbl/3.0;
}


bool IdealWeightInverseMeanRatio::evaluate_element(PatchData &pd,
                                              MsqMeshEntity *e,
                                              double &m,
                                              MsqError &err)
{
  EntityTopology topo = e->get_element_type();

  MsqVertex *vertices = pd.get_vertex_array(err);  MSQ_ERRZERO(err);
  const size_t *v_i = e->get_vertex_index_array();

  Vector3D n;			// Surface normal for 2D objects

  // Prism and Hex element descriptions
  static const int locs_pri[6][4] = {{0, 1, 2, 3}, {1, 2, 0, 4},
				     {2, 0, 1, 5}, {3, 5, 4, 0},
				     {4, 3, 5, 1}, {5, 4, 3, 2}};
  static const int locs_hex[8][4] = {{0, 1, 3, 4}, {1, 2, 0, 5},
				     {2, 3, 1, 6}, {3, 0, 2, 7},
				     {4, 7, 5, 0}, {5, 4, 6, 1},
				     {6, 5, 7, 2}, {7, 6, 4, 3}};

  const Vector3D d_con(1.0, 1.0, 1.0);

  int i;

  m = 0.0;
  bool metric_valid = false;
  switch(topo) {
  case TRIANGLE:
    pd.get_domain_normal_at_element(e, n, err); MSQ_ERRZERO(err);
    n = n / n.length();		// Need unit normal
    mCoords[0] = vertices[v_i[0]];
    mCoords[1] = vertices[v_i[1]];
    mCoords[2] = vertices[v_i[2]];
    metric_valid = m_fcn_2e(m, mCoords, n, a2Con, b2Con, c2Con);
    if (!metric_valid) return false;
    break;
    
  case QUADRILATERAL:
    pd.get_domain_normal_at_element(e, n, err); MSQ_ERRZERO(err);
    n = n / n.length();	// Need unit normal
    for (i = 0; i < 4; ++i) {
      mCoords[0] = vertices[v_i[locs_hex[i][0]]];
      mCoords[1] = vertices[v_i[locs_hex[i][1]]];
      mCoords[2] = vertices[v_i[locs_hex[i][2]]];
      metric_valid = m_fcn_2i(mMetrics[i], mCoords, n, 
			      a2Con, b2Con, c2Con, d_con);
      if (!metric_valid) return false;
    }
    m = average_metrics(mMetrics, 4, err);
    break;

  case TETRAHEDRON:
    mCoords[0] = vertices[v_i[0]];
    mCoords[1] = vertices[v_i[1]];
    mCoords[2] = vertices[v_i[2]];
    mCoords[3] = vertices[v_i[3]];
    metric_valid = m_fcn_3e(m, mCoords, a3Con, b3Con, c3Con);
    if (!metric_valid) return false;
    break;

  case PYRAMID:
    for (i = 0; i < 4; ++i) {
      mCoords[0] = vertices[v_i[ i     ]];
      mCoords[1] = vertices[v_i[(i+1)%4]];
      mCoords[2] = vertices[v_i[(i+3)%4]];
      mCoords[3] = vertices[v_i[ 4     ]];
      metric_valid = m_fcn_3p(mMetrics[i], mCoords, a3Con, b3Con, c3Con);
      if (!metric_valid) return false;
    }
    m = average_metrics(mMetrics, 4, err); MSQ_ERRZERO(err);
    break;

  case PRISM:
    for (i = 0; i < 6; ++i) {
      mCoords[0] = vertices[v_i[locs_pri[i][0]]];
      mCoords[1] = vertices[v_i[locs_pri[i][1]]];
      mCoords[2] = vertices[v_i[locs_pri[i][2]]];
      mCoords[3] = vertices[v_i[locs_pri[i][3]]];
      metric_valid = m_fcn_3w(mMetrics[i], mCoords, a3Con, b3Con, c3Con);
      if (!metric_valid) return false;
    }
    m = average_metrics(mMetrics, 6, err); MSQ_ERRZERO(err);
    break;

  case HEXAHEDRON:
    for (i = 0; i < 8; ++i) {
      mCoords[0] = vertices[v_i[locs_hex[i][0]]];
      mCoords[1] = vertices[v_i[locs_hex[i][1]]];
      mCoords[2] = vertices[v_i[locs_hex[i][2]]];
      mCoords[3] = vertices[v_i[locs_hex[i][3]]];
      metric_valid = m_fcn_3i(mMetrics[i], mCoords, 
			      a3Con, b3Con, c3Con, d_con);
      if (!metric_valid) return false;
    }
    m = average_metrics(mMetrics, 8, err); MSQ_ERRZERO(err);
    break;

  default:
    MSQ_SETERR(err)(MsqError::UNSUPPORTED_ELEMENT,
                    "Element type (%d) not supported in IdealWeightInverseMeanRatio",
                    (int)topo);
    return false;
  } // end switch over element type
  return true;
}

bool IdealWeightInverseMeanRatio::compute_element_analytical_gradient(PatchData &pd,
								 MsqMeshEntity *e,
								 MsqVertex *v[], 
								 Vector3D g[],
								 int nv, 
								 double &m,
                         MsqError &err)
{
  EntityTopology topo = e->get_element_type();

  if (((topo == QUADRILATERAL) || (topo == HEXAHEDRON) || 
       (topo == PYRAMID) || (topo == PRISM)) && 
      ((avgMethod == MINIMUM) || (avgMethod == MAXIMUM))) {
    MSQ_DBGOUT(1) <<
      "Minimum and maximum not continuously differentiable.\n"
      "Element of subdifferential will be returned.\n";
  }

  MsqVertex *vertices = pd.get_vertex_array(err);  MSQ_ERRZERO(err);
  const size_t *v_i = e->get_vertex_index_array();

  Vector3D n;			// Surface normal for 2D objects

  // Prism and Hex element descriptions
  static const int locs_pri[6][4] = {{0, 1, 2, 3}, {1, 2, 0, 4},
				     {2, 0, 1, 5}, {3, 5, 4, 0},
				     {4, 3, 5, 1}, {5, 4, 3, 2}};
  static const int locs_hex[8][4] = {{0, 1, 3, 4}, {1, 2, 0, 5},
				     {2, 3, 1, 6}, {3, 0, 2, 7},
				     {4, 7, 5, 0}, {5, 4, 6, 1},
				     {6, 5, 7, 2}, {7, 6, 4, 3}};

  const Vector3D d_con(1.0, 1.0, 1.0);

  int i;

  bool metric_valid = false;
  const uint32_t fm = fixed_vertex_bitmap( pd, e, v, nv );

  m = 0.0;

  switch(topo) {
  case TRIANGLE:
    pd.get_domain_normal_at_element(e, n, err); MSQ_ERRZERO(err);
    n /= n.length();		// Need unit normal
    mCoords[0] = vertices[v_i[0]];
    mCoords[1] = vertices[v_i[1]];
    mCoords[2] = vertices[v_i[2]];
    if (!g_fcn_2e(m, g, mCoords, n, a2Con, b2Con, c2Con)) return false;

    zero_fixed_gradients( TRIANGLE, fm, g );
    break;

  case QUADRILATERAL:
    pd.get_domain_normal_at_element(e, n, err); MSQ_ERRZERO(err);
    n /= n.length();	// Need unit normal
    for (i = 0; i < 4; ++i) {
      mCoords[0] = vertices[v_i[locs_hex[i][0]]];
      mCoords[1] = vertices[v_i[locs_hex[i][1]]];
      mCoords[2] = vertices[v_i[locs_hex[i][2]]];
      if (!g_fcn_2i(mMetrics[i], mGradients+3*i, mCoords, n,
		    a2Con, b2Con, c2Con, d_con)) return false;
    }

    m = average_corner_gradients( QUADRILATERAL, fm, 4,
                                  mMetrics, mGradients,
                                  g, err ); MSQ_ERRZERO(err);
    break;

  case TETRAHEDRON:
    mCoords[0] = vertices[v_i[0]];
    mCoords[1] = vertices[v_i[1]];
    mCoords[2] = vertices[v_i[2]];
    mCoords[3] = vertices[v_i[3]];
    metric_valid = g_fcn_3e(m, mAccumGrad, mCoords, a3Con, b3Con, c3Con);
    if (!metric_valid) return false;

    zero_fixed_gradients( TETRAHEDRON, fm, g );
    break;

  case PYRAMID:
    for (i = 0; i < 4; ++i) {
      mCoords[0] = vertices[v_i[ i     ]];
      mCoords[1] = vertices[v_i[(i+1)%4]];
      mCoords[2] = vertices[v_i[(i+3)%4]];
      mCoords[3] = vertices[v_i[ 4     ]];
      metric_valid = g_fcn_3p(mMetrics[i], mGradients+4*i, mCoords, a3Con, b3Con, c3Con);
      if (!metric_valid) return false;
    }

    m = average_corner_gradients( PYRAMID, fm, 4,
                                  mMetrics, mGradients,
                                  g, err ); MSQ_ERRZERO(err);
    break;

  case PRISM:
    for (i = 0; i < 6; ++i) {
      mCoords[0] = vertices[v_i[locs_pri[i][0]]];
      mCoords[1] = vertices[v_i[locs_pri[i][1]]];
      mCoords[2] = vertices[v_i[locs_pri[i][2]]];
      mCoords[3] = vertices[v_i[locs_pri[i][3]]];
      if (!g_fcn_3w(mMetrics[i], mGradients+4*i, mCoords, 
		    a3Con, b3Con, c3Con)) return false;
    }
    
    m = average_corner_gradients( PRISM, fm, 6,
                                  mMetrics, mGradients,
                                  g, err ); MSQ_ERRZERO(err);
    break;

  case HEXAHEDRON:
    for (i = 0; i < 8; ++i) {
      mCoords[0] = vertices[v_i[locs_hex[i][0]]];
      mCoords[1] = vertices[v_i[locs_hex[i][1]]];
      mCoords[2] = vertices[v_i[locs_hex[i][2]]];
      mCoords[3] = vertices[v_i[locs_hex[i][3]]];
      if (!g_fcn_3i(mMetrics[i], mGradients+4*i, mCoords, 
		    a3Con, b3Con, c3Con, d_con)) return false;
    }

    m = average_corner_gradients( HEXAHEDRON, fm, 8,
                                  mMetrics, mGradients,
                                  g, err ); MSQ_ERRZERO(err);
    break;

  default:
    MSQ_SETERR(err)(MsqError::UNSUPPORTED_ELEMENT,
                    "Element type (%d) not supported in IdealWeightInverseMeanRatio",
                    (int)topo);
    return false;
  } // end switch over element type

  return true;
}


bool IdealWeightInverseMeanRatio::compute_element_analytical_hessian(PatchData &pd,
								MsqMeshEntity *e,
								MsqVertex *fv[], 
								Vector3D g[],
								Matrix3D h[],
                int nfv, 
								double &m,
								MsqError &err)
{
  EntityTopology topo = e->get_element_type();

  if (((topo == QUADRILATERAL) || (topo == HEXAHEDRON) || 
       (topo == PYRAMID) || (topo == PRISM)) && 
      ((avgMethod == MINIMUM) || (avgMethod == MAXIMUM))) {
    MSQ_DBGOUT(1) <<
      "Minimum and maximum not continuously differentiable.\n"
      "Element of subdifferential will be returned.\n"
      "Who knows what the Hessian is?\n" ;
  }

  MsqVertex *vertices = pd.get_vertex_array(err);  MSQ_ERRZERO(err);
  const size_t *v_i = e->get_vertex_index_array();


  Vector3D n;			// Surface normal for 2D objects

  // Prism and Hex element descriptions
  static const int locs_pri[6][4] = {{0, 1, 2, 3}, {1, 2, 0, 4},
				     {2, 0, 1, 5}, {3, 5, 4, 0},
				     {4, 3, 5, 1}, {5, 4, 3, 2}};
  static const int locs_hex[8][4] = {{0, 1, 3, 4}, {1, 2, 0, 5},
				     {2, 3, 1, 6}, {3, 0, 2, 7},
				     {4, 7, 5, 0}, {5, 4, 6, 1},
				     {6, 5, 7, 2}, {7, 6, 4, 3}};

  const Vector3D d_con(1.0, 1.0, 1.0);

  int i;

  bool metric_valid = false;
  const uint32_t fm = fixed_vertex_bitmap( pd, e, fv, nfv );

  m = 0.0;

  switch(topo) {
  case TRIANGLE:
    pd.get_domain_normal_at_element(e, n, err); MSQ_ERRZERO(err);
    n /= n.length();		// Need unit normal
    mCoords[0] = vertices[v_i[0]];
    mCoords[1] = vertices[v_i[1]];
    mCoords[2] = vertices[v_i[2]];
    if (!h_fcn_2e(m, g, h, mCoords, n, a2Con, b2Con, c2Con)) return false;

    zero_fixed_gradients( TRIANGLE, fm, g );
    zero_fixed_hessians( TRIANGLE, fm, h );
    break;

  case QUADRILATERAL:
    pd.get_domain_normal_at_element(e, n, err); MSQ_ERRZERO(err);
    n /= n.length();	// Need unit normal
    for (i = 0; i < 4; ++i) {
      mCoords[0] = vertices[v_i[locs_hex[i][0]]];
      mCoords[1] = vertices[v_i[locs_hex[i][1]]];
      mCoords[2] = vertices[v_i[locs_hex[i][2]]];
      if (!h_fcn_2i(mMetrics[i], mGradients+3*i, mHessians+6*i, mCoords, n,
		    a2Con, b2Con, c2Con, d_con)) return false;
    }

    m = average_corner_hessians( QUADRILATERAL, fm, 4,
                                 mMetrics, mGradients, mHessians,
                                 g, h, err );
    MSQ_ERRZERO( err );
    break;

  case TETRAHEDRON:
    mCoords[0] = vertices[v_i[0]];
    mCoords[1] = vertices[v_i[1]];
    mCoords[2] = vertices[v_i[2]];
    mCoords[3] = vertices[v_i[3]];
    metric_valid = h_fcn_3e(m, g, h, mCoords, a3Con, b3Con, c3Con);
    if (!metric_valid) return false;

    zero_fixed_gradients( TETRAHEDRON, fm, g );
    zero_fixed_hessians( TETRAHEDRON, fm, h );
    break;

  case PYRAMID:
    for (i = 0; i < 4; ++i) {
      mCoords[0] = vertices[v_i[ i     ]];
      mCoords[1] = vertices[v_i[(i+1)%4]];
      mCoords[2] = vertices[v_i[(i+3)%4]];
      mCoords[3] = vertices[v_i[ 4     ]];
      metric_valid = h_fcn_3p(mMetrics[i], mGradients+4*i, 
                              mHessians+10*i, mCoords, a3Con, b3Con, c3Con);

      if (!metric_valid) return false;
    }

    m = average_corner_hessians( PYRAMID, fm, 4,
                                 mMetrics, mGradients, mHessians,
                                 g, h, err );
    MSQ_ERRZERO( err );
    break;

  case PRISM:
    for (i = 0; i < 6; ++i) {
      mCoords[0] = vertices[v_i[locs_pri[i][0]]];
      mCoords[1] = vertices[v_i[locs_pri[i][1]]];
      mCoords[2] = vertices[v_i[locs_pri[i][2]]];
      mCoords[3] = vertices[v_i[locs_pri[i][3]]];
      if (!h_fcn_3w(mMetrics[i], mGradients+4*i, mHessians+10*i, mCoords,
		    a3Con, b3Con, c3Con)) return false;
    }

    m = average_corner_hessians( PRISM, fm, 6,
                                 mMetrics, mGradients, mHessians,
                                 g, h, err );
    MSQ_ERRZERO( err );
    break;

  case HEXAHEDRON:
    for (i = 0; i < 8; ++i) {
      mCoords[0] = vertices[v_i[locs_hex[i][0]]];
      mCoords[1] = vertices[v_i[locs_hex[i][1]]];
      mCoords[2] = vertices[v_i[locs_hex[i][2]]];
      mCoords[3] = vertices[v_i[locs_hex[i][3]]];
      if (!h_fcn_3i(mMetrics[i], mGradients+4*i, mHessians+10*i, mCoords,
		    a3Con, b3Con, c3Con, d_con)) return false;
    }

    m = average_corner_hessians( HEXAHEDRON, fm, 8,
                                 mMetrics, mGradients, mHessians,
                                 g, h, err );
    MSQ_ERRZERO( err );
    break;

  default:
    MSQ_SETERR(err)(MsqError::UNSUPPORTED_ELEMENT,
                    "Element type (%d) not supported in IdealWeightInverseMeanRatio",
                    (int)topo);
    return false;
  } // end switch over element type

  return true;
}

} // namespace Mesquite
