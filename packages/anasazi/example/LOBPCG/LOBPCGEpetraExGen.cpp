#include "AnasaziConfigDefs.hpp"
#include "AnasaziBasicEigenproblem.hpp"
#include "AnasaziLOBPCGSolMgr.hpp"
#include "AnasaziBasicOutputManager.hpp"
#include "AnasaziEpetraAdapter.hpp"
#include "Epetra_CrsMatrix.h"
#include "Teuchos_CommandLineProcessor.hpp"

#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#include <mpi.h>
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"

#include "ModeLaplace2DQ2.h"

using namespace Anasazi;

int main(int argc, char *argv[]) {

#ifdef HAVE_MPI
  // Initialize MPI
  //
  MPI_Init(&argc,&argv);
#endif

  // Create an Epetra communicator
  //
#ifdef HAVE_MPI
  Epetra_MpiComm Comm(MPI_COMM_WORLD);
#else
  Epetra_SerialComm Comm;
#endif

  // Create an Anasazi output manager
  //
  BasicOutputManager<double> printer;
  printer.stream(Errors) << Anasazi_Version() << endl << endl;

  // Get the sorting string from the command line
  //
  std::string which("LM");
  Teuchos::CommandLineProcessor cmdp(false,true);
  cmdp.setOption("sort",&which,"Targetted eigenvalues (SM or LM).");
  if (cmdp.parse(argc,argv) != Teuchos::CommandLineProcessor::PARSE_SUCCESSFUL) {
#ifdef HAVE_MPI
    MPI_Finalize();
#endif
    return -1;
  }

  
  typedef Epetra_MultiVector MV;
  typedef Epetra_Operator OP;
  typedef MultiVecTraits<double, Epetra_MultiVector> MVT;

  // Number of dimension of the domain
  const int space_dim = 2;

  // Size of each of the dimensions of the domain
  std::vector<double> brick_dim( space_dim );
  brick_dim[0] = 1.0;
  brick_dim[1] = 1.0;

  // Number of elements in each of the dimensions of the domain
  std::vector<int> elements( space_dim );
  elements[0] = 10;
  elements[1] = 10;
  
  // Create problem
  Teuchos::RCP<ModalProblem> testCase = 
    Teuchos::rcp( new ModeLaplace2DQ2(Comm, brick_dim[0], elements[0], brick_dim[1], elements[1]) );
  
  // Get the stiffness and mass matrices
  Teuchos::RCP<Epetra_CrsMatrix> K = Teuchos::rcp( const_cast<Epetra_CrsMatrix *>(testCase->getStiffness()), false );
  Teuchos::RCP<Epetra_CrsMatrix> M = Teuchos::rcp( const_cast<Epetra_CrsMatrix *>(testCase->getMass()), false );

  // Eigensolver parameters
  int nev = 10;
  int blockSize = 5;
  int maxIters = 500;
  double tol = 1.0e-8;

  Teuchos::RCP<Epetra_MultiVector> ivec = Teuchos::rcp( new Epetra_MultiVector(K->OperatorDomainMap(), blockSize) );
  ivec->Random();

  // Create the eigenproblem.
  Teuchos::RCP<BasicEigenproblem<double, MV, OP> > MyProblem =
    Teuchos::rcp( new BasicEigenproblem<double, MV, OP>(K, M, ivec) );

  // Inform the eigenproblem that the operator A is symmetric
  MyProblem->setHermitian(true);

  // Set the number of eigenvalues requested
  MyProblem->setNEV( nev );

  // Inform the eigenproblem that you are finishing passing it information
  bool boolret = MyProblem->setProblem();
  if (boolret != true) {
    printer.print(Errors,"Anasazi::BasicEigenproblem::setProblem() returned an error.\n");
#ifdef HAVE_MPI
    MPI_Finalize();
#endif
    return -1;
  }

  // Create parameter list to pass into the solver manager
  //
  Teuchos::ParameterList MyPL;
  MyPL.set( "Which", which );
  MyPL.set( "Block Size", blockSize );
  MyPL.set( "Maximum Iterations", maxIters );
  MyPL.set( "Convergence Tolerance", tol );
  MyPL.set( "Full Ortho", true );
  MyPL.set( "Use Locking", true );
  //
  // Create the solver manager
  LOBPCGSolMgr<double, MV, OP> MySolverMan(MyProblem, MyPL);

  // Solve the problem
  //
  ReturnType returnCode = MySolverMan.solve();

  // Get the eigenvalues and eigenvectors from the eigenproblem
  //
  Eigensolution<double,MV> sol = MyProblem->getSolution();
  std::vector<Value<double> > evals = sol.Evals;
  Teuchos::RCP<MV> evecs = sol.Evecs;

  // Compute residuals.
  //
  std::vector<double> normR(sol.numVecs);
  if (sol.numVecs > 0) {
    Teuchos::SerialDenseMatrix<int,double> T(sol.numVecs, sol.numVecs);
    Epetra_MultiVector Kvec( K->OperatorDomainMap(), evecs->NumVectors() );
    Epetra_MultiVector Mvec( M->OperatorDomainMap(), evecs->NumVectors() );
    T.putScalar(0.0); 
    for (int i=0; i<sol.numVecs; i++) {
      T(i,i) = evals[i].realpart;
    }
    K->Apply( *evecs, Kvec );  
    M->Apply( *evecs, Mvec );  
    MVT::MvTimesMatAddMv( -1.0, Mvec, T, 1.0, Kvec );
    MVT::MvNorm( Kvec, &normR );
  }

  // Print the results
  //
  ostringstream os;
  os.setf(ios_base::right, ios_base::adjustfield);
  os<<"Solver manager returned " << (returnCode == Converged ? "converged." : "unconverged.") << endl;
  os<<endl;
  os<<"------------------------------------------------------"<<endl;
  os<<std::setw(16)<<"Eigenvalue"
    <<std::setw(18)<<"Direct Residual"
    <<endl;
  os<<"------------------------------------------------------"<<endl;
  for (int i=0; i<sol.numVecs; i++) {
    os<<std::setw(16)<<evals[i].realpart
      <<std::setw(18)<<normR[i]/evals[i].realpart
      <<endl;
  }
  os<<"------------------------------------------------------"<<endl;
  printer.print(Errors,os.str());

#ifdef HAVE_MPI
  MPI_Finalize();
#endif
  return 0;
}
