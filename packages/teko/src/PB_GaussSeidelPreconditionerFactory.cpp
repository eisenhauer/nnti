#include "PB_GaussSeidelPreconditionerFactory.hpp"

#include "PB_BlockUpperTriInverseOp.hpp"
#include "PB_BlockLowerTriInverseOp.hpp"

using Teuchos::rcp;
using Teuchos::RCP;

namespace PB {

GaussSeidelPreconditionerFactory::GaussSeidelPreconditionerFactory(TriSolveType solveType,const LinearOp & invD0,const LinearOp & invD1)
      : invOpsStrategy_(rcp(new StaticInvDiagStrategy(invD0,invD1))), solveType_(solveType)
{ }

GaussSeidelPreconditionerFactory::GaussSeidelPreconditionerFactory(TriSolveType solveType,const RCP<const BlockInvDiagonalStrategy> & strategy)
         : invOpsStrategy_(strategy), solveType_(solveType)
{ }

GaussSeidelPreconditionerFactory::GaussSeidelPreconditionerFactory()
{ }

LinearOp GaussSeidelPreconditionerFactory::buildPreconditionerOperator(BlockedLinearOp & blo,BlockPreconditionerState & state) const
{
   int rows = blockRowCount(blo);
   int cols = blockColCount(blo);
 
   TEUCHOS_ASSERT(rows==cols);

   // get diagonal blocks
   std::vector<LinearOp> invDiag;
   invOpsStrategy_->getInvD(blo,state,invDiag);
   TEUCHOS_ASSERT(rows==invDiag.size());

   if(solveType_==GS_UseUpperTriangle) {
      // create a blocked linear operator
      BlockedLinearOp U = getUpperTriBlocks(blo);

      return createBlockUpperTriInverseOp(U,invDiag,"Gauss Seidel");
   } 
   else if(solveType_==GS_UseLowerTriangle) {
      // create a blocked linear operator
      BlockedLinearOp L = getLowerTriBlocks(blo);

      return createBlockLowerTriInverseOp(L,invDiag,"Gauss Seidel");
   }

   TEUCHOS_ASSERT(false); // we should never make it here!
}

//! Initialize from a parameter list
void GaussSeidelPreconditionerFactory::initializeFromParameterList(const Teuchos::ParameterList & pl)
{
   const std::string inverse_type = "Inverse Type";
   std::vector<RCP<InverseFactory> > inverses;

   RCP<const InverseLibrary> invLib = getInverseLibrary();

   // get string specifying default inverse
   std::string invStr = pl.get<std::string>(inverse_type);
   if(invStr=="") invStr = "Amesos";
   RCP<InverseFactory> defaultInverse = invLib->getInverseFactory(invStr);

   // now check individual solvers
   Teuchos::ParameterList::ConstIterator itr;
   for(itr=pl.begin();itr!=pl.end();++itr) {
      std::string fieldName = itr->first;

      // figure out what the integer is
      if(fieldName.compare(inverse_type)==0 && fieldName!=inverse_type) {
         int position = -1;
         std::string inverse,type;

         // figure out position
         std::stringstream ss(fieldName);
         ss >> inverse >> type >> position;

         if(position<=0)
            PB_DEBUG_MSG("Gauss-Seidel \"Inverse Type\" must be a (strictly) positive integer",1);

         // inserting inverse factory into vector
         std::string invStr = pl.get<std::string>(fieldName);
         if(position>inverses.size()) {
            inverses.resize(position,defaultInverse);
            inverses[position-1] = invLib->getInverseFactory(invStr);
         }
         else
            inverses[position-1] = invLib->getInverseFactory(invStr);
      }
   }

   // use default inverse
   if(inverses.size()==0) 
      inverses.push_back(defaultInverse);

   // based on parameter type build a strategy
   invOpsStrategy_ = rcp(new InvFactoryDiagStrategy(inverses));
}

} // end namspace PB
