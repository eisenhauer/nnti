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


/** \file main.cpp
 *  \brief Implement all experiments from 2D Metrics Paper
 *  \author Jason Kraftcheck 
 */
#include "Mesquite.hpp"
#include "MeshImpl.hpp"
#include "MsqError.hpp"
#include "MsqVertex.hpp"
#include "PMeanPTemplate.hpp"
#include "ConjugateGradient.hpp"
#include "FeasibleNewton.hpp"
#include "PlanarDomain.hpp"
#include "SamplePoints.hpp"
#include "LinearFunctionSet.hpp"
#include "UnitWeight.hpp"
#include "ReferenceMesh.hpp"
#include "RefMeshTargetCalculator.hpp"
#include "TMPQualityMetric.hpp"
#include "InstructionQueue.hpp"
#include "IdealTargetCalculator.hpp"
#include "MeshWriter.hpp"

#include "TSquared2D.hpp"
#include "Target2DShape.hpp"
#include "Target2DShapeBarrier.hpp"
#include "Target2DShapeOrient.hpp"
#include "Target2DShapeOrientAlt1.hpp"
#include "Target2DShapeOrientAlt2.hpp"
#include "Target2DShapeOrientBarrier.hpp"
#include "Target2DShapeSize.hpp"
#include "Target2DShapeSizeBarrier.hpp"
#include "Target2DShapeSizeBarrierAlt1.hpp"
#include "Target2DShapeSizeBarrierAlt2.hpp"
#include "Target2DShapeSizeBarrierAlt1.hpp"
#include "Target2DShapeSizeOrient.hpp"
#include "Target2DShapeSizeOrientAlt1.hpp"
#include "Target2DShapeSizeOrientBarrier.hpp"
#include "Target2DShapeSizeOrientBarrierAlt2.hpp"
#include "InvTransBarrier2D.hpp"

#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <assert.h>
using namespace std;
using namespace Mesquite;

#define CHKERR( A ) if (MSQ_CHKERR((A))) { cerr << (A) << std::endl; exit(3); }

const double P = 1.0; // objective function power
const bool ONE_TRI_SAMPLE = true;
const bool LOCAL_PATCHES = true;
const bool USE_FEAS_NEWT = false;
const bool WRITE_VTK = false;
const bool WRITE_GNUPLOT = true;

void usage() {
  cerr << "main [e[.m]]" << endl;
  cerr << "Run experiments from 2d metrics paper" << endl;
  cerr << "e : expiriment" << endl;
  cerr << "m : metric number within experiment" << endl;
  cerr << "default is all" << endl;
}

typedef void (*mesh_reader_t)( MeshImpl* mesh );

bool run_smoother( mesh_reader_t input_mesh, 
                   mesh_reader_t reference_mesh,
                   int exp, int n,
                   TargetMetric2D* metric );
                   
void write_mesh( mesh_reader_t mesh, const char* filename );
void write_mesh( MeshImpl* mesh, const char* filename );

void quarter_annulus( double inner, double outer, Mesh* mesh );
void parabolic_squash( double height, Mesh* mesh );
void horseshoe( double x_inner, double x_outer, double y_inner, double y_outer, Mesh* mesh );
void scale( double s, Mesh* mesh );

void reference( MeshImpl* mesh )
{
  MsqError err;
  mesh->read_vtk( SRCDIR "reference.vtk", err );
  CHKERR(err)
}

void exp_1_init( MeshImpl* mesh )
{
  MsqError err;
  mesh->read_vtk( SRCDIR "reference.vtk", err );
  CHKERR(err)

  vector<Mesh::VertexHandle> handles;
  mesh->get_all_vertices( handles, err );
  CHKERR(err)
  
  vector<Mesh::VertexHandle>::iterator i;
  for (size_t i = 0; i < 8; ++i) {
    MsqVertex vtx;
    mesh->vertices_get_coordinates( &handles[i], &vtx, 1, err );
    CHKERR(err)
    
    vtx[1] = -0.4 * (vtx[0]-0.5)*(vtx[0]-0.5) + 0.1;
    
    mesh->vertex_set_coordinates( handles[i], vtx, err );
    CHKERR(err)
  }
}

