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
// Questions? Contact Roscoe A. Bartlett (bartlettra@ornl.gov) 
// 
// ***********************************************************************
// @HEADER
*/


#include "Thyra_Simple2DModelEvaluator.hpp"

#include "Teuchos_UnitTestHarness.hpp"


namespace {


using Teuchos::null;
using Teuchos::RCP;
typedef Thyra::ModelEvaluatorBase MEB;
using Thyra::Simple2DModelEvaluator;
using Thyra::simple2DModelEvaluator;


//
// Unit tests
//


TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( SimpleModelEvaluator, construct, Scalar )
{
  RCP<Simple2DModelEvaluator<Scalar> > model = simple2DModelEvaluator<Scalar>();
  TEST_ASSERT(model != null);
  TEST_EQUALITY(model->Np(), 0);
  TEST_EQUALITY(model->Ng(), 0);
  TEST_ASSERT(model->get_x_space() != null);
  TEST_EQUALITY(model->get_x_space()->dim(), 2);
  TEST_ASSERT(model->get_f_space() != null);
  TEST_EQUALITY(model->get_f_space()->dim(), 2);
  // ToDo: Test getNominalValues()
  TEST_ASSERT(model->create_W_op() != null);
  TEST_ASSERT(model->get_W_factory() != null);
  MEB::InArgs<Scalar> inArgs = model->createInArgs();
  TEST_ASSERT(inArgs.supports(MEB::IN_ARG_x));
  TEST_EQUALITY(inArgs.Np(), 0);
}

TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_REAL_SCALAR_TYPES(
  SimpleModelEvaluator, construct )


TEUCHOS_UNIT_TEST_TEMPLATE_1_DECL( SimpleModelEvaluator, eval, Scalar )
{
  RCP<Simple2DModelEvaluator<Scalar> > model = simple2DModelEvaluator<Scalar>();
  // ToDo: Finish this!
  //TEST_FOR_EXCEPT(true);
}

TEUCHOS_UNIT_TEST_TEMPLATE_1_INSTANT_REAL_SCALAR_TYPES(
  SimpleModelEvaluator, eval )


} // namespace
