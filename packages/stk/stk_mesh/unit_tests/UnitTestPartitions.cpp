#include <stdexcept>
#include <stk_util/unit_test_support/stk_utest_macros.hpp>

#include <stk_mesh/base/Selector.hpp>
#include <stk_mesh/base/Bucket.hpp>
#include <stk_mesh/base/Part.hpp>
#include <stk_mesh/base/Types.hpp>
#include <stk_mesh/base/GetBuckets.hpp>
#include <stk_util/environment/WallTime.hpp>

#include <stk_util/parallel/Parallel.hpp>

#include <stk_mesh/fixtures/SelectorFixture.hpp>

#ifdef USE_STK_MESH_IMPL_PARTITION

namespace {

using stk::mesh::fixtures::SelectorFixture ;


// Borrow a lot from UnitTestSelector.  Bulk up the SelectorFixture to have parts
// with enough entities so that each partition (bucket family) comprises multiple
// buckets.

stk::mesh::EntityId
addEntitiesToFixture(SelectorFixture& fixture, stk::mesh::EntityId start_id, size_t num_to_add,
                     const std::vector<stk::mesh::Part*> &partMembership,
                     std::vector<stk::mesh::Entity> &collector)
{
    stk::mesh::EntityId ent_id = start_id;
    for (size_t i = 0; i < num_to_add; ++i)
    {
        stk::mesh::Entity ent =
                fixture.m_bulk_data.declare_entity(stk::mesh::MetaData::NODE_RANK,
                                                   ent_id, partMembership);
        collector.push_back(ent);
        ++ent_id;
    }
    return ent_id;
}

void initialize(SelectorFixture& fixture,
                std::vector<stk::mesh::Entity> &ec1_arg,
                std::vector<stk::mesh::Entity> &ec2_arg,
                std::vector<stk::mesh::Entity> &ec3_arg,
                std::vector<stk::mesh::Entity> &ec4_arg,
                std::vector<stk::mesh::Entity> &ec5_arg
                )
{
    fixture.m_meta_data.commit();
    fixture.m_bulk_data.modification_begin();
    fixture.generate_mesh();

    const size_t bucket_size = 1000;     // Default value for BucketRepository constructor.
    const size_t lb_num_buckets_per_partition = 3;

    const size_t bf_size = bucket_size * lb_num_buckets_per_partition;
    stk::mesh::EntityId ent_id = 1001;   // Want to keep numerical alignment.
    std::vector<stk::mesh::Part*> partMembership;

    // Note that the loop variables start at 1 because SelectorFixture::generate_mesh() has
    // already created an Entity in each partition.

    // Entities in collection 1 are contained in PartA
    partMembership.clear();
    partMembership.push_back( & fixture.m_partA );
    ent_id = addEntitiesToFixture(fixture, ent_id, bf_size - 1, partMembership, ec1_arg);

    // Entities in collection 2 are in PartA and PartB
    ++ent_id;     // For numerical alignment.
    partMembership.clear();
    partMembership.push_back( & fixture.m_partA );
    partMembership.push_back( & fixture.m_partB );
    ent_id = addEntitiesToFixture(fixture, ent_id, bf_size - 1, partMembership, ec2_arg);

    // Entities in collection 3 are in PartB and PartC
    ++ent_id;
    partMembership.clear();
    partMembership.push_back( & fixture.m_partB );
    partMembership.push_back( & fixture.m_partC );
    ent_id = addEntitiesToFixture(fixture, ent_id, bf_size - 1, partMembership, ec3_arg);

    // Entities in collection 4 are in PartC
    ++ent_id;
    partMembership.clear();
    partMembership.push_back( & fixture.m_partC );
    ent_id = addEntitiesToFixture(fixture, ent_id, bf_size - 1, partMembership, ec4_arg);

    // Entities in collection 5 are not contained in any Part
    ++ent_id;
    partMembership.clear();
    ent_id = addEntitiesToFixture(fixture, ent_id, bf_size - 1, partMembership, ec5_arg);

    STKUNIT_ASSERT(fixture.m_bulk_data.modification_end());
}

void initialize(SelectorFixture& fixture)
{
    std::vector<stk::mesh::Entity> ec1;
    std::vector<stk::mesh::Entity> ec2;
    std::vector<stk::mesh::Entity> ec3;
    std::vector<stk::mesh::Entity> ec4;
    std::vector<stk::mesh::Entity> ec5;

    initialize(fixture, ec1, ec2, ec3, ec4, ec5);
}

void initialize_unsorted(SelectorFixture& fixture)
{
    initialize(fixture);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fixture.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::impl::Partition *> partitions =
            bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    size_t num_partitions = partitions.size();
    STKUNIT_EXPECT_EQ(num_partitions, 5u);

    for (size_t i = 0; i < num_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *partitions[i];
        partition.reverseEntityOrderWithinBuckets();
    }
}

// Initialize field data in the fixture to correspond to the EntityIds of the entities.
void initialize_data(SelectorFixture& fix)
{
    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::impl::Partition *> partitions = bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    size_t num_partitions = partitions.size();

    for (size_t i = 0; i < num_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *partitions[i];
        for (std::vector<stk::mesh::Bucket *>::iterator bkt_i= partition.begin(); bkt_i != partition.end(); ++bkt_i)
        {
            stk::mesh::Bucket &bkt = **bkt_i;
            double *field_data = bkt.field_data(fix.m_fieldABC, bkt[0]);
            if (!field_data)
            {
                continue;
            }

            size_t bkt_size = bkt.size();
            for (size_t k = 0; k < bkt_size; ++k)
            {
                stk::mesh::Entity curr_ent = bkt[k];
                field_data[k] = curr_ent.identifier();
            }
        }
    }
}


bool check_all_are_members(const stk::mesh::Selector selects,
                           const std::vector<stk::mesh::Entity> &cands)
{
    for (size_t i = 0; i < cands.size(); ++i)
    {
        if (!selects(cands[i]))
            return false;
    }
    return true;
}

bool check_bucket_ptrs(const stk::mesh::Bucket &bucket)
{
    if (bucket.size() == 0 )
    {
        std::cout << "Bucket has size zero!" << std::endl;
        return false;
    }

    stk::mesh::Bucket::iterator e_i = bucket.begin();
    stk::mesh::Bucket::iterator e_e = bucket.end();
    for(; e_i != e_e; ++e_i)
    {
        if (&bucket != e_i->bucket_ptr())
            return false;
    }
    return true;
}

bool check_strictly_ordered(const stk::mesh::Bucket &bucket)
{
    if (bucket.size() == 0 )
        return true;

    stk::mesh::EntityLess eless;
    stk::mesh::Bucket::iterator e_i = bucket.begin();
    stk::mesh::Bucket::iterator e_last = bucket.end() - 1;
    for(; e_i != e_last; ++e_i)
    {
        if (!eless(*e_i, *(e_i + 1)))
            return false;
    }
    return true;
}

template <typename Data_T>
bool check_nonempty_strictly_ordered(Data_T data[], size_t sz, bool reject_0_lt_0 = true )
{
    if (sz == 0)
        return false;

    for (size_t i = 0; i < sz - 1; ++i)
    {
        if ((data[i] >= data[i + 1]) && reject_0_lt_0)
        {
            std::cout << "i = " << i << ": data[i] = " << data[i]
                      << ", data[i + 1] = " << data[i + 1] << std::endl;
            return false;
        }
    }
    return true;
}

bool check_data_consistent(stk::mesh::Field<double> &data_field, const stk::mesh::Bucket &bucket)
{
    if (bucket.size() == 0 )
        return true;

    const double *field_data = bucket.field_data(data_field, bucket[0]);
    if (field_data)
    {
        size_t num_entities = bucket.size();
        for (size_t i = 0; i < num_entities; ++i)
        {
            stk::mesh::EntityId entity_id = bucket[i].identifier();
            double val = field_data[i];
            if ((val < entity_id - 0.001) || (entity_id + 0.001 < val))
            {
                std::cout << "val = " << val << "  entity_id = " << entity_id << std::endl;
                return false;
            }
        }
    }
    return true;
}


void check_test_partition_invariant(const SelectorFixture& fix,
                                    const stk::mesh::impl::Partition &partition)
{
    const std::vector<unsigned> &partition_key = partition.get_legacy_partition_id();
    for (std::vector<stk::mesh::Bucket *>::const_iterator bkt_i= partition.begin();
            bkt_i != partition.end(); ++bkt_i)
    {
        const stk::mesh::Bucket &bkt = **bkt_i;
        STKUNIT_EXPECT_EQ(&partition, bkt.getPartition() );
        STKUNIT_EXPECT_TRUE(check_bucket_ptrs(bkt));

        // if (bkt.size() > 0)
        // {
        //     std::cout << "Checking bucket beginning with " << bkt[0] << " and ending with "
        //               << bkt[bkt.size() - 1] << std::endl;
        // }

        double *field_data = bkt.field_data(fix.m_fieldABC, bkt[0]);
        if (field_data)
        {
            STKUNIT_EXPECT_TRUE(check_nonempty_strictly_ordered(field_data, bkt.size()));
        }
        const unsigned *bucket_key = bkt.key();
        for (size_t k = 0; k < partition_key.size() - 1; ++k)
        {
            STKUNIT_EXPECT_EQ(partition_key[k], bucket_key[k]);
        }
    }
}


/** \defgroup stk_mesh_partition_unit "stk::mesh::Partition Unit Testing"
  * \addtogroup stk_mesh_partition_unit
  * \{
  *
  * Selector unit testing environment. <br>
  * A special set of mesh parts and entities are set up in the
  * following configuration for the Selector unit testing.<br>
  * Parts:  PartA, PartB, PartC, PartD, PartU <br>
  * PartU = MetaData.universal_part() <br>
  * Entities:  Entity1, Entity2, Entity3, Entity4, Entity5 <br>
  *
  * PartA contains Entity1, Entity2 <br>
  * PartB contains Entity2, Entity3 <br>
  * PartC contains Entity3, Entity4 <br>
  * PartD contains no entities <br>
  * Entity5 is not contained in any Part <br>
  *
  * <PRE>
  * |----------|--|-------|--|----------|    |-------------|
  * |<--PartA---->|       |<--PartC---->|    |   PartD     |
  * |          |<---PartB--->|          |    |             |
  * |  1       |2 |       |3 |       4  | 5  |             |
  * |          |  |       |  |          |    |             |
  * |          |  |       |  |          |    |             |
  * |----------|--|-------|--|----------|    |-------------|
  * </PRE>
  *
  * Note:  The unit test names use the convention of "i" for
  * intersection, "u" for union, and "c" for complement.
  *
  * */


/// Verify we can construct the unit testing fixture.
STKUNIT_UNIT_TEST( UnitTestPartition, Partition_testInitialize )
{
    std::vector<stk::mesh::Entity> ec1;
    std::vector<stk::mesh::Entity> ec2;
    std::vector<stk::mesh::Entity> ec3;
    std::vector<stk::mesh::Entity> ec4;
    std::vector<stk::mesh::Entity> ec5;

    SelectorFixture fix;

    if (fix.m_bulk_data.parallel_size() > 1)
    {
        return;
    }

    initialize(fix, ec1, ec2, ec3, ec4, ec5);
    initialize_data(fix);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::impl::Partition *> partitions =
            bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    size_t num_partitions = partitions.size();
    STKUNIT_EXPECT_EQ(num_partitions, 5u);

    for (size_t i = 0; i < num_partitions; ++i)
    {
        const stk::mesh::impl::Partition &partition = *partitions[i];
        check_test_partition_invariant(fix, partition);
    }

    stk::mesh::Selector selector;

    selector = fix.m_partA & !fix.m_partB;
    STKUNIT_EXPECT_TRUE(check_all_are_members(selector, ec1));

    selector = fix.m_partA & fix.m_partB;
    STKUNIT_EXPECT_TRUE(check_all_are_members(selector, ec2));

    selector = fix.m_partB & fix.m_partC;
    STKUNIT_EXPECT_TRUE(check_all_are_members(selector, ec3));

    selector = !fix.m_partB & fix.m_partC;
    STKUNIT_EXPECT_TRUE(check_all_are_members(selector, ec4));

    selector = !(fix.m_partA | fix.m_partB | fix.m_partC | fix.m_partD);
    STKUNIT_EXPECT_TRUE(check_all_are_members(selector, ec5));
}

/// Test that we can get the Partitions from a BucketRepository.
STKUNIT_UNIT_TEST( UnitTestPartition, Partition_getPartitions)
{
    SelectorFixture fix;

    if (fix.m_bulk_data.parallel_size() > 1)
    {
        return;
    }
    initialize(fix);
    initialize_data(fix);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::impl::Partition *> partitions =
            bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    size_t num_partitions = partitions.size();
    STKUNIT_EXPECT_EQ(num_partitions, 5u);

    for (size_t i = 0; i < num_partitions; ++i)
    {
        const stk::mesh::impl::Partition &partition = *partitions[i];
        check_test_partition_invariant(fix, partition);
    }
}

/// Test of Partition::compress()
STKUNIT_UNIT_TEST( UnitTestPartition, Partition_testCompress)
{
    SelectorFixture fix;

    if (fix.m_bulk_data.parallel_size() > 1)
    {
        return;
    }
    initialize(fix);
    initialize_data(fix);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::impl::Partition *> partitions =
            bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    size_t num_partitions = partitions.size();
    STKUNIT_EXPECT_EQ(num_partitions, 5u);

    for (size_t i = 0; i < num_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *partitions[i];
        partition.compress(true);
        STKUNIT_EXPECT_EQ(partition.num_buckets(), 1u);
        check_test_partition_invariant(fix, partition);
    }
}

/// Test Partition::sort()
STKUNIT_UNIT_TEST( UnitTestPartition, Partition_testSort)
{
    SelectorFixture fix;

    if (fix.m_bulk_data.parallel_size() > 1)
    {
        return;
    }
    initialize_unsorted(fix);
    initialize_data(fix);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::impl::Partition *> partitions =
            bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    size_t num_partitions = partitions.size();
    STKUNIT_EXPECT_EQ(num_partitions, 5u);

    for (size_t i = 0; i < num_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *partitions[i];
        // check_test_partition_invariant(fix, partition);
        partition.sort(true);
        check_test_partition_invariant(fix, partition);
    }
}

/// Test Partition::remove(.)
STKUNIT_UNIT_TEST( UnitTestPartition, Partition_testRemove)
{
    SelectorFixture fix;

    if (fix.m_bulk_data.parallel_size() > 1)
    {
        return;
    }
    initialize(fix);
    initialize_data(fix);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::impl::Partition *> partitions =
            bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    size_t num_partitions = partitions.size();
    STKUNIT_EXPECT_EQ(num_partitions, 5u);

    for (size_t i = 0; i < num_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *partitions[i];

        size_t old_size = partition.size();
        size_t num_removed = 0;

        // Remove non-last entity in a bucket.
        stk::mesh::Bucket &bkt_0 = **partition.begin();
        partition.remove(bkt_0[0]);
        ++num_removed;

        // Remove last entity in a bucket.
        stk::mesh::Bucket &bkt_1 = **partition.begin();
        stk::mesh::Entity e_last_in_1 = bkt_1[bkt_1.size() - 1];
        partition.remove(e_last_in_1);
        ++num_removed;

        // Empty out the last bucket.
        stk::mesh::Bucket *last_bkt = *(partition.end() - 1);
        while (last_bkt == *(partition.end() - 1))
        {
            partition.remove((*last_bkt)[0]);
            ++num_removed;
        }

        // Need to sort before checking whether the invariant holds.
        partition.sort();

        STKUNIT_EXPECT_EQ(old_size,  partition.size() + num_removed);
        check_test_partition_invariant(fix, partition);
    }

    //
    // Now's a good time to exercise BucketRepository::sync_from_partitions().
    //
    bucket_repository.sync_from_partitions();
    partitions = bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    num_partitions = partitions.size();
    STKUNIT_EXPECT_EQ(num_partitions, 5u);

    for (size_t i = 0; i < num_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *partitions[i];
        check_test_partition_invariant(fix, partition);
    }
}

/// Test Partition::add(.).
STKUNIT_UNIT_TEST( UnitTestPartition, Partition_testAdd)
{
    SelectorFixture fix;

    if (fix.m_bulk_data.parallel_size() > 1)
    {
        return;
    }
    initialize(fix);
    initialize_data(fix);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::impl::Partition *> partitions =
            bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    size_t num_partitions = partitions.size();
    STKUNIT_EXPECT_EQ(num_partitions, 5u);

    std::vector<stk::mesh::Entity> first_entities(num_partitions);
    std::vector<size_t> old_sizes(num_partitions);

    // Get the first entity in each partition.
    for (size_t i = 0; i < num_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *partitions[i];
        stk::mesh::Bucket &bkt = **partition.begin();
        first_entities[i] = bkt[0];
        old_sizes[i] = partition.size();
    }

    // Now remove them from those partitions.
    for (size_t i = 0; i < num_partitions; ++i)
    {
         partitions[i]->remove(first_entities[i]);
    }

    // "Rotate" each former first entity to another partition.
    for (size_t i = 0; i < num_partitions; ++i)
    {
        size_t dst_partition_idx = (i + 1) % num_partitions;
        stk::mesh::impl::Partition &dst_partition = *partitions[dst_partition_idx];
        stk::mesh::Entity entity = first_entities[i];;
        dst_partition.add(entity);
        stk::mesh::Bucket &bkt = entity.bucket();
        double *field_data = bkt.field_data(fix.m_fieldABC, bkt[0]);
        if (field_data)
        {
            field_data[entity.bucket_ordinal()] = entity.identifier();
        }
    }

    // Because the each first entity had a low identifier, it will again be
    // the first entity in its new partition.
    for (size_t i = 0; i < num_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *partitions[i];
        partition.sort();
        STKUNIT_EXPECT_EQ(old_sizes[i], partition.size());
        check_test_partition_invariant(fix, partition);
    }
}

/// Test Partition::move_to(..)
STKUNIT_UNIT_TEST( UnitTestPartition, Partition_testMoveTo)
{
    SelectorFixture fix;

    if (fix.m_bulk_data.parallel_size() > 1)
    {
        return;
    }
    initialize(fix);
    initialize_data(fix);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::impl::Partition *> all_partitions =
            bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    const size_t num_all_partitions = all_partitions.size();
    STKUNIT_EXPECT_EQ(num_all_partitions, 5u);

    const size_t num_data_partitions = num_all_partitions - 1;
    std::vector<stk::mesh::impl::Partition *> data_partitions(num_data_partitions);
    std::copy(all_partitions.begin() + 1, all_partitions.end(), &data_partitions[0]);

    std::vector<stk::mesh::Entity> first_entities(num_data_partitions);
    std::vector<size_t> old_sizes(num_data_partitions);

    // Get the first entity in each partition.
    for (size_t i = 0; i < num_data_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *data_partitions[i];
        stk::mesh::Bucket &bkt = **partition.begin();
        first_entities[i] = bkt[0];
        old_sizes[i] = partition.size();
    }

    // "Rotate" each first entity to another partition.
    for (size_t i = 0; i < num_data_partitions; ++i)
    {
        size_t dst_partition_idx = (i + 1) % num_data_partitions;
        stk::mesh::impl::Partition &dst_partition = *data_partitions[dst_partition_idx];
        data_partitions[i]->move_to(first_entities[i], dst_partition);
    }

    // Because the each first entity had a low identifier, it will again be
    // the first entity in its new partition.
    for (size_t i = 0; i < num_data_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *data_partitions[i];
        partition.sort();

        STKUNIT_EXPECT_EQ(old_sizes[i], partition.size());
        check_test_partition_invariant(fix, partition);
    }
}


// Test the OrdinalVector version of get_or_create_partition(..).
STKUNIT_UNIT_TEST( UnitTestPartition, Partition_testGetOrCreateOV)
{
    SelectorFixture fix;

    if (fix.m_bulk_data.parallel_size() > 1)
    {
        return;
    }
    initialize(fix);
    initialize_data(fix);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::PartOrdinal> parts;
    parts.push_back(fix.m_meta_data.universal_part().mesh_meta_data_ordinal());
    parts.push_back(fix.m_meta_data.locally_owned_part().mesh_meta_data_ordinal());
    parts.push_back(fix.m_partA.mesh_meta_data_ordinal() );
    stk::mesh::impl::Partition *partitionA =
            bucket_repository.get_or_create_partition(stk::mesh::MetaData::NODE_RANK, parts);
    STKUNIT_ASSERT(0 != partitionA);
    STKUNIT_EXPECT_EQ(3u, partitionA->num_buckets());

    parts.push_back(fix.m_partC.mesh_meta_data_ordinal());
    stk::mesh::impl::Partition *partitionAC =
                bucket_repository.get_or_create_partition(stk::mesh::MetaData::NODE_RANK, parts);
    STKUNIT_ASSERT(0 != partitionAC);
    STKUNIT_EXPECT_EQ(0u, partitionAC->num_buckets());

    stk::mesh::impl::Partition *partitionAC_again =
                bucket_repository.get_or_create_partition(stk::mesh::MetaData::NODE_RANK, parts);
    STKUNIT_ASSERT(partitionAC == partitionAC_again);
}


// Test the PartVector version of get_or_create_partition(..).
STKUNIT_UNIT_TEST( UnitTestPartition, Partition_testGetOrCreatePV)
{
    SelectorFixture fix;

    if (fix.m_bulk_data.parallel_size() > 1)
    {
        return;
    }
    initialize(fix);
    initialize_data(fix);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::Part *> parts;
    parts.push_back(&fix.m_meta_data.universal_part());
    parts.push_back(&fix.m_meta_data.locally_owned_part());
    parts.push_back(&fix.m_partA);
    stk::mesh::impl::Partition *partitionA =
            bucket_repository.get_or_create_partition(stk::mesh::MetaData::NODE_RANK, parts);
    STKUNIT_ASSERT(0 != partitionA);
    STKUNIT_EXPECT_EQ(3u, partitionA->num_buckets());

    parts.push_back(&fix.m_partC);
    stk::mesh::impl::Partition *partitionAC =
                bucket_repository.get_or_create_partition(stk::mesh::MetaData::NODE_RANK, parts);
    STKUNIT_ASSERT(0 != partitionAC);
    STKUNIT_EXPECT_EQ(0u, partitionAC->num_buckets());

    stk::mesh::impl::Partition *partitionAC_again =
                bucket_repository.get_or_create_partition(stk::mesh::MetaData::NODE_RANK, parts);
    STKUNIT_ASSERT(partitionAC == partitionAC_again);
}


/** \} */


} // namespace



