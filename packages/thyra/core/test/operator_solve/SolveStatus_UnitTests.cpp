/*
// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
//                 Copyright (2004) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER
*/

#include "Thyra_SolveSupportTypes.hpp"
#include "Thyra_DefaultSpmdVectorSpace.hpp"
#include "Thyra_VectorBase.hpp"
#include "Thyra_VectorStdOps.hpp"

#include "Teuchos_UnitTestHarness.hpp"
#include "Thyra_UnitTestHelpers.hpp"


namespace Thyra {


//
// Helper code
//


using Teuchos::as;


const Ordinal n = 4;


/** \brief Mock ReductionFunctional subclass used for unit testing. */
template<class Scalar>
class MockNormReductionFunctional : public ReductionFunctional<Scalar> {
public:

  /** \brief . */
  MockNormReductionFunctional(
    const RCP<const VectorSpaceBase<Scalar> > &space
    )
    : space_(space)
    {}

  /** \name Overridden public functions from Describable. */
  //@{
  virtual std::string description() const
    {
      std::ostringstream oss;
      oss << typeName(*this)<<"{"<<space_->description()<<"}";
      return oss.str();
    }

  //@}
 
protected:

  /** \name Overridded protected functions overridden from ReductionFunctional. */
  //@{

  /** \brief . */
  virtual typename ScalarTraits<Scalar>::magnitudeType
  reduceImpl(const VectorBase<Scalar> &v) const
    {
      typedef ScalarTraits<Scalar> ST;
      return norm<Scalar>(v);
    }

  /** \brief . */
  virtual bool isCompatibleImpl( const VectorBase<Scalar> &v ) const
    {
      return space_->isCompatible(*v.space());
    }

  //@}

private:

