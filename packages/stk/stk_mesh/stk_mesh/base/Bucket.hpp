/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#ifndef stk_mesh_Bucket_hpp
#define stk_mesh_Bucket_hpp

//----------------------------------------------------------------------

#include <iosfwd>
#include <vector>
#include <algorithm>

#include <stk_util/environment/ReportHandler.hpp>


#include <stk_mesh/base/Types.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_mesh/base/Part.hpp>
#include <stk_mesh/base/Entity.hpp>

#include <boost/iterator/transform_iterator.hpp>
#include <boost/iterator/indirect_iterator.hpp>

//----------------------------------------------------------------------

#ifdef SIERRA_MIGRATION

namespace sierra {
namespace Fmwk {

class MeshBulkData;

}
}


#endif

namespace stk {
namespace mesh {

namespace impl {
class BucketRepository;
} // namespace impl


/** \addtogroup stk_mesh_module
 *  \{
 */

/** \brief  Print the \ref stk::mesh::Part "part" names
 *          for which this bucket is a subset.
 */
std::ostream & operator << ( std::ostream & , const Bucket & );

/** \brief  Print the parts and entities of this bucket */
std::ostream &
print( std::ostream & , const std::string & indent , const Bucket & );

// The part count and parts are equal
bool bucket_part_equal( const unsigned * lhs , const unsigned * rhs );

//----------------------------------------------------------------------
/** \brief  Is this bucket a subset of the given
 *          \ref stk::mesh::Part "part"
 */
bool has_superset( const Bucket & ,  const Part & p );

/** \brief  Is this bucket a subset of the given
 *          \ref stk::mesh::Part "part" by partID
 */
bool has_superset( const Bucket & ,  const unsigned & ordinal );

/** \brief  Is this bucket a subset of all of the given
 *          \ref stk::mesh::Part "parts"
 */
bool has_superset( const Bucket & , const PartVector & );


//----------------------------------------------------------------------
/** \brief  A container for the \ref stk_mesh_field_data "field data"
 *          of a homogeneous collection of
 *          \ref stk::mesh::Entity "entities".
 *
 *  The entities are homogeneous in that they are of the same entity type
 *  and are members of the same of parts.
 */
class Bucket {
private:
  friend class impl::BucketRepository;

  class BucketImpl {
  public:

    struct DataMap {
      typedef FieldBase::Restriction::size_type size_type ;
      const size_type * m_stride ;
      size_type         m_base ;
      size_type         m_size ;
    };

    BucketImpl( BulkData & arg_mesh ,
                EntityRank arg_entity_rank,
                const std::vector<unsigned> & arg_key,
                size_t arg_capacity
                );

    //
    // External interface:
    //
    BulkData & mesh() const { return m_mesh ; }
    unsigned entity_rank() const { return m_entity_rank ; }
    const unsigned * key() const { return &m_key[0] ; }
    const std::vector<unsigned> & key_vector() const { return m_key; }

    std::pair<const unsigned *, const unsigned *>
    superset_part_ordinals() const
    {
      return std::pair<const unsigned *, const unsigned *>
        ( key() + 1 , key() + key()[0] );
    }
    unsigned allocation_size() const { return 0 ; }
    size_t capacity() const { return m_capacity ; }
    size_t size() const { return m_size ; }
    Entity & operator[] ( size_t i ) const { return *(m_entities[i]) ; }
    unsigned field_data_size(const FieldBase & field) const
    {
      return m_field_map[ field.mesh_meta_data_ordinal() ].m_size;
    }
    const FieldBase::Restriction::size_type * field_data_stride( const FieldBase & field ) const
    {
      return m_field_map[ field.mesh_meta_data_ordinal() ].m_stride;
    }
    unsigned char * field_data_location( const FieldBase & field, const Entity & entity ) const
    {
      return field_data_location_impl( field.mesh_meta_data_ordinal(), entity.bucket_ordinal() );
    }

