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
//
//   SUMMARY: 
//     USAGE:
//
//    AUTHOR: Thomas Leurent <tleurent@mcs.anl.gov>
//       ORG: Argonne National Laboratory
//    E-MAIL: tleurent@mcs.anl.gov
//
// ORIG-DATE: 13-Nov-02 at 18:05:56
//  LAST-MOD: 23-Jul-03 at 17:33:18 by Thomas Leurent
//
// DESCRIPTION:
// ============
/*! \file InstructionQueueTest.cpp

Unit testing of various functions in the InstructionQueue class. 

 */
// DESCRIP-END.
//



#include "Mesquite.hpp"
#include "InstructionQueue.hpp"
#include "QualityAssessor.hpp"
#include "QualityImprover.hpp"
#include "ShapeQualityMetric.hpp"
#include "MeanRatioQualityMetric.hpp"
#include "LPtoPTemplate.hpp"
#include "SteepestDescent.hpp"
#include "Vector3D.hpp"
#include "PatchData.hpp"

#include "cppunit/extensions/HelperMacros.h"

#ifdef MSQ_USE_OLD_IO_HEADERS
#include <iostream.h>
#else 
#include <iostream>
using std::cout;
using std::endl;
#endif

using namespace Mesquite;

class InstructionQueueTest : public CppUnit::TestFixture
{
private:
   CPPUNIT_TEST_SUITE(InstructionQueueTest);
   CPPUNIT_TEST (test_add_preconditionner);
   CPPUNIT_TEST (test_remove_preconditioner);
   CPPUNIT_TEST (test_insert_preconditioner);
   CPPUNIT_TEST (test_add_quality_assessor);
   CPPUNIT_TEST (test_remove_quality_assessor);
   CPPUNIT_TEST (test_insert_quality_assessor);
   CPPUNIT_TEST_SUITE_END();
   
private:
   QualityAssessor* mQA;
   QualityImprover* mQI;
   ShapeQualityMetric* mQM;
   ObjectiveFunction* mOF;
   InstructionQueue mQueue;

public:
  void setUp()
  {
     MsqPrintError err(cout);
     // creates a quality assessor and a qualilty improver
     mQM = new MeanRatioQualityMetric(err);
     mOF = new LPtoPTemplate(mQM, 2, err);
     mQI = new SteepestDescent( mOF );
     mQA = new QualityAssessor(mQM, QualityAssessor::MAXIMUM);
  }

  void tearDown()
  {
     delete mQA;
     delete mQI;
     delete mQM;
     delete mOF;
  }
  
public:
  InstructionQueueTest()
    {}
  
  void test_add_preconditionner()
  {
     MsqPrintError err(cout);
     mQueue.clear();
     mQueue.add_preconditioner(mQI, err);
     CPPUNIT_ASSERT(!err);
     err.clear();
     mQueue.set_master_quality_improver(mQI,err);
     CPPUNIT_ASSERT(!err);
     err.clear();
     mQueue.add_preconditioner(mQI, err);
     CPPUNIT_ASSERT_MESSAGE("preconditionner cannot be added after master QI"
                            , err);     
  }

   void test_remove_preconditioner()
   {
      MsqPrintError err(cout);
      mQueue.clear();
      mQueue.add_preconditioner(mQI, err);   // 0
      mQueue.add_quality_assessor(mQA, err); // 1
      mQueue.add_preconditioner(mQI, err);   // 2
      mQueue.set_master_quality_improver(mQI, err);
      CPPUNIT_ASSERT(!err);
      err.clear();
      mQueue.remove_preconditioner(2, err);
      CPPUNIT_ASSERT_MESSAGE("should remove QualityImprover", !err);
      err.clear();
      mQueue.remove_preconditioner(3, err);
      CPPUNIT_ASSERT_MESSAGE("should not remove master QualityImprover", err);
      err.clear();
      mQueue.remove_preconditioner(1, err);
      CPPUNIT_ASSERT_MESSAGE("should not remove QualityAssessor", err);
      err.clear();
      mQueue.remove_preconditioner(0, err);
      CPPUNIT_ASSERT_MESSAGE("should  remove QualityImprover", !err);
      err.clear();
      mQueue.remove_preconditioner(0, err);
      CPPUNIT_ASSERT_MESSAGE("should not remove QualityAssessor", err);   
   }

   void test_insert_preconditioner()
   {
      MsqPrintError err(cout);
      mQueue.clear();
      mQueue.add_preconditioner(mQI, err);   // 0
      mQueue.add_quality_assessor(mQA, err); // 1
      mQueue.add_preconditioner(mQI, err);   // 2
      mQueue.set_master_quality_improver(mQI, err);
      CPPUNIT_ASSERT(!err);
      err.clear();
      mQueue.insert_preconditioner(mQI,2, err);
      CPPUNIT_ASSERT(!err);
      err.clear();
      mQueue.insert_preconditioner(mQI, 5, err);
      CPPUNIT_ASSERT_MESSAGE("should not insert after master QualityImprover", err);
      err.clear();
   }

   void test_add_quality_assessor()
  {
     MsqPrintError err(cout);
     mQueue.clear();
     mQueue.add_quality_assessor(mQA, err);
     CPPUNIT_ASSERT(!err);
     err.clear();
     mQueue.set_master_quality_improver(mQI,err);
     CPPUNIT_ASSERT(!err);
     err.clear();
     mQueue.add_quality_assessor(mQA, err);
     CPPUNIT_ASSERT(!err);
  }

   void test_remove_quality_assessor()
   {
      MsqPrintError err(cout);
      mQueue.clear();
      mQueue.add_preconditioner(mQI, err);   // 0
      mQueue.add_quality_assessor(mQA, err); // 1
      mQueue.add_preconditioner(mQI, err);   // 2
      mQueue.set_master_quality_improver(mQI, err);
      CPPUNIT_ASSERT(!err);
      err.clear();
      mQueue.remove_quality_assessor(2, err);
      CPPUNIT_ASSERT_MESSAGE("should not remove QualityImprover", err);
      err.clear();
      mQueue.remove_quality_assessor(3, err);
      CPPUNIT_ASSERT_MESSAGE("should not remove master QualityImprover", err);
      err.clear();
      mQueue.remove_quality_assessor(1, err);
      CPPUNIT_ASSERT_MESSAGE("should remove QualityAssessor", !err);
      err.clear();
      mQueue.remove_quality_assessor(1, err);
      CPPUNIT_ASSERT_MESSAGE("should not remove QualityImprover", err);
   }

   void test_insert_quality_assessor()
   {
      MsqPrintError err(cout);
      mQueue.clear();
      mQueue.add_preconditioner(mQI, err);   // 0
      mQueue.add_quality_assessor(mQA, err); // 1
      mQueue.add_preconditioner(mQI, err);   // 2
      mQueue.set_master_quality_improver(mQI, err);
      CPPUNIT_ASSERT(!err);
      err.clear();
      mQueue.insert_quality_assessor(mQA,2, err);
      CPPUNIT_ASSERT(!err);
      err.clear();
      mQueue.insert_quality_assessor(mQA, 5, err);
      CPPUNIT_ASSERT(!err);
      err.clear();
   }

     
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(InstructionQueueTest, "InstructionQueueTest");
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(InstructionQueueTest, "Unit");
