#ifndef MLAPI_AGGREGATION_H
#define MLAPI_AGGREGATION_H

#include "ml_common.h"

/*!
\file MLAPI_Aggregation.h

\brief Functions to create aggregation-based prolongator operators.

\author Marzio Sala, SNL 9214.

\date Last updated on Feb-05.
*/

namespace Teuchos {
  class ParameterList;
}

namespace MLAPI {

class Operator;
class MultiVector;

//! Builds the tentative prolongator using aggregation.
void GetPtent(const Operator& A, Teuchos::ParameterList& List,
              const MultiVector& ThisNS, 
              Operator& Ptent, MultiVector& NextNS);

//! Builds the tentative prolongator with default null space.
void GetPtent(const Operator& A, Teuchos::ParameterList& List, Operator& Ptent);

} // namespace MLAPI

#endif // MLAPI_AGGREGATION_H
