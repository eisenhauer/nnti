/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2006 Sandia National Laboratories.  Developed at the
    University of Wisconsin--Madison under SNL contract number
    624796.  The U.S. Government and the University of Wisconsin
    retain certain rights to this software.

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
 
    (2006) kraftche@cae.wisc.edu
   
  ***************************************************************** */


/** \file TetLagrangeShape.cpp
 *  \brief 
 *  \author Jason Kraftcheck 
 */

#include "Mesquite.hpp"
#include "TetLagrangeShape.hpp"
#include "MsqError.hpp"
#include <assert.h>

namespace Mesquite {

EntityTopology TetLagrangeShape::element_topology() const
  { return TETRAHEDRON; }

static inline int have_node( unsigned nodebits, unsigned node )
  { return nodebits & (1 << (node-4)); }

void TetLagrangeShape::coefficients_at_corner( unsigned corner, 
                                               unsigned nodebits,
                                               double* coeff_out,
                                               size_t& num_coeff,
                                               MsqError& err ) const
{
  if (nodebits >= (1u << 6)) {
    MSQ_SETERR(err)("TetLagrangeShape does not support mid-face/mid-element nodes",
                    MsqError::UNSUPPORTED_ELEMENT);
    return;
  }
  
  num_coeff = 10;
  std::fill( coeff_out, coeff_out+num_coeff, 0.0 );
  coeff_out[corner] = 1.0;
}

void TetLagrangeShape::coefficients_at_mid_edge( unsigned edge, 
                               unsigned nodebits,
                               double* coeff_out,
                               size_t& num_coeff,
                               MsqError& err ) const
{
  if (nodebits >= (1u << 6)) {
    MSQ_SETERR(err)("TetLagrangeShape does not support mid-face/mid-element nodes",
                    MsqError::UNSUPPORTED_ELEMENT);
    return;
  }
  
  num_coeff = 10;
  std::fill( coeff_out, coeff_out+num_coeff, 0.0 );
  if (nodebits & (1 << edge)) { // if mid-edge node is present
    coeff_out[4+edge] = 1.0;
  }
  else { // no mid node on edge
    if (edge < 3) {
      coeff_out[edge] = 0.5;
      coeff_out[(edge+1) % 3] = 0.5;
    }
    else {
      coeff_out[edge-3]= 0.5;
      coeff_out[3] = 0.5;
    }
  }
}

void TetLagrangeShape::coefficients_at_mid_face( unsigned face, 
                               unsigned nodebits,
                               double* coeff_out,
                               size_t& num_coeff,
                               MsqError& err ) const
{
  if (nodebits >= (1u << 6)) {
    MSQ_SETERR(err)("TetLagrangeShape does not support mid-face/mid-element nodes",
                    MsqError::UNSUPPORTED_ELEMENT);
    return;
  }
  
  num_coeff = 10;
  
  const double one_ninth = 1.0/9.0;
  const double two_ninth = 2.0/9.0;
  const double four_ninth = 4.0/9.0;
  
  if (face < 3) {
    const int next = (face+1)%3;
    const int othr = (face+2)%3;
    coeff_out[face] = -one_ninth;
    coeff_out[next] = -one_ninth;
    coeff_out[othr] = 0.0;
    coeff_out[3]    = -one_ninth;
    if (nodebits & (1<<face)) {
      coeff_out[4+face] = four_ninth;
    }
    else {
      coeff_out[face] += two_ninth;
      coeff_out[next] += two_ninth;
      coeff_out[4+face] = 0.0;
    }
    if (nodebits & (1<<(3+next))) {
      coeff_out[7+next] = four_ninth;
    }
    else {
      coeff_out[face] += two_ninth;
      coeff_out[3]    += two_ninth;
      coeff_out[7+next] = 0.0;
    }
    if (nodebits & (1<<(3+face))) {
      coeff_out[7+face] = four_ninth;
    }
    else {
      coeff_out[next] += two_ninth;
      coeff_out[3]    += two_ninth;
      coeff_out[7+face] = 0.0;
    }
    coeff_out[next+4] = 0.0;
    coeff_out[othr+4] = 0.0;
    coeff_out[othr+7] = 0.0;
  }
  else {
    assert( face == 3);
    coeff_out[0] = -one_ninth;
    coeff_out[1] = -one_ninth;
    coeff_out[2] = -one_ninth;
    coeff_out[3] = 0.0;
    if (nodebits & 1) {
      coeff_out[4] = four_ninth;
    }
    else {
      coeff_out[0] += two_ninth;
      coeff_out[1] += two_ninth;
      coeff_out[4] = 0.0;
    }
    if (nodebits & 2) {
      coeff_out[5] = four_ninth;
    }
    else {
      coeff_out[1] += two_ninth;
      coeff_out[2] += two_ninth;
      coeff_out[5] = 0.0;
    }
    if (nodebits & 4) {
      coeff_out[6] = four_ninth;
    }
    else {
      coeff_out[2] += two_ninth;
      coeff_out[0] += two_ninth;
      coeff_out[6] = 0.0;
    }
    coeff_out[7] = 0.0;
    coeff_out[8] = 0.0;
    coeff_out[9] = 0.0;
  }
}

void TetLagrangeShape::coefficients_at_mid_elem( unsigned nodebits,
                                                 double* coeff_out,
                                                 size_t& num_coeff,
                                                 MsqError& err ) const
{
  if (nodebits >= (1u << 6)) {
    MSQ_SETERR(err)("TetLagrangeShape does not support mid-face/mid-element nodes",
                    MsqError::UNSUPPORTED_ELEMENT);
    return;
  }
  
  num_coeff = 10;
  coeff_out[0] = -0.125;
  coeff_out[1] = -0.125;
  coeff_out[2] = -0.125;
  coeff_out[3] = -0.125;
  if (have_node(nodebits, 4)) {
    coeff_out[4] = 0.25;
  }
  else {
    coeff_out[4] = 0.0;
    coeff_out[0] += 0.125;
    coeff_out[1] += 0.125;
  }
  if (have_node(nodebits, 5)) {
    coeff_out[5] = 0.25;
  }
  else {
    coeff_out[5] = 0.0;
    coeff_out[1] += 0.125;
    coeff_out[2] += 0.125;
  }
  if (have_node(nodebits, 6)) {
    coeff_out[6] = 0.25;
  }
  else {
    coeff_out[6] = 0.0;
    coeff_out[2] += 0.125;
    coeff_out[0] += 0.125;
  }
  if (have_node(nodebits, 7)) {
    coeff_out[7] = 0.25;
  }
  else {
    coeff_out[7] = 0.0;
    coeff_out[0] += 0.125;
    coeff_out[3] += 0.125;
  }
  if (have_node(nodebits, 8)) {
    coeff_out[8] = 0.25;
  }
  else {
    coeff_out[8] = 0.0;
    coeff_out[1] += 0.125;
    coeff_out[3] += 0.125;
  }
  if (have_node(nodebits, 9)) {
    coeff_out[9] = 0.25;
  }
  else {
    coeff_out[9] = 0.0;
    coeff_out[2] += 0.125;
    coeff_out[3] += 0.125;
  }
}

static void get_linear_derivatives( size_t* vertices,
                                    double* derivs )
{
  size_t* v = vertices;
  double* d = derivs;
  
  *v = 0; ++v;
  *d = 1.0; ++d;
  *d = 0.0; ++d;
  *d = 0.0; ++d;
  
  *v = 1; ++v;
  *d = 0.0; ++d;
  *d = 1.0; ++d;
  *d = 0.0; ++d;
  
  *v = 2; ++v;
  *d = 0.0; ++d;
  *d = 0.0; ++d;
  *d = 1.0; ++d;
  
  *v = 3; ++v;
  *d =-1.0; ++d;
  *d =-1.0; ++d;
  *d =-1.0; ++d;
}

static const unsigned edges[][2] = { { 0, 1 },
                                     { 1, 2 },
                                     { 2, 0 },
                                     { 0, 3 },
                                     { 1, 3 },
                                     { 2, 3 } };

void TetLagrangeShape::derivatives_at_corner( unsigned corner,
                                              unsigned nodebits,
                                              size_t* vertices,
                                              double* derivs,
                                              size_t& num_vtx,
                                              MsqError& err) const
{
  if (nodebits >= (1u << 6)) {
    MSQ_SETERR(err)("TetLagrangeShape does not support mid-face/mid-element nodes",
                    MsqError::UNSUPPORTED_ELEMENT);
    return;
  }
  
    // begin with derivatives for linear tetrahedron
  num_vtx = 4;
  get_linear_derivatives( vertices, derivs );
  
    // adjust for the presence of mid-edge nodes
  switch (corner) {
    case 0:
      if (have_node(nodebits, 4)) {
        vertices[num_vtx] = 4;
        derivs[3*num_vtx  ] = 0.0;
        derivs[3*num_vtx+1] = 4.0;
        derivs[3*num_vtx+2] = 0.0;
        derivs[1] -= 2.0;
        derivs[4] -= 2.0;
        ++num_vtx;
      }
      if (have_node(nodebits, 6)) {
        vertices[num_vtx] = 6;
        derivs[3*num_vtx  ] = 0.0;
        derivs[3*num_vtx+1] = 0.0;
        derivs[3*num_vtx+2] = 4.0;
        derivs[2] -= 2.0;
        derivs[8] -= 2.0;
        ++num_vtx;
      }
      if (have_node(nodebits, 7)) {
        vertices[num_vtx] = 7;
        derivs[3*num_vtx  ] = -4.0;
        derivs[3*num_vtx+1] = -4.0;
        derivs[3*num_vtx+2] = -4.0;
        derivs[ 0] += 2.0;
        derivs[ 1] += 2.0;
        derivs[ 2] += 2.0;
        derivs[ 9] += 2.0;
        derivs[10] += 2.0;
        derivs[11] += 2.0;
        ++num_vtx;
      }
      break;

    case 1:
      if (have_node(nodebits, 4)) {
        vertices[num_vtx] = 4;
        derivs[3*num_vtx  ] = 4.0;
        derivs[3*num_vtx+1] = 0.0;
        derivs[3*num_vtx+2] = 0.0;
        derivs[ 0] -= 2.0;
        derivs[ 3] -= 2.0;
        ++num_vtx;
      }
      if (have_node(nodebits, 5)) {
        vertices[num_vtx] = 5;
        derivs[3*num_vtx  ] = 0.0;
        derivs[3*num_vtx+1] = 0.0;
        derivs[3*num_vtx+2] = 4.0;
        derivs[ 5] -= 2.0;
        derivs[ 8] -= 2.0;
        ++num_vtx;
      }
      if (have_node(nodebits, 8)) {
        vertices[num_vtx] = 8;
        derivs[3*num_vtx  ] = -4.0;
        derivs[3*num_vtx+1] = -4.0;
        derivs[3*num_vtx+2] = -4.0;
        derivs[ 3] += 2.0;
        derivs[ 4] += 2.0;
        derivs[ 5] += 2.0;
        derivs[ 9] += 2.0;
        derivs[10] += 2.0;
        derivs[11] += 2.0;
        ++num_vtx;
      }
      break;
  
    case 2:
      if (have_node(nodebits, 5)) {
        vertices[num_vtx] = 5;
        derivs[3*num_vtx  ] = 0.0;
        derivs[3*num_vtx+1] = 4.0;
        derivs[3*num_vtx+2] = 0.0;
        derivs[ 4] -= 2.0;
        derivs[ 7] -= 2.0;
        ++num_vtx;
      }
      if (have_node(nodebits, 6)) {
        vertices[num_vtx] = 6;
        derivs[3*num_vtx  ] = 4.0;
        derivs[3*num_vtx+1] = 0.0;
        derivs[3*num_vtx+2] = 0.0;
        derivs[ 0] -= 2.0;
        derivs[ 6] -= 2.0;
        ++num_vtx;
      }
      if (have_node(nodebits, 9)) {
        vertices[num_vtx] = 9;
        derivs[3*num_vtx  ] = -4.0;
        derivs[3*num_vtx+1] = -4.0;
        derivs[3*num_vtx+2] = -4.0;
        derivs[ 6] += 2.0;
        derivs[ 7] += 2.0;
        derivs[ 8] += 2.0;
        derivs[ 9] += 2.0;
        derivs[10] += 2.0;
        derivs[11] += 2.0;
        ++num_vtx;
      }
      break;
  
    case 3:
      if (have_node(nodebits, 7)) {
        vertices[num_vtx] = 7;
        derivs[3*num_vtx  ] = 4.0;
        derivs[3*num_vtx+1] = 0.0;
        derivs[3*num_vtx+2] = 0.0;
        derivs[ 0] -= 2.0;
        derivs[ 9] -= 2.0;
        ++num_vtx;
      }
      if (have_node(nodebits, 8)) {
        vertices[num_vtx] = 8;
        derivs[3*num_vtx  ] = 0.0;
        derivs[3*num_vtx+1] = 4.0;
        derivs[3*num_vtx+2] = 0.0;
        derivs[ 4] -= 2.0;
        derivs[10] -= 2.0;
        ++num_vtx;
      }
      
      if (have_node(nodebits, 9)) {
        vertices[num_vtx] = 9;
        derivs[3*num_vtx  ] = 0.0;
        derivs[3*num_vtx+1] = 0.0;
        derivs[3*num_vtx+2] = 4.0;
        derivs[ 8]-= 2.0;
        derivs[11]-= 2.0;
        ++num_vtx;
      }
      break;
  }
}
  
void TetLagrangeShape::derivatives_at_mid_edge( unsigned edge,
                                              unsigned nodebits,
                                              size_t* vertices,
                                              double* derivs,
                                              size_t& num_vtx,
                                              MsqError& err) const
{
  if (nodebits >= (1u << 6)) {
    MSQ_SETERR(err)("TetLagrangeShape does not support mid-face/mid-element nodes",
                    MsqError::UNSUPPORTED_ELEMENT);
    return;
  }
  
  switch (edge) {
    case 0:
      num_vtx = 2;
      vertices[0] = 0;
      vertices[1] = 1;
      
      derivs[0] = 1.0;
      derivs[1] = 0.0;
      derivs[2] = 0.0;
      
      derivs[3] = 0.0;
      derivs[4] = 1.0;
      derivs[5] = 0.0;
      
   
      if (have_node(nodebits,5) && have_node(nodebits,6)) {
        vertices[num_vtx] = 2;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] = -1.0;
        ++num_vtx;
      }
      else if (!have_node(nodebits,5) && !have_node(nodebits,6)) {\
        vertices[num_vtx] = 2;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  1.0;
        ++num_vtx;
      }
      
      if (!have_node(nodebits, 7) && !have_node(nodebits, 8)) {
        vertices[num_vtx] = 3;
        derivs[3*num_vtx  ] = -1.0;
        derivs[3*num_vtx+1] = -1.0;
        derivs[3*num_vtx+2] = -1.0;
        ++num_vtx;
      }
      else if (have_node(nodebits, 7) && have_node(nodebits, 8)) {
        vertices[num_vtx] = 3;
        derivs[3*num_vtx  ] = 1.0;
        derivs[3*num_vtx+1] = 1.0;
        derivs[3*num_vtx+2] = 1.0;
        ++num_vtx;
      }
      
      if (have_node(nodebits, 4)) {
        vertices[num_vtx] = 4;
        derivs[3*num_vtx  ] = 2.0;
        derivs[3*num_vtx+1] = 2.0;
        derivs[3*num_vtx+2] = 0.0;
        derivs[0] -= 1.0;
        derivs[1] -= 1.0;
        derivs[3] -= 1.0;
        derivs[4] -= 1.0;
         ++num_vtx;
     }
      
      if (have_node(nodebits, 5)) {
        vertices[num_vtx] = 5;
        derivs[3*num_vtx  ] = 0.0;
        derivs[3*num_vtx+1] = 0.0;
        derivs[3*num_vtx+2] = 2.0;
        derivs[ 5] -= 1.0;
        ++num_vtx;
      }
      
      if (have_node(nodebits, 6)) {
        vertices[num_vtx] = 6;
        derivs[3*num_vtx  ] = 0.0;
        derivs[3*num_vtx+1] = 0.0;
        derivs[3*num_vtx+2] = 2.0;
        derivs[ 2] -= 1.0;
        ++num_vtx;
      }
      
      if (have_node(nodebits, 7)) {
        vertices[num_vtx] = 7;
        derivs[3*num_vtx  ] = -2.0;
        derivs[3*num_vtx+1] = -2.0;
        derivs[3*num_vtx+2] = -2.0;
        derivs[0] += 1.0;
        derivs[1] += 1.0;
        derivs[2] += 1.0;
        ++num_vtx;
      }
      
      if (have_node(nodebits, 8)) {
        vertices[num_vtx] = 8;
        derivs[3*num_vtx  ] = -2.0;
        derivs[3*num_vtx+1] = -2.0;
        derivs[3*num_vtx+2] = -2.0;
        derivs[3] += 1.0;
        derivs[4] += 1.0;
        derivs[5] += 1.0;
        ++num_vtx;
      }
      break;
    
    case 1:
      num_vtx = 2;
      vertices[0] = 1;
      vertices[1] = 2;
      
      derivs[0] = 0.0;
      derivs[1] = 1.0;
      derivs[2] = 0.0;
      
      derivs[3] = 0.0;
      derivs[4] = 0.0;
      derivs[5] = 1.0;
   
      if (have_node(nodebits,4) && have_node(nodebits,6)) {
        vertices[num_vtx] = 0;
        derivs[3*num_vtx  ] = -1.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      else if (!have_node(nodebits,4) && !have_node(nodebits,6)) {
        vertices[num_vtx] = 0;
        derivs[3*num_vtx  ] =  1.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      
      if (!have_node(nodebits, 8) && !have_node(nodebits, 9)) {
        vertices[num_vtx] = 3;
        derivs[3*num_vtx  ] = -1.0;
        derivs[3*num_vtx+1] = -1.0;
        derivs[3*num_vtx+2] = -1.0;
        ++num_vtx;
      }
      else if (have_node(nodebits, 8) && have_node(nodebits, 9)) {
        vertices[num_vtx] = 3;
        derivs[3*num_vtx  ] = 1.0;
        derivs[3*num_vtx+1] = 1.0;
        derivs[3*num_vtx+2] = 1.0;
        ++num_vtx;
      }
      
      if (have_node(nodebits, 4)) {
        vertices[num_vtx] = 4;
        derivs[3*num_vtx  ] = 2.0;
        derivs[3*num_vtx+1] = 0.0;
        derivs[3*num_vtx+2] = 0.0;
        ++num_vtx;
        derivs[0] -= 1.0;
      }
      
      if (have_node(nodebits, 5)) {
        vertices[num_vtx] = 5;
        derivs[3*num_vtx  ] = 0.0;
        derivs[3*num_vtx+1] = 2.0;
        derivs[3*num_vtx+2] = 2.0;
        ++num_vtx;
        derivs[1] -= 1.0;
        derivs[2] -= 1.0;
        derivs[4] -= 1.0;
        derivs[5] -= 1.0;
      }
      
      if (have_node(nodebits, 6)) {
        vertices[num_vtx] = 6;
        derivs[3*num_vtx  ] = 2.0;
        derivs[3*num_vtx+1] = 0.0;
        derivs[3*num_vtx+2] = 0.0;
        ++num_vtx;
        derivs[3] -= 1.0;
      }
      
      if (have_node(nodebits, 8)) {
        vertices[num_vtx] = 8;
        derivs[3*num_vtx  ] = -2.0;
        derivs[3*num_vtx+1] = -2.0;
        derivs[3*num_vtx+2] = -2.0;
        ++num_vtx;
        derivs[0] += 1.0;
        derivs[1] += 1.0;
        derivs[2] += 1.0;
      }
      
      if (have_node(nodebits, 9)) {
        vertices[num_vtx] = 9;
        derivs[3*num_vtx  ] = -2.0;
        derivs[3*num_vtx+1] = -2.0;
        derivs[3*num_vtx+2] = -2.0;
        ++num_vtx;
        derivs[3] += 1.0;
        derivs[4] += 1.0;
        derivs[5] += 1.0;
      }
      break;
      
    case 2:
      num_vtx = 2;
      vertices[0] = 0;
      vertices[1] = 2;
      
      derivs[0] = 1.0;
      derivs[1] = 0.0;
      derivs[2] = 0.0;
      
      derivs[3] = 0.0;
      derivs[4] = 0.0;
      derivs[5] = 1.0;
   
      if (have_node(nodebits,4) && have_node(nodebits,5)) {
        vertices[num_vtx] = 1;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] = -1.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      else if (!have_node(nodebits,4) && !have_node(nodebits,5)) {
        vertices[num_vtx] = 1;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  1.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      
      if (!have_node(nodebits, 7) && !have_node(nodebits, 9)) {
        vertices[num_vtx] = 3;
        derivs[3*num_vtx  ] = -1.0;
        derivs[3*num_vtx+1] = -1.0;
        derivs[3*num_vtx+2] = -1.0;
        ++num_vtx;
      }
      else if (have_node(nodebits, 7) && have_node(nodebits, 9)) {
        vertices[num_vtx] = 3;
        derivs[3*num_vtx  ] = 1.0;
        derivs[3*num_vtx+1] = 1.0;
        derivs[3*num_vtx+2] = 1.0;
        ++num_vtx;
      }
      
      if (have_node(nodebits, 4)) {
        vertices[num_vtx] = 4;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  2.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[ 1] -= 1.0;
      }
      
      if (have_node(nodebits, 5)) {
        vertices[num_vtx] = 5;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  2.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[ 4] -= 1.0;
      }
      
      if (have_node(nodebits, 6)) {
        vertices[num_vtx] = 6;
        derivs[3*num_vtx  ] =  2.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  2.0;
        ++num_vtx;
        derivs[0] -= 1.0;
        derivs[2] -= 1.0;
        derivs[3] -= 1.0;
        derivs[5] -= 1.0;
      }
       
      if (have_node(nodebits, 7)) {
        vertices[num_vtx] = 7;
        derivs[3*num_vtx  ] = -2.0;
        derivs[3*num_vtx+1] = -2.0;
        derivs[3*num_vtx+2] = -2.0;
        ++num_vtx;
        derivs[0] += 1.0;
        derivs[1] += 1.0;
        derivs[2] += 1.0;
      }
      
      if (have_node(nodebits, 9)) {
        vertices[num_vtx] = 9;
        derivs[3*num_vtx  ] = -2.0;
        derivs[3*num_vtx+1] = -2.0;
        derivs[3*num_vtx+2] = -2.0;
        ++num_vtx;
        derivs[3] += 1.0;
        derivs[4] += 1.0;
        derivs[5] += 1.0;
      }
      break;
    
    case 3:
      num_vtx = 2;
      vertices[0] = 0;
      vertices[1] = 3;
      
      derivs[0] = 1.0;
      derivs[1] = 0.0;
      derivs[2] = 0.0;
      
      derivs[3] = -1.0;
      derivs[4] = -1.0;
      derivs[5] = -1.0;
      
      if (!have_node( nodebits, 4 ) && !have_node( nodebits, 8 )) {
        vertices[num_vtx] = 1;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  1.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      else if (have_node( nodebits, 4 ) && have_node( nodebits, 8 )) {
        vertices[num_vtx] = 1;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] = -1.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      
      if (!have_node( nodebits, 6 ) && !have_node( nodebits, 9 )) {
        vertices[num_vtx] = 2;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  1.0;
        ++num_vtx;
      }
      if (have_node( nodebits, 6 ) && have_node( nodebits, 9 )) {
        vertices[num_vtx] = 2;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] = -1.0;
        ++num_vtx;
      }

      if (have_node( nodebits, 4 )) {
        vertices[num_vtx] = 4;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  2.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[1] -= 1.0;
      }
      
      if (have_node( nodebits, 6 )) {
        vertices[num_vtx] = 6;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  2.0;
        ++num_vtx;
        derivs[2] -= 1.0;
      }
      
      if (have_node( nodebits, 7 )) {
        vertices[num_vtx] = 7;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] = -2.0;
        derivs[3*num_vtx+2] = -2.0;
        ++num_vtx;
        derivs[1] += 1.0;
        derivs[2] += 1.0;
        derivs[4] += 1.0;
        derivs[5] += 1.0;
      }
      
      if (have_node( nodebits, 8 )) {
        vertices[num_vtx] = 8;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  2.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[4] -= 1.0;
      }
      if (have_node( nodebits, 9 )) {
        vertices[num_vtx] = 9;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  2.0;
        ++num_vtx;
        derivs[5] -= 1.0;
      }
      break;
    
    case 4:
      num_vtx = 2;
      vertices[0] = 1;
      vertices[1] = 3;
      
      derivs[0] = 0.0;
      derivs[1] = 1.0;
      derivs[2] = 0.0;
      
      derivs[3] = -1.0;
      derivs[4] = -1.0;
      derivs[5] = -1.0;

      if (!have_node( nodebits, 4 ) && !have_node( nodebits, 7 )) {
        vertices[num_vtx] = 0;
        derivs[3*num_vtx  ] =  1.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      else if (have_node( nodebits, 4 ) && have_node( nodebits, 7 )) {
        vertices[num_vtx] = 0;
        derivs[3*num_vtx  ] = -1.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      
      if (!have_node( nodebits, 5 ) && !have_node( nodebits, 9 )) {
        vertices[num_vtx] = 2;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  1.0;
        ++num_vtx;
      }
      else if (have_node( nodebits, 5 ) && have_node( nodebits, 9 )) {
        vertices[num_vtx] = 2;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] = -1.0;
        ++num_vtx;
      }

      if (have_node( nodebits, 4 )) {
        vertices[num_vtx] = 4;
        derivs[3*num_vtx  ] =  2.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[0] -= 1.0;
      }
      
      if (have_node( nodebits, 5 )) {
        vertices[num_vtx] = 5;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  2.0;
        ++num_vtx;
        derivs[2] -= 1.0;
      }
      
      if (have_node( nodebits, 8 )) {
        vertices[num_vtx] = 8;
        derivs[3*num_vtx  ] = -2.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] = -2.0;
        ++num_vtx;
        derivs[0] += 1.0;
        derivs[2] += 1.0;
        derivs[3] += 1.0;
        derivs[5] += 1.0;
      }
      
      if (have_node( nodebits, 7 )) {
        vertices[num_vtx] = 7;
        derivs[3*num_vtx  ] =  2.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[3] -= 1.0;
      }
      if (have_node( nodebits, 9 )) {
        vertices[num_vtx] = 9;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  2.0;
        ++num_vtx;
        derivs[5] -= 1.0;
      }
      break;
    
    case 5:
      num_vtx = 2;
      vertices[0] = 2;
      vertices[1] = 3;
      
      derivs[0] = 0.0;
      derivs[1] = 0.0;
      derivs[2] = 1.0;
      
      derivs[3] = -1.0;
      derivs[4] = -1.0;
      derivs[5] = -1.0;

      if (!have_node( nodebits, 6 ) && !have_node( nodebits, 7 )) {
        vertices[num_vtx] = 0;
        derivs[3*num_vtx  ] =  1.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      else if (have_node( nodebits, 6 ) && have_node( nodebits, 7 )) {
        vertices[num_vtx] = 0;
        derivs[3*num_vtx  ] = -1.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      
      if (!have_node( nodebits, 5 ) && !have_node( nodebits, 8 )) {
        vertices[num_vtx] = 1;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  1.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      else if (have_node( nodebits, 5 ) && have_node( nodebits, 8 )) {
        vertices[num_vtx] = 1;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] = -1.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
      }
      
      if (have_node( nodebits, 5 )) {
        vertices[num_vtx] = 5;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  2.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[1] -= 1.0;
      }

      if (have_node( nodebits, 6 )) {
        vertices[num_vtx] = 6;
        derivs[3*num_vtx  ] =  2.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[0] -= 1.0;
      }
      
      if (have_node( nodebits, 9 )) {
        vertices[num_vtx] = 9;
        derivs[3*num_vtx  ] = -2.0;
        derivs[3*num_vtx+1] = -2.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[0] += 1.0;
        derivs[1] += 1.0;
        derivs[3] += 1.0;
        derivs[4] += 1.0;
      }
      
      if (have_node( nodebits, 7 )) {
        vertices[num_vtx] = 7;
        derivs[3*num_vtx  ] =  2.0;
        derivs[3*num_vtx+1] =  0.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[3] -= 1.0;
      }
      if (have_node( nodebits, 8 )) {
        vertices[num_vtx] = 8;
        derivs[3*num_vtx  ] =  0.0;
        derivs[3*num_vtx+1] =  2.0;
        derivs[3*num_vtx+2] =  0.0;
        ++num_vtx;
        derivs[4] -= 1.0;
      }
      break;
  }
}

// Derivatives of coefficients for higher-order nodes

const double ft = 4.0/3.0;

const double ho_dr[6][4] = { { ft, ft, 0., ft },
                             { 0., 0., 0., 0. },
                             { 0., ft, ft, ft },
                             { 0., ft, 0.,-ft },
                             {-ft,-ft, 0.,-ft },
                             { 0.,-ft,-ft,-ft } };

const double ho_ds[6][4] = { { ft, 0., ft, ft },
                             { 0., ft, ft, ft },
                             { 0., 0., 0., 0. },
                             {-ft, 0.,-ft,-ft },
                             { 0., 0., ft,-ft },
                             { 0.,-ft,-ft,-ft } };

const double ho_dt[6][4] = { { 0., 0., 0., 0. },
                             { ft, ft, 0., ft },
                             { ft, 0., ft, ft },
                             {-ft, 0.,-ft,-ft },
                             {-ft,-ft, 0.,-ft },
                             { ft, 0., 0.,-ft } };


void TetLagrangeShape::derivatives_at_mid_face( unsigned face,
                                              unsigned nodebits,
                                              size_t* vertices,
                                              double* derivs,
                                              size_t& num_vtx,
                                              MsqError& err) const
{
  if (nodebits >= (1u << 6)) {
    MSQ_SETERR(err)("TetLagrangeShape does not support mid-face/mid-element nodes",
                    MsqError::UNSUPPORTED_ELEMENT);
    return;
  }
  
    // begin with derivatives for linear tetrahedron
  num_vtx = 4;
  get_linear_derivatives( vertices, derivs );
  
  for (unsigned i = 0; i < 6; ++i) 
    if (nodebits & (1<<i)) {
      vertices[num_vtx] = i+4;
      derivs[3*num_vtx  ] = ho_dr[i][face];
      derivs[3*num_vtx+1] = ho_ds[i][face];
      derivs[3*num_vtx+2] = ho_dt[i][face];
      ++num_vtx;
      int j = 3*edges[i][0];
      derivs[j  ] -= 0.5*ho_dr[i][face];
      derivs[j+1] -= 0.5*ho_ds[i][face];
      derivs[j+2] -= 0.5*ho_dt[i][face];
      j = 3*edges[i][1];
      derivs[j  ] -= 0.5*ho_dr[i][face];
      derivs[j+1] -= 0.5*ho_ds[i][face];
      derivs[j+2] -= 0.5*ho_dt[i][face];
    }
}

void TetLagrangeShape::derivatives_at_mid_elem( 
                                              unsigned nodebits,
                                              size_t* vertices,
                                              double* derivs,
                                              size_t& num_vtx,
                                              MsqError& err) const
{
  if (nodebits >= (1u << 6)) {
    MSQ_SETERR(err)("TetLagrangeShape does not support mid-face/mid-element nodes",
                    MsqError::UNSUPPORTED_ELEMENT);
    return;
  }
                                    
  bool corners[4] = { false, false, false, false };
  double corner_vals[4][3] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
  
  num_vtx = 0;
  for (unsigned i = 4;  i < 10; ++i) {
    int sign, zero;
    if (i < 7) {
      sign = 1;
      zero = (i - 2) % 3;
    }
    else {
      sign = -1;
      zero = (i - 7);
    }
    
    if (have_node( nodebits, i )) {
      vertices[num_vtx] = i;
      derivs[3*num_vtx  ] = (double)sign;
      derivs[3*num_vtx+1] = (double)sign;
      derivs[3*num_vtx+2] = (double)sign;
      derivs[3*num_vtx+zero] = 0.0;
      ++num_vtx;
    }
    else {
      for (unsigned j = 0; j < 2; ++j) {
        int corner = edges[i-4][j];
        int v1 = (zero + 1) % 3;
        int v2 = (zero + 2) % 3;
        corners[corner] = true;
        corner_vals[corner][v1] += 0.5*sign;
        corner_vals[corner][v2] += 0.5*sign;
      }
    }
  }
  
  for (unsigned i = 0; i < 4; ++i)
    if (corners[i]) {
      vertices[num_vtx] = i;
      derivs[3*num_vtx  ] = corner_vals[i][0];
      derivs[3*num_vtx+1] = corner_vals[i][1];
      derivs[3*num_vtx+2] = corner_vals[i][2];
      ++num_vtx;
    }
}
    


} // namespace Mesquite