    /** Experimental method that skips the if-test for field existence on the bucket.
        This method should only be called if the caller is sure that the field exists on the bucket.
    */
    unsigned char * fast_field_data_location( const FieldBase & field, unsigned ordinal ) const
    {
      return fast_field_data_location_impl( field.mesh_meta_data_ordinal(), ordinal );
    }
    unsigned char * field_data_location( const FieldBase & field, unsigned ordinal ) const
    {
      return field_data_location_impl( field.mesh_meta_data_ordinal(), ordinal );
    }
    unsigned char * field_data_location( const FieldBase & field ) const
    {
      unsigned int zero_ordinal = 0;
      return field_data_location_impl( field.mesh_meta_data_ordinal(), zero_ordinal );
    }

    //
    // Internal interface:
    //
    void increment_size() { ++m_size ; }
    void decrement_size() { --m_size ; }
    void replace_entity(unsigned entity_ordinal, Entity * entity ) { m_entities[entity_ordinal] = entity ; }
    void update_state();

    template< class field_type >
    typename FieldTraits< field_type >::data_type *
    field_data( const field_type & f , const unsigned & entity_ordinal ) const
    {
      typedef typename FieldTraits< field_type >::data_type * data_p ;
      return reinterpret_cast<data_p>(field_data_location_impl(f.mesh_meta_data_ordinal(),entity_ordinal));
    }

    // BucketKey key = ( part-count , { part-ordinals } , counter )
    //  key[ key[0] ] == counter
    unsigned bucket_counter() const { return m_key[ m_key[0] ]; }

    Bucket * last_bucket_in_family() const;
    Bucket * first_bucket_in_family() const;
    void set_last_bucket_in_family( Bucket * last_bucket );
    void set_first_bucket_in_family( Bucket * first_bucket );
    DataMap * get_field_map();
    void initialize_fields( unsigned i_dst );
    void replace_fields( unsigned i_dst , Bucket & k_src , unsigned i_src );
    void set_bucket_family_pointer( Bucket * bucket ) { m_bucket = bucket; }
    const Bucket * get_bucket_family_pointer() const { return m_bucket; }

    bool equivalent( const BucketImpl& other_bucket ) const {
      return first_bucket_in_family() == other_bucket.first_bucket_in_family();
    }

    Entity*const* begin() const { return &m_entities[0]; }
    Entity*const* end() const { return &m_entities[0] + m_size; }

    ~BucketImpl() { delete [] m_field_data; }

  private:
    BucketImpl();

    BulkData             & m_mesh ;        // Where this bucket resides
    const EntityRank       m_entity_rank ; // Type of entities for this bucket
    std::vector<unsigned>  m_key ;
    const size_t           m_capacity ;    // Capacity for entities
    size_t                 m_size ;        // Number of entities
    Bucket               * m_bucket ;      // Pointer to head of bucket family, but head points to tail
    std::vector<DataMap>   m_field_map ;   // Field value data map, shared
    std::vector<Entity*>   m_entities ;    // Array of entity pointers,
    // beginning of field value memory.
    unsigned char* m_field_data;
    unsigned char* m_field_data_end;

    unsigned char * field_data_location_impl( const unsigned & field_ordinal, const unsigned & entity_ordinal ) const
    {
      typedef unsigned char * byte_p ;
      const DataMap & data_map = m_field_map[ field_ordinal ];
      unsigned char * ptr = NULL;
      if ( data_map.m_size ) {
        ptr = const_cast<unsigned char*>(m_field_data) + data_map.m_base + data_map.m_size * entity_ordinal;
        ThrowAssert(ptr < m_field_data_end);
      }
      return ptr ;
    }
    unsigned char * fast_field_data_location_impl( const unsigned & field_ordinal, const unsigned & entity_ordinal ) const
    {
      typedef unsigned char * byte_p ;
      const DataMap & data_map = m_field_map[ field_ordinal ];
      ThrowAssertMsg(data_map.m_size>0,"Field doesn't exist on bucket.");
      return const_cast<unsigned char*>(m_field_data) + data_map.m_base + data_map.m_size * entity_ordinal;
    }
    Bucket * last_bucket_in_family_impl() const;
  };


