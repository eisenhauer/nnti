// -*- Mode : c++; tab-width: 3; c-tab-always-indent: t; indent-tabs-mode: nil; c-basic-offset: 3 -*-
//
//   SUMMARY: 
//     USAGE:
//
//    AUTHOR: Michael Brewer
//       ORG: Sandia National Labs
//    E-MAIL: mbrewer@sandia.gov
//
// ORIG-DATE: 03-Dec-02
//  LAST-MOD: 29-Jan-03 at 11:21:23 by Thomas Leurent
//
// DESCRIPTION:
// ============
/*! \file QualityMetricTest.cpp

Unit testing of various QualityMetrics primarily to test for
correct metric return values. 

 */
// DESCRIP-END.
//

#include "Mesquite.hpp"
#include "PatchData.hpp"
#include "PatchDataInstances.hpp"
#include "QualityMetric.hpp"
#include "ConditionNumberQualityMetric.hpp"
#include "GeneralizedConditionNumberQualityMetric.hpp"
#include "MeanRatioQualityMetric.hpp"
#include "InverseMeanRatioQualityMetric.hpp"
#include "AspectRatioGammaQualityMetric.hpp"
#include "MultiplyQualityMetric.hpp"
#include "EdgeLengthQualityMetric.hpp"
#include "MsqMessage.hpp"
#include "cppunit/extensions/HelperMacros.h"
#include "cppunit/SignalException.h"
#include "ASMQualityMetric.hpp"
#include "SmoothnessQualityMetric.hpp"
#include <math.h>

using namespace Mesquite;

class QualityMetricTest : public CppUnit::TestFixture
{
private:
  CPPUNIT_TEST_SUITE(QualityMetricTest);
    //Test condition number and generalized condition number metrics
  CPPUNIT_TEST (test_condition_number);
    //Test mean ratio and inverse mean ratio metrics
  CPPUNIT_TEST (test_mean_ratio);
    //Test apect ratio gamma (Tri's and Tet's only)
  CPPUNIT_TEST (test_aspect_ratio_gamma);
    //Test composite multiply
  CPPUNIT_TEST (test_composite_multiply);
    //Test edge length metric
  CPPUNIT_TEST (test_edge_length_metric);  
    //Test averaging methods
  CPPUNIT_TEST (test_averaging_method);
  CPPUNIT_TEST (test_mean_ratio_hex_gradient);
  CPPUNIT_TEST (test_mean_ratio_tet_gradient);
  CPPUNIT_TEST (test_mean_ratio_hessian);
    //Test ASM (area smoothness quality metric)
  CPPUNIT_TEST (test_asm);  
  
  CPPUNIT_TEST_SUITE_END();
  
private:
  
  PatchData triPatch;
  PatchData quadPatch;
  PatchData tetPatch;
  PatchData hexPatch;
    //Tol used for double comparisons
  double qualTol;
  int pF;//PRINT_FLAG
public:
  void setUp()
  {
      //pF=1;//PRINT_FLAG IS ON
    pF=0;//PRINT_FLAG IS OFF
    MsqError err;
    
    qualTol = MSQ_MIN;
    size_t ind[20];
    size_t elem_ind[8];
    
     /* Our triangular patch is made of two tris.  tri_1 is a perfect
        equilateral (the ideal for most metrics).  tri_2 is an arbitrary
        triangle.
     */
    create_qm_two_tri_patch(triPatch, err);MSQ_CHKERR(err);
     /* Our quad patch is made of two quads.  quad_1 is a perfect
        square (the ideal for most metrics).  quad_2 is an arbitrary
        quad.
     */
    create_qm_two_quad_patch(quadPatch,err);MSQ_CHKERR(err);
     /* Our tet patch is made of two tets.  tet_1 is a perfect
        equilateral (the ideal for most metrics).  tet_2 is an arbitrary
        tet.
     */
    create_qm_two_tet_patch(tetPatch,err);MSQ_CHKERR(err);
    
     /* Our hex patch is made of two hexes.  hex_1 is a perfect
        unit cube (the ideal for most metrics).  hex_2 is an arbitrary
        hex.
     */
     create_qm_two_hex_patch(hexPatch,err);MSQ_CHKERR(err);
  }

