//@HEADER
// ***********************************************************************
// 
//     EpetraExt: Epetra Extended - Linear Algebra Services Package
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
// ***********************************************************************
//@HEADER

#include <EpetraExt_MatrixMatrix.h>

#include <EpetraExt_Transpose_RowMatrix.h>

#include <Epetra_Export.h>
#include <Epetra_Import.h>
#include <Epetra_Util.h>
#include <Epetra_Map.h>
#include <Epetra_Comm.h>
#include <Epetra_CrsMatrix.h>
#include <Epetra_Directory.h>
#include <Epetra_HashTable.h>
#include <Epetra_Distributor.h>

namespace EpetraExt {

//
//Method for internal use... sparsedot forms a dot-product between two
//sparsely-populated 'vectors'.
//Important assumption: assumes the indices in u_ind and v_ind are sorted.
//
double sparsedot(double* u, int* u_ind, int u_len,
		 double* v, int* v_ind, int v_len)
{
  double result = 0.0;

  int v_idx = 0;
  int u_idx = 0;

  while(v_idx < v_len && u_idx < u_len) {
    int ui = u_ind[u_idx];
    int vi = v_ind[v_idx];

    if (ui < vi) {
      ++u_idx;
    }
    else if (ui > vi) {
      ++v_idx;
    }
    else {
      result += u[u_idx++]*v[v_idx++];
    }
  }

  return(result);
}

//struct that holds views of the contents of a CrsMatrix. These
//contents may be a mixture of local and remote rows of the
//original matrix. 
class CrsMatrixStruct {
public:
  CrsMatrixStruct()
    : numRows(0), numEntriesPerRow(NULL), indices(NULL), values(NULL),
      remote(NULL), numRemote(0), rowMap(NULL), colMap(NULL),
      domainMap(NULL), importColMap(NULL), importMatrix(NULL)
  {}

  virtual ~CrsMatrixStruct()
  {
    deleteContents();
  }

  void deleteContents()
  {
    numRows = 0;
    delete [] numEntriesPerRow; numEntriesPerRow = NULL;
    delete [] indices; indices = NULL;
    delete [] values; values = NULL;
    delete [] remote; remote = NULL;
    numRemote = 0;
    delete importMatrix;
  }

