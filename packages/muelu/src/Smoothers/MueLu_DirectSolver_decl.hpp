#ifndef MUELU_DIRECTSOLVER_DECL_HPP
#define MUELU_DIRECTSOLVER_DECL_HPP

#include "MueLu_ConfigDefs.hpp"

#include "MueLu_SmootherPrototype.hpp"

#include "MueLu_AmesosSmoother.hpp"
#include "MueLu_Amesos2Smoother.hpp"

// Note: DirectSolver is a SmootherPrototype that cannot be turned into a smoother using Setup().
//       When this prototype is cloned using Copy(), the clone is an Amesos or an Amesos2 smoother.
//       The clone can be used as a smoother after calling Setup().

namespace MueLu {

  /*!
    @class DirectSolver
    @brief Class that encapsulates direct solvers. Autoselection of AmesosSmoother or Amesos2Smoother according to the compile time configuration of Trilinos
  */

  template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps> //TODO: or BlockSparseOp ?
  class DirectSolver : public SmootherPrototype<Scalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>
  {
    
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors / destructors
    //@{

    //! @brief Constructor
    //! Note: only parameters shared by Amesos and Amesos2 should be used for type and paramList (example: type= "Klu", "Superlu", paramList = <empty>) .
    DirectSolver(const Xpetra::UnderlyingLib lib, std::string const & type = "", Teuchos::ParameterList const & paramList = Teuchos::ParameterList(), RCP<FactoryBase> AFact = Teuchos::null)
    ;
    
    //! Destructor
    virtual ~DirectSolver() ;
    
    //@}

    //! Input
    //@{

    void DeclareInput(Level &currentLevel) const ;

    //@}

    //! @name Setup and Apply methods.
    //@{

    //! DirectSolver cannot be turned into a smoother using Setup(). Setup() always returns a RuntimeError exception.
    void Setup(Level &currentLevel) ;

    //! DirectSolver cannot be applied. Apply() always returns a RuntimeError exception.
    void Apply(MultiVector &X, MultiVector const &B, bool const &InitialGuessIsZero=false) const
    ;

    //@}

    //! When this prototype is cloned using Copy(), the clone is an Amesos or an Amesos2 smoother.
    RCP<SmootherPrototype> Copy() const ;

    //! @name Overridden from Teuchos::Describable 
    //@{
    
    //! Return a simple one-line description of this object.
    std::string description() const ;
    
    void print(Teuchos::FancyOStream &out, const VerbLevel verbLevel = Default) const ;

    //@}

  private:
    //! Tpetra or Epetra?
    Xpetra::UnderlyingLib lib_;

    //! amesos1/2-specific key phrase that denote smoother type
    std::string type_;
    
    //! parameter list that is used by Amesos internally
    Teuchos::ParameterList paramList_;

    //! A Factory
    RCP<FactoryBase> AFact_;

  }; // class DirectSolver

} // namespace MueLu

#define MUELU_DIRECT_SOLVER_SHORT
#endif // MUELU_DIRECTSOLVER_DECL_HPP
