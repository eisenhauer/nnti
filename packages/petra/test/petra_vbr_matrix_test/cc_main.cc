#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#ifdef PETRA_MPI
#include "mpi.h"
#endif
#ifndef __cplusplus
#define __cplusplus
#endif
#include "Petra_Comm.h"
#include "Petra_RDP_DenseMatrix.h"
#include "Petra_Map.h"
#include "Petra_Time.h"
#include "Petra_RDP_Vector.h"
#include "Petra_RDP_VBR_Matrix.h"

// prototypes

int CompareValues(double * A, int LDA, int NumRowsA, int NumColsA, 
		  double * B, int LDB, int NumRowsB, int NumColsB);

int check(Petra_RDP_VBR_Matrix& A, 
	  int NumMyRows1, int NumGlobalRows1, int NumMyNonzeros1, int NumGlobalNonzeros1, 
	  int NumMyBlockRows1, int NumGlobalBlockRows1, int NumMyBlockNonzeros1, int NumGlobalBlockNonzeros1, 
	  int * MyGlobalElements, bool verbose);

int power_method(bool TransA, Petra_RDP_VBR_Matrix& A, 
		 Petra_RDP_Vector& q,
		 Petra_RDP_Vector& z, 
		 Petra_RDP_Vector& resid, 
		 double * lambda, int niters, double tolerance,
		 bool verbose);

 
