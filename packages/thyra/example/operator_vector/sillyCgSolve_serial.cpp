// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
//                 Copyright (2004) Sandia Corporation
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
// @HEADER

#include "ExampleTridiagSerialLinearOp.hpp"
#include "sillyCgSolve.hpp"
#include "sillierCgSolve.hpp"
#include "Thyra_DefaultScaledAdjointLinearOp.hpp"
#include "Thyra_DefaultMultipliedLinearOp.hpp"
#include "Thyra_VectorStdOps.hpp"
#include "Thyra_TestingTools.hpp"
#include "Thyra_LinearOpTester.hpp"
#include "Teuchos_CommandLineProcessor.hpp"
#include "Teuchos_VerboseObject.hpp"
#include "Teuchos_Time.hpp"
#include "Teuchos_StandardCatchMacros.hpp"

//
// This example program is meant to show how easy it is to create
// serial Thyra objects and use them with an ANA (CG in this case).
//
// This example uses a silly concrete tridiagonal matrix class called
// ExampleSpmdTridiagLinearOp that demonstrates how to write and use such
// subclasses.
//
template<class Scalar>
bool runCgSolveExample(
  const int                                                      dim
  ,const Scalar                                                  diagScale
  ,const bool                                                    symOp
  ,const bool                                                    showAllTests
  ,const bool                                                    verbose
  ,const typename Teuchos::ScalarTraits<Scalar>::magnitudeType   tolerance
  ,const int                                                     maxNumIters
  ,const bool                                                    useSillierCg
  )
{
  using Teuchos::RCP; using Teuchos::rcp; using Teuchos::null;
  using Teuchos::OSTab;
  typedef Teuchos::ScalarTraits<Scalar> ST;
  using Thyra::multiply; using Thyra::scale;
  typedef typename ST::magnitudeType    ScalarMag;
  bool success = true;
  bool result;
  Teuchos::RCP<Teuchos::FancyOStream>
    out = (verbose ? Teuchos::VerboseObjectBase::getDefaultOStream() : Teuchos::null);
  if(verbose)
    *out << "\n***\n*** Running silly CG solver using scalar type = \'" << ST::name() << "\' ...\n***\n";
  Teuchos::Time timer("");
  timer.start(true);
  //
  // (A) Setup a simple linear system with tridiagonal operator:
  //
  //       [   a*2   -1                         ]
  //       [ -r(1)  a*2       -1                ]
  //  A =  [          .        .        .       ]
  //       [             -r(n-2)      a*2    -1 ]
  //       [                      -r(n-1)   a*2 ]
  //
  // (A.1) Create the tridiagonal matrix operator
  if(verbose) *out << "\nConstructing tridiagonal matrix A of dimension = " << dim << " and diagonal multiplier = " << diagScale << " ...\n";
  std::vector<Scalar> lower(dim-1), diag(dim), upper(dim-1);
  const Scalar up = -ST::one(), diagTerm = Scalar(2)*diagScale*ST::one(), low = -(symOp?ST::one():ST::random());
  int k = 0;
  diag[k] = diagTerm; upper[k] = up;                          // First row
  for( k = 1; k < dim - 1; ++k ) {
    lower[k-1] = low; diag[k] = diagTerm; upper[k] = up;      // Middle rows
  }
  lower[k-1] = low; diag[k] = diagTerm;                       // Last row
  RCP<const Thyra::LinearOpBase<Scalar> >
    A = rcp(new ExampleTridiagSerialLinearOp<Scalar>(dim,&lower[0],&diag[0],&upper[0]));
  // (A.2) Testing the linear operator constructed linear operator
  if(verbose) *out << "\nTesting the constructed linear operator A ...\n";
  Thyra::LinearOpTester<Scalar> linearOpTester;
  linearOpTester.enable_all_tests(false);            // DEBUG ONLY
  linearOpTester.check_linear_properties(true);      // DEBUG ONLY
  linearOpTester.set_all_error_tol(tolerance);
  linearOpTester.set_all_warning_tol(ScalarMag(ScalarMag(1e-2)*tolerance));
  linearOpTester.show_all_tests(showAllTests);
  result = linearOpTester.check(*A,out.get());
  if(!result) success = false;
  // (A.3) Create RHS vector b and set to a random value
  RCP<Thyra::VectorBase<Scalar> > b = createMember(A->range());
  Thyra::seed_randomize<Scalar>(0);
  Thyra::randomize( Scalar(-ST::one()), Scalar(+ST::one()), &*b );
  // (A.4) Create LHS vector x and set to zero
  RCP<Thyra::VectorBase<Scalar> > x = createMember(A->domain());
  Thyra::assign( &*x, ST::zero() );
  // (A.5) Create the final linear system
  if(!symOp) {
    if(verbose) *out << "\nSetting up normal equations for unsymmetric system A^H*(A*x-b) => new A*x = b ...\n";
    RCP<const Thyra::LinearOpBase<Scalar> > AtA = multiply(adjoint(A),A);      // A^H*A
    RCP<Thyra::VectorBase<Scalar> >         nb = createMember(AtA->range());   // A^H*b
    Thyra::apply(*A,Thyra::CONJTRANS,*b,&*nb);
    A = AtA;
    b = nb;
  }
  // (A.6) Testing the linear operator used with the solve
  if(verbose) *out << "\nTesting the linear operator used with the solve ...\n";
  linearOpTester.check_for_symmetry(true);
  result = linearOpTester.check(*A,out.get());
  if(!result) success = false;
  //
  // (B) Solve the linear system with the silly CG solver
  //
  if(verbose) *out << "\nSolving the linear system with sillyCgSolve(...) ...\n";
  if(useSillierCg)
    result = sillierCgSolve(*A,*b,maxNumIters,tolerance,&*x,OSTab(out).get());
  else
    result = sillyCgSolve(*A,*b,maxNumIters,tolerance,&*x,OSTab(out).get());
  if(!result) success = false;
  //
  // (C) Check that the linear system was solved to the specified tolerance
  //
  RCP<Thyra::VectorBase<Scalar> > r = createMember(A->range());                     
  Thyra::assign(&*r,*b);                                               // r = b
  Thyra::apply(*A,Thyra::NOTRANS,*x,&*r,Scalar(-ST::one()),ST::one()); // r = -A*x + r
  const ScalarMag r_nrm = Thyra::norm(*r), b_nrm = Thyra::norm(*b);
  const ScalarMag rel_err = r_nrm/b_nrm, relaxTol = ScalarMag(10.0)*tolerance;
  result = rel_err <= relaxTol;
  if(!result) success = false;
  if(verbose) {
    *out << "\nChecking the residual ourselves ...\n";
    {
      OSTab tab(out);
      *out
        << "\n||b-A*x||/||b|| = "<<r_nrm<<"/"<<b_nrm<<" = "<<rel_err<<(result?" <= ":" > ")
        <<"10.0*tolerance = "<<relaxTol<<": "<<(result?"passed":"failed")<<std::endl;
    }
  }
  timer.stop();
  if(verbose) *out << "\nTotal time = " << timer.totalElapsedTime() << " sec\n";

  return success;
} // end runCgSolveExample()


