#ifndef TEST_TILE_HPP
#define TEST_TILE_HPP

#include <Kokkos_Core.hpp>

namespace TestTile {

template < typename Device , typename TileLayout>
struct ReduceTileErrors
{
  typedef Device execution_space ;

  typedef Kokkos::View< ptrdiff_t**, TileLayout, Device>  array_type;
  typedef Kokkos::View< ptrdiff_t[ TileLayout::N0 ][ TileLayout::N1 ], Kokkos::LayoutLeft , Device >  tile_type ;

  array_type m_array ;

  typedef ptrdiff_t value_type;

  ReduceTileErrors( array_type a )
    : m_array(a)
  {}


  KOKKOS_INLINE_FUNCTION
  static void init( value_type & errors )
  {
    errors = 0;
  }

  KOKKOS_INLINE_FUNCTION
  static void join( volatile value_type & errors ,
                    const volatile value_type & src_errors )
  {
    errors += src_errors;
  }

  // Initialize
  KOKKOS_INLINE_FUNCTION
  void operator()( size_t iwork ) const
  {
    const size_t i = iwork % m_array.dimension_0();
    const size_t j = iwork / m_array.dimension_0();
    if ( j < m_array.dimension_1() ) {
      m_array(i,j) = & m_array(i,j) - & m_array(0,0);

// printf("m_array(%d,%d) = %d\n",int(i),int(j),int(m_array(i,j)));

    }
  }

  // Verify:
  KOKKOS_INLINE_FUNCTION
  void operator()( size_t iwork , value_type & errors ) const
  {
    const size_t tile_dim0 = ( m_array.dimension_0() + TileLayout::N0 - 1 ) / TileLayout::N0 ;
    const size_t tile_dim1 = ( m_array.dimension_1() + TileLayout::N1 - 1 ) / TileLayout::N1 ;

    const size_t itile = iwork % tile_dim0 ;
    const size_t jtile = iwork / tile_dim0 ;

    if ( jtile < tile_dim1 ) {

      tile_type tile = Kokkos::tile_subview( m_array , itile , jtile );

      if ( tile(0,0) != ptrdiff_t(( itile + jtile * tile_dim0 ) * TileLayout::N0 * TileLayout::N1 ) ) {
        ++errors ;
      }
      else {

        for ( size_t j = 0 ; j < size_t(TileLayout::N1) ; ++j ) {
        for ( size_t i = 0 ; i < size_t(TileLayout::N0) ; ++i ) {
          const size_t iglobal = i + itile * TileLayout::N0 ;
          const size_t jglobal = j + jtile * TileLayout::N1 ;

          if ( iglobal < m_array.dimension_0() && jglobal < m_array.dimension_1() ) {
            if ( tile(i,j) != ptrdiff_t( tile(0,0) + i + j * TileLayout::N0 ) ) ++errors ;

// printf("tile(%d,%d)(%d,%d) = %d\n",int(itile),int(jtile),int(i),int(j),int(tile(i,j)));

          }
        }
        }
      }
    }
  }
};

template< class Space , unsigned N0 , unsigned N1 >
void test( const size_t dim0 , const size_t dim1 )
{
  typedef Kokkos::LayoutTileLeft<N0,N1>  array_layout ;
  typedef ReduceTileErrors< Space , array_layout > functor_type ;

  const size_t tile_dim0 = ( dim0 + N0 - 1 ) / N0 ;
  const size_t tile_dim1 = ( dim1 + N1 - 1 ) / N1 ;
  
  typename functor_type::array_type array("",dim0,dim1);

  Kokkos::parallel_for( Kokkos::RangePolicy<Space,size_t>(0,dim0*dim1) , functor_type( array ) );

  ptrdiff_t error = 0 ;

  Kokkos::parallel_reduce( Kokkos::RangePolicy<Space,size_t>(0,tile_dim0*tile_dim1) , functor_type( array ) , error );

  EXPECT_EQ( error , ptrdiff_t(0) );
}

} /* namespace TestTile */

#endif //TEST_TILE_HPP

