// -*- Mode : c++; tab-width: 3; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 3 -*-
//
//   SUMMARY: 
//     USAGE:
//
//    AUTHOR: Michael Brewer
//       ORG: Sandia National Labs
//    E-MAIL: mbrewer@sandia.gov
//
// ORIG-DATE: Jan. 29, 2003
//  LAST-MOD: 
//
// DESCRIPTION:
// ============
/*! \file PlanarGeometryTest.cpp

Regression testing using the planar geometry capabilities in
SimplifiedGeometryEngine.
 */
// DESCRIP-END.
//

#include "PatchDataInstances.hpp"
#include "cppunit/extensions/HelperMacros.h"
#include "cppunit/SignalException.h"
#include <math.h>

#include "Mesquite.hpp"
#include "MesquiteError.hpp"
#include "Vector3D.hpp"
#include "InstructionQueue.hpp"
#include "MeshSet.hpp"
#include "PatchData.hpp"
#include "StoppingCriterion.hpp"
#include "QualityAssessor.hpp"

#include "InverseMeanRatioQualityMetric.hpp"
#include "GeneralizedConditionNumberQualityMetric.hpp"
#include "MeanRatioQualityMetric.hpp"
#include "ConditionNumberQualityMetric.hpp"
#include "LPTemplate.hpp"
#include "ASMQualityMetric.hpp"
#include "EdgeLengthQualityMetric.hpp"
#include "LaplacianSmoother.hpp"
#include "LInfTemplate.hpp"
#include "SteepestDescent.hpp"
#include "ConjugateGradient.hpp"
#include "AspectRatioGammaQualityMetric.hpp"
#include "UntangleBetaQualityMetric.hpp"
#include "MultiplyQualityMetric.hpp"
#include "SimplifiedGeometryEngine.hpp"


using namespace Mesquite;

class PlanarGeometryTest : public CppUnit::TestFixture
{
private:
  CPPUNIT_TEST_SUITE(PlanarGeometryTest);
    //run steepest descent on the tangled_tri.vtk mesh
  CPPUNIT_TEST (test_plane_tri_tangled);
    //run cg on tangled_quad.vtk mesh
  CPPUNIT_TEST (test_plane_quad_tangled);
  
  CPPUNIT_TEST_SUITE_END();
  
private:
  double qualTol;//double used for double comparisons
  int pF;//PRINT_FLAG
public:
  void setUp()
  {
      //pF=1;//PRINT_FLAG IS ON
      pF=0;//PRINT_FLAG IS OFF
        //tolerance double
      qualTol=MSQ_MIN;
  }

  void tearDown()
  {
  }
  
public:
  PlanarGeometryTest()
    {}
  
