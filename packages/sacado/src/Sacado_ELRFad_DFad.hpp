// @HEADER
// ***********************************************************************
//
//                           Sacado Package
//                 Copyright (2006) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
// USA
// Questions? Contact David M. Gay (dmgay@sandia.gov) or Eric T. Phipps
// (etphipp@sandia.gov).
//
// ***********************************************************************
// @HEADER

#ifndef SACADO_ELRFAD_DFAD_HPP
#define SACADO_ELRFAD_DFAD_HPP

#include "Sacado_ELRFad_GeneralFadExpr.hpp"
#include "Sacado_ELRFad_DFadTraits.hpp"
#include "Sacado_Fad_DynamicStorage.hpp"
#include "Sacado_dummy_arg.hpp"

namespace Sacado {

  namespace ELRFad {

    /*!
     * \brief Forward-mode AD class using dynamic memory allocation and
     * expression-level reverse mode expression templates.
     */
    /*!
     * This is the user-level class for forward mode AD with dynamic
     * memory allocation, and is appropriate for whenever the number
     * of derivative components is not known at compile time.  The user
     * interface is provided by Sacado::Fad::GeneralFad.
     */
    template <typename ValueT>
    class DFad : public Expr< GeneralFad<ValueT,Fad::DynamicStorage<ValueT> > > {

    public:

      //! Typename of scalar's (which may be different from ValueT)
      typedef typename ScalarType<ValueT>::type ScalarT;

      //! Turn DFad into a meta-function class usable with mpl::apply
      template <typename T>
      struct apply {
        typedef DFad<T> type;
      };

      /*!
       * @name Initialization methods
       */
      //@{

      //! Default constructor.
      /*!
       * Initializes value to 0 and derivative array is empty
       */
      KOKKOS_INLINE_FUNCTION
      DFad() :
        Expr< GeneralFad< ValueT,Fad::DynamicStorage<ValueT> > >() {}

      //! Constructor with supplied value \c x of type ValueT
      /*!
       * Initializes value to \c x and derivative array is empty
       */
      KOKKOS_INLINE_FUNCTION
      DFad(const ValueT& x) :
        Expr< GeneralFad< ValueT,Fad::DynamicStorage<ValueT> > >(x) {}

      //! Constructor with supplied value \c x of type ScalarT
      /*!
       * Initializes value to \c ValueT(x) and derivative array is empty.
       * Creates a dummy overload when ValueT and ScalarT are the same type.
       */
      KOKKOS_INLINE_FUNCTION
      DFad(const typename dummy<ValueT,ScalarT>::type& x) :
        Expr< GeneralFad< ValueT,Fad::DynamicStorage<ValueT> > >(ValueT(x)) {}

      //! Constructor with size \c sz and value \c x
      /*!
       * Initializes value to \c x and derivative array 0 of length \c sz
       */
      KOKKOS_INLINE_FUNCTION
      DFad(const int sz, const ValueT& x) :
        Expr< GeneralFad< ValueT,Fad::DynamicStorage<ValueT> > >(sz,x) {}

      //! Constructor with size \c sz, index \c i, and value \c x
      /*!
       * Initializes value to \c x and derivative array of length \c sz
       * as row \c i of the identity matrix, i.e., sets derivative component
       * \c i to 1 and all other's to zero.
       */
      KOKKOS_INLINE_FUNCTION
      DFad(const int sz, const int i, const ValueT & x) :
        Expr< GeneralFad< ValueT,Fad::DynamicStorage<ValueT> > >(sz,i,x) {}

      //! Copy constructor
      KOKKOS_INLINE_FUNCTION
      DFad(const DFad& x) :
        Expr< GeneralFad< ValueT,Fad::DynamicStorage<ValueT> > >(x) {}

      //! Copy constructor from any Expression object
      template <typename S>
      KOKKOS_INLINE_FUNCTION
      DFad(const Expr<S>& x) :
        Expr< GeneralFad< ValueT,Fad::DynamicStorage<ValueT> > >(x) {}

      //@}

      //! Destructor
      KOKKOS_INLINE_FUNCTION
      ~DFad() {}

      //! Assignment operator with constant right-hand-side
      KOKKOS_INLINE_FUNCTION
      DFad& operator=(const ValueT& v) {
        GeneralFad< ValueT,Fad::DynamicStorage<ValueT> >::operator=(v);
        return *this;
      }

      //! Assignment operator with constant right-hand-side
      /*!
       * Creates a dummy overload when ValueT and ScalarT are the same type.
       */
      KOKKOS_INLINE_FUNCTION
      DFad& operator=(const typename dummy<ValueT,ScalarT>::type& v) {
        GeneralFad< ValueT,Fad::DynamicStorage<ValueT> >::operator=(ValueT(v));
        return *this;
      }

      //! Assignment operator with DFad right-hand-side
      KOKKOS_INLINE_FUNCTION
      DFad& operator=(const DFad& x) {
        GeneralFad< ValueT,Fad::DynamicStorage<ValueT> >::operator=(static_cast<const GeneralFad< ValueT,Fad::DynamicStorage<ValueT> >&>(x));
        return *this;
      }