void exp_2_init( MeshImpl* mesh )
{
  exp_1_init( mesh );
  scale( 8.0, mesh ); 
}

void exp_3_ref( MeshImpl* mesh )
{
  reference( mesh );
  quarter_annulus( 0.65, 1.30, mesh ); 
}

void exp_4_init( MeshImpl* mesh )
{
  reference( mesh );
  parabolic_squash( 0.8, mesh ); 
}

void exp_5_init( MeshImpl* mesh )
{
  reference( mesh );
  horseshoe( 0.26, 0.53, 0.26, 1.3, mesh ); 
}

  

bool run_exp_1( int n );
bool run_exp_2( int n );
bool run_exp_3( int n );
bool run_exp_4( int n );
bool run_exp_5( int n );

typedef bool (*exp_func)(int);
const exp_func experiment[] = { &run_exp_1, &run_exp_2, &run_exp_3, &run_exp_4, &run_exp_5 };

Target2DShapeSizeOrient nb1;
Target2DShapeOrient nb2;
Target2DShapeSize nb3;
Target2DShape nb4;
TSquared2D nb5;
Target2DShapeOrientAlt1 nb6;
Target2DShapeSizeOrientAlt1 nb7;
TargetMetric2D* nb[] = { &nb1, &nb2, &nb3, &nb4, &nb5, &nb6, &nb7 };

Target2DShapeSizeOrientBarrier b1;
Target2DShapeOrientBarrier b2;
Target2DShapeSizeBarrier b3;
Target2DShapeBarrier b4;
Target2DShapeSizeOrientBarrierAlt2 b5;
Target2DShapeSizeBarrierAlt1 b6;
Target2DShapeSizeBarrierAlt2 b7;
TargetMetric2D* b[] = { &b1, &b2, &b3, &b4, &b5, &b6, &b7 };

bool run_exp_1( int n )
{
  if (n > 6)
    return false;
  
  write_mesh( exp_1_init, "Exp1-initial" );
  
  bool r = true, tmpr;
  int beg = 0, end = 5;
  if (n) beg = end = n-1;
  
  for (int i = beg; i <= end; ++i) {
    if (i == 5) 
      tmpr = run_smoother( &exp_1_init, 0, 1, 6, nb[0] );
    else
      tmpr = run_smoother( &exp_1_init, &reference, 1, i+1, nb[i] );
    r = r && tmpr;
  }
  return r;
}

bool run_exp_2( int n )
{
  if (n > 6)
    return false;
  
  write_mesh( exp_2_init, "Exp2-initial" );
  
  bool r = true, tmpr;
  int beg = 0, end = 5;
  if (n) beg = end = n-1;
    
  for (int i = beg; i <= end; ++i) {
    if (i == 5) 
      tmpr = run_smoother( &exp_2_init, 0, 2, 6, nb[0] );
    else
      tmpr = run_smoother( &exp_2_init, &reference, 2, i+1, nb[i] );
    r = r && tmpr;
  }
  return r;
}

bool run_exp_3( int n )
{
  if (n > 6)
    return false;
  
  write_mesh( exp_3_ref, "Exp3-reference" );
  
  bool r = true, tmpr;
  int beg = 0, end = 5;
  if (n) beg = end = n-1;
  
  for (int i = beg; i <= end; ++i) {
    if (i == 5) 
      tmpr = run_smoother( &exp_1_init, 0, 3, 6, nb[0] );
    else
      tmpr = run_smoother( &exp_1_init, &exp_3_ref, 3, i+1, nb[i] );
    r = r && tmpr;
  }
  return r;
}

bool run_exp_4( int n )
{
  if (n > 3)
    return false;
  
  write_mesh( exp_4_init, "Exp4-initial" );
  TargetMetric2D* m[3] = {&nb1, &b1, &b5};
 
  bool r = true, tmpr;
  int beg = 0, end = 2;
  if (n) beg = end = n-1;
  
  for (int i = beg; i <= end; ++i) {
    tmpr = run_smoother( &exp_4_init, &reference, 4, i+1, m[i] );
    r = r && tmpr;
  }
  return r;
}

