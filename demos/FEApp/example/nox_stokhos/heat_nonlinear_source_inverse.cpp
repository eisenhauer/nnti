//@HEADER
// ************************************************************************
// 
//            NOX: An Object-Oriented Nonlinear Solver Package
//                 Copyright (2002) Sandia Corporation
// 
//            LOCA: Library of Continuation Algorithms Package
//                 Copyright (2005) Sandia Corporation
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
// 
// Questions? Contact Roger Pawlowski (rppawlo@sandia.gov) or 
// Eric Phipps (etphipp@sandia.gov), Sandia National Laboratories.
// ************************************************************************
//@HEADER

#include <iostream>
#include <sstream>

#include "NOX.H"
#include "NOX_Epetra.H"

// FEApp is defined in Trilinos/packages/sacado/example/FEApp
#include "FEApp_ModelEvaluator.hpp"
#include "Piro_Epetra_NOXSolver.hpp"

#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif

#include "Stokhos_Epetra.hpp"
#include "Teuchos_TimeMonitor.hpp"

#include "Stokhos_PCEAnasaziKL.hpp"

int main(int argc, char *argv[]) {
  int nelem = 100;
  double alpha = 0.5;
  double leftBC = 0.0;
  double rightBC = 0.1;
  int num_KL = 2;
  int p = 3;

  int MyPID;

  try {

    // Initialize MPI
#ifdef HAVE_MPI
    MPI_Init(&argc,&argv);
#endif

     // Create a communicator for Epetra objects
    Teuchos::RCP<Epetra_Comm> globalComm;
#ifdef HAVE_MPI
    globalComm = Teuchos::rcp(new Epetra_MpiComm(MPI_COMM_WORLD));
#else
    globalComm = Teuchos::rcp(new Epetra_SerialComm);
#endif
    MyPID = globalComm->MyPID();

    // Parse args
    if (argc > 1 && argc != 2) {
      std::cout << "Usage:  pce.exe [num_spatial_procs]" << std::endl;
      exit(-1);
    }

    int num_spatial_procs = -1;
    if (argc == 2)
      num_spatial_procs = std::atoi(argv[1]);

    // Create SG basis to setup parallel correctly
    Teuchos::ParameterList sgParams;
    Teuchos::ParameterList& basisParams = sgParams.sublist("Basis");
    basisParams.set("Dimension", num_KL);
    for (int i=0; i<num_KL; i++) {
      std::ostringstream ss;
      ss << "Basis " << i;
      Teuchos::ParameterList& bp = basisParams.sublist(ss.str());
      bp.set("Type", "Legendre");
      bp.set("Order", p);
    }
    Teuchos::RCP<const Stokhos::OrthogPolyBasis<int,double> > basis = 
      Stokhos::BasisFactory<int,double>::create(sgParams);

    // Create multi-level comm and spatial comm
    Teuchos::RCP<const EpetraExt::MultiComm> sg_comm =
      Stokhos::buildMultiComm(*globalComm, basis->size(), num_spatial_procs);
    Teuchos::RCP<const Epetra_Comm> app_comm = Stokhos::getSpatialComm(sg_comm);

    // Set up application parameters
    Teuchos::RCP<Teuchos::ParameterList> appParams = 
      Teuchos::rcp(new Teuchos::ParameterList);

    // Problem
    Teuchos::ParameterList& problemParams = 
      appParams->sublist("Problem");
    problemParams.set("Name", "Heat Nonlinear Source");

    // Boundary conditions
    problemParams.set("Left BC", leftBC);
    problemParams.set("Right BC", rightBC);

    // Source function
    Teuchos::ParameterList& sourceParams = 
      problemParams.sublist("Source Function");
    sourceParams.set("Name", "Exponential");
    sourceParams.set("Nonlinear Factor", alpha);

     // Material
    Teuchos::ParameterList& matParams = 
      problemParams.sublist("Material Function");
    matParams.set("Name", "KL Exponential Random Field");
    matParams.set("Mean", 1.0);
    matParams.set("Standard Deviation", 0.6);
    matParams.set("Number of KL Terms", num_KL);
    Teuchos::Array<double> a(1), b(1), L(1);
    a[0] = 0.0; b[0] = 1.0; L[0] = 1.0;
    matParams.set("Domain Lower Bounds", a);
    matParams.set("Domain Upper Bounds", b);
    matParams.set("Correlation Lengths", L);

    // Response functions
    Teuchos::ParameterList& responseParams =
      problemParams.sublist("Response Functions");
    responseParams.set("Number", 1);
    responseParams.set("Response 0", "Solution Average");

    // Parameters
    Teuchos::ParameterList& parameterParams = 
      problemParams.sublist("Parameters");
    parameterParams.set("Number of Parameter Vectors", 1);
    Teuchos::ParameterList& pParams = 
      parameterParams.sublist("Parameter Vector 0");
    pParams.set("Number", num_KL);
    for (int i=0; i<num_KL; i++) {
      std::stringstream ss1, ss2;
      ss1 << "Parameter " << i;
      ss2 << "KL Exponential Function Random Variable " << i;
      pParams.set(ss1.str(), ss2.str());
    }
    
    // Mesh
    Teuchos::ParameterList& discParams = appParams->sublist("Discretization");
    discParams.set("Number of Elements", nelem);

    // Set up NOX parameters
    Teuchos::RCP<Teuchos::ParameterList> noxParams =
      Teuchos::rcp(&(appParams->sublist("NOX")),false);

    // Set the nonlinear solver method
    noxParams->set("Nonlinear Solver", "Line Search Based");

    // Set the printing parameters in the "Printing" sublist
    Teuchos::ParameterList& printParams = noxParams->sublist("Printing");
    printParams.set("MyPID", MyPID); 
    printParams.set("Output Precision", 3);
    printParams.set("Output Processor", 0);
    printParams.set("Output Information", 
                    NOX::Utils::OuterIteration + 
                    NOX::Utils::OuterIterationStatusTest + 
                    NOX::Utils::InnerIteration +
                    //NOX::Utils::Parameters + 
                    //NOX::Utils::Details + 
                    NOX::Utils::LinearSolverDetails +
                    NOX::Utils::Warning + 
                    NOX::Utils::Error);

    // Create printing utilities
    NOX::Utils utils(printParams);

    // Sublist for line search 
    Teuchos::ParameterList& searchParams = noxParams->sublist("Line Search");
    searchParams.set("Method", "Full Step");

    // Sublist for direction
    Teuchos::ParameterList& dirParams = noxParams->sublist("Direction");
    dirParams.set("Method", "Newton");
    Teuchos::ParameterList& newtonParams = dirParams.sublist("Newton");
    newtonParams.set("Forcing Term Method", "Constant");

    // Alternative linear solver list for Stratimikos
    Teuchos::ParameterList& stratLinSolParams =
      newtonParams.sublist("Stratimikos Linear Solver");
    Teuchos::ParameterList& stratParams = 
      stratLinSolParams.sublist("Stratimikos");

    // Sublist for linear solver for the Newton method
    stratParams.set("Linear Solver Type", "AztecOO");
    Teuchos::ParameterList& aztecOOParams = 
      stratParams.sublist("Linear Solver Types").sublist("AztecOO").sublist("Forward Solve");
    Teuchos::ParameterList& aztecOOSettings =
      aztecOOParams.sublist("AztecOO Settings");
    aztecOOSettings.set("Aztec Solver","GMRES");
    aztecOOParams.set("Max Iterations", 500);
    aztecOOSettings.set("Size of Krylov Subspace", 500);
    aztecOOParams.set("Tolerance", 1e-12); 
    aztecOOSettings.set("Output Frequency", 50);
    stratParams.set("Preconditioner Type", "Ifpack");

    // Sublist for convergence tests
    Teuchos::ParameterList& statusParams = noxParams->sublist("Status Tests");
    statusParams.set("Test Type", "Combo");
    statusParams.set("Number of Tests", 2);
    statusParams.set("Combo Type", "OR");
    Teuchos::ParameterList& comboParams = statusParams.sublist("Test 0");
    comboParams.set("Test Type", "Combo");
    comboParams.set("Number of Tests", 2);
    comboParams.set("Combo Type", "AND");
    Teuchos::ParameterList& normF = comboParams.sublist("Test 0");
    normF.set("Test Type", "NormF");
    normF.set("Tolerance", 1e-10);
    normF.set("Scale Type", "Scaled");
    Teuchos::ParameterList& normWRMS = comboParams.sublist("Test 1");
    normWRMS.set("Test Type", "NormWRMS");
    normWRMS.set("Relative Tolerance", 1e-6);
    normWRMS.set("Absolute Tolerance", 1e-8);
    Teuchos::ParameterList& maxIters = statusParams.sublist("Test 1");
    maxIters.set("Test Type", "MaxIters");
    maxIters.set("Maximum Iterations", 5);

    // Create application
    Teuchos::RCP<FEApp::Application> app = 
      Teuchos::rcp(new FEApp::Application(app_comm, appParams));

    // Create model evaluator
    Teuchos::RCP<EpetraExt::ModelEvaluator> model = 
      Teuchos::rcp(new FEApp::ModelEvaluator(app, appParams));

    // Create NOX solver
    Piro::Epetra::NOXSolver solver(appParams, model);

    // Evaluate responses at parameters
    EpetraExt::ModelEvaluator::InArgs inArgs = solver.createInArgs();
    EpetraExt::ModelEvaluator::OutArgs outArgs = solver.createOutArgs();
    Teuchos::RCP<const Epetra_Vector> p_init = solver.get_p_init(0);
    Teuchos::RCP<Epetra_Vector> g = 
      Teuchos::rcp(new Epetra_Vector(*(solver.get_g_map(0))));
    Teuchos::RCP<Epetra_Vector> finalSolution = 
      Teuchos::rcp(new Epetra_Vector(*(solver.get_g_map(1))));
    inArgs.set_p(0, p_init);
    outArgs.set_g(0, g);
    outArgs.set_g(1, finalSolution);
    solver.evalModel(inArgs, outArgs);

    g->Print(utils.out());

    Teuchos::TimeMonitor::summarize(std::cout);
    Teuchos::TimeMonitor::zeroOutTimers();

    {

      TEUCHOS_FUNC_TIME_MONITOR("Total PCE Calculation Time");
    
      // Copy in params from above
      appParams->sublist("Stochastic Galerkin Parameters") = sgParams;
      
      // Create SG quadrature
      Teuchos::ParameterList& quadParams = sgParams.sublist("Quadrature");
      quadParams.set("Type", "Tensor Product");
      //quadParams.set("Sparse Grid Level", p);
      Teuchos::RCP<const Stokhos::Quadrature<int,double> > quad = 
	Stokhos::QuadratureFactory<int,double>::create(sgParams);
      
      
      // Create SG expansion
      Teuchos::ParameterList& expParams = sgParams.sublist("Expansion");
      expParams.set("Type", "Quadrature");
      Teuchos::RCP<Stokhos::OrthogPolyExpansion<int,double> > expansion = 
      	Stokhos::ExpansionFactory<int,double>::create(sgParams);
      Teuchos::RCP<const Stokhos::Sparse3Tensor<int,double> > Cijk =
	sgParams.get< Teuchos::RCP<const Stokhos::Sparse3Tensor<int,double> > >
	("Triple Product Tensor");

      int sz = basis->size();
      if (MyPID == 0)
	std::cout << "sz = " << sz << std::endl;

      // Create stochastic parallel distribution
      Teuchos::ParameterList parallelParams;
      Teuchos::RCP<Stokhos::ParallelData> sg_parallel_data =
	Teuchos::rcp(new Stokhos::ParallelData(basis, Cijk, sg_comm,
					       parallelParams));
      
      appParams->set("SG Method", "AD");

      // Create new app for Stochastic Galerkin solve
      app = Teuchos::rcp(new FEApp::Application(app_comm, appParams,
						finalSolution.get()));
      model = Teuchos::rcp(new FEApp::ModelEvaluator(app, appParams));

      Teuchos::RCP<Teuchos::ParameterList> sgSolverParams = 
	Teuchos::rcp(&sgParams.sublist("Solver"), false);
      Teuchos::ParameterList& sgOpParams = 
	sgSolverParams->sublist("SG Operator");
      Teuchos::ParameterList& sgPrecParams = 
      sgSolverParams->sublist("SG Preconditioner");
      //sgOpParams.set("Operator Method", "Matrix Free");
      sgOpParams.set("Operator Method", "Fully Assembled");
      sgOpParams.set("Number of KL Terms", num_KL+1);
      sgPrecParams.set("Preconditioner Method", "Mean-based");
      //sgPrecParams.set("Preconditioner Method", "Fully Assembled");
      //sgPrecParams.set("Mean Preconditioner Type", "ML");
      sgPrecParams.set("Mean Preconditioner Type", "Ifpack");
      Teuchos::ParameterList& precParams = 
        sgPrecParams.sublist("Mean Preconditioner Parameters");
      precParams.set("default values", "SA");
      sgSolverParams->set("Evaluate W with F", false);
      Teuchos::RCP<Stokhos::SGModelEvaluator> sg_model =
	Teuchos::rcp(new Stokhos::SGModelEvaluator(model, basis, quad, 
						   expansion, sg_parallel_data, 
						   sgSolverParams));

      // Set up stochastic parameters
      Teuchos::RCP<Stokhos::EpetraVectorOrthogPoly> sg_p_init =
	sg_model->create_p_sg(0);
      for (int i=0; i<num_KL; i++) {
	sg_p_init->term(i,0)[i] = 0.0;
	sg_p_init->term(i,1)[i] = 1.0;
      }
      sg_model->set_p_sg_init(0, *sg_p_init);

      // Setup stochastic initial guess
      Teuchos::RCP<Stokhos::EpetraVectorOrthogPoly> sg_x = 
        sg_model->create_x_sg();
      sg_x->init(0.0);
      if (sg_x->myGID(0))
        (*sg_x)[0] = *finalSolution;
      sg_model->set_x_sg_init(*sg_x);

      //appParams->set("Transpose Solver Method", "Explicit Transpose");

      // Create SG NOX solver
      Teuchos::RCP<EpetraExt::ModelEvaluator> sg_block_solver = 
        Teuchos::rcp(new Piro::Epetra::NOXSolver(appParams, sg_model));

      // Create ResponseStatistic model evaluator
      Teuchos::Array< Teuchos::RCP<const Epetra_Map> > base_g_maps = 
	sg_model->get_g_sg_base_maps();
      base_g_maps.push_back(app->getMap());
      Teuchos::RCP<const Epetra_BlockMap> block_map =
        sg_model->get_overlap_stochastic_map();
      Teuchos::RCP<EpetraExt::ModelEvaluator> sg_solver = 
	Teuchos::rcp(new Stokhos::ResponseStatisticModelEvaluator(
		       sg_block_solver, base_g_maps, basis, sg_comm, 
                       block_map));
      
      // Evaluate SG responses at SG parameters
      EpetraExt::ModelEvaluator::InArgs sg_inArgs = sg_solver->createInArgs();
      EpetraExt::ModelEvaluator::OutArgs sg_outArgs = 
	sg_solver->createOutArgs();
      int p_index = 0; // PC expansion coefficients of params
      Teuchos::RCP<const Epetra_Vector> p_init = sg_solver->get_p_init(p_index);
      int g_index = 0;
      int num_g = 2;
      Teuchos::RCP<Epetra_Vector> g_mean = 
        Teuchos::rcp(new Epetra_Vector(*(model->get_g_map(g_index))));
      Teuchos::RCP<Epetra_Vector> g_var = 
        Teuchos::rcp(new Epetra_Vector(*(model->get_g_map(g_index))));
      Teuchos::RCP<Epetra_MultiVector> dgdp_mean = 
	Teuchos::rcp(new Epetra_MultiVector(*(sg_solver->get_p_map(p_index)),
					    model->get_g_map(g_index)->NumMyElements()));
      Teuchos::RCP<Epetra_MultiVector> dgdp_var = 
	Teuchos::rcp(new Epetra_MultiVector(*(sg_solver->get_p_map(p_index)),
					    model->get_g_map(g_index)->NumMyElements()));
      
      int g_mean_index = g_index + num_g;
      int g_var_index = g_index + 2*num_g;
      sg_outArgs.set_g(g_mean_index, g_mean);
      sg_outArgs.set_g(g_var_index, g_var);
      sg_outArgs.set_DgDp(
	g_mean_index, p_index, 
	EpetraExt::ModelEvaluator::Derivative(
	  dgdp_mean, 
	  EpetraExt::ModelEvaluator::DERIV_TRANS_MV_BY_ROW
	  )
	);
      sg_outArgs.set_DgDp(
	g_var_index, p_index, 
	EpetraExt::ModelEvaluator::Derivative(
	  dgdp_var, 
	  EpetraExt::ModelEvaluator::DERIV_TRANS_MV_BY_ROW
	  )
	);

      sg_solver->evalModel(sg_inArgs, sg_outArgs);

      // Print mean and standard deviation
      //utils.out().precision(12);
      utils.out() << "Mean =     " << (*g_mean)[0] << std::endl;
      utils.out() << "Variance = " << (*g_var)[0] << std::endl;
      utils.out() << "d(Mean)/dp = " << std::endl << *dgdp_mean << std::endl;
      utils.out() << "d(Var)/dp = " << std::endl << *dgdp_var << std::endl;

      /*
      // Test derivatives with finite differences
      double delta = 1.0e-6;
      int num_p = sg_solver->get_p_map(p_index)->NumMyElements();
      int num_resp = model->get_g_map(g_index)->NumMyElements();
      Teuchos::RCP<Epetra_Vector> p_pert = 
	Teuchos::rcp(new Epetra_Vector((*sg_solver->get_p_map(p_index))));
      Teuchos::RCP<Epetra_Vector> g_mean_pert = 
        Teuchos::rcp(new Epetra_Vector(*(model->get_g_map(g_index))));
      Teuchos::RCP<Epetra_Vector> g_var_pert = 
        Teuchos::rcp(new Epetra_Vector(*(model->get_g_map(g_index))));
      Teuchos::RCP<Epetra_MultiVector> dgdp_mean_fd = 
	Teuchos::rcp(new Epetra_MultiVector(*(sg_solver->get_p_map(p_index)),
					    num_resp));
      Teuchos::RCP<Epetra_MultiVector> dgdp_var_fd = 
	Teuchos::rcp(new Epetra_MultiVector(*(sg_solver->get_p_map(p_index)),
					    num_resp));
      EpetraExt::ModelEvaluator::InArgs sg_inArgs_pert = 
	sg_solver->createInArgs();
      EpetraExt::ModelEvaluator::OutArgs sg_outArgs_pert = 
	sg_solver->createOutArgs();
      sg_inArgs_pert.set_p(p_index, p_pert);
      sg_outArgs_pert.set_g(g_mean_index, g_mean_pert);
      sg_outArgs_pert.set_g(g_var_index, g_var_pert);
      for (int i=0; i<num_p; i++) {

	// Perturb p
	//double h = delta*(std::abs((*p_init)[i])+delta);
	double h = delta;
	*p_pert = *p_init;
	(*p_pert)[i] += h;

	// Init perturbed g
	g_mean_pert->PutScalar(0.0);
	g_var_pert->PutScalar(0.0);
	
	// Compute perturbed g
	sg_solver->evalModel(sg_inArgs_pert, sg_outArgs_pert);

	// Compute FD derivatives
	for (int j=0; j<num_resp; j++) {
	  (*dgdp_mean_fd)[j][i] = ((*g_mean_pert)[j] - (*g_mean)[j])/h;
	  (*dgdp_var_fd)[j][i] = ((*g_var_pert)[j] - (*g_var)[j])/h;
	}
      }

      utils.out() << "d(Mean)/dp = " << std::endl << *dgdp_mean << std::endl;
      utils.out() << "d(Mean)/dp FD = " << std::endl << *dgdp_mean_fd 
		  << std::endl;
      utils.out() << "d(Var)/dp = " << std::endl << *dgdp_var << std::endl;
      utils.out() << "d(Var)/dp FD = " << std::endl << *dgdp_var_fd 
		  << std::endl;
      */
      if (!sg_outArgs.isFailed() && MyPID == 0) 
        utils.out() << "Test Passed!" << endl;

    }

    Teuchos::TimeMonitor::summarize(std::cout);
    Teuchos::TimeMonitor::zeroOutTimers();

#ifdef HAVE_MPI
    MPI_Finalize() ;
#endif

  }
  
  catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }
  catch (string& s) {
    std::cout << s << std::endl;
  }
  catch (char *s) {
    std::cout << s << std::endl;
  }
  catch (...) {
    std::cout << "Caught unknown exception!" <<std:: endl;
  }

}
