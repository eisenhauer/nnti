#include "Galeri_Maps.h"
#include "Galeri_CrsMatrices.h"
#include "Galeri_Utils.h"
#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#include "mpi.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"
#include "Epetra_CrsMatrix.h"
#include "Teuchos_ParameterList.hpp"

using namespace Galeri;

// =========== //
// main driver //
// =========== //

int main(int argv, char* argc[])
{
#ifdef HAVE_MPI
  MPI_Init(&argv, &argc);
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif

  // pointer to the map to be created
  Epetra_Map*            Map;
  // pointer to the matrix to be created
  Epetra_CrsMatrix*      Matrix;
  // container for parameters
  Teuchos::ParameterList GaleriList;
  // here we specify the global dimension of the problem
  int nx = 5 * Comm.NumProc();
  int ny = 5 * Comm.NumProc();
  GaleriList.set("nx", nx);
  GaleriList.set("ny", ny);

  try
  {
    // Creates a simple linear map; for more details on the map creation
    // refer to the documentation
    Map = CreateMap("Cartesian2D", Comm, GaleriList);

    // Creates a diagonal matrix with 1's on the diagonal
    Matrix   = CreateCrsMatrix("Biharmonic2D", Map, GaleriList);

    // print out the matrix
    //cout << *Matrix;

    int GID = -1; // set GID to the global node to use to compute
                  // the stencil, or put -1 to automatically select it.
    PrintStencil2D(Matrix, nx, ny, GID);

    // To created objects must be free'd using delete
    delete Map;
    delete Matrix;
  }
  catch (Exception& rhs)
  {
    if (Comm.MyPID() == 0)
      rhs.Print();
    exit(EXIT_FAILURE);
  }

#ifdef HAVE_MPI
  MPI_Finalize();
#endif

  return(EXIT_SUCCESS);
}
