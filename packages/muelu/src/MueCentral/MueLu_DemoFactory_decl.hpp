#ifndef MUELU_DEMOFACTORY_DECL_HPP
#define MUELU_DEMOFACTORY_DECL_HPP

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_SingleLevelFactoryBase.hpp"
#include "MueLu_DemoFactory_fwd.hpp"

namespace MueLu {

  /*!
    @class DemoFactory class.
    @brief empty factory for demonstration
  */

  template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = Kokkos::DefaultNode::DefaultNodeType, class LocalMatOps = typename Kokkos::DefaultKernels<void,LocalOrdinal,Node>::SparseOps>
  class DemoFactory : public SingleLevelFactoryBase {
#undef MUELU_DEMOFACTORY_SHORT
#include "MueLu_UseShortNames.hpp"

  public:
    //! @name Constructors/Destructors.
    //@{

    //! Constructor.
    DemoFactory();

    //! Destructor.
    virtual ~DemoFactory();

    //@}

    //! @name Input
    //@{

    /*! @brief Specifies the data that this class needs, and the factories that generate that data.

        If the Build method of this class requires some data, but the generating factory is not specified in DeclareInput, then this class
        will fall back to the settings in FactoryManager.
    */
    void DeclareInput(Level &currentLevel) const;

    //@}

    //! @name Build methods.
    //@{

    //! Build an object with this factory.
    void Build(Level & currentLevel) const;

    //@}

  private:
    // TODO add member variables

  }; // class DemoFactory

} // namespace MueLu

#define MUELU_DEMOFACTORY_SHORT
#endif // MUELU_DEMOFACTORY_DECL_HPP
