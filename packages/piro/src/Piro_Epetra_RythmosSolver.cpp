// @HEADER
// ************************************************************************
//
//        Piro: Strategy package for embedded analysis capabilitites
//                  Copyright (2010) Sandia Corporation
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
// Questions? Contact Andy Salinger (agsalin@sandia.gov), Sandia
// National Laboratories.
//
// ************************************************************************
// @HEADER

#include "Piro_Epetra_RythmosSolver.hpp"

#include "Piro_Epetra_InvertMassMatrixDecorator.hpp"
#include "Piro_ValidPiroParameters.hpp"
#include "Piro_Epetra_MatrixFreeDecorator.hpp"

#include "Rythmos_IntegratorBuilder.hpp"
#include "Rythmos_BackwardEulerStepper.hpp"
#include "Rythmos_ExplicitRKStepper.hpp"
#include "Rythmos_SimpleIntegrationControlStrategy.hpp"
#include "Rythmos_ForwardSensitivityStepper.hpp"
#include "Rythmos_StepperAsModelEvaluator.hpp"

#include "Stratimikos_DefaultLinearSolverBuilder.hpp"

#include "Thyra_EpetraModelEvaluator.hpp"
#include "Thyra_EpetraThyraWrappers.hpp"

#include <string>
#include <iostream>

