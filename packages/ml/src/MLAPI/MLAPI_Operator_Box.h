#ifndef MLAPI_OPERATOR_BOX_H
#define MLAPI_OPERATOR_BOX_H

#include "ml_include.h"

namespace MLAPI {

/*!
\class ML_Operator_Box

\brief Simple wrapper for ML_Operator struct.

\author Marzio Sala, SNL 9214.

\date Last updated on Feb-05.
*/

class ML_Operator_Box {
public:
  ML_Operator_Box(ML_Operator* Op, bool Ownership = true)
  {
    Op_ = Op;
    Ownership_ = Ownership;
  }

  ~ML_Operator_Box()
  {
    if (Op_ && Ownership_)
      ML_Operator_Destroy(&Op_);
  }

  ML_Operator* GetData() const 
  {
    return(Op_);
  }

private:
  ML_Operator* Op_;
  bool Ownership_;

};

} // namespace MLAPI

#endif // MLAPI_OPERATOR_BOX_H