      //! Assignment operator with any expression right-hand-side
      template <typename S>
      KOKKOS_INLINE_FUNCTION
      DFad& operator=(const Expr<S>& x)
      {
        GeneralFad< ValueT,Fad::DynamicStorage<ValueT> >::operator=(x);
        return *this;
      }

    }; // class DFad<ValueT>

  } // namespace ELRFad

} // namespace Sacado

//
// Classes needed for Kokkos::View< DFad<...> ... > specializations
//
// Users can disable these view specializations either at configure time or
// by defining SACADO_DISABLE_FAD_VIEW_SPEC in their code.
//

#if defined(HAVE_SACADO_KOKKOSCORE) && defined(HAVE_SACADO_VIEW_SPEC) && !defined(SACADO_DISABLE_FAD_VIEW_SPEC)

#include "impl/Kokkos_AnalyzeShape.hpp"
#include "Kokkos_AnalyzeSacadoShape.hpp"
#include "Sacado_ELRFad_ViewFad.hpp"

namespace Kokkos {
namespace Impl {

// Forward declarations
struct ViewSpecializeSacadoFad;
template <typename T,unsigned,unsigned> struct ViewFadType;

//! The View Fad type associated with this type
template< class ValueType, unsigned length, unsigned stride >
struct ViewFadType< Sacado::ELRFad::DFad< ValueType >, length, stride > {
  typedef Sacado::ELRFad::ViewFad<ValueType,length,stride> type;
};

//! The View Fad type associated with this type
template< class ValueType, unsigned length, unsigned stride >
struct ViewFadType< const Sacado::ELRFad::DFad< ValueType >, length, stride > {
  typedef Sacado::ELRFad::ViewFad<const ValueType,length,stride> type;
};

/** \brief  Analyze the array shape of a Sacado::ELRFad::DFad<T>.
 *
 *  This specialization is required so that the array shape of
 *  Kokkos::View< Sacado::ELRFad::DFad<T>, ... >
 *  can be determined at compile-time.
 *
 *  For View purposes, DFad is treated as a dynamic dimension.
 */
template< class ValueType >
struct AnalyzeShape< Sacado::ELRFad::DFad< ValueType > >
  : Shape< sizeof(Sacado::ELRFad::DFad< ValueType >) , 0 > // Treat as a scalar
{
public:

  typedef ViewSpecializeSacadoFad specialize ;

  typedef Shape< sizeof(Sacado::ELRFad::DFad< ValueType >) , 0 > shape ;

  typedef       Sacado::ELRFad::DFad< ValueType >        array_intrinsic_type ;
  typedef const Sacado::ELRFad::DFad< ValueType >  const_array_intrinsic_type ;
  typedef array_intrinsic_type non_const_array_intrinsic_type ;

  typedef       Sacado::ELRFad::DFad< ValueType >  type ;
  typedef const Sacado::ELRFad::DFad< ValueType >  const_type ;
  typedef       Sacado::ELRFad::DFad< ValueType >  non_const_type ;

  typedef       Sacado::ELRFad::DFad< ValueType >  value_type ;
  typedef const Sacado::ELRFad::DFad< ValueType >  const_value_type ;
  typedef       Sacado::ELRFad::DFad< ValueType >  non_const_value_type ;
};

/** \brief  Analyze the array shape of a Sacado::ELRFad::DFad<T>.
 *
 *  This specialization is required so that the array shape of
 *  Kokkos::View< Sacado::ELRFad::DFad<T>, ... >
 *  can be determined at compile-time.
 *
 *  For View purposes, DFad is treated as a dynamic dimension.
 */
template< class ValueType, class Layout >
struct AnalyzeSacadoShape< Sacado::ELRFad::DFad< ValueType >, Layout >
  : ShapeInsert< typename AnalyzeSacadoShape< ValueType, Layout >::shape , 0 >::type
{
private:

  typedef AnalyzeSacadoShape< ValueType, Layout > nested ;

public:

  typedef ViewSpecializeSacadoFad specialize ;

  typedef typename ShapeInsert< typename nested::shape , 0 >::type shape ;

  typedef typename nested::array_intrinsic_type *        array_intrinsic_type ;
  typedef typename nested::const_array_intrinsic_type *  const_array_intrinsic_type ;
  typedef array_intrinsic_type non_const_array_intrinsic_type ;

  typedef       Sacado::ELRFad::DFad< ValueType >  type ;
  typedef const Sacado::ELRFad::DFad< ValueType >  const_type ;
  typedef       Sacado::ELRFad::DFad< ValueType >  non_const_type ;

  typedef       Sacado::ELRFad::DFad< ValueType >  value_type ;
  typedef const Sacado::ELRFad::DFad< ValueType >  const_value_type ;
  typedef       Sacado::ELRFad::DFad< ValueType >  non_const_value_type ;

  typedef typename nested::type           flat_array_type ;
  typedef typename nested::const_type     const_flat_array_type ;
  typedef typename nested::non_const_type non_const_flat_array_type ;
};

} // namespace Impl
} // namespace Kokkos

#endif

#endif // SACADO_ELRFAD_DFAD_HPP