Piro::Epetra::RythmosSolver::RythmosSolver(Teuchos::RCP<Teuchos::ParameterList> piroParams,
                          Teuchos::RCP<EpetraExt::ModelEvaluator> model_,
                          Teuchos::RCP<Rythmos::IntegrationObserverBase<double> > observer ) :
  model(model_),
  num_p(model->createInArgs().Np()),
  num_g(model->createOutArgs().Ng()),
  out(Teuchos::VerboseObjectBase::getDefaultOStream())
{
  using Teuchos::RCP;
  using Teuchos::rcp;

  // Allow for Matrix-Free implementation
  string jacobianSource = piroParams->get("Jacobian Operator", "Have Jacobian");
  if (jacobianSource == "Matrix-Free") {
    if (piroParams->isParameter("Matrix-Free Perturbation")) {
      model = Teuchos::rcp(new Piro::Epetra::MatrixFreeDecorator(model,
                           piroParams->get<double>("Matrix-Free Perturbation")));
    }
    else model = Teuchos::rcp(new Piro::Epetra::MatrixFreeDecorator(model));
  }


  TEUCHOS_TEST_FOR_EXCEPTION(num_p > 1, Teuchos::Exceptions::InvalidParameter,
                     std::endl << "Error in Piro::Epetra::RythmosSolver " <<
                     "Not Implemented for Np>1 : " << num_p << std::endl);
  TEUCHOS_TEST_FOR_EXCEPTION(num_g > 1, Teuchos::Exceptions::InvalidParameter,
                     std::endl << "Error in Piro::Epetra::RythmosSolver " <<
                     "Not Implemented for Ng>1 : " << num_g << std::endl);

      //
      *out << "\nA) Get the base parameter list ...\n";
      //

//AGS: 6/2012: Separating into old and new parameter list styles. The
//  old style has a Rythmos sublist, which is used here but not compatible
//  with builders in Rythmos. The new style has a sublist called "Rythmos Solver"
//  which inludes settings for this class, a Rytmos sublist that can be
//  directly passed to Rythmols builders, and a Stratimikos sublist. THe
//  old way will be deprecated at some point.

  if (piroParams->isSublist("Rythmos")) {
    /** Old parameter list format **/
      RCP<Teuchos::ParameterList> rythmosPL = sublist(piroParams, "Rythmos", true);
      rythmosPL->validateParameters(*getValidRythmosParameters(),0);

      {
        const string verbosity = rythmosPL->get("Verbosity Level", "VERB_DEFAULT");
        solnVerbLevel = Teuchos::VERB_DEFAULT;
        if      (verbosity == "VERB_NONE")    solnVerbLevel = Teuchos::VERB_NONE;
        else if (verbosity == "VERB_LOW")     solnVerbLevel = Teuchos::VERB_LOW;
        else if (verbosity == "VERB_MEDIUM")  solnVerbLevel = Teuchos::VERB_MEDIUM;
        else if (verbosity == "VERB_HIGH")    solnVerbLevel = Teuchos::VERB_HIGH;
        else if (verbosity == "VERB_EXTREME") solnVerbLevel = Teuchos::VERB_EXTREME;
      }

      const int numTimeSteps = rythmosPL->get("Num Time Steps", 10);
      const double t_init = 0.0;
      t_final = rythmosPL->get("Final Time", 0.1);

      const Rythmos::TimeRange<double> fwdTimeRange(t_init, t_final);
      const double delta_t = t_final / (double) numTimeSteps;
      *out << "\ndelta_t = " << delta_t;

      const string stepperType = rythmosPL->get("Stepper Type", "Backward Euler");

      //
      *out << "\nB) Create the Stratimikos linear solver factory ...\n";
      //
      // This is the linear solve strategy that will be used to solve for the
      // linear system with the W.
      //

      Stratimikos::DefaultLinearSolverBuilder linearSolverBuilder;
      linearSolverBuilder.setParameterList(sublist(rythmosPL, "Stratimikos", true));
      RCP<Thyra::LinearOpWithSolveFactoryBase<double> >
	W_factory = createLinearSolveStrategy(linearSolverBuilder);

      //
      *out << "\nC) Create and initalize the forward model ...\n";
      //

      // C.1) Create the underlying EpetraExt::ModelEvaluator
      // already constructed as "model". Decorate if needed.

      if (stepperType == "Explicit RK") {
        if (rythmosPL->get("Invert Mass Matrix", false)) {
          Teuchos::RCP<EpetraExt::ModelEvaluator> origModel = model;
          bool lump=rythmosPL->get("Lump Mass Matrix", false);
          model = Teuchos::rcp(new Piro::Epetra::InvertMassMatrixDecorator(
                   sublist(rythmosPL,"Stratimikos", true), origModel));
        }
      }

      // C.2) Create the Thyra-wrapped ModelEvaluator

      fwdStateModel = epetraModelEvaluator(model, W_factory);

      const RCP<const Thyra::VectorSpaceBase<double> >
	x_space = fwdStateModel->get_x_space();

      //
      *out << "\nD) Create the stepper and integrator for the forward problem ...\n";
      //

      fwdTimeStepSolver = Rythmos::timeStepNonlinearSolver<double>();

      if (rythmosPL->getEntryPtr("NonLinear Solver")) {
        RCP<Teuchos::ParameterList> nonlinePL =
           sublist(rythmosPL, "NonLinear Solver", true);
        fwdTimeStepSolver->setParameterList(nonlinePL);
      }

      if (stepperType == "Backward Euler")
        fwdStateStepper = Rythmos::backwardEulerStepper<double>
           (fwdStateModel, fwdTimeStepSolver);
      else if (stepperType == "Explicit RK")
        fwdStateStepper = Rythmos::explicitRKStepper<double>(fwdStateModel);
      else
        TEUCHOS_TEST_FOR_EXCEPTION( true, Teuchos::Exceptions::InvalidParameter,
                     std::endl << "Error! Piro::Epetra::RythmosSolver: Invalid Steper Type: "
                     << stepperType << std::endl);

      fwdStateStepper->setParameterList(sublist(rythmosPL, "Rythmos Stepper", true));

      {
        RCP<Teuchos::ParameterList>
          integrationControlPL = sublist(rythmosPL, "Rythmos Integration Control", true);
        bool var = integrationControlPL->get( "Take Variable Steps", false );
        integrationControlPL->set( "Fixed dt", Teuchos::as<double>(delta_t) );

        RCP<Rythmos::DefaultIntegrator<double> >
          defaultIntegrator = Rythmos::controlledDefaultIntegrator<double>(
            Rythmos::simpleIntegrationControlStrategy<double>(integrationControlPL)
            );
        fwdStateIntegrator = defaultIntegrator;
      }
      fwdStateIntegrator->setParameterList(sublist(rythmosPL, "Rythmos Integrator", true));
    }
  else if (piroParams->isSublist("Rythmos Solver")) {
/** New parameter list format **/
      RCP<Teuchos::ParameterList> rythmosSolverPL = sublist(piroParams, "Rythmos Solver", true);
      RCP<Teuchos::ParameterList> rythmosPL = sublist(rythmosSolverPL, "Rythmos", true);
      rythmosSolverPL->validateParameters(*getValidRythmosSolverParameters(),0);

      {
        const string verbosity = rythmosSolverPL->get("Verbosity Level", "VERB_DEFAULT");
        solnVerbLevel = Teuchos::VERB_DEFAULT;
        if      (verbosity == "VERB_NONE")    solnVerbLevel = Teuchos::VERB_NONE;
        else if (verbosity == "VERB_LOW")     solnVerbLevel = Teuchos::VERB_LOW;
        else if (verbosity == "VERB_MEDIUM")  solnVerbLevel = Teuchos::VERB_MEDIUM;
        else if (verbosity == "VERB_HIGH")    solnVerbLevel = Teuchos::VERB_HIGH;
        else if (verbosity == "VERB_EXTREME") solnVerbLevel = Teuchos::VERB_EXTREME;
      }

      t_final = rythmosPL->sublist("Integrator Settings").get("Final Time", 0.1);

      const string stepperType = rythmosPL->sublist("Stepper Settings")
           .sublist("Stepper Selection").get("Stepper Type", "Backward Euler");

      //
      *out << "\nB) Create the Stratimikos linear solver factory ...\n";
      //
      // This is the linear solve strategy that will be used to solve for the
      // linear system with the W.
      //

      Stratimikos::DefaultLinearSolverBuilder linearSolverBuilder;
      linearSolverBuilder.setParameterList(sublist(rythmosSolverPL, "Stratimikos", true));
      RCP<Thyra::LinearOpWithSolveFactoryBase<double> >
	W_factory = createLinearSolveStrategy(linearSolverBuilder);

      //
      *out << "\nC) Create and initalize the forward model ...\n";
      //

      // C.1) Create the underlying EpetraExt::ModelEvaluator
      // already constructed as "model". Decorate if needed.

      // TODO: Generelize to any explicit method, option to invert mass matrix
      if (stepperType == "Explicit RK") {
        if (rythmosSolverPL->get("Invert Mass Matrix", false)) {
          Teuchos::RCP<EpetraExt::ModelEvaluator> origModel = model;
          bool lump=rythmosSolverPL->get("Lump Mass Matrix", false);
          model = Teuchos::rcp(new Piro::Epetra::InvertMassMatrixDecorator(
                   sublist(rythmosSolverPL,"Stratimikos", true), origModel));
        }
      }

      // C.2) Create the Thyra-wrapped ModelEvaluator

      fwdStateModel = epetraModelEvaluator(model, W_factory);

      const RCP<const Thyra::VectorSpaceBase<double> >
	x_space = fwdStateModel->get_x_space();

      //
      *out << "\nD) Create the stepper and integrator for the forward problem ...\n";
      //

      fwdTimeStepSolver = Rythmos::timeStepNonlinearSolver<double>();

      if (rythmosSolverPL->getEntryPtr("NonLinear Solver")) {
        RCP<Teuchos::ParameterList> nonlinePL =
           sublist(rythmosSolverPL, "NonLinear Solver", true);
        fwdTimeStepSolver->setParameterList(nonlinePL);
      }

    // Force Default Integrator since this is needed for Observers
    rythmosPL->sublist("Integrator Settings").sublist("Integrator Selection").
                       set("Integrator Type","Default Integrator");

    RCP<Rythmos::IntegratorBuilder<double> > ib = Rythmos::integratorBuilder<double>();
    ib->setParameterList(rythmosPL);
    Thyra::ModelEvaluatorBase::InArgs<double> ic = fwdStateModel->getNominalValues();
    RCP<Rythmos::IntegratorBase<double> > integrator = ib->create(fwdStateModel,ic,fwdTimeStepSolver);
    fwdStateIntegrator = Teuchos::rcp_dynamic_cast<Rythmos::DefaultIntegrator<double> >(integrator,true);

    fwdStateStepper = fwdStateIntegrator->getNonconstStepper();
  } else {
      TEUCHOS_TEST_FOR_EXCEPTION(piroParams->isSublist("Rythmos")||piroParams->isSublist("Rythmos Solver"),
        Teuchos::Exceptions::InvalidParameter, std::endl <<
       "Error! Piro::Epetra::RythmosSolver: must have either Rythmos or Rythmos Solver sublist ");
  }


  if (observer != Teuchos::null)
    fwdStateIntegrator->setIntegrationObserver(observer);

  thyraImplementation_ = rcp(new ThyraRythmosSolver(
   fwdStateIntegrator,
   fwdStateStepper,
   fwdTimeStepSolver,
   fwdStateModel,
   t_final,
   Teuchos::null, // No initial condition model
   solnVerbLevel));
}

