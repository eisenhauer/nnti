/*@HEADER
// ***********************************************************************
// 
//        AztecOO: An Object-Oriented Aztec Linear Solver Package 
//                 Copyright (2002) Sandia Corporation
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
//@HEADER
*/

#include "test_single_aztecoo_thyra_solver.hpp"
#include "Teuchos_CommandLineProcessor.hpp"
#include "Teuchos_ParameterList.hpp"
#include "az_aztec_defs.h"

struct MatrixTestPacket {
  MatrixTestPacket(
    std::string  _matrixFile
    ,double      _maxFwdError
    ,int         _maxIters
    ,double      _maxResid
    ,double      _maxSolutionError
    ,double      _maxSlackErrorFrac
    ,int         _maxPrecIters
    ,double      _maxPrecResid
    ,double      _maxPrecSolutionError
    ,double      _maxPrecSlackErrorFrac
    )
    :matrixFile(_matrixFile)
    ,maxFwdError(_maxFwdError)
    ,maxIters(_maxIters)
    ,maxResid(_maxResid)
    ,maxSolutionError(_maxSolutionError)
    ,maxSlackErrorFrac(_maxSlackErrorFrac)
    ,maxPrecIters(_maxPrecIters)
    ,maxPrecResid(_maxPrecResid)
    ,maxPrecSolutionError(_maxPrecSolutionError)
    ,maxPrecSlackErrorFrac(_maxPrecSlackErrorFrac)
    {}
  std::string  matrixFile;
  double       maxFwdError;
  int          maxIters;
  double       maxResid;
  double       maxSolutionError;
  double       maxSlackErrorFrac;
  int          maxPrecIters;
  double       maxPrecResid;
  double       maxPrecSolutionError;
  double       maxPrecSlackErrorFrac;
};

