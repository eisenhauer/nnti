#ifndef __PB_StridedEpetraOperator_hpp__
#define __PB_StridedEpetraOperator_hpp__

// Epetra includes
#include "Epetra_Operator.h"

// Teuchos includes
#include "Teuchos_RCP.hpp"

#include "Thyra_LinearOpBase.hpp"

// PB includes
#include "PB_BlockedReordering.hpp"
#include "Epetra/PB_EpetraOperatorWrapper.hpp"
#include "Epetra/PB_StridedMappingStrategy.hpp"

// NOX includes
#include "NOX.H"
#include "NOX_Epetra.H"

namespace PB {
namespace Epetra {

class StridedEpetraOperator : public EpetraOperatorWrapper {
public:
   StridedEpetraOperator(int numVars,const Teuchos::RCP<Epetra_Operator> & content,
                         const std::string & label="<ANYM>");
   StridedEpetraOperator(const std::vector<int> & vars,const Teuchos::RCP<Epetra_Operator> & content,
                         const std::string & label="<ANYM>");

   virtual void SetContent(const std::vector<int> & vars,const Teuchos::RCP<Epetra_Operator> & content);

   virtual void RebuildOps()
   { BuildBlockedOperator(); }

   virtual const Teuchos::RCP<const Epetra_Operator> GetContent() const
   { return fullContent_; }

   virtual const Teuchos::RCP<Epetra_Operator> GetContent()
   { return fullContent_; }

   const Teuchos::RCP<const Epetra_Operator> GetBlock(int i,int j) const;

   /** Use a reorder manager to block this operator as desired.
     * Multiple calls to the function reorder only the underlying object. 
     */
   void Reorder(const BlockReorderManager & brm);

   //! Remove any reordering on this object
   void RemoveReording();

   // functions overloading Epetra_Operator
   ////////////////////////////////////////////////

   // destructor
   virtual ~StridedEpetraOperator() {}

   // attribute set methods
   
   // don't use transpose...ever!
   virtual int SetUseTranspose(bool useTranspose)
   { return -1; }

   virtual int ApplyInverse(const Epetra_MultiVector &X, Epetra_MultiVector &Y) const
   { TEUCHOS_ASSERT(false); return -1; }

   virtual double NormInf() const
   { TEUCHOS_ASSERT(false); return 0.0; }

   // attribute access functions
   virtual bool UseTranspose() const { return false; }
   virtual bool HasNormInf() const { return false; }
   virtual const Epetra_Comm & Comm() const { return fullContent_->Comm(); }

protected:
   // gooey center of this shell
   Teuchos::RCP<Epetra_Operator> fullContent_;
   Teuchos::RCP<StridedMappingStrategy> stridedMapping_;
   Teuchos::RCP<const Thyra::LinearOpBase<double> > stridedOperator_;
   Teuchos::RCP<const BlockReorderManager> reorderManager_;

   std::string label_;

   void BuildBlockedOperator();
};

} // end namespace Epetra
} // end namespace PB

#endif