  RCP<const VectorSpaceBase<Scalar> > space_;

};


template<class Scalar>
RCP<MockNormReductionFunctional<Scalar> >
createMockNormReductionFunctional(
  const RCP<const VectorSpaceBase<Scalar> > &space
  )
{
  return Teuchos::rcp(new MockNormReductionFunctional<Scalar>(space));
}


//
// Unit Tests
//


//
// SolveMeasureType
//


TEUCHOS_UNIT_TEST( SolveMeasureType, matches )
{
  TEST_ASSERT(SolveMeasureType()(SOLVE_MEASURE_ONE, SOLVE_MEASURE_ONE));
  ECHO(SolveMeasureType solveMeasureType(SOLVE_MEASURE_NORM_RESIDUAL, SOLVE_MEASURE_NORM_INIT_RESIDUAL));
  TEST_ASSERT(solveMeasureType(SOLVE_MEASURE_NORM_RESIDUAL, SOLVE_MEASURE_NORM_INIT_RESIDUAL));
}


TEUCHOS_UNIT_TEST( SolveMeasureType, contains )
{
  ECHO(SolveMeasureType solveMeasureType(SOLVE_MEASURE_NORM_RESIDUAL, SOLVE_MEASURE_NORM_INIT_RESIDUAL));
  TEST_EQUALITY_CONST(solveMeasureType.contains(SOLVE_MEASURE_ONE), false);
  TEST_EQUALITY_CONST(solveMeasureType.contains(SOLVE_MEASURE_NORM_RESIDUAL), true);
  TEST_EQUALITY_CONST(solveMeasureType.contains(SOLVE_MEASURE_NORM_SOLUTION), false);
  TEST_EQUALITY_CONST(solveMeasureType.contains(SOLVE_MEASURE_NORM_INIT_RESIDUAL), true);
  TEST_EQUALITY_CONST(solveMeasureType.contains(SOLVE_MEASURE_NORM_RHS), false);
}


//
// ReductionFunctional
//


TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( ReductionFunctional, reduce, Scalar )
{
  typedef ScalarTraits<Scalar> ST; typedef typename ST::magnitudeType ScalarMag;
  typedef ScalarTraits<ScalarMag> SMT;
  const RCP<VectorBase<Scalar> > v = createMember<Scalar>(defaultSpmdVectorSpace<Scalar>(n));
  V_S(v.ptr(), ST::one());
  MockNormReductionFunctional<Scalar> mockNormReductionFunctional(v->space());
  TEST_FLOATING_EQUALITY(mockNormReductionFunctional.reduce(*v),
    SMT::squareroot(n),
    as<ScalarMag>(SMT::eps() * 10.0) );
}
THYRA_UNIT_TEST_TEMPLATE_1_INSTANT_SCALAR_TYPES( ReductionFunctional, reduce )


#ifdef THYRA_DEBUG

  
TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( ReductionFunctional, notCompatible, Scalar )
{
  typedef ScalarTraits<Scalar> ST; typedef typename ST::magnitudeType ScalarMag;
  typedef ScalarTraits<ScalarMag> SMT;
  const RCP<VectorBase<Scalar> > v = createMember<Scalar>(defaultSpmdVectorSpace<Scalar>(n));
  MockNormReductionFunctional<Scalar> mockNormReductionFunctional(
    defaultSpmdVectorSpace<Scalar>(n/2));
  TEST_THROW(mockNormReductionFunctional.reduce(*v), Exceptions::IncompatibleVectorSpaces);
}
THYRA_UNIT_TEST_TEMPLATE_1_INSTANT_SCALAR_TYPES( ReductionFunctional, notCompatible )


#endif // THYRA_DEBUG


//
// SolveCriteria
//

  
TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( SolveCriteria, defaultConstruct, Scalar )
{
  typedef ScalarTraits<Scalar> ST; typedef typename ST::magnitudeType ScalarMag;
  typedef ScalarTraits<ScalarMag> SMT;
  SolveCriteria<Scalar> solveCriteria;
  TEST_EQUALITY(solveCriteria.solveMeasureType.numerator, SOLVE_MEASURE_ONE);
  TEST_EQUALITY(solveCriteria.solveMeasureType.denominator, SOLVE_MEASURE_ONE);
  TEST_EQUALITY(solveCriteria.requestedTol, SolveCriteria<Scalar>::unspecifiedTolerance());
  TEST_EQUALITY_CONST(solveCriteria.extraParameters, Teuchos::null);
  TEST_EQUALITY_CONST(solveCriteria.numeratorReductionFunc, Teuchos::null);
  TEST_EQUALITY_CONST(solveCriteria.denominatorReductionFunc, Teuchos::null);
}
THYRA_UNIT_TEST_TEMPLATE_1_INSTANT_SCALAR_TYPES( SolveCriteria, defaultConstruct )

  
TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( SolveCriteria, constructAll, Scalar )
{
  typedef ScalarTraits<Scalar> ST; typedef typename ST::magnitudeType ScalarMag;
  typedef ScalarTraits<ScalarMag> SMT;
  const SolveMeasureType solveMeasureType(SOLVE_MEASURE_NORM_RESIDUAL, SOLVE_MEASURE_NORM_RHS);
  const ScalarMag requestedTol = 0.5;
  RCP<ParameterList> extraParameters = Teuchos::parameterList();
  RCP<ReductionFunctional<Scalar> > numeratorReductionFunc =
    createMockNormReductionFunctional<Scalar>(Teuchos::null);
  RCP<ReductionFunctional<Scalar> > denominatorReductionFunc =
    createMockNormReductionFunctional<Scalar>(Teuchos::null);
  SolveCriteria<Scalar> solveCriteria(solveMeasureType, requestedTol,
    extraParameters, numeratorReductionFunc, denominatorReductionFunc);
  TEST_EQUALITY(solveCriteria.solveMeasureType.numerator, SOLVE_MEASURE_NORM_RESIDUAL);
  TEST_EQUALITY(solveCriteria.solveMeasureType.denominator, SOLVE_MEASURE_NORM_RHS);
  TEST_EQUALITY(solveCriteria.requestedTol, requestedTol);
  TEST_EQUALITY(solveCriteria.extraParameters, extraParameters);
  TEST_EQUALITY(solveCriteria.numeratorReductionFunc, numeratorReductionFunc);
  TEST_EQUALITY(solveCriteria.denominatorReductionFunc, denominatorReductionFunc);
}
THYRA_UNIT_TEST_TEMPLATE_1_INSTANT_SCALAR_TYPES( SolveCriteria, constructAll )



} // namespace Thyra
