/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#ifndef Stk_Rebalance_Use_Cases_UseCase_1_hpp
#define Stk_Rebalance_Use_Cases_UseCase_1_hpp

#include <stk_util/parallel/Parallel.hpp>

namespace stk {
namespace rebalance {
namespace use_cases {

class UseCase_1_Rebalance
{
public:
  ~UseCase_1_Rebalance();

  UseCase_1_Rebalance( stk::ParallelMachine comm );
};


} //namespace use_cases
} //namespace rebalance
} //namespace stk

#endif // Stk_Rebalance_Use_Cases_UseCase_1_hpp

