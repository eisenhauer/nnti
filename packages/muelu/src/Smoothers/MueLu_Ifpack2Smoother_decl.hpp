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
#ifndef MUELU_IFPACK2SMOOTHER_DECL_HPP
#define MUELU_IFPACK2SMOOTHER_DECL_HPP

#include <Teuchos_ParameterList.hpp>
#include <Xpetra_Matrix_fwd.hpp>
#include <Xpetra_Matrix.hpp>
#include <Xpetra_CrsMatrixWrap.hpp>
#include <Xpetra_MultiVectorFactory_fwd.hpp>
#ifdef HAVE_XPETRA_TPETRA // needed for clone()
#include <Xpetra_TpetraCrsMatrix.hpp>
#endif

#include "MueLu_ConfigDefs.hpp"
#include "MueLu_Ifpack2Smoother_fwd.hpp"

#if defined(HAVE_MUELU_TPETRA) && defined(HAVE_MUELU_IFPACK2)

#include <Ifpack2_Preconditioner.hpp>
#include <Ifpack2_Factory_decl.hpp>
#include <Ifpack2_Factory_def.hpp>
#include <Tpetra_CrsMatrix.hpp>

#include "MueLu_SmootherPrototype.hpp"
#include "MueLu_Level_fwd.hpp"
#include "MueLu_FactoryBase_fwd.hpp"
#include "MueLu_Utilities_fwd.hpp"
namespace MueLu {

  /*!
    @class Ifpack2Smoother
    @brief Class that encapsulates Ifpack2 smoothers.

    //   This class creates an Ifpack2 preconditioner factory. The factory creates a smoother based on the
    //   type and ParameterList passed into the constructor. See the constructor for more information.
    */

  template <class Scalar = double, class LocalOrdinal = int, class GlobalOrdinal = LocalOrdinal, class Node = KokkosClassic::DefaultNode::DefaultNodeType, class LocalMatOps = typename KokkosClassic::DefaultKernels<void,LocalOrdinal,Node>::SparseOps> //TODO: or BlockSparseOp ?
  class Ifpack2Smoother : public SmootherPrototype<Scalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>
  {
#undef MUELU_IFPACK2SMOOTHER_SHORT
#include "MueLu_UseShortNames.hpp"

  public:

    //! @name Constructors / destructors
    //@{
    //TODO: update doc for Ifpack2. Right now, it's a copy of the doc of IfpackSmoother
    /*! @brief Constructor

    The options passed into Ifpack2Smoother are those given in the Ifpack2 user's manual.

    @param type smoother type
    @param list options for the particular smoother (e.g., fill factor or damping parameter)

    Here is how to select some of the most common smoothers.

    - Gauss-Seidel
      - <tt>type</tt> = <tt>point relaxation stand-alone</tt>
      - parameter list options
        - <tt>relaxation: type</tt> = <tt>Gauss-Seidel</tt>
        - <tt>relaxation: damping factor</tt>
    - symmetric Gauss-Seidel
      - <tt>type</tt> = <tt>point relaxation stand-alone</tt>
      - parameter list options
        - <tt>relaxation: type</tt> = <tt>symmetric Gauss-Seidel</tt>
        - <tt>relaxation: damping factor</tt>
    - Chebyshev
      - <tt>type</tt> = <tt>Chebyshev</tt>
      - parameter list options
        - <tt>chebyshev: ratio eigenvalue</tt>
        - <tt>chebyshev: min eigenvalue</tt>
        - <tt>chebyshev: max eigenvalue</tt>
        - <tt>chebyshev: degree</tt>
        - <tt>chebyshev: zero starting solution</tt> (defaults to <tt>true</tt>)
    - ILU
      - <tt>type</tt> = <tt>ILU</tt>
      - parameter list options
        - <tt>fact: level-of-fill</tt>

    See also Ifpack2::Relaxation, Ifpack2::Chebyshev, Ifpack2::ILUT, Ifpack2::Krylov.
    */

    template<class Scalar2, class LocalOrdinal2, class GlobalOrdinal2, class Node2, class LocalMatOps2>
    friend class Ifpack2Smoother;

    Ifpack2Smoother(std::string const & type, Teuchos::ParameterList const & paramList = Teuchos::ParameterList(), LO const &overlap=0); //TODO: empty paramList valid for Ifpack??

    //! Destructor
    virtual ~Ifpack2Smoother() { }

    //@}

    void SetParameterList(const Teuchos::ParameterList& paramList);

    //! Input
    //@{

    void DeclareInput(Level &currentLevel) const;

    //@}

    //! @name Computational methods.
    //@{

    /*! @brief Set up the smoother.

    This creates the underlying Ifpack2 smoother object, copies any parameter list options
    supplied to the constructor to the Ifpack2 object, and computes the preconditioner.

    TODO The eigenvalue estimate should come from A_, not the Ifpack2 parameter list.
    */
    void Setup(Level &currentLevel);

