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
//
// The forward-mode AD classes in Sacado are a derivative work of the
// expression template classes in the Fad package by Nicolas Di Cesare.
// The following banner is included in the original Fad source code:
//
// ************ DO NOT REMOVE THIS BANNER ****************
//
//  Nicolas Di Cesare <Nicolas.Dicesare@ann.jussieu.fr>
//  http://www.ann.jussieu.fr/~dicesare
//
//            CEMRACS 98 : C++ courses,
//         templates : new C++ techniques
//            for scientific computing
//
//********************************************************
//
//  A short implementation ( not all operators and
//  functions are overloaded ) of 1st order Automatic
//  Differentiation in forward mode (FAD) using
//  EXPRESSION TEMPLATES.
//
//********************************************************
// @HEADER

#include "Sacado_ConfigDefs.h"

#define SACADO_GENERALFAD_ENABLE_FUNC \
  typename Sacado::mpl::enable_if< \
    Sacado::mpl::is_same< \
      typename Sacado::Fad::Expr<S>::value_type, \
      typename Sacado::Fad::GeneralFad<T,Storage>::value_type\
    >, \
    Sacado::Fad::GeneralFad<T,Storage>& \
  >::type

template <typename T, typename Storage>
template <typename S>
KOKKOS_INLINE_FUNCTION
Sacado::Fad::GeneralFad<T,Storage>::
GeneralFad(const Expr<S>& x, SACADO_ENABLE_EXPR_CTOR_DEF) :
  Storage(x.size(), T(0.)),
  update_val_(x.updateValue())
{
  const int sz = x.size();

  if (sz) {
    if (x.hasFastAccess())
      for(int i=0; i<sz; ++i)
        this->fastAccessDx(i) = x.fastAccessDx(i);
    else
      for(int i=0; i<sz; ++i)
        this->fastAccessDx(i) = x.dx(i);
  }

  if (update_val_)
    this->val() = x.val();
}


template <typename T, typename Storage>
KOKKOS_INLINE_FUNCTION
void
Sacado::Fad::GeneralFad<T,Storage>::
diff(const int ith, const int n)
{
  if (this->size() != n)
    this->resize(n);

  this->zero();
  this->fastAccessDx(ith) = T(1.);

}

template <typename T, typename Storage>
KOKKOS_INLINE_FUNCTION
Sacado::Fad::GeneralFad<T,Storage>&
Sacado::Fad::GeneralFad<T,Storage>::
operator=(const Sacado::Fad::GeneralFad<T,Storage>& x)
{
  // Copy value & dx_
  Storage::operator=(x);
  update_val_ = x.update_val_;

  return *this;
}

template <typename T, typename Storage>
template <typename S>
KOKKOS_INLINE_FUNCTION
SACADO_GENERALFAD_ENABLE_FUNC
Sacado::Fad::GeneralFad<T,Storage>::
operator=(const Expr<S>& x)
{
  const int xsz = x.size();

  if (xsz != this->size())
    this->resizeAndZero(xsz);

  const int sz = this->size();

  // For ViewStorage, the resize above may not in fact resize the
  // derivative array, so it is possible that sz != xsz at this point.
  // The only valid use case here is sz > xsz == 0, so we use sz in the
  // assignment below

  if (sz) {
    if (x.hasFastAccess())
      for(int i=0; i<sz; ++i)
        this->fastAccessDx(i) = x.fastAccessDx(i);
    else
      for(int i=0; i<sz; ++i)
        this->fastAccessDx(i) = x.dx(i);
  }

  update_val_ = x.updateValue();
  if (update_val_)
    this->val() = x.val();

  return *this;
}

template <typename T, typename Storage>
KOKKOS_INLINE_FUNCTION
Sacado::Fad::GeneralFad<T,Storage>&
Sacado::Fad::GeneralFad<T,Storage>::
operator += (const Sacado::Fad::GeneralFad<T,Storage>& x)
{
  const int xsz = x.size(), sz = this->size();

#if defined(SACADO_DEBUG) && !defined(__CUDA_ARCH__ )
  if ((xsz != sz) && (xsz != 0) && (sz != 0))
    throw "Fad Error:  Attempt to assign with incompatible sizes";
#endif

  if (xsz) {
    if (sz) {
      for (int i=0; i<sz; ++i)
        this->fastAccessDx(i) += x.fastAccessDx(i);
    }
    else {
      this->resizeAndZero(xsz);
      for (int i=0; i<xsz; ++i)
        this->fastAccessDx(i) = x.fastAccessDx(i);
    }
  }

  update_val_ = x.updateValue();
  if (update_val_)
    this->val() += x.val();

  return *this;
}