int main(int argc, char *argv[])
{
  int ierr = 0, i, j;
  bool debug = false;

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


  char tmp;
  if (rank==0) cout << "Press any key to continue..."<< endl;
  if (rank==0) cin >> tmp;
  Comm.Barrier();

  int MyPID = Comm.MyPID();
  int NumProc = Comm.NumProc();
  if (verbose) cout << "Processor "<<MyPID<<" of "<< NumProc
              << " is alive."<<endl;

  bool verbose1 = verbose;

  // Redefine verbose to only print on PE 0
  if (verbose && rank!=0) verbose = false;

  int NumMyElements = 1000;
  if (MyPID < 3) NumMyElements++;

  // Define pseudo-random block sizes using a Petra Vector of random numbers
  Petra_Map & randmap = * new Petra_Map(-1, NumMyElements, 0, Comm);
  Petra_RDP_Vector & randvec = * new Petra_RDP_Vector(randmap);
  randvec.Random(); // Fill with random numbers
  int * ElementSizeList = new int[NumMyElements];
  int MinSize = 3;
  int MaxSize = 8;
  int SizeRange = MaxSize - MinSize + 1;
  
  for (i=0; i<NumMyElements; i++) ElementSizeList[i] = 3 + SizeRange * (int) (fabs(randvec[i]) * .99);

  // Construct a Map

  int *randMyGlobalElements = randmap.MyGlobalElements();

  Petra_BlockMap& Map = *new Petra_BlockMap(-1, NumMyElements, 
					    randMyGlobalElements, ElementSizeList, 0, Comm);
  delete &randvec;
  delete &randmap; // Done with these
  
  // Get update list and number of local elements from newly created Map
  int NumGlobalElements = Map.NumGlobalElements();
  int * MyGlobalElements = Map.MyGlobalElements();
  bool DistributedGlobal = Map.DistributedGlobal();

  // Create an integer vector NumNz that is used to build the Petra Matrix.
  // NumNz[i] is the Number of OFF-DIAGONAL term for the ith global equation on this processor

  int * NumNz = new int[NumMyElements];

  // We are building a block tridiagonal matrix

  for (i=0; i<NumMyElements; i++)
    if (MyGlobalElements[i]==0 || MyGlobalElements[i] == NumGlobalElements-1)
      NumNz[i] = 2;
    else
      NumNz[i] = 3;
  // Create a Petra_Matrix

  Petra_RDP_VBR_Matrix& A = *new Petra_RDP_VBR_Matrix(Copy, Map, NumNz);
  assert(!A.IndicesAreGlobal());
  assert(!A.IndicesAreLocal());
  
  // Use an array of Petra_RDP_DenseMatrix objects to build VBR matrix

  Petra_RDP_DenseMatrix ** BlockEntries = new Petra_RDP_DenseMatrix*[SizeRange];

  // The array of dense matrices will increase in size from MinSize to MaxSize (defined above)
  for (int kr=0; kr<SizeRange; kr++) {
    BlockEntries[kr] = new Petra_RDP_DenseMatrix[SizeRange];
    int RowDim = ElementSizeList[kr];
    for (int kc = 0; kc<SizeRange; kc++) {
      int ColDim = ElementSizeList[kc];
      Petra_RDP_DenseMatrix * curmat = &(BlockEntries[kr][kc]);
      curmat->Shape(RowDim,ColDim);
    for (j=0; j < ColDim; j++)
      for (i=0; i < RowDim; i++) {
	BlockEntries[kr][kc][j][i] = -1.0;
	if (i==j && kr==kc) BlockEntries[kr][kc][j][i] = 9.0;
	else BlockEntries[kr][kc][j][i] = -1.0;
      }
    }
  }
  
    
  // Add  rows one-at-a-time

  int *Indices = new int[3];
  int *ColDims = new int[3];
  int NumEntries;
  int NumMyNonzeros = 0, NumMyEquations = 0;
  
  for (i=0; i<NumMyElements; i++) {
    int CurRow = MyGlobalElements[i];
    int RowDim = ElementSizeList[i]-MinSize;
    NumMyEquations += BlockEntries[RowDim][RowDim].M();
    
    if (CurRow==0)
      {
	Indices[0] = CurRow;
	Indices[1] = CurRow+1;
	NumEntries = 2;
	ColDims[0] = ElementSizeList[i] - MinSize;
	ColDims[1] = ElementSizeList[i+1] - MinSize; // Assumes linear global ordering and > 1 row/proc.
      }
    else if (CurRow == NumGlobalElements-1)
      {
	Indices[0] = CurRow-1;
	Indices[1] = CurRow;
	NumEntries = 2;
	ColDims[0] = ElementSizeList[i-1] - MinSize;
	  ColDims[1] = ElementSizeList[i] - MinSize; // Assumes linear global ordering and > 1 row/proc.
      }
      else {
	Indices[0] = CurRow-1;
	Indices[1] = CurRow;
	Indices[2] = CurRow+1;
	NumEntries = 3;
	if (i==0) ColDims[0] = maxfn(MinSize, minfn(MaxSize, MyPID-1)) - MinSize; // ElementSize on MyPID-1
	else ColDims[0] = ElementSizeList[i-1];
	ColDims[1] = ElementSizeList[i];
	// ElementSize on MyPID+1
	if (i==NumMyElements-1) ColDims[2] = maxfn(MinSize, minfn(MaxSize, MyPID)) - MinSize;
	else ColDims[0] = ElementSizeList[i+1] - MinSize;
      }
    assert(A.BeginInsertGlobalValues(CurRow, NumEntries, Indices)==0);
    for (j=0; j < NumEntries; j++) {
      Petra_RDP_DenseMatrix AD = BlockEntries[RowDim][ColDims[j]];
      NumMyNonzeros += AD.M() * AD.N();	  
      assert(A.SubmitBlockEntry(AD.A(), AD.LDA(), AD.M(), AD.N())==0);
    }

      A.EndSubmitEntries();
  }
  
  // Finish up
  assert(A.IndicesAreGlobal());
  assert(A.TransformToLocal()==0);
  assert(A.IndicesAreLocal());
  assert(!A.StorageOptimized());
  // A.OptimizeStorage();
  // assert(A.StorageOptimized());
  assert(!A.UpperTriangular());
  assert(!A.LowerTriangular());

  

  int NumMyBlockEntries = 3*NumMyElements;
  if (A.LRID(0)>=0) NumMyBlockEntries--; // If I own first global row, then there is one less nonzero
  if (A.LRID(NumGlobalElements-1)>=0) NumMyBlockEntries--; // If I own last global row, then there is one less nonzero

  int NumGlobalBlockEntries = 3*NumGlobalElements-2;
  int NumGlobalNonzeros, NumGlobalEquations;
  Comm.SumAll(&NumMyNonzeros, &NumGlobalNonzeros, 1);
  Comm.SumAll(&NumMyEquations, &NumGlobalEquations, 1);
  

  assert(check(A, NumMyEquations, NumGlobalEquations, NumMyNonzeros, NumGlobalNonzeros, 
	       NumMyElements, NumGlobalElements, NumMyBlockEntries, NumGlobalBlockEntries, 
	       MyGlobalElements, verbose)==0);

  for (i=0; i<NumMyElements; i++) assert(A.NumGlobalBlockEntries(MyGlobalElements[i])==NumNz[i]);
  for (i=0; i<NumMyElements; i++) assert(A.NumMyBlockEntries(i)==NumNz[i]);

  if (verbose) cout << "\n\nNumEntries function check OK" << endl<< endl;


  // Create vectors for Power method

  Petra_RDP_Vector& q = *new Petra_RDP_Vector(Map);
  Petra_RDP_Vector& z = *new Petra_RDP_Vector(Map);
  Petra_RDP_Vector& z_initial = *new Petra_RDP_Vector(Map);
  Petra_RDP_Vector& resid = *new Petra_RDP_Vector(Map);

  
  // Fill z with random Numbers 
  z_initial.Random();

  // variable needed for iteration
  double lambda = 0.0;
  int niters = 100;
  // int niters = 200;
  double tolerance = 1.0e-3;

  /////////////////////////////////////////////////////////////////////////////////////////////////

  // Iterate

  z = z_initial;  // Start with common initial guess
  Petra_Time & timer = *new Petra_Time(Comm);
  ierr += power_method(false, A, q, z, resid, &lambda, niters, tolerance, verbose);
  double elapsed_time = timer.ElapsedTime();
  double total_flops = A.Flops() + q.Flops() + z.Flops() + resid.Flops();
  double MFLOPs = total_flops/elapsed_time/1000000.0;

  if (verbose) cout << "\n\nTotal MFLOPs for first solve = " << MFLOPs << endl<< endl;

  /////////////////////////////////////////////////////////////////////////////////////////////////

  // Solve transpose problem

  if (verbose) cout << "\n\nUsing transpose of matrix and solving again (should give same result).\n\n"
		    << endl;
  // Iterate
  lambda = 0.0;
  z = z_initial;  // Start with common initial guess
  A.ResetFlops(); q.ResetFlops(); z.ResetFlops(); resid.ResetFlops();
  timer.ResetStartTime();
  ierr += power_method(true, A, q, z, resid, &lambda, niters, tolerance, verbose);
  elapsed_time = timer.ElapsedTime();
  total_flops = A.Flops() + q.Flops() + z.Flops() + resid.Flops();
  MFLOPs = total_flops/elapsed_time/1000000.0;

  if (verbose) cout << "\n\nTotal MFLOPs for transpose solve = " << MFLOPs << endl<< endl;

  /////////////////////////////////////////////////////////////////////////////////////////////////

  // Increase diagonal dominance

  if (verbose) cout << "\n\nIncreasing the magnitude of first diagonal term and solving again\n\n"
		    << endl;

  
  if (A.MyGlobalBlockRow(0)) {
    int numvals = A.NumGlobalBlockEntries(0);
    double ** Rowvals;
    int* Rowinds = new int[numvals];
    int  RowDim;
    int* ColDims;
    int* LDAs;
    A.ExtractGlobalBlockRowPointers(0, numvals, RowDim, numvals, Rowinds, 
				   ColDims, LDAs, Rowvals); // Get A[0,:]

    for (i=0; i<numvals; i++) if (Rowinds[i] == 0)
      Rowvals[i][0] *= 10.0; // Multiply first diag value by 10.0
    
    delete [] Rowinds;
  }
  // Iterate (again)
  lambda = 0.0;
  z = z_initial;  // Start with common initial guess
  A.ResetFlops(); q.ResetFlops(); z.ResetFlops(); resid.ResetFlops();
  timer.ResetStartTime();
  ierr += power_method(false, A, q, z, resid, &lambda, niters, tolerance, verbose);
  elapsed_time = timer.ElapsedTime();
  total_flops = A.Flops() + q.Flops() + z.Flops() + resid.Flops();
  MFLOPs = total_flops/elapsed_time/1000000.0;

  if (verbose) cout << "\n\nTotal MFLOPs for second solve = " << MFLOPs << endl<< endl;

  /////////////////////////////////////////////////////////////////////////////////////////////////

  // Solve transpose problem

  if (verbose) cout << "\n\nUsing transpose of matrix and solving again (should give same result).\n\n"
		    << endl;

  // Iterate (again)
  lambda = 0.0;
  z = z_initial;  // Start with common initial guess
  A.ResetFlops(); q.ResetFlops(); z.ResetFlops(); resid.ResetFlops();
  timer.ResetStartTime();
  ierr += power_method(true, A, q, z, resid, &lambda, niters, tolerance, verbose);
  elapsed_time = timer.ElapsedTime();
  total_flops = A.Flops() + q.Flops() + z.Flops() + resid.Flops();
  MFLOPs = total_flops/elapsed_time/1000000.0;

  delete &timer;

  if (verbose) cout << "\n\nTotal MFLOPs for tranpose of second solve = " << MFLOPs << endl<< endl;


  if (debug) Comm.Barrier();

  if (verbose) cout << "\n\n*****Testing copy constructor" << endl<< endl;

  Petra_RDP_VBR_Matrix & B = *new Petra_RDP_VBR_Matrix(A);

  assert(check(B, NumMyEquations, NumGlobalEquations, NumMyNonzeros, NumGlobalNonzeros, 
	       NumMyElements, NumGlobalElements, NumMyBlockEntries, NumGlobalBlockEntries, 
	       MyGlobalElements, verbose)==0);


  if (debug) Comm.Barrier();

  if (verbose) cout << "\n\n*****Testing post construction modifications" << endl<< endl;

  int One = 1;
  if (B.MyGRID(0)) assert(B.BeginInsertGlobalValues(0, 1, &One)==-2);
  delete &B;

  // Release all objects
  delete [] Indices;
  delete [] NumNz;

  delete &resid;
  delete &z;
  delete &z_initial;
  delete &q;
  delete &A;
  delete &Map;
			
  /*
  if (verbose1) {
    // Test ostream << operator (if verbose1)
    // Construct a Map that puts 2 equations on each PE
    
    int NumMyElements1 = 2;
    int NumMyElements1 = NumMyElements1;
    int NumGlobalElements1 = NumMyElements1*NumProc;

    Petra_Map& Map1 = *new Petra_Map(-1, NumMyElements1, 0, Comm);
    
    // Get update list and number of local equations from newly created Map
    int * MyGlobalElements1 = new int[Map1.NumMyElements()];
    Map1.MyGlobalElements(MyGlobalElements1);
    
    // Create an integer vector NumNz that is used to build the Petra Matrix.
    // NumNz[i] is the Number of OFF-DIAGONAL term for the ith global equation on this processor
    
    int * NumNz1 = new int[NumMyElements1];
    
    // We are building a tridiagonal matrix where each row has (-1 2 -1)
    // So we need 2 off-diagonal terms (except for the first and last equation)
    
    for (i=0; i<NumMyElements1; i++)
      if (MyGlobalElements1[i]==0 || MyGlobalElements1[i] == NumGlobalElements1-1)
	NumNz1[i] = 1;
      else
	NumNz1[i] = 2;
    
    // Create a Petra_Matrix
    
    Petra_RDP_VBR_Matrix& A1 = *new Petra_RDP_VBR_Matrix(Copy, Map1, NumNz1);
    
    // Add  rows one-at-a-time
    // Need some vectors to help
    // Off diagonal Values will always be -1
    
    
    int *Indices1 = new int[2];
    double two1 = 2.0;
    int NumEntries1;
    
    for (i=0; i<NumMyElements1; i++)
      {
	if (MyGlobalElements1[i]==0)
	  {
	    Indices1[0] = 1;
	    NumEntries1 = 1;
	  }
	else if (MyGlobalElements1[i] == NumGlobalElements1-1)
	  {
	    Indices1[0] = NumGlobalElements1-2;
	    NumEntries1 = 1;
	  }
	else
	  {
	    Indices1[0] = MyGlobalElements1[i]-1;
	    Indices1[1] = MyGlobalElements1[i]+1;
	    NumEntries1 = 2;
	  }
	assert(A1.InsertGlobalValues(MyGlobalElements1[i], NumEntries1, Values1, Indices1)==0);
	assert(A1.InsertGlobalValues(MyGlobalElements1[i], 1, &two1, MyGlobalElements1+i)>0); // Put in the diagonal entry
      }
    
    // Finish up
    assert(A1.TransformToLocal()==0);
    
    if (verbose) cout << "\n\nPrint out tridiagonal matrix, each part on each processor.\n\n" << endl;
    cout << A1 << endl;
    
    // Release all objects
    delete [] NumNz1;
    delete [] Values1;
    delete [] Indices1;
    delete [] MyGlobalElements1;

    delete &A1;
    delete &Map1;
  }
  */

  delete &Comm;
			
#ifdef PETRA_MPI
  MPI_Finalize() ;
#endif

/* end main
*/
return ierr ;
}

