//@HEADER
// ************************************************************************
// 
//               Epetra: Linear Algebra Services Package 
//                 Copyright (2001) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ************************************************************************
//@HEADER

#include <Teuchos_UnitTestHarness.hpp>
#include <Teuchos_TestForException.hpp>
#include <Teuchos_StandardCatchMacros.hpp>
#include <Teuchos_CommandLineProcessor.hpp>
#include <Teuchos_Time.hpp>
#include <Teuchos_As.hpp>

#include "Tpetra_DefaultPlatform.hpp"
#include "Tpetra_CrsMatrix.hpp"
#include "Tpetra_MultiVector.hpp"
#include "Tpetra_Vector.hpp"

namespace {

using Teuchos::Comm;
using Teuchos::rcp;
using Teuchos::RCP;
using Tpetra::Platform;
using Tpetra::DefaultPlatform;
using Tpetra::Map;
using Tpetra::CrsMatrix;
using Tpetra::MultiVector;
using Tpetra::Vector;
using Teuchos::Time;
using Teuchos::OrdinalTraits;
using Teuchos::ScalarTraits;
using Teuchos::NO_TRANS;
using Teuchos::CONJ_TRANS;
using Teuchos::ETransp;
using Teuchos::Array;
using Teuchos::ArrayRCP;
using Teuchos::arcp;
using Teuchos::null;
using Teuchos::rcp_dynamic_cast;
using Teuchos::as;
using Teuchos::FancyOStream;
using std::max_element;
using std::endl;

// globals
bool testMpi = true;
int numNodesX = 2;
int numNodesY = 2;
int numProcsX = 1;  // NumProcX*NumProcY must equal the number of processors used to run the problem
int numProcsY = 1;
int numPoints = 5;  // numPoints==[5,9,25] in the stencil
bool shortTest = false;

TEUCHOS_STATIC_SETUP()
{
  Teuchos::CommandLineProcessor &clp = Teuchos::UnitTestRepository::getCLP();
  clp.addOutputSetupOptions(true);
  clp.setOption(
    "test-mpi", "test-serial", &testMpi,
    "Test MPI (if available) or force test of serial.  In a serial build,"
    " this option is ignored and a serial comm is always used." );
  clp.setOption("numProcsX",&numProcsX,"number of processors dividing the X axis");
  clp.setOption("numProcsY",&numProcsY,"number of processors dividing the Y axis");
  clp.setOption("numNodesX",&numNodesX,"number of nodes in the X direction for each processor");
  clp.setOption("numNodesY",&numNodesY,"number of nodes in the Y direction for each processor");
  clp.setOption("numPoints",&numPoints,"number of points in the stencil (5,9,25)");
  clp.setOption("st","lt",&shortTest,"Run short test");
}


template<class Ordinal>
RCP<const Platform<Ordinal> > getDefaultPlatform()
{
  if (testMpi) {
    return DefaultPlatform<Ordinal>::getPlatform();
  }
  return rcp(new Tpetra::SerialPlatform<Ordinal>());
}


template <class Ordinal, class Scalar>
void GenerateCrsProblem(int numNodesX, int numNodesY, int numProcsX, int numProcsY, int numPoints, 
            int *xoff, int *yoff, int numRHS,
            const Platform<Ordinal> &platform,
            RCP<Map<Ordinal> > &map,
            RCP<CrsMatrix<Ordinal,Scalar> > &A,
            RCP<MultiVector<Ordinal,Scalar> > &b,
            RCP<MultiVector<Ordinal,Scalar> > &bt,
            RCP<MultiVector<Ordinal,Scalar> > &xexact,
            FancyOStream &out);

template <class Ordinal>
ArrayRCP<Ordinal> GenerateMyGlobalElements(int numNodesX, int numNodesY, int numProcsX, int myPID);

template <class Ordinal, class Scalar>
void runMatrixTests(RCP<CrsMatrix<Ordinal,Scalar> > A,  RCP<MultiVector<Ordinal,Scalar> > b, RCP<MultiVector<Ordinal,Scalar> > bt,
                    RCP<MultiVector<Ordinal,Scalar> > xexact, FancyOStream &out);

TEUCHOS_UNIT_TEST_TEMPLATE_2_DECL( BasicPerfTest, MatrixAndMultiVector, Ordinal, Scalar )
{
  RCP<const Platform<Ordinal> > platform = getDefaultPlatform<Ordinal>();
  RCP<const Comm<Ordinal> > comm = platform->createComm();
  if (comm->getSize() != numProcsX*numProcsY) {
    out << "numProcsX*numProcsY must equal numProcs!" << endl;
    success = false;
    return;
  }

  if (comm->getRank() == 0) {
    out << " Number of local nodes in X direction  = " << numNodesX << endl
        << " Number of local nodes in Y direction  = " << numNodesY << endl
        << " Number of global nodes in X direction = " << numNodesX*numProcsX << endl
        << " Number of global nodes in Y direction = " << numNodesY*numProcsY << endl
        << " Number of local nonzero entries       = " << numNodesX*numNodesY*numPoints << endl
        << " Number of global nonzero entries      = " << numNodesX*numNodesY*numPoints*numProcsX*numProcsY << endl
        << " Number of Processors in X direction   = " << numProcsX << endl
        << " Number of Processors in Y direction   = " << numProcsY << endl
        << " Number of Points in stencil           = " << numPoints << endl << endl;
  }

  Array<int> Xoff, Yoff;
  if (numPoints==5) {
     // Generate a 5-point 2D Finite Difference matrix
    Xoff.resize(5);
    Yoff.resize(5);
    Xoff[0] = -1; Xoff[1] = 1; Xoff[2] = 0; Xoff[3] = 0;  Xoff[4] = 0; 
    Yoff[0] = 0;  Yoff[1] = 0; Yoff[2] = 0; Yoff[3] = -1; Yoff[4] = 1; 
  }
  else if (numPoints==9) {
    // Generate a 9-point 2D Finite Difference matrix
    Xoff.resize(9);
    Yoff.resize(9);
    Xoff[0] = -1;  Xoff[1] =  0; Xoff[2] =  1; 
    Yoff[0] = -1;  Yoff[1] = -1; Yoff[2] = -1; 
    Xoff[3] = -1;  Xoff[4] =  0; Xoff[5] =  1; 
    Yoff[3] =  0;  Yoff[4] =  0; Yoff[5] =  0; 
    Xoff[6] = -1;  Xoff[7] =  0; Xoff[8] =  1; 
    Yoff[6] =  1;  Yoff[7] =  1; Yoff[8] =  1; 
  }
  else {
    // Generate a 25-point 2D Finite Difference matrix
    Xoff.resize(25);
    Yoff.resize(25);
    Ordinal xi = 0, yi = 0;
    Ordinal xo = -2, yo = -2;
    Xoff[xi++] = xo++;  Xoff[xi++] = xo++; Xoff[xi++] = xo++; Xoff[xi++] = xo++; Xoff[xi++] = xo++;
    Yoff[yi++] = yo  ;  Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; 
    xo = -2, yo++;
    Xoff[xi++] = xo++;  Xoff[xi++] = xo++; Xoff[xi++] = xo++; Xoff[xi++] = xo++; Xoff[xi++] = xo++;
    Yoff[yi++] = yo  ;  Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; 
    xo = -2, yo++;
    Xoff[xi++] = xo++;  Xoff[xi++] = xo++; Xoff[xi++] = xo++; Xoff[xi++] = xo++; Xoff[xi++] = xo++;
    Yoff[yi++] = yo  ;  Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; 
    xo = -2, yo++;
    Xoff[xi++] = xo++;  Xoff[xi++] = xo++; Xoff[xi++] = xo++; Xoff[xi++] = xo++; Xoff[xi++] = xo++;
    Yoff[yi++] = yo  ;  Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; 
    xo = -2, yo++;
    Xoff[xi++] = xo++;  Xoff[xi++] = xo++; Xoff[xi++] = xo++; Xoff[xi++] = xo++; Xoff[xi++] = xo++;
    Yoff[yi++] = yo  ;  Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; Yoff[yi++] = yo  ; 
  }

  RCP<Map<Ordinal> > map;
  RCP<CrsMatrix<Ordinal,Scalar> > A;
  RCP<MultiVector<Ordinal,Scalar> > b;
  RCP<MultiVector<Ordinal,Scalar> > bt;
  RCP<MultiVector<Ordinal,Scalar> > xexact;
  Array<Scalar> scavec;
  Array<typename ScalarTraits<Scalar>::magnitudeType> magvec;

  // Timings
  Time timer("loop");
  double elapsed_time, MFLOPs;
  int flops;

  int jstop = 2;
  if (shortTest) {
    jstop = 1;
  }
  for (int j=0; j<jstop; j++) {
    for (int k=1; k<17; k++) {
      if ( (!shortTest && (k<7 || k%2==0)) 
         || (shortTest && (k<6 || k%4==0))) {
        int nrhs=k;
        out << "\n*************** Results for " << nrhs << " RHS ";

        GenerateCrsProblem(numNodesX, numNodesY, numProcsX, numProcsY, numPoints,
            Xoff.getRawPtr(), Yoff.getRawPtr(), nrhs, *platform, 
            map, A, b, bt, xexact, out);

        runMatrixTests(A, b, bt, xexact, out);

        A = null;
        b = null;
        bt = null;
        xexact = null;

        MultiVector<Ordinal,Scalar> q(*map,nrhs);
        MultiVector<Ordinal,Scalar> z(q), r(q);

        scavec.resize(nrhs);
        magvec.resize(nrhs);

        // 10 norms
        timer.start(true);
        for( int i = 0; i < 10; ++i ) {
          q.norm2( magvec() );
        }
        elapsed_time = timer.stop();
        flops = 10*2*map->getNumGlobalEntries()*nrhs;
        MFLOPs = flops/elapsed_time/1000000.0;
        out << "\nTotal MFLOPs for 10 Norm2's= " << MFLOPs << endl;

        // 10 dots
        timer.start(true);
        for( int i = 0; i < 10; ++i ) {
          q.dot(z, scavec());
        }
        elapsed_time = timer.stop();
        flops = 10*2*map->getNumGlobalEntries()*nrhs;
        MFLOPs = flops/elapsed_time/1000000.0;
        out << "Total MFLOPs for 10 Dot's  = " << MFLOPs << endl;

        // 10 updates
        timer.start(true);
        for( int i = 0; i < 10; ++i ) {
          q.update(1.0, z, 1.0, r, 0.0);
        }
        elapsed_time = timer.stop();
        flops = 10*map->getNumGlobalEntries()*nrhs;
        MFLOPs = flops/elapsed_time/1000000.0;
        out << "Total MFLOPs for 10 Updates= " << MFLOPs << endl;
      }
    }
  }
  success = true;
}

// Constructs a 2D PDE finite difference matrix using the list of x and y offsets.
// 
// nx      (In) - number of grid points in x direction
// ny      (In) - number of grid points in y direction
//   The total number of equations will be nx*ny ordered such that the x direction changes
//   most rapidly: 
//      First equation is at point (0,0)
//      Second at                  (1,0)
//       ...
//      nx equation at             (nx-1,0)
//      nx+1st equation at         (0,1)
//
// numPoints (In) - number of points in finite difference stencil
// xoff    (In) - stencil offsets in x direction (of length numPoints)
// yoff    (In) - stencil offsets in y direction (of length numPoints)
//   A standard 5-point finite difference stencil would be described as:
//     numPoints = 5
//     xoff = [-1, 1, 0,  0, 0]
//     yoff = [ 0, 0, 0, -1, 1]
//
// nrhs - Number of rhs to generate. (First interface produces vectors, so nrhs is not needed
//
// comm    (In) - an Epetra_Comm object describing the parallel machine (numProcs and my proc ID)
// map    (Out) - Map<Ordinal> describing distribution of matrix and vectors/multivectors
// A      (Out) - CrsMatrix<Ordinal,Scalar> constructed for nx by ny grid using prescribed stencil
//                Off-diagonal values are random between 0 and 1.  If diagonal is part of stencil,
//                diagonal will be slightly diag dominant.
// b      (Out) - Generated RHS.  Values satisfy b = A*xexact
// bt     (Out) - Generated RHS.  Values satisfy b = A'*xexact
// xexact (Out) - Generated exact solution to Ax = b and b' = A'xexact
//
template <class Ordinal, class Scalar>
void GenerateCrsProblem(int numNodesX, int numNodesY, int numProcsX, int /*numProcsY*/, int numPoints, 
            int * xoff, int * yoff, int nrhs,
            const Platform<Ordinal> &platform, 
            RCP<Map<Ordinal> > &map,
            RCP<CrsMatrix<Ordinal,Scalar> > &A,
            RCP<MultiVector<Ordinal,Scalar> > &b,
            RCP<MultiVector<Ordinal,Scalar> > &bt,
            RCP<MultiVector<Ordinal,Scalar> > &xexact,
            FancyOStream &out)
{
  Time timer("GenerateCrsProblem",false);
  RCP<Comm<Ordinal> > comm = platform.createComm();

  // Determine my global IDs
  ArrayRCP<Ordinal> myGlobalElements = GenerateMyGlobalElements<Ordinal>(numNodesX, numNodesY, numProcsX, comm->getRank());

  int numMyEquations = numNodesX*numNodesY;

  map = rcp(new Map<Ordinal>(OrdinalTraits<Ordinal>::invalid(), myGlobalElements(), 0, platform)); // Create map with 2D block partitioning.
  myGlobalElements = null;
  Ordinal numGlobalEquations = map->getNumGlobalEntries();

  A = rcp(new CrsMatrix<Ordinal,Scalar>(*map));

  Array<Ordinal> indices(numPoints);
  Array<Scalar>   values(numPoints);
  Scalar dnumPoints = as<Scalar>(numPoints);
  Ordinal nx = numNodesX*numProcsX;

  timer.start(true);
  for (int i=0; i<numMyEquations; i++) {
    Ordinal rowID = map->getGlobalIndex(i);
    int numIndices = 0;
    for (int j=0; j<numPoints; j++) {
      Ordinal colID = rowID + xoff[j] + nx*yoff[j]; // Compute column ID based on stencil offsets
      if (colID>=OrdinalTraits<Ordinal>::zero() && colID<numGlobalEquations) {
        indices[numIndices] = colID;
        Scalar value = -ScalarTraits<Scalar>::random();
        if (colID==rowID) {
          values[numIndices++] = dnumPoints - value; // Make diagonal dominant
        }
        else {
          values[numIndices++] = value;
        }
      }
    }
    A->submitEntries(rowID, indices(0,numIndices), values(0,numIndices));
  }
  double insertTime = timer.stop();

  timer.start(true);
  A->fillComplete();
  double fillCompleteTime = timer.stop();

  out << "Time to insert matrix values = " << insertTime << endl
      << "Time to complete fill        = " << fillCompleteTime << endl;

  if (nrhs<=1) {  
    b = rcp(new Vector<Ordinal,Scalar>(*map));
    bt = rcp(new Vector<Ordinal,Scalar>(*map));
    xexact = rcp(new Vector<Ordinal,Scalar>(*map));
  }
  else {
    b = rcp(new MultiVector<Ordinal,Scalar>(*map, nrhs));
    bt = rcp(new MultiVector<Ordinal,Scalar>(*map, nrhs));
    xexact = rcp(new MultiVector<Ordinal,Scalar>(*map, nrhs));
  }

  xexact->random(); // Fill xexact with random values
  A->apply(*xexact, *b ,NO_TRANS);
  A->apply(*xexact, *bt,CONJ_TRANS);

  return;
}

template <class Ordinal>
ArrayRCP<Ordinal> GenerateMyGlobalElements(int numNodesX, int numNodesY, int numProcsX, int myPID)
{
  ArrayRCP<Ordinal> myGEs = arcp<Ordinal>(numNodesX*numNodesY);
  int myProcX = myPID%numProcsX;
  int myProcY = myPID/numProcsX;
  int curGID = myProcY*(numProcsX*numNodesX)*numNodesY+myProcX*numNodesX;
  for (int j=0; j<numNodesY; j++) {
    for (int i=0; i<numNodesX; i++) {
      myGEs[j*numNodesX+i] = as<Ordinal>(curGID+i);
    }
    curGID+=numNodesX*numProcsX;
  }
  return myGEs;
}

template <class Ordinal, class Scalar>
void runMatrixTests(RCP<CrsMatrix<Ordinal,Scalar> > A,  RCP<MultiVector<Ordinal,Scalar> > b, RCP<MultiVector<Ordinal,Scalar> > bt,
                    RCP<MultiVector<Ordinal,Scalar> > xexact, FancyOStream &out)
{
  MultiVector<Ordinal,Scalar> z(*b);
  MultiVector<Ordinal,Scalar> r(*b);
  Array<typename ScalarTraits<Scalar>::magnitudeType> resvec(b->numVectors());

  Time timer("runMatrixTests");
  for (int j=0; j<2; j++) { // j = 0 is notrans, j = 1 is trans
    ETransp TransA = (j==1 ? CONJ_TRANS : NO_TRANS);

    timer.start(true);
    for( int i = 0; i < 10; ++i ) {
      A->apply(*xexact, z, TransA); // Compute z = A*xexact or z = A'*xexact
    }
    double elapsed_time = timer.stop();

    // Compute residual
    if (TransA == CONJ_TRANS) {
      r.update(-1.0, z, 1.0, *bt, 0.0); // r = bt - z
    }
    else {
      r.update(-1.0, z, 1.0, *b, 0.0); // r = b - z
    }
    r.norm2(resvec());

    out << "ResNorm = " << *max_element(resvec.begin(),resvec.end()) << ": ";
    out << "Total time for 10 MatVec's (Trans = " << (TransA == CONJ_TRANS ? "CONJ_TRANS" : "NO_TRANS")
      << ") = " << elapsed_time << " s" <<endl;
  }
}

