/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#ifndef stk_mesh_BucketRepository_hpp
#define stk_mesh_BucketRepository_hpp

#include <stk_mesh/base/Types.hpp>
#include <stk_mesh/base/Bucket.hpp>
#include <stk_mesh/base/Iterators.hpp>
#include <stk_mesh/baseImpl/Partition.hpp>


#define USE_STK_MESH_IMPL_PARTITION

namespace stk {
namespace mesh {
namespace impl {

class EntityRepository;

class BucketRepository {
public:
  ~BucketRepository();
  BucketRepository(
      BulkData & mesh,
      unsigned bucket_capacity,
      unsigned entity_rank_count,
      EntityRepository & entity_repo
      );

  /** \brief  Query all buckets of a given entity rank */
  const std::vector<Bucket*> & buckets( EntityRank rank ) const
  {
    ThrowAssertMsg( rank < m_buckets.size(), "Invalid entity rank " << rank );
#ifdef USE_STK_MESH_IMPL_PARTITION
    // ThrowAssertMsg(!m_need_sync_from_partitions[rank], "Getting buckets without sync-ing with partitions");
    if (m_need_sync_from_partitions[rank])
    {
        const_cast<BucketRepository *>(this)->sync_from_partitions();
    }
#endif

    return m_buckets[ rank ];
  }

#ifndef USE_STK_MESH_IMPL_PARTITION
  /*  Entity modification consequences:
   *  1) Change entity relation => update via part relation => change parts
   *  2) Change parts => update forward relations via part relation
   *                  => update via field relation
   */
  void remove_entity( Bucket * , unsigned );

#endif

  //------------------------------------
  /** \brief  Query the upper bound on the number of mesh entities
    *         that may be associated with a single bucket.
    */
  unsigned bucket_capacity() const { return m_bucket_capacity; }


  //------------------------------------

  /** \brief  Rotate the field data of multistate fields.
   *
   *  <PRE>
   *  Rotation of states:
   *    StateN   <- StateNP1 (StateOld <- StateNew)
   *    StateNM1 <- StateN   (StateNM1 <- StateOld)
   *    StateNM2 <- StateNM1
   *    StateNM3 <- StateNM2
   *    StateNM3 <- StateNM2
   *  </PRE>
   */
  void update_field_data_states() const ;

  // Destroy the last empty bucket in a partition:
  void destroy_bucket( const unsigned & entity_rank , Bucket * last );
  void destroy_bucket( Bucket * bucket );
  void declare_nil_bucket();
  Bucket * get_nil_bucket() const { return m_nil_bucket; }

#ifndef USE_STK_MESH_IMPL_PARTITION
  Bucket * declare_bucket(
      const unsigned entity_rank ,
      const unsigned part_count ,
      const unsigned part_ord[] ,
      const std::vector< FieldBase * > & field_set
      );
#endif

  void copy_fields( Bucket & k_dst , unsigned i_dst ,
                           Bucket & k_src , unsigned i_src )
  { k_dst.replace_fields(i_dst,k_src,i_src); }

  void initialize_fields( Bucket & k_dst , unsigned i_dst );

  void internal_sort_bucket_entities();

  void optimize_buckets();

#ifndef USE_STK_MESH_IMPL_PARTITION
  void add_entity_to_bucket( Entity entity, Bucket & bucket )
  {
    bucket.replace_entity( bucket.size() , entity ) ;
    bucket.increment_size();
  }
#endif

  void internal_propagate_relocation( Entity );

  AllBucketsRange get_bucket_range() const
  {
    return stk::mesh::get_bucket_range(m_buckets);
  }

  AllBucketsRange get_bucket_range(EntityRank entity_rank) const
  {
    std::vector< std::vector<Bucket*> >::const_iterator itr = m_buckets.begin() + entity_rank;
    return stk::mesh::get_bucket_range(m_buckets, itr);
  }

  ////
  //// Experimental section.
  ////

  friend class Partition;

  Partition *get_or_create_partition(const unsigned arg_entity_rank ,
                                     const PartVector &parts);

  Partition *get_or_create_partition(const unsigned arg_entity_rank ,
                                     const OrdinalVector &parts);

  // Update m_buckets from the partitions.
  void sync_from_partitions();
  void sync_from_partitions(EntityRank rank);

  // Used in unit tests.  Returns the current partitions.
  std::vector<Partition *> get_partitions(EntityRank rank);

  // Used in unit tests. Delete the Partitions in m_partitions, clear it, and then (re-)build
  // the Partitions from the m_buckets.
  void sync_to_partitions();

  void babbleForEntity(EntityRank entity_rank, EntityId entity_id);


private:
  BucketRepository();

  BulkData                            & m_mesh ; // Associated Bulk Data Aggregate
  unsigned                              m_bucket_capacity ; // Maximum number of entities per bucket
  std::vector< std::vector<Bucket*> >   m_buckets ; // Vector of bucket pointers by rank
  Bucket                              * m_nil_bucket ; // nil bucket

  EntityRepository                    & m_entity_repo ;

  std::vector<std::vector<Partition *> >         m_partitions;  // Experimental.
  std::vector<bool>                              m_need_sync_from_partitions;
};

} // namespace impl
} // namespace mesh
} // namespace stk


#include <stk_mesh/baseImpl/Partition_inl.hpp>

#endif // stk_mesh_BucketRepository_hpp
