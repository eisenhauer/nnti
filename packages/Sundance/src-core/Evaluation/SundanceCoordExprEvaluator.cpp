/* @HEADER@ */
/* @HEADER@ */

#include "SundanceEvaluator.hpp"
#include "SundanceEvalManager.hpp"
#include "SundanceCoordExpr.hpp"
#include "SundanceExceptions.hpp"
#include "SundanceSet.hpp"
#include "SundanceTabs.hpp"
#include "SundanceOut.hpp"

using namespace SundanceCore;
using namespace SundanceUtils;

using namespace SundanceCore::Internal;
using namespace Teuchos;
using namespace TSFExtended;

CoordExprEvaluator::CoordExprEvaluator(const CoordExpr* expr, 
                                       const EvalContext& context)
  : SubtypeEvaluator<CoordExpr>(expr, context), 
    doValue_(false),
    doDeriv_(false),
    stringRep_(expr->toString())
{

  Tabs tabs;
  SUNDANCE_VERB_LOW(tabs << "initializing coord expr evaluator for " 
                    << expr->toString());
  SUNDANCE_VERB_MEDIUM(tabs << "return sparsity " << endl << *sparsity());

  TEST_FOR_EXCEPTION(sparsity()->numDerivs() > 2, InternalError,
                     "CoordExprEvaluator ctor found a sparsity table "
                     "with more than two entries. The bad sparsity table is "
                     << *sparsity());

  /* 
   * There are only two possible entries in the nozeros table for a
   * coordinate expression: a zeroth derivative, and a first-order
   * spatial derivative in the same direction as the expr's coordinate. 
   */
  
  for (int i=0; i<sparsity()->numDerivs(); i++)
    {
      const MultipleDeriv& d = sparsity()->deriv(i);

      /* for a zeroth-order derivative, evaluate the coord expr */
      if (d.order()==0) 
        {
          doValue_ = true;
          addVectorIndex(i, 0);
        }
      else /* for a first-order deriv, make sure it's in the proper direction,
            * then evaluate the spatial derivative. */
        {
          TEST_FOR_EXCEPTION(!sparsity()->isSpatialDeriv(i), InternalError,
                             "CoordExprEvaluator ctor found an entry in the "
                             "sparsity superset that is not a spatial derivative. "
                             "The bad entry is " << sparsity()->deriv(i) 
                             << ". The superset is " 
                             << *sparsity());

          const MultiIndex& mi = sparsity()->multiIndex(i);
          
          TEST_FOR_EXCEPTION(mi.order() != 1, InternalError,
                             "CoordExprEvaluator ctor found a multiindex of "
                             "order != 1. Bad multiindex is " << mi.toString());
          
          TEST_FOR_EXCEPTION(mi[expr->dir()]!=1, InternalError,
                             "CoordExprEvaluator sparsity pattern has an "
                             "element corresponding to differentiation wrt "
                             "a coordinate direction other than that of the "
                             "coord expr's direction");
          doDeriv_ = true;
          addConstantIndex(i, 0);
        }
    }
  
}



void CoordExprEvaluator::internalEval(const EvalManager& mgr,
                                      Array<double>& constantResults,
                                      Array<RefCountPtr<EvalVector> >& vectorResults) const 
{
  TimeMonitor timer(coordEvalTimer());
  Tabs tabs;

  SUNDANCE_OUT(verbosity() > VerbLow, tabs << "---CoordExprEvaluator---");

  if (verbosity() > 1)
    {
      cerr << tabs << "CoordExprEvaluator::eval: expr=" << expr()->toString() 
           << endl;
      cerr << tabs << "sparsity = " << endl << *sparsity() << endl;
    }

  if (doValue_)
    {
      vectorResults.resize(1);
      vectorResults[0] = mgr.popVector();
      mgr.evalCoordExpr(expr(), vectorResults[0]);
      if (EvalVector::shadowOps()) vectorResults[0]->setString(stringRep_);
    }
  
  if (doDeriv_)
    {
      constantResults.resize(1);
      constantResults[0] = 1.0;
    }

  if (verbosity() > VerbMedium)
    {
      cerr << tabs << "results " << endl;
      sparsity()->print(cerr, vectorResults,
                            constantResults);
    }

}

