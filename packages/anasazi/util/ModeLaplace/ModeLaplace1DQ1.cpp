//**************************************************************************
//
//                                 NOTICE
//
// This software is a result of the research described in the report
//
// " A comparison of algorithms for modal analysis in the absence 
//   of a sparse direct method", P. Arbenz, R. Lehoucq, and U. Hetmaniuk,
//  Sandia National Laboratories, Technical report SAND2003-1028J.
//
// It is based on the Epetra, AztecOO, and ML packages defined in the Trilinos
// framework ( http://software.sandia.gov/trilinos/ ).
//
// The distribution of this software follows also the rules defined in Trilinos.
// This notice shall be marked on any reproduction of this software, in whole or
// in part.
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//
// Code Authors: U. Hetmaniuk (ulhetma@sandia.gov), R. Lehoucq (rblehou@sandia.gov)
//
//**************************************************************************

#include "ModeLaplace1DQ1.h"


const int ModeLaplace1DQ1::dofEle = 2;
const int ModeLaplace1DQ1::maxConnect = 3;
#ifndef M_PI
const double ModeLaplace1DQ1::M_PI = 3.14159265358979323846;
#endif


ModeLaplace1DQ1::ModeLaplace1DQ1(const Epetra_Comm &_Comm, double _Lx, int _nX) 
                : myVerify(_Comm),
                  MyComm(_Comm),
                  mySort(),
                  Map(0),
                  K(0),
                  M(0),
                  Lx(_Lx),
                  nX(_nX),
                  x(0)
                {

  preProcess();

}


ModeLaplace1DQ1::~ModeLaplace1DQ1() {

  if (Map)
    delete Map;
  Map = 0;

  if (K)
    delete K;
  K = 0;

  if (M)
    delete M;
  M = 0;

  if (x)
    delete[] x;
  x = 0;

}


void ModeLaplace1DQ1::preProcess() {

  // Create the distribution of equations across processors
  makeMap();

  // Count the number of elements touched by this processor
  bool *isTouched = new bool[nX];
  int i;
  for (i=0; i<nX; ++i)
    isTouched[i] = false;

  int numEle = countElements(isTouched);

  // Create the mesh
  int *elemTopo = new int[dofEle*numEle];
  makeMyElementsTopology(elemTopo, isTouched);

  delete[] isTouched;

  // Get the number of nonzeros per row
  int localSize = Map->NumMyElements();
  int *numNz = new int[localSize];
  int *connectivity = new int[localSize*maxConnect];
  makeMyConnectivity(elemTopo, numEle, connectivity, numNz);

  // Make the stiffness matrix
  makeStiffness(elemTopo, numEle, connectivity, numNz);

  // Assemble the mass matrix
  makeMass(elemTopo, numEle, connectivity, numNz);

  // Free some memory
  delete[] elemTopo;
  delete[] numNz;
  delete[] connectivity;

  // Get the geometrical coordinates of the managed nodes
  double hx = Lx/nX;
  x = new double[localSize];

  int globalSize = Map->NumGlobalElements();
  for (i=0; i<globalSize; ++i) {
    if (Map->LID(i) > -1) {
      x[Map->LID(i)] = (i+1)*hx;
    }
  }

}


void ModeLaplace1DQ1::makeMap() {

  int globalSize = nX - 1;
  assert(globalSize > MyComm.NumProc());

  // Create a uniform distribution of the unknowns across processors
  Map = new Epetra_Map(globalSize, 0, MyComm);

}


int ModeLaplace1DQ1::countElements(bool *isTouched) {

  // This routine counts and flags the elements that contain the nodes
  // on this processor.

  int i;
  int numEle = 0;

  for (i=0; i<nX; ++i) {
    int node;
    node = (i==0)  ? -1 : i-1;
    if ((node > -1) && (Map->LID(node) > -1)) {
      isTouched[i] = true;
      numEle += 1;
      continue;
    }
    node = (i==nX-1) ? -1 : i;
    if ((node > -1) && (Map->LID(node) > -1)) {
      isTouched[i] = true;
      numEle += 1;
      continue;
    }
  }

  return numEle;

}


