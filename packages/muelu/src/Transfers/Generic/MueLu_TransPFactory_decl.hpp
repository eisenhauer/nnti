#ifndef MUELU_TRANSPFACTORY_DECL_HPP
#define MUELU_TRANSPFACTORY_DECL_HPP

#include "MueLu_ConfigDefs.hpp"

#include <iostream>

#include "Xpetra_CrsOperator.hpp"

#include "MueLu_Level.hpp"
#include "MueLu_RFactory.hpp"
#include "MueLu_Exceptions.hpp"
#include "MueLu_Utilities.hpp"

namespace MueLu {

  /*!
    @class TransPFactory class.
    @brief Factory for building restriction operators.

    This factory currently depends on an underlying matrix-matrix multiply with the identity
    matrix to do the transpose.  This should probably be fixed at some point.
  */

template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps> //TODO: or BlockSparseOp ?
  class TransPFactory : public RFactory {

#include "MueLu_UseShortNames.hpp"

  public:
    //! @name Constructors/Destructors.
    //@{

    //! Constructor.
    TransPFactory(RCP<FactoryBase> PFact = Teuchos::null)
    ;

    //! Destructor.
    virtual ~TransPFactory() ;
    //@}

    //! Input
    //@{

    void DeclareInput(Level &fineLevel, Level &coarseLevel) const ;

    //@}

    //! @name Build methods.
    //@{
/*
   FIXME this uses the Tpetra RowMatrixTransposer.  This has revealed a bug somewhere.
   FIXME so disabling it right now.
    void BuildR(Level & fineLevel, Level & coarseLevel) ; //BuildR
*/

    void Build(Level & fineLevel, Level & coarseLevel) const ;

    void BuildR(Level & fineLevel, Level & coarseLevel) const ; //BuildR

    //@}

    //! @name Set methods.
    //@{
    void UsePtent(bool ToF) ;
    //@}

    //! P Factory
    RCP<FactoryBase> PFact_;

  }; //class TransPFactory

} //namespace MueLu

#define MUELU_TRANSPFACTORY_SHORT
#endif // MUELU_TRANSPFACTORY_DECL_HPP