bool run_exp_5( int n )
{
  if (n > 12 || n == 5)
    return false;
  
  write_mesh( exp_5_init, "Exp5-initial" );
  
  bool r = true, tmpr;
  int beg = 0, end = 11;
  if (n) beg = end = n-1;
  
  for (int i = beg; i <= end; ++i) {
    switch(i) {
      case 0: case 1: case 2: case 3: case 4:
        tmpr = run_smoother( &exp_5_init, &reference, 5, i+1, b[i] );
        break;
      case 5:
        break;
      case 6: case 7: case 8: case 9: case 10:
      {
        InvTransBarrier2D metric( nb[i-6] );
        tmpr = run_smoother( &exp_5_init, &reference, 5, i+1, &metric );
      }
      break;
      case 11:
      {
        InvTransBarrier2D metric( nb[0] );
        tmpr = run_smoother( &exp_5_init, 0, 5, 12, &metric );
      }
      break;
    }
    r = r && tmpr;
  }
  return r;
}

void scale( double s, Mesh* mesh )
{
  MsqError err;
  vector<Mesh::VertexHandle> handles;
  mesh->get_all_vertices( handles, err );
  CHKERR(err)
  
  vector<Mesh::VertexHandle>::iterator i;
  for (i = handles.begin(); i != handles.end(); ++i) {
    MsqVertex vtx;
    mesh->vertices_get_coordinates( &*i, &vtx, 1, err );
    CHKERR(err)
    
    vtx *= s;
    
    mesh->vertex_set_coordinates( *i, vtx, err );
    CHKERR(err)
  }
}

void quarter_annulus( double inner, double outer, Mesh* mesh )
{
  MsqError err;
  vector<Mesh::VertexHandle> handles;
  mesh->get_all_vertices( handles, err );
  CHKERR(err)
  
  vector<Mesh::VertexHandle>::iterator i;
  for (i = handles.begin(); i != handles.end(); ++i) {
    MsqVertex vtx;
    mesh->vertices_get_coordinates( &*i, &vtx, 1, err );
    CHKERR(err)
    
    double r = inner + (outer - inner) * vtx[1];
    double a = M_PI * 0.5 * (1.0 - vtx[0]);
    vtx[0] = r * cos(a);
    vtx[1] = r * sin(a);
    
    mesh->vertex_set_coordinates( *i, vtx, err );
    CHKERR(err)
  }
}

void parabolic_squash( double height, Mesh* mesh )
{
  MsqError err;
  vector<Mesh::VertexHandle> handles;
  mesh->get_all_vertices( handles, err );
  CHKERR(err)
  
  vector<Mesh::VertexHandle>::iterator i;
  for (i = handles.begin(); i != handles.end(); ++i) {
    MsqVertex vtx;
    mesh->vertices_get_coordinates( &*i, &vtx, 1, err );
    CHKERR(err)
    
    const double b = (1.0 - vtx[1]) * height;
    const double a = -4 * b;
    vtx[1] += a * (vtx[0]-0.5)*(vtx[0]-0.5) + b;
    
    mesh->vertex_set_coordinates( *i, vtx, err );
    CHKERR(err)
  }
}

void horseshoe( double x_inner, double x_outer, double y_inner, double y_outer, Mesh* mesh )
{
  MsqError err;
  vector<Mesh::VertexHandle> handles;
  mesh->get_all_vertices( handles, err );
  CHKERR(err)
  
  vector<Mesh::VertexHandle>::iterator i;
  for (i = handles.begin(); i != handles.end(); ++i) {
    MsqVertex vtx;
    mesh->vertices_get_coordinates( &*i, &vtx, 1, err );
    CHKERR(err)
    
    double a = M_PI * (1-vtx[0]);
    vtx[0] =  (x_inner + (x_outer - x_inner) * vtx[1]) * cos(a);
    vtx[1] =  (y_inner + (y_outer - y_inner) * vtx[1]) * sin(a);
    
    mesh->vertex_set_coordinates( *i, vtx, err );
    CHKERR(err)
  }
}

void write_mesh( MeshImpl* mesh, const char* filename )
{
  MsqError err;
  if (WRITE_VTK) {
    string vfile = string(filename) + ".vtk";
    mesh->write_vtk( vfile.c_str(), err );
    CHKERR(err)
    cout << "Wrote: \"" << vfile << '"' << endl;
  }
  if (WRITE_GNUPLOT) {
    string vfile = string(filename) + ".gpt";
    MeshWriter::write_gnuplot( mesh, filename, err );
    CHKERR(err)
    cout << "Wrote: \"" << vfile << '"' << endl;
  }
}

