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

#ifndef phdmesh_Part_hpp
#define phdmesh_Part_hpp

//----------------------------------------------------------------------

#include <iosfwd>
#include <string>
#include <vector>

#include <util/CSet.hpp>
#include <mesh/Types.hpp>

//----------------------------------------------------------------------

namespace phdmesh {

/** Supersets and subsets of parts are of the same schema and
 *  maintained in case-insensitive alphabetical order.
 */
typedef std::vector<Part*> PartSet ;

/** Order a collection of parts: invoke sort and then unique */
void order( std::vector<Part*> & );

/** Insert a part into a properly ordered collection of parts.
 *  Return if this is a new insertion.
 */
bool insert( PartSet & , Part & );

/** Find a part by name in a properly ordered collection of parts. */
Part * find( const PartSet & , const std::string & );

/** Query containment for properly ordered PartSet */
bool contain( const PartSet & , const Part & );
bool contain( const PartSet & , const PartSet & );

/** Query cardinality of intersection of two PartSets */
unsigned intersect( const PartSet & , const PartSet & );

/** Generate intersection of two PartSets */
unsigned intersect( const PartSet & , const PartSet & , PartSet & );

/** Query if two parts intersect:
 *  If one is a subset of the other or they share a common subset
 */
bool intersect( const Part & , const Part & );

//----------------------------------------------------------------------
/** Verify consistency of supersets, subsets, and partitions */
bool verify( const Part & , std::string & );

/** Print a part.  Each line starts with the given leader string */
std::ostream & print( std::ostream & , const char * const , const Part & );

//----------------------------------------------------------------------
/** A part within a mesh.
 *  Parts are derived from component sets.
 */
class Part {
public:

  /** Mesh schema in which this part resides */
  Schema & schema() const { return m_schema ; }

  /** Text name of this mesh part */
  const std::string & name() const { return m_name ; }

  /** Ordinal of this part within the mesh schema */
  unsigned schema_ordinal() const { return m_schema_ordinal ; }

  /** Parts that are supersets of this part.  */
  const PartSet & supersets() const { return m_supersets ; }

  /** Parts that are subsets of this part. */
  const PartSet & subsets() const { return m_subsets ; }

  /** Parts for which this part is defined to be the intersection */
  const PartSet & intersection_of() const { return m_intersect ; }

  /** Add subset */
  void add_subset( Part & );

  bool operator == ( const Part & rhs ) const { return this == & rhs ; }
  bool operator != ( const Part & rhs ) const { return this != & rhs ; }

  const CSet & cset_query() const { return m_cset ; }
        CSet & cset_update();

private:

  /* Only a mesh Schema can create and delete parts */
  friend class Schema ;

  /** Construct a part within a given mesh */
  Part( Schema & , const std::string & , const PartSet & );

  ~Part();
  Part();
  Part( const Part & );
  Part & operator = ( const Part & );

  std::string  m_name ;
  CSet         m_cset ;
  Schema     & m_schema ;
  unsigned     m_schema_ordinal ;
  PartSet      m_subsets ;
  PartSet      m_supersets ;
  PartSet      m_intersect ;
};

} // namespace phdmesh

//----------------------------------------------------------------------
//----------------------------------------------------------------------

#endif

