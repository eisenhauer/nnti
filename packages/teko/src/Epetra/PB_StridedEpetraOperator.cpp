#include "Epetra/PB_StridedEpetraOperator.hpp"
#include "Epetra/PB_StridedMappingStrategy.hpp"
#include "Epetra/PB_ReorderedMappingStrategy.hpp"

#include "Thyra_LinearOpBase.hpp"
#include "Thyra_EpetraLinearOp.hpp"
#include "Thyra_EpetraThyraWrappers.hpp"
#include "Thyra_DefaultProductMultiVector.hpp"
#include "Thyra_DefaultProductVectorSpace.hpp"
#include "Thyra_DefaultBlockedLinearOp.hpp"
#include "Thyra_get_Epetra_Operator.hpp"

#include "EpetraExt_MultiVectorOut.h"

namespace PB {
namespace Epetra {

using Teuchos::RCP;
using Teuchos::rcp;
using Teuchos::rcp_dynamic_cast;

StridedEpetraOperator::StridedEpetraOperator(int numVars,const Teuchos::RCP<Epetra_Operator> & content,
                                             const std::string & label) 
      : PB::Epetra::EpetraOperatorWrapper(), label_(label)
{
   std::vector<int> vars;
   
   // build vector describing the sub maps
   for(int i=0;i<numVars;i++) vars.push_back(1);

   SetContent(vars,content);
}

StridedEpetraOperator::StridedEpetraOperator(const std::vector<int> & vars,const Teuchos::RCP<Epetra_Operator> & content,
                                             const std::string & label) 
      : PB::Epetra::EpetraOperatorWrapper(), label_(label)
{
   SetContent(vars,content);
}

void StridedEpetraOperator::SetContent(const std::vector<int> & vars,const Teuchos::RCP<Epetra_Operator> & content)
{ 
   fullContent_ = content;
   stridedMapping_ = rcp(new StridedMappingStrategy(vars,Teuchos::rcpFromRef(fullContent_->OperatorDomainMap()),
                                                         fullContent_->Comm()));
   SetMapStrategy(stridedMapping_);

   // build thyra operator
   BuildBlockedOperator(); 
}

void StridedEpetraOperator::BuildBlockedOperator()
{
   TEUCHOS_ASSERT(stridedMapping_!=Teuchos::null);

   // get a CRS matrix
   const RCP<const Epetra_CrsMatrix> crsContent = rcp_dynamic_cast<const Epetra_CrsMatrix>(fullContent_);

   // ask the strategy to build the Thyra operator for you
   const RCP<const Thyra::LinearOpBase<double> > A = stridedMapping_->buildBlockedThyraOp(crsContent,label_);
         //= rcp_dynamic_cast<const StridedMappingStrategy>(mapStrategy_)->buildBlockedThyraOp(crsContent,label_);

   stridedOperator_ = A;

   // set whatever is returned
   SetOperator(stridedOperator_,false);

   // reorder if neccessary
   if(reorderManager_!=Teuchos::null) 
      Reorder(*reorderManager_);
}

const Teuchos::RCP<const Epetra_Operator> StridedEpetraOperator::GetBlock(int i,int j) const
{
   const RCP<const Thyra::BlockedLinearOpBase<double> > blkOp 
         = Teuchos::rcp_dynamic_cast<const Thyra::BlockedLinearOpBase<double> >(getThyraOp());

   return Thyra::get_Epetra_Operator(*blkOp->getBlock(i,j));
}

/** Use a reorder manager to block this operator as desired.
  * Multiple calls to the function reorder only the underlying object. 
  */
void StridedEpetraOperator::Reorder(const BlockReorderManager & brm)
{
   reorderManager_ = rcp(new BlockReorderManager(brm));

   // build reordered objects
   RCP<const MappingStrategy> reorderMapping = rcp(new ReorderedMappingStrategy(*reorderManager_,stridedMapping_));
   RCP<const Thyra::BlockedLinearOpBase<double> > blockOp
         = rcp_dynamic_cast<const Thyra::BlockedLinearOpBase<double> >(stridedOperator_);
   RCP<const Thyra::LinearOpBase<double> > A = buildReorderedLinearOp(*reorderManager_,blockOp);

   // set them as working values
   SetMapStrategy(reorderMapping);
   SetOperator(A,false);
}

//! Remove any reordering on this object
void StridedEpetraOperator::RemoveReording()
{
   SetMapStrategy(stridedMapping_);
   SetOperator(stridedOperator_,false);
   reorderManager_ = Teuchos::null;
}

} // end namespace Epetra
} // end namespace PB