int Piro::Epetra::RythmosSolver::Np() const
{
  return num_p;
}

int Piro::Epetra::RythmosSolver::Ng() const
{
  return num_g + 1;
}

Teuchos::RCP<const Epetra_Map> Piro::Epetra::RythmosSolver::get_x_map() const
{
  Teuchos::RCP<const Epetra_Map> neverused;
  return neverused;
}

Teuchos::RCP<const Epetra_Map> Piro::Epetra::RythmosSolver::get_f_map() const
{
  Teuchos::RCP<const Epetra_Map> neverused;
  return neverused;
}

Teuchos::RCP<const Epetra_Map> Piro::Epetra::RythmosSolver::get_p_map(int l) const
{
  TEUCHOS_TEST_FOR_EXCEPTION(l >= num_p || l < 0, Teuchos::Exceptions::InvalidParameter,
                     std::endl <<
                     "Error in Piro::Epetra::RythmosSolver::get_p_map():  " <<
                     "Invalid parameter index l = " <<
                     l << std::endl);
  return model->get_p_map(l);
}

Teuchos::RCP<const Epetra_Map> Piro::Epetra::RythmosSolver::get_g_map(int j) const
{
  TEUCHOS_TEST_FOR_EXCEPTION(j > num_g || j < 0, Teuchos::Exceptions::InvalidParameter,
                     std::endl <<
                     "Error in Piro::Epetra::RythmosSolver::get_g_map():  " <<
                     "Invalid response index j = " <<
                     j << std::endl);

  if      (j < num_g) return model->get_g_map(j);
  else if (j == num_g) return model->get_x_map();
}

