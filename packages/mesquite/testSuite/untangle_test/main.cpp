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
// -*- Mode : c++; tab-width: 2; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 2 -*-
//
//   SUMMARY: 
//     USAGE:
//
// ORIG-DATE: 19-Feb-02 at 10:57:52
//  LAST-MOD: 23-Jul-03 at 18:08:13 by Thomas Leurent
//
//
// DESCRIPTION:
// ============
/*! \file main.cpp

describe main.cpp here

 */
// DESCRIP-END.
//

#include "MeshImpl.hpp"
#include "MsqTimer.hpp"
#include "Mesquite.hpp"
#include "MsqError.hpp"
#include "Vector3D.hpp"
#include "InstructionQueue.hpp"
#include "MeshSet.hpp"
#include "PatchData.hpp"
#include "TerminationCriterion.hpp"
#include "QualityAssessor.hpp"

// algorythms
#include "Randomize.hpp"
#include "MeanRatioQualityMetric.hpp"
#include "ConditionNumberQualityMetric.hpp"
#include "UntangleBetaQualityMetric.hpp"
#include "LPtoPTemplate.hpp"
#include "LInfTemplate.hpp"
#include "SteepestDescent.hpp"
#include "ConjugateGradient.hpp"

#ifndef MSQ_USE_OLD_IO_HEADERS
#include <iostream>
using std::cout;
using std::endl;
#else
#include <iostream.h>
#endif

#ifdef MSQ_USE_OLD_C_HEADERS
#include <cstdlib>
#else
#include <stdlib.h>
#endif


using namespace Mesquite;


int main()
{
  Mesquite::MeshImpl *mesh = new Mesquite::MeshImpl;
  MsqError err;
  mesh->read_vtk("../../meshFiles/2D/VTK/tangled_quad.vtk", err);
  if (err) return 1;
  
    // initialises a MeshSet object
  MeshSet mesh_set1;
  mesh_set1.add_mesh(mesh, err); 
  if (err) return 1;
  
    // creates an intruction queue
  InstructionQueue queue1;
  
    // creates a mean ratio quality metric ...
  ShapeQualityMetric* shape_metric = new ConditionNumberQualityMetric;
  UntangleQualityMetric* untangle = new UntangleBetaQualityMetric(2);
  Randomize* pass0 = new Randomize(.05);
    // ... and builds an objective function with it
    //LInfTemplate* obj_func = new LInfTemplate(shape_metric);
  LInfTemplate* obj_func = new LInfTemplate(untangle);
  LPtoPTemplate* obj_func2 = new LPtoPTemplate(shape_metric, 2, err);
  if (err) return 1;
    // creates the steepest descent optimization procedures
  obj_func2->set_gradient_type(ObjectiveFunction::ANALYTICAL_GRADIENT);
  ConjugateGradient* pass1 = new ConjugateGradient( obj_func, err );
  if (err) return 1;
  
    //SteepestDescent* pass2 = new SteepestDescent( obj_func2 );
  ConjugateGradient* pass2 = new ConjugateGradient( obj_func2, err );
  if (err) return 1;
  pass2->set_patch_type(PatchData::ELEMENTS_ON_VERTEX_PATCH,err,1,1);
  if (err) return 1;
  pass2->set_patch_type(PatchData::GLOBAL_PATCH,err);
  if (err) return 1;
  QualityAssessor stop_qa=QualityAssessor(shape_metric,QualityAssessor::MAXIMUM);
  QualityAssessor stop_qa2=QualityAssessor(shape_metric,QualityAssessor::MAXIMUM);
  stop_qa2.add_quality_assessment(shape_metric,QualityAssessor::AVERAGE,err);
  if (err) return 1;
  
  stop_qa.add_quality_assessment(untangle,QualityAssessor::ALL_MEASURES,err);
  if (err) return 1;
  stop_qa.set_stopping_assessment(untangle,QualityAssessor::MAXIMUM,err);
  if (err) return 1;
    // **************Set stopping criterion**************
    //untangle beta should be 0 when untangled
  TerminationCriterion sc1;
  sc1.add_criterion_type_with_double(TerminationCriterion::QUALITY_IMPROVEMENT_RELATIVE,
                                     .000001,err);
  if (err) return 1;
  TerminationCriterion sc3;
  sc3.add_criterion_type_with_int(TerminationCriterion::NUMBER_OF_ITERATES,10,
                                  err);
  if (err) return 1;
  TerminationCriterion sc_rand;
  sc_rand.add_criterion_type_with_int(TerminationCriterion::NUMBER_OF_ITERATES,1,
                                      err);
  if (err) return 1;
  
    //StoppingCriterion sc1(&stop_qa,-1.0,.0000001);
    //StoppingCriterion sc3(&stop_qa2,.9,1.00000001);
    //StoppingCriterion sc2(StoppingCriterion::NUMBER_OF_PASSES,10);
    //StoppingCriterion sc_rand(StoppingCriterion::NUMBER_OF_PASSES,1);
    //either until untangled or 10 iterations
  pass0->set_outer_termination_criterion(&sc_rand);
  pass1->set_outer_termination_criterion(&sc1);
  pass2->set_inner_termination_criterion(&sc3);
  
    // sets a culling method on the first QualityImprover
  pass0->add_culling_method(PatchData::NO_BOUNDARY_VTX);
  pass1->add_culling_method(PatchData::NO_BOUNDARY_VTX);
  pass2->add_culling_method(PatchData::NO_BOUNDARY_VTX);
    // adds 1 pass of pass1 to mesh_set1
  queue1.add_quality_assessor(&stop_qa,err); 
  if (err) return 1;
    //queue1.add_preconditioner(pass0,err);MSQ_CHKERR(err);
    //queue1.add_preconditioner(pass1,err);MSQ_CHKERR(err);
    //queue1.set_master_quality_improver(pass2, err); MSQ_CHKERR(err);
  queue1.set_master_quality_improver(pass1, err);
  if (err) return 1;
  queue1.add_quality_assessor(&stop_qa2,err);
  if (err) return 1;
  mesh->write_vtk("original_mesh", err);
  if (err) return 1;
  
    // launches optimization on mesh_set1
  queue1.run_instructions(mesh_set1, err);
  if (err) return 1;
  
  mesh->write_vtk("smoothed_mesh", err); 
  if (err) return 1;
  
  print_timing_diagnostics(cout);
  return 0;
}
