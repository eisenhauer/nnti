/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#ifndef stk_mesh_BoundaryAnalysis_hpp
#define stk_mesh_BoundaryAnalysis_hpp

#include <vector>
#include <stk_util/util/NamedPair.hpp>

namespace stk {
namespace mesh {

class BulkData;
class Entity;

/**
 * A pair of Entity* and a local side id (defined a part of a side)
 */
NAMED_PAIR(EntitySideComponent, Entity *, entity, unsigned, side_ordinal)
/**
 * A pair of EntitySideComponents (defines a side of the boundary)
 * Most sides will have two EntitySideComponents, but degenerate cases
 * ie shells will can have sides with more than two components)
 */
NAMED_PAIR(EntitySide, EntitySideComponent, inside, EntitySideComponent, outside)

/**
 * A vector of EntitySide (defines a boundary)
 */
typedef std::vector<EntitySide> EntitySideVector;

/** \brief Given a closure, return a boundary of items of closure_rank-1
 *
 * A boundary will contain the entities "touching" the "outside" of the
 * closure.
 */
void boundary_analysis(const BulkData& bulk_data,
                       const std::vector< Entity *> & entities_closure,
                       unsigned closure_rank,
                       EntitySideVector& boundary);

/** \brief  Get the entities adjacent to the input entity.
 *
 *  The adjacent entities are of the same rank as the input entity.
 *  Adjacency is defined by the input entity sharing a common
 *  sub-cell with the adjacent entities.
 *
 *  subcell_rank defines the rank of the (potentially) common subcell
 *  subcell_identifier defined the local id of the common subcell
 *  adjacent_entities is an output parameter that contains pairs that
 *     have the adjacent entity and the local id of the common subcell
 *     with respect to the adjacent entity.
 */
void get_adjacent_entities( const Entity & entity ,
                            unsigned subcell_rank ,
                            unsigned subcell_identifier ,
                            std::vector< EntitySideComponent > & adjacent_entities );
}
}
#endif
