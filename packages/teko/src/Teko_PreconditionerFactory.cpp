#include "Teko_PreconditionerFactory.hpp"

#include "Teko_InverseLibrary.hpp"
#include "Teko_Preconditioner.hpp"

// Specific preconditioners included for dynamic creation
#include "Teko_JacobiPreconditionerFactory.hpp"
#include "Teko_GaussSeidelPreconditionerFactory.hpp"
#include "Teko_AddPreconditionerFactory.hpp"
#include "Teko_MultPreconditionerFactory.hpp"
#include "Teko_LU2x2PreconditionerFactory.hpp"
#include "NS/Teko_LSCPreconditionerFactory.hpp"
#include "NS/Teko_SIMPLEPreconditionerFactory.hpp"


#include "Thyra_DefaultPreconditioner.hpp"

using namespace Thyra;
using Teuchos::RCP;

namespace Teko {
/////////////////////////////////////////////////////

//! is this operator compatiable with the preconditioner factory?
bool PreconditionerFactory::isCompatible(const Thyra::LinearOpSourceBase<double> &fwdOpSrc) const
{
   RCP<const Thyra::LinearOpBase<double> > A = fwdOpSrc.getOp();
   return A!=Teuchos::null;
}

//! create an instance of the preconditioner
RCP<Thyra::PreconditionerBase<double> > PreconditionerFactory::createPrec() const
{
   // build a preconditioner, give it some inital state
   RCP<Preconditioner> bp = rcp(new Preconditioner());
   bp->setStateObject(buildPreconditionerState());
   bp->getStateObject()->setInitialized(false);

   return bp;
}

//! initialize a newly created preconditioner object
void PreconditionerFactory::initializePrec(const RCP<const LinearOpSourceBase<double> > & ASrc,
                    PreconditionerBase<double> * prec,
                    const ESupportSolveUse supportSolveUse) const
{
   // get the blocked linear operator
   LinearOp A = Teuchos::rcp_const_cast<Thyra::LinearOpBase<double> >(ASrc->getOp());

   Preconditioner * blkPrec = dynamic_cast<Preconditioner *>(prec);
   TEUCHOS_ASSERT(blkPrec!=0);
 
   // grab the state object
   RCP<PreconditionerState> state = blkPrec->getStateObject();
   state->setInitialized(false);

   // build the preconditioner
   const RCP<const LinearOpBase<double> > M = buildPreconditionerOperator(A,*state);

   // must first cast that to be initialized
   DefaultPreconditioner<double> & dPrec = Teuchos::dyn_cast<DefaultPreconditioner<double> >(*prec);
   dPrec.initializeUnspecified(Teuchos::rcp_const_cast<LinearOpBase<double> >(M));
}

//! initialize a newly created preconditioner object
void PreconditionerFactory::initializePrec(const RCP<const LinearOpSourceBase<double> > & ASrc,
                                                const RCP<const Thyra::MultiVectorBase<double> > & solnVec,
                                                PreconditionerBase<double> * prec,
                                                const ESupportSolveUse supportSolveUse) const
{
   Preconditioner * blkPrec = dynamic_cast<Preconditioner *>(prec);
   blkPrec->setSourceVector(Teuchos::rcp_const_cast<Thyra::MultiVectorBase<double> >(solnVec));

   initializePrec(ASrc,prec,supportSolveUse);
}

//! wipe clean a already initialized preconditioner object
void PreconditionerFactory::uninitializePrec(PreconditionerBase<double> * prec, 
                      RCP<const LinearOpSourceBase<double> > * fwdOpSrc,
                      ESupportSolveUse *supportSolveUse) const
{
   // Preconditioner * blkPrec = dynamic_cast<Preconditioner *>(prec);

   // what do I do here?
   TEST_FOR_EXCEPT_MSG(true,"\"PreconditionerFactory::uninitializePrec not implemented\"");
}

// for ParameterListAcceptor
///////////////////////////////////////////////////////////////////////

//! Set parameters from a parameter list and return with default values.
void PreconditionerFactory::setParameterList(const RCP<Teuchos::ParameterList> & paramList)
{
   paramList_ = paramList;
}

//! Get the parameter list that was set using setParameterList().
RCP< Teuchos::ParameterList > PreconditionerFactory::getNonconstParameterList()
{
   return paramList_;
}

//! Unset the parameter list that was set using setParameterList(). 
RCP< Teuchos::ParameterList > PreconditionerFactory::unsetParameterList()
{
   RCP<Teuchos::ParameterList> _paramList = paramList_;
   paramList_ = Teuchos::null;
   return _paramList;
}

//! Set the inverse library used by this preconditioner factory
void PreconditionerFactory::setInverseLibrary(const RCP<const InverseLibrary> & il)
{
   inverseLibrary_ = il;
}

//! Get the inverse library used by this preconditioner factory
RCP<const InverseLibrary> PreconditionerFactory::getInverseLibrary() const
{
   // lazily build the inverse library only when needed
   if(inverseLibrary_==Teuchos::null)
      return InverseLibrary::buildFromStratimikos();

   return inverseLibrary_;
}

/////////////////////////////////////////////////////
// Static members and methods
/////////////////////////////////////////////////////

//! for creating the preconditioner factories objects
CloneFactory<PreconditionerFactory> PreconditionerFactory::precFactoryBuilder_;

/** \brief Builder function for creating preconditioner factories (yes
  *        this is a factory factory.
  *
  * Builder function for creating preconditioner factories (yes
  * this is a factory factory.
  * 
  * \param[in] name     String name of factory to build
  * \param[in] settings Parameter list describing the parameters for the
  *                     factory to build
  * \param[in] invLib   Inverse library for the factory to use.
  *
  * \returns If the name is associated with a preconditioner
  *          a pointer is returned, otherwise Teuchos::null is returned.
  */
RCP<PreconditionerFactory> 
PreconditionerFactory::buildPreconditionerFactory(const std::string & name,
                                                       const Teuchos::ParameterList & settings,
                                                       const RCP<const InverseLibrary> & invLib)
{
   Teko_DEBUG_SCOPE("PreconditionerFactory::buildPreconditionerFactory",10);

   // initialize the defaults if necessary
   if(precFactoryBuilder_.cloneCount()==0) initializePrecFactoryBuilder();

   // request the preconditioner factory from the CloneFactory
   RCP<PreconditionerFactory> precFact = precFactoryBuilder_.build(name);

   Teko_DEBUG_MSG_BEGIN(5);
      DEBUG_STREAM << "Looked up \"" << name << "\"" << std::endl;
      DEBUG_STREAM << "Built " << precFact << std::endl;
   Teko_DEBUG_MSG_END();

   if(precFact==Teuchos::null)  
      return Teuchos::null;

   // add in the inverse library
   if(invLib!=Teuchos::null)
      precFact->setInverseLibrary(invLib);

   // now that inverse library has been set,
   // pass in the parameter list
   precFact->initializeFromParameterList(settings);

   return precFact;
}

/** \brief Add a preconditioner factory to the builder. This is done using the
  *        clone pattern. 
  *
  * Add a preconditioner factory to the builder. This is done using the
  * clone pattern. If your class does not support the Cloneable interface then
  * you can use the AutoClone class to construct your object.
  *
  * \note If this method is called twice with the same string, the latter clone pointer
  *       will be used.
  *
  * \param[in] name String to associate with this object
  * \param[in] clone Pointer to Cloneable object
  */
void PreconditionerFactory::addPreconditionerFactory(const std::string & name,const RCP<Cloneable> & clone)
{
   // initialize the defaults if necessary
   if(precFactoryBuilder_.cloneCount()==0) initializePrecFactoryBuilder();

   // add clone to builder
   precFactoryBuilder_.addClone(name,clone); 
}

//! This is where the default objects are put into the precFactoryBuilder_
void PreconditionerFactory::initializePrecFactoryBuilder()
{
   RCP<Cloneable> clone;

   // add various preconditioners to factory
   clone = rcp(new AutoClone<LU2x2PreconditionerFactory>());
   precFactoryBuilder_.addClone("Block LU2x2",clone);

   clone = rcp(new AutoClone<JacobiPreconditionerFactory>());
   precFactoryBuilder_.addClone("Block Jacobi",clone);

   clone = rcp(new AutoClone<GaussSeidelPreconditionerFactory>());
   precFactoryBuilder_.addClone("Block Gauss-Seidel",clone);

   clone = rcp(new AutoClone<AddPreconditionerFactory>());
   precFactoryBuilder_.addClone("Block Add",clone);

   clone = rcp(new AutoClone<MultPreconditionerFactory>());
   precFactoryBuilder_.addClone("Block Multiply",clone);

   clone = rcp(new AutoClone<NS::LSCPreconditionerFactory>());
   precFactoryBuilder_.addClone("NS LSC",clone);

   clone = rcp(new AutoClone<NS::SIMPLEPreconditionerFactory>());
   precFactoryBuilder_.addClone("NS SIMPLE",clone);
}

void PreconditionerFactory::getPreconditionerFactoryNames(std::vector<std::string> & names)
{ 
   // initialize the defaults if necessary
   if(precFactoryBuilder_.cloneCount()==0) initializePrecFactoryBuilder();
   precFactoryBuilder_.getCloneNames(names); 
}

} // end namespace Teko
