// @HEADER
// @HEADER

#include "Thyra_NonlinearSolver_NOX.hpp"

#include "NOX.H"
#include "NOX_Thyra.H"

// ****************************************************************
// ****************************************************************
Thyra::NOXNonlinearSolver::NOXNonlinearSolver()
{
  param_list_ = Teuchos::rcp(new Teuchos::ParameterList);
  valid_param_list_ = Teuchos::rcp(new Teuchos::ParameterList);
}

// ****************************************************************
// ****************************************************************
Thyra::NOXNonlinearSolver::~NOXNonlinearSolver()
{
  
}

// ****************************************************************
// ****************************************************************
void Thyra::NOXNonlinearSolver::
setParameterList(Teuchos::RCP<Teuchos::ParameterList> const& p)
{
  TEST_FOR_EXCEPT(Teuchos::is_null(p));
  param_list_ = p;
  this->resetSolver();
}

// ****************************************************************
// ****************************************************************
Teuchos::RCP<Teuchos::ParameterList> 
Thyra::NOXNonlinearSolver::getParameterList()
{
  return param_list_;
}

// ****************************************************************
// ****************************************************************
Teuchos::RCP<Teuchos::ParameterList>
Thyra::NOXNonlinearSolver::unsetParameterList()
{
  Teuchos::RCP<Teuchos::ParameterList> _paramList = param_list_;
  param_list_ = Teuchos::null;
  return _paramList;
}

// ****************************************************************
// ****************************************************************
Teuchos::RCP<const Teuchos::ParameterList>
Thyra::NOXNonlinearSolver::getParameterList() const
{
  return param_list_;
}

// ****************************************************************
// ****************************************************************
Teuchos::RCP<const Teuchos::ParameterList>
Thyra::NOXNonlinearSolver::getValidParameters() const
{
  return valid_param_list_;
}

// ****************************************************************
// ****************************************************************
void Thyra::NOXNonlinearSolver::
setModel(const Teuchos::RCP<const ModelEvaluator<double> >& model)
{
  TEST_FOR_EXCEPT(model.get()==NULL);
  model_ = model;
  J_ = Teuchos::null;
}

// ****************************************************************
// ****************************************************************
Teuchos::RCP< const Thyra::ModelEvaluator<double> >
Thyra::NOXNonlinearSolver::getModel() const
{
  return model_;
}

// ****************************************************************
// ****************************************************************
Thyra::SolveStatus<double> Thyra::NOXNonlinearSolver::
solve(VectorBase<double> *x,
      const SolveCriteria<double> *solveCriteria,
      VectorBase<double> *delta)
{
  TEST_FOR_EXCEPT(model_.get()==NULL);
  TEST_FOR_EXCEPT(param_list_.get()==NULL);
 
  NOX::Thyra::Vector initial_guess(Teuchos::rcp(x, false));  // View of x

  if (Teuchos::is_null(nox_group_)) {
    nox_group_ = Teuchos::rcp(new NOX::Thyra::Group(initial_guess, model_));
  }
  else
    nox_group_->setX(initial_guess);

  if (Teuchos::is_null(status_test_))
    status_test_ = this->buildStatusTests(*param_list_);

  if (Teuchos::is_null(solver_))
    solver_ = Teuchos::rcp(new NOX::Solver::Manager(nox_group_, status_test_, 
						    param_list_));
  else
    solver_->reset(nox_group_);

  NOX::StatusTest::StatusType solvStatus = solver_->solve();
  
  Thyra::SolveStatus<double> t_status;

  if (solvStatus == NOX::StatusTest::Converged)
    t_status.solveStatus = SOLVE_STATUS_CONVERGED;
  else if (solvStatus == NOX::StatusTest::Unconverged) 
    t_status.solveStatus = SOLVE_STATUS_UNCONVERGED;
  else if (solvStatus == NOX::StatusTest::Failed) 
    t_status.solveStatus = SOLVE_STATUS_UNCONVERGED;
  else
    t_status.solveStatus = SOLVE_STATUS_UNCONVERGED;

  // Get the solution and update
  const NOX::Abstract::Group& final_group = solver_->getSolutionGroup();
  const NOX::Abstract::Vector& final_solution = final_group.getX();
  
  const NOX::Thyra::Vector& vec = 
    dynamic_cast<const NOX::Thyra::Vector&>(final_solution);
  
  const ::Thyra::VectorBase<double>& new_x = 
    vec.getThyraVector();

  if (delta)
    Thyra::V_StVpStV<double>(delta,1.0,new_x,-1.0,*x);

  *x = new_x;

  return t_status;
}


Teuchos::RCP< Thyra::LinearOpWithSolveBase<double> >
Thyra::NOXNonlinearSolver::get_nonconst_W()
{
  return J_;
}


Teuchos::RCP<const Thyra::LinearOpWithSolveBase<double> >
Thyra::NOXNonlinearSolver::get_W() const
{
  return J_;
}

// ****************************************************************
// ****************************************************************
Teuchos::RCP<NOX::StatusTest::Generic> Thyra::NOXNonlinearSolver::
buildStatusTests(Teuchos::ParameterList& p)
{
  Teuchos::RCP<NOX::StatusTest::Generic> status_test;

  NOX::Utils utils(p.sublist("Printing"));

  if (p.isSublist("Status Tests")) {
    NOX::StatusTest::Factory stf;
    status_test = stf.buildStatusTests(p.sublist("Status Tests"), utils);
  }
  else { // Default status test
    Teuchos::RCP<NOX::StatusTest::NormF> absresid =
      Teuchos::rcp(new NOX::StatusTest::NormF(1.0e-8));
    Teuchos::RCP<NOX::StatusTest::NormWRMS> wrms =
      Teuchos::rcp(new NOX::StatusTest::NormWRMS(1.0e-2, 1.0e-8));
    Teuchos::RCP<NOX::StatusTest::Combo> converged =
      Teuchos::rcp(new NOX::StatusTest::Combo(NOX::StatusTest::Combo::AND));
    converged->addStatusTest(absresid);
    converged->addStatusTest(wrms);
    Teuchos::RCP<NOX::StatusTest::MaxIters> maxiters =
      Teuchos::rcp(new NOX::StatusTest::MaxIters(20));
    Teuchos::RCP<NOX::StatusTest::FiniteValue> fv =
      Teuchos::rcp(new NOX::StatusTest::FiniteValue);
    Teuchos::RCP<NOX::StatusTest::Combo> combo =
      Teuchos::rcp(new NOX::StatusTest::Combo(NOX::StatusTest::Combo::OR));
    combo->addStatusTest(fv);
    combo->addStatusTest(converged);
    combo->addStatusTest(maxiters);

    status_test = combo;
  }

  return status_test;
}

// ****************************************************************
// ****************************************************************
void Thyra::NOXNonlinearSolver::resetSolver()
{
  nox_group_ = Teuchos::null;
  status_test_ = Teuchos::null;
  solver_ = Teuchos::null;
}

// ****************************************************************
// ****************************************************************