template <typename T, typename Storage>
KOKKOS_INLINE_FUNCTION
Sacado::Fad::GeneralFad<T,Storage>&
Sacado::Fad::GeneralFad<T,Storage>::
operator -= (const Sacado::Fad::GeneralFad<T,Storage>& x)
{
  const int xsz = x.size(), sz = this->size();

#if defined(SACADO_DEBUG) && !defined(__CUDA_ARCH__ )
  if ((xsz != sz) && (xsz != 0) && (sz != 0))
    throw "Fad Error:  Attempt to assign with incompatible sizes";
#endif

  if (xsz) {
    if (sz) {
      for(int i=0; i<sz; ++i)
        this->fastAccessDx(i) -= x.fastAccessDx(i);
    }
    else {
      this->resizeAndZero(xsz);
      for(int i=0; i<xsz; ++i)
        this->fastAccessDx(i) = -x.fastAccessDx(i);
    }
  }

  update_val_ = x.updateValue();
  if (update_val_)
    this->val() -= x.val();


  return *this;
}

template <typename T, typename Storage>
KOKKOS_INLINE_FUNCTION
Sacado::Fad::GeneralFad<T,Storage>&
Sacado::Fad::GeneralFad<T,Storage>::
operator *= (const Sacado::Fad::GeneralFad<T,Storage>& x)
{
  const int xsz = x.size(), sz = this->size();
  T xval = x.val();
  T v = this->val();

#if defined(SACADO_DEBUG) && !defined(__CUDA_ARCH__ )
  if ((xsz != sz) && (xsz != 0) && (sz != 0))
    throw "Fad Error:  Attempt to assign with incompatible sizes";
#endif

  if (xsz) {
    if (sz) {
      for(int i=0; i<sz; ++i)
        this->fastAccessDx(i) = v*x.fastAccessDx(i) + this->fastAccessDx(i)*xval;
    }
    else {
      this->resizeAndZero(xsz);
      for(int i=0; i<xsz; ++i)
        this->fastAccessDx(i) = v*x.fastAccessDx(i);
    }
  }
  else {
    if (sz) {
      for (int i=0; i<sz; ++i)
        this->fastAccessDx(i) *= xval;
    }
  }

  update_val_ = x.updateValue();
  if (update_val_)
    this->val() *= xval;

  return *this;
}

template <typename T, typename Storage>
KOKKOS_INLINE_FUNCTION
Sacado::Fad::GeneralFad<T,Storage>&
Sacado::Fad::GeneralFad<T,Storage>::
operator /= (const Sacado::Fad::GeneralFad<T,Storage>& x)
{
  const int xsz = x.size(), sz = this->size();
  T xval = x.val();
  T v = this->val();

#if defined(SACADO_DEBUG) && !defined(__CUDA_ARCH__ )
  if ((xsz != sz) && (xsz != 0) && (sz != 0))
    throw "Fad Error:  Attempt to assign with incompatible sizes";
#endif

  if (xsz) {
    if (sz) {
      for(int i=0; i<sz; ++i)
        this->fastAccessDx(i) =
          ( this->fastAccessDx(i)*xval - v*x.fastAccessDx(i) )/ (xval*xval);
    }
    else {
      this->resizeAndZero(xsz);
      for(int i=0; i<xsz; ++i)
        this->fastAccessDx(i) = - v*x.fastAccessDx(i) / (xval*xval);
    }
  }
  else {
    if (sz) {
      for (int i=0; i<sz; ++i)
        this->fastAccessDx(i) /= xval;
    }
  }

  update_val_ = x.updateValue();
  if (update_val_)
    this->val() /= xval;

  return *this;
}

template <typename T, typename Storage>
template <typename S>
KOKKOS_INLINE_FUNCTION
SACADO_GENERALFAD_ENABLE_FUNC
Sacado::Fad::GeneralFad<T,Storage>::
operator += (const Sacado::Fad::Expr<S>& x)
{
  const int xsz = x.size(), sz = this->size();

#if defined(SACADO_DEBUG) && !defined(__CUDA_ARCH__ )
  if ((xsz != sz) && (xsz != 0) && (sz != 0))
    throw "Fad Error:  Attempt to assign with incompatible sizes";
#endif

  if (xsz) {
    if (sz) {
      if (x.hasFastAccess())
        for (int i=0; i<sz; ++i)
          this->fastAccessDx(i) += x.fastAccessDx(i);
      else
        for (int i=0; i<sz; ++i)
          this->fastAccessDx(i) += x.dx(i);
    }
    else {
      this->resizeAndZero(xsz);
      if (x.hasFastAccess())
        for (int i=0; i<xsz; ++i)
          this->fastAccessDx(i) = x.fastAccessDx(i);
      else
        for (int i=0; i<xsz; ++i)
          this->fastAccessDx(i) = x.dx(i);
    }
  }

  update_val_ = x.updateValue();
  if (update_val_)
    this->val() += x.val();

  return *this;
}

