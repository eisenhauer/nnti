#ifndef MUELU_SAPFACTORY_DECL_HPP
#define MUELU_SAPFACTORY_DECL_HPP

#include <string>

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_PFactory.hpp"
#include "MueLu_SaPFactory_fwd.hpp"

#include "MueLu_Level_fwd.hpp"
#include "MueLu_SingleLevelFactoryBase_fwd.hpp"
#include "MueLu_TentativePFactory_fwd.hpp"
#include "MueLu_Utilities_fwd.hpp"

namespace MueLu {

  /*!
    @class SaPFactory class.
    @brief Factory for building Smoothed Aggregation prolongators.
    @ingroup MueLuTransferClasses
  */

  template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps> //TODO: or BlockSparseOp ?
  class SaPFactory : public PFactory {
#undef MUELU_SAPFACTORY_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors/Destructors.
    //@{
  
    /*! @brief Constructor.
      User can supply a factory for generating the tentative prolongator.
    */
    SaPFactory(RCP<const FactoryBase> InitialPFact = Teuchos::null, RCP<const FactoryBase> AFact = Teuchos::null);
  
    //! Destructor.
    virtual ~SaPFactory();
  
    //@}

    //! @name Set methods.
    //@{

    //! Set prolongator smoother damping factor.
    void SetDampingFactor(Scalar dampingFactor);

    //! Change view of diagonal.
    void SetDiagonalView(std::string const& diagView);
    //@}

    //! @name Get methods.
    //@{

    //! Returns prolongator smoother damping factor.
    Scalar GetDampingFactor();

    //! Returns current view of diagonal.
    std::string GetDiagonalView();

    //@}

    //! Input
    //@{

    void DeclareInput(Level &fineLevel, Level &coarseLevel) const;

    //@}

    //! @name Build methods.
    //@{
  
    /*!
      @brief Build method.

      Builds smoothed aggregation prolongator and returns it in <tt>coarseLevel</tt>.
      //FIXME what does the return code mean (unclear in MueMat)?
      */
    void Build(Level& fineLevel, Level &coarseLevel) const;

    void BuildP(Level &fineLevel, Level &coarseLevel) const; //Build()

    //@}

    /*
    //TODO
    function [this] = SaPFactory(CoalesceFact,AggFact, diagonalView) //copy ctor
    function SetDiagonalView(this, diagonalView)
    */


  private:

    //! Input factories
    RCP<const FactoryBase> initialPFact_; //! Ptentative Factory
    RCP<const FactoryBase> AFact_;        //! A Factory
    
    //! Factory parameters
    Scalar dampingFactor_;
    std::string diagonalView_;

  }; //class SaPFactory

} //namespace MueLu

#define MUELU_SAPFACTORY_SHORT
#endif // MUELU_SAPFACTORY_DECL_HPP
