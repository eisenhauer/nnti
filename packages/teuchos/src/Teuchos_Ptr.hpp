// @HEADER
// ***********************************************************************
// 
//                    Teuchos: Common Tools Package
//                 Copyright (2004) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER


#ifndef TEUCHOS_PTR_HPP
#define TEUCHOS_PTR_HPP


#include "Teuchos_ENull.hpp"
#include "Teuchos_TypeNameTraits.hpp"


namespace Teuchos {


/** \brief Simple wrapper class for raw pointers to single objects where no
 * persisting relationship exists.
 *
 * This class is meant to replace all but the lowest-level use of raw pointers
 * that point to single objects where the use of <tt>RCP</tt> is not justified
 * for performance or semantic reasons.  When built in optimized mode, this
 * class should impart little extra space or time overhead and should be
 * exactly equivalent to a raw C++ pointer except for default initalization to
 * NULL.
 *
 * The main advantages of using this class over a raw pointer however are:
 *
 * <ul>
 *
 * <li> <tt>Ptr</tt> objects always default construct to null
 *
 * <li> <tt>Ptr</tt> objects will throw exceptions on attempts to dereference
 * the underlying null pointer when debugging support is compiled in.
 *
 * <li> <tt>Ptr</tt> does not allow array-like operations like
 * <tt>ptr[i]</tt>, <tt>++ptr</tt> or <tt>ptr+i</tt> that can only result in
 * disaster when the a pointer points to only a single object that can not be
 * assumed to be part of an array of objects.
 *
 * <li> <tt>Ptr</tt> is part of a system of types defined in <tt>Teuchos</tt>
 * that keeps your code away from raw pointers which are the cause of most
 * defects in C++ code.
 *
 * </ul>
 *
 * Debugging support is compiled in when the macro <tt>TEUCHOS_DEBUG</tt> is
 * defined which happens automatically when <tt>--enable-teuchos-debug</tt> is
 * specified on the configure line.  When debugging support is not compiled
 * in, the only overhead imparted by this class is it's default initialization
 * to null.  Therefore, this class can provide for very high performance on
 * optimized builds of the code.
 *
 * An implicit conversion from a raw pointer to a <tt>Ptr</tt> object is okay
 * since we don't assume any ownership of the object, hense the constructor
 * taking a raw pointer is not declared explicit.  However, this class does
 * not support an implicit conversion to a raw pointer since we want to limit
 * the exposure of raw pointers in our software.  If we have to convert back
 * to a raw pointer, then we want to make that explicit by calling
 * <tt>get()</tt>.
 *
 * This class should be used to replace most raw uses of C++ pointers to
 * single objects where using the <tt>RCP</tt> class is not appropriate,
 * unless the runtime cost of null-initialization it too expensive.
 */
template<class T>
class Ptr {
public:

  /** \brief Default construct to NULL.
	 *
	 * <b>Postconditons:</b><ul>
	 * <li> <tt>this->get() == NULL</tt>
	 * </ul>
   */
  Ptr( ENull null_in = null );

  /** \brief Construct given a raw pointer.
	 *
	 * <b>Postconditons:</b><ul>
	 * <li> <tt>this->get() == ptr</tt>
	 * </ul>
   *
   * Note: This constructor is <em>not</em> declared <tt>explicit</tt> so it
   * defines an implicit conversion from a raw C++ pointer to a <tt>Ptr</tt>
   * object.  This is okay in this context since there is not ownership issues
   * to consider and it makes it easier to update code.
   */
  Ptr( T *ptr );

  /** \brief Copy construct from same type.
	 *
	 * <b>Postconditons:</b><ul>
	 * <li> <tt>this->get() == ptr.get()</tt>
	 * </ul>
   */
	Ptr(const Ptr<T>& ptr);

  /** \brief Copy construct from another type.
	 *
	 * <b>Postconditons:</b><ul>
	 * <li> <tt>this->get() == ptr.get()</tt> (unless virtual base classes
   *      are involved)
	 * </ul>
   */
	template<class T2>
	Ptr(const Ptr<T2>& ptr);

	/** \brief Shallow copy of the underlying pointer.
	 *
	 * <b>Postconditons:</b><ul>
	 * <li> <tt>this->get() == ptr.get()</tt>
	 * </ul>
	 */
	Ptr<T>& operator=(const Ptr<T>& ptr);

	/** \brief Pointer (<tt>-></tt>) access to members of underlying object.
	 *
	 * <b>Preconditions:</b><ul>
	 * <li> <tt>this->get() != NULL</tt> (throws <tt>std::logic_error</tt>)
	 * </ul>
	 */
	T* operator->() const;

	/** \brief Dereference the underlying object.
	 *
	 * <b>Preconditions:</b><ul>
	 * <li> <tt>this->get() != NULL</tt> (throws <tt>std::logic_error</tt>)
	 * </ul>
	 */
	T& operator*() const;

  /** \brief Get the raw C++ pointer to the underlying object. */
	T* get() const;

	/** \brief Throws <tt>std::logic_error</tt> if <tt>this->get()==NULL</tt>,
   * otherwise returns reference to <tt>*this</tt>.
   */
	const Ptr<T>& assert_not_null() const;

private:

  T *ptr_;

};


/** \brief create a non-persisting output argument for a function call.
 *
 * \relates Ptr
 */
template<typename T> inline
Ptr<T> outArg( T& arg )
{
  return Ptr<T>(&arg);
}


/** \brief Traits specialization for Ptr.
 *
 * \ingroup teuchos_mem_mng_grp
 */
template<typename T>
class TypeNameTraits<Ptr<T> > {
public:
  static std::string name() { return "Ptr<"+TypeNameTraits<T>::name()+">"; }
};


} // namespace Teuchos


// /////////////////////////////////////////////////////////////////////////
// Inline implementations below, not for the client to look at.


namespace Teuchos {
namespace PtrPrivateUtilityPack {
void throw_null( const std::string &type_name );
} // namespace PtrPrivateUtilityPack
} // namespace Teuchos


namespace Teuchos {


template<class T> inline
Ptr<T>::Ptr( ENull null_in )
  : ptr_(0)
{}


template<class T> inline
Ptr<T>::Ptr( T *ptr )
  : ptr_(ptr)
{}


template<class T> inline
Ptr<T>::Ptr(const Ptr<T>& ptr)
  :ptr_(ptr.ptr_)
{}


template<class T>
template<class T2> inline
Ptr<T>::Ptr(const Ptr<T2>& ptr)
  :ptr_(ptr.ptr_)
{}


template<class T> inline
Ptr<T>& Ptr<T>::operator=(const Ptr<T>& ptr)
{
  ptr_ = ptr.get();
}


template<class T> inline
T* Ptr<T>::operator->() const
{
#ifdef TEUCHOS_DEBUG
  assert_not_null();
#endif
  return ptr_;
}


template<class T> inline
T& Ptr<T>::operator*() const
{
#ifdef TEUCHOS_DEBUG
  assert_not_null();
#endif
  return *ptr_;
}


template<class T> inline
T* Ptr<T>::get() const
{
  return ptr_;
}


template<class T>
const Ptr<T>& Ptr<T>::assert_not_null() const
{
  if(!ptr_)
    PtrPrivateUtilityPack::throw_null(TypeNameTraits<T>::name());
  return *this;
}


} // namespace Teuchos


#endif // TEUCHOS_PTR_HPP