void ModeLaplace1DQ1::makeMyElementsTopology(int *elemTopo, bool *isTouched) {

  // Create the element topology for the elements containing nodes for this processor
  // Note: Put the flag -1 when the node has a Dirichlet boundary condition

  int i;
  int numEle = 0;

  for (i=0; i<nX; ++i) {
    if (isTouched[i] == false)
      continue;
    elemTopo[dofEle*numEle]   = (i==0)  ? -1 : i-1;
    elemTopo[dofEle*numEle+1] = (i==nX-1) ? -1 : i;
    numEle += 1;
  }

}


void ModeLaplace1DQ1::makeMyConnectivity(int *elemTopo, int numEle, int *connectivity,
                                         int *numNz) {

  // This routine creates the connectivity of each managed node
  // from the element topology.

  int i, j;
  int localSize = Map->NumMyElements();

  for (i=0; i<localSize; ++i) {
    numNz[i] = 0;
    for (j=0; j<maxConnect; ++j) {
      connectivity[i*maxConnect + j] = -1; 
    }
  }

  for (i=0; i<numEle; ++i) {
    for (j=0; j<dofEle; ++j) {
      if (elemTopo[dofEle*i+j] == -1)
        continue;
      int node = Map->LID(elemTopo[dofEle*i+j]);
      if (node > -1) {
        int k;
        for (k=0; k<dofEle; ++k) {
          int neighbor = elemTopo[dofEle*i+k];
          if (neighbor == -1)
            continue;
          // Check if this neighbor is already stored
          int l;
          for (l=0; l<maxConnect; ++l) {
            if (neighbor == connectivity[node*maxConnect + l])
              break;
            if (connectivity[node*maxConnect + l] == -1) {
              connectivity[node*maxConnect + l] = neighbor;
              numNz[node] += 1;
              break;
            }
          } // for (l = 0; l < maxConnect; ++l)
        } // for (k = 0; k < dofEle; ++k)
      } // if (node > -1)
    } // for (j = 0; j < dofEle; ++j)
  } // for (i = 0; i < numEle; ++i)

}


void ModeLaplace1DQ1::makeStiffness(int *elemTopo, int numEle, int *connectivity,
                                    int *numNz) {

  // Create Epetra_Matrix for stiffness
  K = new Epetra_CrsMatrix(Copy, *Map, numNz);

  int i;
  int localSize = Map->NumMyElements();

  double *values = new double[maxConnect];
  for (i=0; i<maxConnect; ++i) 
    values[i] = 0.0;

  for (i=0; i<localSize; ++i) {
    assert(K->InsertGlobalValues(Map->GID(i), numNz[i], values, 
           connectivity+maxConnect*i) == 0);
  }

  // Define the elementary matrix
  double hx = Lx/nX;
  double *kel = new double[dofEle*dofEle];
  kel[0] = 1.0/hx; kel[1] = -1.0/hx;
  kel[2] = -1.0/hx; kel[3] = 1.0/hx;

  // Assemble the matrix
  int *indices = new int[dofEle];
  int numEntries;
  int j;
  for (i=0; i<numEle; ++i) {
    for (j=0; j<dofEle; ++j) {
      if (elemTopo[dofEle*i + j] == -1)
        continue;
      if (Map->LID(elemTopo[dofEle*i+j]) == -1)
        continue;
      numEntries = 0;
      int k;
      for (k=0; k<dofEle; ++k) {
        if (elemTopo[dofEle*i+k] == -1)
          continue;
        indices[numEntries] = elemTopo[dofEle*i+k];
        values[numEntries] = kel[dofEle*j + k];
        numEntries += 1;
      }
      assert(K->SumIntoGlobalValues(elemTopo[dofEle*i+j], numEntries, values, indices) == 0);
    }
  }

  delete[] kel;
  delete[] values;
  delete[] indices;

  assert(K->FillComplete()== 0);
  assert(K->OptimizeStorage() == 0);

}


