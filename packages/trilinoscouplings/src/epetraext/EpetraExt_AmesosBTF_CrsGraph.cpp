// @HEADER
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
// @HEADER

#include <EpetraExt_AmesosBTF_CrsGraph.h>
#include <EpetraExt_Transpose_CrsGraph.h>

#include <Epetra_Import.h>
#include <Epetra_CrsGraph.h>
#include <Epetra_Map.h>

#include <btf.h>

#include <vector>
using std::vector;

namespace EpetraExt {

AmesosBTF_CrsGraph::
~AmesosBTF_CrsGraph()
{
}

AmesosBTF_CrsGraph::NewTypeRef
AmesosBTF_CrsGraph::
operator()( OriginalTypeRef orig )
{
  origObj_ = &orig;

  if( orig.RowMap().DistributedGlobal() )
  { cout << "FAIL for Global!\n"; abort(); }
  if( orig.IndicesAreGlobal() )
  { cout << "FAIL for Global Indices!\n"; abort(); }

  // Extract the CCS information
  int n = orig.NumMyRows();
  int nnz = orig.NumMyNonzeros();

  vector<int> Ap(n+1,0);  // column pointers
  vector<int> Ai(nnz);    // row indices
  int cnt;
  for( int i = 0; i < n; ++i )
  {
    int * tmpP = &Ai[Ap[i]];
    orig.ExtractMyRowCopy( i, nnz-Ap[i], cnt, tmpP );
    Ap[i+1] = Ap[i] + cnt;
  }

#ifdef BTF_VERBOSE
  cout << "-----------------------------------------\n";
  cout << "CCS Format Graph\n";
  cout << "-----------------------------------------\n";
  for( int i = 0; i < n; ++i )
  {
    cout << i << ": " << Ap[i+1] << ": ";
    for( int j = Ap[i]; j<Ap[i+1]; ++j )
      cout << " " << Ai[j];
    cout << endl;
  }
  cout << "-----------------------------------------\n";
#endif

  // Transformation information
  int numBlocks = 0;      // number of blocks found.
  int numMatch = 0;       // number of nonzeros on diagonal after permutation.
  double maxWork =  0.0;  // no limit on how much work to perform in max-trans.
  double workPerf = 0.0;  // how much work was performed in max-trans.

  // Create a work vector for the BTF code.
  vector<int> work(5*n);

  // Storage for the row and column permutations.
  vector<int> rowperm(n);
  vector<int> colperm(n);
  vector<int> blkPtr(n+1);

  // NOTE:  The permutations are sent in backwards since the matrix is transposed.
  // On output, rowperm and colperm are the row and column permutations of A, where 
  // i = BTF_UNFLIP(rowperm[k]) if row i of A is the kth row of P*A*Q, and j = colperm[k] 
  // if column j of A is the kth column of P*A*Q.  If rowperm[k] < 0, then the 
  // (k,k)th entry in P*A*Q is structurally zero.

  numBlocks = btf_order( n, &Ap[0], &Ai[0], maxWork, &workPerf,
			 &colperm[0], &rowperm[0], &blkPtr[0], 
			 &numMatch, &work[0] );

  // Reverse ordering of permutation to get upper triangular
  vector<int> rowperm_t( n );
  vector<int> colperm_t( n );
  for( int i = 0; i < n; ++i )
  {
    rowperm_t[i] = BTF_UNFLIP(rowperm[(n-1)-i]);
    colperm_t[i] = colperm[(n-1)-i];
  }

#ifdef BTF_VERBOSE
  cout << "-----------------------------------------\n";
  cout << "BTF Output (n = " << n << ")\n";
  cout << "-----------------------------------------\n";
  cout << "Num Blocks: " << numBlocks << endl;
  cout << "Num NNZ Diags: " << numMatch << endl;
  cout << "RowPerm and ColPerm \n";
  for( int i = 0; i<n; ++i )
    cout << rowperm_t[i] << "\t" << colperm_t[i] << endl;
  cout << "-----------------------------------------\n";
#endif

  //Generate New Domain and Range Maps
  //for now, assume they start out as identical
  const Epetra_BlockMap & OldMap = orig.RowMap();
  vector<int> newDomainElements( n );
  vector<int> newRangeElements( n );
  for( int i = 0; i < n; ++i )
  {
    newRangeElements[ i ] = rowperm_t[i];
    newDomainElements[ i ] = colperm_t[i];
  }

  NewRowMap_ = Teuchos::rcp( new Epetra_Map( n, n, &newRangeElements[0], OldMap.IndexBase(), OldMap.Comm() ) );
  NewDomainMap_ = Teuchos::rcp( new Epetra_Map( n, n, &newDomainElements[0], OldMap.IndexBase(), OldMap.Comm() ) );

#ifdef BTF_VERBOSE
  cout << "New Row Map\n";
  cout << *NewRowMap_ << endl;
  cout << "New Domain Map\n";
  cout << *NewDomainMap_ << endl;
#endif

  //Generate New Graph
  NewGraph_ = Teuchos::rcp( new Epetra_CrsGraph( Copy, *NewRowMap_, 0 ) );
  Epetra_Import Importer( *NewRowMap_, OldMap );
  NewGraph_->Import( orig, Importer, Insert );
  NewGraph_->FillComplete();
  NewGraph_->FillComplete( *NewDomainMap_, *NewRowMap_ );

#ifdef BTF_VERBOSE
  cout << "New CrsGraph\n";
  cout << *NewGraph_ << endl;
#endif

  newObj_ = &*NewGraph_;

  return *NewGraph_;
}

} //namespace EpetraExt
