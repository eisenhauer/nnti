/*
// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
//                 Copyright (2004) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Roscoe A. Bartlett (bartlettra@ornl.gov) 
// 
// ***********************************************************************
// @HEADER
*/

#include "Thyra_DefaultProductVectorSpace.hpp"
#include "Thyra_VectorStdOps.hpp"
#include "Thyra_MultiVectorStdOps.hpp"
#include "Thyra_VectorSpaceTester.hpp"
#include "Thyra_TestingTools.hpp"
#include "EpetraThyraAdaptersTestHelpers.hpp"
#include "Teuchos_GlobalMPISession.hpp"

#include "Teuchos_UnitTestHarness.hpp"


namespace {


using Teuchos::ptrFromRef;
using Teuchos::Comm;


void runVectorSpaceTesterTest(const int emptyProc, 
  Teuchos::FancyOStream &out, bool &success)
{
  using Thyra::VectorSpaceBase;
  using Thyra::MultiVectorBase;
  
  RCP<const VectorSpaceBase<double> > vs;
  RCP<const Epetra_Map> epetra_map;
  createEpetraVsAndMap(g_localDim, outArg(vs), outArg(epetra_map), emptyProc);
  const int numProcs = epetra_map->Comm().NumProc();

  if (emptyProc >= numProcs) {
    out << "emptyProc = " << emptyProc << " >= numProcs = " << numProcs
        << ": Skipping this test case!\n";
    return;
  }

  const Ordinal dimMultiplier = (emptyProc < 0 ? numProcs : numProcs-1);
  
  TEST_EQUALITY(vs->dim(), g_localDim * dimMultiplier);
  
  Thyra::VectorSpaceTester<double> vectorSpaceTester;
  const double tol = 100.0 * Teuchos::ScalarTraits<double>::eps();
  vectorSpaceTester.warning_tol((0.1)*tol);
  vectorSpaceTester.error_tol(tol);
  vectorSpaceTester.show_all_tests(g_show_all_tests);
  vectorSpaceTester.dump_all(g_dumpAll);
  TEST_ASSERT(vectorSpaceTester.check(*vs, &out));
}


//
// Unit Tests
//

// Test Thyra::create_Comm() and Thyra::get_Epetra_Comm()


TEUCHOS_UNIT_TEST( EpetraThyraWrappers, comm )
{
  typedef Teuchos::GlobalMPISession GMPIS;
  const RCP<const Epetra_Comm> epetra_comm = getEpetraComm();
  TEST_EQUALITY(epetra_comm->NumProc(), GMPIS::getNProc());
  TEST_EQUALITY(epetra_comm->MyPID(), GMPIS::getRank());
  const RCP<const Comm<Ordinal> > comm = Thyra::create_Comm(epetra_comm);
  TEST_EQUALITY(comm->getSize(), GMPIS::getNProc());
  TEST_EQUALITY(comm->getRank(), GMPIS::getRank());
  const RCP<const Epetra_Comm> epetra_comm2 = Thyra::get_Epetra_Comm(*comm);
  TEST_EQUALITY(epetra_comm2->NumProc(), GMPIS::getNProc());
  TEST_EQUALITY(epetra_comm2->MyPID(), GMPIS::getRank());
}


// Test Thyra::create_VectorSpace() and Thyra::get_Epetra_Map() with empty process

// Test Thyra::create_LocallyReplicatedVectorSpace() with empty process

// Test Thyra::create_Vector() (const and nonconst) and all views with empty processes

// Test Thyra::create_MultiVector() (const and nonconst) and all views with empty processes


TEUCHOS_UNIT_TEST( EpetraThyraWrappers, vectorSpaceTester )
{
  runVectorSpaceTesterTest(-1, out, success);
}


TEUCHOS_UNIT_TEST( EpetraThyraWrappers, vectorSpaceTester_empty_p0 )
{
  runVectorSpaceTesterTest(0, out, success);
}


TEUCHOS_UNIT_TEST( EpetraThyraWrappers, vectorSpaceTester_empty_p1 )
{
  runVectorSpaceTesterTest(1, out, success);
}


TEUCHOS_UNIT_TEST( EpetraThyraWrappers, vectorSpaceTester_empty_pLast )
{
  runVectorSpaceTesterTest(Teuchos::GlobalMPISession::getNProc()-1, out, success);
}


TEUCHOS_UNIT_TEST( EpetraThyraWrappers, get_Epetra_MultiVector_singleBlockProductVector )
{
using Thyra::VectorSpaceBase;
using Thyra::MultiVectorBase;

  RCP<const VectorSpaceBase<double> > vs;
  RCP<const Epetra_Map> epetra_map;
  createEpetraVsAndMap(g_localDim, outArg(vs), outArg(epetra_map));

  const RCP<const VectorSpaceBase<double> > pvs = Thyra::productVectorSpace(vs, 1);

  const RCP<MultiVectorBase<double> > pmv = Thyra::createMembers(pvs, 1);

  const double alpha = 3.5;
  Thyra::assign<double>( pmv.ptr(), alpha );

  const RCP<Epetra_MultiVector> epetra_mv =
    Thyra::get_Epetra_MultiVector(*epetra_map, pmv);

  const RCP<MultiVectorBase<double> > mv2 =
    Thyra::create_MultiVector(epetra_mv, pvs);

  Thyra::testRelNormDiffErr<double>(
    "*pmv->col(0)", *pmv->col(0),
    "*mv2->col(0)", *mv2->col(0),
    "max-error", 0.0,
    "max-warning", 0.0,
    &out
    );
   
}


} // namespace
