/*
#@HEADER
# ************************************************************************
#
#               ML: A Multilevel Preconditioner Package
#                 Copyright (2002) Sandia Corporation
#
# Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the U.S. Government.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
# Questions? Contact Jonathan Hu (jhu@sandia.gov) or Ray Tuminaro 
# (rstumin@sandia.gov).
#
# ************************************************************************
#@HEADER
*/
                                                                                
// 1D Finite Element Test Problem
/* Solves the nonlinear equation:
 *
 * d2u 
 * --- - k * u**2 = 0
 * dx2
 *
 * subject to @ x=0, u=1
 */
// see documentation for the ml-class ML_NOX::ML_Nox_Preconditioner.
// compiling and running this example requires a bunch of packages:
// --enable-nox --enable-prerelease
// --enable-nox-epetra
// --enable-epetra
// --enable-epetraext
// --enable-teuchos
// --enable-ml --with-ml_nox
// --enable-aztecoo
// --enable-amesos
// due to cross-dependencies with nox, it is necessary to 
// configure Trilinos and make install WITHOUT the --with-ml_nox option, then 
// configure Trilinos and make install WITH the --with-ml_nox option again.
// The example should now work, if not, contact Michael Gee mwgee@sandia.gov.
//
// usage:
// ml_nox_1Delasticity_example.exe <number_of_elements>
// Try a large number (otherwise You don't get several levels), let's say
// at least 50000

// ml objects
#include "ml_common.h"

#if defined(HAVE_ML_NOX) && defined(HAVE_ML_EPETRA) && defined(HAVE_ML_AZTECOO) && defined(HAVE_ML_TEUCHOS) && defined(HAVE_ML_IFPACK) && defined(HAVE_ML_AMESOS) && defined(HAVE_ML_EPETRAEXT)
// ml objects
#include "nlnml_preconditioner.H"

// NOX Objects
#include "NOX.H"
#include "NOX_Epetra.H"

// Trilinos Objects
#ifdef ML_MPI
#include "Epetra_MpiComm.h"
#else
#include "Epetra_SerialComm.h"
#endif
#include "Epetra_Map.h"
#include "Epetra_Vector.h"
#include "Epetra_RowMatrix.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_Map.h"
#include "Epetra_LinearProblem.h"
#include "AztecOO.h"

// User's application specific files 
#include "Problem_Interface.H"    // Interface to application
#include "FiniteElementProblem.H" // the application            

using namespace std;

