/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2007 Sandia National Laboratories.  Developed at the
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

    (2007) kraftche@cae.wisc.edu    

  ***************************************************************** */


/** \file main.cpp
 *  \brief Test high aspect ratio case
 *  \author Jason Kraftcheck 
 */

#include "Mesquite.hpp"
#include "MsqError.hpp"
#include "MeshImpl.hpp"
#include "XYRectangle.hpp"

#include "ReferenceMesh.hpp"
#include "RefMeshTargetCalculator.hpp"
#include "Target2DShapeBarrier.hpp"
#include "TMPQualityMetric.hpp"

#include "TerminationCriterion.hpp"
#include "QualityAssessor.hpp"
#include "PMeanPTemplate.hpp"
#include "FeasibleNewton.hpp"
#include "ConjugateGradient.hpp"
#include "InstructionQueue.hpp"

#ifdef MSQ_USE_OLD_IO_HEADERS
#  include <iostream.h>
#  include <fstream.h>
#else
#  include <iostream>
#  include <fstream>
#endif

#ifdef MSQ_USE_OLD_C_HEADERS
# include <stdlib.h>
# include <stdio.h>
#else
# include <cstdlib>
# include <cstdio>
#endif

using namespace Mesquite;

//! Struct in which to store mesh description
struct MeshParams { double x, y, w, h; };
msq_stdio::ostream& operator<<( msq_stdio::ostream& str, const MeshParams& p )
  { return str << p.x << ',' << p.y << ',' << p.w << ',' << p.h; }

//! Default values for parameters.
const char default_out_file[] = "high_aspect.vtk";
const MeshParams default_mesh = { 0.6, 0.3, 2.0, 1.0 };
const MeshParams default_ref  = { 0.5, 0.5, 1.0, 1.0 };

const int INNER_ITERATES = 1;
const int OUTER_ITERATES = 10;

enum ExitCodes {
  NO_ERROR = 0,
  USAGE_ERROR = 1,
  NOT_AT_TARGET = 2,
  FAR_FROM_TARGET = 3,
  WRONG_DIRECTION = 4,
  DEGENERATE_ELEMENT = 5,
  INVERTED_ELEMENT = 6,
  LAST_EXIT_CODE = INVERTED_ELEMENT
};

void usage( const char* argv0, bool brief = true )
{
  msq_stdio::ostream& str = brief ? msq_stdio::cerr : msq_stdio::cout;
  
  str << "Usage: " << argv0 
      << " [-o <output_file>]"
      << " [-f|-F] [-t|-T] [-n|-c] [-i <n>]"
      << " [-m <x>,<y>[,<w>,<h>]]"
      << " [-r <x>,<y>[,<w>,<h>]]"
      << msq_stdio::endl;
  if (brief) {
    str << "       " << argv0 << " -h" << msq_stdio::endl;
    msq_std::exit(USAGE_ERROR);
  }
  
  str << "  -o  Specify output file (default is \"" << default_out_file << "\")" << msq_stdio::endl
      << "  -f  Fixed boundary vertices" << msq_stdio::endl
      << "  -F  Free boundary vertices (default)" << msq_stdio::endl
      << "  -t  Write VTK timesteps" << msq_stdio::endl
      << "  -T  Write GNUPlot timesteps" << msq_stdio::endl
      << "  -m  Specify input mesh parameters (default " << default_mesh << ")" << msq_stdio::endl
      << "  -r  Specify reference mesh parameters (default " << default_ref << ")" << msq_stdio::endl
      << "  -n  Use FeasibleNewton solver" << msq_stdio::endl
      << "  -c  Use ConjugateGradient solver (default)" << msq_stdio::endl
      << "  -i  Specify number of iterations (default:" << OUTER_ITERATES << ")" << msq_stdio::endl
      << msq_stdio::endl;
  
  msq_std::exit(NO_ERROR);
}

#define CHECKERR \
  if (err) { \
    msq_stdio::cerr << "Internal error at line " << __LINE__ << ":" << msq_stdio::endl \
                    << (err) << msq_stdio::endl; \
    msq_std::exit( LAST_EXIT_CODE + (err).error_code() );  \
  } 


