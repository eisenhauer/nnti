// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
//                 Copyright (2004) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef THYRA_LINEAR_OP_TESTER_DEF_HPP
#define THYRA_LINEAR_OP_TESTER_DEF_HPP

#include "Thyra_LinearOpTester_decl.hpp"
#include "Thyra_LinearOpBase.hpp"
#include "Thyra_DefaultScaledAdjointLinearOp.hpp"
#include "Thyra_describeLinearOp.hpp"
#include "Thyra_VectorStdOps.hpp"
#include "Thyra_TestingTools.hpp"
#include "Thyra_UniversalMultiVectorRandomizer.hpp"


namespace Thyra {


template<class Scalar>
class SymmetricLinearOpTester {
public:
  typedef typename Teuchos::ScalarTraits<Scalar>::magnitudeType ScalarMag;
  static void checkSymmetry(
    const LinearOpBase<Scalar> &op,
    MultiVectorRandomizerBase<Scalar> *dRand,
    Teuchos::FancyOStream &oss,
    const int num_rhs,
    const int num_random_vectors,
    const Teuchos::EVerbosityLevel verbLevel,
    const bool dump_all,
    const ScalarMag &symmetry_error_tol,
    const ScalarMag &symmetry_warning_tol,
    const Ptr<bool> &these_results
    )
    {

      using std::endl;

      bool result;
      using Teuchos::OSTab;
      typedef Teuchos::ScalarTraits<Scalar> ST;
      const Scalar half = Scalar(0.4)*ST::one();
      RCP<const VectorSpaceBase<Scalar> > domain = op.domain();
      
      oss << endl << "op.domain()->isCompatible(*op.range()) == true : ";
      result = op.domain()->isCompatible(*op.range());
      if(!result) *these_results = false;
      oss << passfail(result) << endl;
      
      if(result) {
        
        oss
          << endl << "Checking that the operator is symmetric as:\n"
          << endl << "  <0.5*op*v2,v1> == <v2,0.5*op*v1>"
          << endl << "   \\_______/            \\_______/"
          << endl << "      v4                    v3"
          << endl << ""
          << endl << "         <v4,v1> == <v2,v3>"
          << endl;
        
        for( int rand_vec_i = 1; rand_vec_i <= num_random_vectors; ++rand_vec_i ) {
          
          oss << endl << "Random vector tests = " << rand_vec_i << endl;

          OSTab tab(Teuchos::rcp(&oss,false));

          if(dump_all) oss << endl << "v1 = randomize(-1,+1); ...\n" ;
          RCP<MultiVectorBase<Scalar> > v1 = createMembers(domain,num_rhs);
          dRand->randomize(&*v1);
          if(dump_all) oss << endl << "v1 =\n" << describe(*v1,verbLevel);
          
          if(dump_all) oss << endl << "v2 = randomize(-1,+1); ...\n" ;
          RCP<MultiVectorBase<Scalar> > v2 = createMembers(domain,num_rhs);
          dRand->randomize(&*v2);
          if(dump_all) oss << endl << "v2 =\n" << describe(*v2,verbLevel);
          
          if(dump_all) oss << endl << "v3 = 0.5*op*v1 ...\n" ;
          RCP<MultiVectorBase<Scalar> > v3 = createMembers(domain,num_rhs);
          apply( op, NONCONJ_ELE, *v1, &*v3, half );
          if(dump_all) oss << endl << "v3 =\n" << describe(*v3,verbLevel);
          
          if(dump_all) oss << endl << "v4 = 0.5*op*v2 ...\n" ;
          RCP<MultiVectorBase<Scalar> > v4 = createMembers(domain,num_rhs);
          apply( op, NONCONJ_ELE, *v2, &*v4, half );
          if(dump_all) oss << endl << "v4 =\n" << describe(*v4,verbLevel);

          std::vector<Scalar> prod1(num_rhs), prod2(num_rhs);
          domain->scalarProds(*v4,*v1,&prod1[0]);
          domain->scalarProds(*v2,*v3,&prod2[0]);
          
          result = testRelErrors(
            num_rhs
            ,"<v4,v1>", &prod1[0]
            ,"<v2,v3>", &prod2[0]
            ,"symmetry_error_tol()", symmetry_error_tol
            ,"symmetry_warning_tol()", symmetry_warning_tol
            ,&oss
            );
          if(!result) *these_results = false;
        
        }
      }
      else {
        oss << endl << "Range and domain spaces are different, skipping check!\n";
      }
    }
};


//
// LinearOpTester
//


template<class Scalar>
LinearOpTester<Scalar>::LinearOpTester(
  const bool check_linear_properties_in,
  const ScalarMag linear_properties_warning_tol_in,
  const ScalarMag linear_properties_error_tol_in,
  const bool check_adjoint_in,
  const ScalarMag adjoint_warning_tol_in,
  const ScalarMag adjoint_error_tol_in,
  const bool check_for_symmetry_in,
  const ScalarMag symmetry_warning_tol_in,
  const ScalarMag symmetry_error_tol_in,
  const int num_random_vectors_in,
  const bool show_all_tests_in,
  const bool dump_all_in,
  const int num_rhs_in
  )
  :check_linear_properties_(check_linear_properties_in),
   linear_properties_warning_tol_(linear_properties_warning_tol_in),
   linear_properties_error_tol_(linear_properties_error_tol_in),
   check_adjoint_(check_adjoint_in),
   adjoint_warning_tol_(adjoint_warning_tol_in),
   adjoint_error_tol_(adjoint_error_tol_in),
   check_for_symmetry_(check_for_symmetry_in),
   symmetry_warning_tol_(symmetry_warning_tol_in),
   symmetry_error_tol_(symmetry_error_tol_in),
   num_random_vectors_(num_random_vectors_in),
   show_all_tests_(show_all_tests_in),
   dump_all_(dump_all_in),
   num_rhs_(num_rhs_in)
{}


template<class Scalar>
void LinearOpTester<Scalar>::enable_all_tests( const bool enable_all_tests_in )
{
  check_linear_properties_ = enable_all_tests_in;
  check_adjoint_ = enable_all_tests_in;
  check_for_symmetry_ = enable_all_tests_in;
}


template<class Scalar>
void LinearOpTester<Scalar>::set_all_warning_tol( const ScalarMag warning_tol_in )
{
  linear_properties_warning_tol_ = warning_tol_in;
  adjoint_warning_tol_ = warning_tol_in;
  symmetry_warning_tol_ = warning_tol_in;
}


template<class Scalar>
void LinearOpTester<Scalar>::set_all_error_tol( const ScalarMag error_tol_in )
{
  linear_properties_error_tol_ = error_tol_in;
  adjoint_error_tol_ = error_tol_in;
  symmetry_error_tol_ = error_tol_in;
}


template<class Scalar>
bool LinearOpTester<Scalar>::check(
  const LinearOpBase<Scalar> &op,
  const Ptr<MultiVectorRandomizerBase<Scalar> > &rangeRandomizer,
  const Ptr<MultiVectorRandomizerBase<Scalar> > &domainRandomizer,
  const Ptr<Teuchos::FancyOStream> &out_inout
  ) const
{

  using std::endl;
  using Teuchos::as;
  using Teuchos::rcp;
  using Teuchos::rcpFromPtr;
  using Teuchos::rcpFromRef;
  using Teuchos::outArg;
  using Teuchos::fancyOStream;
  using Teuchos::FancyOStream;
  using Teuchos::OSTab;
  typedef Teuchos::ScalarTraits<Scalar>  RST;
  typedef Teuchos::ScalarTraits<Scalar> DST;
  bool success = true, result;
  const int loc_num_rhs = this->num_rhs();
  const Scalar r_one  = RST::one();
  const Scalar d_one  = DST::one();
  const Scalar r_half = as<Scalar>(0.5)*r_one;
  const Scalar d_half = as<Scalar>(0.5)*d_one;

  RCP<FancyOStream> out;
  if (!is_null(out_inout))
    out = Teuchos::rcpFromPtr(out_inout);
  else
    out = Teuchos::fancyOStream(rcp(new Teuchos::oblackholestream));

  const Teuchos::EVerbosityLevel verbLevel =
    (dump_all()?Teuchos::VERB_EXTREME:Teuchos::VERB_MEDIUM);

  OSTab tab2(out,1,"THYRA");

  // ToDo 04/28/2005:
  // * Test the MultiVectorBase apply() function and output to the VectorBase apply() function!

  *out << endl << "*** Entering LinearOpTester<"<<RST::name()<<","<<DST::name()<<">::check(op,...) ...\n";
  if(show_all_tests()) {
    *out << endl << "describe op:\n" << Teuchos::describe(op,verbLevel);
/*
  if(op.applyTransposeSupports(CONJ_ELE) && verbLevel==Teuchos::VERB_EXTREME) {
  *out << endl << "describe adjoint op:\n";
  describeLinearOp(*adjoint(Teuchos::rcp(&op,false)),*out,verbLevel);
  }
*/
  }
  else {
    *out << endl << "describe op:\n" << Teuchos::describe(op,Teuchos::VERB_LOW);
  }

  RCP< MultiVectorRandomizerBase<Scalar> >  rRand;
  if (!is_null(rangeRandomizer))
    rRand = rcpFromPtr(rangeRandomizer);
  else
    rRand = rcp(new UniversalMultiVectorRandomizer<Scalar>());
  RCP< MultiVectorRandomizerBase<Scalar> > dRand;
  if (!is_null(domainRandomizer))
    dRand = rcpFromPtr(domainRandomizer);
  else
    dRand = rcp(new UniversalMultiVectorRandomizer<Scalar>());
  
  *out << endl << "Checking the domain and range spaces ... ";

  RCP<const VectorSpaceBase<Scalar> >  range  = op.range();
  RCP<const VectorSpaceBase<Scalar> > domain = op.domain();
  
  {

    std::ostringstream ossStore;
    const RCP<FancyOStream> oss = fancyOStream(rcpFromRef(ossStore));
    ossStore.copyfmt(*out);
    bool these_results = true;

    *oss << endl << "op.domain().get() != NULL ? ";
    result = domain.get() != NULL;
    if(!result) these_results = false;
    *oss << passfail(result) << endl;
    
    *oss << endl << "op.range().get() != NULL ? ";
    result = range.get() != NULL;
    if(!result) these_results = false;
    *oss << passfail(result) << endl;

    printTestResults(these_results,ossStore.str(),show_all_tests(),&success,OSTab(out).get());

  }

  if( check_linear_properties() ) {

    *out << endl << "this->check_linear_properties()==true:"
         << "Checking the linear properties of the forward linear operator ... ";

    std::ostringstream ossStore;
    const RCP<FancyOStream> oss = fancyOStream(rcpFromRef(ossStore));
    ossStore.copyfmt(*out);
    bool these_results = true;

    *oss << endl << "op.applySupports(NONCONJ_ELE) == true ? ";
    result = op.applySupports(NONCONJ_ELE);
    if(!result) these_results = false;
    *oss << passfail(result) << endl;

    if(result) {
    
      *oss
        << endl << "Checking that the forward operator is truly linear:\n"
        << endl << "  0.5*op*(v1 + v2) == 0.5*op*v1 + 0.5*op*v2"
        << endl << "          \\_____/         \\___/"
        << endl << "             v3            v5"
        << endl << "  \\_____________/     \\___________________/"
        << endl << "         v4                    v5"
        << endl << ""
        << endl << "           sum(v4) == sum(v5)"
        << endl;
      
      for( int rand_vec_i = 1; rand_vec_i <= num_random_vectors(); ++rand_vec_i ) {
        
        *oss << endl << "Random vector tests = " << rand_vec_i << endl;

        OSTab tab3(oss);
        
        *oss << endl << "v1 = randomize(-1,+1); ...\n" ;
        RCP<MultiVectorBase<Scalar> > v1 = createMembers(domain,loc_num_rhs);
        dRand->randomize(&*v1);
        if(dump_all()) *oss << endl << "v1 =\n" << describe(*v1,verbLevel);
        
        *oss << endl << "v2 = randomize(-1,+1); ...\n" ;
        RCP<MultiVectorBase<Scalar> > v2 = createMembers(domain,loc_num_rhs);
        dRand->randomize(&*v2);
        if(dump_all()) *oss << endl << "v2 =\n" << describe(*v2,verbLevel);
        
        *oss << endl << "v3 = v1 + v2 ...\n" ;
        RCP<MultiVectorBase<Scalar> > v3 = createMembers(domain,loc_num_rhs);
        V_VpV(&*v3,*v1,*v2);
        if(dump_all()) *oss << endl << "v3 =\n" << describe(*v3,verbLevel);
        
        *oss << endl << "v4 = 0.5*op*v3 ...\n" ;
        RCP<MultiVectorBase<Scalar> > v4 = createMembers(range,loc_num_rhs);
        apply( op, NONCONJ_ELE, *v3, &*v4, r_half );
        if(dump_all()) *oss << endl << "v4 =\n" << describe(*v4,verbLevel);
        
        *oss << endl << "v5 = op*v1 ...\n" ;
        RCP<MultiVectorBase<Scalar> > v5 = createMembers(range,loc_num_rhs);
        apply( op, NONCONJ_ELE, *v1, &*v5 );
        if(dump_all()) *oss << endl << "v5 =\n" << describe(*v5,verbLevel);
        
        *oss << endl << "v5 = 0.5*op*v2 + 0.5*v5 ...\n" ;
        apply( op, NONCONJ_ELE, *v2, &*v5, r_half, r_half );
        if(dump_all()) *oss << endl << "v5 =\n" << describe(*v5,verbLevel);

        std::vector<Scalar> sum_v4(loc_num_rhs), sum_v5(loc_num_rhs);
        sums(*v4,&sum_v4[0]);
        sums(*v5,&sum_v5[0]);
        
        result = testRelErrors(
          loc_num_rhs
          ,"sum(v4)", &sum_v4[0]
          ,"sum(v5)", &sum_v5[0]
          ,"linear_properties_error_tol()", linear_properties_error_tol()
          ,"linear_properties_warning_tol()", linear_properties_warning_tol()
          ,&*oss
          );
        if(!result) these_results = false;
        
      }
    }
    else {
      *oss << endl << "Forward operator not supported, skipping check!\n";
    }

    printTestResults(these_results,ossStore.str(),show_all_tests(),&success,OSTab(out).get());

  }
  else {
    *out << endl << "this->check_linear_properties()==false: Skipping the check of the linear properties of the forward operator!\n";
  }

  if( check_linear_properties() && check_adjoint() ) {

    *out << endl << "(this->check_linear_properties()&&this->check_adjoint())==true:"
         << " Checking the linear properties of the adjoint operator ... ";

    std::ostringstream ossStore;
    RCP<FancyOStream> oss =
      rcp(new FancyOStream(rcpFromRef(ossStore)));
    ossStore.copyfmt(*out);
    bool these_results = true;

    *oss << endl << "op.applyTransposeSupports(CONJ_ELE) == true ? ";
    result = op.applyTransposeSupports(CONJ_ELE);
    if(!result) these_results = false;
    *oss << passfail(result) << endl;

    if(result) {
    
      *oss
        << endl << "Checking that the adjoint operator is truly linear:\n"
        << endl << "  0.5*op'*(v1 + v2) == 0.5*op'*v1 + 0.5*op'*v2"
        << endl << "           \\_____/         \\____/"
        << endl << "              v3             v5"
        << endl << "  \\_______________/    \\_____________________/"
        << endl << "         v4                      v5"
        << endl << ""
        << endl << "           sum(v4) == sum(v5)"
        << endl;
      
      for( int rand_vec_i = 1; rand_vec_i <= num_random_vectors(); ++rand_vec_i ) {
        
        *oss << endl << "Random vector tests = " << rand_vec_i << endl;

        OSTab tab(oss);
        
        *oss << endl << "v1 = randomize(-1,+1); ...\n" ;
        RCP<MultiVectorBase<Scalar> > v1 = createMembers(range,loc_num_rhs);
        rRand->randomize(&*v1);
        if(dump_all()) *oss << endl << "v1 =\n" << describe(*v1,verbLevel);
        
        *oss << endl << "v2 = randomize(-1,+1); ...\n" ;
        RCP<MultiVectorBase<Scalar> > v2 = createMembers(range,loc_num_rhs);
        rRand->randomize(&*v2);
        if(dump_all()) *oss << endl << "v2 =\n" << describe(*v2,verbLevel);
        
        *oss << endl << "v3 = v1 + v2 ...\n" ;
        RCP<MultiVectorBase<Scalar> > v3 = createMembers(range,loc_num_rhs);
        V_VpV(&*v3,*v1,*v2);
        if(dump_all()) *oss << endl << "v3 =\n" << describe(*v3,verbLevel);
        
        *oss << endl << "v4 = 0.5*op'*v3 ...\n" ;
        RCP<MultiVectorBase<Scalar> > v4 = createMembers(domain,loc_num_rhs);
        applyTranspose( op, CONJ_ELE, *v3, &*v4, d_half );
        if(dump_all()) *oss << endl << "v4 =\n" << describe(*v4,verbLevel);
        
        *oss << endl << "v5 = op'*v1 ...\n" ;
        RCP<MultiVectorBase<Scalar> > v5 = createMembers(domain,loc_num_rhs);
        applyTranspose( op, CONJ_ELE, *v1, &*v5 );
        if(dump_all()) *oss << endl << "v5 =\n" << describe(*v5,verbLevel);
        
        *oss << endl << "v5 = 0.5*op'*v2 + 0.5*v5 ...\n" ;
        applyTranspose( op, CONJ_ELE, *v2, &*v5, d_half, d_half );
        if(dump_all()) *oss << endl << "v5 =\n" << describe(*v5,verbLevel);
        

        std::vector<Scalar> sum_v4(loc_num_rhs), sum_v5(loc_num_rhs);
        sums(*v4,&sum_v4[0]);
        sums(*v5,&sum_v5[0]);
        
        result = testRelErrors(
          loc_num_rhs
          ,"sum(v4)", &sum_v4[0]
          ,"sum(v5)", &sum_v5[0]
          ,"linear_properties_error_tol()", linear_properties_error_tol()
          ,"linear_properties_warning_tol()", linear_properties_warning_tol()
          ,&*oss
          );
        if(!result) these_results = false;
        
      }
    }
    else {
      *oss << endl << "Adjoint operator not supported, skipping check!\n";
    }

    printTestResults(these_results,ossStore.str(),show_all_tests(),&success,OSTab(out).get());

  }
  else {
    *out << endl << "(this->check_linear_properties()&&this->check_adjoint())==false: Skipping the check of the linear properties of the adjoint operator!\n";
  }
  
  if( check_adjoint() ) {

    *out << endl << "this->check_adjoint()==true:"
         << " Checking the agreement of the adjoint and forward operators ... ";

    std::ostringstream ossStore;
    RCP<FancyOStream> oss =
      rcp(new FancyOStream(rcpFromRef(ossStore)));
    ossStore.copyfmt(*out);
    bool these_results = true;
    
    *oss << endl << "op.applyTransposeSupports(CONJ_ELE) == true ? ";
    result = op.applyTransposeSupports(CONJ_ELE);
    if(!result) these_results = false;
    *oss << passfail(result) << endl;

    if(result) {
    
      *oss
        << endl << "Checking that the adjoint agrees with the non-adjoint operator as:\n"
        << endl << "  <0.5*op'*v2,v1> == <v2,0.5*op*v1>"
        << endl << "   \\________/            \\_______/"
        << endl << "       v4                   v3"
        << endl << ""
        << endl << "         <v4,v1>  == <v2,v3>"
        << endl;
    
      for( int rand_vec_i = 1; rand_vec_i <= num_random_vectors(); ++rand_vec_i ) {
      
        *oss << endl << "Random vector tests = " << rand_vec_i << endl;

        OSTab tab(oss);
      
        *oss << endl << "v1 = randomize(-1,+1); ...\n" ;
        RCP<MultiVectorBase<Scalar> > v1 = createMembers(domain,loc_num_rhs);
        dRand->randomize(&*v1);
        if(dump_all()) *oss << endl << "v1 =\n" << describe(*v1,verbLevel);
      
        *oss << endl << "v2 = randomize(-1,+1); ...\n" ;
        RCP<MultiVectorBase<Scalar> > v2 = createMembers(range,loc_num_rhs);
        rRand->randomize(&*v2);
        if(dump_all()) *oss << endl << "v2 =\n" << describe(*v2,verbLevel);
      
        *oss << endl << "v3 = 0.5*op*v1 ...\n" ;
        RCP<MultiVectorBase<Scalar> > v3 = createMembers(range,loc_num_rhs);
        apply( op, NONCONJ_ELE, *v1, &*v3, r_half );
        if(dump_all()) *oss << endl << "v3 =\n" << describe(*v3,verbLevel);
      
        *oss << endl << "v4 = 0.5*op'*v2 ...\n" ;
        RCP<MultiVectorBase<Scalar> > v4 = createMembers(domain,loc_num_rhs);
        applyTranspose( op, CONJ_ELE, *v2, &*v4, d_half );
        if(dump_all()) *oss << endl << "v4 =\n" << describe(*v4,verbLevel);

        std::vector<Scalar> prod_v4_v1(loc_num_rhs);
        domain->scalarProds(*v4,*v1,&prod_v4_v1[0]);
        std::vector<Scalar> prod_v2_v3(loc_num_rhs);
        range->scalarProds(*v2,*v3,&prod_v2_v3[0]);
        
        result = testRelErrors(
          loc_num_rhs
          ,"<v4,v1>", &prod_v4_v1[0]
          ,"<v2,v3>", &prod_v2_v3[0]
          ,"adjoint_error_tol()", adjoint_error_tol()
          ,"adjoint_warning_tol()", adjoint_warning_tol()
          ,&*oss
          );
        if(!result) these_results = false;
        
      }
    }
    else {
      *oss << endl << "Adjoint operator not supported, skipping check!\n";
    }

    printTestResults(these_results,ossStore.str(),show_all_tests(),&success,OSTab(out).get());

  }
  else {
    *out << endl << "this->check_adjoint()==false:"
         << " Skipping check for the agreement of the adjoint and forward operators!\n";
  }


  if( check_for_symmetry() ) {

    *out << endl << "this->check_for_symmetry()==true: Performing check of symmetry ... ";


    std::ostringstream ossStore;
    RCP<FancyOStream> oss = fancyOStream(rcpFromRef(ossStore));
    ossStore.copyfmt(*out);
    bool these_results = true;
    
    SymmetricLinearOpTester<Scalar>::checkSymmetry(
      op,&*dRand, *oss, loc_num_rhs,num_random_vectors(), verbLevel,dump_all(),
      symmetry_error_tol(), symmetry_warning_tol(),
      outArg(these_results)
      );
    
    printTestResults(these_results,ossStore.str(),show_all_tests(),&success,OSTab(out).get());
    
  }
  else {
    *out << endl << "this->check_for_symmetry()==false: Skipping check of symmetry ...\n";
  }
  
  if(success)
    *out << endl <<"Congratulations, this LinearOpBase object seems to check out!\n";
  else
    *out << endl <<"Oh no, at least one of the tests performed with this LinearOpBase object failed (see above failures)!\n";
  *out << endl << "*** Leaving LinearOpTester<"<<RST::name()<<","<<DST::name()<<">::check(...)\n";

  return success;

}


template<class Scalar>
bool LinearOpTester<Scalar>::check(
  const LinearOpBase<Scalar> &op,
  const Ptr<Teuchos::FancyOStream> &out
  ) const
{
  using Teuchos::null;
  return check(op, null, null, out);
}


template<class Scalar>
bool LinearOpTester<Scalar>::compare(
  const LinearOpBase<Scalar>  &op1
  ,const LinearOpBase<Scalar> &op2
  ,MultiVectorRandomizerBase<Scalar>      *domainRandomizer
  ,Teuchos::FancyOStream                        *out_arg
  ) const
{

  using std::endl;
  using Teuchos::arrayArg;
  using Teuchos::FancyOStream;
  using Teuchos::OSTab;
  typedef Teuchos::ScalarTraits<Scalar>  RST;
  typedef Teuchos::ScalarTraits<Scalar> DST;
  bool success = true, result;
  const int loc_num_rhs = this->num_rhs();
  const Scalar  r_half = Scalar(0.5)*RST::one();
  RCP<FancyOStream> out = Teuchos::rcp(out_arg,false);
  const Teuchos::EVerbosityLevel verbLevel = (dump_all()?Teuchos::VERB_EXTREME:Teuchos::VERB_MEDIUM);

  OSTab tab(out,1,"THYRA");

  if(out.get()) {
    *out
      << endl << "*** Entering LinearOpTester<"<<RST::name()<<","<<DST::name()<<">::compare(op1,op2,...) ...\n";
    if(show_all_tests())
      *out << endl << "describe op1:\n" << Teuchos::describe(op1,verbLevel);
    else
      *out << endl << "describe op1: " << op1.description() << endl;
    if(show_all_tests())
      *out << endl << "describe op2:\n" << Teuchos::describe(op2,verbLevel);
    else
      *out << endl << "describe op2: " << op2.description() << endl;
  }

  RCP< MultiVectorRandomizerBase<Scalar> > dRand;
  if(domainRandomizer)  dRand = Teuchos::rcp(domainRandomizer,false);
  else                  dRand = Teuchos::rcp(new UniversalMultiVectorRandomizer<Scalar>());

  RCP<const VectorSpaceBase<Scalar> >  range  = op1.range();
  RCP<const VectorSpaceBase<Scalar> > domain = op1.domain();

  if(out.get()) *out << endl << "Checking that range and domain spaces are compatible ... ";

  {

    std::ostringstream ossStore;
    RCP<FancyOStream> oss = Teuchos::rcp(new FancyOStream(Teuchos::rcp(&ossStore,false)));
    if(out.get()) ossStore.copyfmt(*out);
    bool these_results = true;

    *oss << endl << "op1.domain()->isCompatible(*op2.domain()) ? ";
    result = op1.domain()->isCompatible(*op2.domain());
    if(!result) these_results = false;
    *oss << passfail(result) << endl;
    
    *oss << endl << "op1.range()->isCompatible(*op2.range()) ? ";
    result = op1.range()->isCompatible(*op2.range());
    if(!result) these_results = false;
    *oss << passfail(result) << endl;

    printTestResults(these_results,ossStore.str(),show_all_tests(),&success,OSTab(out).get());

  }

  if(!success) {
    if(out.get()) *out << endl << "Skipping further checks since operators are not compatible!\n";
    return success;
  }

  if(out.get()) *out << endl << "Checking that op1 == op2 ... ";

  {

    std::ostringstream ossStore;
    RCP<FancyOStream> oss = Teuchos::rcp(new FancyOStream(Teuchos::rcp(&ossStore,false)));
    if(out.get()) ossStore.copyfmt(*out);
    bool these_results = true;

    *oss
      << endl << "Checking that op1 and op2 produce the same results:\n"
      << endl << "  0.5*op1*v1 == 0.5*op2*v1"
      << endl << "  \\________/    \\________/"
      << endl << "      v2            v3"
      << endl << ""
      << endl << "   |sum(v2)| == |sum(v3)|"
      << endl;

    for( int rand_vec_i = 1; rand_vec_i <= num_random_vectors(); ++rand_vec_i ) {
      
      *oss << endl << "Random vector tests = " << rand_vec_i << endl;

      OSTab tab2(oss);
      
      if(dump_all()) *oss << endl << "v1 = randomize(-1,+1); ...\n" ;
      RCP<MultiVectorBase<Scalar> > v1 = createMembers(domain,loc_num_rhs);
      dRand->randomize(&*v1);
      if(dump_all()) *oss << endl << "v1 =\n" << *v1;
      
      if(dump_all()) *oss << endl << "v2 = 0.5*op1*v1 ...\n" ;
      RCP<MultiVectorBase<Scalar> > v2 = createMembers(range,loc_num_rhs);
      apply( op1, NONCONJ_ELE, *v1, &*v2, r_half );
      if(dump_all()) *oss << endl << "v2 =\n" << *v2;
      
      if(dump_all()) *oss << endl << "v3 = 0.5*op2*v1 ...\n" ;
      RCP<MultiVectorBase<Scalar> > v3 = createMembers(range,loc_num_rhs);
      apply( op2, NONCONJ_ELE, *v1, &*v3, r_half );
      if(dump_all()) *oss << endl << "v3 =\n" << *v3;
      
      std::vector<Scalar> sum_v2(loc_num_rhs), sum_v3(loc_num_rhs);
      sums(*v2,&sum_v2[0]);
      sums(*v3,&sum_v3[0]);
      
      result = testRelErrors(
        loc_num_rhs
        ,"sum(v2)", &sum_v2[0]
        ,"sum(v3)", &sum_v3[0]
        ,"linear_properties_error_tol()", linear_properties_error_tol()
        ,"linear_properties_warning_tol()", linear_properties_warning_tol()
        ,&*oss
        );
      if(!result) these_results = false;
      
    }

    printTestResults(these_results, ossStore.str(), show_all_tests(),
      &success, OSTab(out).get() );

  }
  
  if(out.get()) {
    if(success)
      *out << endl <<"Congratulations, these two LinearOpBase objects seem to be the same!\n";
    else
      *out << endl <<"Oh no, these two LinearOpBase objects seem to be different (see above failures)!\n";
    *out << endl << "*** Leaving LinearOpTester<"<<RST::name()<<","<<DST::name()<<">::compare(...)\n";
  }

  return success;

}


template<class Scalar>
bool LinearOpTester<Scalar>::compare(
  const LinearOpBase<Scalar>  &op1
  ,const LinearOpBase<Scalar> &op2
  ,Teuchos::FancyOStream                        *out
  ) const
{
  return compare(op1,op2,NULL,out);
}


} // namespace Thyra


#endif // THYRA_LINEAR_OP_TESTER_DEF_HPP
