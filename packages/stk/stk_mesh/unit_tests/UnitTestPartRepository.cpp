/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/


#include <stddef.h>                     // for NULL
#include <stdexcept>                    // for runtime_error
#include <stk_mesh/base/MetaData.hpp>   // for MetaData
#include <stk_mesh/baseImpl/PartRepository.hpp>  // for PartRepository
#include <gtest/gtest.h>
#include "Shards_CellTopologyData.h"    // for CellTopologyData
#include "stk_mesh/base/Types.hpp"      // for PartVector
#include "stk_topology/topology.hpp"    // for topology, etc
namespace stk { namespace mesh { class BulkData; } }
namespace stk { namespace mesh { class Part; } }





using stk::mesh::MetaData;
using stk::mesh::BulkData;
using stk::mesh::Part;
using stk::mesh::impl::PartRepository;

class UnitTestPartRepository
{
public:
  UnitTestPartRepository();
  ~UnitTestPartRepository() {}

   const int spatial_dimension;
   MetaData meta;
   stk::mesh::impl::PartRepository partRepo;
   stk::mesh::impl::PartRepository partRepo_1;
   stk::mesh::impl::PartRepository partRepo_2;

   stk::mesh::Part * universal_part;
   stk::mesh::Part * part_A;
   stk::mesh::Part * part_B;
   stk::mesh::Part * part_C;
   stk::mesh::Part * part_D;
   stk::mesh::Part * part_1_A;
   stk::mesh::Part * part_1_B;
   stk::mesh::Part * part_2_A;
   const CellTopologyData * singleton;
};

UnitTestPartRepository::UnitTestPartRepository()
  : spatial_dimension(3)
  , meta( spatial_dimension )
  , partRepo( &meta )
  , partRepo_1( &meta )
  , partRepo_2( &meta )
  , universal_part(      partRepo.universal_part()    )
  , part_A   (           partRepo.declare_part("A",stk::topology::NODE_RANK) )
  , part_B   (           partRepo.declare_part("B",stk::topology::NODE_RANK) )
  , part_C   (           partRepo.declare_part("C",stk::topology::NODE_RANK) )
  , part_D   (           partRepo.declare_part("D",stk::topology::EDGE_RANK) )
  , part_1_A (           partRepo_1.declare_part("A",stk::topology::NODE_RANK) )
  , part_1_B (           partRepo_1.declare_part("B",stk::topology::NODE_RANK) )
  , part_2_A (           partRepo_2.declare_part("A",stk::topology::NODE_RANK) )
  , singleton ( NULL )
{
 meta.commit();
}

namespace {

TEST( UnitTestPartRepository, construct )
{
  UnitTestPartRepository upr;
  EXPECT_TRUE( upr.universal_part != NULL );
}

TEST( UnitTestPartRepository, universal_in_subset )
{
  UnitTestPartRepository upr;
  ASSERT_THROW(
    upr.partRepo.declare_subset(*upr.part_A,*upr.universal_part),
    std::runtime_error
    );
}

TEST( UnitTestPartRepository, subset_equal_superset )
{
  UnitTestPartRepository upr;
  ASSERT_THROW(
    upr.partRepo.declare_subset(*upr.part_A,*upr.part_A),
    std::runtime_error
    );
}


TEST( UnitTestPartRepository, circular_subset )
{
  UnitTestPartRepository upr;
  upr.partRepo.declare_subset(*upr.part_A,*upr.part_B);
  upr.partRepo.declare_subset(*upr.part_B,*upr.part_C);
  ASSERT_THROW(
    upr.partRepo.declare_subset(*upr.part_C,*upr.part_A),
    std::runtime_error
    );
}

TEST( UnitTestPartRepository, inconsistent_rank_subset )
{
  UnitTestPartRepository upr;
  // lower rank cannot contain higher rank:
  ASSERT_THROW(
    upr.partRepo.declare_subset(*upr.part_A,*upr.part_D),
    std::runtime_error
    );
  // higher rank can contain lower rank:
  ASSERT_NO_THROW(
    upr.partRepo.declare_subset(*upr.part_D,*upr.part_A)
    );
}

TEST( UnitTestPartRepository, two_part_repositories )
{
  UnitTestPartRepository upr;
  // subset/superset parts must come from same part repository
  ASSERT_THROW(
    upr.partRepo_1.declare_subset(*upr.part_1_A,*upr.part_2_A),
    std::runtime_error
    );
}

//Test covers declare_attribute_no_delete in PartRepository.hpp and PartImpl.hpp
TEST( UnitTestPartRepository, declare_attribute_no_delete )
{
  UnitTestPartRepository upr;
  upr.partRepo.declare_attribute_no_delete(*upr.part_A, upr.singleton);

}

TEST( UnitTestPartRepository, get_all_parts )
{
  UnitTestPartRepository upr;
  const stk::mesh::PartVector & pv = upr.partRepo.get_all_parts();
  ASSERT_EQ( pv.size(), 5u );
}

TEST( UnitTestPartRepository, get_mesh_parts )
{
  UnitTestPartRepository upr;
  stk::mesh::PartVector mpv = upr.partRepo.get_mesh_parts();
  ASSERT_EQ( mpv.size(), 4u );
}


}
