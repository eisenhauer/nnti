// @HEADER
// ***********************************************************************
//
//		   Belos: Block Linear Solvers Package
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
//

#ifndef BELOS_STATUS_TEST_SIMPLE_OUTPUT_HPP
#define BELOS_STATUS_TEST_SIMPLE_OUTPUT_HPP

/*!
  \file BelosStatusTestSimpleOutput.hpp
  \brief Special StatusTest for printing status tests in simple format for residuals.
*/

#include <vector>
#include "BelosConfigDefs.hpp"
#include "BelosTypes.hpp"
#include "BelosIteration.hpp"

#include "BelosStatusTest.hpp"
#include "BelosStatusTestCombo.hpp"
#include "BelosStatusTestMaxIters.hpp"
#include "BelosStatusTestResNorm.hpp"


namespace Belos {

  /*! 
    \class StatusTestSimpleOutput
    \brief A special StatusTest for printing other status tests in a simple format. 
    
    StatusTestSimpleOutput is a wrapper around an StatusTest that calls 
    StatusTest::print() on the underlying object on calls to StatusTestSimpleOutput::checkStatus().
    The frequency and occasion of the printing can be dictated according to some parameters passed to 
    StatusTestSimpleOutput::StatusTestSimpleOutput().
  */
template <class ScalarType, class MV, class OP>
class StatusTestSimpleOutput : public StatusTest<ScalarType,MV,OP> {

  typedef Belos::StatusTestCombo<ScalarType,MV,OP>  StatusTestCombo_t;
  typedef Belos::StatusTestResNorm<ScalarType,MV,OP>  StatusTestResNorm_t;

 public:
  //! @name Constructors/destructors
  //@{ 

  /*! \brief Constructor
   *
   * The StatusTestSimpleOutput requires an OutputManager for printing the underlying StatusTest on
   * calls to checkStatus(), as well as an underlying StatusTest.
   *
   * The last two parameters, described below, in addition to the verbosity level of the OutputManager, control when printing is 
   * called. When both the \c mod criterion and the \c printStates criterion are satisfied, the status test will be printed to the 
   * OutputManager with ::MsgType of ::StatusTestDetails.
   *
   * @param[in] iterTest A reference-counted pointer to a Belos::StatusTestMaxIters object
   * @param[in] resTest A reference-counted pointer to a object that derives from Belos::StatusTestResNorm or a combination of Belos::StatusTestResNorm objects
   * @param[in] mod A positive number describes how often the output should be printed. On every call to checkStatus(), an internal counter
   *                is incremented. Printing may only occur when this counter is congruent to zero modulo \c mod. Default: 1 (attempt to print on every call to checkStatus())
   * @param[in] printStates A combination of ::StatusType values for which the output may be printed. Default: ::Passed (attempt to print whenever checkStatus() will return ::Passed)
   *
   */
  StatusTestSimpleOutput(const Teuchos::RCP<OutputManager<ScalarType> > &printer, 
			Teuchos::RCP<StatusTestMaxIters<ScalarType,MV,OP> > iterTest,
			Teuchos::RCP<StatusTest<ScalarType,MV,OP> > resTest,
			int mod = 1,
			int printStates = Passed)
    : printer_(printer), 
      resTest_(resTest), 
      iterTest_(iterTest), 
      state_(Undefined), 
      headerPrinted_(false),
      stateTest_(printStates), 
      modTest_(mod), 
      lastNumIters_(-1),
      comboType_(0),
      numResTests_(0),
      blockSize_(1),
      currNumRHS_(0),
      currLSNum_(0)
    {
      // Create the status test combination of the iteration and residual test.
      test_ = Teuchos::rcp( new StatusTestCombo_t( StatusTestCombo_t::OR, iterTest_, resTest_ ) );

      // First check if the residual status test is a single test
      Teuchos::RCP<StatusTestResNorm_t> tmpResTest = Teuchos::rcp_dynamic_cast<StatusTestResNorm_t>(resTest);
      // If the residual status test is a single test, put in the vector
      if (tmpResTest != Teuchos::null) {
	resTestVec_.push_back( tmpResTest );
        numResTests_ = 1;
      } else {
        // Check if the residual test is a combination of several StatusTestResNorm objects.
        Teuchos::RCP<StatusTestCombo_t> tmpComboTest = Teuchos::rcp_dynamic_cast<StatusTestCombo_t>(resTest);
        TEST_FOR_EXCEPTION(tmpComboTest == Teuchos::null,StatusTestError,"StatusTestSimpleOutput():  resTest must be Belos::StatusTestResNorm or Belos::StatusTestCombo.");
        std::vector<Teuchos::RCP<StatusTest<ScalarType,MV,OP> > > tmpVec = tmpComboTest->getStatusTests();
        comboType_ = tmpComboTest->getComboType();
        numResTests_ = tmpVec.size();
        resTestVec_.resize( numResTests_ );
        for (int i=0; i<numResTests_; ++i) {
	  tmpResTest = Teuchos::rcp_dynamic_cast<StatusTestResNorm_t>(tmpVec[i]);
          TEST_FOR_EXCEPTION(tmpResTest == Teuchos::null,StatusTestError,"StatusTestSimpleOutput():  resTest must be a vector of Belos::StatusTestResNorm.");
          resTestVec_[i] = tmpResTest;
        }
      }
    }

