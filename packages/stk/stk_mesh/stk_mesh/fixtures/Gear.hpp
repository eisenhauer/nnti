/*------------------------------------------------------------------------*/
/*                 Copyright 2010 Sandia Corporation.                     */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*  Export of this program may require a license from the                 */
/*  United States Government.                                             */
/*------------------------------------------------------------------------*/

#ifndef STK_MESH_FIXTURES_GEAR_HPP
#define STK_MESH_FIXTURES_GEAR_HPP

#include <stddef.h>                     // for size_t
#include <stk_mesh/base/CoordinateSystems.hpp>  // for Cartesian
#include <stk_mesh/base/Field.hpp>      // for Field
#include <stk_mesh/base/Types.hpp>      // for EntityId, EntityVector
#include "stk_mesh/base/Entity.hpp"     // for Entity
#include "stk_mesh/base/FieldState.hpp"  // for FieldState
namespace stk { namespace mesh { class BulkData; } }
namespace stk { namespace mesh { class MetaData; } }
namespace stk { namespace mesh { class Part; } }


namespace {

const double PI     = 3.14159265358979;
const double TWO_PI = 2 * PI;

} // namespace

namespace stk {
namespace mesh {
namespace fixtures {

/**
 * Defines a single movement of a gear. This include rotation and movement in
 * the x, y, z dimensions.
 */
struct GearMovement
{
  double rotation;
  double x;
  double y;
  double z;

  GearMovement(
      double arg_rotation = 0.0,
      double arg_x = 0.0,
      double arg_y = 0.0,
      double arg_z = 0.0
      ) :
     rotation(arg_rotation)
   , x(arg_x)
   , y(arg_y)
   , z(arg_z)
  {}
};

class Gear {

  typedef Field< double ,Cartesian>    CartesianField ;
  typedef Field< double ,Cylindrical>  CylindricalField ;

  enum { SpatialDimension = 3 };

 public:
  Gear(
       MetaData & meta,
       BulkData & bulk,
       Part & gear,
       Part & cylindrical_coord,
       Part & hex,
       Part & wedge,
       CartesianField    & arg_cartesian_coord_field,
       CartesianField    & arg_displacement_field,
       CartesianField    & arg_translation_field,
       CylindricalField  & arg_cylindrical_coord_field,
       double arg_element_size  =  0.10,
       double arg_radius_min    =  0.6,
       double arg_radius_max    =  1.0 + 0.05,
       double arg_height_min = -0.4,
       double arg_height_max =  0.4
       );

  const double element_size;
  const double rad_min, rad_max;
  const double height_min, height_max;

  const size_t angle_num;
  const size_t rad_num;
  const size_t height_num;

  const double angle_increment;
  const double rad_increment;
  const double height_increment;

  const size_t num_elements;
  const size_t num_nodes;

  MetaData & meta_data;
  BulkData & bulk_data;

  Part & gear_part;

  //must be called between modification_begin / modification_end
  void generate_gear();

  void move( const GearMovement & data );


  Entity get_node (
        size_t iz ,       // Thickness index
        size_t ir ,       // Radial index
        size_t ia ) const // Angle index
  {
    return gear_entities[ node_index(iz,ir,ia)];
  }

    Entity get_element(
        size_t iz ,       // Thickness index
        size_t ir ,       // Radial index
        size_t ia ) const // Angle index
  {
    return gear_entities[ elem_index(iz,ir,ia)];
  }

    EntityId node_index(
        size_t iz ,       // Thickness index
        size_t ir ,       // Radial index
        size_t ia ) const // Angle index
  {
    return static_cast<stk::mesh::EntityId>(iz + height_num * ( ir + rad_num * ia ));
  }

  EntityId elem_index(
        size_t iz ,       // Thickness index
        size_t ir ,       // Radial index
        size_t ia ) const // Angle index
  {
    return static_cast<stk::mesh::EntityId>(num_nodes + iz + (height_num-1) * ( ir + (rad_num-1) * ia ));
  }

  void populate_fields(stk::mesh::FieldState state);

  Part & cylindrical_coord_part;
  Part & hex_part;
  Part & wedge_part;

  CartesianField    & cartesian_coord_field ;
  CartesianField    & displacement_field ;
  CartesianField    & translation_field ;
  CylindricalField  & cylindrical_coord_field ;

 private:
  EntityVector gear_entities;

  Gear(const Gear &);
  void operator = (const Gear &);
};

} // fixtures
} // mesh
} // stk

#endif // STK_MESH_FIXTURES_GEAR_HPP