int main(int argc, char *argv[])
{
  int ierr = 0;

  // Initialize MPI
#ifdef HAVE_MPI
  MPI_Init(&argc,&argv);
#endif

  // Create a communicator for Epetra objects
#ifdef HAVE_MPI
  Epetra_MpiComm Comm( MPI_COMM_WORLD );
#else
  Epetra_SerialComm Comm;
#endif

  // Get the process ID and the total number of processors
  int MyPID = Comm.MyPID();
  int NumProc = Comm.NumProc();

  // Get the number of elements from the command line
  if (argc!=2) { 
    cout << "Usage: " << argv[0] << " number_of_elements" << endl;
    exit(1);
  }
  int NumGlobalElements = atoi(argv[1]) + 1;

  // The number of unknowns must be at least equal to the 
  // number of processors.
  if (NumGlobalElements < NumProc) {
    cout << "numGlobalBlocks = " << NumGlobalElements 
	 << " cannot be < number of processors = " << NumProc << endl;
    exit(1);
  }

  // Create the FiniteElementProblem class.  This creates all required
  // Epetra objects for the problem and allows calls to the 
  // function (RHS) and Jacobian evaluation routines.
  FiniteElementProblem nlnproblem(NumGlobalElements, Comm);

  // Get the vector from the Problem
  Epetra_Vector& soln = nlnproblem.getSolution();

  // Initialize Solution
  soln.PutScalar(1.0);
  
  // evaluate the nonlinear function once to be safe
  {
     Epetra_Vector* rhs = new Epetra_Vector(Copy,soln,0);
     nlnproblem.evaluate(ALL,&soln,rhs,NULL);
     delete rhs; rhs = 0;
  }
  
  
  // Create the interface between the test problem and the nonlinear solver
  // This is created by the user using inheritance of the abstract base class:
  // Ml_Nox_Fineinterface (which inherits from NOX::EpetraNew:: - interfaces)
  Problem_Interface fineinterface(nlnproblem);

  // Begin Nonlinear Solver ************************************
   // Create the top level nox parameter list
   NOX::Parameter::List nlParams;

   // Set the printing parameters in the "Printing" sublist
   NOX::Parameter::List& printParams = nlParams.sublist("Printing");
   printParams.setParameter("MyPID", MyPID); 
   printParams.setParameter("Output Precision", 9);
   printParams.setParameter("Output Processor", 0);
   printParams.setParameter("Output Information", 
  			    NOX::Utils::OuterIteration + 
			    NOX::Utils::Warning
                            );

   //----------------- Newton------------------------------
#if 0
   bool isnlnCG = false;
   // Set the nonlinear solver method
   nlParams.setParameter("Nonlinear Solver", "Line Search Based");
   
   // Sublist for line search 
   NOX::Parameter::List& searchParams = nlParams.sublist("Line Search");
   searchParams.setParameter("Method", "Full Step");
   
   // Sublist for direction
   NOX::Parameter::List& dirParams = nlParams.sublist("Direction");
   dirParams.setParameter("Method", "Newton");
   NOX::Parameter::List& newtonParams = dirParams.sublist("Newton");
   newtonParams.setParameter("Forcing Term Method", "Constant");
   //newtonParams.setParameter("Forcing Term Method", "Type 1");
   //newtonParams.setParameter("Forcing Term Method", "Type 2");
   newtonParams.setParameter("Forcing Term Minimum Tolerance", 1.0e-6);
   newtonParams.setParameter("Forcing Term Maximum Tolerance", 0.1);
   
   NOX::Parameter::List& lsParams = newtonParams.sublist("Linear Solver");
   lsParams.setParameter("Aztec Solver", "CG"); 
   lsParams.setParameter("Max Iterations", 100);  
   lsParams.setParameter("Tolerance", 1e-7);
   lsParams.setParameter("Output Frequency", 50);   

   //lsParams.setParameter("Preconditioning", "AztecOO: Jacobian Matrix");
   lsParams.setParameter("Preconditioning", "User Supplied Preconditioner");
   lsParams.setParameter("Preconditioner","User Defined");

   lsParams.setParameter("Aztec Preconditioner", "ilu");
   lsParams.setParameter("Graph Fill", 2);
   lsParams.setParameter("Fill Factor", 1);
   //-----------------------------------------------------------



   //-----------------nonlinearCG------------------------------
#else
   bool isnlnCG = true;
   // Set the nonlinear solver method as line search
   nlParams.setParameter("Nonlinear Solver", "Line Search Based");

   // get sublist for type of linesearch
   NOX::Parameter::List& searchParams = nlParams.sublist("Line Search");
   
   // set the nonlinearCG method
   searchParams.setParameter("Method", "NonlinearCG");

   // Sublist for direction
   NOX::Parameter::List& dirParams = nlParams.sublist("Direction");
   dirParams.setParameter("Method", "NonlinearCG");
   
   // sublist for nlnCG params
   NOX::Parameter::List& nlcgParams = dirParams.sublist("Nonlinear CG");
   nlcgParams.setParameter("Restart Frequency", 500);
   //nlcgParams.setParameter("Precondition", "Off");
   nlcgParams.setParameter("Precondition", "On");
   nlcgParams.setParameter("Orthogonalize", "Polak-Ribiere");
   //nlcgParams.setParameter("Orthogonalize", "Fletcher-Reeves");
   nlcgParams.setParameter("Restart Frequency", 25);
   
   NOX::Parameter::List& lsParams = nlcgParams.sublist("Linear Solver");
   lsParams.setParameter("Aztec Solver", "CG"); 
   lsParams.setParameter("Max Iterations", 1);  
   lsParams.setParameter("Tolerance", 1e-11);
   lsParams.setParameter("Output Frequency", 50);   
   //lsParams.setParameter("Preconditioning", "None");
   lsParams.setParameter("Preconditioning", "User Supplied Preconditioner");
   
   // EpetraNew takes this as user supplied preconditioner
   lsParams.setParameter("Preconditioner","User Defined");
   
   //lsParams.setParameter("Preconditioning", "AztecOO: Jacobian Matrix");
   lsParams.setParameter("Aztec Preconditioner", "ilu");
   lsParams.setParameter("Graph Fill", 0);
   lsParams.setParameter("Fill Factor", 1);
#endif

  // End Nonlinear Solver ************************************


  // Begin Preconditioner ************************************
   Teuchos::ParameterList mlparams;
   mlparams.set("nlnML output",                                      6          ); // ML-output-level (0-10)
   mlparams.set("nlnML max levels",                                  10         ); // max. # levels (minimum = 2 !)
   mlparams.set("nlnML coarse: max size",                            500        ); // the size ML stops generating coarser levels
   mlparams.set("nlnML is linear preconditioner",                    false      );
   mlparams.set("nlnML is matrixfree",                               false      ); 
   mlparams.set("nlnML finite difference fine level",                false      );
   mlparams.set("nlnML finite difference alpha",                     1.0e-08    );    
   mlparams.set("nlnML finite difference beta",                      1.0e-07    );    
   mlparams.set("nlnML finite difference centered",                  false      );     

   mlparams.set("nlnML absolute residual tolerance",                 1.0e-06    );
   mlparams.set("nlnML max cycles",                                  500        );
   mlparams.set("nlnML adaptive recompute",                          0.0        ); // recompute if residual is larger then this value
   mlparams.set("nlnML offset recompute",                            0          ); // every offset this preconditioner is recomputed     
   mlparams.set("nlnML additional adaptive nullspace",               0          ); // compute adaptive nullspace (additional kernel vectors)
   mlparams.set("nlnML PDE equations",                               1          ); // dof per node
   mlparams.set("nlnML null space: dimension",                       1          ); // dimension of nullspace
   mlparams.set("nlnML spatial dimension",                           1          );
   mlparams.set("nlnML coarse: type",                                "Uncoupled"); // Uncoupled METIS VBMETIS
   mlparams.set("nlnML nodes per aggregate",                         3          ); // # nodes per agg for coarsening METIS and VBMETIS

   mlparams.set("nlnML use nlncg on fine level",                     true); // use nlnCG or mod. Newton's method   
   mlparams.set("nlnML use nlncg on medium level",                   true);    
   mlparams.set("nlnML use nlncg on coarsest level",                 true);    
   
   mlparams.set("nlnML max iterations newton-krylov fine level",     5); // # iterations of lin. CG in mod. Newton's method    
   mlparams.set("nlnML max iterations newton-krylov medium level" ,  5);    
   mlparams.set("nlnML max iterations newton-krylov coarsest level", 5);    

   mlparams.set("nlnML linear smoother type fine level",             "MLS"); // SGS BSGS Jacobi MLS Bcheby AmesosKLU   
   mlparams.set("nlnML linear smoother type medium level",           "SGS"); 
   mlparams.set("nlnML linear smoother type coarsest level",         "AmesosKLU"); 
   mlparams.set("nlnML linear smoother sweeps fine level",           3);
   mlparams.set("nlnML linear smoother sweeps medium level",         1);
   mlparams.set("nlnML linear smoother sweeps coarsest level",       1);

   mlparams.set("nlnML nonlinear presmoothing sweeps fine level",    1);
   mlparams.set("nlnML nonlinear presmoothing sweeps medium level",  0);
   mlparams.set("nlnML nonlinear smoothing sweeps coarse level",     6);
   mlparams.set("nlnML nonlinear postsmoothing sweeps medium level", 1);
   mlparams.set("nlnML nonlinear postsmoothing sweeps fine level",   2);

   NLNML::NLNML_FineLevelNoxInterface& interface = 
     dynamic_cast<NLNML::NLNML_FineLevelNoxInterface&>(fineinterface);
   
   NLNML::NLNML_Preconditioner Prec(interface,mlparams,Comm);
   

  // End Preconditioner **************************************
#if 0


  // run the preconditioner as a solver **********************
#if 0
   // the preconditioner can also act as a multigrid solver (without outer Krylov method)
   if (islinearPrec==false)
   {
      double t0 = GetClock();
      int notconverged = Prec.solve();
      double t1 = GetClock();
      if (ml_printlevel>0 && Comm.MyPID()==0)
         cout << "NOX/ML :============solve time incl. setup : " << (t1-t0) << " sec\n";
      double appltime = fineinterface.getsumtime();
      if (ml_printlevel>0 && Comm.MyPID()==0)
      {
         cout << "NOX/ML :===========of which time in ccarat : " << appltime << " sec\n";
         cout << "NOX/ML :======number calls to computeF in this solve : " << fineinterface.getnumcallscomputeF() << "\n\n\n";
      }
      fineinterface.resetsumtime();
      fineinterface.setnumcallscomputeF(0);
      return notconverged;
   }
#endif
  // End run the preconditioner as a solver *******************



   // for nlnCG:
   NOX::EpetraNew::MatrixFree*                B            = 0;
   Epetra_CrsMatrix*                          A            = 0;
   ML_NOX::Ml_Nox_LinearSystem*               linSys       = 0;
   NOX::EpetraNew::Interface::Preconditioner* iPrec        = 0;
   NOX::EpetraNew::Interface::Jacobian*       iJac         = 0;
   NOX::EpetraNew::Interface::Required*       iReq         = 0;

   // for Newton:
   NOX::EpetraNew::LinearSystemAztecOO*       azlinSys     = 0;
   Epetra_Vector*                             clone        = 0;

   if (isnlnCG)
   {
     B = new NOX::EpetraNew::MatrixFree(fineinterface, soln, false);
     iPrec  = &Prec;
     iJac   = B;
     iReq   = &fineinterface;
     bool matrixfree    = mlparams.get("nlnML is matrixfree",false);
     int  ml_printlevel = mlparams.get("nlnML output",6);
     linSys = new ML_NOX::Ml_Nox_LinearSystem(*iJac,*B,*iPrec,NULL,Prec,soln,matrixfree,0,ml_printlevel);
   }
   else
   {
     A        = fineinterface.getJacobian();
     iPrec    = &Prec;
     iJac     = &fineinterface;
     iReq     = &fineinterface;
     clone    = new Epetra_Vector(soln); 
     
     azlinSys = new NOX::EpetraNew::LinearSystemAztecOO(printParams,lsParams,
                                                        *iJac,*A,*iPrec,
                                                        Prec,*clone);
   }


   // creat initial guess
   NOX::Epetra::Vector initialGuess(soln, NOX::DeepCopy, true);
   
   // Create the Group
   NOX::EpetraNew::Group* grp = 0;
   if (isnlnCG)
      grp = new NOX::EpetraNew::Group(printParams, *iReq, initialGuess, *linSys); 
   else 
      grp = new NOX::EpetraNew::Group(printParams, *iReq, initialGuess, *azlinSys); 

   // Create the convergence tests
   double FAS_normF = mlparams.get("nlnML absolute residual tolerance",1.0e-06);
   NOX::StatusTest::NormF absresid(FAS_normF);
   NOX::StatusTest::NormUpdate nupdate(FAS_normF);
   NOX::StatusTest::Combo converged(NOX::StatusTest::Combo::AND);
   converged.addStatusTest(absresid);
   converged.addStatusTest(nupdate);
   
   NOX::StatusTest::FiniteValue fv;
   NOX::StatusTest::MaxIters maxiters(30000);
   NOX::StatusTest::Combo combo(NOX::StatusTest::Combo::OR);
   combo.addStatusTest(maxiters);
   combo.addStatusTest(converged);
   combo.addStatusTest(fv);

   // Create the method
   NOX::Solver::Manager solver(*grp, combo, nlParams);
   
   // register the solver class in the preconditioner in case of nonlinear preconditioning
   Prec.set_nox_solver(&solver);


   // solve
   double t0 = GetClock();
   NOX::StatusTest::StatusType status = solver.solve();
   double t1 = GetClock();
   int  ml_printlevel = mlparams.get("nlnML output",6);
   if (ml_printlevel>0 && Comm.MyPID()==0)
      cout << "NOX/ML :============solve time incl. setup : " << (t1-t0) << " sec\n";
   double appltime = fineinterface.getsumtime();
   if (ml_printlevel>0 && Comm.MyPID()==0)
   {
      cout << "NOX/ML :===========of which time in application : " << appltime << " sec\n";
      cout << "NOX/ML :======number calls to computeF in this solve : " 
           << fineinterface.getnumcallscomputeF() << "\n\n\n";
   }
   fineinterface.resetsumtime();
   fineinterface.setnumcallscomputeF(0);
   
   if (status != NOX::StatusTest::Converged) 
   {
      if (Comm.MyPID()==0)
         cout << "***WRN***: ML/NOX not converged!";
      return(1);
   }
   else
      return(0);

#ifdef HAVE_MPI
  MPI_Finalize() ;
#endif

/* end main
*/
#endif
return ierr ;
}




#else  
// ml objects
#include <iostream>
#include "ml_common.h"
int main(int argc, char *argv[])
{
  std::cout << "running ml_nox_1Delasticity_example.exe needs: \n"
       << "defined(HAVE_ML_NOX) && defined(HAVE_ML_EPETRA) && defined(HAVE_ML_AZTECOO) && defined(HAVE_ML_TEUCHOS) && defined(HAVE_ML_IFPACK) && defined(HAVE_ML_AMESOS) && defined(HAVE_ML_EPETRAEXT)\n"
       << "see documentation for the ml-class ML_NOX::ML_Nox_Preconditioner\n";
  fflush(stdout);
  return 0;
}
#endif 