  void tearDown()
  {
  }
  
public:
  QualityMetricTest()
    {}
  
   void test_condition_number()
   {
     bool v_flag;
       //START WITH TRI's
     MsqError err;
     double val, val2;
     MsqMeshEntity* elems;
     MsqVertex* verts = triPatch.get_vertex_array(err);
     elems=triPatch.get_element_array(err);
     MSQ_CHKERR(err);
     ShapeQualityMetric *met = ConditionNumberQualityMetric::create_new();
     ShapeQualityMetric *gmet = GeneralizedConditionNumberQualityMetric::create_new();
       //Check condition number of ideal tri
     met->evaluate_element(triPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
       //For now, make sure cond num and generalized cond num give
       //equivalent answer for arbitrary tri.
     met->evaluate_element(triPatch,&elems[1],val,err); MSQ_CHKERR(err);
     gmet->evaluate_element(triPatch,&elems[1],val2,err); MSQ_CHKERR(err);
     val -= val2;
     if(pF)
       PRINT_INFO("\nGEN TRI %f", val2);
     
     CPPUNIT_ASSERT(fabs(val)<qualTol);
     
       //SECOND: QUAD's
     verts = quadPatch.get_vertex_array(err);
     elems = quadPatch.get_element_array(err);
       //Check condition number of ideal quad
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
       //For now, make sure cond num and generalized cond num give
       //equivalent answer for arbitrary quad.
     met->evaluate_element(quadPatch,&elems[1],val,err); MSQ_CHKERR(err);
     gmet->evaluate_element(quadPatch,&elems[1],val2,err); MSQ_CHKERR(err);
     
     val -= val2;
     if(pF)
       PRINT_INFO("\nGEN QUA %f", val2);
     
     CPPUNIT_ASSERT(fabs(val)<qualTol);

       //THIRD TET's
     verts = tetPatch.get_vertex_array(err);
     elems = tetPatch.get_element_array(err);
       //Check condition number of ideal tet
     val = met->evaluate_element(tetPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
       //For now, make sure cond num and generalized cond num give
       //equivalent answer for arbitrary tet.
     met->evaluate_element(tetPatch,&elems[1],val,err); MSQ_CHKERR(err);
     v_flag=gmet->evaluate_element(tetPatch,&elems[1],val2,err); MSQ_CHKERR(err);
     CPPUNIT_ASSERT(v_flag==true);
     
     val -= val2;
     if(pF)
       PRINT_INFO("\nGEN TET %f", val2);
     
       //CPPUNIT_ASSERT(fabs(val)<qualTol);

       //FOURTH HEX's
     verts = hexPatch.get_vertex_array(err);
     elems = hexPatch.get_element_array(err);
       //Check condition number of ideal hex
     met->evaluate_element(hexPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
       //For now, make sure cond num and generalized cond num give
       //equivalent answer for arbitrary tet.
     v_flag=met->evaluate_element(hexPatch,&elems[1],val,err); MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nCON HEX %f", val);
     CPPUNIT_ASSERT(v_flag==true);
     
     gmet->evaluate_element(hexPatch,&elems[1],val2,err); MSQ_CHKERR(err);
     val -= val2;
     if(pF)
       PRINT_INFO("\nGEN HEX %f", val2);
     
     CPPUNIT_ASSERT(fabs(val)<qualTol);
   }


   void test_mean_ratio()
   {
       //START WITH TRI's
     MsqError err;
     double val;
     MsqMeshEntity* elems;
     MsqVertex* verts = triPatch.get_vertex_array(err);
     elems=triPatch.get_element_array(err);
     MSQ_CHKERR(err);
     ShapeQualityMetric *met = MeanRatioQualityMetric::create_new();
     ShapeQualityMetric *imet = InverseMeanRatioQualityMetric::create_new();
       //Check mean ratio of ideal tri
     met->evaluate_element(triPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nMEAN TRI %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
       //Check inverse mean ratio of ideal tri (INVERSE)
     imet->evaluate_element(triPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nInv MEAN TRI %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
     
       //SECOND: QUAD's
     verts = quadPatch.get_vertex_array(err);
     elems = quadPatch.get_element_array(err);
       //Check mean ratio of ideal quad
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nMEAN QUAD %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
       //Check inverse mean ratio of ideal quad (INVERSE)
     imet->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nInv MEAN QUAD %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);

       //THIRD TET's
     verts = tetPatch.get_vertex_array(err);
     elems = tetPatch.get_element_array(err);
       //Check mean ratio of ideal tet
     met->evaluate_element(tetPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nMEAN TET %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
     //Check inverse mean ratio of ideal tet (INVERSE)
     imet->evaluate_element(tetPatch,&elems[0],val,err); MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nInv MEAN TET %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);

       //FOURTH HEX's
     verts = hexPatch.get_vertex_array(err);
     elems = hexPatch.get_element_array(err);
       //Check mean ratio of ideal hex
     met->evaluate_element(hexPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nMEAN HEX %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
       //Check inverse mean ratio of ideal hex (INVERSE)
     imet->evaluate_element(hexPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nInv MEAN HEX %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
   }
  
  void test_aspect_ratio_gamma()
   {
       //START WITH TRI's
     MsqError err;
     double val;
     MsqMeshEntity* elems;
     MsqVertex* verts = triPatch.get_vertex_array(err);
     elems=triPatch.get_element_array(err);
     MSQ_CHKERR(err);
     ShapeQualityMetric *met = AspectRatioGammaQualityMetric::create_new();
       //Check aspect ratio gamma of ideal tri
     met->evaluate_element(triPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
     
       //THIRD TET's
     verts = tetPatch.get_vertex_array(err);
     elems = tetPatch.get_element_array(err);
       //Check aspect ratio gamma of ideal tet
     met->evaluate_element(tetPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);

   }

  void test_composite_multiply()
   {
       //START WITH TRI's
     MsqError err;
     double val;
     MsqMeshEntity* elems;
     MsqVertex* verts = triPatch.get_vertex_array(err);
     elems=triPatch.get_element_array(err);
     MSQ_CHKERR(err);
     ShapeQualityMetric *mmet = MeanRatioQualityMetric::create_new();
     ShapeQualityMetric *cmet = ConditionNumberQualityMetric::create_new();
     CompositeQualityMetric *met = MultiplyQualityMetric::create_new(mmet,
                                                                     cmet,
                                                                     err);
       //Check ideal tri
     met->evaluate_element(triPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nMULT TRI %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);

       //SECOND: QUAD's
     verts = quadPatch.get_vertex_array(err);
     elems = quadPatch.get_element_array(err);
       //Check ideal quad
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nMULT QUAD %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);

       //THIRD TET's
     verts = tetPatch.get_vertex_array(err);
     elems = tetPatch.get_element_array(err);
       //Check ideal tet
     met->evaluate_element(tetPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nMULT TET %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);

       //FOURTH HEX's
     verts = hexPatch.get_vertex_array(err);
     elems = hexPatch.get_element_array(err);
       //Check ideal hex
     met->evaluate_element(hexPatch,&elems[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nMULT HEX %f", val);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
   }

  void test_edge_length_metric()
   {
       //START WITH TRI's
     MsqError err;
     double val;
     MsqMeshEntity* elems;
     MsqVertex* verts = triPatch.get_vertex_array(err);
     elems=triPatch.get_element_array(err);
     MSQ_CHKERR(err);
     SmoothnessQualityMetric *met = EdgeLengthQualityMetric::create_new();
       //Check aspect ratio gamma of ideal tri patch
       //Vert[2] has two edges connected of length 1
     met->evaluate_vertex(triPatch,&verts[2],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nEdge Length Metric tris (should be 2) %f", val);
     CPPUNIT_ASSERT(fabs(val-2.0)<qualTol);
       //Vert[1] has two edges connected of length 1
       //THIRD TET's
     verts = tetPatch.get_vertex_array(err);
     elems = tetPatch.get_element_array(err);
       //Check aspect ratio gamma of ideal tet
     met->evaluate_vertex(tetPatch,&verts[0],val,err);MSQ_CHKERR(err);
     if(pF)
       PRINT_INFO("\nEdge Length Metric tets (should be 3) %f", val);
     CPPUNIT_ASSERT(fabs(val-3.0)<qualTol);
     
   }
  
  void test_asm()
     {
       QualityMetric *met = ASMQualityMetric::create_new();
       MsqError err;
         //Test the metric on a single elemnt patch
       PatchData p1;
       create_one_tri_patch(p1, err);
       MsqMeshEntity* elem1=p1.get_element_array(err);
       double first_val;
       bool first_bool=met->evaluate_element(p1,&elem1[0],first_val,err);
         //Any non-connected element should have asm of 0.0
       CPPUNIT_ASSERT_DOUBLES_EQUAL(first_val, 0.0, MSQ_MIN);
       CPPUNIT_ASSERT(first_bool==true);
         //Test on a patch with two ideal tris
       PatchData p2;
       create_two_tri_patch(p2, err);
       MsqMeshEntity* elem2=p2.get_element_array(err);
       double second_val;
       bool second_bool=met->evaluate_element(p2,&elem2[0],second_val,err);
         //Two neighboring tris with equal area should have asm of 0.0
       CPPUNIT_ASSERT_DOUBLES_EQUAL(second_val, 0.0, MSQ_MIN);
       CPPUNIT_ASSERT(second_bool==true);

        //Test on a patch with two tris one not ideal
       PatchData p3;
       create_qm_two_tri_patch(p3, err);
       MsqMeshEntity* elem3=p3.get_element_array(err);
       double third_val;
       bool third_bool=met->evaluate_element(p3,&elem3[0],third_val,err);
         //Two neighboring tris with equal area should have asm of 0.0
       CPPUNIT_ASSERT(third_val>0.0);
       CPPUNIT_ASSERT(third_val<1.0);
       CPPUNIT_ASSERT(third_bool==true);

       
     }
  
  void test_averaging_method()
     {
         //USE QUAD's
     MsqError err;
     double val;
     MsqMeshEntity* elems;
     MsqVertex* verts = quadPatch.get_vertex_array(err);
     elems=quadPatch.get_element_array(err);
     MSQ_CHKERR(err);
     ShapeQualityMetric *met = MeanRatioQualityMetric::create_new();
       //Check mean ratio of ideal quad
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
     met->set_averaging_method(QualityMetric::GEOMETRIC, err);
       //Check mean ratio of ideal quad GEOMETRIC
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
     met->set_averaging_method(QualityMetric::HARMONIC, err);
       //Check mean ratio of ideal quad HARMONIC
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
     met->set_averaging_method(QualityMetric::LINEAR, err);
       //Check mean ratio of ideal quad LINEAR
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
     met->set_averaging_method(QualityMetric::MAXIMUM, err);
       //Check mean ratio of ideal quad MAXIMUM
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
     met->set_averaging_method(QualityMetric::MINIMUM, err);
       //Check mean ratio of ideal quad MINIMUM
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
     met->set_averaging_method(QualityMetric::RMS, err);
       //Check mean ratio of ideal quad RMS
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-1.0)<qualTol);
     met->set_averaging_method(QualityMetric::SUM, err);
       //Check mean ratio of ideal SUM (NOTICE:: should be 4.0)
     met->evaluate_element(quadPatch,&elems[0],val,err);MSQ_CHKERR(err);
     CPPUNIT_ASSERT(fabs(val-4.0)<qualTol);
   }

  void test_mean_ratio_gradient(PatchData &pd)
  {
    MsqError err;
    Vector3D* grad_num = new Vector3D[2];
    Vector3D* grad_ana = new Vector3D[2];
    double metric_value;

    MsqMeshEntity* elems = pd.get_element_array(err);MSQ_CHKERR(err);
    MsqVertex* vertices =  pd.get_vertex_array(err);MSQ_CHKERR(err);

    std::vector<size_t> bad_elem_vertex_indices;
    elems[1].get_vertex_indices(bad_elem_vertex_indices);
    MsqVertex* two_vtces[2];
    two_vtces[0] = &vertices[bad_elem_vertex_indices[0]];
    two_vtces[1] = &vertices[bad_elem_vertex_indices[2]];
    
    // creates a mean ratio quality metric ...
    ShapeQualityMetric* mean_ratio = MeanRatioQualityMetric::create_new();
    mean_ratio->set_averaging_method(QualityMetric::SUM, err);

    mean_ratio->set_gradient_type(QualityMetric::NUMERICAL_GRADIENT);
    mean_ratio->compute_element_gradient (pd, &elems[1], two_vtces,
                                          grad_num, 2, metric_value, err); MSQ_CHKERR(err);

//     std::cout << "NUMERICAL GRADIENT\n";
//     for (int i=0; i<2; ++i)
//        for (int j=0; j<3; ++j)
//          std::cout << grad_num[i][j] << std::endl;

    mean_ratio->set_gradient_type(QualityMetric::ANALYTICAL_GRADIENT);
    mean_ratio->compute_element_gradient (pd, &elems[1], two_vtces,
                                          grad_ana, 2, metric_value, err); MSQ_CHKERR(err);
    
//     std::cout << "ANALYTICAL GRADIENT\n";
//     for (int i=0; i<2; ++i)
//       for (int j=0; j<3; ++j)
//         std::cout << grad_ana[i][j] << std::endl;

    for (int i=0; i<2; ++i)
      for (int j=0; j<3; ++j)
        CPPUNIT_ASSERT_DOUBLES_EQUAL(grad_num[i][j], grad_ana[i][j], 0.001);
    
    delete grad_num;
    delete grad_ana;
  }

  void test_mean_ratio_hex_gradient()
  {
    test_mean_ratio_gradient(hexPatch);
  }
       
  void test_mean_ratio_tet_gradient()
  {
    test_mean_ratio_gradient(tetPatch);
  }
       
  void test_mean_ratio_hessian()
  {
    MsqError err;
    Vector3D* grad = new Vector3D[4];
    Matrix3D* hessian_num = new Matrix3D[10];
    Matrix3D* hessian_ana = new Matrix3D[10];
    double metric_value;

    MsqMeshEntity* elems = tetPatch.get_element_array(err);MSQ_CHKERR(err);
    MsqVertex* vertices =  tetPatch.get_vertex_array(err);MSQ_CHKERR(err);

    std::vector<size_t> bad_elem_vertex_indices;
    elems[1].get_vertex_indices(bad_elem_vertex_indices);
    MsqVertex* all_vtces[4];
    all_vtces[0] = &vertices[bad_elem_vertex_indices[0]];
    all_vtces[1] = &vertices[bad_elem_vertex_indices[1]];
    all_vtces[2] = &vertices[bad_elem_vertex_indices[2]];
    all_vtces[3] = &vertices[bad_elem_vertex_indices[3]];
    
    // creates a mean ratio quality metric ...
    ShapeQualityMetric* mean_ratio = MeanRatioQualityMetric::create_new();
//    mean_ratio->set_gradient_type(QualityMetric::NUMERICAL_GRADIENT);
    mean_ratio->set_gradient_type(QualityMetric::ANALYTICAL_GRADIENT);
    mean_ratio->set_averaging_method(QualityMetric::SUM, err); MSQ_CHKERR(err);

    mean_ratio->set_hessian_type(QualityMetric::NUMERICAL_HESSIAN);
    mean_ratio->compute_element_hessian(tetPatch, &elems[1], all_vtces,
                                        grad, hessian_num, 4, metric_value,
                                        err); MSQ_CHKERR(err);

//     std::cout << "GRADIENT\n";
//     for (int i=0; i<4; ++i)
//       for (int j=0; j<3; ++j)
//         std::cout << grad[i][j] << std::endl;

//     std::cout << "NUMERICAL HESSIAN\n";
//     for (int i=0; i<10; ++i)
//          std::cout << hessian_num[i] << std::endl;

    mean_ratio->set_hessian_type(QualityMetric::ANALYTICAL_HESSIAN);
    mean_ratio->compute_element_hessian(tetPatch, &elems[1], all_vtces,
                                        grad, hessian_ana, 4, metric_value,
                                        err); MSQ_CHKERR(err);

//     std::cout << "ANALYTICAL HESSIAN\n";
//     for (int i=0; i<10; ++i)
//         std::cout << hessian_ana[i] << std::endl;

    for (int m=0; m<10; ++m)
      for (int i=0; i<3; ++i)
        for (int j=0; j<3; ++j)
          CPPUNIT_ASSERT_DOUBLES_EQUAL(hessian_num[m][i][j], hessian_ana[m][i][j], 0.001);
    
    delete[] grad;
    delete[] hessian_num;
    delete[] hessian_ana;
  }
       
   
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(QualityMetricTest, "QualityMetricTest");

