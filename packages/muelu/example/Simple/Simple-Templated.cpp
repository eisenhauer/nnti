#include <iostream>

// MueLu main header: include most common header files in one line
#include <MueLu.hpp>

// Define default template types
typedef double Scalar;
typedef int    LocalOrdinal;
typedef int    GlobalOrdinal;

int main(int argc, char *argv[]) {
  using Teuchos::RCP; // reference count pointers

  //
  // MPI initialization using Teuchos
  //

  Teuchos::GlobalMPISession mpiSession(&argc, &argv, NULL);
  RCP< const Teuchos::Comm<int> > comm = Teuchos::DefaultComm<int>::getComm();

  //
  // Parameters
  //

  // problem size
  GlobalOrdinal numGlobalElements = 256;

  // linear algebra library
  // this example require Tpetra+Ifpack2+Amesos2 or Epetra+Ifpack+Amesos
#if   defined(HAVE_MUELU_TPETRA) && defined(HAVE_MUELU_IFPACK2) && defined(HAVE_MUELU_AMESOS2)
  Xpetra::UnderlyingLib lib = Xpetra::UseTpetra; 
#elif defined(HAVE_MUELU_EPETRA) && defined(HAVE_MUELU_IFPACK)  && defined(HAVE_MUELU_AMESOS)
  Xpetra::UnderlyingLib lib = Xpetra::UseEpetra;
#endif

  //
  // Construct the problem
  //

  // Construct a Map that puts approximately the same number of equations on each processor
  RCP<const Xpetra::Map<LocalOrdinal, GlobalOrdinal> > map = Xpetra::MapFactory<LocalOrdinal, GlobalOrdinal>::createUniformContigMap(lib, numGlobalElements, comm);

  // Get update list and number of local equations from newly created map.
  const size_t numMyElements = map->getNodeNumElements();
  Teuchos::ArrayView<const GlobalOrdinal> myGlobalElements = map->getNodeElementList();

  // Create a CrsMatrix using the map, with a dynamic allocation of 3 entries per row
  RCP<Xpetra::Operator<Scalar, LocalOrdinal, GlobalOrdinal> > A = rcp(new Xpetra::CrsOperator<Scalar, LocalOrdinal, GlobalOrdinal>(map, 3));

  // Add rows one-at-a-time
  for (size_t i = 0; i < numMyElements; i++) {
    if (myGlobalElements[i] == 0) {
      A->insertGlobalValues(myGlobalElements[i], 
                            Teuchos::tuple<GlobalOrdinal>(myGlobalElements[i], myGlobalElements[i] +1), 
                            Teuchos::tuple<Scalar> (2.0, -1.0));
    }
    else if (myGlobalElements[i] == numGlobalElements - 1) {
      A->insertGlobalValues(myGlobalElements[i], 
                            Teuchos::tuple<GlobalOrdinal>(myGlobalElements[i] -1, myGlobalElements[i]), 
                            Teuchos::tuple<Scalar> (-1.0, 2.0));
    }
    else {
      A->insertGlobalValues(myGlobalElements[i], 
                            Teuchos::tuple<GlobalOrdinal>(myGlobalElements[i] -1, myGlobalElements[i], myGlobalElements[i] +1), 
                            Teuchos::tuple<Scalar> (-1.0, 2.0, -1.0));
    }
  }

  // Complete the fill, ask that storage be reallocated and optimized
  A->fillComplete();

  //
  // Construct a multigrid preconditioner
  //

  // Multigrid Hierarchy
  MueLu::Hierarchy<Scalar, LocalOrdinal, GlobalOrdinal> H(A);
  H.setVerbLevel(Teuchos::VERB_HIGH);

  // Multigrid setup phase (using default parameters)
  MueLu::FactoryManager<Scalar, LocalOrdinal, GlobalOrdinal> M;                         // -
  M.SetFactory("A", rcp(new MueLu::RAPFactory<Scalar, GlobalOrdinal, LocalOrdinal>())); // TODO: to be remove, but will require some work
  H.Setup(M);                                                                           // -
  // Should be instead: H.Setup();

  //
  // Solve Ax = b
  //

  RCP<Xpetra::Vector<Scalar, LocalOrdinal, GlobalOrdinal> > X = Xpetra::VectorFactory<Scalar, LocalOrdinal, GlobalOrdinal>::Build(map);
  RCP<Xpetra::Vector<Scalar, LocalOrdinal, GlobalOrdinal> > B = Xpetra::VectorFactory<Scalar, LocalOrdinal, GlobalOrdinal>::Build(map);
  
  X->putScalar((Scalar) 0.0);
  B->setSeed(846930886); B->randomize();

  // Use AMG directly as an iterative solver (not as a preconditionner)
  int nIts = 9;

  H.Iterate(*B, nIts, *X);

  // Print relative residual norm
  typedef Teuchos::ScalarTraits<Scalar> ST;
  ST::magnitudeType residualNorms = MueLu::Utils<Scalar, LocalOrdinal, GlobalOrdinal>::ResidualNorm(*A, *X, *B)[0];
  if (comm->getRank() == 0)
    std::cout << "||Residual|| = " << residualNorms << std::endl;

  return EXIT_SUCCESS;
}
