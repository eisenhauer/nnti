/* @HEADER@ */
/* @HEADER@ */

#ifndef SUNDANCE_TEMPSTACK_H
#define SUNDANCE_TEMPSTACK_H

#include "SundanceDefs.hpp"
#include "SundanceEvalVector.hpp"
#include "SundanceNoncopyable.hpp"
#include <stack>

#ifndef DOXYGEN_DEVELOPER_ONLY

namespace SundanceCore
{
  using namespace SundanceUtils;
  namespace Internal
    {
      /**
       * TempStack provides a stack of temporary variables for use during
       * evaluation. 
       *
       * During the course of evaluating an expression, it is often necessary
       * to create temporary variables. For example, in evaluating
       * \code
       * a += b*(c+d)
       * \endcode
       * it is required to create three temporaries (ignoring for
       * explanatory purposes any copies made in cases where one of
       * the vectors will be used elsewhere). We can see this
       * by breaking the operation
       * down into the following steps:
       * \code
       * 1. Create a temporary variable t1
       * 2. Evaluate expression b into t1
       * 3. Create a temporary variable t2
       * 4. Evaluate expression c into t2
       * 3. Create a temporary variable t3
       * 4. Evaluate expression d into t3
       * 5. Carry out t2 += t3
       * 6. Carry out t1 *= t2
       * 7. Carry out a += t1
       * \endcode
       * The number of temporaries required for a given expression
       * will depend on the graph of the expression. In general, we want to
       * create exactly as many temporaries as are needed, and reuse any
       * temporaries that are no longer needed. This is a well-known problem
       * in compiler design, and can be accomplished by maintaining a
       * stack of temporaries. When a new temporary is needed, it is popped
       * from the stack; if the stack is empty, a new temporary is allocated.
       * When a step of a calculation is done, any temporaries used are
       * put back on the stack for further use.
       */
      class TempStack : public Noncopyable
        {
        public:
          /** Empty ctor */
          TempStack();

          /** Construct with an initial vector size */
          TempStack(int vecSize);

          /** Push vector data onto the stack */
          void pushVectorData(const RefCountPtr<Array<double> >& vecData) ;

          /** Pop vector data from the stack */
          RefCountPtr<Array<double> > popVectorData() ;

          /** Get a new vector (which will often reuse stack data) */
          RefCountPtr<EvalVector> popVector() 
          {return rcp(new EvalVector(this));}

          /** */
          void setVecSize(int vecSize) {vecSize_ = vecSize;}

          /** */
          void resetCounter() ;

          /** */
          int numVecsAccessed() const {return numVecsAccessed_;}

          /** */
          int numVecsAllocated() const {return numVecsAllocated_;}

          /** */
          int vecSize() const {return vecSize_;}

        private:
          
          int vecSize_;

          std::stack<RefCountPtr<Array<double> > > stack_;

          int numVecsAllocated_;

          int numVecsAccessed_;
        };
    }
}


#endif /* DOXYGEN_DEVELOPER_ONLY */
#endif
