/*
//@HEADER
// ************************************************************************
// 
//          Kokkos: Node API and Parallel Node Kernels
//              Copyright (2008) Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ************************************************************************
//@HEADER
*/

#ifndef KOKKOS_IMPL_VIEW_FACTORY_HPP
#define KOKKOS_IMPL_VIEW_FACTORY_HPP

#include <iostream>

#include <impl/KokkosArray_StaticAssert.hpp>
#include <impl/KokkosArray_ArrayTraits.hpp>

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace KokkosArray {

namespace Impl {

template< class > struct ViewCreateMirror ;

template< class DataType , class LayoutType , class DeviceType >
struct ViewCreateMirror< View< DataType , LayoutType , DeviceType > >
{
  typedef View< DataType , LayoutType , DeviceType > output_type ;

  inline static
  output_type create( const output_type & src ) { return src ; }

  template< class DeviceSrc >
  inline static
  output_type create( const View< DataType , LayoutType , DeviceSrc > & src )
  {
    return output_type( "mirror" , src.shape() );
  }
};

} // namespace Impl

template< class DataType , class LayoutType , class DeviceType >
typename View< DataType , LayoutType , DeviceType >::HostMirror
create_mirror_view( const View<DataType,LayoutType,DeviceType> & input )
{
  typedef View< DataType , LayoutType , DeviceType > input_type ;
  typedef typename input_type::HostMirror            output_type ;

  return Impl::ViewCreateMirror< output_type >::create( input );
}

template< class DataType , class LayoutType , class DeviceType >
typename View< DataType , LayoutType , DeviceType >::HostMirror
create_mirror( const View<DataType,LayoutType,DeviceType> & input )
{
  typedef View< DataType , LayoutType , DeviceType > input_type ;
  typedef typename input_type::HostMirror            output_type ;

#if KOKKOS_MIRROR_VIEW_OPTIMIZE
  return Impl::ViewCreateMirror< output_type >::create( input );
#else
  return output_type( "mirror" , input.shape() );
#endif
}

} // namespace KokkosArray

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace KokkosArray {

/** \brief  Span of a vector */
template< typename ValueType , class LayoutSpec , class DeviceType >
inline
View< ValueType[] , LayoutSpec , DeviceType >
view( const View< ValueType[] , LayoutSpec , DeviceType > & input ,
      const size_t iBeg , const size_t iEnd )
{
  typedef View< ValueType[] , LayoutSpec , DeviceType > input_type ;
  typedef typename input_type::shape_type input_shape ;

  typedef typename Impl::assert_shape_is_rank_one< input_shape >::type ok_rank ;

  return Impl::Factory< void , input_type >::view( input , iBeg , iEnd );
}

/** \brief  Vector of a multivector */
template< typename ValueType , class DeviceType >
inline
View< ValueType[] , LayoutLeft , DeviceType >
view( const View< ValueType[][0] , LayoutLeft , DeviceType > & input ,
      const size_t J )
{
  typedef typename
    Impl::StaticAssert< 0 == Impl::rank<ValueType>::value >::type ok_rank ;

  typedef View< ValueType[][0] , LayoutLeft , DeviceType > input_type ;

  return Impl::Factory< void , input_type >::view( input , J );
}

/** \brief  Value within a multivector */
template< typename ValueType , class DeviceType >
inline
View< ValueType , LayoutLeft , DeviceType >
view( const View< ValueType[][0] , LayoutLeft , DeviceType > & input ,
      const size_t I , const size_t J )
{
  typedef typename
    Impl::StaticAssert< 0 == Impl::rank<ValueType>::value >::type ok_rank ;

  typedef View< ValueType[][0] , LayoutLeft , DeviceType > input_type ;

  return Impl::Factory< void , input_type >::view( input , I , J );
}

/** \brief  Value within a vector */
template< typename ValueType , class DeviceType >
inline
View< ValueType , LayoutLeft , DeviceType >
view( const View< ValueType[] , LayoutLeft , DeviceType > & input ,
      const size_t I )
{
  typedef typename
    Impl::StaticAssert< 0 == Impl::rank<ValueType>::value >::type ok_rank ;

  typedef View< ValueType[] , LayoutLeft , DeviceType > input_type ;

  return Impl::Factory< void , input_type >::view( input , I );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/** \brief  Deep copy compatible arrays */

namespace Impl {

template< class DstType ,
          class SrcType ,
          class SameValue = typename
            is_same< typename DstType::value_type ,
                     typename remove_const<
                       typename SrcType::value_type >::type
                   >::type ,
          class SameLayout = typename
            is_same< typename DstType::array_layout ,
                     typename SrcType::array_layout >::type ,
          class SameRank = typename
             bool_< DstType::Rank == SrcType::Rank >::type >
struct ViewDeepCopy ;

// Deep copy compatible views:

template< class DataDst , class LayoutDst , class DeviceDst ,
          class DataSrc , class LayoutSrc , class DeviceSrc >
struct ViewDeepCopy< View< DataDst , LayoutDst , DeviceDst > ,
                     View< DataSrc , LayoutSrc , DeviceSrc > ,
                     true_type /* Same value_type   */ ,
                     true_type /* Same array_layout */ ,
                     true_type /* Same rank */ >
{
  typedef View< DataDst , LayoutDst , DeviceDst >  dst_type ;
  typedef View< DataSrc , LayoutSrc , DeviceSrc >  src_type ;

  inline static
  void apply( const dst_type & dst , const src_type & src )
  {
    if ( dst != src ) {
      assert_shapes_are_equal( dst.shape() , src.shape() );

      DeepCopy<typename dst_type::value_type,DeviceDst,DeviceSrc>(
        dst.ptr_on_device() ,
        src.ptr_on_device() ,
        allocation_count( dst.shape() ) );
    }
  }
};

} // namespace Impl

template< typename ValueType , class LayoutSrc , class DeviceSrc >
inline
void deep_copy( ValueType & dst ,
                const View< ValueType , LayoutSrc , DeviceSrc > & src )
{
  typedef View< ValueType , LayoutSrc , DeviceSrc > src_type ;
  typedef typename src_type::shape_type             src_shape ;

  typedef typename Impl::assert_shape_is_rank_zero< src_shape >::type ok_rank ;

  Impl::DeepCopy<ValueType,Host,DeviceSrc>( & dst , src.ptr_on_device() , 1 );
}

template< typename ValueType , class LayoutDst , class DeviceDst >
inline
void deep_copy( const View< ValueType , LayoutDst , DeviceDst > & dst ,
                const ValueType & src )
{
  typedef View< ValueType , LayoutDst , DeviceDst > dst_type ;
  typedef typename dst_type::shape_type             dst_shape ;

  typedef typename Impl::assert_shape_is_rank_zero< dst_shape >::type ok_rank ;

  Impl::DeepCopy<ValueType,DeviceDst,Host>( dst.ptr_on_device(), & src , 1 );
}

// Deep copy a span of rank 1 arrays:

template< class DataDst , class LayoutDst , class DeviceDst ,
          class DataSrc , class LayoutSrc , class DeviceSrc >
inline
void deep_copy( const View< DataDst , LayoutDst , DeviceDst > & dst ,
                const View< DataSrc , LayoutSrc , DeviceSrc > & src ,
                size_t count )
{
  typedef View< DataDst , LayoutDst , DeviceDst > dst_type ;
  typedef View< DataSrc , LayoutSrc , DeviceSrc > src_type ;

  typedef typename dst_type::shape_type  dst_shape ;
  typedef typename src_type::shape_type  src_shape ;

  typedef typename dst_type::value_type dst_value_type ;
  typedef typename src_type::value_type src_value_type ;

  // Verify arrays are rank 1

  typedef typename
    Impl::assert_shape_is_rank_one< dst_shape >::type ok_dst_rank ;

  typedef typename
    Impl::assert_shape_is_rank_one< dst_shape >::type ok_dst_rank ;

  // Verify value is the same type

  typedef typename
    Impl::StaticAssertSame< dst_value_type ,
                            typename Impl::remove_const< src_value_type >::type
                          >::type  ok_assign ;

  // Copy if the destination is not simply a view of the source:

  if ( count && dst != src ) {
    Impl::assert_shape_bounds( dst.shape() , count - 1 );
    Impl::assert_shape_bounds( src.shape() , count - 1 );

    Impl::DeepCopy<dst_value_type,DeviceDst,DeviceSrc>(
      dst.ptr_on_device() ,
      src.ptr_on_device() ,
      count );
  }
}

// Deep copy arbitrary arrays:

template< class DataDst , class LayoutDst , class DeviceDst ,
          class DataSrc , class LayoutSrc , class DeviceSrc >
inline
void deep_copy( const View< DataDst , LayoutDst , DeviceDst > & dst ,
                const View< DataSrc , LayoutSrc , DeviceSrc > & src )
{
  typedef View< DataDst , LayoutDst , DeviceDst > dst_type ;
  typedef View< DataSrc , LayoutSrc , DeviceSrc > src_type ;

  Impl::ViewDeepCopy<dst_type,src_type>::apply( dst , src );
}

//----------------------------------------------------------------------------

} /* namespace KokkosArray */

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace KokkosArray {
namespace Impl {

/** \brief  Create subviews of multivectors */
template< typename ValueType , class LayoutSpec , class Device >
struct Factory< void , View< ValueType[][0] , LayoutSpec , Device > >
{
  typedef typename StaticAssert< rank<ValueType>::value == 0 >::type ok_rank ;

  // The multivector type:
  typedef View< ValueType[][0] , LayoutSpec , Device > input_type ;

  typedef typename Device::memory_space memory_space ;

  inline static
  View< ValueType[] , LayoutSpec , Device >
  view( const input_type & input , const size_t J )
  {
    View< ValueType[] , LayoutSpec , Device > output ;

    if ( input ) {
      output.m_shape.N0 = input.m_shape.N0 ;
      output.m_ptr_on_device = input.ptr_on_device( 0 , J );
      memory_space::increment( output.m_ptr_on_device );
    }

    return output ;
  }

  inline static
  View< ValueType , LayoutSpec , Device >
  view( const input_type & input , const size_t I , size_t J )
  {

    View< ValueType , LayoutSpec , Device > output ;

    if ( input ) {
      output.m_ptr_on_device = input.ptr_on_device( I , J );
      memory_space::increment( output.m_ptr_on_device );
    }

    return output ;
  }
};

/** \brief  Create subview of vectors */
template< typename ValueType , class LayoutSpec , class Device >
struct Factory< void , View< ValueType[] , LayoutSpec , Device > >
{
  typedef typename StaticAssert< rank<ValueType>::value == 0 >::type ok_rank ;

  // The multivector type:
  typedef View< ValueType[] , LayoutSpec , Device > input_type ;

  typedef typename Device::memory_space memory_space ;

  inline static
  View< ValueType , LayoutSpec , Device >
  view( const input_type & input , const size_t I )
  {
    View< ValueType , LayoutSpec , Device > output ;

    if ( input ) {
      output.m_ptr_on_device = & input( I );
      memory_space::increment( output.m_ptr_on_device );
    }

    return output ;
  }

  inline static
  View< ValueType[] , LayoutSpec , Device >
  view( const input_type & input , const size_t iBeg , const size_t iEnd )
  {
    View< ValueType[] , LayoutSpec , Device > output ;

    if ( input ) {
      output.m_shape.N0 = iEnd - iBeg ;
      output.m_ptr_on_device = input.m_ptr_on_device + iBeg ;
      memory_space::increment( output.m_ptr_on_device );
    }

    return output ;
  }
};

} // namespace Impl
} // namespace KokkosArray

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#endif /* #ifndef KOKKOS_IMPL_VIEW_FACTORY_HPP */