int main(int argc, char* argv[])
{
  
  using Teuchos::CommandLineProcessor;

  bool result, success = true;
  bool verbose = true;

  std::ostream &out = std::cout;

	try {

    //
    // Read options from command-line
    //
    
    std::string    matrixDir              = "";
    int            numRandomVectors       = 1;
    bool           showAllTests           = false;
    bool           showAllTestsDetails    = false;
    bool           dumpAll                = false;
    std::string    aztecOutputLevel       = "freq";
    int            aztecOutputFreq        = 0;

    CommandLineProcessor  clp(false); // Don't throw exceptions
    clp.setOption( "matrix-dir", &matrixDir, "Base directory for the test matrices" );
    clp.setOption( "num-random-vectors", &numRandomVectors, "Number of times a test is performed with different random vectors." );
    clp.setOption( "verbose", "quiet", &verbose, "Set if output is printed or not." );
    clp.setOption( "show-all-tests", "no-show-all-tests", &showAllTests, "Set if all the tests are shown or not." );
    clp.setOption( "show-all-tests-details", "no-show-all-tests-details", &showAllTestsDetails, "Set if all the details of the tests are shown or not." );
    clp.setOption( "dump-all", "no-dump-all", &dumpAll, "Determines if vectors are printed or not." );
    clp.setOption( "aztec-output-level", &aztecOutputLevel, "Aztec output level (freq,last,summary,warnings,all)" );
    clp.setOption( "aztec-output-freq", &aztecOutputFreq, "Aztec output freqency (> 0)" );
    CommandLineProcessor::EParseCommandLineReturn parse_return = clp.parse(argc,argv);
    if( parse_return != CommandLineProcessor::PARSE_SUCCESSFUL ) return parse_return;

    TEST_FOR_EXCEPT( matrixDir == "" );

    Teuchos::ParameterList fwdSolveParamList, adjSolveParamList;
    if( aztecOutputLevel != "freq" ) {
      fwdSolveParamList.set("AZ_output",aztecOutputLevel);
      adjSolveParamList.set("AZ_output",aztecOutputLevel);
    }
    else {
      fwdSolveParamList.set("AZ_output",aztecOutputFreq);
      adjSolveParamList.set("AZ_output",aztecOutputFreq);
    }

    //
    // Define the test matrices
    //

    const int numTestMatrices = 9;

    typedef MatrixTestPacket MTP;

    // Set up the matices and the tolerances.
    // Note, we may need to adjust these for bad platforms ...
    const MTP testMatrices[numTestMatrices] =
      {
        MTP("In_bcsstk01.mtx"       ,1e-12, 40 , 1e-4, 0.5,      1.0, 20 , 1e-10, 0.5,      1.0)
        ,MTP("In_bcsstk02.mtx"      ,1e-12, 40 , 1e-3, 0.5,      1.0, 2  , 1e-10, 0.5,      1.0)
        ,MTP("In_bcsstk04.mtx"      ,1e-12, 80 , 1e-4, 0.999990, 1.0, 40 , 1e-10, 0.999990, 1.0)
        ,MTP("In_Diagonal.mtx"      ,1e-12, 4  , 1e-6, 1e-14,    1.0, 2  , 1e-10, 1e-14,    1.0)
        ,MTP("In_FourByFour.mtx"    ,1e-12, 4  , 1e-6, 1e-14,    1.0, 2  , 1e-10, 1e-14,    1.0)
        ,MTP("In_KheadK.mtx"        ,1e-12, 8  , 1e-6, 1e-14,    1.0, 2  , 1e-10, 1e-14,    1.0)
        ,MTP("In_KheadSorted.mtx"   ,1e-12, 8  , 1e-6, 1e-14,    1.0, 2  , 1e-10, 1e-14,    1.0)
        ,MTP("In_nos1.mtx"          ,1e-12, 200, 1e-4, 0.6,      1.0, 237, 1e-2,  5.0,      1.0)
        ,MTP("In_nos5.mtx"          ,1e-12, 468, 1e-5, 0.5,      1.0, 468, 1e-10, 0.5,      1.0)
      };
    //
    // Loop through all of the test matrices
    //
    for( int matrix_i = 0; matrix_i < numTestMatrices; ++matrix_i ) {
      const MatrixTestPacket
        mtp = testMatrices[matrix_i];
      //
      // Do unpreconditioned and preconditioned solves
      //
      for( int prec_i = 0; prec_i < 2; ++prec_i ) {
        if(verbose)
          out << std::endl<<matrix_i<<":"<<prec_i<<": Testing, matrixFile=\'"<<mtp.matrixFile<<"\', ";
        bool testTranspose;
        int    maxIters;
        double maxResid;
        double maxSolutionError;
        double maxSlackErrorFrac;
        if(prec_i==0) {
          out << "no aztec preconditioning ... ";
          fwdSolveParamList.set("AZ_precond","none");
          testTranspose = true;
          maxIters = mtp.maxIters;
          maxResid = mtp.maxResid;
          maxSolutionError = mtp.maxSolutionError;
          maxSlackErrorFrac = mtp.maxSlackErrorFrac;
        }
        else {
          out << "using aztec preconditioning ... ";
          fwdSolveParamList.set("AZ_precond","dom_decomp");
          fwdSolveParamList.set("AZ_subdomain_solve","ilu");
          testTranspose = false;
          maxIters = mtp.maxPrecIters;
          maxResid = mtp.maxPrecResid;
          maxSolutionError = mtp.maxPrecSolutionError;
          maxSlackErrorFrac = mtp.maxPrecSlackErrorFrac;
        }
        std::ostringstream oss;
        result =
          Thyra::test_single_aztecoo_thyra_solver(
            matrixDir+"/"+mtp.matrixFile,testTranspose,numRandomVectors
            ,mtp.maxFwdError,maxIters,maxResid,maxSolutionError
            ,showAllTestsDetails,dumpAll,&fwdSolveParamList,&adjSolveParamList,&oss
            );
        if(!result) success = false;
        if(verbose) {
          if(result) {
            if(showAllTests)
              out << std::endl << oss.str();
            else
              out << " : passed!\n";
          }
          else {
            if(showAllTests)
              out << std::endl << oss.str();
            else
              out << " : failed!\n";
          }
        }
      }
    }

	}
	catch( const std::exception &excpt ) {
		std::cerr << "*** Caught standard exception : " << excpt.what() << std::endl;
		success = false;
	}
	catch( ... ) {
		std::cerr << "*** Caught an unknown exception\n";
		success = false;
	}
	
	if (verbose) {
		if(success)  out << "\nCongratulations! All of the tests checked out!\n";
		else         out << "\nOh no! At least one of the tests failed!\n";
	}

  return ( success ? 0 : 1 );
}
