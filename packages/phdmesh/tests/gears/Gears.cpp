/*------------------------------------------------------------------------*/
/*      phdMesh : Parallel Heterogneous Dynamic unstructured Mesh         */
/*                Copyright (2007) Sandia Corporation                     */
/*                                                                        */
/*  Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive   */
/*  license for use of this work by or on behalf of the U.S. Government.  */
/*                                                                        */
/*  This library is free software; you can redistribute it and/or modify  */
/*  it under the terms of the GNU Lesser General Public License as        */
/*  published by the Free Software Foundation; either version 2.1 of the  */
/*  License, or (at your option) any later version.                       */
/*                                                                        */
/*  This library is distributed in the hope that it will be useful,       */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of        */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU     */
/*  Lesser General Public License for more details.                       */
/*                                                                        */
/*  You should have received a copy of the GNU Lesser General Public      */
/*  License along with this library; if not, write to the Free Software   */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307   */
/*  USA                                                                   */
/*------------------------------------------------------------------------*/
/**
 * @author H. Carter Edwards
 */

#include <math.h>
#include <iostream>
#include <limits>
#include <stdexcept>

#include <util/ParallelComm.hpp>

#include <mesh/Schema.hpp>
#include <mesh/Mesh.hpp>
#include <mesh/Comm.hpp>
#include <mesh/EntityManager.hpp>
#include <mesh_io/FieldName.hpp>

#include "Gears.hpp"

//----------------------------------------------------------------------
//----------------------------------------------------------------------

