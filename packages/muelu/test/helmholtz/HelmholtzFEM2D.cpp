// @HEADER
//
// ***********************************************************************
//
//        MueLu: A package for multigrid based preconditioning
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact
//                    Jeremie Gaidamour (jngaida@sandia.gov)
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#include <iostream>
#include <complex>

// Xpetra and Galeri
#include <Xpetra_MultiVectorFactory.hpp>
#include <Galeri_XpetraParameters.hpp>
#include <Galeri_XpetraProblemFactory_Helmholtz.hpp>
#include <Galeri_XpetraUtils.hpp>
#include <Galeri_XpetraMaps.hpp>

// MueLu
#include "MueLu.hpp"
#include <MueLu_Level.hpp>
#include <MueLu_BaseClass.hpp>
#include <MueLu_Utilities.hpp>
#include <MueLu_UseDefaultTypesComplex.hpp>
#include <MueLu_UseShortNames.hpp>
#include <MueLu_MutuallyExclusiveTime.hpp>

// Belos
#include <BelosConfigDefs.hpp>
#include <BelosLinearProblem.hpp>
#include <BelosBlockGmresSolMgr.hpp>
#include <BelosXpetraAdapter.hpp>
#include <BelosMueLuAdapter.hpp>

typedef Tpetra::Vector<SC,LO,GO,NO>                  TVEC;
typedef Tpetra::MultiVector<SC,LO,GO,NO>             TMV;
typedef Tpetra::CrsMatrix<SC,LO,GO,NO,LMO>           TCRS;
typedef Xpetra::CrsMatrix<SC,LO,GO,NO,LMO>           XCRS;
typedef Xpetra::TpetraCrsMatrix<SC,LO,GO,NO,LMO>     XTCRS; 
typedef Xpetra::Matrix<SC,LO,GO,NO,LMO>              XMAT;
typedef Xpetra::CrsMatrixWrap<SC,LO,GO,NO,LMO>       XWRAP;

typedef Belos::OperatorT<TMV>                        TOP;
typedef Belos::OperatorTraits<SC,TMV,TOP>            TOPT;
typedef Belos::MultiVecTraits<SC,TMV>                TMVT;
typedef Belos::LinearProblem<SC,TMV,TOP>             TProblem;
typedef Belos::SolverManager<SC,TMV,TOP>             TBelosSolver;
typedef Belos::BlockGmresSolMgr<SC,TMV,TOP>          TBelosGMRES;