Teuchos::RCP<const Epetra_Vector> Piro::Epetra::RythmosSolver::get_x_init() const
{
  Teuchos::RCP<const Epetra_Vector> neverused;
  return neverused;
}

Teuchos::RCP<const Epetra_Vector> Piro::Epetra::RythmosSolver::get_p_init(int l) const
{
  TEUCHOS_TEST_FOR_EXCEPTION(l >= num_p || l < 0, Teuchos::Exceptions::InvalidParameter,
                     std::endl <<
                     "Error in Piro::Epetra::RythmosSolver::get_p_init():  " <<
                     "Invalid parameter index l = " <<
                     l << std::endl);
  return model->get_p_init(l);
}

EpetraExt::ModelEvaluator::InArgs Piro::Epetra::RythmosSolver::createInArgs() const
{
  EpetraExt::ModelEvaluator::InArgsSetup inArgs;
  inArgs.setModelEvalDescription(this->description());
  inArgs.set_Np(thyraImplementation_->Np());
  return inArgs;
}

EpetraExt::ModelEvaluator::OutArgs Piro::Epetra::RythmosSolver::createOutArgs() const
{
  EpetraExt::ModelEvaluator::OutArgsSetup outArgs;
  outArgs.setModelEvalDescription(this->description());

  {
    const int thyraNp = thyraImplementation_->Np();
    const int thyraNg = thyraImplementation_->Ng();

    outArgs.set_Np_Ng(thyraNp, thyraNg);

    const Thyra::ModelEvaluatorBase::OutArgs<double> thyraOutArgs = thyraImplementation_->createOutArgs();
    for (int j = 0; j < thyraNg; ++j) {
      for (int l = 0; l < thyraNp; ++l) {
        EpetraExt::ModelEvaluator::DerivativeSupport dgdp_support;
        {
          const Thyra::ModelEvaluatorBase::DerivativeSupport thyra_dgdp_support =
            thyraOutArgs.supports(Thyra::ModelEvaluatorBase::OUT_ARG_DgDp, j, l);

          if (thyra_dgdp_support.supports(Thyra::ModelEvaluatorBase::DERIV_MV_JACOBIAN_FORM)) {
            dgdp_support.plus(EpetraExt::ModelEvaluator::DERIV_MV_BY_COL);
          }
          if (thyra_dgdp_support.supports(Thyra::ModelEvaluatorBase::DERIV_MV_GRADIENT_FORM)) {
            dgdp_support.plus(EpetraExt::ModelEvaluator::DERIV_TRANS_MV_BY_ROW);
          }
        }
        outArgs.setSupports(EpetraExt::ModelEvaluator::OUT_ARG_DgDp, j, l, dgdp_support);
      }
    }
  }

  return outArgs;
}