   void test_plane_tri_tangled()
   {
     char file_name[128];
       /* Reads a TSTT Mesh file */
     TSTT::Mesh_Handle mesh;
     TSTT::MeshError tstt_err;
     TSTT::Mesh_Create(&mesh, &tstt_err);
     strcpy(file_name, "../../meshFiles/2D/VTK/tangled_tri.vtk");
     TSTT::Mesh_Load(mesh, file_name, &tstt_err);
       // Mesquite error object
     MsqError err;
     
       // initialises a MeshSet object
     MeshSet mesh_set1;
     mesh_set1.add_mesh(mesh, err); MSQ_CHKERR(err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
       //create geometry: plane z=5, normal (0,0,1)
     Vector3D pnt(0,0,5);
     Vector3D s_norm(0,0,1);
     SimplifiedGeometryEngine msq_geom;
     msq_geom.set_geometry_to_plane(s_norm,pnt,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     mesh_set1.set_simplified_geometry_engine(&msq_geom);
     
       // creates an intruction queue
     InstructionQueue queue1, queue2;

       // creates a mean ratio quality metric ...
     ShapeQualityMetric* shape = ConditionNumberQualityMetric::create_new();
     UntangleQualityMetric* untan = UntangleBetaQualityMetric::create_new(.1);
  
       // ... and builds an objective function with it (untangle)
     LInfTemplate* untan_func = new LInfTemplate(untan);
     LPTemplate* shape_func = new LPTemplate(shape,2,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     shape_func->set_gradient_type(ObjectiveFunction::ANALYTICAL_GRADIENT);
       // creates the steepest descent optimization procedures
     SteepestDescent* pass1 = new SteepestDescent( untan_func );
     SteepestDescent* pass2 = new SteepestDescent( shape_func );
     pass1->set_patch_type(PatchData::ELEMENTS_ON_VERTEX_PATCH, err,1 ,1);
     pass2->set_patch_type(PatchData::GLOBAL_PATCH, err,1 ,1);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     QualityAssessor stop_qa=QualityAssessor(untan,QualityAssessor::MAXIMUM);
     QualityAssessor qa=QualityAssessor(shape,QualityAssessor::MAXIMUM);
     if(pF==0){
       stop_qa.disable_printing_results();
       qa.disable_printing_results();
     }  
       //**********Set stopping criterion  untangle ver small ********
     StoppingCriterion sc_qa(&stop_qa,-100,MSQ_MIN);
     pass1->set_stopping_criterion(&sc_qa);
       //**********Set stopping criterion  5 iterates ****************
     StoppingCriterion sc5(StoppingCriterion::NUMBER_OF_PASSES,5);
     pass2->set_stopping_criterion(&sc5);
       // sets a culling method on the first QualityImprover
     pass1->add_culling_method(PatchData::NO_BOUNDARY_VTX);
     pass2->add_culling_method(PatchData::NO_BOUNDARY_VTX);
     pass2->set_maximum_iteration(5);
  
     queue1.set_master_quality_improver(pass1, err); MSQ_CHKERR(err);
     queue2.set_master_quality_improver(pass2, err); MSQ_CHKERR(err);
       //********************UNTANGLE*******************************
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
       // launches optimization on mesh_set1
     double orig_qa_val=stop_qa.assess_mesh_quality(mesh_set1,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     queue1.run_instructions(mesh_set1, err); MSQ_CHKERR(err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     double fin_qa_val=stop_qa.assess_mesh_quality(mesh_set1,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
       //make sure 'quality' improved
     CPPUNIT_ASSERT( (fin_qa_val-orig_qa_val) <= 0.0 );
       //make sure sc_qa really was satisfied
     CPPUNIT_ASSERT( fin_qa_val <= MSQ_MIN );

      //********************SMOOTH*******************************
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
       // launches optimization on mesh_set1
     orig_qa_val=qa.assess_mesh_quality(mesh_set1,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     queue2.run_instructions(mesh_set1, err); MSQ_CHKERR(err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     fin_qa_val=qa.assess_mesh_quality(mesh_set1,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
       //make sure 'quality' improved
     CPPUNIT_ASSERT( (fin_qa_val-orig_qa_val) <= 0.0 );
     
   }
  
  void test_plane_quad_tangled()
     {
             char file_name[128];
       /* Reads a TSTT Mesh file */
     TSTT::Mesh_Handle mesh;
     TSTT::MeshError tstt_err;
     TSTT::Mesh_Create(&mesh, &tstt_err);
     strcpy(file_name, "../../meshFiles/2D/VTK/tangled_quad.vtk");
     TSTT::Mesh_Load(mesh, file_name, &tstt_err);
       // Mesquite error object
     MsqError err;
     
       // initialises a MeshSet object
     MeshSet mesh_set1;
     mesh_set1.add_mesh(mesh, err); MSQ_CHKERR(err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
       //create geometry: plane z=5, normal (0,0,1)
     Vector3D pnt(0,0,5);
     Vector3D s_norm(0,0,1);
     SimplifiedGeometryEngine msq_geom;
     msq_geom.set_geometry_to_plane(s_norm,pnt,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     mesh_set1.set_simplified_geometry_engine(&msq_geom);
     
       // creates an intruction queue
     InstructionQueue queue1, queue2;

       // creates a mean ratio quality metric ...
     ShapeQualityMetric* shape = ConditionNumberQualityMetric::create_new();
     UntangleQualityMetric* untan = UntangleBetaQualityMetric::create_new(.1);
  
       // ... and builds an objective function with it (untangle)
     LInfTemplate* untan_func = new LInfTemplate(untan);
     LPTemplate* shape_func = new LPTemplate(shape,2,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     shape_func->set_gradient_type(ObjectiveFunction::ANALYTICAL_GRADIENT);
       // creates the cg optimization procedures
     ConjugateGradient* pass1 = new ConjugateGradient( untan_func );
     ConjugateGradient* pass2 = new ConjugateGradient( shape_func );
     pass1->set_patch_type(PatchData::ELEMENTS_ON_VERTEX_PATCH, err,1 ,1);
     pass2->set_patch_type(PatchData::GLOBAL_PATCH, err,1 ,1);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     QualityAssessor stop_qa=QualityAssessor(untan,QualityAssessor::MAXIMUM);
     QualityAssessor qa=QualityAssessor(shape,QualityAssessor::MAXIMUM);
       //turn off printing if print flag not set.
     if(pF==0){
       stop_qa.disable_printing_results();
       qa.disable_printing_results();
     }  
       //**********Set stopping criterion  untangle ver small ********
     StoppingCriterion sc_qa(&stop_qa,-100,MSQ_MIN);
     pass1->set_stopping_criterion(&sc_qa);
       //**********Set stopping criterion  5 iterates ****************
     StoppingCriterion sc5(StoppingCriterion::NUMBER_OF_PASSES,5);
     pass2->set_stopping_criterion(&sc5);
       // sets a culling method on the first QualityImprover
     pass1->add_culling_method(PatchData::NO_BOUNDARY_VTX);
     pass2->add_culling_method(PatchData::NO_BOUNDARY_VTX);
     pass2->set_maximum_iteration(5);
  
     queue1.set_master_quality_improver(pass1, err); MSQ_CHKERR(err);
     queue2.set_master_quality_improver(pass2, err); MSQ_CHKERR(err);
       //********************UNTANGLE*******************************
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
       // launches optimization on mesh_set1
     double orig_qa_val=stop_qa.assess_mesh_quality(mesh_set1,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     queue1.run_instructions(mesh_set1, err); MSQ_CHKERR(err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     double fin_qa_val=stop_qa.assess_mesh_quality(mesh_set1,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
       //make sure 'quality' improved
     CPPUNIT_ASSERT( (fin_qa_val-orig_qa_val) <= 0.0 );
       //make sure sc_qa really was satisfied
     CPPUNIT_ASSERT( fin_qa_val <= MSQ_MIN );

      //********************SMOOTH*******************************
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
       // launches optimization on mesh_set1
     orig_qa_val=qa.assess_mesh_quality(mesh_set1,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     queue2.run_instructions(mesh_set1, err); MSQ_CHKERR(err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
     fin_qa_val=qa.assess_mesh_quality(mesh_set1,err);
       //Make sure no errors
     CPPUNIT_ASSERT(!err.errorOn);
       //make sure 'quality' improved
     CPPUNIT_ASSERT( (fin_qa_val-orig_qa_val) <= 0.0 );
     }
  
   
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(PlanarGeometryTest, "PlanarGeometryTest");
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(PlanarGeometryTest, "Regression");