  BucketImpl       m_bucketImpl;

#ifdef SIERRA_MIGRATION
  const void*            m_fmwk_mesh_bulk_data;
#endif

public:


  ////
  //// New API functions
  ////
//   void add_entities_impl(size_t how_many);

//   void remove_entities_impl(size_t how_many);

//   void move_entities_impl(partition_impl& to, size_t how_many);

//   void swap(partition_offset first, partition_offset second);
  ////
  //// End New API function.
  ////

  //--------------------------------
  // Container-like types and methods:

  typedef boost::indirect_iterator<Entity*const*> iterator ;

  /** \brief Beginning of the bucket */
  inline iterator begin() const { return iterator(m_bucketImpl.begin()); }

  /** \brief End of the bucket */
  inline iterator end() const { return iterator(m_bucketImpl.end()); }

  /** \brief  Number of entities associated with this bucket */
  size_t size() const { return m_bucketImpl.size() ; }

  /** \brief  Capacity of this bucket */
  size_t capacity() const { return m_bucketImpl.capacity() ; }

  /** \brief  Query the i^th entity */
  Entity & operator[] ( size_t i ) const { return m_bucketImpl[i] ; }

  /** \brief  Query the size of this field data specified by FieldBase */
  unsigned field_data_size(const FieldBase & field) const
  { return m_bucketImpl.field_data_size(field); }

  /** \brief  Query the stride of this field data specified by FieldBase */
  const FieldBase::Restriction::size_type * field_data_stride( const FieldBase & field ) const
  { return m_bucketImpl.field_data_stride(field); }

  /** \brief  Query the location of this field data specified by FieldBase and Entity */
  unsigned char * field_data_location( const FieldBase & field, const Entity & entity ) const
  { return m_bucketImpl.field_data_location(field,entity); }

  /** \brief  Query the location of this field data specified by FieldBase and Entity-bucket-ordinal */
  unsigned char * field_data_location( const FieldBase & field, unsigned ordinal ) const
  { return m_bucketImpl.field_data_location(field, ordinal); }

  /** \brief  Query the location of this field data specified by FieldBase and Entity-bucket-ordinal
     This method should only be called if the caller knows that the field exists on the bucket.
     In an attempt to improve performance, this method skips the if-test that is normally done.
   */
  unsigned char * fast_field_data_location( const FieldBase & field, unsigned ordinal ) const
  { return m_bucketImpl.fast_field_data_location(field, ordinal); }

  /** \brief  Query the location of this field data specified by FieldBase */
  unsigned char * field_data_location( const FieldBase & field ) const
  { return m_bucketImpl.field_data_location(field); }

  /** \brief  Query the location of this field data specified by FieldBase and Entity */
  template< class field_type >
  typename FieldTraits< field_type >::data_type *
  field_data( const field_type & field , const Entity & entity ) const
  { return m_bucketImpl.field_data(field,entity.bucket_ordinal()); }

  //--------------------------------
  /** \brief  The \ref stk::mesh::BulkData "bulk data manager"
   *          that owns this bucket.
   */
  BulkData & mesh() const { return m_bucketImpl.mesh(); }

  /** \brief  Type of entities in this bucket */
  unsigned entity_rank() const { return m_bucketImpl.entity_rank(); }

  /** \brief  This bucket is a subset of these \ref stk::mesh::Part "parts" */
  void supersets( PartVector & ) const ;
  void supersets( OrdinalVector & ) const ;

  //--------------------------------
  /** \brief  Bucket is a subset of the given part */
  bool member( const Part & ) const ;

