#include "PB_BlockPreconditionerFactory.hpp"

#include "PB_InverseLibrary.hpp"
#include "PB_JacobiPreconditionerFactory.hpp"
#include "PB_GaussSeidelPreconditionerFactory.hpp"
#include "PB_AddPreconditionerFactory.hpp"
#include "PB_MultPreconditionerFactory.hpp"
#include "NS/PB_LSCPreconditionerFactory.hpp"
#include "NS/PB_SIMPLEPreconditionerFactory.hpp"

#include "Thyra_DefaultPreconditioner.hpp"

using namespace Thyra;

namespace PB {

/////////////////////////////////////////////////////

//! Set parameters from a parameter list and return with default values.
void BlockPreconditionerState::setParameterList(const RCP<ParameterList> & paramList)
{
   paramList_ = paramList;
}

//! Get the parameter list that was set using setParameterList().
RCP<ParameterList> BlockPreconditionerState::getNonconstParameterList()
{
   return paramList_;
}

//! Unset the parameter list that was set using setParameterList(). 
RCP<ParameterList> BlockPreconditionerState::unsetParameterList()
{
   RCP<ParameterList> paramList = paramList_;
   paramList_ = Teuchos::null;
   return paramList;
}

/////////////////////////////////////////////////////

LinearOp BlockPreconditionerFactory::buildPreconditionerOperator(LinearOp & lo,BlockPreconditionerState & state) const
{
   // get the blocked linear operator
   RCP<LinearOpBase<double> > loA = Teuchos::rcp_const_cast<Thyra::LinearOpBase<double> >(lo);
   BlockedLinearOp A = Teuchos::rcp_dynamic_cast<Thyra::PhysicallyBlockedLinearOpBase<double> >(loA);

   state.setInitialized(false);

   return buildPreconditionerOperator(A,state);
}

//! is this operator compatiable with the preconditioner factory?
bool BlockPreconditionerFactory::isCompatible(const Thyra::LinearOpSourceBase<double> &fwdOpSrc) const
{
   RCP<const Thyra::PhysicallyBlockedLinearOpBase<double> > A 
         = Teuchos::rcp_dynamic_cast<const Thyra::PhysicallyBlockedLinearOpBase<double> >(fwdOpSrc.getOp());
   return A!=Teuchos::null;
}

//! create an instance of the preconditioner
RCP<Thyra::PreconditionerBase<double> > BlockPreconditionerFactory::createPrec() const
{
   // build a preconditioner, give it some inital state
   RCP<BlockPreconditioner> bp = rcp(new BlockPreconditioner());
   bp->setStateObject(buildPreconditionerState());
   bp->getStateObject()->setInitialized(false);

   return bp;
}

//! initialize a newly created preconditioner object
void BlockPreconditionerFactory::initializePrec(const RCP<const LinearOpSourceBase<double> > & ASrc,
                    PreconditionerBase<double> * prec,
                    const ESupportSolveUse supportSolveUse) const
{
   // get the blocked linear operator
   RCP<LinearOpBase<double> > loA = Teuchos::rcp_const_cast<Thyra::LinearOpBase<double> >(ASrc->getOp());
   BlockedLinearOp A = Teuchos::rcp_dynamic_cast<Thyra::PhysicallyBlockedLinearOpBase<double> >(loA);

   BlockPreconditioner * blkPrec = dynamic_cast<BlockPreconditioner *>(prec);
   TEUCHOS_ASSERT(blkPrec!=0);
 
   // grab the state object
   RCP<BlockPreconditionerState> state = blkPrec->getStateObject();
   state->setInitialized(false);

   // build the preconditioner
   const RCP<const LinearOpBase<double> > M = buildPreconditionerOperator(A,*state);

   // must first cast that to be initialized
   DefaultPreconditioner<double> & dPrec = Teuchos::dyn_cast<DefaultPreconditioner<double> >(*prec);
   dPrec.initializeUnspecified(Teuchos::rcp_const_cast<LinearOpBase<double> >(M));
}

//! initialize a newly created preconditioner object
void BlockPreconditionerFactory::initializePrec(const RCP<const LinearOpSourceBase<double> > & ASrc,
                                                const RCP<const Thyra::MultiVectorBase<double> > & solnVec,
                                                PreconditionerBase<double> * prec,
                                                const ESupportSolveUse supportSolveUse) const
{
   BlockPreconditioner * blkPrec = dynamic_cast<BlockPreconditioner *>(prec);
   blkPrec->setSourceVector(Teuchos::rcp_const_cast<Thyra::MultiVectorBase<double> >(solnVec));

   initializePrec(ASrc,prec,supportSolveUse);
}

//! wipe clean a already initialized preconditioner object
void BlockPreconditionerFactory::uninitializePrec(PreconditionerBase<double> * prec, 
                      RCP<const LinearOpSourceBase<double> > * fwdOpSrc,
                      ESupportSolveUse *supportSolveUse) const
{
   BlockPreconditioner * blkPrec = dynamic_cast<BlockPreconditioner *>(prec);

   // what do I do here?
   TEST_FOR_EXCEPT_MSG(true,"\"BlockPreconditionerFactory::uninitializePrec not implemented\"");
}

// for ParameterListAcceptor
///////////////////////////////////////////////////////////////////////

//! Set parameters from a parameter list and return with default values.
void BlockPreconditionerFactory::setParameterList(const RCP<ParameterList> & paramList)
{
   paramList_ = paramList;
}

//! Get the parameter list that was set using setParameterList().
RCP< ParameterList > BlockPreconditionerFactory::getNonconstParameterList()
{
   return paramList_;
}

//! Unset the parameter list that was set using setParameterList(). 
RCP< ParameterList > BlockPreconditionerFactory::unsetParameterList()
{
   RCP<ParameterList> _paramList = paramList_;
   paramList_ = Teuchos::null;
   return _paramList;
}

//! Set the inverse library used by this preconditioner factory
void BlockPreconditionerFactory::setInverseLibrary(const RCP<const InverseLibrary> & il)
{
   inverseLibrary_ = il;
}

//! Get the inverse library used by this preconditioner factory
RCP<const InverseLibrary> BlockPreconditionerFactory::getInverseLibrary() const
{
   // lazily build the inverse library only when needed
   if(inverseLibrary_==Teuchos::null)
      return InverseLibrary::buildFromStratimikos();

   return inverseLibrary_;
}

RCP<BlockPreconditionerFactory> 
BlockPreconditionerFactory::buildPreconditionerFactory(const std::string & name,
                                                       const Teuchos::ParameterList & settings,
                                                       const RCP<const InverseLibrary> & invLib)
{
   RCP<BlockPreconditionerFactory> precFact;

   // build various preconditioners
   if(name=="Block Jacobi")
      precFact = rcp(new JacobiPreconditionerFactory());
   else if(name=="Block Gauss Seidel")
      precFact = rcp(new GaussSeidelPreconditionerFactory());
   else if(name=="Block Add")
      precFact = rcp(new AddPreconditionerFactory());
   else if(name=="Block Multiply")
      precFact = rcp(new MultPreconditionerFactory());
   else if(name=="NS LSC")
      precFact = rcp(new NS::LSCPreconditionerFactory());
   else if(name=="NS SIMPLE")
      precFact = rcp(new NS::SIMPLEPreconditionerFactory());

   TEUCHOS_ASSERT(precFact!=Teuchos::null);

   // add in the inverse library
   if(invLib!=Teuchos::null)
      precFact->setInverseLibrary(invLib);

   // now that inverse library has been set,
   // pass in the parameter list
   precFact->initializeFromParameterList(settings);

   return precFact;
}

} // end namespace PB
