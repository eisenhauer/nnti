#ifndef __PB_LSCStrategy_hpp__
#define __PB_LSCStrategy_hpp__

#include "Teuchos_RCP.hpp"

#include "Thyra_LinearOpBase.hpp"

#include "PB_Utilities.hpp"
#include "PB_InverseFactory.hpp"
#include "PB_BlockPreconditionerFactory.hpp"

namespace PB {
namespace NS {

class LSCPrecondState; // forward declaration

// simple strategy for driving LSCPreconditionerFactory
class LSCStrategy {
public:
   /** This informs the strategy object to build the state associated
     * with this operator.
     *
     * \param[in] A The linear operator to be preconditioned by LSC.
     * \param[in] state State object for storying reusable information about
     *                  the operator A.
     */
   virtual void buildState(BlockedLinearOp & A,BlockPreconditionerState & state) const = 0;

   /** Get the inverse of \f$B Q_u^{-1} B^T\f$. 
     *
     * \param[in] A The linear operator to be preconditioned by LSC.
     * \param[in] state State object for storying reusable information about
     *                  the operator A.
     *
     * \returns An (approximate) inverse of \f$B Q_u^{-1} B^T\f$.
     */
   virtual LinearOp getInvBQBt(const BlockedLinearOp & A,BlockPreconditionerState & state) const = 0;

   /** Get the inverse of the \f$F\f$ block.
     *
     * \param[in] A The linear operator to be preconditioned by LSC.
     * \param[in] state State object for storying reusable information about
     *                  the operator A.
     *
     * \returns An (approximate) inverse of \f$F\f$.
     */
   virtual LinearOp getInvF(const BlockedLinearOp & A,BlockPreconditionerState & state) const = 0;

   /** Get the inverse for stabilizing the whole schur complement approximation.
     *
     * \param[in] A The linear operator to be preconditioned by LSC.
     * \param[in] state State object for storying reusable information about
     *                  the operator A.
     *
     * \returns The operator to stabilize the whole Schur complement.
     */
   virtual LinearOp getInvD(const BlockedLinearOp & A,BlockPreconditionerState & state) const = 0;

   /** Get the inverse mass matrix.
     *
     * \param[in] A The linear operator to be preconditioned by LSC.
     * \param[in] state State object for storying reusable information about
     *                  the operator A.
     *
     * \returns The inverse of the mass matrix \f$Q_u\f$.
     */
   virtual LinearOp getInvMass(const BlockedLinearOp & A,BlockPreconditionerState & state) const = 0;

   /** Should the approximation of the inverse use a full LDU decomposition, or
     * is a upper triangular approximation sufficient.
     *
     * \returns True if the full LDU decomposition should be used, otherwise
     *          only an upper triangular version is used.
     */
   virtual bool useFullLDU() const = 0;

   //! Initialize from a parameter list
   virtual void initializeFromParameterList(const Teuchos::ParameterList & pl,const InverseLibrary & invLib) {}

   //! For assiting in construction of the preconditioner
   virtual Teuchos::RCP<Teuchos::ParameterList> getRequestedParameters() const {}

   //! For assiting in construction of the preconditioner
   virtual bool updateRequestedParameters(const Teuchos::ParameterList & pl) {}
};

} // end namespace NS
} // end namespace PB

#endif
