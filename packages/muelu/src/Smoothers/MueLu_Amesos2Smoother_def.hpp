#ifndef MUELU_AMESOS2SMOOTHER_DEF_HPP
#define MUELU_AMESOS2SMOOTHER_DEF_HPP

#ifdef HAVE_MUELU_AMESOS2
#include "MueLu_Amesos2Smoother_decl.hpp"
#include "MueLu_Level.hpp"
#include "MueLu_Monitor.hpp"

namespace MueLu {

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  Amesos2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Amesos2Smoother(std::string const & type, Teuchos::ParameterList const & paramList, RCP<FactoryBase> AFact)
    : type_(type), paramList_(paramList), AFact_(AFact)
  {

#if defined(HAVE_AMESOS2_SUPERLU)
    type_ = "Superlu";
#elif defined(HAVE_AMESOS2_KLU)
    type_ = "Klu";
#endif
    TEUCHOS_TEST_FOR_EXCEPTION(type_ == "", Exceptions::RuntimeError, "MueLu::Amesos2Smoother::Amesos2Smoother(): Amesos2 compiled without KLU and SuperLU. Cannot define a solver by default for this Amesos2Smoother object");

  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  Amesos2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::~Amesos2Smoother() {}

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void Amesos2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::DeclareInput(Level &currentLevel) const {
    currentLevel.DeclareInput("A", AFact_.get());
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void Amesos2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Setup(Level &currentLevel) {
    Monitor m(*this, "Setup Smoother");
    if (SmootherPrototype::IsSetup() == true) this->GetOStream(Warnings0, 0) << "Warning: MueLu::Amesos2Smoother::Setup(): Setup() has already been called";

    RCP<Operator> A_ = currentLevel.Get< RCP<Operator> >("A", AFact_.get());

    RCP<Tpetra_CrsMatrix> tA = Utils::Op2NonConstTpetraCrs(A_);
  
    prec_ = Amesos2::create<Tpetra_CrsMatrix,Tpetra_MultiVector>(type_, tA);
    TEUCHOS_TEST_FOR_EXCEPTION(prec_ == Teuchos::null, Exceptions::RuntimeError, "Amesos2::create returns Teuchos::null");

    //TODO      prec_->setParameters(paramList_);
    //TODO
    // int rv = prec_->numericFactorization();
    //       if (rv != 0) {
    //         std::ostringstream buf;
    //         buf << rv;
    //         std::string msg = "Amesos2_BaseSolver::NumericFactorization return value of " + buf.str(); //TODO: BaseSolver or ... ?
    //         throw(Exceptions::RuntimeError(msg));
    //       }

    SmootherPrototype::IsSetup(true);
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void Amesos2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Apply(MultiVector &X, MultiVector const &B, bool const &InitialGuessIsZero) const
  {
    TEUCHOS_TEST_FOR_EXCEPTION(SmootherPrototype::IsSetup() == false, Exceptions::RuntimeError, "MueLu::Amesos2Smoother::Apply(): Setup() has not been called");

    RCP<Tpetra_MultiVector> tX = Utils::MV2NonConstTpetraMV2(X);
    MultiVector & BNonC = const_cast<MultiVector&>(B);
    RCP<Tpetra_MultiVector> tB = Utils::MV2NonConstTpetraMV2(BNonC);
    prec_->setX(tX);
    prec_->setB(tB);

    prec_->solve();

    prec_->setX(Teuchos::null);
    prec_->setB(Teuchos::null);
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  RCP<MueLu::SmootherPrototype<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps> > Amesos2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Copy() const {
    return rcp( new Amesos2Smoother(*this) );
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  std::string Amesos2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::description() const {
    std::ostringstream out;
    out << SmootherPrototype::description();
    out << "{type = " << type_ << "}";
    return out.str();
  }
    
  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void Amesos2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::print(Teuchos::FancyOStream &out, const VerbLevel verbLevel) const {
    MUELU_DESCRIBE;

    if (verbLevel & Parameters0) {
      out0 << "Prec. type: " << type_ << std::endl;
    }
      
    if (verbLevel & Parameters1) { 
      out0 << "Parameter list: " << std::endl; { Teuchos::OSTab tab2(out); out << paramList_; }
    }
      
    if (verbLevel & External) {
      if (prec_ != Teuchos::null) { Teuchos::OSTab tab2(out); out << *prec_ << std::endl; }
    }

    if (verbLevel & Debug) {
      out0 << "IsSetup: " << Teuchos::toString(SmootherPrototype::IsSetup()) << std::endl
           << "-" << std::endl
           << "RCP<prec_>: " << prec_ << std::endl;
    }
  }

} // namespace MueLu

#endif // HAVE_MUELU_AMESOS2
#endif // MUELU_AMESOS2SMOOTHER_DEF_HPP