/*    |<----- x ----->|
 *   (6)-------------(7)-------------(8)--
 *    |               |               | ^
 *    |               |               | |
 *    |      [2]      |      [3]      | |
 *    |               |               | |
 *    |               |               | |
 * --(3)-------------(4)-------------(5)h
 *  ^ |               |               | |
 *  | |               |               | |
 *  y |      [0]      |      [1]      | |
 *  | |               |               | |
 *  v |               |               | v
 * --(0)-------------(1)-------------(2)--
 *    |<------------- w ------------->|
 *
 * z = 0
 */
void create_input_mesh( const MeshParams& params,
                        bool all_fixed,
                        MeshImpl& mesh, 
                        MsqError& err );

void parse_options( char* argv[], 
                    int argc,
                    MeshParams& mesh,
                    MeshParams& ref,
                    msq_std::string& output_file,
                    bool& fixed_boundary,
                    TerminationCriterion::TimeStepFileType& write_timesteps,
                    bool& use_feas_newt,
                    int& num_iterations );

msq_std::string base_name( msq_std::string filename );

int main( int argc, char* argv[] )
{
  MeshParams input_params, reference_params;
  bool fixed_boundary_vertices, feas_newt_solver;
  TerminationCriterion::TimeStepFileType write_timestep_files;
  msq_std::string output_file_name;
  int num_iterations;
  
  parse_options( argv, argc,
                 input_params, reference_params,
                 output_file_name,
                 fixed_boundary_vertices,
                 write_timestep_files,
                 feas_newt_solver,
                 num_iterations );
  
  MsqError err;
  MeshImpl mesh, refmesh;
  XYRectangle domain( input_params.w, input_params.h );
  create_input_mesh( input_params, fixed_boundary_vertices, mesh, err ); CHECKERR
  create_input_mesh( reference_params, fixed_boundary_vertices, refmesh, err ); CHECKERR
  domain.setup( &mesh, err ); CHECKERR

  ReferenceMesh rmesh( &refmesh );
  RefMeshTargetCalculator tc( &rmesh );
  Target2DShapeBarrier tm;
  TMPQualityMetric qm( &tc, &tm, 0 );
  
  PMeanPTemplate of( 1.0, &qm );
  ConjugateGradient cg( &of );
  cg.use_element_on_vertex_patch();
  FeasibleNewton fn( &of );
  fn.use_element_on_vertex_patch();
  VertexMover* solver = feas_newt_solver ? (VertexMover*)&fn : (VertexMover*)&cg;
  
  TerminationCriterion inner, outer;
  inner.add_iteration_limit( INNER_ITERATES );
  outer.add_iteration_limit( num_iterations );
  if (write_timestep_files != TerminationCriterion::NOTYPE) 
    outer.write_mesh_steps( base_name( output_file_name ).c_str(), write_timestep_files );
  solver->set_inner_termination_criterion( &inner );
  solver->set_outer_termination_criterion( &outer );
  
  QualityAssessor qa( &qm );
  InstructionQueue q;
  q.add_quality_assessor( &qa, err );
  q.set_master_quality_improver( solver, err );
  q.add_quality_assessor( &qa, err );
  
  q.run_instructions( &mesh, &domain, err ); CHECKERR
  
  mesh.write_vtk( output_file_name.c_str(), err ); CHECKERR
  
    // check for inverted elements
  int inv, unk;
  qa.get_inverted_element_count( inv, unk, err );
  if (inv) {
    msq_stdio::cerr << inv << " inverted elements in final mesh" << msq_stdio::endl;
    return INVERTED_ELEMENT;
  }
  else if (unk) {
    msq_stdio::cerr << unk << " degenerate elements in final mesh" << msq_stdio::endl;
    return DEGENERATE_ELEMENT;
  }
    
    // find the free vertex
  msq_std::vector<Mesh::VertexHandle> vertices;
  mesh.get_all_vertices( vertices, err );
  if (vertices.empty()) {
    std::cerr << "Mesh contains no vertices" << std::endl;
    return USAGE_ERROR;
  }
  msq_std::vector<unsigned short> dof( vertices.size(), -1 );
  domain.domain_DoF( &vertices[0], &dof[0], vertices.size(), err ); CHECKERR
  int idx = std::find(dof.begin(), dof.end(), 2) - dof.begin();
  const Mesh::VertexHandle free_vertex = vertices[idx];
  MsqVertex coords;
  mesh.vertices_get_coordinates( &free_vertex, &coords,1, err ); CHECKERR
  
    // calculate optimal position for vertex
  const double xf = reference_params.x / reference_params.w;
  const double yf = reference_params.y / reference_params.h;
  Vector3D expect( xf * input_params.w, yf * input_params.h, 0 );

  // Test that we aren't further from the expected location
  // than when we started. 
  const Vector3D init( input_params.x, input_params.y, 0 );
  if ((coords - expect).length() > (init - expect).length()) {
    msq_stdio::cerr << "Vertex moved away from expected optimal position: "
                    << "(" << coords[0] << ", " << coords[1] << msq_stdio::endl;
    return WRONG_DIRECTION;
  }
  
    // check if vertex moved MIN_FRACT of the way from the original position
    // to the desired one in the allowed iterations
  const double MIN_FRACT = 0.2; // 20% of the way in 10 iterations
  const double fract = (coords - init).length() / (expect - init).length();
  if (fract < MIN_FRACT) {
    msq_stdio::cerr << "Vertex far from optimimal location" << msq_stdio::endl
                    << "  Expected: (" << expect[0] << ", " << expect[1] << ", " << expect[2] << ")" << msq_stdio::endl
                    << "  Actual:   (" << coords[0] << ", " << coords[1] << ", " << coords[2] << ")" << msq_stdio::endl;
    return FAR_FROM_TARGET;
  }
  
    // check if vertex is at destired location
  const double EPS = 5e-2; // allow a little leway
  if (fabs(coords[0] - expect[0]) > EPS * input_params.w ||
      fabs(coords[1] - expect[1]) > EPS * input_params.h ||
      fabs(expect[2]            ) > EPS                  ) {
    msq_stdio::cerr << "Vertex not at optimimal location" << msq_stdio::endl
                    << "  Expected: (" << expect[0] << ", " << expect[1] << ", " << expect[2] << ")" << msq_stdio::endl
                    << "  Actual:   (" << coords[0] << ", " << coords[1] << ", " << coords[2] << ")" << msq_stdio::endl;
    return NOT_AT_TARGET;
  }
  
  return 0;
}