void ModeLaplace1DQ1::makeMass(int *elemTopo, int numEle, int *connectivity,
                               int *numNz) {

  // Create Epetra_Matrix for mass
  M = new Epetra_CrsMatrix(Copy, *Map, numNz);

  int i;
  int localSize = Map->NumMyElements();

  double *values = new double[maxConnect];
  for (i=0; i<maxConnect; ++i) 
    values[i] = 0.0;
  for (i=0; i<localSize; ++i) 
    assert( M->InsertGlobalValues(Map->GID(i), numNz[i], values,
                                   connectivity + maxConnect*i) == 0); 

  // Define the elementary matrix
  double hx = Lx/nX;

  double *mel = new double[dofEle*dofEle];
  mel[0] = hx/3.0; mel[1] = hx/6.0;
  mel[2] = hx/6.0; mel[3] = hx/3.0;

  // Assemble the matrix
  int *indices = new int[dofEle];
  int numEntries;
  int j;
  for (i=0; i<numEle; ++i) {
    for (j=0; j<dofEle; ++j) {
      if (elemTopo[dofEle*i + j] == -1)
        continue;
      if (Map->LID(elemTopo[dofEle*i+j]) == -1)
        continue;
      numEntries = 0;
      int k;
      for (k=0; k<dofEle; ++k) {
        if (elemTopo[dofEle*i+k] == -1)
          continue;
        indices[numEntries] = elemTopo[dofEle*i+k];
        values[numEntries] = mel[dofEle*j + k];
        numEntries += 1;
      }
      assert(M->SumIntoGlobalValues(elemTopo[dofEle*i+j], numEntries, values, indices) == 0);
    }
  }

  delete[] mel;
  delete[] values;
  delete[] indices;

  assert(M->FillComplete()== 0);
  assert(M->OptimizeStorage() == 0);

}


double ModeLaplace1DQ1::getFirstMassEigenValue() const {

  return Lx/(3.0*nX)*(2.0-cos(M_PI/nX));

}