int power_method(bool TransA, Petra_RDP_VBR_Matrix& A, 
		 Petra_RDP_Vector& q,
		 Petra_RDP_Vector& z, 
		 Petra_RDP_Vector& resid, 
		 double * lambda, int niters, double tolerance,
		 bool verbose) {  

  // variable needed for iteration
  double normz, residual;

  int ierr = 1;

  for (int iter = 0; iter < niters; iter++)
    {
      z.Norm2(&normz); // Compute 2-norm of z
      q.Scale(1.0/normz, z);
      A.Multiply(TransA, q, z); // Compute z = A*q
      q.Dot(z, lambda); // Approximate maximum eigenvaluE
      if (iter%100==0 || iter+1==niters)
	{
	  resid.Update(1.0, z, -(*lambda), q, 0.0); // Compute A*q - lambda*q
	  resid.Norm2(&residual);
	  if (verbose) cout << "Iter = " << iter << "  Lambda = " << *lambda 
			     << "  Residual of A*q - lambda*q = " << residual << endl;
	} 
      if (residual < tolerance) {
	ierr = 0;
	break;
      }
    }
  return(ierr);
}
int check(Petra_RDP_VBR_Matrix& A, 
	  int NumMyRows1, int NumGlobalRows1, int NumMyNonzeros1, int NumGlobalNonzeros1, 
	  int NumMyBlockRows1, int NumGlobalBlockRows1, int NumMyBlockNonzeros1, int NumGlobalBlockNonzeros1, 
	  int * MyGlobalElements, bool verbose) {

  // Test query functions

  int NumMyRows = A.NumMyRows();
  if (verbose) cout << "\n\nNumber of local Rows = " << NumMyRows << endl<< endl;

  assert(NumMyRows==NumMyRows1);

  int NumMyNonzeros = A.NumMyNonzeros();
  if (verbose) cout << "\n\nNumber of local Nonzero entries = " << NumMyNonzeros << endl<< endl;

  assert(NumMyNonzeros==NumMyNonzeros1);

  int NumGlobalRows = A.NumGlobalRows();
  if (verbose) cout << "\n\nNumber of global Rows = " << NumGlobalRows << endl<< endl;

  assert(NumGlobalRows==NumGlobalRows1);

  int NumGlobalNonzeros = A.NumGlobalNonzeros();
  if (verbose) cout << "\n\nNumber of global Nonzero entries = " << NumGlobalNonzeros << endl<< endl;

  assert(NumGlobalNonzeros==NumGlobalNonzeros1);

  int NumMyBlockRows = A.NumMyBlockRows();
  if (verbose) cout << "\n\nNumber of local Block Rows = " << NumMyBlockRows << endl<< endl;

  assert(NumMyBlockRows==NumMyBlockRows1);

  int NumMyBlockNonzeros = A.NumMyBlockEntries();
  if (verbose) cout << "\n\nNumber of local Nonzero Block entries = " << NumMyBlockNonzeros << endl<< endl;

  assert(NumMyBlockNonzeros==NumMyBlockNonzeros1);

  int NumGlobalBlockRows = A.NumGlobalBlockRows();
  if (verbose) cout << "\n\nNumber of global Block Rows = " << NumGlobalBlockRows << endl<< endl;

  assert(NumGlobalBlockRows==NumGlobalBlockRows1);

  int NumGlobalBlockNonzeros = A.NumGlobalBlockEntries();
  if (verbose) cout << "\n\nNumber of global Nonzero Block entries = " << NumGlobalBlockNonzeros << endl<< endl;

  assert(NumGlobalNonzeros==NumGlobalNonzeros1);


  // Other binary tests

  assert(!A.NoDiagonal());
  assert(A.Filled());
  assert(A.Sorted());
  assert(A.MyGRID(A.RowMap().MaxMyGID()));
  assert(A.MyGRID(A.RowMap().MinMyGID()));
  assert(!A.MyGRID(1+A.RowMap().MaxMyGID()));
  assert(!A.MyGRID(-1+A.RowMap().MinMyGID()));
  assert(A.MyLRID(0));
  assert(A.MyLRID(NumMyBlockRows-1));
  assert(!A.MyLRID(-1));
  assert(!A.MyLRID(NumMyBlockRows));

    
  int i, j;
  int NumGlobalBlockEntries;
  int NumMyBlockEntries;
  int MaxNumBlockEntries = A.MaxNumBlockEntries();

  // Pointer Extraction approach

  //   Local index
  int MyPointersRowDim, MyPointersNumBlockEntries;
  int * MyPointersBlockIndices = new int[MaxNumBlockEntries];
  int * MyPointersColDims, * MyPointersLDAs;
  double **MyPointersValuesPointers;
  //   Global Index
  int GlobalPointersRowDim, GlobalPointersNumBlockEntries;
  int * GlobalPointersBlockIndices = new int[MaxNumBlockEntries];
  int * GlobalPointersColDims, * GlobalPointersLDAs;
  double **GlobalPointersValuesPointers;

  // Copy Extraction approach

  //   Local index
  int MyCopyRowDim, MyCopyNumBlockEntries;
  int * MyCopyBlockIndices = new int[MaxNumBlockEntries];
  int * MyCopyColDims = new int[MaxNumBlockEntries];
  int * MyCopyLDAs = new int[MaxNumBlockEntries];
  int MaxRowDim = A.MaxRowDim();
  int MaxColDim = A.MaxColDim();
  int MyCopySizeOfValues = MaxRowDim*MaxColDim;
  double ** MyCopyValuesPointers = new double*[MaxNumBlockEntries];
  for (i=0; i<MaxNumBlockEntries; i++)
    MyCopyValuesPointers[i] = new double[MaxRowDim*MaxColDim];

  //   Global Index
  int GlobalCopyRowDim, GlobalCopyNumBlockEntries;
  int * GlobalCopyBlockIndices = new int[MaxNumBlockEntries];
  int * GlobalCopyColDims = new int[MaxNumBlockEntries];
  int * GlobalCopyLDAs = new int[MaxNumBlockEntries];
  
  int GlobalMaxRowDim = A.GlobalMaxRowDim();
  int GlobalMaxColDim = A.GlobalMaxColDim();
  int GlobalCopySizeOfValues = GlobalMaxRowDim*GlobalMaxColDim;
  double ** GlobalCopyValuesPointers = new double*[MaxNumBlockEntries];
  for (i=0; i<MaxNumBlockEntries; i++)
    GlobalCopyValuesPointers[i] = new double[GlobalMaxRowDim*GlobalMaxColDim];

  // View Extraction approaches

  //   Local index (There is no global view available)
  int MyView1RowDim, MyView1NumBlockEntries;
  int * MyView1BlockIndices;
  int * MyView1ColDims, * MyView1LDAs;
  double **MyView1ValuesPointers = new double*[MaxNumBlockEntries];

  //   Local index version 2 (There is no global view available)
  int MyView2RowDim, MyView2NumBlockEntries;
  int * MyView2BlockIndices;
  int * MyView2ColDims, * MyView2LDAs;
  double **MyView2ValuesPointers;


  // For each row, test six approaches to extracting data from a given local index matrix
  for (i=0; i<NumMyBlockRows; i++) {
    int MyRow = i;
    int GlobalRow = A.GRID(i);
    // Get a copy of block indices in local index space, pointers to everything else
    A.ExtractMyBlockRowPointers(MyRow, MaxNumBlockEntries, MyPointersRowDim, 
				    MyPointersNumBlockEntries, MyPointersBlockIndices,
				    MyPointersColDims, MyPointersLDAs,MyPointersValuesPointers);
    // Get a copy of block indices in local index space, pointers to everything else
    A.ExtractGlobalBlockRowPointers(GlobalRow, MaxNumBlockEntries, GlobalPointersRowDim, 
				    GlobalPointersNumBlockEntries, GlobalPointersBlockIndices,
				    GlobalPointersColDims, GlobalPointersLDAs,GlobalPointersValuesPointers);

    // Initiate a copy of block row in local index space.
    A.BeginExtractMyBlockRowCopy(MyRow, MaxNumBlockEntries, MyCopyRowDim, 
				 MyCopyNumBlockEntries, MyCopyBlockIndices,
				 MyCopyColDims);
    // Copy Values
    for (j=0; j<MyCopyNumBlockEntries; j++) {
      A.ExtractEntryCopy(MyCopySizeOfValues, MyCopyValuesPointers[j], MaxRowDim, false);
      MyCopyLDAs[j] = MaxRowDim;
    }

    // Initiate a copy of block row in global index space.
    A.BeginExtractGlobalBlockRowCopy(GlobalRow, MaxNumBlockEntries, GlobalCopyRowDim, 
				    GlobalCopyNumBlockEntries, GlobalCopyBlockIndices,
				    GlobalCopyColDims);
    // Copy Values
    for (j=0; j<GlobalCopyNumBlockEntries; j++) {
      A.ExtractEntryCopy(GlobalCopySizeOfValues, GlobalCopyValuesPointers[j], GlobalMaxRowDim, false);
      GlobalCopyLDAs[j] = GlobalMaxRowDim;
    }

    // Initiate a view of block row in local index space (Version 1)
    A.BeginExtractMyBlockRowView(MyRow, MyView1RowDim, 
				 MyView1NumBlockEntries, MyView1BlockIndices,
				 MyView1ColDims, MyView1LDAs);
    // Set pointers to values
    for (j=0; j<MyView1NumBlockEntries; j++) 
      A.ExtractEntryView(MyView1ValuesPointers[j]);


    // Extract a view of block row in local index space (version 2)
    A.ExtractMyBlockRowView(MyRow, MyView2RowDim, 
				 MyView2NumBlockEntries, MyView2BlockIndices,
				 MyView2ColDims, MyView2LDAs, MyView2ValuesPointers);

    assert(MyPointersNumBlockEntries==GlobalPointersNumBlockEntries);
    assert(MyPointersNumBlockEntries==MyCopyNumBlockEntries);
    assert(MyPointersNumBlockEntries==GlobalCopyNumBlockEntries);
    assert(MyPointersNumBlockEntries==MyView1NumBlockEntries);
    assert(MyPointersNumBlockEntries==MyView2NumBlockEntries);
    for (j=1; j<MyPointersNumBlockEntries; j++) {
      assert(MyCopyBlockIndices[j-1]<MyCopyBlockIndices[j]);
      assert(MyView1BlockIndices[j-1]<MyView1BlockIndices[j]);
      assert(MyView2BlockIndices[j-1]<MyView2BlockIndices[j]);

      assert(GlobalPointersBlockIndices[j]==A.GCID(MyPointersBlockIndices[j]));
      assert(A.LCID(GlobalPointersBlockIndices[j])==MyPointersBlockIndices[j]);
      assert(GlobalPointersBlockIndices[j]==GlobalCopyBlockIndices[j]);
      
      assert(CompareValues(MyPointersValuesPointers[j], MyPointersLDAs[j], 
			   MyPointersRowDim, MyPointersColDims[j], 
			   GlobalPointersValuesPointers[j], GlobalPointersLDAs[j], 
			   GlobalPointersRowDim, GlobalPointersColDims[j])==0);
      assert(CompareValues(MyPointersValuesPointers[j], MyPointersLDAs[j], 
			   MyPointersRowDim, MyPointersColDims[j], 
			   MyCopyValuesPointers[j], MyCopyLDAs[j], 
			   MyCopyRowDim, MyCopyColDims[j])==0);
      assert(CompareValues(MyPointersValuesPointers[j], MyPointersLDAs[j], 
			   MyPointersRowDim, MyPointersColDims[j], 
			   GlobalCopyValuesPointers[j], GlobalCopyLDAs[j], 
			   GlobalCopyRowDim, GlobalCopyColDims[j])==0);
      assert(CompareValues(MyPointersValuesPointers[j], MyPointersLDAs[j], 
			   MyPointersRowDim, MyPointersColDims[j], 
			   MyView1ValuesPointers[j], MyView1LDAs[j], 
			   MyView1RowDim, MyView1ColDims[j])==0);
      assert(CompareValues(MyPointersValuesPointers[j], MyPointersLDAs[j], 
			   MyPointersRowDim, MyPointersColDims[j], 
			   MyView2ValuesPointers[j], MyView2LDAs[j], 
			   MyView2RowDim, MyView2ColDims[j])==0);
    }
  }

  // GlobalRowView should be illegal (since we have local indices)
  assert(A.BeginExtractGlobalBlockRowView(A.GRID(0), MyView1RowDim, 
					  MyView1NumBlockEntries, MyView1BlockIndices,
					  MyView1ColDims, MyView1LDAs)==-2);
  
  // Extract a view of block row in local index space (version 2)
  assert(A.ExtractGlobalBlockRowView(A.GRID(0), MyView2RowDim, 
				     MyView2NumBlockEntries, MyView2BlockIndices,
				     MyView2ColDims, MyView2LDAs, MyView2ValuesPointers)==-2);
  
  delete [] MyPointersBlockIndices;
  delete [] GlobalPointersBlockIndices;
  delete [] MyCopyBlockIndices;
  delete [] MyCopyColDims;
  delete [] MyCopyLDAs;
  for (i=0; i<MaxNumBlockEntries; i++) delete [] MyCopyValuesPointers[i];
  delete [] MyCopyValuesPointers;
  delete [] GlobalCopyBlockIndices;
  delete [] GlobalCopyColDims;
  delete [] GlobalCopyLDAs;
  for (i=0; i<MaxNumBlockEntries; i++) delete [] GlobalCopyValuesPointers[i];
  delete [] GlobalCopyValuesPointers;
  delete [] MyView1ValuesPointers;
  if (verbose) cout << "\n\nRows sorted check OK" << endl<< endl;
  
  return(0);
}

//=============================================================================
int CompareValues(double * A, int LDA, int NumRowsA, int NumColsA, 
		  double * B, int LDB, int NumRowsB, int NumColsB) {
  
  int i, j;
  double * ptr1 = B;
  double * ptr2;
  
  if (NumRowsA!=NumRowsB) return(-2);
  if (NumColsA!=NumColsB) return(-3);
 

    for (j=0; j<NumColsA; j++) {
      ptr1 = B + j*LDB;
      ptr2 = A + j*LDA;
      for (i=0; i<NumRowsA; i++) if (*ptr1++ != *ptr2++) return(-1);
    }
  return(0);
}
