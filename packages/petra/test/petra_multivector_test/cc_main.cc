// Petra_BlockMap Test routine

#include <math.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#ifdef PETRA_MPI
#include <mpi.h>
#endif
#include "Petra_Comm.h"
#include "Petra_Time.h"
#include "Petra_BlockMap.h"
#include "Petra_RDP_MultiVector.h"
#include "BuildTestProblems.h"
#include "ExecuteTestProblems.h"

int main(int argc, char *argv[]) {

  int ierr = 0, i, j;

#ifdef PETRA_MPI

  // Initialize MPI

  MPI_Init(&argc,&argv);
  int size, rank; // Number of MPI processes, My process ID

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

#else

  int size = 1; // Serial case (not using MPI)
  int rank = 0;

#endif

  bool verbose = false;

  // Check if we should print results to standard out
  if (argc>1) if (argv[1][0]=='-' && argv[1][1]=='v') verbose = true;


#ifdef PETRA_MPI
  Petra_Comm & Comm = *new Petra_Comm( MPI_COMM_WORLD );
#else
  Petra_Comm & Comm = *new Petra_Comm();
#endif


  //  char tmp;
  //  if (rank==0) cout << "Press any key to continue..."<< endl;
  //  if (rank==0) cin >> tmp;
  //  Comm.Barrier();

  int MyPID = Comm.MyPID();
  int NumProc = Comm.NumProc();
  if (verbose) cout << "Processor "<<MyPID<<" of "<< NumProc
		    << " is alive."<<endl;

  bool verbose1 = verbose;

  // Redefine verbose to only print on PE 0
  if (verbose && rank!=0) verbose = false;

  int NumMyElements = 10000;
  int NumMyElements1 = NumMyElements; // Needed for localmap
  int NumGlobalElements = NumMyElements*NumProc+minfn(NumProc,3);
  if (MyPID < 3) NumMyElements++;
  int IndexBase = 0;
  int ElementSize = 7;
  bool DistributedGlobal = (NumGlobalElements>NumMyElements);
  int NumVectors = 4;
  
  // Test LocalMap constructor

  if (verbose) cout << "Checking Petra_LocalMap(NumMyElements1, IndexBase, Comm)" << endl;

  Petra_LocalMap *LocalMap = new Petra_LocalMap(NumMyElements1, IndexBase,
                              Comm);
  // Test Petra-defined uniform linear distribution constructor
  if (verbose) cout << "Checking Petra_BlockMap(NumGlobalElements, ElementSize, IndexBase, Comm)" << endl;

  Petra_BlockMap * BlockMap = new Petra_BlockMap(NumGlobalElements, ElementSize, IndexBase, Comm);
  ierr = RDP_MultiVectorTests(*BlockMap, NumVectors, verbose);
  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = RDP_MatrixTests(*BlockMap, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete BlockMap;

  // Test User-defined linear distribution constructor

  if (verbose) cout << "Checking Petra_BlockMap(NumGlobalElements, NumMyElements, ElementSize, IndexBase, Comm)" << endl;

  BlockMap = new Petra_BlockMap(NumGlobalElements, NumMyElements, ElementSize, IndexBase, Comm);

  ierr = RDP_MultiVectorTests(*BlockMap, NumVectors, verbose);
  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = RDP_MatrixTests(*BlockMap, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete BlockMap;

  // Test User-defined arbitrary distribution constructor
  // Generate Global Element List.  Do in reverse for fun!

  int * MyGlobalElements = new int[NumMyElements];
  int MaxMyGID = (Comm.MyPID()+1)*NumMyElements-1+IndexBase;
  if (Comm.MyPID()>2) MaxMyGID+=3;
  for (i = 0; i<NumMyElements; i++) MyGlobalElements[i] = MaxMyGID-i;

  if (verbose) cout << "Checking Petra_BlockMap(NumGlobalElements, NumMyElements, MyGlobalElements,  ElementSize, IndexBase, Comm)" << endl;

  BlockMap = new Petra_BlockMap(NumGlobalElements, NumMyElements, MyGlobalElements, ElementSize,
		      IndexBase, Comm);
  ierr = RDP_MultiVectorTests(*BlockMap, NumVectors, verbose);
  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = RDP_MatrixTests(*BlockMap, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete BlockMap;

  int * ElementSizeList = new int[NumMyElements];
  int NumMyEquations = 0;
  int NumGlobalEquations = 0;
  for (i = 0; i<NumMyElements; i++) 
    {
      ElementSizeList[i] = i%6+2; // blocksizes go from 2 to 7
      NumMyEquations += ElementSizeList[i];
    }
  ElementSize = 7; // Set to maximum for use in checkmap
  NumGlobalEquations = Comm.NumProc()*NumMyEquations;

  // Adjust NumGlobalEquations based on processor ID
  if (Comm.NumProc() > 3)
    {
      if (Comm.MyPID()>2)
	NumGlobalEquations += 3*((NumMyElements)%6+2);
      else 
	NumGlobalEquations -= (Comm.NumProc()-3)*((NumMyElements-1)%6+2);
    }

  if (verbose) cout << "Checking Petra_BlockMap(NumGlobalElements, NumMyElements, MyGlobalElements,  ElementSizeList, IndexBase, Comm)" << endl;

  BlockMap = new Petra_BlockMap(NumGlobalElements, NumMyElements, MyGlobalElements, ElementSizeList,
		      IndexBase, Comm);
  ierr = RDP_MultiVectorTests(*BlockMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = RDP_MatrixTests(*BlockMap, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  // Test Copy constructor

  if (verbose) cout << "Checking Petra_BlockMap(*BlockMap)" << endl;

  Petra_BlockMap * BlockMap1 = new Petra_BlockMap(*BlockMap);

  ierr = RDP_MultiVectorTests(*BlockMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = RDP_MatrixTests(*BlockMap, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete [] ElementSizeList;
  delete [] MyGlobalElements;
  delete BlockMap;
  delete BlockMap1;


  // Test Petra-defined uniform linear distribution constructor

  if (verbose) cout << "Checking Petra_Map(NumGlobalElements, IndexBase, Comm)" << endl;

  Petra_Map * Map = new Petra_Map(NumGlobalElements, IndexBase, Comm);
  ierr = RDP_MultiVectorTests(*Map, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = RDP_MatrixTests(*Map, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete Map;

  // Test User-defined linear distribution constructor

  if (verbose) cout << "Checking Petra_Map(NumGlobalElements, NumMyElements, IndexBase, Comm)" << endl;

  Map = new Petra_Map(NumGlobalElements, NumMyElements, IndexBase, Comm);

  ierr = RDP_MultiVectorTests(*Map, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = RDP_MatrixTests(*Map, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete Map;

  // Test User-defined arbitrary distribution constructor
  // Generate Global Element List.  Do in reverse for fun!

  MyGlobalElements = new int[NumMyElements];
  MaxMyGID = (Comm.MyPID()+1)*NumMyElements-1+IndexBase;
  if (Comm.MyPID()>2) MaxMyGID+=3;
  for (i = 0; i<NumMyElements; i++) MyGlobalElements[i] = MaxMyGID-i;

  if (verbose) cout << "Checking Petra_Map(NumGlobalElements, NumMyElements, MyGlobalElements,  IndexBase, Comm)" << endl;

  Map = new Petra_Map(NumGlobalElements, NumMyElements, MyGlobalElements, 
		      IndexBase, Comm);
  ierr = RDP_MultiVectorTests(*Map, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = RDP_MatrixTests(*Map, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  // Test Copy constructor

  if (verbose) cout << "Checking Petra_Map(*Map)" << endl;
 
  Petra_Map * Map1 = new Petra_Map(*Map);

  ierr = RDP_MultiVectorTests(*Map, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  ierr = RDP_MatrixTests(*Map, *LocalMap, NumVectors, verbose);

  if (verbose)
    if (ierr==0) cout << "Checked OK\n\n" <<endl;
    else cout << "Error code: "<< ierr << endl;

  assert(ierr==0);

  delete [] MyGlobalElements;
  delete Map;
  delete Map1;
  delete LocalMap;

  if (verbose1)
    {
      // Test MultiVector MFLOPS for 2D Dot Product
      int M = 27;
      int N = 27;
      int K = 10000;
      Petra_Map * Map2 = new Petra_Map(-1, K, IndexBase, Comm);
      Petra_LocalMap * Map3 = new Petra_LocalMap(M, IndexBase, Comm);
      
      Petra_RDP_MultiVector & A = *new Petra_RDP_MultiVector(*Map2,N);A.Random();
      Petra_RDP_MultiVector & B = *new Petra_RDP_MultiVector(*Map2,N);B.Random();
      Petra_RDP_MultiVector & C = *new Petra_RDP_MultiVector(*Map3,N);C.Random();

      if (verbose) cout << "Testing Assignment operator" << endl;

      double tmp1 = 1.00001* (double) (MyPID+1);
      double tmp2 = tmp1;
      A[1][1] = tmp1;
      tmp2 = A[1][1];
      cout << "On PE "<< MyPID << "  A[1][1] should equal = " << tmp1;
      if (tmp1==tmp2) cout << " and it does!" << endl;
      else cout << " but it equals " << tmp2;
 
      Comm.Barrier();
	  
      if (verbose) cout << "Testing MFLOPs" << endl;
      Petra_Time & mytimer = *new Petra_Time(Comm);
      C.Multiply('T', 'N', 0.5, A, B, 0.0);
      double Multiply_time = mytimer.ElapsedTime();
      double Multiply_flops = C.Flops();
      if (verbose) cout << "\n\nTotal FLOPs = " << Multiply_flops << endl;
      if (verbose) cout << "Total Time  = " << Multiply_time << endl;
      if (verbose) cout << "MFLOPs      = " << Multiply_flops/Multiply_time/1000000.0 << endl;

      delete &mytimer;
      delete &A;
      delete &B;
      delete &C;
      delete Map2;
      delete Map3;

      Comm.Barrier();
	  
      // Test MultiVector ostream operator with Petra-defined uniform linear distribution constructor
      // and a small vector
      
      Petra_Map * Map4 = new Petra_Map(100, IndexBase, Comm);
      double * Dp = new double[200];
      for (j=0; j<2; j++)
	for (i=0; i<100; i++)
	  Dp[i+j*100] = i+j*100;
      Petra_RDP_MultiVector & D = *new Petra_RDP_MultiVector(View, *Map4,Dp, 100, 2);
	  
      if (verbose) cout << "\n\nTesting ostream operator:  Multivector  should be 100-by-2 and print i,j indices" 
	   << endl << endl;
      cout << D << endl;
      delete &D;
      delete [] Dp;
      delete Map4;
    }

  delete &Comm;

#ifdef PETRA_MPI
  MPI_Finalize();
#endif

  return 0;
}

