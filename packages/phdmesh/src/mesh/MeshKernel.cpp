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

#include <stdlib.h>
#include <memory.h>

#include <stdexcept>
#include <iostream>
#include <sstream>
#include <algorithm>

#include <mesh/Kernel.hpp>
#include <mesh/Entity.hpp>
#include <mesh/Mesh.hpp>
#include <mesh/Schema.hpp>

namespace phdmesh {

namespace {

void memory_copy( unsigned char * dst , unsigned char * src , unsigned n )
{ memcpy( dst , src , n ); }

void memory_zero( unsigned char * dst , unsigned n )
{ memset( dst , 0 , n ); }

}

//----------------------------------------------------------------------
// KernelKey key = ( part-count , { part-ordinals } , counter )
//  key[ key[0] ] == counter

namespace {

unsigned kernel_counter( const unsigned * const key )
{ return key[ *key ]; }

// The part count and parts are equal
bool kernel_equal( const unsigned * lhs , const unsigned * rhs )
{
  bool result = true ;
  {
    const unsigned * const end_lhs = lhs + *lhs ;
    while ( result && end_lhs != lhs ) {
      result = *lhs == *rhs ;
      ++lhs ; ++rhs ;
    }
  }
  return result ;
}

}

// The part count and part ordinals are less
bool KernelLess::operator()( const unsigned * lhs ,
                             const unsigned * rhs ) const
{
  const unsigned * const last_lhs = lhs + ( *lhs < *rhs ? *lhs : *rhs );
  while ( last_lhs != lhs && *lhs == *rhs ) { ++lhs ; ++rhs ; }
  return *lhs < *rhs ;
}

bool Kernel::has_superset( const Part & p ) const
{
  const unsigned ordinal = p.schema_ordinal();
  const unsigned *       key_beg = key();
  const unsigned * const key_end = key_beg + *key_beg ; ++key_beg ;

  key_beg = std::lower_bound( key_beg , key_end , ordinal );

  return key_beg < key_end && ordinal == *key_beg ;
}

bool Kernel::has_superset( const PartSet & ps ) const
{
  const unsigned *       key_beg = key();
  const unsigned * const key_end = key_beg + *key_beg ; ++key_beg ;

  bool result = ! ps.empty();

  for ( PartSet::const_iterator
        i = ps.begin() ; result && i != ps.end() ; ++i ) {

    const unsigned ordinal = (*i)->schema_ordinal();

    key_beg = std::lower_bound( key_beg , key_end , ordinal );

    result = key_beg < key_end && ordinal == *key_beg ;
  }
  return result ;
}

void Kernel::supersets( PartSet & ps ) const
{
  const Schema & schema = m_mesh.schema();
  const PartSet & parts = schema.get_parts();

  const unsigned * key_val = key();
  const unsigned n = *key_val - 1 ; ++key_val ;

  ps.resize( n );
  for ( unsigned i = 0 ; i < n ; ++i , ++key_val ) {
    ps[i] = parts[ *key_val ];
  }
}

void Kernel::supersets( std::vector<unsigned> & ps ) const
{
  const unsigned * key_val = key();
  const unsigned n = *key_val - 1 ; ++key_val ;

  ps.resize( n );
  for ( unsigned i = 0 ; i < n ; ++i , ++key_val ) {
    ps[i] = *key_val ;
  }
}

//----------------------------------------------------------------------

bool Kernel::valid( const Field<void,0> & f ,
                    unsigned ord ,
                    const char * required_by ) const
{
  const Schema & schema = m_mesh.schema();
  const bool ok_schema = & schema      == & f.schema() ;
  const bool ok_type   = m_entity_type ==   f.entity_type();
  const bool ok_ord    = ord < m_size ;
  const bool ok        = ok_schema && ok_type && ok_ord ;

  if ( required_by && ! ok ) {
    std::ostringstream msg ;
    msg << "phdmesh::Kernel::valid( " ;
    f.print( msg );
    msg << " , " ;
    msg << ord ;
    msg << " , " ;
    msg << required_by ;
    msg << " ) FAILED with " ;
    if ( ! ok_schema ) {
      msg << " different Schema" ;
    }
    else if ( ! ok_type ) {
      msg << " different EntityType " ;
      msg << entity_type_name( m_entity_type ) ;
    }
    else {
      msg << " Ordinal " ;
      msg << m_size ;
      msg << " <= " ;
      msg << ord ;
    }
    throw std::runtime_error( msg.str() );
  }

  return ok ;
}

//----------------------------------------------------------------------

Kernel::Kernel( Mesh & arg_mesh ,
                EntityType arg_type ,
                const unsigned * arg_key )
: SetvMember<const unsigned * const>( arg_key ),
  m_mesh( arg_mesh ) ,
  m_entity_type( arg_type ),
  m_size( 0 ),
  m_capacity( 0 ),
  m_entities( NULL )
{}


//----------------------------------------------------------------------

void Kernel::zero_fields( Kernel & k_dst , unsigned i_dst )
{
  const std::vector<Field<void,0>*> & field_set =
    k_dst.mesh().schema().get_fields( k_dst.m_entity_type );

  unsigned char * const p = reinterpret_cast<unsigned char*>(k_dst.m_entities);
  const DataMap *       i = k_dst.m_field_map ;
  const DataMap * const e = i + field_set.size();

  for ( ; i != e ; ++i ) {
    if ( i->first ) {
      memory_zero( p + i->first + i->second * i_dst , i->second );
    }
  }
}

void Kernel::copy_fields( Kernel & k_dst , unsigned i_dst ,
                          Kernel & k_src , unsigned i_src )
{
  static const char method[] = "phdmesh::Kernel::copy_fields" ;

  const std::vector<Field<void,0>*> & field_set =
    k_dst.mesh().schema().get_fields( k_dst.m_entity_type );

  unsigned char * const s = reinterpret_cast<unsigned char*>(k_src.m_entities);
  unsigned char * const d = reinterpret_cast<unsigned char*>(k_dst.m_entities);
  DataMap *       j = k_src.m_field_map ;
  DataMap *       i = k_dst.m_field_map ;
  DataMap * const e = i + field_set.size();

  for ( ; i != e ; ++i , ++j ) {

    if ( i->first ) {
      if ( j->first ) {
        if ( i->second == j->second ) {
          memory_copy( d + i->first + i->second * i_dst ,
                       s + j->first + j->second * i_src , i->second );
        }
        else {
          std::ostringstream msg ;
          msg << method ;
          msg << " FAILED WITH INCOMPATIBLE FIELD SIZES" ;
          throw std::runtime_error( msg.str() );
        }
      }
      else {
        memory_zero( d + i->first + i->second * i_dst , i->second );
      }
    }
  }
}

//----------------------------------------------------------------------

namespace {

inline size_t align( size_t nb )
{
  enum { BYTE_ALIGN = 16 };
  const size_t gap = nb % BYTE_ALIGN ;
  if ( gap ) { nb += BYTE_ALIGN - gap ; }
  return nb ;
}

const FieldDimension & dimension( const Field<void,0> & field ,
                                  const PartSet & parts ,
                                  const char * const method )
{
  static const FieldDimension empty ;

  const FieldDimension * dim = & empty ;

  for ( PartSet::const_iterator
        i = parts.begin() ; i != parts.end() ; ++i ) {

    const FieldDimension & tmp = field.dimension( **i );

    if ( tmp.length() ) {
      if ( dim->length() == 0 ) { dim = & tmp ; } 

      if ( tmp != *dim ) { // ERROR
        std::string msg ;
        msg.append( method );
        msg.append( " FAILED WITH INCOMPATIBLE FIELD DIMENSIONS" );
        throw std::runtime_error( msg );
      }
    }
  }

  return *dim ;
}

}

const FieldDimension & dimension( const Field<void,0> & field ,
                                  const Kernel & kernel )
{
  static const char method[] = "phdmesh::dimension( Field , Kernel )" ;

  PartSet parts ; kernel.supersets( parts );

  return dimension( field , parts , method );
}

//----------------------------------------------------------------------

void Kernel::update_state()
{
  if ( 0 == kernel_counter( key() ) ) {

    const Schema & S = m_mesh.schema();
    const std::vector<Field<void,0>*> & field_set = S.get_fields(m_entity_type);

    for ( unsigned i = 0 ; i < field_set.size() ; ) {

      DataMap * const tmp = m_field_map + i ;
      const Field<void,0> & field = * field_set[i] ;
      const unsigned num_state = field.number_of_states();
      i += num_state ;

      if ( 1 < num_state && tmp->first ) {
        unsigned offset[ MaximumFieldStates ] ;

        for ( unsigned j = 0 ; j < num_state ; ++j ) {
          offset[j] = tmp[j].first ;
        }

        for ( unsigned j = 0 ; j < num_state ; ++j ) {
          const unsigned j_new = ( j + num_state - 1 ) % num_state ;
          tmp[j_new].first = offset[j] ;
        }
      }
    }
  }
}

//----------------------------------------------------------------------

Kernel::~Kernel()
{}

void Mesh::destroy_kernel( KernelSet::iterator ik )
{
  Kernel * const k = & * ik ;

  KernelSet & kernel_set = m_kernels[ k->entity_type() ];

  kernel_set.remove( *ik ); // 'ik' is now invalidated

  if ( 0 == kernel_counter( k->key() ) ) {
    free( (void*) k->m_field_map );
  }
  k->m_field_map = NULL ;

  k->~Kernel();

  free( (void*) k );
}

//----------------------------------------------------------------------

KernelSet::iterator
Mesh::declare_kernel( const EntityType arg_entity_type ,
                      const PartSet & entity_parts )
{
  static const char method[] = "phdmesh::Kernel" ;
  const unsigned max = ~((unsigned) 0);

  const unsigned kernel_capacity = m_kernel_capacity[ arg_entity_type ];

  KernelSet & kernel_set = m_kernels[ arg_entity_type ];

  const std::vector< Field<void,0> * > & field_set =
    m_schema.get_fields( arg_entity_type );

  // Entity parts are complete and contain all supersets
  // Initial key is upper bound key for kernel with these parts

  const unsigned key_size = entity_parts.size() + 2 ;

  std::vector<unsigned> key_tmp( key_size );
  key_tmp[0] = key_size - 1 ;
  key_tmp[ key_tmp[0] ] = max ;
  for ( unsigned i = 0 ; i < entity_parts.size() ; ++i ) {
    key_tmp[i+1] = entity_parts[i]->schema_ordinal();
  }

  // Does this kernel already exist?

  const unsigned * const key = & key_tmp[0] ;

  KernelSet::iterator ik = kernel_set.upper_bound( key );

  // Key value for first kernel with these parts

  key_tmp[ key[0] ] = 0 ;

  Kernel * const match_kernel =
    ( ik != kernel_set.begin() ) && kernel_equal( (--ik)->key() , key )
    ? ( & * ik ) : NULL ;

  Kernel * kernel = NULL ;

  if ( match_kernel != NULL ) {
    const size_t cap = ik->capacity();
    if ( ik->size() < cap ) {
      kernel = match_kernel ;
    }
    else if ( match_kernel->key()[ key[0] ] < max ) {
      key_tmp[ key[0] ] = 1 + match_kernel->key()[ key[0] ] ;
    }
    else {
      // ERROR insane number of kernels!
      std::string msg ;
      msg.append( method );
      msg.append( " FAILED due to impossibly large number of kernels" );
      throw std::logic_error( msg );
    }
  }

  // If it does not exist must allocate and insert

  if ( kernel == NULL ) {

    const unsigned num_fields = field_set.size();

    Kernel::DataMap * field_map = NULL ;

    if ( match_kernel != NULL ) {
      field_map = match_kernel->m_field_map ;
    }
    else {
      field_map = reinterpret_cast<Kernel::DataMap*>(
                    malloc( sizeof(Kernel::DataMap) * ( num_fields + 1 ) ) );

      size_t value_offset = 0 ;

      value_offset += align( sizeof(Entity*) * kernel_capacity );

      for ( unsigned i = 0 ; i < num_fields ; ++i ) {
        const Field<void,0>  & field = * field_set[i] ;
        const FieldDimension & dim   = dimension( field, entity_parts, method);
        const unsigned value_size    = dim.size();

        field_map[i].first  = value_offset ;
        field_map[i].second = value_size ;

        value_offset += align( value_size * kernel_capacity );
      }
      field_map[ num_fields ].first  = value_offset ;
      field_map[ num_fields ].second = 0 ;
    }

    // Allocation size:
    //   sizeof(Kernel) +
    //   key_size * sizeof(unsigned) +
    //   sizeof(Entity*) * capacity() +
    //   sum[number_of_fields]( fieldsize * capacity )

    const size_t size = align( sizeof(Kernel) ) +
                        align( sizeof(unsigned) * key_size ) +
                        field_map[ num_fields ].first ;

    // All fields checked and sized, Ready to allocate

    unsigned char * ptr = (unsigned char *) malloc( size );

    kernel = (Kernel *) ptr ; ptr += align( sizeof(Kernel) );

    {
      unsigned * new_key = (unsigned *) ptr ;

      ptr += align( sizeof(unsigned) * key_size );

      for ( unsigned i = 0 ; i < key_size ; ++i ) { new_key[i] = key[i] ; }

      new(kernel) Kernel( *this , arg_entity_type , new_key );
    }

    kernel->m_size      = 0 ;
    kernel->m_capacity  = kernel_capacity ;
    kernel->m_field_map = field_map ;
    kernel->m_entities  = (Entity **) ptr ;

    std::pair<KernelSet::iterator,bool> result = kernel_set.insert( kernel );

    if ( ! result.second ) {
      std::string msg ;
      msg.append( method );
      msg.append( " FAILED INSERTION" );
      throw std::logic_error( msg );
    }

    ik = result.first ;
  }
  return ik ;
}

//----------------------------------------------------------------------

void Mesh::insert_entity( Entity & e , const PartSet & parts )
{
  const KernelSet::iterator ik_old = e.m_kernel ;
  const unsigned            i_old  = e.m_kernel_ord ;

  KernelSet::iterator ik = declare_kernel( e.entity_type() , parts );

  // If changing kernels then copy its field values from old to new kernel
  if ( ik_old ) {
    Kernel::copy_fields( *ik , ik->m_size , *ik_old , i_old );
  }
  else {
    Kernel::zero_fields( *ik , ik->m_size );
  }

  // Set the new kernel
  e.m_kernel     = ik ;
  e.m_kernel_ord = ik->m_size ;
  ik->m_entities[ ik->m_size ] = & e ;
  ++( ik->m_size );

  // If changing kernels then remove the entity from the kernel,
  if ( ik_old ) { remove_entity( ik_old , i_old ); }
}

//----------------------------------------------------------------------

void Mesh::remove_entity( KernelSet::iterator ik , unsigned i )
{
  const EntityType entity_type = ik->m_entity_type ;

  KernelSet & kset = m_kernels[ entity_type ] ;

  // Find the last compatible kernel and move the
  // last entity up to fill in the gap.
  // Compatible entities have the same part list (key.first)
  // and incrementing ordinals (key.second).
  // Thus the next kernel with a zero ordinal will have
  // a different part list.

  const KernelSet::iterator ek = kset.end();

  KernelSet::iterator jk = ik ;
  while ( ++jk != ek && kernel_counter( jk->key() ) );
  --jk ;

  // Only move if not the last entity being removed

  if ( jk != ik || ik->m_size != i + 1 ) {

    // Not the same kernel or not the last entity

    // Copy last entity in jk to ik slot i

    Kernel::copy_fields( *ik , i , *jk , jk->m_size - 1 );

    ik->m_entities[i] = jk->m_entities[ jk->m_size - 1 ];
    ik->m_entities[i]->m_kernel     = ik ;
    ik->m_entities[i]->m_kernel_ord = i ;
  }

  --( jk->m_size );

  if ( jk->m_size != 0 ) {
    jk->m_entities[ jk->m_size ] = NULL ;
  }
  else {
    destroy_kernel( jk );
  }
}

//----------------------------------------------------------------------

std::ostream &
Kernel::print( std::ostream & os , const std::string & lead ) const
{
  const Schema & schema = m_mesh.schema();
  const PartSet & parts = schema.get_parts();
  const char * const entity_name = entity_type_name( m_entity_type );

  const unsigned * k = key();
  const unsigned n = *k ; ++k ;

  os << lead
     << "Kernel(" << entity_name << " : " ;
  for ( unsigned i = 1 ; i < n ; ++i , ++k ) {
    const std::string & name = parts[ *k ]->name(); os << " " << name ;
  }

  os << " " << *k << " ){ "
     << m_size << " of " << m_capacity << " }"
     << std::endl << lead
     << "  members {" ;

  for ( unsigned j = 0 ; j < m_size ; ++j ) {
    const long id = m_entities[j]->identifier(); os << " " << id ;
  }
  os << " }" << std::endl ;

  return os ;
}

//----------------------------------------------------------------------

} // namespace phdmesh

