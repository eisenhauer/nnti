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
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Andrey Prokopenko (aprokop@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef MUELU_IFPACK2SMOOTHER_DEF_HPP
#define MUELU_IFPACK2SMOOTHER_DEF_HPP

#include "MueLu_ConfigDefs.hpp"

#if defined(HAVE_MUELU_TPETRA) && defined(HAVE_MUELU_IFPACK2)

#include <Teuchos_ParameterList.hpp>

#include <Ifpack2_Chebyshev.hpp>
#include <Ifpack2_Factory.hpp>
#include <Ifpack2_Parameters.hpp>

#include <Xpetra_MultiVectorFactory.hpp>

#include "MueLu_Ifpack2Smoother_decl.hpp"
#include "MueLu_Level.hpp"
#include "MueLu_Utilities.hpp"
#include "MueLu_Monitor.hpp"

namespace MueLu {

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Ifpack2Smoother(std::string const & type, Teuchos::ParameterList const & paramList, LO const &overlap)
    : type_(type), overlap_(overlap)
  {
    SetParameterList(paramList);
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::SetParameterList(const Teuchos::ParameterList& paramList) {
    Factory::SetParameterList(paramList);

    if (SmootherPrototype::IsSetup()) {
      // It might be invalid to change parameters after the setup, but it depends entirely on Ifpack implementation.
      // TODO: I don't know if Ifpack returns an error code or exception or ignore parameters modification in this case...
      SetPrecParameters();
    }
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::SetPrecParameters(const Teuchos::ParameterList& list) const {
    ParameterList& paramList = const_cast<ParameterList&>(this->GetParameterList());
    paramList.setParameters(list);

    RCP<ParameterList> precList = this->RemoveFactoriesFromList(this->GetParameterList());

    prec_->setParameters(*precList);

    paramList.setParameters(*precList);
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::DeclareInput(Level &currentLevel) const {
    this->Input(currentLevel, "A");
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Setup(Level& currentLevel) {
    FactoryMonitor m(*this, "Setup Smoother", currentLevel);

    if (this->IsSetup() == true)
      this->GetOStream(Warnings0, 0) << "Warning: MueLu::Ifpack2Smoother::Setup(): Setup() has already been called";

    A_ = Factory::Get< RCP<Matrix> >(currentLevel, "A");

    Scalar negone = -Teuchos::ScalarTraits<Scalar>::one();

    Scalar lambdaMax = negone;
    if (type_ == "CHEBYSHEV")
      try {
        lambdaMax = Teuchos::getValue<Scalar>(this->GetParameter("chebyshev: max eigenvalue"));
        this->GetOStream(Statistics1, 0) << "chebyshev: max eigenvalue (cached with smoother parameter list)" << " = " << lambdaMax << std::endl;

      } catch (Teuchos::Exceptions::InvalidParameterName) {
        lambdaMax = A_->GetMaxEigenvalueEstimate();
        if (lambdaMax != negone) {
          this->GetOStream(Statistics1, 0) << "chebyshev: max eigenvalue (cached with matrix)" << " = " << lambdaMax << std::endl;
          this->SetParameter("chebyshev: max eigenvalue", ParameterEntry(lambdaMax));
        }
      }

    RCP<const Tpetra::CrsMatrix<SC, LO, GO, NO, LMO> > tpA = Utils::Op2NonConstTpetraCrs(A_);
    prec_ = Ifpack2::Factory::create(type_, tpA, overlap_);

    SetPrecParameters();
    prec_->initialize();
    prec_->compute();

    SmootherPrototype::IsSetup(true);

    if (type_ == "CHEBYSHEV" && lambdaMax == negone) {
      typedef Tpetra::CrsMatrix<SC, LO, GO, NO, LMO> MatrixType;

      Teuchos::RCP<Ifpack2::Chebyshev<MatrixType> > chebyPrec = rcp_dynamic_cast<Ifpack2::Chebyshev<MatrixType> >(prec_);
      if (chebyPrec != Teuchos::null) {
        lambdaMax = chebyPrec->getLambdaMaxForApply();
        A_->SetMaxEigenvalueEstimate(lambdaMax);
        this->GetOStream(Statistics1, 0) << "chebyshev: max eigenvalue (calculated by Ifpack2)" << " = " << lambdaMax << std::endl;
      }
      TEUCHOS_TEST_FOR_EXCEPTION(lambdaMax == negone, Exceptions::RuntimeError, "MueLu::IfpackSmoother::Setup(): no maximum eigenvalue estimate");
    }

    this->GetOStream(Statistics0, 0) << description() << std::endl;
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Apply(MultiVector& X, const MultiVector& B, bool InitialGuessIsZero) const {
    TEUCHOS_TEST_FOR_EXCEPTION(SmootherPrototype::IsSetup() == false, Exceptions::RuntimeError, "MueLu::IfpackSmoother::Apply(): Setup() has not been called");

    // Forward the InitialGuessIsZero option to Ifpack2
    //  TODO: It might be nice to switch back the internal
    //        "zero starting solution" option of the ifpack2 object prec_ to his
    //        initial value at the end but there is no way right now to get
    //        the current value of the "zero starting solution" in ifpack2.
    //        It's not really an issue, as prec_  can only be used by this method.
    Teuchos::ParameterList paramList;
    if (type_ == "CHEBYSHEV") {
      paramList.set("chebyshev: zero starting solution", InitialGuessIsZero);

    } else if (type_ == "RELAXATION") {
      paramList.set("relaxation: zero starting solution", InitialGuessIsZero);

    } else if (type_ == "KRYLOV") {
      paramList.set("krylov: zero starting solution", InitialGuessIsZero);

    } else if (type_ == "SCHWARZ") {
      int overlap=0;
      Ifpack2::getParameter(paramList, "schwarz: overlap level", overlap);
      if (InitialGuessIsZero == true)
        paramList.set("schwarz: zero starting solution", InitialGuessIsZero);

    } else if (type_ == "ILUT" || type_ == "RILUK" || type_ == "AMESOS2") {
      //do nothing

    } else {
      // TODO: When https://software.sandia.gov/bugzilla/show_bug.cgi?id=5283#c2 is done
      // we should remove the if/else/elseif and just test if this
      // option is supported by current ifpack2 preconditioner
      TEUCHOS_TEST_FOR_EXCEPTION(true, Exceptions::RuntimeError, "Ifpack2Smoother::Apply(): Ifpack2 preconditioner '" + type_ + "' not supported");
    }
    SetPrecParameters(paramList);

    // Apply
    if ( (type_ != "ILUT" && type_ != "SCHWARZ") || InitialGuessIsZero) {
      Tpetra::MultiVector<SC,LO,GO,NO> &tpX = Utils::MV2NonConstTpetraMV(X);
      Tpetra::MultiVector<SC,LO,GO,NO> const &tpB = Utils::MV2TpetraMV(B);
      prec_->apply(tpB,tpX);
    } else {
      typedef Teuchos::ScalarTraits<Scalar> TST;
      RCP<MultiVector> Residual = Utils::Residual(*A_,X,B);
      RCP<MultiVector> Correction = MultiVectorFactory::Build(A_->getDomainMap(), X.getNumVectors());
      Tpetra::MultiVector<SC,LO,GO,NO> &tpX = Utils::MV2NonConstTpetraMV(*Correction);
      Tpetra::MultiVector<SC,LO,GO,NO> const &tpB = Utils::MV2TpetraMV(*Residual);
      prec_->apply(tpB,tpX);
      X.update(TST::one(), *Correction, TST::one());
    }
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  RCP<MueLu::SmootherPrototype<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps> > Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Copy() const {
    RCP<Ifpack2Smoother> smoother = rcp(new Ifpack2Smoother(*this) );
    smoother->SetParameterList(this->GetParameterList());
    return smoother;
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  std::string Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::description() const {
    std::ostringstream out;
    if (SmootherPrototype::IsSetup()) {
      out << prec_->description();
    } else {
      out << SmootherPrototype::description();
      out << "{type = " << type_ << "}";
    }
    return out.str();
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::print(Teuchos::FancyOStream &out, const VerbLevel verbLevel) const {
    MUELU_DESCRIBE;

    if (verbLevel & Parameters0)
      out0 << "Prec. type: " << type_ << std::endl;

    if (verbLevel & Parameters1) {
      out0 << "Parameter list: " << std::endl;
      Teuchos::OSTab tab2(out);
      out << this->GetParameterList();
      out0 << "Overlap: "        << overlap_ << std::endl;
    }

    if (verbLevel & External)
      if (prec_ != Teuchos::null) {
        Teuchos::OSTab tab2(out);
        out << *prec_ << std::endl;
      }

    if (verbLevel & Debug) {
      out0 << "IsSetup: " << Teuchos::toString(SmootherPrototype::IsSetup()) << std::endl
           << "-" << std::endl
           << "RCP<prec_>: " << prec_ << std::endl;
    }
  }

} // namespace MueLu

#endif // HAVE_MUELU_TPETRA && HAVE_MUELU_IFPACK2
#endif // MUELU_IFPACK2SMOOTHER_DEF_HPP
