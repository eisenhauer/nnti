#ifndef MUELU_SMOOTHERFACTORY_DEF_HPP
#define MUELU_SMOOTHERFACTORY_DEF_HPP

#include "MueLu_SmootherFactory_decl.hpp"

namespace MueLu {

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::SmootherFactory(RCP<SmootherPrototype> preAndPostSmootherPrototype)
    : preSmootherPrototype_(preAndPostSmootherPrototype), postSmootherPrototype_(preAndPostSmootherPrototype)
  { 
    TEUCHOS_TEST_FOR_EXCEPTION(preAndPostSmootherPrototype != Teuchos::null && preAndPostSmootherPrototype->IsSetup() == true, Exceptions::RuntimeError, "preAndPostSmootherPrototype is not a smoother prototype (IsSetup() == true)");
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::SmootherFactory(RCP<SmootherPrototype> preSmootherPrototype, RCP<SmootherPrototype> postSmootherPrototype)
    : preSmootherPrototype_(preSmootherPrototype), postSmootherPrototype_(postSmootherPrototype)
  { 
    TEUCHOS_TEST_FOR_EXCEPTION(preSmootherPrototype  != Teuchos::null && preSmootherPrototype->IsSetup()  == true, Exceptions::RuntimeError, "preSmootherPrototype is not a smoother prototype (IsSetup() == true)");
    TEUCHOS_TEST_FOR_EXCEPTION(postSmootherPrototype != Teuchos::null && postSmootherPrototype->IsSetup() == true, Exceptions::RuntimeError, "postSmootherPrototype is not a smoother prototype (IsSetup() == true)");
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::~SmootherFactory() {}
 
  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::SetSmootherPrototypes(RCP<SmootherPrototype> preAndPostSmootherPrototype) {
    TEUCHOS_TEST_FOR_EXCEPTION(preAndPostSmootherPrototype != Teuchos::null && preAndPostSmootherPrototype->IsSetup() == true, Exceptions::RuntimeError, "preAndPostSmootherPrototype is not a smoother prototype (IsSetup() == true)");

    preSmootherPrototype_ = preAndPostSmootherPrototype;
    postSmootherPrototype_ = preAndPostSmootherPrototype;
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::SetSmootherPrototypes(RCP<SmootherPrototype> preSmootherPrototype, RCP<SmootherPrototype> postSmootherPrototype) {
    TEUCHOS_TEST_FOR_EXCEPTION(preSmootherPrototype  != Teuchos::null && preSmootherPrototype->IsSetup()  == true, Exceptions::RuntimeError, "preSmootherPrototype is not a smoother prototype (IsSetup() == true)");
    TEUCHOS_TEST_FOR_EXCEPTION(postSmootherPrototype != Teuchos::null && postSmootherPrototype->IsSetup() == true, Exceptions::RuntimeError, "postSmootherPrototype is not a smoother prototype (IsSetup() == true)");
    preSmootherPrototype_ = preSmootherPrototype;
    postSmootherPrototype_ = postSmootherPrototype;
  }
    
  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::GetSmootherPrototypes(RCP<SmootherPrototype> & preSmootherPrototype, RCP<SmootherPrototype> & postSmootherPrototype) const {
    preSmootherPrototype = preSmootherPrototype_;
    postSmootherPrototype = postSmootherPrototype_;
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::DeclareInput(Level &currentLevel) const { 
    if (preSmootherPrototype_ != Teuchos::null) {
      preSmootherPrototype_->DeclareInput(currentLevel);
    }
    if ((postSmootherPrototype_ != Teuchos::null) && (preSmootherPrototype_ != postSmootherPrototype_)) {
      postSmootherPrototype_->DeclareInput(currentLevel);
    }
  }

  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::Build(Level& currentLevel) const {
    return BuildSmoother(currentLevel, BOTH);
  }
    
  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::BuildSmoother(Level & currentLevel, PreOrPost const preOrPost) const {
    RCP<SmootherPrototype> preSmoother;
    RCP<SmootherPrototype> postSmoother;
      
    if ((preOrPost == BOTH || preOrPost == PRE) && (preSmootherPrototype_ != Teuchos::null)) {
      preSmoother = preSmootherPrototype_->Copy();
      //preSmoother = rcp( new SmootherPrototype(preSmootherPrototype_) );
      //TODO if outputlevel high enough
      //TODO preSmoother.Print();
      preSmoother->Setup(currentLevel);
        
      // Level Set
      currentLevel.Set<RCP<SmootherBase> >("PreSmoother", preSmoother, this);
      currentLevel.Set<RCP<SmootherBase> >("PreSmoother", preSmoother); //TODO: remove this
    }
      
    if ((preOrPost == BOTH || preOrPost == POST) && (postSmootherPrototype_ != Teuchos::null))
      {
        if (preOrPost == BOTH && preSmootherPrototype_ == postSmootherPrototype_) {
              
          // Very simple reuse. TODO: should be done in MueMat too
          postSmoother = preSmoother;
              
          //            }  else if (preOrPost == BOTH &&
          //                        preSmootherPrototype_ != Teuchos::null &&
          //                        preSmootherPrototype_->GetType() == postSmootherPrototype_->GetType()) {
              
          //               // More complex reuse case: need implementation of CopyParameters() and a smoothers smart enough to know when parameters affect the setup phase.
              
          //               // YES: post-smoother == pre-smoother 
          //               // => copy the pre-smoother to avoid the setup phase of the post-smoother.
          //               postSmoother = preSmoother->Copy();
          //               // If the post-smoother parameters are different from
          //               // pre-smoother, the parameters stored in the post-smoother
          //               // prototype are copied in the new post-smoother object.
          //               postSmoother->CopyParameters(postSmootherPrototype_);
          //               // If parameters don't influence the Setup phase (it is the case
          //               // for Jacobi, Chebyshev...), PostSmoother is already setup. Nothing
          //               // more to do. In the case of ILU, parameters of the smoother
          //               // are in fact the parameters of the Setup phase. The call to
          //               // CopyParameters resets the smoother (only if parameters are
          //               // different) and we must call Setup() again.
          //               postSmoother->Setup(currentLevel);

          //               // TODO: if CopyParameters do not exist, do setup twice.

        } else {
              
          // NO reuse: preOrPost==POST or post-smoother != pre-smoother
          // Copy the prototype and run the setup phase.
          postSmoother = postSmootherPrototype_->Copy();
          postSmoother->Setup(currentLevel);
              
        }
            
        // Level Set
        currentLevel.Set<RCP<SmootherBase> >("PostSmoother", postSmoother, this);
        currentLevel.Set<RCP<SmootherBase> >("PostSmoother", postSmoother); // TODO: remove this
      }
        
  } // Build()
    
  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  std::string SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::description() const {
    std::ostringstream out;
    out << SmootherFactoryBase::description();
    std::string preStr  = (preSmootherPrototype_ == Teuchos::null) ? "null" : preSmootherPrototype_->description();
    std::string postStr = (preSmootherPrototype_ == postSmootherPrototype_) ? "pre" : ( (postSmootherPrototype_ == Teuchos::null) ? "null" : preSmootherPrototype_->description() );
    out << "{pre = "  << preStr << ", post = "<< postStr << "}";
    return out.str();
  }
    
  template <class Scalar,class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  void SmootherFactory<Scalar, LocalOrdinal, GlobalOrdinal, Node, LocalMatOps>::describe(Teuchos::FancyOStream &out, const VerbLevel verbLevel) const {
    MUELU_DESCRIBE;

    if (verbLevel & Parameters0) {
      out0 << "PreSmoother : "; if (preSmootherPrototype_  == Teuchos::null) { out0 << "null" << std::endl; } else { Teuchos::OSTab tab2(out); preSmootherPrototype_->describe(out, verbLevel); }

      out0 << "PostSmoother: ";
      if      (postSmootherPrototype_ == preSmootherPrototype_) { out0 << "same as PreSmoother" << std::endl; } 
      else if (postSmootherPrototype_ == Teuchos::null)         { out0 << "null" << std::endl; }
      else { 
        { Teuchos::OSTab tab2(out); postSmootherPrototype_->describe(out, verbLevel); }
        out0 << "PostSmoother is different than PreSmoother (not the same object)" << std::endl;
      }
    }
      
    if (verbLevel & Debug) {
      if (preSmootherPrototype_  != Teuchos::null || postSmootherPrototype_ != Teuchos::null) { out0 << "-" << std::endl; }
      if (preSmootherPrototype_  != Teuchos::null) { out0 << "RCP<preSmootherPrototype_> : " << preSmootherPrototype_  << std::endl; }
      if (postSmootherPrototype_ != Teuchos::null) { out0 << "RCP<postSmootherPrototype_>: " << postSmootherPrototype_ << std::endl; }
    }
  }


} // namespace MueLu

//TODO: doc: setup done twice if PostSmoother object != PreSmoother object and no adv. reused capability
#endif // MUELU_SMOOTHERFACTORY_DEF_HPP