template <typename T, typename Storage>
template <typename S>
KOKKOS_INLINE_FUNCTION
SACADO_GENERALFAD_ENABLE_FUNC
Sacado::Fad::GeneralFad<T,Storage>::
operator -= (const Sacado::Fad::Expr<S>& x)
{
  const int xsz = x.size(), sz = this->size();

#if defined(SACADO_DEBUG) && !defined(__CUDA_ARCH__ )
  if ((xsz != sz) && (xsz != 0) && (sz != 0))
    throw "Fad Error:  Attempt to assign with incompatible sizes";
#endif

  if (xsz) {
    if (sz) {
      if (x.hasFastAccess())
        for(int i=0; i<sz; ++i)
          this->fastAccessDx(i) -= x.fastAccessDx(i);
      else
        for (int i=0; i<sz; ++i)
          this->fastAccessDx(i) -= x.dx(i);
    }
    else {
      this->resizeAndZero(xsz);
      if (x.hasFastAccess())
        for(int i=0; i<xsz; ++i)
          this->fastAccessDx(i) = -x.fastAccessDx(i);
      else
        for (int i=0; i<xsz; ++i)
          this->fastAccessDx(i) = -x.dx(i);
    }
  }

  update_val_ = x.updateValue();
  if (update_val_)
    this->val() -= x.val();


  return *this;
}

template <typename T, typename Storage>
template <typename S>
KOKKOS_INLINE_FUNCTION
SACADO_GENERALFAD_ENABLE_FUNC
Sacado::Fad::GeneralFad<T,Storage>::
operator *= (const Sacado::Fad::Expr<S>& x)
{
  const int xsz = x.size(), sz = this->size();
  T xval = x.val();
  T v = this->val();

#if defined(SACADO_DEBUG) && !defined(__CUDA_ARCH__ )
  if ((xsz != sz) && (xsz != 0) && (sz != 0))
    throw "Fad Error:  Attempt to assign with incompatible sizes";
#endif

  if (xsz) {
    if (sz) {
      if (x.hasFastAccess())
        for(int i=0; i<sz; ++i)
          this->fastAccessDx(i) = v*x.fastAccessDx(i) + this->fastAccessDx(i)*xval;
      else
        for (int i=0; i<sz; ++i)
          this->fastAccessDx(i) = v*x.dx(i) + this->fastAccessDx(i)*xval;
    }
    else {
      this->resizeAndZero(xsz);
      if (x.hasFastAccess())
        for(int i=0; i<xsz; ++i)
          this->fastAccessDx(i) = v*x.fastAccessDx(i);
      else
        for (int i=0; i<xsz; ++i)
          this->fastAccessDx(i) = v*x.dx(i);
    }
  }
  else {
    if (sz) {
      for (int i=0; i<sz; ++i)
        this->fastAccessDx(i) *= xval;
    }
  }

  update_val_ = x.updateValue();
  if (update_val_)
    this->val() *= xval;

  return *this;
}

template <typename T, typename Storage>
template <typename S>
KOKKOS_INLINE_FUNCTION
SACADO_GENERALFAD_ENABLE_FUNC
Sacado::Fad::GeneralFad<T,Storage>::
operator /= (const Sacado::Fad::Expr<S>& x)
{
  const int xsz = x.size(), sz = this->size();
  T xval = x.val();
  T v = this->val();

#if defined(SACADO_DEBUG) && !defined(__CUDA_ARCH__ )
  if ((xsz != sz) && (xsz != 0) && (sz != 0))
    throw "Fad Error:  Attempt to assign with incompatible sizes";
#endif

  if (xsz) {
    if (sz) {
      if (x.hasFastAccess())
        for(int i=0; i<sz; ++i)
          this->fastAccessDx(i) = ( this->fastAccessDx(i)*xval - v*x.fastAccessDx(i) )/ (xval*xval);
      else
        for (int i=0; i<sz; ++i)
          this->fastAccessDx(i) = ( this->fastAccessDx(i)*xval - v*x.dx(i) )/ (xval*xval);
    }
    else {
      this->resizeAndZero(xsz);
      if (x.hasFastAccess())
        for(int i=0; i<xsz; ++i)
          this->fastAccessDx(i) = - v*x.fastAccessDx(i) / (xval*xval);
      else
        for (int i=0; i<xsz; ++i)
          this->fastAccessDx(i) = -v*x.dx(i) / (xval*xval);
    }
  }
  else {
    if (sz) {
      for (int i=0; i<sz; ++i)
        this->fastAccessDx(i) /= xval;
    }
  }

  update_val_ = x.updateValue();
  if (update_val_)
    this->val() /= xval;

  return *this;
}

#undef SACADO_GENERALFAD_ENABLE_FUNC
