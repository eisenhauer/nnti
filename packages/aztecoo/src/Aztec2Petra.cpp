
/* Copyright (2001) Sandia Corportation. Under the terms of Contract 
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this 
 * work by or on behalf of the U.S. Government.  Export of this program
 * may require a license from the United States Government. */


/* NOTICE:  The United States Government is granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable worldwide
 * license in ths data to reproduce, prepare derivative works, and
 * perform publicly and display publicly.  Beginning five (5) years from
 * July 25, 2001, the United States Government is granted for itself and
 * others acting on its behalf a paid-up, nonexclusive, irrevocable
 * worldwide license in this data to reproduce, prepare derivative works,
 * distribute copies to the public, perform publicly and display
 * publicly, and to permit others to do so.
 * 
 * NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT
 * OF ENERGY, NOR SANDIA CORPORATION, NOR ANY OF THEIR EMPLOYEES, MAKES
 * ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR
 * RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY
 * INFORMATION, APPARATUS, PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS
 * THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS. */

#include "Aztec2Petra.h"

int Aztec2Petra(int * proc_config,
		AZ_MATRIX * Amat, double * az_x, double * az_b,
		Epetra_Comm * & comm,
		Epetra_BlockMap * & map,
		Epetra_RowMatrix * &A,
		Epetra_Vector * & x,
		Epetra_Vector * & b) {

  // Build Epetra_Comm object

#ifdef AZTEC_MPI
    MPI_Comm * mpicomm = (MPI_Comm * ) AZ_get_comm(proc_config);
    comm = (Epetra_Comm *) new Epetra_MpiComm(*mpicomm);
#else
    comm = (Epetra_Comm *) new Epetra_SerialComm();
#endif  

  int * MyGlobalElements, * global_bindx, *update;
  
  if (!Amat->has_global_indices) {
    //create a global bindx
    AZ_revert_to_global(proc_config, Amat, &global_bindx, &update);
    MyGlobalElements = update;
  }
  else // Already have global ordering
    {
      global_bindx = Amat->bindx;
      MyGlobalElements = Amat->update;
      if (MyGlobalElements==0) EPETRA_CHK_ERR(-1);
    }

  // Get matrix information
  int NumMyElements = 0;
  if (Amat->data_org[AZ_matrix_type] == AZ_VBR_MATRIX)
    NumMyElements = Amat->data_org[AZ_N_int_blk] + Amat->data_org[AZ_N_bord_blk];
  else
    NumMyElements = Amat->data_org[AZ_N_internal] + Amat->data_org[AZ_N_border];
  // int NumMyElements = Amat->N_update; // Note: This "official" way does not always work
  int * bpntr = Amat->bpntr;
  int * rpntr = Amat->rpntr;
  int * indx = Amat->indx;
  double * val = Amat->val;

  int NumGlobalElements;
  comm->SumAll(&NumMyElements, &NumGlobalElements, 1);


  // Make ElementSizeList (if VBR) - number of block entries in each block row

  int * ElementSizeList = 0;

  if (Amat->data_org[AZ_matrix_type] == AZ_VBR_MATRIX) {
  
    ElementSizeList = new int[NumMyElements];
    if (ElementSizeList==0) EPETRA_CHK_ERR(-1); // Ran out of memory
    
    for (int i=0; i<NumMyElements; i++) ElementSizeList[i] = rpntr[i+1] - rpntr[i];

    map = new Epetra_BlockMap(NumGlobalElements, NumMyElements, MyGlobalElements, 
			     ElementSizeList, 0, *comm);

    if (map==0) EPETRA_CHK_ERR(-2); // Ran out of memory

    delete [] ElementSizeList;
 
    Epetra_VbrMatrix * AA = new Epetra_VbrMatrix(Copy, *map, 0);
  
    if (AA==0) EPETRA_CHK_ERR(-3); // Ran out of memory

    /* Add block rows one-at-a-time */
    for (int i=0; i<NumMyElements; i++) {
      int BlockRow = MyGlobalElements[i];
      int NumBlockEntries = bpntr[i+1] - bpntr[i];
      int *BlockIndices = global_bindx + bpntr[i];
      int ierr = AA->BeginInsertGlobalValues(BlockRow, NumBlockEntries, BlockIndices);
      if (ierr!=0) {
	cerr << "Error in BeginInsertGlobalValues(GlobalBlockRow = " << BlockRow 
	     << ") = " << ierr << endl; 
	EPETRA_CHK_ERR(ierr);
      }
      int LDA = rpntr[i+1] - rpntr[i];
      int NumRows = LDA;
      for (int j=bpntr[i]; j<bpntr[i+1]; j++) {
	int NumCols = (indx[j+1] - indx[j])/LDA;
	double * Values = val + indx[j];
	ierr = AA->SubmitBlockEntry(Values, LDA, NumRows, NumCols);
	if (ierr!=0) {
	  cerr << "Error in SubmitBlockEntry, GlobalBlockRow = " << BlockRow 
	       << "GlobalBlockCol = " << BlockIndices[j] << "Error = " << ierr << endl; 
	  EPETRA_CHK_ERR(ierr);
	}
      }
      ierr = AA->EndSubmitEntries();
      if (ierr!=0) {
	cerr << "Error in EndSubmitEntries(GlobalBlockRow = " << BlockRow 
	     << ") = " << ierr << endl; 
	EPETRA_CHK_ERR(ierr);
      }
    }  
    int ierr=AA->TransformToLocal();    
    if (ierr!=0) {
      cerr <<"Error in Epetra_VbrMatrix TransformToLocal" << ierr << endl;
      EPETRA_CHK_ERR(ierr);
    }

    A = (Epetra_RowMatrix *) AA; // cast VBR pointer to RowMatrix pointer
  }
  else if  (Amat->data_org[AZ_matrix_type] == AZ_MSR_MATRIX) {
  
    /* Make numNzBlks - number of block entries in each block row */

    int * numNz = new int[NumMyElements];
    for (int i=0; i<NumMyElements; i++) numNz[i] = global_bindx[i+1] - global_bindx[i] + 1;

    Epetra_Map * map1 = new Epetra_Map(NumGlobalElements, NumMyElements,
				     MyGlobalElements, 0, *comm);

    Epetra_CrsMatrix * AA = new Epetra_CrsMatrix(Copy, *map1, numNz);

    map = (Epetra_BlockMap *) map1; // cast Epetra_Map to Epetra_BlockMap

    /* Add  rows one-at-a-time */

    for (int row=0; row<NumMyElements; row++) {
      double * row_vals = val + global_bindx[row];
      int * col_inds = global_bindx + global_bindx[row];
      int numEntries = global_bindx[row+1] - global_bindx[row];
      int ierr = AA->InsertGlobalValues(MyGlobalElements[row], numEntries, row_vals, col_inds);
      if (ierr!=0) {
	cerr << "Error puting row " << MyGlobalElements[row] << endl;
	EPETRA_CHK_ERR(ierr);
      }
      ierr = AA->InsertGlobalValues(MyGlobalElements[row], 1, val+row, MyGlobalElements+row);
      if (ierr!=0) {
	cerr << "Error putting  diagonal" << endl;
	EPETRA_CHK_ERR(ierr);
      }
    }

    int ierr=AA->TransformToLocal();
    if (ierr!=0) {
      cerr << "Error in Epetra_CrsMatrix_TransformToLocal" << endl;
      EPETRA_CHK_ERR(ierr);
    }
    A = (Epetra_RowMatrix *) AA; // cast CRS pointer to RowMatrix pointer
  }
  else cerr << "Not a supported AZ_MATRIX data type" << endl;

  // Create x vector, note that it is a "long" vector (has ghost entries).
  x = new Epetra_Vector(View, A->BlockImportMap(),az_x);

  b = new Epetra_Vector (View, *map, az_b);

  if (!Amat->has_global_indices) {
    AZ_free((void *) global_bindx);
    AZ_free((void *) update);
  }
  return 0;
}