//
// Actual main driver program
//
int main(int argc, char *argv[])
{
  
  using Teuchos::CommandLineProcessor;

  bool success = true;
  bool verbose = true;
  bool result;

  Teuchos::RCP<Teuchos::FancyOStream>
    out = Teuchos::VerboseObjectBase::getDefaultOStream();

  try {

    //
    // Read in command-line options
    //

    int    dim          = 500;
    double diagScale    = 1.001;
    bool   symOp        = true;
    bool   showAllTests = false;
    double tolerance    = 1e-4;
    int    maxNumIters  = 300;
    bool   useSillierCg = false;

    CommandLineProcessor  clp;
    clp.throwExceptions(false);
    clp.addOutputSetupOptions(true);

    clp.setOption( "verbose", "quiet", &verbose, "Determines if any output is printed or not." );
    clp.setOption( "dim", &dim, "Dimension of the linear system." );
    clp.setOption( "diag-scale", &diagScale, "Scaling of the diagonal to improve conditioning." );
    clp.setOption( "sym-op", "unsym-op", &symOp, "Determines if the operator is symmetric or not." );
    clp.setOption( "show-all-tests", "show-summary-only", &showAllTests, "Show all LinearOpTester tests or not" );
    clp.setOption( "tol", &tolerance, "Relative tolerance for linear system solve." );
    clp.setOption( "max-num-iters", &maxNumIters, "Maximum of CG iterations." );
    clp.setOption( "use-sillier-cg", "use-silly-cg", &useSillierCg,
                   "Use the handle-based sillerCgSolve() function or the nonhandle-based sillyCgSolve() function");

    CommandLineProcessor::EParseCommandLineReturn parse_return = clp.parse(argc,argv);
    if( parse_return != CommandLineProcessor::PARSE_SUCCESSFUL ) return parse_return;

    TEST_FOR_EXCEPTION( dim < 2, std::logic_error, "Error, dim=" << dim << " < 2 is not allowed!" );

#if defined(HAVE_TEUCHOS_FLOAT)
    // Run using float
    result = runCgSolveExample<float>(dim,diagScale,symOp,showAllTests,verbose,tolerance,maxNumIters,useSillierCg);
    if(!result) success = false;
#endif

    // Run using double
    result = runCgSolveExample<double>(dim,diagScale,symOp,showAllTests,verbose,tolerance,maxNumIters,useSillierCg);
    if(!result) success = false;

#ifdef HAVE_TEUCHOS_COMPLEX

#if defined(HAVE_TEUCHOS_FLOAT)
    // Run using std::complex<float>
    result = runCgSolveExample<std::complex<float> >(dim,diagScale,symOp,showAllTests,verbose,tolerance,maxNumIters,useSillierCg);
    if(!result) success = false;
#endif

    // Run using std::complex<double>
    result = runCgSolveExample<std::complex<double> >(dim,diagScale,symOp,showAllTests,verbose,tolerance,maxNumIters,useSillierCg);
    if(!result) success = false;

#endif // HAVE_TEUCHOS_COMPLEX

#ifdef HAVE_TEUCHOS_GNU_MP

    // Run using mpf_class
    result = runCgSolveExample<mpf_class>(dim,diagScale,symOp,showAllTests,verbose,tolerance,maxNumIters,useSillierCg);
    if(!result) success = false;

#ifdef HAVE_TEUCHOS_COMPLEX

    // Run using std::complex<mpf_class>
    //result = runCgSolveExample<std::complex<mpf_class> >(dim,mpf_class(diagScale),symOp,showAllTests,verbose,mpf_class(tolerance),maxNumIters);
    //if(!result) success = false;
    //The above commented-out code throws a floating-point exception?

#endif // HAVE_TEUCHOS_COMPLEX

#endif // HAVE_TEUCHOS_GNU_MP

  }
  TEUCHOS_STANDARD_CATCH_STATEMENTS(true,*out,success)

  if (verbose) {
    if(success)   *out << "\nCongratulations! All of the tests checked out!\n";
    else          *out << "\nOh no! At least one of the tests failed!\n";
  }
  
  return success ? 0 : 1;

} // end main()
