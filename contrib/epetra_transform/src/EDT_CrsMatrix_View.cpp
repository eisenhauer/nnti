
#include <EDT_CrsMatrix_View.h>

#include <Epetra_CrsGraph.h>
#include <Epetra_CrsMatrix.h>

namespace Epetra_Transform {

std::auto_ptr<Epetra_CrsMatrix> CrsMatrix_View::operator()( const Epetra_CrsMatrix & original )
{
  if( original.IndicesAreGlobal() ) cout << "EDT_CrsMatrix_View: Indices must be LOCAL!\n";
  assert( !original.IndicesAreGlobal() );

  //test graph, new graph must be left subset of old

  //intial construction of matrix 
  std::auto_ptr<Epetra_CrsMatrix> newMatrix( new Epetra_CrsMatrix( View, NewGraph_ ) );

  //insert views of row values
  int * myIndices;
  double * myValues;
  int indicesCnt;
  int numMyRows = newMatrix->NumMyRows();
  for( int i = 0; i < numMyRows; ++i )
  {
    original.ExtractMyRowView( i, indicesCnt, myValues, myIndices );

    int newIndicesCnt = indicesCnt;
    bool done = false;
    for( int j = 0; j < indicesCnt; ++j )
      if( !done && NewGraph_.GCID( myIndices[j] ) == -1 )
      {
        newIndicesCnt = j;
        done = true;
      }

    newMatrix->InsertMyValues( i, newIndicesCnt, myValues, myIndices );
  }

  newMatrix->TransformToLocal();

  return newMatrix;
}

} //namespace Epetra_Transform