namespace phdmesh {

//----------------------------------------------------------------------

GearFields::GearFields( Schema & S )
: gear_coord(
    S.declare_field<double,1>( Node, std::string("gear_coordinates"), 1 ) ),
  model_coord(
    S.declare_field<double,1>( Node, std::string("model_coordinates"), 1 ) ),
  current_coord(
    S.declare_field<double,1>( Node, std::string("coordinates") , 2 ) ),
  displacement(
    S.declare_field<double,1>( Node, std::string("displacement") , 2 ) ),
  element_attr(
    S.declare_field<double,1>( Element, std::string("element_attribute"), 1 ) ),
  element_value(
    S.declare_field<double,2>( Element, std::string("element_value"), 1 ) ),
  node_value(
    S.declare_field<double,1>( Node, std::string("node_value") , 2 ) )
{
  const unsigned n3 = 3 ;
  const Part & universe = S.universal_part();
  gear_coord   .set_dimension( universe , n3 );
  model_coord  .set_dimension( universe , n3 );
  current_coord.set_dimension( universe , n3 );
  displacement .set_dimension( universe , n3 );

  declare( gear_coord ,    cylindrical_vector() );
  declare( model_coord ,   cartesian_vector() );
  declare( current_coord , cartesian_vector() );
  declare( displacement ,  cartesian_vector() );
}

//----------------------------------------------------------------------

namespace {

unsigned long
identifier( unsigned nthick ,  // Number of entities through the thickness
            unsigned nradius , // Number of entities through the radius
            unsigned iz ,      // Thickness index
            unsigned ir ,      // Radial index
            unsigned ia )      // Angle index
{
  return iz + nthick * ( ir + nradius * ia );
}

}

Gear::Gear( Schema & S ,
            const std::string & name ,
            const GearFields & gear_fields ,
            const double center[] ,
            const double rad_min ,
            const double rad_max ,
            const unsigned rad_num ,
            const double z_min ,
            const double z_max ,
            const unsigned z_num ,
            const unsigned angle_num ,
            const int      turn_dir )
  : m_schema( S ),
    m_mesh( NULL ),
    m_gear( S.declare_part(std::string("Gear_").append(name)) ),
    m_surf( S.declare_part(std::string("Surf_").append(name)) ),
    m_gear_coord(    gear_fields.gear_coord ),
    m_model_coord(   gear_fields.model_coord ),
    m_current_coord( gear_fields.current_coord ),
    m_displacement(  gear_fields.displacement ),
    m_elem_value(    gear_fields.element_value ),
    m_node_value(    gear_fields.node_value )
{
  enum { NNODE = 8 };

  gear_fields.gear_coord   .set_dimension( m_gear , 3 );
  gear_fields.model_coord  .set_dimension( m_gear , 3 );
  gear_fields.current_coord.set_dimension( m_gear , 3 );
  gear_fields.displacement .set_dimension( m_gear , 3 );
  gear_fields.element_value.set_dimension( m_gear , 1 , NNODE );
  gear_fields.node_value   .set_dimension( m_gear , 1 );

  const double TWO_PI = 2.0 * acos( (double) -1.0 );

  m_center[0] = center[0] ;
  m_center[1] = center[1] ;
  m_center[2] = center[2] ;

  m_z_min     = z_min ;
  m_z_max     = z_max ;
  m_z_inc     = (z_max - z_min) / ((double) ( z_num - 1 ));

  m_rad_min   = rad_min ;
  m_rad_max   = rad_max ;
  m_rad_inc   = (rad_max - rad_min) / ((double) ( rad_num - 1 ));

  m_ang_inc   = TWO_PI / (double) angle_num ;

  m_rad_num   = rad_num ;
  m_z_num     = z_num ;
  m_angle_num = angle_num ;
  m_turn_dir  = turn_dir ;
}

//----------------------------------------------------------------------

Entity * Gear::create_node(
  const std::vector<Part*> & parts ,
  const unsigned long node_id_base ,
  const unsigned iz ,
  const unsigned ir ,
  const unsigned ia ) const
{
  const unsigned p_owner = m_schema.parallel_rank();

  const double angle     = m_ang_inc * ia ;
  const double cos_angle = cos( angle );
  const double sin_angle = sin( angle );

  const double radius = m_rad_min + m_rad_inc * ir ;
  const double x = m_center[0] + radius * cos_angle ;
  const double y = m_center[1] + radius * sin_angle ;
  const double z = m_center[2] + m_z_min + m_z_inc * iz ;

  // Create the node and set the model_coordinates

  unsigned long id_gear = identifier( m_z_num, m_rad_num, iz, ir, ia );
  unsigned long id = node_id_base + id_gear ;

  Entity & node = m_mesh->declare_entity(Node,id,parts,p_owner);

  double * const gear_data    = node.data( m_gear_coord );
  double * const model_data   = node.data( m_model_coord );
  double * const current_data = node.data( m_current_coord );
  double * const disp_data    = node.data( m_displacement );

  gear_data[0] = radius ;
  gear_data[1] = angle ;
  gear_data[2] = z - m_center[2] ;

  model_data[0] = x ;
  model_data[1] = y ;
  model_data[2] = z ;

  current_data[0] = x ;
  current_data[1] = y ;
  current_data[2] = z ;

  disp_data[0] = 0.0 ;
  disp_data[1] = 0.0 ;
  disp_data[2] = 0.0 ;

  return & node ;
}

//----------------------------------------------------------------------

void Gear::mesh( Mesh & M )
{
  static const char method[] = "phdmesh::Gear::mesh" ;

  const double TWO_PI = 2.0 * acos( (double) -1.0 );

  m_mesh = & M ;

  const EntityManager manager ;

  const unsigned p_size = m_schema.parallel_size();
  const unsigned p_rank = m_schema.parallel_rank();

  unsigned long counts[ EntityTypeMaximum ];
  unsigned long max_id[ EntityTypeMaximum ];

  comm_mesh_stats( M , counts , max_id );

  const unsigned long node_id_base = max_id[ Node ] + 1 ;
  const unsigned long face_id_base = max_id[ Face ] + 1 ;
  const unsigned long elem_id_base = max_id[ Element ] + 1 ;

  const unsigned long elem_id_gear_max =
    m_angle_num * ( m_rad_num - 1 ) * ( m_z_num - 1 );

  std::vector<Part*> elem_parts ;
  std::vector<Part*> face_parts ;

  {
    Part * const p_gear   = & m_gear ;
    Part * const p_surf   = & m_surf ;
    Part * const p_owns  = & m_schema.owns_part();

    elem_parts.push_back( p_gear );
    elem_parts.push_back( p_owns );

    face_parts.push_back( p_gear );
    face_parts.push_back( p_surf );
    face_parts.push_back( p_owns );
  }

  for ( unsigned ia = 0 ; ia < m_angle_num ; ++ia ) {
    for ( unsigned ir = 0 ; ir < m_rad_num - 1 ; ++ir ) {
      for ( unsigned iz = 0 ; iz < m_z_num - 1 ; ++iz ) {

        unsigned long elem_id_gear =
          identifier( m_z_num-1 , m_rad_num-1 , iz , ir , ia );

        if ( ( ( elem_id_gear * p_size ) / elem_id_gear_max ) == p_rank ) {

          unsigned long elem_id = elem_id_base + elem_id_gear ;

          // Create the node and set the model_coordinates

          const unsigned ia_1 = ( ia + 1 ) % m_angle_num ;
          const unsigned ir_1 = ir + 1 ;
          const unsigned iz_1 = iz + 1 ;

          Entity * node[8] ;

          node[0] = create_node( elem_parts, node_id_base, iz  , ir  , ia_1 );
          node[1] = create_node( elem_parts, node_id_base, iz  , ir  , ia   );
          node[2] = create_node( elem_parts, node_id_base, iz_1, ir  , ia   );
          node[3] = create_node( elem_parts, node_id_base, iz_1, ir  , ia_1 );
          node[4] = create_node( elem_parts, node_id_base, iz  , ir_1, ia_1 );
          node[5] = create_node( elem_parts, node_id_base, iz  , ir_1, ia   );
          node[6] = create_node( elem_parts, node_id_base, iz_1, ir_1, ia   );
          node[7] = create_node( elem_parts, node_id_base, iz_1, ir_1, ia_1 );

          std::vector<Connect> rel ;

          // Centroid of the element for verification

          const double angle = m_ang_inc * ( 0.5 + (double) ia );
          const double z = m_center[2] + m_z_min + m_z_inc * (0.5 + (double)iz);

          double c[3] = { 0 , 0 , 0 };

          for ( unsigned j = 0 ; j < 8 ; ++j ) {
            Connect con( *node[j] , Uses , j );
            rel.push_back( con );

            double * const coord_data = node[j]->data( m_model_coord );
            c[0] += coord_data[0] ;
            c[1] += coord_data[1] ;
            c[2] += coord_data[2] ;
          }
          c[0] /= 8 ; c[1] /= 8 ; c[2] /= 8 ;
          c[0] -= m_center[0] ;
          c[1] -= m_center[1] ;

          double val_a = atan2( c[1] , c[0] );
          if ( val_a < 0 ) { val_a += TWO_PI ; }
          const double err_a = angle - val_a ;
          const double err_z = z - c[2] ;

          const double eps = 100 * std::numeric_limits<double>::epsilon();

          if ( err_z < - eps || eps < err_z ||
               err_a < - eps || eps < err_a ) {
            std::string msg ;
            msg.append("problem setup element centroid error" );
            throw std::logic_error( msg );
          }

          Entity & elem =
            * manager.declare_entity( M , Element , elem_id ,
                                      p_rank , elem_parts , rel );

          for ( unsigned j = 0 ; j < 8 ; ++j ) {
            Connect con( elem , UsedBy , j );
            node[j]->add_connection( con );
          }
        }
      }
    }
  }

  // Array of faces on the surface

  {
    const unsigned ir = m_rad_num - 1 ;

    for ( unsigned ia = 0 ; ia < m_angle_num ; ++ia ) {
      for ( unsigned iz = 0 ; iz < m_z_num - 1 ; ++iz ) {

        unsigned long elem_id_gear =
          identifier( m_z_num-1 , m_rad_num-1 , iz , ir-1 , ia );

        if ( ( ( elem_id_gear * p_size ) / elem_id_gear_max ) == p_rank ) {

          unsigned long elem_id = elem_id_base + elem_id_gear ;

          unsigned long face_id =
            face_id_base + identifier( m_z_num-1 , 1 , iz , 0 , ia );

          unsigned face_ord = 5 ;
          Entity * node[4] ;

          const unsigned ia_1 = ( ia + 1 ) % m_angle_num ;
          const unsigned iz_1 = iz + 1 ;

          node[0] = create_node( face_parts, node_id_base, iz  , ir  , ia_1 );
          node[1] = create_node( face_parts, node_id_base, iz  , ir  , ia   );
          node[2] = create_node( face_parts, node_id_base, iz_1, ir  , ia   );
          node[3] = create_node( face_parts, node_id_base, iz_1, ir  , ia_1 );

          std::vector<Connect> rel ;

          for ( unsigned j = 0 ; j < 4 ; ++j ) {
            Connect con( *node[j] , Uses , j );
            rel.push_back( con );
          }

          Entity & elem = * M.get_entity(Element,elem_id,method);

          {
            Connect con( elem , UsedBy , face_ord );
            rel.push_back( con );
          }

          Entity & face =
              * manager.declare_entity( M , Face , face_id ,
                                        p_rank , face_parts , rel );

          for ( unsigned j = 0 ; j < 4 ; ++j ) {
            Connect con( face , UsedBy , j );
            node[j]->add_connection( con );
          }

          {
            Connect con( face , Uses , face_ord );
            elem.add_connection( con );
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------
// Iterate nodes and turn them by the angle

void Gear::turn( double turn_angle ) const
{
  const unsigned Length = 3 ;

  const KernelSet & ks = m_mesh->kernels( Node );
  const KernelSet::iterator ek = ks.end();
        KernelSet::iterator ik = ks.begin();
  for ( ; ik != ek ; ++ik ) {
    Kernel & k = *ik ;
    if ( k.has_superset( m_gear ) ) {
      const unsigned n = k.size();
      double * const kernel_gear_data    = k.data( m_gear_coord );
      double * const kernel_model_data   = k.data( m_model_coord );
      double * const kernel_current_data = k.data( m_current_coord );
      double * const kernel_disp_data    = k.data( m_displacement );

      for ( unsigned i = 0 ; i < n ; ++i ) {
        double * const gear_data    = kernel_gear_data    + i * Length ;
        double * const model_data   = kernel_model_data   + i * Length ;
        double * const current_data = kernel_current_data + i * Length ;
        double * const disp_data    = kernel_disp_data    + i * Length ;
    
        const double radius = gear_data[0] ;
        const double angle  = gear_data[1] + turn_angle * m_turn_dir ;

        current_data[0] = m_center[0] + radius * cos( angle );
        current_data[1] = m_center[1] + radius * sin( angle );
        current_data[2] = m_center[2] + gear_data[2] ;

        disp_data[0] = current_data[0] - model_data[0] ;
        disp_data[1] = current_data[1] - model_data[1] ;
        disp_data[2] = current_data[2] - model_data[2] ;
      }
    }
  }
}

//----------------------------------------------------------------------

namespace {

template<typename T, unsigned NV, unsigned NC>
bool verify_gather( const Kernel & kernel , const Field<T,1> & field )
{
  bool result = true ;
  const EntityType con_type = field.entity_type();
  const FieldDimension dim = dimension( field , kernel );
  result = NV == dim[0] ;
  for ( unsigned i = 0 ; result && i < kernel.size() ; ++i ) {
    ConnectSpan con = kernel[i]->connections( con_type );
    result = std::distance( con.first , con.second ) <= NC ;
    while ( result && con.first != con.second ) {
      result = NULL != con.first->entity()->data( field );
      ++con.first ;
    }
  }
  return result ;
}


template<typename T, unsigned NV, unsigned NC>
void gather( T * dst , const Entity & src , const Field<T,1> & field )
{
  ConnectSpan con = src.connections( field.entity_type() );
  for ( unsigned n = 0 ; n < NC ; ++n , ++con.first , dst += NV ) {
    Copy<NV>( dst , con.first->entity()->data( field ) );
  }
}

}

void Gear::element_update() const
{
  enum { NNODE = 8 };
  double nval[ NNODE ];

  const KernelSet & ks = m_mesh->kernels( Element );
  const KernelSet::iterator ek = ks.end();
        KernelSet::iterator ik = ks.begin();

  for ( ; ik != ek ; ++ik ) {
    Kernel & k = *ik ;
    if ( k.has_superset( m_gear ) ) {

      const unsigned number = k.size();

      double * const kernel_elem_data = k.data( m_elem_value );

      for ( unsigned j = 0 ; j < number ; ++j ) {

        gather<double,1,NNODE>( nval , * k[j] , m_node_value );

        double tmp = nval[0] + nval[1] + nval[2] + nval[3] +
                     nval[4] + nval[5] + nval[6] + nval[7] ;

        tmp /= (double) NNODE ;

        Copy<NNODE>( kernel_elem_data + NNODE * j , tmp );
      }
    }
  }
}

//----------------------------------------------------------------------

}