  int numRows;
  int* numEntriesPerRow;
  int** indices;
  double** values;
  bool* remote;
  int numRemote;
  const Epetra_Map* origRowMap;
  const Epetra_Map* rowMap;
  const Epetra_Map* colMap;
  const Epetra_Map* domainMap;
  const Epetra_Map* importColMap;
  Epetra_CrsMatrix* importMatrix;
};

int dumpCrsMatrixStruct(const CrsMatrixStruct& M)
{
  cout << "proc " << M.rowMap->Comm().MyPID()<<endl;
  cout << "numRows: " << M.numRows<<endl;
  for(int i=0; i<M.numRows; ++i) {
    for(int j=0; j<M.numEntriesPerRow[i]; ++j) {
      if (M.remote[i]) {
	cout << "  *"<<M.rowMap->GID(i)<<"   "
	     <<M.importColMap->GID(M.indices[i][j])<<"   "<<M.values[i][j]<<endl;
      }
      else {
	cout << "   "<<M.rowMap->GID(i)<<"   "
	     <<M.colMap->GID(M.indices[i][j])<<"   "<<M.values[i][j]<<endl;
      }
    }
  }
  return(0);
}

//kernel method for computing the local portion of C = A*B
int mult_A_B(CrsMatrixStruct& Aview,
	     CrsMatrixStruct& Bview,
	     Epetra_CrsMatrix& C)
{
  int C_firstCol = Bview.colMap->MinLID();
  int C_lastCol = Bview.colMap->MaxLID();

  int C_firstCol_import = 0;
  int C_lastCol_import = -1;

  if (Bview.importColMap != NULL) {
    C_firstCol_import = Bview.importColMap->MinLID();
    C_lastCol_import = Bview.importColMap->MaxLID();
  }

  int C_numCols = C_lastCol - C_firstCol + 1;
  int C_numCols_import = C_lastCol_import - C_firstCol_import + 1;

  double* dwork = new double[C_numCols+C_numCols_import];

  double* C_row_i = dwork;
  double* C_row_i_import = dwork+C_numCols;

  int i, j, k;

  for(j=0; j<C_numCols; ++j) {
    C_row_i[j] = 0.0;
  }

  for(j=0; j<C_numCols_import; ++j) {
    C_row_i_import[j] = 0.0;
  }

  //To form C = A*B we're going to execute this expression:
  //
  // C(i,j) = sum_k( A(i,k)*B(k,j) )
  //
  //Our goal, of course, is to navigate the data in A and B once, without
  //performing searches for column-indices, etc.

  //loop over the rows of A.
  for(i=0; i<Aview.numRows; ++i) {

    //only navigate the local portion of Aview... (It's probable that we
    //imported more of A than we need for A*B, because other cases like A^T*B 
    //need the extra rows.)
    if (Aview.remote[i]) {
      continue;
    }

    int* Aindices_i = Aview.indices[i];
    double* Aval_i  = Aview.values[i];

    //loop across the i-th row of A and for each corresponding row
    //in B, loop across colums and accumulate product
    //A(i,k)*B(k,j) into our partial sum quantities C_row_i. In other words,
    //as we stride across B(k,:) we're calculating updates for row i of the
    //result matrix C.

    for(k=0; k<Aview.numEntriesPerRow[i]; ++k) {
      int Ak = Bview.rowMap->LID(Aview.colMap->GID(Aindices_i[k]));
      double Aval = Aval_i[k];

      int* Bcol_inds = Bview.indices[Ak];
      double* Bvals_k = Bview.values[Ak];

      if (Bview.remote[Ak]) {
	for(j=0; j<Bview.numEntriesPerRow[Ak]; ++j) {
	  int loc = Bcol_inds[j] - C_firstCol_import;
	  C_row_i_import[loc] += Aval*Bvals_k[j];
	}
      }
      else {
	for(j=0; j<Bview.numEntriesPerRow[Ak]; ++j) {
	  int loc = Bcol_inds[j] - C_firstCol;
	  C_row_i[loc] += Aval*Bvals_k[j];
	}
      }
    }

    //
    //Now loop across the C_row_i values and put the non-zeros into C.
    //

    int global_row = Aview.rowMap->GID(i);

    for(j=0; j<C_numCols; ++j) {
      //If this value is zero, don't put it into the C matrix.
      if (C_row_i[j] == 0.0) continue;

      int global_col = Bview.colMap->GID(C_firstCol + j);

      //Now put the C_ij quantity into the result C matrix.
      //
      //Try SumInto first, and if that returns a positive error code (meaning
      //that the location doesn't exist) then use Insert.

      int err = C.SumIntoGlobalValues(global_row, 1, &(C_row_i[j]), &global_col);
      if (err < 0) {
	return(err);
      }
      if (err > 0) {
	err = C.InsertGlobalValues(global_row, 1, &(C_row_i[j]), &global_col);
	if (err < 0) {
	  //If we jump out here, it probably means C is Filled, and doesn't
	  //have all the necessary nonzero locations.
	  return(err);
	}
      }

      C_row_i[j] = 0.0;
    }

    //Now loop across the C_row_i_import values and put the non-zeros into C.

    for(j=0; j<C_numCols_import; ++j) {
      //If this value is zero, don't put it into the C matrix.
      if (C_row_i_import[j] == 0.0) continue;

      int global_col = Bview.importColMap->GID(C_firstCol_import + j);

      //Now put the C_ij quantity into the result C matrix.
      //
      //Try SumInto first, and if that returns a positive error code (meaning
      //that the location doesn't exist) then use Insert.

      int err = C.SumIntoGlobalValues(global_row, 1,
				      &(C_row_i_import[j]), &global_col);
      if (err < 0) {
	return(err);
      }
      if (err > 0) {
	err = C.InsertGlobalValues(global_row, 1,
				   &(C_row_i_import[j]), &global_col);
	if (err < 0) {
	  //If we jump out here, it probably means C is Filled, and doesn't
	  //have all the necessary nonzero locations.
	  return(err);
	}
      }

      C_row_i_import[j] = 0.0;
    }
  }

  delete [] dwork;

  return(0);
}

//kernel method for computing the local portion of C = A*B^T
int mult_A_Btrans(CrsMatrixStruct& Aview,
		  CrsMatrixStruct& Bview,
		  Epetra_CrsMatrix& C)
{
  int i, j, k;
  int returnValue = 0;

  int maxlen = 0;
  for(i=0; i<Aview.numRows; ++i) {
    if (Aview.numEntriesPerRow[i] > maxlen) maxlen = Aview.numEntriesPerRow[i];
  }
  for(i=0; i<Bview.numRows; ++i) {
    if (Bview.numEntriesPerRow[i] > maxlen) maxlen = Bview.numEntriesPerRow[i];
  }

  //cout << "Aview: " << endl;
  //dumpCrsMatrixStruct(Aview);

  //cout << "Bview: " << endl;
  //dumpCrsMatrixStruct(Bview);

  int numBcols = Bview.colMap->NumMyElements();

  int iworklen = maxlen*2;
  int* iwork = new int[iworklen+numBcols];

  int* bcols = iwork+iworklen;
  int* bgids = Bview.colMap->MyGlobalElements();
  double* bvals = new double[maxlen*2];
  double* avals = bvals+maxlen;

  //bcols will hold the GIDs from B's column-map for fast access
  //during the computations below
  for(i=0; i<numBcols; ++i) {
    int blid = Bview.colMap->LID(bgids[i]);
    bcols[blid] = bgids[i];
  }

  Epetra_Util util;

  int* Aind = iwork;
  int* Bind = iwork+maxlen;

  //To form C = A*B^T, we're going to execute this expression:
  //
  // C(i,j) = sum_k( A(i,k)*B(j,k) )
  //
  //This is the easiest case of all to code (easier than A*B, A^T*B, A^T*B^T).
  //But it requires the use of a 'sparsedot' function (we're simply forming
  //dot-products with row A_i and row B_j for all i and j).

  //loop over the rows of A.
  for(i=0; i<Aview.numRows; ++i) {
    if (Aview.remote[i]) {
      continue;
    }

    int* Aindices_i = Aview.indices[i];
    double* Aval_i  = Aview.values[i];
    int A_len_i = Aview.numEntriesPerRow[i];

    for(k=0; k<A_len_i; ++k) {
      Aind[k] = Aview.colMap->GID(Aindices_i[k]);
      avals[k] = Aval_i[k];
    }

    util.Sort(true, A_len_i, Aind, 1, &avals, 0, NULL);

    int global_row = Aview.rowMap->GID(i);

    //loop over the rows of B and form results C_ij = dot(A(i,:),B(j,:))
    for(j=0; j<Bview.numRows; ++j) {
      int* Bindices_j = Bview.indices[j];
      bool remoterows = false;
      for(k=0; k<Bview.numEntriesPerRow[j]; ++k) {
	if (Bview.remote[j]) {
	  Bind[k] = Bview.importColMap->GID(Bindices_j[k]);
          bvals[k] = Bview.values[j][k];
          remoterows = true;
	}
	else {
	  Bind[k] = bcols[Bindices_j[k]];
          bvals[k] = Bview.values[j][k];
	}
      }

      if (remoterows) {
        util.Sort(true, Bview.numEntriesPerRow[j], Bind, 1, &bvals, 0, NULL);
      }

      double C_ij = sparsedot(avals, Aind, A_len_i,
			      bvals, Bind,
			      Bview.numEntriesPerRow[j]);

      if (C_ij == 0.0) {
	continue;
      }
      int global_col = Bview.rowMap->GID(j);

      int err = C.SumIntoGlobalValues(global_row, 1, &C_ij, &global_col);
      if (err < 0) {
	return(err);
      }
      if (err > 0) {
	err = C.InsertGlobalValues(global_row, 1, &C_ij, &global_col);
	if (err < 0) {
	  //If we jump out here, it means C.Filled()==true, and C doesn't
	  //have all the necessary nonzero locations, or that global_row
	  //or global_col is out of range (less than 0 or non local).
	  return(err);
	}
        if (err > 1) {
          cerr << "EpetraExt::MatrixMatrix::Multiply Warning: failed to insert"
              << " value in result matrix at position "<<global_row<<","
              <<global_col<<", possibly because result matrix has a column-map"
              <<" that doesn't include column "<<global_col<<" on this proc."
              <<endl;
          returnValue = err;
        }
      }
    }
  }

  delete [] iwork;
  delete [] bvals;

  return(returnValue);
}

//kernel method for computing the local portion of C = A^T*B
int mult_Atrans_B(CrsMatrixStruct& Aview,
		  CrsMatrixStruct& Bview,
		  Epetra_CrsMatrix& C)
{
  int C_firstCol = Bview.colMap->MinLID();
  int C_lastCol = Bview.colMap->MaxLID();

  int C_firstCol_import = 0;
  int C_lastCol_import = -1;

  if (Bview.importColMap != NULL) {
    C_firstCol_import = Bview.importColMap->MinLID();
    C_lastCol_import = Bview.importColMap->MaxLID();
  }

  int C_numCols = C_lastCol - C_firstCol + 1;
  int C_numCols_import = C_lastCol_import - C_firstCol_import + 1;

  double* dwork = new double[C_numCols+C_numCols_import];

  double* C_row_i = dwork;
  double* C_row_i_import = dwork+C_numCols;

  int i, j, k;

  for(j=0; j<C_numCols; ++j) {
    C_row_i[j] = 0.0;
  }

  for(j=0; j<C_numCols_import; ++j) {
    C_row_i_import[j] = 0.0;
  }

  //To form C = A^T*B, we're going to execute this expression:
  //
  // C(i,j) = sum_k( A(k,i)*B(k,j) )
  //
  //Our goal, of course, is to navigate the data in A and B once, without
  //performing searches for column-indices, etc. In other words, we avoid
  //column-wise operations like the plague...

  //dumpCrsMatrixStruct(Aview);
  //dumpCrsMatrixStruct(Bview);
  int localProc = Bview.colMap->Comm().MyPID();

  int* Arows = Aview.rowMap->MyGlobalElements();

  //loop over the rows of A.
  for(k=0; k<Aview.numRows; ++k) {

    int* Aindices_k = Aview.indices[k];
    double* Aval_k  = Aview.values[k];

    int Bk = Bview.rowMap->LID(Arows[k]);
    if (Bk<0) {
      cout << "mult_Atrans_B ERROR, proc "<<localProc<<" needs row "
	   <<Arows[k]<<" of matrix B, doesn't have it."<<endl;
      return(-1);
    }

    int* Bcol_inds = Bview.indices[Bk];
    double* Bvals_k = Bview.values[Bk];

    //loop across the k-th row of A and for each corresponding row
    //in B, loop across colums and accumulate product
    //A(k,i)*B(k,j) into our partial sum quantities C_row_i.

    for(i=0; i<Aview.numEntriesPerRow[k]; ++i) {
      int Ai = Aindices_k[i];
      double Aval = Aval_k[i];

      int global_row;
      if (Aview.remote[k]) {
	global_row = Aview.importColMap->GID(Ai);
      }
      else {
	global_row = Aview.colMap->GID(Ai);
      }

      if (!C.RowMap().MyGID(global_row)) {
        continue;
      }

      if (Bview.remote[Bk]) {
	for(j=0; j<Bview.numEntriesPerRow[Bk]; ++j) {
	  int loc = Bcol_inds[j] - C_firstCol_import;
	  C_row_i_import[loc] += Aval*Bvals_k[j];
	}
      }
      else {
	for(j=0; j<Bview.numEntriesPerRow[Bk]; ++j) {
	  int loc = Bcol_inds[j] - C_firstCol;
	  C_row_i[loc] += Aval*Bvals_k[j];
	}
      }

      //
      //Now loop across the C_row_i values and put the non-zeros into C.
      //

      for(j=0; j<C_numCols; ++j) {
	//If this value is zero, don't put it into the C matrix.
	if (C_row_i[j] == 0.0) continue;

	int global_col = Bview.colMap->GID(C_firstCol + j);

	//Now put the C_ij quantity into the result C matrix.
	//
	//Try SumInto first, and if that returns a positive error code (meaning
	//that the location doesn't exist) then use Insert.

	int err = C.SumIntoGlobalValues(global_row, 1, &(C_row_i[j]), &global_col);
	if (err < 0) {
	  return(err);
	}
	if (err > 0) {
	  err = C.InsertGlobalValues(global_row, 1, &(C_row_i[j]), &global_col);
	  if (err < 0) {
	    //If we jump out here, it probably means C is Filled, and doesn't
	    //have all the necessary nonzero locations.
	    return(err);
	  }
	}

	C_row_i[j] = 0.0;
      }

      //Now loop across the C_row_i_import values and put the non-zeros into C.

      for(j=0; j<C_numCols_import; ++j) {
	//If this value is zero, don't put it into the C matrix.
	if (C_row_i_import[j] == 0.0) continue;

	int global_col = Bview.importColMap->GID(C_firstCol_import + j);

	//Now put the C_ij quantity into the result C matrix.
	//
	//Try SumInto first, and if that returns a positive error code (meaning
	//that the location doesn't exist) then use Insert.

	int err = C.SumIntoGlobalValues(global_row, 1,
					&(C_row_i_import[j]), &global_col);
	if (err < 0) {
	  return(err);
	}
	if (err > 0) {
	  err = C.InsertGlobalValues(global_row, 1,
				     &(C_row_i_import[j]), &global_col);
	  if (err < 0) {
	    //If we jump out here, it probably means C is Filled, and doesn't
	    //have all the necessary nonzero locations.
	    return(err);
	  }
	}

	C_row_i_import[j] = 0.0;
      }
    }
  }

  delete [] dwork;

  return(0);
}

//kernel method for computing the local portion of C = A^T*B^T
int mult_Atrans_Btrans(CrsMatrixStruct& Aview,
		       CrsMatrixStruct& Bview,
		       Epetra_CrsMatrix& C)
{
  int C_firstCol = Aview.rowMap->MinLID();
  int C_lastCol = Aview.rowMap->MaxLID();

  int C_firstCol_import = 0;
  int C_lastCol_import = -1;

  if (Aview.importColMap != NULL) {
    C_firstCol_import = Aview.importColMap->MinLID();
    C_lastCol_import = Aview.importColMap->MaxLID();
  }

  int C_numCols = C_lastCol - C_firstCol + 1;
  int C_numCols_import = C_lastCol_import - C_firstCol_import + 1;

  double* dwork = new double[C_numCols+C_numCols_import];

  double* C_col_j = dwork;

  double* C_col_j_import = dwork+C_numCols;

  //cout << "Aview: " << endl;
  //dumpCrsMatrixStruct(Aview);

  //cout << "Bview: " << endl;
  //dumpCrsMatrixStruct(Bview);


  int i, j, k;

  for(j=0; j<C_numCols; ++j) {
    C_col_j[j] = 0.0;
  }

  for(j=0; j<C_numCols_import; ++j) {
    C_col_j_import[j] = 0.0;
  }

  const Epetra_Map* Crowmap = &(C.RowMap());

  //To form C = A^T*B^T, we're going to execute this expression:
  //
  // C(i,j) = sum_k( A(k,i)*B(j,k) )
  //
  //Our goal, of course, is to navigate the data in A and B once, without
  //performing searches for column-indices, etc. In other words, we avoid
  //column-wise operations like the plague...

  int* Brows = Bview.rowMap->MyGlobalElements();

  //loop across the rows of B
  for(j=0; j<Bview.numRows; ++j) {
    int* Bindices_j = Bview.indices[j];
    double* Bvals_j = Bview.values[j];

    int global_col = Brows[j];

    //loop across columns in the j-th row of B and for each corresponding
    //row in A, loop across columns and accumulate product
    //A(k,i)*B(j,k) into our partial sum quantities in C_col_j. In other
    //words, as we stride across B(j,:), we use selected rows in A to
    //calculate updates for column j of the result matrix C.

    for(k=0; k<Bview.numEntriesPerRow[j]; ++k) {
      int bk = Bindices_j[k];
      double Bval = Bvals_j[k];

      int global_k;
      if (Bview.remote[j]) {
	global_k = Bview.importColMap->GID(bk);
      }
      else {
	global_k = Bview.colMap->GID(bk);
      }

      //get the corresponding row in A
      int ak = Aview.rowMap->LID(global_k);
      if (ak<0) {
	continue;
      }

      int* Aindices_k = Aview.indices[ak];
      double* Avals_k = Aview.values[ak];

      if (Aview.remote[ak]) {
	for(i=0; i<Aview.numEntriesPerRow[ak]; ++i) {
	  int loc = Aindices_k[i] - C_firstCol_import;
	  C_col_j_import[loc] += Avals_k[i]*Bval;
	}
      }
      else {
	for(i=0; i<Aview.numEntriesPerRow[ak]; ++i) {
	  int loc = Aindices_k[i] - C_firstCol;
	  C_col_j[loc] += Avals_k[i]*Bval;
	}
      }

      //Now loop across the C_col_j values and put non-zeros into C.

      for(i=0; i<C_numCols; ++i) {
	if (C_col_j[i] == 0.0) continue;

	int global_row = Aview.colMap->GID(C_firstCol+i);
	if (!Crowmap->MyGID(global_row)) {
	  continue;
	}

	int err = C.SumIntoGlobalValues(global_row, 1, &(C_col_j[i]),
					&global_col);
	if (err < 0) {
	  return(err);
	}
	if (err > 0) {
	  err = C.InsertGlobalValues(global_row, 1, &(C_col_j[i]),
				     &global_col);
	  if (err < 0) {
	    return(err);
	  }
	}

	C_col_j[i] = 0.0;
      }

      for(i=0; i<C_numCols_import; ++i) {
	if (C_col_j_import[i] == 0.0) continue;

	int global_row = Aview.importColMap->GID(C_firstCol_import + i);
	if (!Crowmap->MyGID(global_row)) {
	  continue;
	}

	int err = C.SumIntoGlobalValues(global_row, 1, &(C_col_j_import[i]),
					&global_col);
	if (err < 0) {
	  return(err);
	}
	if (err > 0) {
	  err = C.InsertGlobalValues(global_row, 1, &(C_col_j_import[i]),
				     &global_col);
	  if (err < 0) {
	    return(err);
	  }
	}

	C_col_j_import[i] = 0.0;
      }
    }
  }

  delete [] dwork;

  return(0);
}

int import_and_extract_views(const Epetra_CrsMatrix& M,
			     const Epetra_Map& targetMap,
			     CrsMatrixStruct& Mview)
{
  //The goal of this method is to populate the 'Mview' struct with views of the
  //rows of M, including all rows that correspond to elements in 'targetMap'.
  //
  //If targetMap includes local elements that correspond to remotely-owned rows
  //of M, then those remotely-owned rows will be imported into
  //'Mview.importMatrix', and views of them will be included in 'Mview'.

  Mview.deleteContents();

  const Epetra_Map& Mrowmap = M.RowMap();

  int numProcs = Mrowmap.Comm().NumProc();

  Mview.numRows = targetMap.NumMyElements();

  int* Mrows = targetMap.MyGlobalElements();

  if (Mview.numRows > 0) {
    Mview.numEntriesPerRow = new int[Mview.numRows];
    Mview.indices = new int*[Mview.numRows];
    Mview.values = new double*[Mview.numRows];
    Mview.remote = new bool[Mview.numRows];
  }

  Mview.numRemote = 0;

  int i;
  for(i=0; i<Mview.numRows; ++i) {
    int mlid = Mrowmap.LID(Mrows[i]);
    if (mlid < 0) {
      Mview.remote[i] = true;
      ++Mview.numRemote;
    }
    else {
      EPETRA_CHK_ERR( M.ExtractMyRowView(mlid, Mview.numEntriesPerRow[i],
					 Mview.values[i], Mview.indices[i]) );
      Mview.remote[i] = false;
    }
  }

  Mview.origRowMap = &(M.RowMap());
  Mview.rowMap = &targetMap;
  Mview.colMap = &(M.ColMap());
  Mview.domainMap = &(M.DomainMap());
  Mview.importColMap = NULL;

  if (numProcs < 2) {
    if (Mview.numRemote > 0) {
      cerr << "EpetraExt::MatrixMatrix::Multiply ERROR, numProcs < 2 but "
	   << "attempting to import remote matrix rows."<<endl;
      return(-1);
    }

    //If only one processor we don't need to import any remote rows, so return.
    return(0);
  }

  //
  //Now we will import the needed remote rows of M, if the global maximum
  //value of numRemote is greater than 0.
  //

  int globalMaxNumRemote = 0;
  Mrowmap.Comm().MaxAll(&Mview.numRemote, &globalMaxNumRemote, 1);

  if (globalMaxNumRemote > 0) {
    //Create a map that describes the remote rows of M that we need.

    int* MremoteRows = Mview.numRemote>0 ? new int[Mview.numRemote] : NULL;
    int offset = 0;
    for(i=0; i<Mview.numRows; ++i) {
      if (Mview.remote[i]) {
	MremoteRows[offset++] = Mrows[i];
      }
    }

    Epetra_Map MremoteRowMap(-1, Mview.numRemote, MremoteRows,
			     Mrowmap.IndexBase(), Mrowmap.Comm());

    //Create an importer with target-map MremoteRowMap and 
    //source-map Mrowmap.
    Epetra_Import importer(MremoteRowMap, Mrowmap);

    //Now create a new matrix into which we can import the remote rows of M
    //that we need.
    Mview.importMatrix = new Epetra_CrsMatrix(Copy, MremoteRowMap, 1);

    EPETRA_CHK_ERR( Mview.importMatrix->Import(M, importer, Insert) );

    EPETRA_CHK_ERR( Mview.importMatrix->FillComplete(M.DomainMap(), M.RangeMap()) );

    //Finally, use the freshly imported data to fill in the gaps in our views
    //of rows of M.
    for(i=0; i<Mview.numRows; ++i) {
      if (Mview.remote[i]) {
	int importLID = MremoteRowMap.LID(Mrows[i]);
	EPETRA_CHK_ERR( Mview.importMatrix->ExtractMyRowView(importLID,
						  Mview.numEntriesPerRow[i],
						  Mview.values[i],
						  Mview.indices[i]) );
      }
    }

    Mview.importColMap = &(Mview.importMatrix->ColMap());

    delete [] MremoteRows;
  }

  return(0);
}

int distribute_list(const Epetra_Comm& Comm,
                    int lenSendList,
                    const int* sendList,
                    int& maxSendLen,
                    int*& recvList)
{
  maxSendLen = 0; 
  Comm.MaxAll(&lenSendList, &maxSendLen, 1);
  int numProcs = Comm.NumProc();
  recvList = new int[numProcs*maxSendLen];
  int* send = new int[maxSendLen];
  for(int i=0; i<lenSendList; ++i) {
    send[i] = sendList[i];
  }

  Comm.GatherAll(send, recvList, maxSendLen);
  delete [] send;

  return(0);
}

Epetra_Map* create_map_from_imported_rows(const Epetra_Map* map,
					  int totalNumSend,
					  int* sendRows,
					  int numProcs,
					  int* numSendPerProc)
{
  //Perform sparse all-to-all communication to send the row-GIDs
  //in sendRows to appropriate processors according to offset
  //information in numSendPerProc.
  //Then create and return a map containing the rows that we
  //received on the local processor.

  Epetra_Distributor* distributor = map->Comm().CreateDistributor();

  int* sendPIDs = totalNumSend>0 ? new int[totalNumSend] : NULL;
  int offset = 0;
  for(int i=0; i<numProcs; ++i) {
    for(int j=0; j<numSendPerProc[i]; ++j) {
      sendPIDs[offset++] = i;
    }
  }

  int numRecv = 0;
  int err = distributor->CreateFromSends(totalNumSend, sendPIDs,
					 true, numRecv);
  assert( err == 0 );

  char* c_recv_objs = numRecv>0 ? new char[numRecv*sizeof(int)] : NULL;
  int num_c_recv = numRecv*sizeof(int);

  err = distributor->Do(reinterpret_cast<char*>(sendRows),
			sizeof(int), num_c_recv, c_recv_objs);
  assert( err == 0 );

  int* recvRows = reinterpret_cast<int*>(c_recv_objs);

  //Now create a map with the rows we've received from other processors.
  Epetra_Map* import_rows = new Epetra_Map(-1, numRecv, recvRows,
					   map->IndexBase(), map->Comm());

  delete [] c_recv_objs;
  delete [] sendPIDs;

  delete distributor;

  return( import_rows );
}

int form_map_union(const Epetra_Map* map1,
		   const Epetra_Map* map2,
		   const Epetra_Map*& mapunion)
{
  //form the union of two maps

  if (map1 == NULL) {
    mapunion = new Epetra_Map(*map2);
    return(0);
  }

  if (map2 == NULL) {
    mapunion = new Epetra_Map(*map1);
    return(0);
  }

  int map1_len       = map1->NumMyElements();
  int* map1_elements = map1->MyGlobalElements();
  int map2_len       = map2->NumMyElements();
  int* map2_elements = map2->MyGlobalElements();

  int* union_elements = new int[map1_len+map2_len];

  int map1_offset = 0, map2_offset = 0, union_offset = 0;

  while(map1_offset < map1_len && map2_offset < map2_len) {
    int map1_elem = map1_elements[map1_offset];
    int map2_elem = map2_elements[map2_offset];

    if (map1_elem < map2_elem) {
      union_elements[union_offset++] = map1_elem;
      ++map1_offset;
    }
    else if (map1_elem > map2_elem) {
      union_elements[union_offset++] = map2_elem;
      ++map2_offset;
    }
    else {
      union_elements[union_offset++] = map1_elem;
      ++map1_offset;
      ++map2_offset;
    }
  }

  int i;
  for(i=map1_offset; i<map1_len; ++i) {
    union_elements[union_offset++] = map1_elements[i];
  }

  for(i=map2_offset; i<map2_len; ++i) {
    union_elements[union_offset++] = map2_elements[i];
  }

  mapunion = new Epetra_Map(-1, union_offset, union_elements,
			    map1->IndexBase(), map1->Comm());

  delete [] union_elements;

  return(0);
}

Epetra_Map* find_rows_containing_cols(const Epetra_CrsMatrix& M,
                                      const Epetra_Map* colmap)
{
  //The goal of this function is to find all rows in the matrix M that contain
  //column-indices which are in 'colmap'. A map containing those rows is
  //returned.

  int numProcs = colmap->Comm().NumProc();
  int localProc = colmap->Comm().MyPID();

  if (numProcs < 2) {
    Epetra_Map* result_map = NULL;

    int err = form_map_union(&(M.RowMap()), NULL,
                             (const Epetra_Map*&)result_map);
    if (err != 0) {
      return(NULL);
    }
    return(result_map);
  }

  int MnumRows = M.NumMyRows();
  int numCols = colmap->NumMyElements();

  int* iwork = new int[numCols+2*numProcs+numProcs*MnumRows];
  int iworkOffset = 0;

  int* cols = &(iwork[iworkOffset]); iworkOffset += numCols;

  cols[0] = numCols;
  colmap->MyGlobalElements( &(cols[1]) );

  //cols are not necessarily sorted at this point, so we'll make sure
  //they are sorted.
  Epetra_Util util;
  util.Sort(true, numCols, &(cols[1]), 0, NULL, 0, NULL);

  int* all_proc_cols = NULL;
  
  int max_num_cols;
  distribute_list(colmap->Comm(), numCols+1, cols, max_num_cols, all_proc_cols);

  const Epetra_CrsGraph& Mgraph = M.Graph();
  const Epetra_Map& Mrowmap = M.RowMap();
  const Epetra_Map& Mcolmap = M.ColMap();
  int MminMyLID = Mrowmap.MinLID();

  int* procNumCols = &(iwork[iworkOffset]); iworkOffset += numProcs;
  int* procNumRows = &(iwork[iworkOffset]); iworkOffset += numProcs;
  int* procRows_1D = &(iwork[iworkOffset]);
  int** procCols = new int*[numProcs];
  int** procRows = new int*[numProcs];
  int i, err;
  int offset = 0;
  for(i=0; i<numProcs; ++i) {
    procNumCols[i] = all_proc_cols[offset];
    procCols[i] = &(all_proc_cols[offset+1]);
    offset += max_num_cols;

    procNumRows[i] = 0;
    procRows[i] = &(procRows_1D[i*MnumRows]);
  }

  int* Mindices;

  for(int row=0; row<MnumRows; ++row) {
    int localRow = MminMyLID+row;
    int globalRow = Mrowmap.GID(localRow);
    int MnumCols;
    err = Mgraph.ExtractMyRowView(localRow, MnumCols, Mindices);
    if (err != 0) {
      cerr << "proc "<<localProc<<", error in Mgraph.ExtractMyRowView, row "
           <<localRow<<endl;
      return(NULL);
    }

    for(int j=0; j<MnumCols; ++j) {
      int colGID = Mcolmap.GID(Mindices[j]);

      for(int p=0; p<numProcs; ++p) {
        if (p==localProc) continue;

        int insertPoint;
        int foundOffset = Epetra_Util_binary_search(colGID, procCols[p],
                                                    procNumCols[p], insertPoint);
        if (foundOffset > -1) {
          int numRowsP = procNumRows[p];
          int* prows = procRows[p];
          if (numRowsP < 1 || prows[numRowsP-1] < globalRow) {
            prows[numRowsP] = globalRow;
            procNumRows[p]++;
          }
        }
      }
    }
  }

  //Now make the contents of procRows occupy a contiguous section
  //of procRows_1D.
  offset = procNumRows[0];
  for(i=1; i<numProcs; ++i) {
    for(int j=0; j<procNumRows[i]; ++j) {
      procRows_1D[offset++] = procRows[i][j];
    }
  }

  int totalNumSend = offset;
  //Next we will do a sparse all-to-all communication to send the lists of rows
  //to the appropriate processors, and create a map with the rows we've received
  //from other processors.
  Epetra_Map* recvd_rows =
    create_map_from_imported_rows(&Mrowmap, totalNumSend,
                                  procRows_1D, numProcs, procNumRows);

  Epetra_Map* result_map = NULL;

  err = form_map_union(&(M.RowMap()), recvd_rows, (const Epetra_Map*&)result_map);
  if (err != 0) {
    return(NULL);
  }

  delete [] iwork;
  delete [] procCols;
  delete [] procRows;
  delete [] all_proc_cols;
  delete recvd_rows;

  return(result_map);
}

int MatrixMatrix::Multiply(const Epetra_CrsMatrix& A,
			   bool transposeA,
			   const Epetra_CrsMatrix& B,
			   bool transposeB,
			   Epetra_CrsMatrix& C)
{
  //
  //This method forms the matrix-matrix product C = op(A) * op(B), where
  //op(A) == A   if transposeA is false,
  //op(A) == A^T if transposeA is true,
  //and similarly for op(B).
  //

  //A and B should already be Filled.
  //(Should we go ahead and call FillComplete() on them if necessary?
  // or error out? For now, we choose to error out.)
  if (!A.Filled() || !B.Filled()) {
    EPETRA_CHK_ERR(-1);
  }

  //We're going to refer to the different combinations of op(A) and op(B)
  //as scenario 1 through 4.

  int scenario = 1;//A*B
  if (transposeB && !transposeA) scenario = 2;//A*B^T
  if (transposeA && !transposeB) scenario = 3;//A^T*B
  if (transposeA && transposeB)  scenario = 4;//A^T*B^T

  //now check size compatibility
  int Aouter = transposeA ? A.NumGlobalCols() : A.NumGlobalRows();
  int Bouter = transposeB ? B.NumGlobalRows() : B.NumGlobalCols();
  int Ainner = transposeA ? A.NumGlobalRows() : A.NumGlobalCols();
  int Binner = transposeB ? B.NumGlobalCols() : B.NumGlobalRows();
  if (Ainner != Binner) {
    cerr << "MatrixMatrix::Multiply: ERROR, inner dimensions of op(A) and op(B) "
         << "must match for matrix-matrix product. op(A) is "
         <<Aouter<<"x"<<Ainner << ", op(B) is "<<Binner<<"x"<<Bouter<<endl;
    return(-1);
  }

  //The result matrix C must at least have a row-map that reflects the
  //correct row-size. Don't check the number of columns because rectangular
  //matrices which were constructed with only one map can still end up
  //having the correct capacity and dimensions when filled.
  if (Aouter > C.NumGlobalRows()) {
    cerr << "MatrixMatrix::Multiply: ERROR, dimensions of result C must "
         << "match dimensions of op(A) * op(B). C has "<<C.NumGlobalRows()
         << " rows, should have at least "<<Aouter << endl;
    return(-1);
  }

  //It doesn't matter whether C is already Filled or not. If it is already
  //Filled, it must have space allocated for the positions that will be
  //referenced in forming C = op(A)*op(B). If it doesn't have enough space,
  //we'll error out later when trying to store result values.

  //We're going to need to import remotely-owned sections of A and/or B
  //if more than 1 processor is performing this run, depending on the scenario.
  int numProcs = A.Comm().NumProc();

  //If we are to use the transpose of A and/or B, we'll need to be able to 
  //access, on the local processor, all rows that contain column-indices in
  //the domain-map.
  const Epetra_Map* domainMap_A = &(A.DomainMap());
  const Epetra_Map* domainMap_B = &(B.DomainMap());

  const Epetra_Map* rowmap_A = &(A.RowMap());
  const Epetra_Map* rowmap_B = &(B.RowMap());

  //Declare some 'work-space' maps which may be created depending on
  //the scenario, and which will be deleted before exiting this function.
  const Epetra_Map* workmap1 = NULL;
  const Epetra_Map* workmap2 = NULL;
  const Epetra_Map* mapunion1 = NULL;

  //Declare a couple of structs that will be used to hold views of the data
  //of A and B, to be used for fast access during the matrix-multiplication.
  CrsMatrixStruct Aview;
  CrsMatrixStruct Bview;

  const Epetra_Map* targetMap_A = rowmap_A;
  const Epetra_Map* targetMap_B = rowmap_B;

  if (numProcs > 1) {
    //If op(A) = A^T, find all rows of A that contain column-indices in the
    //local portion of the domain-map. (We'll import any remote rows
    //that fit this criteria onto the local processor.)
    if (transposeA) {
      workmap1 = find_rows_containing_cols(A, domainMap_A);
      targetMap_A = workmap1;
    }
  }

  //Now import any needed remote rows and populate the Aview struct.
  EPETRA_CHK_ERR( import_and_extract_views(A, *targetMap_A, Aview) );

  //We will also need local access to all rows of B that correspond to the
  //column-map of op(A).
  if (numProcs > 1) {
    const Epetra_Map* colmap_op_A = NULL;
    if (transposeA) {
      colmap_op_A = targetMap_A;
    }
    else {
      colmap_op_A = &(A.ColMap());
    }

    targetMap_B = colmap_op_A;

    //If op(B) = B^T, find all rows of B that contain column-indices in the
    //local-portion of the domain-map, or in the column-map of op(A).
    //We'll import any remote rows that fit this criteria onto the
    //local processor.
    if (transposeB) {
      EPETRA_CHK_ERR( form_map_union(colmap_op_A, domainMap_B, mapunion1) );
      workmap2 = find_rows_containing_cols(B, mapunion1);
      targetMap_B = workmap2;
    }
  }

  //Now import any needed remote rows and populate the Bview struct.
  EPETRA_CHK_ERR( import_and_extract_views(B, *targetMap_B, Bview) );

  //zero the result matrix before we start the calculations.
  EPETRA_CHK_ERR( C.PutScalar(0.0) );


  //Now call the appropriate method to perform the actual multiplication.

  switch(scenario) {
  case 1:    EPETRA_CHK_ERR( mult_A_B(Aview, Bview, C) );
    break;
  case 2:    EPETRA_CHK_ERR( mult_A_Btrans(Aview, Bview, C) );
    break;
  case 3:    EPETRA_CHK_ERR( mult_Atrans_B(Aview, Bview, C) );
    break;
  case 4:    EPETRA_CHK_ERR( mult_Atrans_Btrans(Aview, Bview, C) );
    break;
  }


  //We'll call FillComplete on the C matrix before we exit, and give
  //it a domain-map and a range-map.
  //The domain-map will be the domain-map of B, unless
  //op(B)==transpose(B), in which case the range-map of B will be used.
  //The range-map will be the range-map of A, unless
  //op(A)==transpose(A), in which case the domain-map of A will be used.

  const Epetra_Map* domainmap =
    transposeB ? &(B.RangeMap()) : &(B.DomainMap());

  const Epetra_Map* rangemap =
    transposeA ? &(A.DomainMap()) : &(A.RangeMap());

  if (!C.Filled()) {
    EPETRA_CHK_ERR( C.FillComplete(*domainmap, *rangemap) );
  }


  //Finally, delete the objects that were potentially created
  //during the course of importing remote sections of A and B.

  delete mapunion1; mapunion1 = NULL;
  delete workmap1; workmap1 = NULL;
  delete workmap2; workmap2 = NULL;

  return(0);
}

int MatrixMatrix::Add(const Epetra_CrsMatrix& A,
                      bool transposeA,
                      double scalarA,
                      Epetra_CrsMatrix& B,
                      double scalarB )
{
  //
  //This method forms the matrix-matrix sum B = scalarA * op(A) + scalarB * B, where

  //A should already be Filled. It doesn't matter whether B is
  //already Filled, but if it is, then its graph must already contain
  //all nonzero locations that will be referenced in forming the
  //sum.

  if (!A.Filled() ) EPETRA_CHK_ERR(-1);

  //explicit tranpose A formed as necessary
  Epetra_CrsMatrix * Aprime = 0;
  EpetraExt::RowMatrix_Transpose * Atrans = 0;
  if( transposeA )
  {
    Atrans = new EpetraExt::RowMatrix_Transpose();
    Aprime = &(dynamic_cast<Epetra_CrsMatrix&>(((*Atrans)(const_cast<Epetra_CrsMatrix&>(A)))));
  }
  else
    Aprime = const_cast<Epetra_CrsMatrix*>(&A);

  //Initialize if B already filled
  if( B.Filled() )
    EPETRA_CHK_ERR( B.Scale( scalarB ) );

  //Loop over B's rows and sum into
  int MaxNumEntries = EPETRA_MAX( A.MaxNumEntries(), B.MaxNumEntries() );
  int NumEntries;
  int * Indices = new int[MaxNumEntries];
  double * Values = new double[MaxNumEntries];

  int NumMyRows = B.NumMyRows();
  int Row, err;

  if( scalarA )
  {
    for( int i = 0; i < NumMyRows; ++i )
    {
      Row = B.GRID(i);
      EPETRA_CHK_ERR( A.ExtractGlobalRowCopy( Row, MaxNumEntries, NumEntries, Values, Indices ) );
      if( scalarA != 1.0 )
        for( int j = 0; j < NumEntries; ++j ) Values[j] *= scalarA;
      if( B.Filled() ) {//Sum In Values
        err = B.SumIntoGlobalValues( Row, NumEntries, Values, Indices );
        assert( err == 0 );
      }
      else {
        err = B.InsertGlobalValues( Row, NumEntries, Values, Indices );
        assert( err == 0 || err == 1 );
      }
    }
  }

  delete [] Indices;
  delete [] Values;

  if( Atrans ) delete Atrans;

  if( !B.Filled() ) 
    EPETRA_CHK_ERR( B.FillComplete() );

  return(0);
}

} // namespace EpetraExt