void Piro::Epetra::RythmosSolver::evalModel(const InArgs& inArgs, const OutArgs& outArgs) const
{
  Thyra::ModelEvaluatorBase::InArgs<double> thyraInArgs = thyraImplementation_->createInArgs();
  for (int l = 0; l < thyraInArgs.Np(); ++l) {
    const Teuchos::RCP<const Epetra_Vector> p_in = inArgs.get_p(l);
    if (Teuchos::nonnull(p_in)) {
      thyraInArgs.set_p(l, Thyra::create_Vector(p_in, thyraImplementation_->get_p_space(l)));
    }
  }

  Thyra::ModelEvaluatorBase::OutArgs<double> thyraOutArgs = thyraImplementation_-> createOutArgs();
  {
    // Translate outArgs -> thyraOutArgs
    // Responses
    for (int j = 0; j < thyraOutArgs.Ng(); ++j) {
      const Teuchos::RCP<Epetra_Vector> g_out = outArgs.get_g(j);
      if (Teuchos::nonnull(g_out)) {
        thyraOutArgs.set_g(j, Thyra::create_Vector(g_out, thyraImplementation_->get_g_space(j)));
      }
    }
    // Derivatives
    for (int j = 0; j < thyraOutArgs.Ng(); ++j) {
      for (int l = 0; l < thyraOutArgs.Np(); ++l) {
        const Thyra::ModelEvaluatorBase::DerivativeSupport dgdp_support =
          thyraOutArgs.supports(Thyra::ModelEvaluatorBase::OUT_ARG_DgDp, j, l);
        if (!dgdp_support.none()) {
          const EpetraExt::ModelEvaluator::Derivative dgdp_out = outArgs.get_DgDp(j, l);
          if (!dgdp_out.isEmpty()) {
            TEUCHOS_ASSERT(Teuchos::is_null(dgdp_out.getLinearOp()));
            const Thyra::ModelEvaluatorBase::EDerivativeMultiVectorOrientation mv_orient =
              Thyra::convert(dgdp_out.getMultiVectorOrientation());
            const Teuchos::RCP<const Thyra::VectorSpaceBase<double> > mv_range =
              (mv_orient == Thyra::ModelEvaluatorBase::DERIV_MV_JACOBIAN_FORM) ?
              thyraImplementation_->get_g_space(j) :
              thyraImplementation_->get_p_space(l);
            const Teuchos::RCP<Thyra::MultiVectorBase<double> > mv_thyra =
              Thyra::create_MultiVector(dgdp_out.getMultiVector(), mv_range);
            thyraOutArgs.set_DgDp(j, l, Thyra::ModelEvaluatorBase::Derivative<double>(mv_thyra, mv_orient));
          }
        }
      }
    }
  }

  thyraImplementation_->evalModel(thyraInArgs, thyraOutArgs);
}

Teuchos::RCP<const Teuchos::ParameterList>
Piro::Epetra::RythmosSolver::getValidRythmosParameters() const
{
  Teuchos::RCP<Teuchos::ParameterList> validPL =
     Teuchos::rcp(new Teuchos::ParameterList("ValidRythmosParams"));;
  validPL->sublist("NonLinear Solver", false, "");
  validPL->set<int>("Num Time Steps", 0, "");
  validPL->set<double>("Final Time", 1.0, "");
  validPL->sublist("Rythmos Stepper", false, "");
  validPL->sublist("Rythmos Integrator", false, "");
  validPL->sublist("Rythmos Integration Control", false, "");
  validPL->sublist("Stratimikos", false, "");
  validPL->set<std::string>("Verbosity Level", "", "");
  validPL->set<std::string>("Stepper Type", "", "");

  validPL->set<double>("Alpha", 1.0, "");
  validPL->set<double>("Beta", 1.0, "");
  validPL->set<double>("Max State Error", 1.0, "");
  validPL->set<std::string>("Name", "", "");
  validPL->set<bool>("Invert Mass Matrix", false, "");
  validPL->set<bool>("Lump Mass Matrix", false, "");
  validPL->set<std::string>("Stepper Method", "", "");
  return validPL;
}

Teuchos::RCP<const Teuchos::ParameterList>
Piro::Epetra::RythmosSolver::getValidRythmosSolverParameters() const
{
  Teuchos::RCP<Teuchos::ParameterList> validPL =
     Teuchos::rcp(new Teuchos::ParameterList("ValidRythmosSolverParams"));;
  validPL->sublist("Rythmos", false, "");
  validPL->sublist("Stratimikos", false, "");
  validPL->sublist("NonLinear Solver", false, "");
  validPL->set<std::string>("Verbosity Level", "", "");
  validPL->set<bool>("Invert Mass Matrix", false, "");
  validPL->set<bool>("Lump Mass Matrix", false, "");
  return validPL;
}