  /** \brief  Bucket is a subset of all of the given parts */
  bool member_all( const PartVector & ) const ;
  bool member_all( const OrdinalVector & ) const ;

  /** \brief  Bucket is a subset of any of the given parts */
  bool member_any( const PartVector & ) const ;
  bool member_any( const OrdinalVector & ) const ;

  //--------------------------------
  /** Query bucket's supersets' ordinals. */
  std::pair<const unsigned *, const unsigned *>
    superset_part_ordinals() const { return m_bucketImpl.superset_part_ordinals() ; }

  /** \brief Equivalent buckets have the same parts
   */
  bool equivalent( const Bucket& b ) const {
    return m_bucketImpl.equivalent(b.m_bucketImpl);
  }

#ifndef DOXYGEN_COMPILE
  const unsigned * key() const { return m_bucketImpl.key() ; }
#endif /* DOXYGEN_COMPILE */

  /** \brief  The allocation size, in bytes, of this bucket */
  unsigned allocation_size() const { return m_bucketImpl.allocation_size() ; }

  /** \brief  A method to assist in unit testing - accesses private data as necessary. */
  bool assert_correct() const;

#ifdef SIERRA_MIGRATION
  typedef std::pair<iterator, iterator> EntityRange;

  bool is_empty() const { return size() == 0; }

  const sierra::Fmwk::MeshBulkData* get_bulk_data() const
  {
    return static_cast<const sierra::Fmwk::MeshBulkData*>(m_fmwk_mesh_bulk_data);
  }

  template <class T>
  void set_bulk_data(const T* bulk_ptr) { m_fmwk_mesh_bulk_data = bulk_ptr; }
#endif

private:
  /** \brief  The \ref stk::mesh::BulkData "bulk data manager"
   *          that owns this bucket.
   */
  BulkData & bulk_data() const { return m_bucketImpl.mesh(); }

  // Only reason to define this at all is to ensure it's private
  ~Bucket() {}

  Bucket();
  Bucket( const Bucket & );
  Bucket & operator = ( const Bucket & );

  Bucket( BulkData & arg_mesh ,
          EntityRank arg_entity_rank,
          const std::vector<unsigned> & arg_key,
          size_t arg_capacity
        );

  friend class ::stk::mesh::BulkData;
};


struct BucketLess {
  bool operator()( const Bucket * lhs_bucket , const unsigned * rhs ) const ;
  bool operator()( const unsigned * lhs , const Bucket * rhs_bucket ) const ;
};


inline
std::vector<Bucket*>::iterator
lower_bound( std::vector<Bucket*> & v , const unsigned * key )
{ return std::lower_bound( v.begin() , v.end() , key , BucketLess() ); }

inline
Bucket::Bucket( BulkData & arg_mesh ,
                EntityRank arg_entity_rank,
                const std::vector<unsigned> & arg_key,
                size_t arg_capacity
        )
  : m_bucketImpl(arg_mesh,arg_entity_rank,arg_key,arg_capacity)
{}

/** \} */

inline
bool Bucket::member_all( const OrdinalVector& parts ) const
{
  const unsigned * const i_beg = key() + 1 ;
  const unsigned * const i_end = key() + key()[0] ;

  const OrdinalVector::const_iterator ip_end = parts.end();
        OrdinalVector::const_iterator ip     = parts.begin() ;

  bool result_all = true ;

  for ( ; result_all && ip_end != ip ; ++ip ) {
    const unsigned ord = *ip;
    result_all = contains_ordinal(i_beg, i_end, ord);
  }
  return result_all ;
}

struct To_Ptr : std::unary_function<Entity&, Entity*>
{
  Entity* operator()(Entity& entity) const
  {
    return &entity;
  }
};

// Sometimes, we want a bucket-iterator to dereference to an Entity*
typedef boost::transform_iterator<To_Ptr, Bucket::iterator> BucketPtrIterator;

typedef Bucket::iterator BucketIterator;

} // namespace mesh
} // namespace stk

#endif
