/*------------------------------------------------------------------------*/
/*                 Copyright 2010, 2011 Sandia Corporation.               */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/


#include <sstream>
#include <stdexcept>

#include <stk_util/unit_test_support/stk_utest_macros.hpp>

#include <stk_util/parallel/Parallel.hpp>

#include <stk_mesh/base/MetaData.hpp>

#include <stk_mesh/baseImpl/PartRepository.hpp>
#include <stk_mesh/baseImpl/EntityRepository.hpp>
#include <stk_mesh/baseImpl/FieldBaseImpl.hpp>
#include <stk_mesh/base/CoordinateSystems.hpp>
#include <stk_mesh/base/FindRestriction.hpp>

#include <Shards_BasicTopologies.hpp>

namespace stk {
namespace mesh {

class UnitTestFieldImpl {
public:
  UnitTestFieldImpl() {}

  void testFieldRestriction();

};

}//namespace mesh
}//namespace stk

namespace {

STKUNIT_UNIT_TEST(UnitTestFieldRestriction, testUnit)
{
  stk::mesh::UnitTestFieldImpl ufield;
  ufield.testFieldRestriction();
}

}//namespace <anonymous>

//----------------------------------------------------------------------

namespace stk {
namespace mesh {

namespace {

// Simple tags for testing field dimensions

SHARDS_ARRAY_DIM_TAG_SIMPLE_DECLARATION( ATAG )
SHARDS_ARRAY_DIM_TAG_SIMPLE_DECLARATION( BTAG )
SHARDS_ARRAY_DIM_TAG_SIMPLE_DECLARATION( CTAG )
SHARDS_ARRAY_DIM_TAG_SIMPLE_DECLARATION( DTAG )

SHARDS_ARRAY_DIM_TAG_SIMPLE_IMPLEMENTATION( ATAG )
SHARDS_ARRAY_DIM_TAG_SIMPLE_IMPLEMENTATION( BTAG )
SHARDS_ARRAY_DIM_TAG_SIMPLE_IMPLEMENTATION( CTAG )
SHARDS_ARRAY_DIM_TAG_SIMPLE_IMPLEMENTATION( DTAG )

}

//----------------------------------------------------------------------
// Test field restrictions: the mapping of ( field , part ) -> dimensions

void UnitTestFieldImpl::testFieldRestriction()
{
  unsigned stride[8] ;

  stride[0] = 10 ;
  for ( unsigned i = 1 ; i < 8 ; ++i ) {
    stride[i] = ( i + 1 ) * stride[i-1] ;
  }

  std::vector< std::string > dummy_names(4, "dummy");

  MetaData meta_data(0 /*dim*/,dummy_names);

  const FieldVector  & allocated_fields = meta_data.get_fields();

  //------------------------------

  typedef stk::mesh::Field<double,stk::mesh::Cartesian> VectorField;

  FieldBase * const f2 =
    &meta_data.declare_field<VectorField>( stk::topology::NODE_RANK, std::string("F2"), 1/* # states */ );

  //------------------------------

//  FieldBase * const f3 = &meta_data.declare_field<VectorField>( std::string("F3"), 2/* #states*/);
  FieldBase * const nodeField = &meta_data.declare_field<VectorField>( stk::topology::NODE_RANK, std::string("nodeField"), 2/* #states*/);
  FieldBase * const edgeField = &meta_data.declare_field<VectorField>( stk::topology::EDGE_RANK, std::string("edgeField"), 2/* #states*/);
  FieldBase * const faceField = &meta_data.declare_field<VectorField>( stk::topology::FACE_RANK, std::string("faceField"), 2/* #states*/);

  FieldBase * const f3_old = nodeField->field_state( StateOld ) ;

  //------------------------------
  // Test for correctness of vector of declared fields.
  STKUNIT_ASSERT_EQ(7u,  allocated_fields.size());
  STKUNIT_ASSERT( f2 == allocated_fields[0] );
  STKUNIT_ASSERT( nodeField == allocated_fields[1] );

  //------------------------------
  // Test for correctness of field internal state access:

  STKUNIT_ASSERT( f2     == f2->field_state( StateNone ) );
  STKUNIT_ASSERT( NULL   == f2->field_state( StateOld ) );
  STKUNIT_ASSERT( nodeField     == nodeField->field_state( StateNew ) );
  STKUNIT_ASSERT( f3_old == nodeField->field_state( StateOld ) );
  STKUNIT_ASSERT( NULL   == nodeField->field_state( StateNM1 ) );
  STKUNIT_ASSERT( nodeField     == f3_old->field_state( StateNew ) );
  STKUNIT_ASSERT( f3_old == f3_old->field_state( StateOld ) );
  STKUNIT_ASSERT( NULL   == f3_old->field_state( StateNM1 ) );

  //------------------------------
  // Declare some parts for restrictions:

  Part & pA = meta_data.declare_part( std::string("A") , 0 );
  Part & pB = meta_data.declare_part( std::string("B") , 0 );
  Part & pC = meta_data.declare_part( std::string("C") , 0 );
  Part & pD = meta_data.declare_part( std::string("D") , 0 );

  // Declare three restrictions:

  meta_data.declare_field_restriction(*nodeField, pA , stride[nodeField->field_array_rank()-1], stride[0] );
  meta_data.declare_field_restriction(*edgeField, pB , stride[edgeField->field_array_rank()], stride[1] );
  meta_data.declare_field_restriction(*faceField, pC , stride[faceField->field_array_rank()+1], stride[2] );

  // Check for correctness of restrictions:

  STKUNIT_ASSERT( nodeField->restrictions().size() == 1 );
  STKUNIT_ASSERT( nodeField->restrictions()[0] ==
                  FieldRestriction( pA ) );
  STKUNIT_ASSERT( edgeField->restrictions()[0] ==
                  FieldRestriction( pB ) );
  STKUNIT_ASSERT( faceField->restrictions()[0] ==
                  FieldRestriction( pC ) );

  meta_data.declare_field_restriction(*nodeField , pB , stride[nodeField->field_array_rank()], stride[1] );

  STKUNIT_ASSERT_EQUAL( nodeField->max_size( stk::topology::NODE_RANK ) , 20u );

  //------------------------------
  // Check for error detection of bad stride:
  {
    unsigned bad_stride[4] = { 5 , 4 , 6 , 3 };
    STKUNIT_ASSERT_THROW(
      meta_data.declare_field_restriction(*nodeField , pA, 5*4*6 , bad_stride[0] ),
      std::runtime_error
    );
    STKUNIT_ASSERT_EQ(2u, nodeField->restrictions().size());
  }

  // Check for error detection in re-declaring an incompatible
  // field restriction.
  {
    STKUNIT_ASSERT_THROW(
      meta_data.declare_field_restriction(*nodeField , pA , stride[nodeField->field_array_rank()], stride[1] ),
      std::runtime_error
    );
    STKUNIT_ASSERT_EQ(2u, nodeField->restrictions().size());
  }

  // Verify and clean out any redundant restructions:

  STKUNIT_ASSERT( nodeField->restrictions().size() == 2 );

  //------------------------------
  // Introduce a redundant restriction, clean it, and
  // check that it was cleaned.

  std::cout<<"pA ord: "<<pA.mesh_meta_data_ordinal()<<", pD ord: "<<pD.mesh_meta_data_ordinal()<<std::endl;
  meta_data.declare_part_subset( pD, pA );
  meta_data.declare_field_restriction(*f2 , pA , stride[f2->field_array_rank()-1], stride[0] );
  meta_data.declare_field_restriction(*f2 , pD , stride[f2->field_array_rank()-1], stride[0] );

  STKUNIT_ASSERT( f2->restrictions().size() == 1 );

  {
    const FieldBase::Restriction & rA = stk::mesh::find_restriction(*f2, stk::topology::NODE_RANK, pA );
    const FieldBase::Restriction & rD = stk::mesh::find_restriction(*f2, stk::topology::NODE_RANK, pD );
    STKUNIT_ASSERT( & rA == & rD );
    STKUNIT_ASSERT( rA.selector() == pD );
  }

  //------------------------------
  // Introduce a new restriction, then introduce a
  // subset-superset relationship that renders the new restriction
  // redundant and incompatible.
  // Check that the verify_and_clean_restrictions method detects
  // this error condition.
  {
    meta_data.declare_field_restriction(*f2 , pB , stride[f2->field_array_rank()], stride[1] );
    STKUNIT_ASSERT_THROW(
      meta_data.declare_part_subset( pD, pB ),
      std::runtime_error
    );
  }

  //Coverage for error from print_restriction in FieldBaseImpl.cpp when there is no stride (!stride[i])
  //Call print_restriction from insert_restriction
  {
    unsigned arg_no_stride[2];

    arg_no_stride[0] = 1;
    arg_no_stride[1] = 0;

    STKUNIT_ASSERT_THROW(
      meta_data.declare_field_restriction(*f2, pA, 0, arg_no_stride[0]),
      std::runtime_error
    );
  }
}


}
}

