/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2006 Lawrence Livermore National Laboratory.  Under 
    the terms of Contract B545069 with the University of Wisconsin -- 
    Madison, Lawrence Livermore National Laboratory retains certain
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

    (2006) kraftche@cae.wisc.edu    

  ***************************************************************** */


/** \file LinearHexahedron.cpp
 *  \brief Implement shape function for linear hexahedron
 *  \author Jason Kraftcheck 
 */

#include "Mesquite.hpp"
#include "MsqError.hpp"
#include "LinearHexahedron.hpp"

namespace Mesquite {

static const char* nonlinear_error 
 = "Attempt to use LinearHexahedron mapping function for a nonlinear element\n";

EntityTopology LinearHexahedron::element_topology() const
  { return HEXAHEDRON; }

static inline int coeff_xi_sign( unsigned coeff )
  { return 2*(((coeff+1)/2)%2) - 1; }
static inline int coeff_eta_sign( unsigned coeff )
  { return 2*((coeff/2)%2) - 1; }
static inline int coeff_zeta_sign( unsigned coeff )
  { return 2*(coeff/4) - 1; }
  
void LinearHexahedron::coefficients_at_corner( unsigned corner, 
                               unsigned nodebits,
                               msq_std::vector<double>& coeff_out,
                               MsqError& err ) const 
{
  if (nodebits) {
    MSQ_SETERR(err)(nonlinear_error, MsqError::UNSUPPORTED_ELEMENT );
    return;
  }
  
  coeff_out.resize(8);
  coeff_out[0] = 0.0;
  coeff_out[1] = 0.0;
  coeff_out[2] = 0.0;
  coeff_out[3] = 0.0;
  coeff_out[4] = 0.0;
  coeff_out[5] = 0.0;
  coeff_out[6] = 0.0;
  coeff_out[7] = 0.0;
  coeff_out[corner] = 1.0;
}
  
const unsigned xi = 0, eta = 1, zeta = 2;
const int edge_dir[] = { xi, eta, xi, eta, zeta, zeta, zeta, zeta, xi, eta, xi, eta };
const int edge_beg[]       = { 0, 1, 2, 3, 0, 1, 2, 3, 4, 5, 6, 7 }; // start vertex by edge number
const int edge_end[]       = { 1, 2, 3, 0, 4, 5, 6, 7, 5, 6, 7, 4 }; // end vetex by edge number
const int edge_opposite[]  = {10,11, 8, 9, 6, 7, 4, 5, 2, 3, 0, 1 }; // opposite edge
const int edge_beg_orth1[] = { 3, 5, 1, 7, 1, 0, 3, 2, 7, 1, 5, 3 }; // vtx adjacent to edge start in edge_dir[e]+1 direction
const int edge_beg_orth2[] = { 4, 0, 6, 2, 3, 2, 1, 0, 0, 4, 2, 6 }; // vtx adjacent to edge start in edge_dir[e]+2 direction
const int edge_end_orth1[] = { 2, 6, 0, 4, 5, 4, 7, 6, 6, 2, 4, 0 }; // vtx adjacent to edge end in edge_dir[e]+1 direction
const int edge_end_orth2[] = { 5, 3, 7, 1, 7, 6, 5, 4, 1, 7, 3, 5 }; // vtx adjacent to edge end in edge_dir[e]+2 direction

void LinearHexahedron::coefficients_at_mid_edge( unsigned edge, 
                                 unsigned nodebits,
                                 msq_std::vector<double>& coeff_out,
                                 MsqError& err ) const
{
  if (nodebits) {
    MSQ_SETERR(err)(nonlinear_error, MsqError::UNSUPPORTED_ELEMENT );
    return;
  }
  
  coeff_out.resize(8);
  coeff_out[0] = 0.0;
  coeff_out[1] = 0.0;
  coeff_out[2] = 0.0;
  coeff_out[3] = 0.0;
  coeff_out[4] = 0.0;
  coeff_out[5] = 0.0;
  coeff_out[6] = 0.0;
  coeff_out[7] = 0.0;
  coeff_out[edge_beg[edge]] = 0.5;
  coeff_out[edge_end[edge]] = 0.5;
}  
  
  
const int face_vtx[6][4] = { { 0, 1, 4, 5 },  // face 0 vertices
                             { 1, 2, 5, 6 },  // face 1 vertices
                             { 2, 3, 6, 7 },  // face 2
                             { 0, 3, 4, 7 },  // face 3
                             { 0, 1, 2, 3 },  // face 4
                             { 4, 5, 6, 7 } };// face 5
const int face_opp[6] = { 2, 3, 0, 1, 5, 4 };  // opposite faces on hex
const int face_dir[6] = { eta, xi, eta, xi, zeta, zeta }; // normal direction

void LinearHexahedron::coefficients_at_mid_face( unsigned face, 
                                 unsigned nodebits,
                                 msq_std::vector<double>& coeff_out,
                                 MsqError& err ) const
{
  if (nodebits) {
    MSQ_SETERR(err)(nonlinear_error, MsqError::UNSUPPORTED_ELEMENT );
    return;
  }
  
  coeff_out.resize(8);
  coeff_out[0] = 0.0;
  coeff_out[1] = 0.0;
  coeff_out[2] = 0.0;
  coeff_out[3] = 0.0;
  coeff_out[4] = 0.0;
  coeff_out[5] = 0.0;
  coeff_out[6] = 0.0;
  coeff_out[7] = 0.0;
  coeff_out[face_vtx[face][0]] = 0.25;
  coeff_out[face_vtx[face][1]] = 0.25;
  coeff_out[face_vtx[face][2]] = 0.25;
  coeff_out[face_vtx[face][3]] = 0.25;
}
                                           
  

void LinearHexahedron::coefficients_at_mid_elem( unsigned nodebits,
                                 msq_std::vector<double>& coeff_out,
                                 MsqError& err ) const
{
  if (nodebits) {
    MSQ_SETERR(err)(nonlinear_error, MsqError::UNSUPPORTED_ELEMENT );
    return;
  }

  coeff_out.resize(8);
  coeff_out[0] = 0.125;
  coeff_out[1] = 0.125;
  coeff_out[2] = 0.125;
  coeff_out[3] = 0.125;
  coeff_out[4] = 0.125;
  coeff_out[5] = 0.125;
  coeff_out[6] = 0.125;
  coeff_out[7] = 0.125;
}

void LinearHexahedron::derivatives_at_corner( unsigned corner, 
                              unsigned nodebits,
                              msq_std::vector<size_t>& vertex_indices_out,
                              msq_std::vector<double>& d_coeff_d_xi_out,
                              MsqError& err ) const
{
  if (nodebits) {
    MSQ_SETERR(err)(nonlinear_error, MsqError::UNSUPPORTED_ELEMENT );
    return;
  }
  
  const int   xi_sign = coeff_xi_sign(corner);
  const int  eta_sign = coeff_eta_sign(corner);
  const int zeta_sign = coeff_zeta_sign(corner);
  const int offset = 4*(corner/4);
  //const int adj_in_xi   = offset + (9 - corner)%4;
  const int adj_in_xi   = corner + 1 - 2*(corner%2);
  const int adj_in_eta  = 3 + 2*offset - (int)corner;
  const int adj_in_zeta = corner - zeta_sign*4;
  
  vertex_indices_out.resize(4);
  vertex_indices_out[0] = corner;
  vertex_indices_out[1] = adj_in_xi;
  vertex_indices_out[2] = adj_in_eta;
  vertex_indices_out[3] = adj_in_zeta;
  
  d_coeff_d_xi_out.resize(12);
  
  d_coeff_d_xi_out[ 0] =   xi_sign * 0.5;
  d_coeff_d_xi_out[ 1] =  eta_sign * 0.5;
  d_coeff_d_xi_out[ 2] = zeta_sign * 0.5;
  
  d_coeff_d_xi_out[ 3] =  -xi_sign * 0.5;
  d_coeff_d_xi_out[ 4] =             0.0;
  d_coeff_d_xi_out[ 5] =             0.0;
  
  d_coeff_d_xi_out[ 6] =             0.0;
  d_coeff_d_xi_out[ 7] = -eta_sign * 0.5;
  d_coeff_d_xi_out[ 8] =             0.0;
  
  d_coeff_d_xi_out[ 9] =             0.0;
  d_coeff_d_xi_out[10] =             0.0;
  d_coeff_d_xi_out[11] =-zeta_sign * 0.5;
}


void LinearHexahedron::derivatives_at_mid_edge( unsigned edge, 
                                unsigned nodebits,
                                msq_std::vector<size_t>& vertex_indices_out,
                                msq_std::vector<double>& d_coeff_d_xi_out,
                                MsqError& err ) const
{
  if (nodebits) {
    MSQ_SETERR(err)(nonlinear_error, MsqError::UNSUPPORTED_ELEMENT );
    return;
  }
  
  const int direction = edge_dir[edge];
  const int ortho1    = (direction+1)%3;
  const int ortho2    = (direction+2)%3;
  const int vtx = edge_beg[edge];
  const int signs[] = { coeff_xi_sign(vtx), coeff_eta_sign(vtx), coeff_zeta_sign(vtx) };
  const int sign_dir = signs[direction];
  const int sign_or1 = signs[ortho1   ];
  const int sign_or2 = signs[ortho2   ];

  vertex_indices_out.resize(6);
  vertex_indices_out[0] = edge_beg[edge];
  vertex_indices_out[1] = edge_end[edge];
  vertex_indices_out[2] = edge_beg_orth1[edge];
  vertex_indices_out[3] = edge_end_orth1[edge];
  vertex_indices_out[4] = edge_beg_orth2[edge];
  vertex_indices_out[5] = edge_end_orth2[edge];
  
  d_coeff_d_xi_out.resize(18);
  
  
  d_coeff_d_xi_out[ 0+direction] =  sign_dir * 0.50;
  d_coeff_d_xi_out[ 0+ortho1   ] =  sign_or1 * 0.25;
  d_coeff_d_xi_out[ 0+ortho2   ] =  sign_or2 * 0.25;
  
  d_coeff_d_xi_out[ 3+direction] = -sign_dir * 0.50;
  d_coeff_d_xi_out[ 3+ortho1   ] =  sign_or1 * 0.25;
  d_coeff_d_xi_out[ 3+ortho2   ] =  sign_or2 * 0.25;
 
  d_coeff_d_xi_out[ 6+direction] =             0.00;
  d_coeff_d_xi_out[ 6+ortho1   ] = -sign_or1 * 0.25;
  d_coeff_d_xi_out[ 6+ortho2   ] =             0.00;
 
  d_coeff_d_xi_out[ 9+direction] =             0.00;
  d_coeff_d_xi_out[ 9+ortho1   ] = -sign_or1 * 0.25;
  d_coeff_d_xi_out[ 9+ortho2   ] =             0.00;
 
  d_coeff_d_xi_out[12+direction] =             0.00;
  d_coeff_d_xi_out[12+ortho1   ] =             0.00;
  d_coeff_d_xi_out[12+ortho2   ] = -sign_or2 * 0.25;
 
  d_coeff_d_xi_out[15+direction] =             0.00;
  d_coeff_d_xi_out[15+ortho1   ] =             0.00;
  d_coeff_d_xi_out[15+ortho2   ] = -sign_or2 * 0.25;
}


void LinearHexahedron::derivatives_at_mid_face( unsigned face, 
                                unsigned nodebits,
                                msq_std::vector<size_t>& vertex_indices_out,
                                msq_std::vector<double>& d_coeff_d_xi_out,
                                MsqError& err ) const
{
  if (nodebits) {
    MSQ_SETERR(err)(nonlinear_error, MsqError::UNSUPPORTED_ELEMENT );
    return;
  }
  
  const int vtx_signs[4][3] = { { coeff_xi_sign  (face_vtx[face][0]),
                                  coeff_eta_sign (face_vtx[face][0]),
                                  coeff_zeta_sign(face_vtx[face][0]) },
                                { coeff_xi_sign  (face_vtx[face][1]),
                                  coeff_eta_sign (face_vtx[face][1]),
                                  coeff_zeta_sign(face_vtx[face][1]) },
                                { coeff_xi_sign  (face_vtx[face][2]),
                                  coeff_eta_sign (face_vtx[face][2]),
                                  coeff_zeta_sign(face_vtx[face][2]) },
                                { coeff_xi_sign  (face_vtx[face][3]),
                                  coeff_eta_sign (face_vtx[face][3]),
                                  coeff_zeta_sign(face_vtx[face][3]) } };
  const int ortho_dir = face_dir[face];
  const int face_dir1 = (ortho_dir+1) % 3;
  const int face_dir2 = (ortho_dir+2) % 3;
  const int ortho_sign = vtx_signs[0][ortho_dir];  // same for all 4 verts
  
  vertex_indices_out.resize(8);
  vertex_indices_out[0] = face_vtx[face][0];
  vertex_indices_out[1] = face_vtx[face][1];
  vertex_indices_out[2] = face_vtx[face][2];
  vertex_indices_out[3] = face_vtx[face][3];
  vertex_indices_out[4] = face_vtx[face_opp[face]][0];
  vertex_indices_out[5] = face_vtx[face_opp[face]][1];
  vertex_indices_out[6] = face_vtx[face_opp[face]][2];
  vertex_indices_out[7] = face_vtx[face_opp[face]][3];
  
  d_coeff_d_xi_out.resize( 24 );
  
  d_coeff_d_xi_out[ 0+ortho_dir] = ortho_sign * 0.125;
  d_coeff_d_xi_out[ 0+face_dir1] = vtx_signs[0][face_dir1] * 0.25;
  d_coeff_d_xi_out[ 0+face_dir2] = vtx_signs[0][face_dir2] * 0.25;
  
  d_coeff_d_xi_out[ 3+ortho_dir] = ortho_sign * 0.125;
  d_coeff_d_xi_out[ 3+face_dir1] = vtx_signs[1][face_dir1] * 0.25;
  d_coeff_d_xi_out[ 3+face_dir2] = vtx_signs[1][face_dir2] * 0.25;
  
  d_coeff_d_xi_out[ 6+ortho_dir] = ortho_sign * 0.125;
  d_coeff_d_xi_out[ 6+face_dir1] = vtx_signs[2][face_dir1] * 0.25;
  d_coeff_d_xi_out[ 6+face_dir2] = vtx_signs[2][face_dir2] * 0.25;
  
  d_coeff_d_xi_out[ 9+ortho_dir] = ortho_sign * 0.125;
  d_coeff_d_xi_out[ 9+face_dir1] = vtx_signs[3][face_dir1] * 0.25;
  d_coeff_d_xi_out[ 9+face_dir2] = vtx_signs[3][face_dir2] * 0.25;
  
  d_coeff_d_xi_out[12+ortho_dir] = -ortho_sign * 0.125;
  d_coeff_d_xi_out[12+face_dir1] = 0.0;
  d_coeff_d_xi_out[12+face_dir2] = 0.0;
  
  d_coeff_d_xi_out[15+ortho_dir] = -ortho_sign * 0.125;
  d_coeff_d_xi_out[15+face_dir1] = 0.0;
  d_coeff_d_xi_out[15+face_dir2] = 0.0;
  
  d_coeff_d_xi_out[18+ortho_dir] = -ortho_sign * 0.125;
  d_coeff_d_xi_out[18+face_dir1] = 0.0;
  d_coeff_d_xi_out[18+face_dir2] = 0.0;
  
  d_coeff_d_xi_out[21+ortho_dir] = -ortho_sign * 0.125;
  d_coeff_d_xi_out[21+face_dir1] = 0.0;
  d_coeff_d_xi_out[21+face_dir2] = 0.0;
}

void LinearHexahedron::derivatives_at_mid_elem( unsigned nodebits,
                                msq_std::vector<size_t>& vertex_indices_out,
                                msq_std::vector<double>& d_coeff_d_xi_out,
                                MsqError& err ) const

{
  if (nodebits) {
    MSQ_SETERR(err)(nonlinear_error, MsqError::UNSUPPORTED_ELEMENT );
    return;
  }
  
  vertex_indices_out.resize(8);
  vertex_indices_out[0] = 0;
  vertex_indices_out[1] = 1;
  vertex_indices_out[2] = 2;
  vertex_indices_out[3] = 3;
  vertex_indices_out[4] = 4;
  vertex_indices_out[5] = 5;
  vertex_indices_out[6] = 6;
  vertex_indices_out[7] = 7;
  
  d_coeff_d_xi_out.resize(24);

  d_coeff_d_xi_out[ 0] = -0.125;
  d_coeff_d_xi_out[ 1] = -0.125;
  d_coeff_d_xi_out[ 2] = -0.125;

  d_coeff_d_xi_out[ 3] =  0.125;
  d_coeff_d_xi_out[ 4] = -0.125;
  d_coeff_d_xi_out[ 5] = -0.125;

  d_coeff_d_xi_out[ 6] =  0.125;
  d_coeff_d_xi_out[ 7] =  0.125;
  d_coeff_d_xi_out[ 8] = -0.125;

  d_coeff_d_xi_out[ 9] = -0.125;
  d_coeff_d_xi_out[10] =  0.125;
  d_coeff_d_xi_out[11] = -0.125;

  d_coeff_d_xi_out[12] = -0.125;
  d_coeff_d_xi_out[13] = -0.125;
  d_coeff_d_xi_out[14] =  0.125;

  d_coeff_d_xi_out[15] =  0.125;
  d_coeff_d_xi_out[16] = -0.125;
  d_coeff_d_xi_out[17] =  0.125;

  d_coeff_d_xi_out[18] =  0.125;
  d_coeff_d_xi_out[19] =  0.125;
  d_coeff_d_xi_out[20] =  0.125;

  d_coeff_d_xi_out[21] = -0.125;
  d_coeff_d_xi_out[22] =  0.125;
  d_coeff_d_xi_out[23] =  0.125;
}


} // namespace Mesquite