int main(int argc, char *argv[]) {

  using Teuchos::RCP;
  using Teuchos::rcp;
  using Teuchos::TimeMonitor;

  Teuchos::GlobalMPISession mpiSession(&argc, &argv, NULL);
  RCP< const Teuchos::Comm<int> > comm = Teuchos::DefaultComm<int>::getComm();
  Teuchos::CommandLineProcessor clp(false);

  //***********************//
  //   Galeri Parameters   //
  //***********************//

  GO nx,ny,nz;
  double stretchx, stretchy, stretchz, h, delta;
  int PMLXL, PMLXR, PMLYL, PMLYR, PMLZL, PMLZR;
  double omega, shift;

  std::ifstream inputfile;
  inputfile.open("helm2D.xml");
  inputfile >> nx       >> ny       >> nz ;
  if(comm->getRank()==0)
    std::cout<<"nx: "<<nx<<"  ny: "<<ny<<"  nz: "<<nz<<std::endl;
  inputfile >> stretchx >> stretchy >> stretchz ;
  if(comm->getRank()==0)
    std::cout<<"stretchx: "<<stretchx<<"  stretchy: "<<stretchy<<"  stretchz: "<<stretchz<<std::endl;
  inputfile >> h        >> delta ;
  if(comm->getRank()==0)
    std::cout<<"h: "<<h<<"  delta: "<<delta<<std::endl;
  inputfile >> PMLXL    >> PMLXR ;
  if(comm->getRank()==0)
    std::cout<<"PMLXL: "<<PMLXL<<"  PMLXR: "<<PMLXR<<std::endl;
  inputfile >> PMLYL    >> PMLYR ;
  if(comm->getRank()==0)
    std::cout<<"PMLYL: "<<PMLYL<<"  PMLYR: "<<PMLYR<<std::endl;
  inputfile >> PMLZL    >> PMLZR ;
  if(comm->getRank()==0)
    std::cout<<"PMLZL: "<<PMLZL<<"  PMLZR: "<<PMLZR<<std::endl;
  inputfile >> omega    >> shift ;
  if(comm->getRank()==0)
    std::cout<<"omega: "<<omega<<"  shift: "<<shift<<std::endl;

  Galeri::Xpetra::Parameters<GO> matrixParameters_laplace  (clp, nx, ny, nz, "HelmholtzFEM2D", 0, stretchx, stretchy, stretchz,
							    h, delta, 0,     0,     0,     0,     0,     0,     0.0,   0.0  );
  Galeri::Xpetra::Parameters<GO> matrixParameters_helmholtz(clp, nx, ny, nz, "HelmholtzFEM2D", 0, stretchx, stretchy, stretchz,
							    h, delta, PMLXL, PMLXR, PMLYL, PMLYR, PMLZL, PMLZR, omega, 0.0  );
  Galeri::Xpetra::Parameters<GO> matrixParameters_shift    (clp, nx, ny, nz, "HelmholtzFEM2D", 0, stretchx, stretchy, stretchz,
							    h, delta, PMLXL, PMLXR, PMLYL, PMLYR, PMLZL, PMLZR, omega, shift);
  Xpetra::Parameters             xpetraParameters(clp);


  //****************************************//
  //   Setup Galeri Problems and Matrices   //
  //****************************************//

  RCP<TimeMonitor> globalTimeMonitor = rcp (new TimeMonitor(*TimeMonitor::getNewTimer("ScalingTest: S - Global Time")));
  RCP<TimeMonitor> tm = rcp (new TimeMonitor(*TimeMonitor::getNewTimer("ScalingTest: 1 - Matrix Build")));

  Teuchos::ParameterList pl = matrixParameters_helmholtz.GetParameterList();
  RCP<MultiVector> coordinates;
  Teuchos::ParameterList galeriList;
  galeriList.set("nx", pl.get("nx", nx));
  galeriList.set("ny", pl.get("ny", ny));
  galeriList.set("nz", pl.get("nz", nz));
  RCP<const Map> map;

  if (matrixParameters_helmholtz.GetMatrixType() == "Helmholtz1D") {
    map = MapFactory::Build(xpetraParameters.GetLib(), matrixParameters_helmholtz.GetNumGlobalElements(), 0, comm);
    coordinates = Galeri::Xpetra::Utils::CreateCartesianCoordinates<SC, LO, GO, Map, MultiVector>("1D", map, matrixParameters_helmholtz.GetParameterList());
  }
  else if (matrixParameters_helmholtz.GetMatrixType() == "HelmholtzFEM2D") {
    map = Galeri::Xpetra::CreateMap<LO, GO, Node>(xpetraParameters.GetLib(), "Cartesian2D", comm, galeriList);
    coordinates = Galeri::Xpetra::Utils::CreateCartesianCoordinates<SC, LO, GO, Map, MultiVector>("2D", map, matrixParameters_helmholtz.GetParameterList());
  }
  else if (matrixParameters_helmholtz.GetMatrixType() == "HelmholtzFEM3D") {
    map = Galeri::Xpetra::CreateMap<LO, GO, Node>(xpetraParameters.GetLib(), "Cartesian3D", comm, galeriList);
    coordinates = Galeri::Xpetra::Utils::CreateCartesianCoordinates<SC, LO, GO, Map, MultiVector>("3D", map, matrixParameters_helmholtz.GetParameterList());
  }

  RCP<const Tpetra::Map<LO, GO, NO> > tmap = Xpetra::toTpetra(map);

  Teuchos::ParameterList matrixParams_laplace   = matrixParameters_laplace.GetParameterList();
  Teuchos::ParameterList matrixParams_helmholtz = matrixParameters_helmholtz.GetParameterList();
  Teuchos::ParameterList matrixParams_shift     = matrixParameters_shift.GetParameterList();

  RCP<Galeri::Xpetra::Problem<Map,CrsMatrixWrap,MultiVector> > Pr_laplace =
      Galeri::Xpetra::BuildProblem<SC,LO,GO,Map,CrsMatrixWrap,MultiVector>(matrixParameters_laplace.GetMatrixType(), map, matrixParams_laplace);
  RCP<Matrix> A_laplace = Pr_laplace->BuildMatrix();

  RCP<Galeri::Xpetra::Problem<Map,CrsMatrixWrap,MultiVector> > Pr_helmholtz =
      Galeri::Xpetra::BuildProblem<SC,LO,GO,Map,CrsMatrixWrap,MultiVector>(matrixParameters_helmholtz.GetMatrixType(), map, matrixParams_helmholtz);
  RCP<Matrix> A_helmholtz = Pr_helmholtz->BuildMatrix();

  RCP<Galeri::Xpetra::Problem<Map,CrsMatrixWrap,MultiVector> > Pr_shift =
      Galeri::Xpetra::BuildProblem<SC,LO,GO,Map,CrsMatrixWrap,MultiVector>(matrixParameters_shift.GetMatrixType(), map, matrixParams_shift);
  RCP<Matrix> A_shift = Pr_shift->BuildMatrix();

  RCP<MultiVector> nullspace = MultiVectorFactory::Build(map,1);
  nullspace->putScalar( (SC) 1.0);
 
  comm->barrier();

  tm = Teuchos::null;

  //************************************//
  //   Setup Multigrid Preconditioner   //
  //************************************//

  tm = rcp (new TimeMonitor(*TimeMonitor::getNewTimer("ScalingTest: 2 - MueLu Setup")));

  // Factories
  RCP<TentativePFactory>  TentPFact = rcp( new TentativePFactory );
  RCP<SaPFactory>         Pfact     = rcp( new SaPFactory        );
  RCP<TransPFactory>      Rfact     = rcp( new TransPFactory     );
  RCP<RAPFactory>         Acfact    = rcp( new RAPFactory        );

  // Smoother
  RCP<SmootherPrototype> smooProto;
  std::string ifpack2Type;
  Teuchos::ParameterList ifpack2List;
  ifpack2Type = "KRYLOV";
  ifpack2List.set("krylov: iteration type",1);
  ifpack2List.set("krylov: number of iterations",4);
  ifpack2List.set("krylov: residual tolerance",1e-6);
  ifpack2List.set("krylov: block size",1);
  ifpack2List.set("krylov: zero starting solution",true);
  ifpack2List.set("krylov: preconditioner type",3);
  ifpack2List.set("schwarz: compute condest", false);
  ifpack2List.set("schwarz: overlap level", 0);
  smooProto = Teuchos::rcp( new Ifpack2Smoother(ifpack2Type,ifpack2List) );
  RCP<SmootherFactory> SmooFact;
  LO maxLevels = 6;
  if (maxLevels > 1)
    SmooFact = rcp( new SmootherFactory(smooProto) );

  // Coarse grid solver
  RCP<SmootherPrototype> coarsestSmooProto;
  std::string type = "";
  Teuchos::ParameterList coarsestSmooList;
#if defined(HAVE_AMESOS_SUPERLU)
  coarsestSmooProto = rcp( new DirectSolver("Superlu",coarsestSmooList) );
#else
  coarsestSmooProto = rcp( new DirectSolver("Klu",coarsestSmooList) );
#endif
  RCP<SmootherFactory> coarsestSmooFact = rcp(new SmootherFactory(coarsestSmooProto, Teuchos::null));

  // Setup and Keep R's and P's
  RCP<Hierarchy> H = rcp(new Hierarchy(A_laplace));
  H->GetLevel(0)->Set("Nullspace",nullspace);
  FactoryManager Manager;
  Manager.SetFactory("P",      Pfact     );
  Manager.SetFactory("R",      Rfact     );
  Manager.SetFactory("A",      Acfact    );
  Manager.SetFactory("Ptent",  TentPFact );
  H->Keep("P",      Pfact.get()     );
  H->Keep("R",      Rfact.get()     );
  H->Keep("Ptent",  TentPFact.get() );
  H->Setup(Manager, 0, maxLevels);

  // Setup coarse grid operators and smoothers
  RCP<Level> finestLevel = H->GetLevel();
  finestLevel->Set("A", A_shift);
  Manager.SetFactory("Smoother", SmooFact);
  Manager.SetFactory("CoarseSolver", coarsestSmooFact);
  H->Setup(Manager, 0, H->GetNumLevels());
  //H->Write(-1,-1);

  tm = Teuchos::null;
  
  //************************************//
  //   Solve linear system with Belos   //
  //************************************//

  tm = rcp (new TimeMonitor(*TimeMonitor::getNewTimer("ScalingTest: 3 - LHS and RHS initialization")));

  RCP<TVEC> X = Tpetra::createVector<SC,LO,GO,NO>(tmap);
  RCP<TVEC> B = Tpetra::createVector<SC,LO,GO,NO>(tmap);  
  X->putScalar((SC) 0.0);
  B->putScalar((SC) 0.0);
  int pointsourceid=nx*ny/2+nx/2;
  if(map->isNodeGlobalElement(pointsourceid)==true) {
    B->replaceGlobalValue(pointsourceid, (SC) 1.0);
  }

  tm = Teuchos::null;

  tm = rcp (new TimeMonitor(*TimeMonitor::getNewTimer("ScalingTest: 4 - Belos Solve")));

  // Define Operator and Preconditioner
  RCP<TOP> belosOp   = rcp(new Belos::XpetraOp<SC,LO,GO,NO,LMO> (A_helmholtz) );    // Turns a Xpetra::Matrix object into a Belos operator
  RCP<TOP> belosPrec = rcp(new Belos::MueLuOp<SC,LO,GO,NO,LMO>  (H)           );    // Turns a MueLu::Hierarchy object into a Belos operator

  // Construct a Belos LinearProblem object
  RCP<TProblem> belosProblem = rcp(new TProblem(belosOp,X,B));
  belosProblem->setRightPrec(belosPrec); 
  bool set = belosProblem->setProblem();
  if (set == false) {
    if(comm->getRank()==0)
      std::cout << std::endl << "ERROR:  Belos::LinearProblem failed to set up correctly!" << std::endl;    
    return EXIT_FAILURE;
  }
    
  // Belos parameter list
  int maxIts = 100;
  double tol = 1e-6;
  Teuchos::ParameterList belosList;
  belosList.set("Maximum Iterations",    maxIts); // Maximum number of iterations allowed
  belosList.set("Convergence Tolerance", tol);    // Relative convergence tolerance requested
  belosList.set("Flexible Gmres", true);          // set flexible GMRES on/off
  belosList.set("Verbosity", Belos::Errors + Belos::Warnings + Belos::StatusTestDetails);
  belosList.set("Output Frequency",1);
  belosList.set("Output Style",Belos::Brief);

  // Create solver manager
  RCP<TBelosSolver> solver = rcp( new TBelosGMRES(belosProblem, rcp(&belosList, false)) );

  // Perform solve
  Belos::ReturnType ret=Belos::Unconverged;
  try {
    ret = solver->solve();
    if (comm->getRank() == 0)
      std::cout << "Number of iterations performed for this solve: " << solver->getNumIters() << std::endl;
  }

  catch(...) {
    if (comm->getRank() == 0)
      std::cout << std::endl << "ERROR:  Belos threw an error! " << std::endl;
  }
  
  // Check convergence
  if (ret != Belos::Converged) {
    if (comm->getRank() == 0) std::cout << std::endl << "ERROR:  Belos did not converge! " << std::endl;
  } else {
    if (comm->getRank() == 0) std::cout << std::endl << "SUCCESS:  Belos converged!" << std::endl;
  }

  // Compute actual residuals.
  int numrhs=1;
  bool badRes = false;
  std::vector<double> actual_resids(numrhs);
  std::vector<double> rhs_norm(numrhs);
  RCP<TMV> resid = Tpetra::createMultiVector<SC,LO,GO,NO>(tmap, numrhs);     
  TOPT::Apply(*belosOp, *X, *resid);
  TMVT::MvAddMv(-1.0, *resid, 1.0, *B, *resid);
  TMVT::MvNorm(*resid, actual_resids);
  TMVT::MvNorm(*B, rhs_norm);
  if(comm->getRank()==0) {
    std::cout<< "---------- Actual Residuals (normalized) ----------"<<std::endl<<std::endl;
  }
  for (int i = 0; i < numrhs; i++) {
    double actRes = abs(actual_resids[i])/rhs_norm[i];
    if(comm->getRank()==0) {
      std::cout <<"Problem " << i << " : \t" << actRes <<std::endl;
    }
    if (actRes > tol) { badRes = true; }
  }

  // Get the number of iterations for this solve.
  if(comm->getRank()==0)
    std::cout << "Number of iterations performed for this solve: " << solver->getNumIters() << std::endl;
 
  tm = Teuchos::null;

  globalTimeMonitor = Teuchos::null;

  TimeMonitor::summarize();

} //main
