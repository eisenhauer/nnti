/* @HEADER@ */
// ************************************************************************
// 
//                             Sundance
//                 Copyright 2011 Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Kevin Long (kevin.long@ttu.edu)
// 

/* @HEADER@ */

#ifndef SUNDANCE_TESTEVALMEDIATOR_H
#define SUNDANCE_TESTEVALMEDIATOR_H

#ifndef DOXYGEN_DEVELOPER_ONLY

#include "SundanceDefs.hpp"
#include "SundanceAbstractEvalMediator.hpp"
#include "SundanceSparsitySuperset.hpp"
#include "SundanceTestUnknownFunction.hpp"
#include "SundanceTestDiscreteFunction.hpp"
#include "SundancePoint.hpp"
#include "SundanceCoordExpr.hpp"
#include "SundanceMap.hpp"

namespace SundanceTesting
{
  using namespace Sundance;
  using namespace Sundance;
  using namespace Sundance;
  using Sundance::Map;
  /**
   *
   */
  class TestEvalMediator : public AbstractEvalMediator
  {
  public:
    /** */
    TestEvalMediator(const Expr& fields);

    /** */
    virtual ~TestEvalMediator(){;}

    /** */
    void setEvalPoint(const Point& x) {x_=x;}

    /** */
    int numFields() const {return fields_.size();}

    /** */
    const std::string& fieldName(int i) const {return fieldNames_[i];}

    /** */
    void setFieldCoeff(int i, double A) {fields_[i].setCoeff(A);}

    /** */
    double fieldCoeff(int i) const {return fields_[i].coeff();}

    /** */
    const Map<int, int>& funcIdToFieldNumberMap() const
    {return funcIdToFieldNumberMap_;}

    /** Evaluate the given coordinate expression, putting
     * its numerical values in the given LoadableVector. */
    virtual void evalCoordExpr(const CoordExpr* expr,
                               RCP<EvalVector>& vec) const ;

    /** Evaluate the given cell diameter expression, putting
     * its numerical values in the given EvalVector. */
    virtual void evalCellDiameterExpr(const CellDiameterExpr* expr,
                                      RCP<EvalVector>& vec) const ;

    /** Evaluate the given cell vector expression, putting
     * its numerical values in the given EvalVector. */
    virtual void evalCellVectorExpr(const CellVectorExpr* expr,
                                      RCP<EvalVector>& vec) const ;

    /** Evaluate the given discrete function, putting
     * its numerical values in the given LoadableVector. */
    virtual void evalDiscreteFuncElement(const DiscreteFuncElement* expr,
                                         const Array<MultiIndex>& mi,
                                         Array<RCP<EvalVector> >& vec) const ;

    double evalDummyBasis(int m, const MultiIndex& mi) const ;


  private:

    Point x_;
    Map<int, int> funcIdToFieldNumberMap_;
    Array<ADField> fields_;
    Array<string> fieldNames_;
  };
}


#endif /* DOXYGEN_DEVELOPER_ONLY */
#endif
