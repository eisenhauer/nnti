/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#ifndef stk_mesh_base_Combo_hpp
#define stk_mesh_base_Combo_hpp

#include <stk_mesh/base/EntityKey.hpp>
#include <stk_mesh/base/Types.hpp>
#include <stk_mesh/base/Trace.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_mesh/base/Part.hpp>
#include <stk_mesh/base/ConnectivityMap.hpp>

#include <stk_util/environment/ReportHandler.hpp>
#include <stk_util/environment/OutputLog.hpp>
#include <stk_util/util/PairIter.hpp>

#include <boost/static_assert.hpp>
#include <boost/range.hpp>
#include <boost/type_traits/is_pod.hpp>

#include <stk_topology/topology.hpp>

#include <utility>
#include <vector>
#include <iosfwd>
#include <string>
#include <algorithm>

#include <boost/functional/hash.hpp>

#ifdef SIERRA_MIGRATION
namespace stk {
namespace mesh {
typedef RelationVector::const_iterator   RelationIterator;
typedef boost::iterator_range<RelationIterator> RelationRange;
}
}
#endif // SIERRA_MIGRATION

#include <stk_mesh/base/Entity.tcc> //only place where this file should be included

#include <stk_mesh/base/Relation.tcc> //only place where this file should be included

#include <stk_mesh/base/BucketConnectivity.tcc> //only place where this file should be included

#include <stk_mesh/base/Bucket.tcc> //only place where this file should be included

///////////////////////////////////////////////////////////////////////////////
// Put methods below that could not otherwise be inlined due to cyclic dependencies between
// Relation/Entity/Bucket
///////////////////////////////////////////////////////////////////////////////

namespace stk {
namespace mesh {

//
// Relation
//

inline
Entity Relation::entity() const
{
  return m_target_entity;
}

#ifdef SIERRA_MIGRATION

inline
Relation::Relation(EntityRank rel_rank, Entity obj, const unsigned relation_type, const unsigned ordinal, const unsigned orient)
  :
      m_raw_relation( Relation::raw_relation_id(rel_rank, ordinal )),
      m_attribute( (relation_type << fmwk_orientation_digits) | orient ),
      m_target_entity(obj)
{
  ThrowAssertMsg( orient <= fmwk_orientation_mask,
      "orientation " << orient << " exceeds maximum allowed value");
}

inline
Relation::Relation(Entity obj, const unsigned relation_type, const unsigned ordinal, const unsigned orient)
  : m_raw_relation( Relation::raw_relation_id(obj.entity_rank(), ordinal )),
    m_attribute( (relation_type << fmwk_orientation_digits) | orient ),
    m_target_entity(obj)
{
#ifdef STK_MESH_ALLOW_DEPRECATED_ENTITY_FNS
  ThrowAssertMsg( orient <= fmwk_orientation_mask,
                  "orientation " << orient << " exceeds maximum allowed value");
#else
  ThrowErrorMsg("Relation::Relation(Entity, unsigned, unsigned, unsigned) has been deprecated, use the construct that takes the entity rank");
#endif
}

inline
void Relation::setMeshObj(Entity object, EntityRank object_rank )
{
  m_raw_relation = Relation::raw_relation_id( object_rank, relation_ordinal() );
  m_target_entity = object;
}

inline
void Relation::setMeshObj(Entity object)
{
#ifdef STK_MESH_ALLOW_DEPRECATED_ENTITY_FNS
  setMeshObj(object, object.entity_rank());
#else
  ThrowErrorMsg("Relation::setMeshObj(Entity) has been deprecated");
#endif
}

#endif

//
// Entity
//

inline
size_t hash_value( Entity entity) {
  return boost::hash_value(entity.local_offset());
}

//
// BucketConnectivity
//

template <typename EntityVector, typename  RankTypeVector, typename BulkData> // hack to get around dependency
void sync_rank_id_targets(const EntityVector &target_entities, RankTypeVector &targets, BulkData* mesh)
{
  typedef typename RankTypeVector::value_type RankType;

  targets.resize(target_entities.size(), TopoHelper<RankType>::invalid);

  for (size_t i = 0, end = target_entities.size(); i < end; ++i) {
    Entity target = target_entities[i];
    if (mesh->is_valid(target)) {
      // TODO: Cleanup/remove MeshIndex
      const MeshIndex& index = mesh->mesh_index(target);
      uint64_t bucket_id  = index.bucket->bucket_id();
      uint64_t bucket_ord = index.bucket_ordinal;
      targets[i] = make_rank_id<RankType>(bucket_id, bucket_ord);
    }
  }
}

template <EntityRank TargetRank>
template <typename BulkData> // hack to get around dependency
inline
void impl::BucketConnectivity<TargetRank, FIXED_CONNECTIVITY>::end_modification(BulkData* mesh)
{
  //TODO: If bucket is blocked, no longer need to shrink to fit!

  {
    EntityVector temp(m_target_entities.begin(), m_target_entities.end());
    m_target_entities.swap(temp);
  }

  {
    PermutationVector temp(m_permutations.begin(), m_permutations.end());
    m_permutations.swap(temp);
  }

  if (mesh->maintain_fast_indices()) {
    sync_rank_id_targets(m_target_entities, m_targets, mesh);
  }
  invariant_check_helper(mesh);
}

template <EntityRank TargetRank>
template <typename BulkData>
inline
void impl::BucketConnectivity<TargetRank, DYNAMIC_CONNECTIVITY>::end_modification(BulkData* mesh)
{
  if (m_active) {
    resize_and_order_by_index();

    {
      UInt32Vector temp(m_indices.begin(), m_indices.end());
      m_indices.swap(temp);
    }

    {
      UInt16Vector temp(m_num_connectivities.begin(), m_num_connectivities.end());
      m_num_connectivities.swap(temp);
    }

    if (mesh->maintain_fast_indices()) {
      sync_rank_id_targets(m_target_entities, m_targets, mesh);
    }

    invariant_check_helper(mesh);
  }
}

template <typename BulkData>
inline
bool impl::LowerConnectivitityRankSensitiveCompare<BulkData>::operator()(Entity first_entity, ConnectivityOrdinal first_ordinal,
                                                                         Entity second_entity, ConnectivityOrdinal second_ordinal) const
{
  const EntityRank first_rank = m_mesh.entity_rank(first_entity);
  const EntityRank second_rank = m_mesh.entity_rank(second_entity);

  return (first_rank < second_rank)
         || ((first_rank == second_rank) && (first_ordinal < second_ordinal));
}

template <typename BulkData>
inline
bool impl::HigherConnectivityRankSensitiveCompare<BulkData>::operator()(Entity first_entity, ConnectivityOrdinal first_ordinal, Entity second_entity, ConnectivityOrdinal second_ordinal) const
{
  const EntityRank first_rank = m_mesh.entity_rank(first_entity);
  const EntityRank second_rank = m_mesh.entity_rank(second_entity);

  if (first_rank < second_rank) {
    return true;
  }
  if (first_rank > second_rank) {
    return false;
  }
  // Needs to match LessRelation in BulkData.hpp
  return std::make_pair(first_ordinal,  first_entity.is_local_offset_valid() ?  first_entity.local_offset()  : Entity::MaxEntity) <
         std::make_pair(second_ordinal, second_entity.is_local_offset_valid() ? second_entity.local_offset() : Entity::MaxEntity);
}

} // namespace mesh
} // namespace stk

#endif /* stk_mesh_Combo_hpp */