int ModeLaplace1DQ1::eigenCheck(const Epetra_MultiVector &Q, double *lambda, 
                                double *normWeight, bool smallest) const {

  using std::cout;
  using std::ios;

  int info = 0;
  int qc = Q.NumVectors();
  int myPid = MyComm.MyPID();

  cout.precision(2);
  cout.setf(ios::scientific, ios::floatfield);

  // Check orthonormality of eigenvectors
  double tmp = myVerify.errorOrthonormality(&Q, M);
  if (myPid == 0)
    cout << " Maximum coefficient in matrix Q^T M Q - I = " << tmp << endl;

  // Print out norm of residuals
  myVerify.errorEigenResiduals(Q, lambda, K, M, normWeight);

  // Check the eigenvalues
  int numX = (int) ceil(sqrt(Lx*Lx*lambda[qc-1]/M_PI/M_PI));
  numX = (numX > nX) ? nX : numX;
  int newSize = (numX-1);
  double *discrete = new (std::nothrow) double[2*newSize];
  if (discrete == 0) {
    return -1;
  }
  double *continuous = discrete + newSize;

  double hx = Lx/nX;

  int i;
  for (i = 1; i < numX; ++i) {
    continuous[i-1] = (M_PI/Lx)*(M_PI/Lx)*i*i;
    discrete[i-1] = 6.0*(1.0-cos(i*(M_PI/Lx)*hx))/(2.0+cos(i*(M_PI/Lx)*hx))/hx/hx;
  }

  // Sort the eigenvalues in ascending order
  mySort.sortScalars(newSize, continuous);

  int *used = new (std::nothrow) int[newSize];
  if (used == 0) {
    delete[] discrete;
    return -1;
  }

  mySort.sortScalars(newSize, discrete, used);

  int *index = new (std::nothrow) int[newSize];
  if (index == 0) {
    delete[] discrete;
    delete[] used;
    return -1;
  }

  for (i=0; i<newSize; ++i) {
    index[used[i]] = i;
  }
  delete[] used;

  // sort the eigenvalues/vectors in ascending order
  double *lambdasorted = new (std::nothrow) double[qc];
  if (lambdasorted == 0) {
    delete[] discrete;
    return -1;
  }
  for (i=0; i<qc; i++) {
    lambdasorted[i] = lambda[i];
  }
  mySort.sortScalars(qc, lambdasorted);
  // compare the discrete eigenvalues with the user eigenvalues
  int nMax = myVerify.errorLambda(continuous, discrete, newSize, lambdasorted, qc, smallest);
  // 0 <= nMax < newSize
  // if smallest == true, nMax is the rightmost value in continuous that we matched against
  // if smallest == false, nMax is the leftmost value in continuous that we matched against

  // Define the exact discrete eigenvectors
  int localSize = Map->NumMyElements();
  double *vQ = new (std::nothrow) double[(nMax+2)*localSize];
  if (vQ == 0) {
    delete[] discrete;
    delete[] index;
    info = -1;
    return info;
  }

  int Qexdim;
  if (smallest) {
    Qexdim = nMax+1;
  }
  else {
    Qexdim = numX-nMax-1;
  }

  Epetra_MultiVector Qex(View, *Map, vQ, localSize, Qexdim);

  if ((myPid == 0) && (Qexdim > 0)) {
    cout << endl;
    cout << " --- Relative discretization errors for exact eigenvectors ---" << endl;
    cout << endl;
    cout << "       Cont. Values   Disc. Values     Error      H^1 norm   L^2 norm\n";
  }

  if (smallest) {
    for (i=1; i < numX; ++i) {
      if (index[i-1] <= nMax) {
        // Form the exact discrete eigenvector
        double coeff = (2.0 + cos(i*M_PI/Lx*hx))*Lx/6.0;
        coeff = 1.0/sqrt(coeff);
        int ii;
        for (ii=0; ii<localSize; ++ii) {
          Qex.ReplaceMyValue(ii, index[i-1], coeff*sin(i*(M_PI/Lx)*x[ii]));
        }
        // Compute the L2 norm
        Epetra_MultiVector shapeInt(View, *Map, vQ + (nMax+1)*localSize, localSize, 1);
        Epetra_MultiVector Qi(View, Qex, index[i-1], 1);
        for (ii=0; ii<localSize; ++ii) {
          double iX = 4.0*sqrt(2.0/Lx)*sin(i*(M_PI/Lx)*x[ii])/hx*
                      pow(sin(i*(M_PI/Lx)*0.5*hx)/(i*M_PI/Lx), 2.0);
          shapeInt.ReplaceMyValue(ii, 0, iX);
        }
        double normL2 = 0.0;
        Qi.Dot(shapeInt, &normL2);
        double normH1 = continuous[i-1]*(1.0 - 2.0*normL2) + discrete[i-1];
        normL2 = 2.0 - 2.0*normL2;
        normH1+= normL2;
        // Print out the result
        if (myPid == 0) {
          cout << " ";
          cout.width(4);
          cout << index[i-1]+1 << ". ";
          cout.setf(ios::scientific, ios::floatfield);
          cout.precision(8);
          cout << continuous[i-1] << " " << discrete[i-1] << "  ";
          cout.precision(3);
          cout << fabs(discrete[i-1] - continuous[i-1])/continuous[i-1] << "  ";
          cout << sqrt(fabs(normH1)/(continuous[i-1]+1.0)) << "  ";
          cout << sqrt(fabs(normL2)) << endl;
        }
      }
    }
  }
  else {
    for (i=1; i < numX; ++i) {
      if (index[i-1] >= nMax) {
        // Form the exact discrete eigenvector
        double coeff = (2.0 + cos(i*M_PI/Lx*hx))*Lx/6.0;
        coeff = 1.0/sqrt(coeff);
        int ii;
        for (ii=0; ii<localSize; ++ii) {
          Qex.ReplaceMyValue(ii, index[i-1]-nMax, coeff*sin(i*(M_PI/Lx)*x[ii]));
        }
        // Compute the L2 norm
        Epetra_MultiVector shapeInt(View, *Map, vQ + (numX-nMax-1)*localSize, localSize, 1);
        Epetra_MultiVector Qi(View, Qex, index[i-1]-nMax, 1);
        for (ii=0; ii<localSize; ++ii) {
          double iX = 4.0*sqrt(2.0/Lx)*sin(i*(M_PI/Lx)*x[ii])/hx*
                      pow(sin(i*(M_PI/Lx)*0.5*hx)/(i*M_PI/Lx), 2.0);
          shapeInt.ReplaceMyValue(ii, 0, iX);
        }
        double normL2 = 0.0;
        Qi.Dot(shapeInt, &normL2);
        double normH1 = continuous[i-1]*(1.0 - 2.0*normL2) + discrete[i-1];
        normL2 = 2.0 - 2.0*normL2;
        normH1+= normL2;
        // Print out the result
        if (myPid == 0) {
          cout << " ";
          cout.width(4);
          cout << index[i-1]+1 << ". ";
          cout.setf(ios::scientific, ios::floatfield);
          cout.precision(8);
          cout << continuous[i-1] << " " << discrete[i-1] << "  ";
          cout.precision(3);
          cout << fabs(discrete[i-1] - continuous[i-1])/continuous[i-1] << "  ";
          cout << sqrt(fabs(normH1)/(continuous[i-1]+1.0)) << "  ";
          cout << sqrt(fabs(normL2)) << endl;
        }
      }
    }
  }

  // Check the angles between exact discrete eigenvectors and computed
  myVerify.errorSubspaces(Q, Qex, M);

  delete[] vQ;

  delete[] discrete;
  delete[] index;

  return info;

}