void write_mesh( mesh_reader_t mesh_func, const char* filename )
{
  MeshImpl mesh;
  mesh_func( &mesh );
  write_mesh( &mesh, filename );
}

bool run_smoother( mesh_reader_t input_mesh, 
                   mesh_reader_t reference_mesh,
                   int exp, int n,
                   TargetMetric2D* target_metric )
{
  MsqError err;
  MeshImpl active, reference;
  input_mesh( &active );

  ReferenceMesh refmesh( &reference );
  RefMeshTargetCalculator ref_target( &refmesh );
  IdealTargetCalculator ident_target;
  TargetCalculator* target;
  if (reference_mesh) {
    reference_mesh( &reference );
    target = &ref_target;
  }
  else {
    target = &ident_target;
  }
  
  UnitWeight weight;
  SamplePoints corners(true,false,false,false);
  if (ONE_TRI_SAMPLE) {
    corners.dont_sample_at( TRIANGLE, 0 );
    corners.sample_at( TRIANGLE, 2 );
  }
    
  TMPQualityMetric metric( &corners, target, &weight, target_metric, 0 );
  
  TerminationCriterion outer, inner;
  if (LOCAL_PATCHES) {
    outer.add_criterion_type_with_int( TerminationCriterion::NUMBER_OF_ITERATES, 100, err );
    CHKERR(err)
    outer.add_criterion_type_with_double( TerminationCriterion::VERTEX_MOVEMENT_ABSOLUTE, 1e-3, err );
    CHKERR(err)
    inner.add_criterion_type_with_int( TerminationCriterion::NUMBER_OF_ITERATES, 3, err );
    CHKERR(err)
  }
  else {
    outer.add_criterion_type_with_int( TerminationCriterion::NUMBER_OF_ITERATES, 1, err );
    CHKERR(err)
    inner.add_criterion_type_with_double( TerminationCriterion::VERTEX_MOVEMENT_ABSOLUTE, 1e-4, err );
    CHKERR(err)
    inner.add_criterion_type_with_int( TerminationCriterion::NUMBER_OF_ITERATES, 100, err );
    CHKERR(err)
  }
  
  PMeanPTemplate of( P, &metric );
  ConjugateGradient cg( &of );
  FeasibleNewton fn( &of );
  if (LOCAL_PATCHES) {
    cg.use_element_on_vertex_patch();
    fn.use_element_on_vertex_patch();
  }
  else {
    cg.use_global_patch();
    fn.use_global_patch();
  }
  VertexMover* solver = USE_FEAS_NEWT ? (VertexMover*)&fn : (VertexMover*)&cg;
  LinearFunctionSet mfs;
  PlanarDomain plane( PlanarDomain::XY );
  solver->set_inner_termination_criterion( &inner );
  solver->set_outer_termination_criterion( &outer );

  InstructionQueue q;
  q.set_master_quality_improver( solver, err );
  CHKERR(err)

  cout << "Running " << exp << "." << n << " ...";
  q.run_instructions( &active, &plane, &mfs, err );
  if (MSQ_CHKERR(err)) {
    cout << "######## EXPERIMENT " << exp << "." << n << " FAILED! ##########" << endl;
    return false;
  }

  ostringstream s;
  s << "Exp" << exp << "-" << n;
  write_mesh( &active, s.str().c_str() );
  return true;
}

int main( int argc, char* argv[] )
{
  if (argc > 2) usage();
  int exp = 0, n = 0;
  if (argc == 2 && !sscanf(argv[1], "%d.%d", &exp, &n)) 
    usage();
  
  if (exp > 5) 
    usage();
  
  int lower, upper;
  if (exp > 0) {
    lower = upper = exp-1;
  }
  else {
    lower = 0; upper = 4;
  }
  
  if (WRITE_GNUPLOT)
    write_mesh( reference, "reference" );
  
  int fail = 0;
  for (int e = lower; e <= upper; ++e)
    fail += !(experiment[e](n));
    
  return fail;
}