void parse_mesh_params( const char* argv, const char* arg, MeshParams& result )
{
  int c = msq_std::sscanf( arg, "%lf,%lf,%lf,%lf", &result.x, &result.y, &result.w, &result.h );
  if (c != 2 && c != 4) {
    msq_stdio::cerr << "Error parsing mesh dimensions: \"" << arg << '"' << msq_stdio::endl;
    usage(argv);
  }
}
  

enum ParseState { OPEN, EXPECTING_M, EXPECTING_R, EXPECTING_O, EXPECTING_I };
void parse_options( char* argv[], 
                    int argc,
                    MeshParams& mesh,
                    MeshParams& ref,
                    msq_std::string& output_file,
                    bool& fixed_boundary,
                    TerminationCriterion::TimeStepFileType& write_timesteps,
                    bool& feas_newt_solver,
                    int& num_iterations )
{
    // begin with defaults
  mesh = default_mesh;
  ref  = default_ref;
  output_file = default_out_file;
  fixed_boundary = false;
  write_timesteps = TerminationCriterion::NOTYPE;
  feas_newt_solver = false;
  num_iterations = OUTER_ITERATES;

    // parse CLI args
  ParseState state = OPEN;
  for (int i = 1; i < argc; ++i) {
    switch (state) {
      case EXPECTING_M: 
        parse_mesh_params( argv[0], argv[i], mesh );
        state = OPEN;
        break;
      case EXPECTING_R:
        parse_mesh_params( argv[0], argv[i], ref );
        state = OPEN;
        break;
      case EXPECTING_O:
        output_file = argv[i];
        state = OPEN;
        break;
      case EXPECTING_I:
        num_iterations = atoi( argv[i] );
        state = OPEN;
        break;
      case OPEN:
        if (argv[i][0] != '-' || argv[i][1] == '\0' || argv[i][2] != '\0') {
          msq_stdio::cerr << "Unexpected argument: \"" << argv[i] << '"' << msq_stdio::endl;
          usage(argv[0]);
        }
        
        switch (argv[i][1]) {
          default : usage(argv[0], true );   break;
          case 'h': usage(argv[0], false);   break;
          case 'o': state = EXPECTING_O;     break;
          case 'f': fixed_boundary = true;   break;
          case 'F': fixed_boundary = false;  break;
          case 't': write_timesteps = TerminationCriterion::VTK;  break;
          case 'T': write_timesteps = TerminationCriterion::GNUPLOT; break;
          case 'm': state = EXPECTING_M;     break;
          case 'r': state = EXPECTING_R;     break;
          case 'n': feas_newt_solver = true; break;
          case 'c': feas_newt_solver = false;break;
          case 'i': state = EXPECTING_I;     break;
        }
        break;
    }
  }
}