/// Test Partition::move_to(..) more rigorously
STKUNIT_UNIT_TEST( UnitTestPartition, Partition_testMoveToBetter)
{
    SelectorFixture fix;

    if (fix.m_bulk_data.parallel_size() > 1)
    {
        return;
    }
    initialize(fix);
    initialize_data(fix);

    stk::mesh::impl::BucketRepository &bucket_repository =
            stk::mesh::impl::Partition::getRepository(fix.m_bulk_data);
    bucket_repository.sync_to_partitions();

    std::vector<stk::mesh::impl::Partition *> all_partitions =
            bucket_repository.get_partitions(stk::mesh::MetaData::NODE_RANK);
    size_t num_all_partitions = all_partitions.size();
    STKUNIT_EXPECT_EQ(num_all_partitions, 5u);

    const size_t num_data_partitions = num_all_partitions - 1;
    std::vector<stk::mesh::impl::Partition *> data_partitions(num_data_partitions);
    std::copy(all_partitions.begin() + 1, all_partitions.end(), &data_partitions[0]);

    std::vector<std::vector<stk::mesh::Entity> > entities_to_move(num_data_partitions);
    std::vector<size_t> old_sizes(num_data_partitions);

    // Get the entities from the first bucket in each partition.
    for (size_t i = 0; i < num_data_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *data_partitions[i];
        stk::mesh::Bucket &bkt = **partition.begin();

        size_t bkt_sz = bkt.size();
        STKUNIT_EXPECT_EQ(bkt_sz, bucket_repository.bucket_capacity());
        for (size_t j = 0; j < bkt_sz; ++j)
        {
            entities_to_move[i].push_back(bkt[j]);
        }
        old_sizes[i] = partition.size();
        STKUNIT_EXPECT_EQ(old_sizes[i], 3000u);
    }

    // "Rotate" the entities to another partition.
    for (size_t i = 0; i < num_data_partitions; ++i)
    {
        size_t dst_partition_idx = (i + 1) % num_data_partitions;
        stk::mesh::impl::Partition &dst_partition = *data_partitions[dst_partition_idx];

        size_t num_to_move = entities_to_move[i].size();
        // std::cout << "Moving entities from " << *data_partitions[i] << " to " << dst_partition
        //           << "\n starting with " << entities_to_move[i][0] << " and ending with "
        //           << entities_to_move[i][num_to_move - 1] << std::endl;
        for (size_t j = 0; j < num_to_move; ++j)
        {
            data_partitions[i]->move_to(entities_to_move[i][j], dst_partition);
        }
    }

    for (size_t i = 0; i < num_data_partitions; ++i)
    {
        stk::mesh::impl::Partition &partition = *data_partitions[i];
        partition.sort();

        STKUNIT_EXPECT_EQ(partition.size(), old_sizes[i]);

        // std::cout << "Check " << partition << std::endl;
        // std::cout << "source partition was " << src_partition << std::endl;

        STKUNIT_EXPECT_EQ(old_sizes[i], partition.size());
        check_test_partition_invariant(fix, partition);
    }
}

#endif