void ModeLaplace1DQ1::memoryInfo() const {

  using std::cout;
  using std::ios;

  int myPid = MyComm.MyPID();

  Epetra_RowMatrix *Mat = dynamic_cast<Epetra_RowMatrix *>(M);
  if ((myPid == 0) && (Mat)) {
    cout << " Total number of nonzero entries in mass matrix      = ";
    cout.width(15);
    cout << Mat->NumGlobalNonzeros() << endl;
    double memSize = Mat->NumGlobalNonzeros()*(sizeof(double) + sizeof(int));
    memSize += 2*Mat->NumGlobalRows()*sizeof(int);
    cout << " Memory requested for mass matrix per processor      = (EST) ";
    cout.precision(2);
    cout.width(6);
    cout.setf(ios::fixed, ios::floatfield);
    cout << memSize/1024.0/1024.0/MyComm.NumProc() << " MB " << endl;
    cout << endl;
  }

  Mat = dynamic_cast<Epetra_RowMatrix *>(K);
  if ((myPid == 0) && (Mat)) {
    cout << " Total number of nonzero entries in stiffness matrix = ";
    cout.width(15);
    cout << Mat->NumGlobalNonzeros() << endl;
    double memSize = Mat->NumGlobalNonzeros()*(sizeof(double) + sizeof(int));
    memSize += 2*Mat->NumGlobalRows()*sizeof(int);
    cout << " Memory requested for stiffness matrix per processor = (EST) ";
    cout.precision(2);
    cout.width(6);
    cout.setf(ios::fixed, ios::floatfield);
    cout << memSize/1024.0/1024.0/MyComm.NumProc() << " MB " << endl;
    cout << endl;
  }

}


void ModeLaplace1DQ1::problemInfo() const {

  using std::cout;
  using std::ios;

  int myPid = MyComm.MyPID();

  if (myPid == 0) {
    cout.precision(2);
    cout.setf(ios::fixed, ios::floatfield);
    cout << " --- Problem definition ---\n\n";
    cout << " >> Laplace equation in 1D with homogeneous Dirichlet condition\n";
    cout << " >> Domain = [0, " << Lx << "]\n";
    cout << " >> Orthogonal mesh uniform per direction with Q1 elements\n";
    cout << endl;
    cout << " Global size = " << Map->NumGlobalElements() << endl;
    cout << endl;
    cout << " Number of elements in [0, " << Lx << "] (X-direction): " << nX << endl;
    cout << endl;
    cout << " Number of interior nodes in the X-direction: " << nX-1 << endl;
    cout << endl;
  }

}
