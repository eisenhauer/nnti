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
//  CVS Information
//  $Source$
//  $Author$
//  $Date$
//  $Revision$
// ************************************************************************
//@HEADER

#include <iostream>
#include <sstream>

// FEApp is defined in Trilinos/packages/sacado/example/FEApp
#include "FEApp_ModelEvaluator.hpp"

// Epetra communicator
#ifdef HAVE_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif

// AztecOO solver
#include "AztecOO.h"

// Stokhos Stochastic Galerkin
#include "Stokhos.hpp"
#include "EpetraExt_BlockVector.h"

// Timing utilities
#include "Teuchos_TimeMonitor.hpp"

int main(int argc, char *argv[]) {
  int nelem = 100;
  double h = 1.0/nelem;
  int num_KL = 3;
  int p = 5;
  bool full_expansion = true;

// Initialize MPI
#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
#endif

  int MyPID;

  try {

    {
    TEUCHOS_FUNC_TIME_MONITOR("Total PCE Calculation Time");

    // Create a communicator for Epetra objects
    Teuchos::RCP<Epetra_Comm> Comm;
#ifdef HAVE_MPI
    Comm = Teuchos::rcp(new Epetra_MpiComm(MPI_COMM_WORLD));
#else
    Comm = Teuchos::rcp(new Epetra_SerialComm);
#endif

    MyPID = Comm->MyPID();
    
    // Create mesh
    std::vector<double> x(nelem+1);
    for (int i=0; i<=nelem; i++)
      x[i] = h*i;

    // Set up application parameters
    Teuchos::RCP<Teuchos::ParameterList> appParams = 
      Teuchos::rcp(new Teuchos::ParameterList);

    // Problem
    Teuchos::ParameterList& problemParams = 
      appParams->sublist("Problem");
    problemParams.set("Name", "Heat Nonlinear Source");

    // Boundary conditions
    problemParams.set("Left BC", 0.0);
    problemParams.set("Right BC", 0.0);

    // Source function
    Teuchos::ParameterList& sourceParams = 
      problemParams.sublist("Source Function");
    sourceParams.set("Name", "Constant");
    sourceParams.set("Constant Value", 1.0);

    // Material
    Teuchos::ParameterList& matParams = 
      problemParams.sublist("Material Function");
    matParams.set("Name", "KL Exponential Random Field");
    matParams.set("Mean", 1.0);
    matParams.set("Standard Deviation", 0.5);
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

    // Free parameters (determinisic, e.g., for sensitivities)
    Teuchos::RefCountPtr< Teuchos::Array<std::string> > free_param_names =
	Teuchos::rcp(new Teuchos::Array<std::string>);
    free_param_names->push_back("Constant Source Function Value");
    
    // Create Stochastic Galerkin basis and expansion
    Teuchos::Array< Teuchos::RCP<const Stokhos::OneDOrthogPolyBasis<int,double> > > bases(num_KL); 
    for (int i=0; i<num_KL; i++)
      bases[i] = Teuchos::rcp(new Stokhos::LegendreBasis<int,double>(p));
    Teuchos::RCP<const Stokhos::CompletePolynomialBasis<int,double> > basis = 
      Teuchos::rcp(new Stokhos::CompletePolynomialBasis<int,double>(bases));
    int sz = basis->size();
    Teuchos::RCP<Stokhos::Sparse3Tensor<int,double> > Cijk;
    if (full_expansion)
      Cijk = basis->computeTripleProductTensor(sz);
    else
      Cijk = basis->computeTripleProductTensor(num_KL+1);
    Teuchos::RCP<Stokhos::OrthogPolyExpansion<int,double> > expansion = 
      Teuchos::rcp(new Stokhos::AlgebraicOrthogPolyExpansion<int,double>(basis,
									 Cijk));
    std::cout << "Stochastic Galerkin expansion size = " << sz << std::endl;

    // Create application
    appParams->set("SG Method", "AD");
    Teuchos::RCP<FEApp::Application> app = 
      Teuchos::rcp(new FEApp::Application(x, Comm, appParams, false));
    
    // Set up stochastic parameters
    Epetra_LocalMap p_sg_map(num_KL, 0, *Comm);
    Teuchos::RCP<Stokhos::EpetraVectorOrthogPoly> sg_p_init = 
      Teuchos::rcp(new Stokhos::EpetraVectorOrthogPoly(basis, p_sg_map));
    for (int i=0; i<num_KL; i++) {
      sg_p_init->term(i,0)[i] = 0.0;
      sg_p_init->term(i,1)[i] = 1.0;
    }
    Teuchos::RefCountPtr< Teuchos::Array<std::string> > sg_param_names =
      Teuchos::rcp(new Teuchos::Array<std::string>);
    for (int i=0; i<num_KL; i++) {
      std::stringstream ss;
      ss << "KL Exponential Function Random Variable " << i;
      sg_param_names->push_back(ss.str());
    }

    // Setup stochastic initial guess
    Teuchos::RCP<Stokhos::EpetraVectorOrthogPoly> sg_x_init = 
      Teuchos::rcp(new Stokhos::EpetraVectorOrthogPoly(basis, 
						       *(app->getMap())));
    sg_x_init->init(0.0);
    
    // Create application model evaluator
    Teuchos::RCP<EpetraExt::ModelEvaluator> model = 
      Teuchos::rcp(new FEApp::ModelEvaluator(app, free_param_names,
					     sg_param_names, sg_x_init, 
					     sg_p_init));
    
    // Setup stochastic Galerkin algorithmic parameters
    Teuchos::RCP<Teuchos::ParameterList> sgParams = 
      Teuchos::rcp(&(appParams->sublist("SG Parameters")),false);
    if (!full_expansion) {
      sgParams->set("Parameter Expansion Type", "Linear");
      sgParams->set("Jacobian Expansion Type", "Linear");
    }
    sgParams->set("Jacobian Method", "Matrix Free");
    sgParams->set("Mean Preconditioner Type", "ML");
    Teuchos::ParameterList& precParams = 
      sgParams->sublist("Preconditioner Parameters");
    precParams.set("default values", "DD");

    // Create stochastic Galerkin model evaluator
    Teuchos::RCP<Stokhos::SGModelEvaluator> sg_model =
      Teuchos::rcp(new Stokhos::SGModelEvaluator(model, basis, Teuchos::null,
						 expansion, Cijk, sgParams,
						 Comm));

    // Create vectors and operators
    Teuchos::RCP<const Epetra_Vector> sg_p = sg_model->get_p_init(2);
    Teuchos::RCP<Epetra_Vector> sg_x = 
      Teuchos::rcp(new Epetra_Vector(*(sg_model->get_x_map())));
    Teuchos::RCP<Epetra_Vector> sg_y = 
      Teuchos::rcp(new Epetra_Vector(*(sg_model->get_x_map())));
    *sg_x = *(sg_model->get_x_init());
    Teuchos::RCP<Epetra_Vector> sg_f = 
      Teuchos::rcp(new Epetra_Vector(*(sg_model->get_f_map())));
    Teuchos::RCP<Epetra_Vector> sg_df = 
      Teuchos::rcp(new Epetra_Vector(*(sg_model->get_f_map())));
    Teuchos::RCP<Epetra_Vector> sg_dx = 
      Teuchos::rcp(new Epetra_Vector(*(sg_model->get_x_map())));
    Teuchos::RCP<Epetra_Operator> sg_J = sg_model->create_W();
    Teuchos::RCP<Epetra_Operator> sg_M = sg_model->create_WPrec()->PrecOp;
    
    // std::cout << "parameter PC expansion:" << std::endl 
//            << *sg_p_init << std::endl;
  //  std::cout << "parameter block vector:" << std::endl 
//            << *sg_p << std::endl;
//
  //  std::cout << "parameter PC expansion:" << std::endl 
//            << *sg_x_init << std::endl;
  //  std::cout << "parameter block vector:" << std::endl 
//            << *sg_x << std::endl;

  //  EpetraExt::BlockVector sg_p_block(View, p_sg_map, *sg_p);
  //  Teuchos::RCP<const Epetra_Vector> sg_p_vec = sg_p_block.GetBlock(2);

//    EpetraExt::BlockVector sg_x_block(View, *(app->getMap()), *sg_x);
  //  Teuchos::RCP<const Epetra_Vector> sg_x_vec = sg_x_block.GetBlock(2);


    // Setup InArgs and OutArgs
    EpetraExt::ModelEvaluator::InArgs sg_inArgs = sg_model->createInArgs();
    EpetraExt::ModelEvaluator::OutArgs sg_outArgs = sg_model->createOutArgs();
    sg_inArgs.set_p(2, sg_p);
    sg_inArgs.set_x(sg_x);
    sg_outArgs.set_f(sg_f);
    sg_outArgs.set_W(sg_J);
    sg_outArgs.set_WPrec(sg_M);

    // Evaluate model
    sg_model->evalModel(sg_inArgs, sg_outArgs);

    Teuchos::RCP<Stokhos::MatrixFreeEpetraOp> stokhos_op =
      Teuchos::rcp_dynamic_cast<Stokhos::MatrixFreeEpetraOp>(sg_J, true);
    Teuchos::RCP<Stokhos::VectorOrthogPoly<Epetra_Operator> > sg_J_poly
      = stokhos_op->getOperatorBlocks();
    // to get k-th matrix:  (*sg_J_poly)[k]

    Teuchos::RCP<Stokhos::MeanEpetraOp> mean_op = 
      Teuchos::rcp_dynamic_cast<Stokhos::MeanEpetraOp>(sg_M, true);

    Teuchos::RCP<Epetra_Operator> mean_prec
      = mean_op->getMeanOperator();

    //const Stokhos::VectorOrthogPoly<Epetra_Operator>& sg_J_poly_ref = *sg_J_poly;

    std::vector< Teuchos::RCP<const Epetra_Vector> > sg_p_vec_all ;
    std::vector< Teuchos::RCP< Epetra_Vector> > sg_x_vec_all ;
    std::vector< Teuchos::RCP< Epetra_Vector> > sg_dx_vec_all ; 
    std::vector< Teuchos::RCP< Epetra_Vector> > sg_f_vec_all ;

    // Extract blocks
    EpetraExt::BlockVector sg_p_block(View, p_sg_map, *sg_p);
    EpetraExt::BlockVector sg_x_block(View, *(app->getMap()), *sg_x);
    EpetraExt::BlockVector sg_dx_block(View, *(app->getMap()), *sg_dx);
    EpetraExt::BlockVector sg_f_block(View, *(app->getMap()), *sg_f);

    // sg_p_vec_all.push_back(sg_p_block.GetBlock(0));
    for (int i=0; i<sz; i++) {
      sg_p_vec_all.push_back(sg_p_block.GetBlock(i));
      sg_x_vec_all.push_back(sg_x_block.GetBlock(i));
      sg_dx_vec_all.push_back(sg_dx_block.GetBlock(i));
      sg_f_vec_all.push_back(sg_f_block.GetBlock(i));
    }

   // std::cout << "parameter block vector:" << std::endl
     //         << (*sg_J_poly).getCoeffPtr(0) << std::endl;
//    std::cout << "parameter block vector:" << std::endl
  //            << ((*sg_J_poly).getCoefficients())[0] << std::endl;
//    std::cout << "Block Matrices:" << std::endl
  //            << (*sg_J_poly)[0] << std::endl;

   // std::cout << "force block vector:" << std::endl
     //         << *sg_f_vec_all[0] << std::endl;

    // Print initial residual norm
    double norm_f,norm_df;
    sg_f->Norm2(&norm_f);
    std::cout << "\nInitial residual norm = " << norm_f << std::endl;
    
    std::vector< Teuchos::RCP< Epetra_Vector> > sg_df_vec_all ;
    std::vector< Teuchos::RCP< Epetra_Vector> > sg_kx_vec_all ;
    for (int i=0; i<sz; i++) {
      Teuchos::RCP<Epetra_Vector> dff =
	   Teuchos::rcp(new Epetra_Vector(*(app->getMap())));
      Teuchos::RCP<Epetra_Vector> dxx =
	   Teuchos::rcp(new Epetra_Vector(*(app->getMap())));
      sg_df_vec_all.push_back(dff);
      sg_kx_vec_all.push_back(dxx);      
    }

//  Teuchos::RCP< Epetra_Vector> kx ;
    Teuchos::RCP<Epetra_Vector> kx =
      Teuchos::rcp(new Epetra_Vector(*(app->getMap())));
    Teuchos::RCP<Epetra_Vector> dx =
      Teuchos::rcp(new Epetra_Vector(*(app->getMap())));
    Teuchos::RCP<Epetra_Vector> df =
      Teuchos::rcp(new Epetra_Vector(*(app->getMap())));

   // (*sg_J_poly)[0].Apply(*(sg_f_vec_all[0]),*kx);
   // std::cout << "f(0):" << std::endl
   //           << *(sg_f_vec_all[0]) << std::endl;
   // std::cout << "kx:" << std::endl
  //            << *kx << std::endl;

    // Setup AztecOO solver
    AztecOO aztec;
    aztec.SetAztecOption(AZ_solver, AZ_gmres);
    aztec.SetAztecOption(AZ_precond, AZ_none);
    aztec.SetAztecOption(AZ_kspace, 20);
    aztec.SetAztecOption(AZ_conv, AZ_r0);
    aztec.SetAztecOption(AZ_output, 1);
    //aztec.SetAztecOption(AZ_output, AZ_none);
    aztec.SetUserOperator((*sg_J_poly).getCoeffPtr(0).get());
    aztec.SetPrecOperator(mean_prec.get());
//    aztec.SetLHS(dx.get());

 std::vector<double> cii0(sz);
 int nj = Cijk->num_j(0);
 const Teuchos::Array<int>& j_indices = Cijk->Jindices(0);
 //std::cout << "j_indices for k =" << k << j_indices << std::endl;
 for (int jj=0; jj<nj; jj++) {
   int j = j_indices[jj];
   const Teuchos::Array<double>& cijk_values = Cijk->values(0,jj);
   const Teuchos::Array<int>& i_indices = Cijk->Iindices(0,jj);
   int ni = i_indices.size();
   for (int ii=0; ii<ni; ii++) {
     int i = i_indices[ii];
     if (i==j) {
      cii0[i] = cijk_values[ii];  // C(i,j,k)
     }
   }
 } 

norm_df = 1.0;
int iter = 0;
//for (int iter=0;iter<1;iter++){
while ((norm_df/norm_f)>1e-12) {
    TEUCHOS_FUNC_TIME_MONITOR("Total global solve Time");
  iter++;
     // Extract blocks
    EpetraExt::BlockVector sg_f_block(View, *(app->getMap()), *sg_f);

    // sg_p_vec_all.push_back(sg_p_block.GetBlock(0));
    // for (int i=0; i<sz; i++) {
    //  sg_f_vec_all.push_back(sg_f_block.GetBlock(i));
    // }

//     double c0kk;
    // Loop over Cijk entries including a non-zero in the graph at
    // indices (i,j) if there is any k for which Cijk is non-zero
  //  ordinal_type Cijk_size = Cijk.size();
    for (int i=0; i<sz; i++) {
      (sg_df_vec_all[i])->Update(1.0, *sg_f_vec_all[i], 0.0);
    } 
    for (int k=1; k<num_KL+1; k++) {
//      df->Update(1.0, *sg_f_vec_all[k], 0.0);
      int nj = Cijk->num_j(k);
      const Teuchos::Array<int>& j_indices = Cijk->Jindices(k);
      //std::cout << "j_indices for k =" << k << j_indices << std::endl;
      for (int jj=0; jj<nj; jj++) {
        int j = j_indices[jj];
        (*sg_J_poly)[k].Apply(*(sg_dx_vec_all[j]),*(sg_kx_vec_all[j]));
      }
      for (int jj=0; jj<nj; jj++) {
        int j = j_indices[jj];
        const Teuchos::Array<double>& cijk_values = Cijk->values(k,jj);
        const Teuchos::Array<int>& i_indices = Cijk->Iindices(k,jj);
        int ni = i_indices.size();
        for (int ii=0; ii<ni; ii++) {
          int i = i_indices[ii];
          double c = cijk_values[ii];  // C(i,j,k)
          sg_df_vec_all[i]->Update(-1.0*c,*(sg_kx_vec_all[j]),1.0);          
        }
      }    
    } //End of k loop

    for(int i=0; i<sz; i++) {
      sg_df_vec_all[i]->Scale(1/cii0[i]);
      aztec.SetRHS((sg_df_vec_all[i]).get());
      aztec.SetLHS((sg_dx_vec_all[i]).get());
      // Solve linear system
      {
       TEUCHOS_FUNC_TIME_MONITOR("Total deterministic solve Time");
       aztec.Iterate(100, 1e-12);
      }
      //std::cout << "sg_dx_vec_all[0]" << *(sg_dx_vec_all[0]) << std::endl;
      // Update x
//      sg_dx_vec_all[i]->Update(1.0, *(sg_dx_vec_all[i]), 0.0);
    } 

    sg_J->Apply(*(sg_dx),*(sg_y));
    sg_df->Update(1.0,*sg_y,-1.0,*sg_f,0.0);
//    sg_df->Update(1.0,*sg_y,-1.0,*sg_dx,0.0);
    sg_df->Norm2(&norm_df);
    std::cout << "rel residual norm at iteration "<< iter <<" is " << norm_df/norm_f << std::endl;
//    sg_y->Update(1.0,*sg_dx,0.0);
  } //End of iter loop 

  for (int k=0; k<sz; k++) {
     sg_x_vec_all[k]->Update(-1.0, *sg_dx_vec_all[k], 1.0);
  }

//  sg_J->Apply(*(sg_dx),*(sg_y));
//  sg_df->Update(1.0,*sg_y,-1.0,*sg_f,0.0);
//  std::cout << "sg_dx:" << std::endl
  //           << *sg_y << std::endl;

//  sg_df->Norm2(&norm_df);

//  std::cout << "\nFinal residual norm df= " << norm_df << std::endl;
//  std::cout << "relative norm" << norm_df/norm_f << std::endl;  
   // std::cout << "x(0):" << std::endl
    //          << *(sg_x_vec_all[55]) << std::endl;

    //  int LoadBlockValues(const Epetra_Vector & BaseVec, int BlockRow)
    //    sg_x_block->LoadBlockValues(*(sg_x_vec_all[0]),0);

//    std::cout << "sg_x_block" << *sg_y << std::endl;

//    sg_inArgs.set_x(sg_x);

    // Compute new residual & response function
    Teuchos::RCP<Epetra_Vector> sg_g = 
      Teuchos::rcp(new Epetra_Vector(*(sg_model->get_g_map(1))));
    EpetraExt::ModelEvaluator::OutArgs sg_outArgs2 = sg_model->createOutArgs();
    sg_outArgs2.set_f(sg_f);
    sg_outArgs2.set_g(1, sg_g);
    sg_model->evalModel(sg_inArgs, sg_outArgs2);

    // Print Final residual norm
    sg_f->Norm2(&norm_f);
    std::cout << "\nFinal residual norm = " << norm_f << std::endl;

    // Print mean and standard deviation
    Stokhos::EpetraVectorOrthogPoly sg_g_poly(basis, View, 
					      *(model->get_g_map(0)), *sg_g);
    Epetra_Vector mean(*(model->get_g_map(0)));
    Epetra_Vector std_dev(*(model->get_g_map(0)));
    sg_g_poly.computeMean(mean);
    sg_g_poly.computeStandardDeviation(std_dev);
    std::cout << "\nResponse Expansion = " << std::endl;
    std::cout.precision(12);
    sg_g_poly.print(std::cout);
    std::cout << "\nResponse Mean =      " << std::endl << mean << std::endl;
    std::cout << "Response Std. Dev. = " << std::endl << std_dev << std::endl;

    if (norm_f < 1.0e-10)
      std::cout << "Test Passed!" << std::endl;

    }

    Teuchos::TimeMonitor::summarize(std::cout);
    Teuchos::TimeMonitor::zeroOutTimers();

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

#ifdef HAVE_MPI
  MPI_Finalize() ;
#endif

}