const char* temp_file = "high_aspect_input.vtk";
void create_input_mesh( const MeshParams& p, 
                        bool all_fixed,
                        MeshImpl& mesh, MsqError& err )
{
  const double z = 0;
  const int F = all_fixed;
  msq_stdio::ofstream vtkfile( temp_file );
  double bx = all_fixed ? 0.5 * p.w : p.x;
  double by = all_fixed ? 0.5 * p.h : p.y;
  vtkfile << "# vtk DataFile Version 3.0" << msq_stdio::endl
          << "Mesquite High Aspect Ratio test" << msq_stdio::endl
          << "ASCII" << msq_stdio::endl
          << "DATASET UNSTRUCTURED_GRID" << msq_stdio::endl
          << "POINTS 9 float" << msq_stdio::endl
          << 0.0 << ' ' << 0.0 << ' ' << z << msq_stdio::endl
          << bx  << ' ' << 0.0 << ' ' << z << msq_stdio::endl
          << p.w << ' ' << 0.0 << ' ' << z << msq_stdio::endl
          << 0.0 << ' ' << by  << ' ' << z << msq_stdio::endl
          << p.x << ' ' << p.y << ' ' << z << msq_stdio::endl
          << p.w << ' ' << by  << ' ' << z << msq_stdio::endl
          << 0.0 << ' ' << p.h << ' ' << z << msq_stdio::endl
          << bx  << ' ' << p.h << ' ' << z << msq_stdio::endl
          << p.w << ' ' << p.h << ' ' << z << msq_stdio::endl
          << "CELLS 4 20" << msq_stdio::endl
          << "4 0 1 4 3" << msq_stdio::endl
          << "4 1 2 5 4" << msq_stdio::endl
          << "4 4 5 8 7" << msq_stdio::endl
          << "4 3 4 7 6" << msq_stdio::endl
          << "CELL_TYPES 4" << msq_stdio::endl
          << "9 9 9 9" << msq_stdio::endl
          << "POINT_DATA 9" << msq_stdio::endl
          << "SCALARS fixed int" << msq_stdio::endl
          << "LOOKUP_TABLE default" << msq_stdio::endl
          << "1 " << F << " 1" << msq_stdio::endl
          <<  F << " 0 " << F  << msq_stdio::endl
          << "1 " << F << " 1" << msq_stdio::endl
          ;
          
  mesh.read_vtk( temp_file, err );
  msq_std::remove( temp_file );
  MSQ_CHKERR(err);
}
  
msq_std::string base_name( msq_std::string filename )
{
  msq_std::string::size_type i = filename.rfind(".");
  if (!i || i == msq_std::string::npos)
    return filename;
  else
    return filename.substr( 0, i );
}