  //! Destructor
  virtual ~StatusTestSimpleOutput() {};
  //@}

  //! @name Status methods
  //@{ 
  /*! Check and return status of underlying StatusTest.

    This method calls checkStatus() on the StatusTest object passed in the constructor. If appropriate, the
    method will follow this call with a call to print() on the underlying object, using the OutputManager passed via the constructor
    with verbosity level ::StatusTestDetails.

    The internal counter will be incremented during this call, but only after
    performing the tests to decide whether or not to print the underlying
    StatusTest. This way, the very first call to checkStatus() following
    initialization or reset() will enable the underlying StatusTest to be
    printed, regardless of the mod parameter, as the current number of calls
    will be zero.

    If the specified Teuchos::RCP for the child class is Teuchos::null, then calling checkStatus() will result in a StatusTestError std::exception being thrown.
    
    \return ::StatusType indicating whether the underlying test passed or failed.
  */
  StatusType checkStatus( Iteration<ScalarType,MV,OP>* solver ) 
  {
    TEST_FOR_EXCEPTION(iterTest_ == Teuchos::null,StatusTestError,"StatusTestSimpleOutput::checkStatus():  iteration test pointer is null.");
    TEST_FOR_EXCEPTION(resTest_ == Teuchos::null,StatusTestError,"StatusTestSimpleOutput::checkStatus():  residual test pointer is null.");
    state_ = test_->checkStatus(solver);

    // Update some information for the header, if it has not printed or the linear system has changed.
    LinearProblem<ScalarType,MV,OP> currProb = solver->getProblem();
    if (!headerPrinted_ || currLSNum_ != currProb.getLSNumber()) {
      currLSNum_ = currProb.getLSNumber();
      blockSize_ = solver->getBlockSize();
      currIdx_ = currProb.getLSIndex();
      currNumRHS_ = currIdx_.size();
    }
    // Print out current iteration information if it hasn't already been printed, or the status has changed
    if (((iterTest_->getNumIters() % modTest_ == 0) && (iterTest_->getNumIters()!=lastNumIters_)) || (state_ == Passed)) {
      lastNumIters_ = iterTest_->getNumIters();
      if ( (state_ & stateTest_) == state_) {
        if ( printer_->isVerbosity(StatusTestDetails) ) {
          print( printer_->stream(StatusTestDetails) );
        }
        else if ( printer_->isVerbosity(Debug) ) {
          print( printer_->stream(Debug) );
        }
      }
    }

    return state_;
  }

  //! Return the result of the most recent checkStatus call, or undefined if it has not been run.
  StatusType getStatus() const {
    return state_;
  }
  //@}


  //! @name Accessor methods
  //@{ 

  /*! \brief Set the output manager.
   */ 
  void setOutputManager(const Teuchos::RCP<OutputManager<ScalarType> > &printer) { printer_ = printer; }

  /*! \brief Set how often the child test is printed.
   */
  void setOutputFrequency(int mod) { modTest_ = mod; }

  /*! \brief Set a short solver description for output clarity.
   */
  void setSolverDesc(const std::string& solverDesc) { solverDesc_ = solverDesc; }

