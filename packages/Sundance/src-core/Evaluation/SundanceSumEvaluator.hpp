#ifndef SUNDANCE_SUMEVALUATOR_H
#define SUNDANCE_SUMEVALUATOR_H

#include "SundanceDefs.hpp"
#include "SundanceEvaluator.hpp"
#include "Teuchos_TimeMonitor.hpp"

#ifndef DOXYGEN_DEVELOPER_ONLY

namespace SundanceCore 
{
  namespace Internal 
  {
    class SumExpr;
    
    /**
     *
     */
    class SumEvaluator : public BinaryEvaluator<SumExpr>
    {
    public:
      /** */
      SumEvaluator(const SumExpr* expr,
                   const EvalContext& context);

      /** */
      virtual ~SumEvaluator(){;}

      /** */
      virtual void internalEval(const EvalManager& mgr,
                   Array<double>& constantResults,
                   Array<RefCountPtr<EvalVector> >& vectorResults) const ;

      /** */
      TEUCHOS_TIMER(evalTimer, "sum evaluation");
    private:
      int sign_;
      Array<Array<int> > singleRightConstant_;
      Array<Array<int> > singleRightVector_;
      Array<Array<int> > singleLeftConstant_;
      Array<Array<int> > singleLeftVector_;
      Array<Array<int> > ccSums_;
      Array<Array<int> > cvSums_;
      Array<Array<int> > vcSums_;
      Array<Array<int> > vvSums_;
    }; 
  }
}

                  
#endif  /* DOXYGEN_DEVELOPER_ONLY */  


#endif