    /*! @brief Apply the preconditioner.

    Solves the linear system <tt>AX=B</tt> using the constructed smoother.

    @param X initial guess
    @param B right-hand side
    @param InitialGuessIsZero (optional) If false, some work can be avoided. Whether this actually saves any work depends on the underlying Ifpack2 implementation.
    */
    void Apply(MultiVector& X, const MultiVector& B, bool InitialGuessIsZero = false) const;

    //@}

    //! @name Utilities
    //@{

    RCP<SmootherPrototype> Copy() const;

    //@}

    //! Clone the smoother to a different node type
    template<typename Node2, typename LocalMatOps2>
    RCP<MueLu::Ifpack2Smoother<Scalar,LocalOrdinal,GlobalOrdinal,Node2,LocalMatOps2> > clone(const RCP<Node2>& node2, const Teuchos::RCP<const Xpetra::Matrix<Scalar,LocalOrdinal,GlobalOrdinal,Node2,LocalMatOps2> >& A_newnode) const;

    //! @name Overridden from Teuchos::Describable
    //@{

    //! Return a simple one-line description of this object.
    std::string description() const;

    //! Print the object with some verbosity level to an FancyOStream object.
    //using MueLu::Describable::describe; // overloading, not hiding
    //void describe(Teuchos::FancyOStream &out, const VerbLevel verbLevel = Default) const
    void print(Teuchos::FancyOStream &out, const VerbLevel verbLevel = Default) const;

    //@}

  private:
    void SetPrecParameters(const Teuchos::ParameterList& list = Teuchos::ParameterList()) const;

  private:

    //! ifpack2-specific key phrase that denote smoother type
    std::string type_;

    //! overlap when using the smoother in additive Schwarz mode
    LO overlap_;

    //! pointer to Ifpack2 preconditioner object
    RCP<Ifpack2::Preconditioner<Scalar,LocalOrdinal,GlobalOrdinal,Node> > prec_;

    //! matrix, used in apply if solving residual equation
    RCP<Matrix> A_;

  }; // class Ifpack2Smoother

  template<class Scalar, class LocalOrdinal, class GlobalOrdinal, class Node, class LocalMatOps>
  template<typename Node2, typename LocalMatOps2>
  RCP<MueLu::Ifpack2Smoother<Scalar,LocalOrdinal,GlobalOrdinal,Node2,LocalMatOps2> >
  Ifpack2Smoother<Scalar,LocalOrdinal,GlobalOrdinal,Node,LocalMatOps>::clone(const RCP<Node2>& node2, const RCP<const Xpetra::Matrix<Scalar,LocalOrdinal,GlobalOrdinal,Node2, LocalMatOps2> >& A_newnode) const {
#ifdef HAVE_XPETRA_TPETRA
    const ParameterList& paramList = this->GetParameterList();
    typedef Tpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node> Matrix1;
    typedef Tpetra::CrsMatrix<Scalar,LocalOrdinal,GlobalOrdinal,Node2> Matrix2;
    RCP<Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node2> > cloneSmoother =
        rcp(new Ifpack2Smoother<Scalar, LocalOrdinal, GlobalOrdinal, Node2>(type_, paramList, overlap_));

    //Get Tpetra::CrsMatrix from Xpetra::Matrix
    RCP<const Xpetra::CrsMatrixWrap<Scalar, LocalOrdinal, GlobalOrdinal, Node2, LocalMatOps2> > crsOp =
        rcp_dynamic_cast<const Xpetra::CrsMatrixWrap<Scalar, LocalOrdinal, GlobalOrdinal, Node2, LocalMatOps2> >(A_newnode);
    const RCP<const Xpetra::TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node2, LocalMatOps2> >& tmp =
        rcp_dynamic_cast<const Xpetra::TpetraCrsMatrix<Scalar, LocalOrdinal, GlobalOrdinal, Node2, LocalMatOps2> >(crsOp->getCrsMatrix());

    Ifpack2::Factory factory;
    cloneSmoother->prec_ = factory.clone<Matrix1, Matrix2>(prec_, tmp->getTpetra_CrsMatrix(), paramList);
    cloneSmoother->type_ = type_;
    cloneSmoother->SetParameterList(paramList);
    cloneSmoother->IsSetup(this->IsSetup());
    return cloneSmoother;
#else
    TEUCHOS_TEST_FOR_EXCEPTION(true, Exceptions::RuntimeError,
        "MueLu::Ifpack2Smoother::clone(): clone only available with Tpetra.");
#endif
  }


} // namespace MueLu

#define MUELU_IFPACK2SMOOTHER_SHORT
#endif // HAVE_MUELU_TPETRA && HAVE_MUELU_IFPACK2
#endif // MUELU_IFPACK2SMOOTHER_DECL_HPP