  /*! \brief Set a short preconditioner description for output clarity.
   */
  void setPrecondDesc(const std::string& precondDesc) { precondDesc_ = precondDesc; }
  //@}


  //! @name Reset methods
  //@{ 
  /*! \brief Informs the status test that it should reset its internal configuration to the uninitialized state.
   *
   *  This resets the cached state to an ::Undefined state and calls reset() on the underlying test. It also 
   *  resets the counter for the number of calls to checkStatus().
   */
  void reset() { 
    state_ = Undefined;
    test_->reset();
    lastNumIters_ = -1;
    headerPrinted_ = false;
  }

  //! Clears the results of the last status test.
  //! This resets the cached state to an ::Undefined state and calls clearStatus() on the underlying test.
  void clearStatus() {
    state_ = Undefined;
    test_->clearStatus();
    headerPrinted_ = false;
  }

  //@}

  //! @name Print methods
  //@{ 
  
  //! Output formatted description of stopping test to output stream.
  void print(std::ostream& os, int indent = 0) const {
    std::string ind(indent,' ');
    std::string starLine(60,'*');
    std::string starFront(5,'*');

    os.setf(std::ios::scientific, std::ios::floatfield);
    os.precision(6);
    
    // Print header if this is the first call to this output status test.
    if (!headerPrinted_) {
      os << std::endl << ind << starLine << std::endl;
      os << ind << starFront << " Belos Iterative Solver: " << solverDesc_ << std::endl;
      if (precondDesc_ != "")
        os << ind << starFront << " Preconditioner: " << precondDesc_ << std::endl;
      os << ind << starFront << " Maximum Iterations: " << iterTest_->getMaxIters() << std::endl;
      os << ind << starFront << " Block Size: " << blockSize_ << std::endl;
      if (numResTests_ > 1) {
        os << ind << starFront << " Residual Tests (" 
           << ((comboType_ == StatusTestCombo_t::OR) ? "OR" : (comboType_ == StatusTestCombo_t::AND) ? "AND" :"SEQ")
           << "): " << std::endl;
      } else {
        os << ind << starFront << " Residual Test: " << std::endl;
      } 
      for (int i=0; i<numResTests_; ++i) {
        os << ind << starFront << "   Test " << i+1 << " : " << resTestVec_[i]->description() << std::endl;
      }
      os << ind << starLine << std::endl;
      headerPrinted_ = true;
    }

    // Print out residuals for each residual test.
    os.setf(std::ios_base::right, std::ios_base::adjustfield);
    std::string ind2(12,' ');
    os << ind << "Iter " << std::setw(5) << iterTest_->getNumIters() << " :";
    for (int i=0; i<currNumRHS_; ++i) {
      if ( i > 0 ) {
        // Put in space where 'Iter :' is in the previous lines
        os << ind << ind2;
      }
      for (int j=0; j<numResTests_; ++j) {
        if ( resTestVec_[j]->getStatus() != Undefined ) {
          os << std::setw(15) << (*resTestVec_[j]->getTestValue())[currIdx_[i]];
        } else {
          os << std::setw(15) << "---"; 
        }
      }
      os << std::endl;
    }
  }
 
  //@}

  private:
    // Output manager.
    Teuchos::RCP<OutputManager<ScalarType> > printer_;

    // Overall status test.
    Teuchos::RCP<StatusTest<ScalarType,MV,OP> > test_;

    // Residual test (as passed in).
    Teuchos::RCP<StatusTest<ScalarType,MV,OP> > resTest_;

    // Iteration test (as passed in).
    Teuchos::RCP<StatusTestMaxIters<ScalarType,MV,OP> > iterTest_;

    //! Vector of residual status tests
    std::vector<Teuchos::RCP<StatusTestResNorm<ScalarType,MV,OP> > > resTestVec_;

    std::string solverDesc_;
    std::string precondDesc_;
    std::vector<int> currIdx_;
    StatusType state_;
    mutable bool headerPrinted_;
    int stateTest_, modTest_;
    int lastNumIters_, comboType_;
    int numResTests_, blockSize_;
    int currNumRHS_, currLSNum_;
};

} // end of Belos namespace

#endif /* BELOS_STATUS_TEST_OUTPUT_HPP */