  // 
  // INSTANTIATIONS
  //

#ifdef HAVE_TEUCHOS_COMPLEX
#  define UNIT_TEST_GROUP_ORDINAL_COMPLEX_FLOAT(ORDINAL)\
     typedef std::complex<float> ComplexFloat; \
     UNIT_TEST_GROUP_ORDINAL_SCALAR(ORDINAL, ComplexFloat)
#  define UNIT_TEST_GROUP_ORDINAL_COMPLEX_DOUBLE(ORDINAL)\
     typedef std::complex<double> ComplexDouble; \
     UNIT_TEST_GROUP_ORDINAL_SCALAR(ORDINAL, ComplexDouble)
#else
#  define UNIT_TEST_GROUP_ORDINAL_COMPLEX_FLOAT(ORDINAL)
#  define UNIT_TEST_GROUP_ORDINAL_COMPLEX_DOUBLE(ORDINAL)
#endif

  // Uncomment this for really fast development cycles but make sure to comment
  // it back again before checking in so that we can test all the types.
  #define FAST_DEVELOPMENT_UNIT_TEST_BUILD

#define UNIT_TEST_GROUP_ORDINAL_SCALAR( ORDINAL, SCALAR ) \
      TEUCHOS_UNIT_TEST_TEMPLATE_2_INSTANT( BasicPerfTest, MatrixAndMultiVector, ORDINAL, SCALAR )

# ifdef FAST_DEVELOPMENT_UNIT_TEST_BUILD
#    define UNIT_TEST_GROUP_ORDINAL( ORDINAL ) \
         UNIT_TEST_GROUP_ORDINAL_COMPLEX_DOUBLE(ORDINAL) \
         UNIT_TEST_GROUP_ORDINAL_SCALAR(ORDINAL, double)
     UNIT_TEST_GROUP_ORDINAL(int)
# else // not FAST_DEVELOPMENT_UNIT_TEST_BUILD

#    define UNIT_TEST_GROUP_ORDINAL( ORDINAL ) \
         UNIT_TEST_GROUP_ORDINAL_SCALAR(ORDINAL, char)   \
         UNIT_TEST_GROUP_ORDINAL_SCALAR(ORDINAL, int)    \
         UNIT_TEST_GROUP_ORDINAL_SCALAR(ORDINAL, float)  \
         UNIT_TEST_GROUP_ORDINAL_SCALAR(ORDINAL, double) \
         UNIT_TEST_GROUP_ORDINAL_COMPLEX_FLOAT(ORDINAL)  \
         UNIT_TEST_GROUP_ORDINAL_COMPLEX_DOUBLE(ORDINAL)
     UNIT_TEST_GROUP_ORDINAL(int)

     typedef long int LongInt;
     UNIT_TEST_GROUP_ORDINAL(LongInt)
#    ifdef HAVE_TEUCHOS_LONG_LONG_INT
        typedef long long int LongLongInt;
        UNIT_TEST_GROUP_ORDINAL(LongLongInt)
#    endif

# endif // FAST_DEVELOPMENT_UNIT_TEST_BUILD
}
