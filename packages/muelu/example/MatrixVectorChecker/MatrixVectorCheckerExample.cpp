// Teuchos
#include <Teuchos_RCP.hpp>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_CommandLineProcessor.hpp>
#include <Teuchos_GlobalMPISession.hpp>
#include <Teuchos_DefaultComm.hpp>

// Using Galeri:
// #include <Epetra_Maps.h>
// #include <Epetra_CrsMatrix.h>
// #include <Galeri_Maps.h>
// #include <Galeri_CrsMatrices.h>

// Using MueLu:
#include <Cthulhu_Parameters.hpp>
#include <Cthulhu_Map.hpp>
#include <Cthulhu_MapFactory.hpp>
#include <Cthulhu_CrsOperator.hpp>
#include <Cthulhu_CrsMatrix.hpp>
#include <Cthulhu.hpp> //TODO

#include <MueLu_UseDefaultTypes.hpp>
#include <MueLu_UseShortNames.hpp>

// MueLu Gallery :
#define CTHULHU_ENABLED
#include <MueLu_GalleryParameters.hpp>
#include <MueLu_MatrixFactory.hpp>

//
#include "MatrixVectorChecker.hpp"

int main(int argc, char *argv[])
{
  
  Teuchos::oblackholestream blackhole;
  Teuchos::GlobalMPISession mpiSession(&argc,&argv, &blackhole);
  Teuchos::RCP<const Teuchos::Comm<int> > comm = Teuchos::DefaultComm<int>::getComm();

  /**********************************************************************************/
  /* SET TEST PARAMETERS                                                            */
  /**********************************************************************************/
  // Note: use --help to list available options.
  Teuchos::CommandLineProcessor cmdp(false);
  
  MueLu::Gallery::Parameters matrixParameters(cmdp);   // manage parameters of the test case
  Cthulhu::Parameters cthulhuParameters(cmdp);         // manage parameters of cthulhu
  
  switch (cmdp.parse(argc,argv)) {
  case Teuchos::CommandLineProcessor::PARSE_HELP_PRINTED:        return EXIT_SUCCESS; break;
  case Teuchos::CommandLineProcessor::PARSE_UNRECOGNIZED_OPTION: return EXIT_FAILURE; break;
  case Teuchos::CommandLineProcessor::PARSE_SUCCESSFUL:                               break;
  }
  
  matrixParameters.check();
  cthulhuParameters.check();

  matrixParameters.print();
  cthulhuParameters.print();

  /**********************************************************************************/
  /* CREATE INITAL MATRIX                                                           */
  /**********************************************************************************/
  const RCP<const Map> map = MapFactory::Build(cthulhuParameters.GetLib(), matrixParameters.GetNumGlobalElements(), 0, comm);
  RCP<const Operator>   Op = MueLu::Gallery::CreateCrsMatrix<SC, LO, GO, Map, CrsOperator>(matrixParameters.GetMatrixType(), map, matrixParameters.GetParameterList()); //TODO: Operator vs. CrsOperator

  // Using Galeri:
  //
  // RCP<Tpetra_Map> map = rcp( Galeri::CreateMap("Cartesian2D", comm, paramList) );
  // RCP<Tpetra_CrsMatrix> Op = rcp( Galeri::CreateCrsMatrix("Laplace2D", map.get(), paramList) );
  /**********************************************************************************/
  /*                                                                                */
  /**********************************************************************************/

  if ( MatrixVectorChecker<SC,LO,GO,NO>(Op) ) {
    std::cout << "OK !" << std::endl;
    return EXIT_SUCCESS;
  } else {
    std::cout << "FAILURE !" << std::endl;
    return EXIT_FAILURE;
  }

}

// JG TODO:
// - add a method CreateMap for cthulhu/gallery (as Galeri)
// - wrap galeri matrix for the new MatrixVectorChecker
