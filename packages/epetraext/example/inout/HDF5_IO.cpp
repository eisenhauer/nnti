#include "EpetraExt_ConfigDefs.h"
#ifdef HAVE_MPI
#include "mpi.h"
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include <vector>
#include "Epetra_Map.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_MultiVector.h"
#include "EpetraExt_HDF5.h"
#include "EpetraExt_Utils.h"
#include "EpetraExt_Exception.h"
#include "Teuchos_ParameterList.hpp"

// Showing the usage of HDF5 I/O.
// This example can be run with any number of processors.
//
// \author Marzio Sala, D-INFK/ETHZ.
//
// \date Last modified on 09-Mar-06.

int main (int argc, char **argv)
{
#ifdef HAVE_MPI
  MPI_Init(&argc, &argv);
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif

  try 
  {
    int n = Comm.NumProc() * 4;
    Epetra_Map Map(n, 0, Comm);
    Epetra_MultiVector x(Map, 2); x.Random();
    Epetra_MultiVector b(Map, 2); x.Random();
    Epetra_CrsMatrix Matrix(Copy, Map, 0);
    // diagonal matrix
    for (int i = 0; i < Map.NumMyElements(); ++i)
    {
      int ii = Map.GID(i);
      double one = 1.0;
      Matrix.InsertGlobalValues(ii, 1, &one, &ii);
    }
    Matrix.FillComplete();

    // create a Teuchos::ParameterList and populate it
    Teuchos::ParameterList List;
    List.set("bool type", true);
    List.set("int type", 2);
    List.set("double type", 3.0);
    List.set("string type", "a string");

    // This is the HDF5 file manager
    EpetraExt::HDF5 HDF5(Comm);

    // creates a new file. To open an existing file, use Open("myfile.h5")
    HDF5.Create("myfile.h5");

    // =========================== //
    // P A R T   I:  W R I T I N G //
    // =========================== //

    if (Comm.MyPID() == 0)
      cout << "Writing objects to HDF5 file myfile.h5..." << endl << endl;

    // We first write the map, whose name contains the number of processors
    HDF5.Write("map-" + EpetraExt::toString(Comm.NumProc()), Map);
    // Then we write the matrix...
    HDF5.Write("matrix", Matrix);
    // ... and the x, b vectors
    HDF5.Write("x", x);
    HDF5.Write("b", b);
    // we can associate basic data types with a given group, for example:
    HDF5.Write("matrix", "integration order", 1);
    HDF5.Write("matrix", "numerical drop", 0.1);
    HDF5.Write("matrix", "package name", "EpetraExt");
    HDF5.Write("matrix", "author", "Marzio Sala");
    HDF5.Write("matrix", "institution", "ETHZ/D-INFK");
    // or put them in a new group
    HDF5.Write("my parameters", "latitude", 12);
    HDF5.Write("my parameters", "longitude", 67);

    // Now we write the parameter list we have used to create the matrix
    HDF5.Write("List", List);
    // We can also write integers/doubles/arrays. All these quantities are
    // supposed to have the same value on all processors.
    vector<int> iarray(3); 
    iarray[0] = 0, iarray[1] = 1; iarray[2] = 2;
    HDF5.Write("my parameters", "int array", H5T_NATIVE_INT, 3, &iarray[0]);

    vector<double> darray(3); 
    darray[0] = 0.1, darray[1] = 1.1; darray[2] = 2.1;
    HDF5.Write("my parameters", "double array", H5T_NATIVE_DOUBLE, 3, &darray[0]);

    // To analyze the content of the file, one can use 
    // "h5dump filename.h5" or "h5dump filename.h5 -H" o

    // ============================ //
    // P A R T   II:  R E A D I N G //
    // ============================ //

    if (Comm.MyPID() == 0)
      cout << "Reading objects from HDF5 file myfile.h5..." << endl << endl;

    Epetra_Map* NewMap = 0;
    Epetra_CrsMatrix* NewMatrix = 0;
    Epetra_MultiVector* NewX,* NewB;

    // Check if the map is there (in this case it is). If it is, read the
    // matrix using the map, otherwise read the matrix with a linear map.
    if (HDF5.IsContained("map-" + EpetraExt::toString(Comm.NumProc())))
    {
      HDF5.Read("map-" + EpetraExt::toString(Comm.NumProc()), NewMap);
      HDF5.Read("matrix", *NewMap, *NewMap, NewMatrix);
    }
    else
    {
      int NumGlobalRows;
      HDF5.Read("matrix", "NumGlobalRows", NumGlobalRows);
      NewMap = new Epetra_Map(NumGlobalRows, 0, Comm);
      HDF5.Read("matrix", *NewMap, *NewMap, NewMatrix);
    }

    // read the number of nonzeros from file, compare them with the
    // actual number of NewMatrix
    int NewNumGlobalNonzeros;
    HDF5.Read("matrix", "NumGlobalNonzeros", NewNumGlobalNonzeros);

    assert (Matrix.NumGlobalNonzeros() == NewNumGlobalNonzeros);

    // Now read the MultiVector's
    HDF5.Read("x", NewX);
    HDF5.Read("b", NewB);

    // the int/double values, and the int/double arrays
    int new_latitude, new_longitude;
    vector<int>    new_iarray(3);
    vector<double> new_darray(3);
    HDF5.Read("my parameters", "latitude", new_latitude);
    HDF5.Read("my parameters", "longitude", new_longitude);
    HDF5.Read("my parameters", "int array", H5T_NATIVE_INT, 3, &new_iarray[0]);
    HDF5.Read("my parameters", "double array", H5T_NATIVE_DOUBLE, 3, &new_darray[0]);
    // and the string values associated with group "matrix", recordered
    // with dataset "package name".
    string PackageName;
    HDF5.Read("matrix", "package name", PackageName);

    Teuchos::ParameterList newList;
    HDF5.Read("List", newList);
    if (Comm.MyPID() == 0)
    {
      cout << "New list as read from file is:" << endl;
      cout << "bool type = " << newList.get("bool type", false) << endl;
      cout << "int type = " << newList.get("int type", -1) << endl;
      cout << "double type = " << newList.get("double type", -1.0) << endl;
      cout << "string type = " << newList.get("string type", "not-set") << endl;
      cout << endl;

      cout << "Checking some read and written data..." << endl;
      for (int i = 0; i < 3; ++i)
        cout << "iarray[" << i << "] = " << iarray[i] << " should be " << new_iarray[i] << endl;
      for (int i = 0; i < 3; ++i)
        cout << "darray[" << i << "] = " << darray[i] << " should be " << new_darray[i] << endl;
      cout << endl;
      cout << "Try to print out the content of myfile.h5 using the command" << endl;
      cout << "    h5dump myfile.h5" << endl;
    }

    // We finally close the file. Better to close it before calling
    // MPI_Finalize() to avoid MPI-related errors, since Close() might call MPI
    // functions.
    HDF5.Close();

    // delete memory
    if (NewMap   ) delete NewMap;
    if (NewMatrix) delete NewMatrix;
    if (NewX     ) delete NewX;
    if (NewB     ) delete NewB;
  }
  catch(EpetraExt::Exception& rhs) 
  {
    rhs.Print();
  }
  catch (...) 
  {
    cerr << "Caught generic exception" << endl;
  }


#ifdef HAVE_MPI
  MPI_Finalize();
#endif

  return(EXIT_SUCCESS);
}
