/*------------------------------------------------------------------------*/
/*                shards : Shared Discretization Tools                    */
/*                Copyright (2008) Sandia Corporation                     */
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

#define HAVE_SHARDS_DEBUG

#include <iostream>
#include <stdexcept>
#include <Shards_CellTopology.hpp>
#include <Shards_BasicTopologies.hpp>

namespace {

#define REQUIRE( S ) \
  if ( ! ( S ) ) { throw std::runtime_error(std::string(#S)); }

#define REQUIRE_EX( S ) \
  { bool flag = true ; \
    try { S ; } catch( const std::exception & ) { flag = false ; } \
    if ( flag ) { throw std::runtime_error(std::string(#S)); } \
  }

template< class Traits , unsigned Dim , unsigned Ord >
void test_subcell( const shards::CellTopology & parent )
{
  typedef typename Traits::template subcell< Dim , Ord > Subcell ;
  typedef typename Subcell::topology SubcellTraits ;

  std::cout << "    subcell(" << Dim << "," << Ord << ") = "
            << parent.getTopology(Dim,Ord)->name << std::endl ;

  REQUIRE( SubcellTraits::key          == parent.getKey(Dim,Ord) )
  REQUIRE( SubcellTraits::side_count   == parent.getSideCount(Dim,Ord) )
  REQUIRE( SubcellTraits::node_count   == parent.getNodeCount(Dim,Ord) )
  REQUIRE( SubcellTraits::edge_count   == parent.getEdgeCount(Dim,Ord) )
  REQUIRE( SubcellTraits::vertex_count == parent.getVertexCount(Dim,Ord) )
  REQUIRE( SubcellTraits::vertex_count == parent.getVertexCount(Dim,Ord) )
}


template< class Traits , unsigned Dim , unsigned Count >
struct test_all_subcell
{
  test_all_subcell( const shards::CellTopology & parent )
  {
    test_all_subcell< Traits , Dim , Count - 1 > previous( parent );
    test_subcell< Traits , Dim , Count - 1 >( parent );
  }
};

template< class Traits , unsigned Dim >
struct test_all_subcell<Traits,Dim,1>
{
  test_all_subcell( const shards::CellTopology & parent )
  {
    test_subcell< Traits , Dim , 0 >( parent );
  }
};

template< class Traits , unsigned Dim >
struct test_all_subcell<Traits,Dim,0>
{
  test_all_subcell( const shards::CellTopology & )
  { }
};

template< class Traits >
void test_cell()
{
  typedef typename Traits::template subcell< 0 > Subcell_0 ;
  typedef typename Traits::template subcell< 1 > Subcell_1 ;
  typedef typename Traits::template subcell< 2 > Subcell_2 ;
  typedef typename Traits::template subcell< 3 > Subcell_3 ;
  typedef typename Traits::template subcell< Traits::dimension > SelfSubcell ;
  typedef typename SelfSubcell::topology SelfTraits ;

  const CellTopologyData * const cell_data =
    shards::getCellTopologyData< Traits >();

  std::cout << "  Testing " << cell_data->name << std::endl ;

  const shards::CellTopology top( cell_data );

  enum { same_type = shards::SameType< typename Traits::Traits , SelfTraits >::value };

  REQUIRE( cell_data == shards::getCellTopologyData< SelfTraits >() );
  REQUIRE( same_type );
  REQUIRE( cell_data            == top.getTopology() )
  REQUIRE( Traits::key          == top.getKey() )
  REQUIRE( Traits::side_count   == top.getSideCount() )
  REQUIRE( Traits::node_count   == top.getNodeCount() )
  REQUIRE( Traits::edge_count   == top.getEdgeCount() )
  REQUIRE( Traits::vertex_count == top.getVertexCount() )
  REQUIRE( Traits::vertex_count == top.getVertexCount() )
  REQUIRE( Subcell_0::count     == top.getSubcellCount(0) )
  REQUIRE( Subcell_1::count     == top.getSubcellCount(1) )
  REQUIRE( Subcell_2::count     == top.getSubcellCount(2) )
  REQUIRE( Subcell_3::count     == top.getSubcellCount(3) )

  REQUIRE_EX( top.getSubcellCount(4) )
  REQUIRE_EX( top.getSubcellCount(5) )

  test_all_subcell< Traits , 0 , Subcell_0::count > testd0( top );
  test_all_subcell< Traits , 1 , Subcell_1::count > testd1( top );
  test_all_subcell< Traits , 2 , Subcell_2::count > testd2( top );
  test_all_subcell< Traits , 3 , Subcell_3::count > testd3( top );
}

void local_test_cell_topology()
{
  test_cell< shards::Node >();
  test_cell< shards::Particle >();

  test_cell< shards::Line<2> >();
  test_cell< shards::Line<3> >();
  test_cell< shards::ShellLine<2> >();
  test_cell< shards::ShellLine<3> >();
  test_cell< shards::Beam<2> >();
  test_cell< shards::Beam<3> >();

  test_cell< shards::Triangle<3> >();
  test_cell< shards::Triangle<6> >();
  test_cell< shards::ShellTriangle<3> >();
  test_cell< shards::ShellTriangle<6> >();

  test_cell< shards::Quadrilateral<4> >();
  test_cell< shards::Quadrilateral<8> >();
  test_cell< shards::Quadrilateral<9> >();
  test_cell< shards::ShellQuadrilateral<4> >();
  test_cell< shards::ShellQuadrilateral<8> >();
  test_cell< shards::ShellQuadrilateral<9> >();

  test_cell< shards::Tetrahedron<4> >();
  test_cell< shards::Tetrahedron<10> >();

  test_cell< shards::Pyramid<5> >();
  test_cell< shards::Pyramid<13> >();
  test_cell< shards::Pyramid<14> >();

  test_cell< shards::Wedge<6> >();
  test_cell< shards::Wedge<15> >();
  test_cell< shards::Wedge<18> >();

  test_cell< shards::Hexahedron<8> >();
  test_cell< shards::Hexahedron<20> >();
  test_cell< shards::Hexahedron<27> >();
}

}

void test_shards_cell_topology()
{
  static const char method[] = "test_shards_cell_topology" ;

  try {
    local_test_cell_topology();
    std::cout << method << "\n" << "End Result: TEST PASSED" << std::endl ;
  }
  catch( const std::exception & x ) {
    std::cout << method << "\n" << "End Result: TEST FAILED: " << x.what() << std::endl ;
  }
  catch( ... ) {
    std::cout << method << "\n" << "End Result: TEST FAILED: <unknown>" << std::endl ;
  }
}

