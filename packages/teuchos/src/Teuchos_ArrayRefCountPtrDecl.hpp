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

#ifndef TEUCHOS_ARRAY_REFCOUNTPTR_DECL_HPP
#define TEUCHOS_ARRAY_REFCOUNTPTR_DECL_HPP

/*! \file Teuchos_ArrayRefCountPtrDecl.hpp
    \brief Reference-counted array pointer class and non-member templated function
	definitions
*/

#include "Teuchos_RefCountPtr.hpp"

namespace Teuchos {

/** \brief Array reference-counted pointer class.
 *
 * This is a reference-counted class similar to <tt>RefCountPtr</tt> except
 * that it is designed to use reference counting to manage an array of objects
 * that use value semantics.  Managing an array of objects is very different
 * from managing a pointer to an individual, possibly polymophic, object.  For
 * example, while implicit conversions from derived to base types is a good
 * thing when dealing with pointers to single objects, it is a very bad thing
 * when working with arrays of objects.  Therefore, this class contains those
 * capabilities of raw pointers that are good dealing with arrays of objects
 * but excludes those that are bad, such as implicit conversions from derived
 * to base types.
 *
 * Note that all access will be checked at runtime to avoid reading invalid
 * memory if <tt>HAVE_TEUCHOS_ARRAY_BOUNDSCHECK</tt> is defined which it is if
 * <tt>--enable-teuchos-abc</tt> is given to the <tt>configure</tt> script.
 * In order to be able to check access, every <tt>%ArrayRefCountPtr</tt> must
 * be constructed given a range.  When <tt>HAVE_TEUCHOS_ARRAY_BOUNDSCHECK</tt>
 * is defined, this class simply does not give up a raw pointer or raw
 * reference to any internally referenced object if that object does not fall
 * with the range of valid data.
 *
 * ToDo: Finish documentation!
 */
template<class T>
class ArrayRefCountPtr {
public:

  /** \name Public types */
  //@{

	/** \brief . */
	typedef T	element_type;
  /** \brief. */
  typedef Teuchos_Index Ordinal;
#ifdef HAVE_TEUCHOS_ARRAY_BOUNDSCHECK
	/** \brief . */
	typedef ArrayRefCountPtr<T> iterator;
#else
	typedef T* iterator;
#endif
#ifdef HAVE_TEUCHOS_ARRAY_BOUNDSCHECK
/** \brief . */
	typedef ArrayRefCountPtr<T> const_iterator;
#else
	typedef T* const_iterator;
#endif

  //@}

  /** \name Constructors/Initializers */
  //@{

	/** \brief Initialize <tt>ArrayRefCountPtr<T></tt> to NULL.
	 *
	 * This allows clients to write code like:
	 \code
	 ArrayRefCountPtr<int> p = null;
	 \endcode
	 * or
	 \code
	 ArrayRefCountPtr<int> p;
	 \endcode
	 * and construct to <tt>NULL</tt>
	 */
	ArrayRefCountPtr( ENull null_arg = null );

	/** \brief Initialize from another <tt>ArrayRefCountPtr<T></tt> object.
	 *
	 * After construction, <tt>this</tt> and <tt>r_ptr</tt> will
	 * reference the same array.
	 *
	 * This form of the copy constructor is required even though the
	 * below more general templated version is sufficient since some
	 * compilers will generate this function automatically which will
	 * give an incorrect implementation.
	 *
	 * Postconditons:<ul>
	 * <li><tt>this->get() == r_ptr.get()</tt>
	 * <li><tt>this->count() == r_ptr.count()</tt>
	 * <li><tt>this->has_ownership() == r_ptr.has_ownership()</tt>
	 * <li> If <tt>r_ptr.get() != NULL</tt> then <tt>r_ptr.count()</tt> is incremented by 1
	 * </ul>
	 */
	ArrayRefCountPtr(const ArrayRefCountPtr<T>& r_ptr);

	/** \brief Removes a reference to a dynamically allocated array and possibly deletes
	 * the array if owned.
	 *
	 * Deallocates array if <tt>this->has_ownership() == true</tt> and
	 * <tt>this->count() == 1</tt>.  If <tt>this->count() == 1</tt> but
	 * <tt>this->has_ownership() == false</tt> then the array is not deleted
	 * (usually using <tt>delete []</tt>).  If <tt>this->count() > 1</tt> then
	 * the internal reference count shared by all the other related
	 * <tt>ArrayRefCountPtr<...></tt> objects for this shared array is
	 * deincremented by one.  If <tt>this->get() == NULL</tt> then nothing
	 * happens.
	 */
	~ArrayRefCountPtr();

	/** \brief Copy the pointer to the referenced array and increment the
	 * reference count.
	 *
	 * If <tt>this->has_ownership() == true</tt> and <tt>this->count() == 1</tt>
	 * before this operation is called, then the array will be deleted prior to
	 * binding to the pointer (possibly <tt>NULL</tt>) pointed to in
	 * <tt>r_ptr</tt>.  Assignment to self (i.e. <tt>this->get() ==
	 * r_ptr.get()</tt>) is harmless and this function does nothing.
	 *
	 * Postconditons:
	 * <ul>
	 * <li><tt>this->get() == r_ptr.get()</tt>
	 * <li><tt>this->count() == r_ptr.count()</tt>
	 * <li><tt>this->has_ownership() == r_ptr.has_ownership()</tt>
	 * <li> If <tt>r_ptr.get() != NULL</tt> then <tt>r_ptr.count()</tt> is incremented by 1
	 * </ul>
	 */
	ArrayRefCountPtr<T>& operator=(const ArrayRefCountPtr<T>& r_ptr);

  //@}

  /** \name Object/Pointer Access Functions */
  //@{

	/** \brief Pointer (<tt>-></tt>) access to members of underlying object for
	 * current position.
	 *
	 * <b>Preconditions:</b><ul>
	 * <li><tt>this->get() != NULL</tt>
   * <li><tt>this->lowerOffset() <= 0</tt>
   * <li><tt>this->upperOffset() >= 0</tt>
	 * </ul>
	 */
	T* operator->() const;

	/** \brief Dereference the underlying object for the current pointer
	 * position.
	 *
	 * <b>Preconditions:</b><ul>
	 * <li><tt>this->get() != NULL</tt>
   * <li><tt>this->lowerOffset() <= 0</tt>
   * <li><tt>this->upperOffset() >= 0</tt>
	 * </ul>
	 */
	T& operator*() const;

  /** \brief Get the raw C++ pointer to the underlying object.
   *
	 * <b>Preconditions:</b><ul>
   * <li>[<tt>*this != null</tt>] <tt>this->lowerOffset() <= 0</tt>
   * <li>[<tt>*this != null</tt>] <tt>this->upperOffset() >= 0</tt>
	 * </ul>
	 */
	T* get() const;

	/** \brief Random object access.
	 *
	 * <b>Preconditions:</b><ul>
	 * <li><tt>this->get() != NULL</tt>
   * <li><tt>this->lowerOffset() <= offset && offset <= this->upperOffset()</tt>
	 * </ul>
   */
	T& operator[](Ordinal offset) const;

  //@}

  /** \name Pointer Arithmetic Functions */
  //@{

	/** \brief Prefix increment of pointer (i.e. ++ptr).
   *
   * Does nothing if <tt>this->get() == NULL</tt>.
	 *
	 * <b>Postconditions:</b><ul>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->get()</tt> is incremented by <tt>1</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->lowerOffset()</tt> is deincremented by <tt>1</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->upperOffset()</tt> is deincremented by <tt>1</tt>
	 * </ul>
   */
	ArrayRefCountPtr<T>& operator++();

	/** \brief Postfix increment of pointer (i.e. ptr++).
   *
   * Does nothing if <tt>this->get() == NULL</tt>.
	 *
	 * <b>Postconditions:</b><ul>
   * <li><tt>this->get()</tt> is incremented by <tt>1</tt>
   * <li><tt>this->lowerOffset()</tt> is deincremented by <tt>1</tt>
   * <li><tt>this->upperOffset()</tt> is deincremented by <tt>1</tt>
	 * </ul>
   */
	ArrayRefCountPtr<T> operator++(int);

	/** \brief Prefix deincrement of pointer (i.e. --ptr).
   *
   * Does nothing if <tt>this->get() == NULL</tt>.
	 *
	 * <b>Postconditions:</b><ul>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->get()</tt> is deincremented by <tt>1</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->lowerOffset()</tt> is incremented by <tt>1</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->upperOffset()</tt> is incremented by <tt>1</tt>
	 * </ul>
   */
	ArrayRefCountPtr<T>& operator--();

	/** \brief Postfix deincrement of pointer (i.e. ptr--).
   *
   * Does nothing if <tt>this->get() == NULL</tt>.
	 *
	 * <b>Postconditions:</b><ul>
   * <li><tt>this->get()</tt> is dincremented by <tt>1</tt>
   * <li><tt>this->lowerOffset()</tt> is incremented by <tt>1</tt>
   * <li><tt>this->upperOffset()</tt> is incremented by <tt>1</tt>
	 * </ul>
   */
	ArrayRefCountPtr<T> operator--(int);

	/** \brief Pointer integer increment (i.e. ptr+=offset).
   *
   * Does nothing if <tt>this->get() == NULL</tt>.
	 *
	 * <b>Postconditions:</b><ul>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->get()</tt> is incremented by <tt>offset</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->lowerOffset()</tt> is deincremented by <tt>offset</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->upperOffset()</tt> is deincremented by <tt>offset</tt>
	 * </ul>
   */
	ArrayRefCountPtr<T>& operator+=(Ordinal offset);

	/** \brief Pointer integer increment (i.e. ptr-=offset).
   *
   * Does nothing if <tt>this->get() == NULL</tt>.
	 *
	 * <b>Postconditions:</b><ul>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->get()</tt> is deincremented by <tt>offset</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->lowerOffset()</tt> is incremented by <tt>offset</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>this->upperOffset()</tt> is incremented by <tt>offset</tt>
	 * </ul>
   */
	ArrayRefCountPtr<T>& operator-=(Ordinal offset);

	/** \brief Pointer integer increment (i.e. ptr+offset).
   *
   * Returns a null pointer if <tt>this->get() == NULL</tt>.
	 *
	 * <b>Postconditions:</b><ul>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>return->get() == this->get() + offset</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>return->lowerOffset() == this->lowerOffset() - offset</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>return->upperOffset() == this->upperOffset() - offset</tt>
	 * </ul>
   *
   * Note that since implicit conversion of <tt>ArrayRefCountPtr<T></tt>
   * objects is not allowed that it does not help at all to make this function
   * into a non-member function.
   */
	ArrayRefCountPtr<T> operator+(Ordinal offset) const;

	/** \brief Pointer integer deincrement (i.e. ptr-offset).
   *
   * Returns a null pointer if <tt>this->get() == NULL</tt>.
	 *
	 * <b>Postconditions:</b><ul>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>return->get() == this->get() - offset</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>return->lowerOffset() == this->lowerOffset() + offset</tt>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>return->upperOffset() == this->upperOffset() + offset</tt>
	 * </ul>
   *
   * Note that since implicit conversion of <tt>ArrayRefCountPtr<T></tt>
   * objects is not allowed that it does not help at all to make this function
   * into a non-member function.
   */
	ArrayRefCountPtr<T> operator-(Ordinal offset) const;

  //@}

  /** \name Views */
  //@{

	/** \brief Return object for only const access to data.
   *
   * This function should compile only sucessfully if the type <tt>T</tt> is
   * not already declared <tt>const</tt>!
   */
	ArrayRefCountPtr<const T> getConst() const;

	/** \brief Return a view of a contiguous range of elements.
	 *
	 * <b>Preconditions:</b><ul>
	 * <li><tt>this->get() != NULL</tt>
   * <li><tt>this->lowerOffset() <= lowerOffset</tt>
   * <li><tt>lowerOffset + size - 1 <= this->upperOffset()</tt>
	 * </ul>
	 *
	 * <b>Postconditions:</b><ul>
   * <li><tt>return->get() == this->get() + lowerOffset</tt>
   * <li><tt>return->lowerOffset() == 0</tt>
   * <li><tt>return->upperOffset() == size-1</tt>
	 * </ul>
   */
	ArrayRefCountPtr<T> subview( Ordinal lowerOffset, Ordinal size ) const;

  //@}

  /** \name General query functions */
  //@{

	/** \brief Return the number of <tt>ArrayRefCountPtr<></tt> objects that have a reference
	 * to the underlying pointer that is being shared.
	 *
	 * @return  If <tt>this->get() == NULL</tt> then this function returns 0.
	 * Otherwise, this function returns <tt>> 0</tt>.
	 */
	int count() const;

	/** \brief Returns true if the smart pointers share the same underlying reference-counted object.
	 *
	 * This method does more than just check if <tt>this->get() == r_ptr.get()</tt>.
	 * It also checks to see if the underlying reference counting machinary is the
	 * same.
	 */
	template<class T2>
	bool shares_resource(const ArrayRefCountPtr<T2>& r_ptr) const;

  /** \brief Return the lower offset to valid data. */
  Ordinal lowerOffset() const;

  /** \brief Return the upper offset to valid data. */
  Ordinal upperOffset() const;

  /** \brief The total number of items in the managed array
   * (i.e. <tt>upperOffset()-lowerOffset()+1</tt>).
   */
  Ordinal dim() const;

  //@}

  /** \name Standard Container-Like Functions */
  //@{

  /** \brief Return an iterator to beginning of the array of data.
   *
   * If <tt>HAVE_TEUCHOS_ARRAY_BOUNDSCHECK</tt> is defined then the iterator
   * returned is an <tt>ArrayRefCountPtr<T></tt> object and all operations are
   * checked at runtime.  When <tt>HAVE_TEUCHOS_ARRAY_BOUNDSCHECK</tt> is not
   * defined, the a raw pointer <tt>T*</tt> is returned for fast execution.
   *
   * <b>Postconditions:</b><ul>
   * <li>[this->get()!=NULL</tt>] <tt>&*return == this->get()</tt>
   * <li>[<tt>this->get()==NULL</tt>] <tt>return == (null or NULL)</tt>
   * </ul>
   */
  const_iterator begin() const;

  /** \brief Return an iterator to past the end of the array of data.
   *
   * If <tt>HAVE_TEUCHOS_ARRAY_BOUNDSCHECK</tt> is defined then the iterator
   * returned is an <tt>ArrayRefCountPtr<T></tt> object and all operations are
   * checked at runtime.  When <tt>HAVE_TEUCHOS_ARRAY_BOUNDSCHECK</tt> is not
   * defined, the a raw pointer <tt>T*</tt> is returned for fast execution.
   *
   * <b>Postconditions:</b><ul>
   * <li>[<tt>this->get()!=NULL</tt>] <tt>&*end == this->get()+(this->upperOffset()+1)</tt>
   * <li>[<tt>this->get()==NULL</tt>] <tt>return == (null or NULL)</tt>
   * </ul>
   */
  const_iterator end() const;

  //@}

  /** \name Ownership */
  //@{

	/** \brief Release the ownership of the underlying array.
	 *
	 * After this function is called then the client is responsible for deleting
	 * the returned pointer no matter how many <tt>ref_count_prt<T></tt> objects
	 * have a reference to it.  If <tt>this-></tt>get() <tt>== NULL</tt>, then
	 * this call is meaningless.
	 *
	 * Note that this function does not have the exact same semantics as does
	 * <tt>auto_ptr<T>::release()</tt>.  In <tt>auto_ptr<T>::release()</tt>,
	 * <tt>this</tt> is set to <tt>NULL</tt> while here in ArrayRefCountPtr<T>::
	 * release() only an ownership flag is set and <tt>this</tt> still points to
	 * the same array.  It would be difficult to duplicate the behavior of
	 * <tt>auto_ptr<T>::release()</tt> for this class.
	 *
	 * <b>Postconditions:</b><ul>
	 * <li><tt>this->has_ownership() == false</tt>
	 * </ul>
	 *
	 * @return Returns the value of <tt>this->get()</tt>
	 */
	T* release();

	/** \brief Give <tt>this</tt> and other <tt>ArrayRefCountPtr<></tt> objects
	 * ownership of the underlying referenced array to delete it.
	 *
	 * See <tt>~ArrayRefCountPtr()</tt> above.  This function does nothing if
	 * <tt>this->get() == NULL</tt>.
	 *
	 * <b>Postconditions:</b><ul>
	 * <li> If <tt>this->get() == NULL</tt> then
	 *   <ul>
	 *   <li><tt>this->has_ownership() == false</tt> (always!).
	 *   </ul>
	 * <li> else
	 *   <ul>
	 *   <li><tt>this->has_ownership() == true</tt>
	 *   </ul>
	 * </ul>
	 */
	void set_has_ownership();

	/** \brief Returns true if <tt>this</tt> has ownership of object pointed to
	 * by <tt>this->get()</tt> in order to delete it.
	 *
	 * See <tt>~ArrayRefCountPtr()</tt> above.
	 *
	 * \return If this->get() <tt>== NULL</tt> then this function always returns
	 * <tt>false</tt>.  Otherwise the value returned from this function depends
	 * on which function was called most recently, if any;
	 * <tt>set_has_ownership()</tt> (<tt>true</tt>) or <tt>release()</tt>
	 * (<tt>false</tt>).
	 */
	bool has_ownership() const;

  //@}

  /** \name Assertion Functions. */
  //@{

	/** \brief Throws <tt>std::logic_error</tt> if <tt>this->get()==NULL</tt>,
   * otherwise returns reference to <tt>*this</tt>.
   */
	const ArrayRefCountPtr<T>& assert_not_null() const;

	/** \brief Throws <tt>std::logic_error</tt> if <tt>this->get()==NULL</tt>
   * or<tt>this->get()!=NULL && (lowerOffset < this->lowerOffset() ||
   * this->upperOffset() < upperOffset</tt>, otherwise returns reference to
   * <tt>*this</tt>
   */
	const ArrayRefCountPtr<T>& assert_in_range( Ordinal lowerOffset, Ordinal size ) const;

  //@}

public: // Bad bad bad

	// //////////////////////////////////////
	// Private types

	typedef PrivateUtilityPack::RefCountPtr_node  node_t;

private:

	// //////////////////////////////////////////////////////////////
	// Private data members

	T       *ptr_;  // NULL if this pointer is null
	node_t	*node_;	// NULL if this pointer is null
  Ordinal lowerOffset_;
  Ordinal upperOffset_;

public:
#ifndef DOXYGEN_COMPILE
	// These constructors should be private but I have not had good luck making
	// this portable (i.e. using friendship etc.) in the past
	ArrayRefCountPtr( T* p, Ordinal lowerOffset, Ordinal upperOffset, bool has_ownership );
	template<class Dealloc_T>
	ArrayRefCountPtr( T* p, Ordinal lowerOffset, Ordinal upperOffset, Dealloc_T dealloc, bool has_ownership );
	// This is a very bad breach of encapsulation that is needed since MS VC++ 5.0 will
	// not allow me to declare template functions as friends.
	ArrayRefCountPtr( T* p, Ordinal lowerOffset, Ordinal upperOffset, node_t* node);
	T*&           access_ptr();
	T*            access_ptr() const; // No preconditions
	node_t*&      access_node();
	node_t*       access_node() const;
#endif

};	// end class ArrayRefCountPtr<...>

/** \brief Traits specialization.
 *
 * \relates RefCountPtr
 */
template<typename T>
class TypeNameTraits<ArrayRefCountPtr<T> > {
public:
  static std::string name() { return "ArrayRefCountPtr<"+TypeNameTraits<T>::name()+">"; }
};

/** \brief Wrapps a preallocated array of data with the assumption to call the
 * array version of delete.
 *
 * \relates ArrayRefCountPtr
 */
template<class T>
ArrayRefCountPtr<T> arcp(
  T* p, typename ArrayRefCountPtr<T>::Ordinal lowerOffset
  ,typename ArrayRefCountPtr<T>::Ordinal size
  , bool owns_mem = true
  );

/** \brief Wrapps a preallocated array of data and uses a templated
 * deallocation strategy object to define deletion .
 *
 * \relates ArrayRefCountPtr
 */
template<class T, class Dealloc_T>
ArrayRefCountPtr<T> arcp(
  T* p, typename ArrayRefCountPtr<T>::Ordinal lowerOffset
  ,typename ArrayRefCountPtr<T>::Ordinal size
  , Dealloc_T dealloc, bool owns_mem
  );
 
/** \brief Allocate a new array just given a dimension.
 *
 *
 * <b>Warning!</b> The memory is allocated using <tt>new T[dim]</tt> and is
 * *not* initialized (unless there is a default constructor for a user-defined
 * type).
 *
 * \relates ArrayRefCountPtr
 */
template<class T>
ArrayRefCountPtr<T> arcp( typename ArrayRefCountPtr<T>::Ordinal dim );

/** \brief Wrap an <tt>std::vector<T></tt> object as an
 * <tt>ArrayRefCountPtr<T></tt> object.
 */
template<class T>
ArrayRefCountPtr<T> arcp( const RefCountPtr<std::vector<T> > &v );

/** \brief Get an <tt>std::vector<T></tt> object out of an
 * <tt>ArrayRefCountPtr<T></tt> object that was created using the
 * <tt>arcp()</tt> above to wrap the vector in the first place..
 */
template<class T>
RefCountPtr<std::vector<T> > get_std_vector( const ArrayRefCountPtr<T> &ptr );

/** \brief Wrap a <tt>const std::vector<T></tt> object as an
 * <tt>ArrayRefCountPtr<const T></tt> object.
 */
template<class T>
ArrayRefCountPtr<const T> arcp( const RefCountPtr<const std::vector<T> > &v );

/** \brief Get a <tt>const std::vector<T></tt> object out of an
 * <tt>ArrayRefCountPtr<const T></tt> object that was created using the
 * <tt>arcp()</tt> above to wrap the vector in the first place.
 */
template<class T>
RefCountPtr<const std::vector<T> > get_std_vector( const ArrayRefCountPtr<const T> &ptr );

/** \brief Returns true if <tt>p.get()==NULL</tt>.
 *
 * \relates ArrayRefCountPtr
 */
template<class T>
bool is_null( const ArrayRefCountPtr<T> &p );

/** \brief Returns true if <tt>p.get()==NULL</tt>.
 *
 * \relates ArrayRefCountPtr
 */
template<class T>
bool operator==( const ArrayRefCountPtr<T> &p, ENull );

/** \brief Returns true if <tt>p.get()!=NULL</tt>.
 *
 * \relates ArrayRefCountPtr
 */
template<class T>
bool operator!=( const ArrayRefCountPtr<T> &p, ENull );

/** \brief .
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
bool operator==( const ArrayRefCountPtr<T1> &p1, const ArrayRefCountPtr<T2> &p2 );

/** \brief .
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
bool operator!=( const ArrayRefCountPtr<T1> &p1, const ArrayRefCountPtr<T2> &p2 );

/** \brief .
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
bool operator<( const ArrayRefCountPtr<T1> &p1, const ArrayRefCountPtr<T2> &p2 );

/** \brief .
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
bool operator<=( const ArrayRefCountPtr<T1> &p1, const ArrayRefCountPtr<T2> &p2 );

/** \brief .
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
bool operator>( const ArrayRefCountPtr<T1> &p1, const ArrayRefCountPtr<T2> &p2 );

/** \brief .
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
bool operator>=( const ArrayRefCountPtr<T1> &p1, const ArrayRefCountPtr<T2> &p2 );

/** \brief Reinterpret cast of underlying <tt>ArrayRefCountPtr</tt> type from
 * <tt>T1*</tt> to <tt>T2*</tt>.
 *
 * The function will compile only if (<tt>reinterpret_cast<T2*>(p1.get());</tt>) compiles.
 *
 * \relates ArrayRefCountPtr
 */
template<class T2, class T1>
ArrayRefCountPtr<T2> arcp_reinterpret_cast(const ArrayRefCountPtr<T1>& p1);

/** \brief Set extra data associated with a <tt>ArrayRefCountPtr</tt> object.
 *
 * @param  extra_data
 *               [in] Data object that will be set (copied)
 * @param  name  [in] The name given to the extra data.  The value of
 *               <tt>name</tt> together with the data type <tt>T1</tt> of the
 *               extra data must be unique from any other such data or
 *               the other data will be overwritten.
 * @param  p     [out] On output, will be updated with the input <tt>extra_data</tt>
 * @param  destroy_when
 *               [in] Determines when <tt>extra_data</tt> will be destoryed
 *               in relation to the underlying reference-counted object.
 *               If <tt>destroy_when==PRE_DESTROY</tt> then <tt>extra_data</tt>
 *               will be deleted before the underlying reference-counted object.
 *               If <tt>destroy_when==POST_DESTROY</tt> (the default) then <tt>extra_data</tt>
 *               will be deleted after the underlying reference-counted object.
 * @param  force_unique
 *               [in] Determines if this type and name pair must be unique
 *               in which case if an object with this same type and name
 *               already exists, then an exception will be thrown.
 *               The default is <tt>true</tt> for safety.
 *
 * If there is a call to this function with the same type of extra
 * data <tt>T1</tt> and same arguments <tt>p</tt> and <tt>name</tt>
 * has already been made, then the current piece of extra data already
 * set will be overwritten with <tt>extra_data</tt>.  However, if the
 * type of the extra data <tt>T1</tt> is different, then the extra
 * data can be added and not overwrite existing extra data.  This
 * means that extra data is keyed on both the type and name.  This
 * helps to minimize the chance that clients will unexpectedly
 * overwrite data by accident.
 *
 * When the last <tt>RefcountPtr</tt> object is removed and the
 * reference-count node is deleted, then objects are deleted in the following
 * order: (1) All of the extra data that where added with
 * <tt>destroy_when==PRE_DESTROY</tt> are first, (2) then the underlying
 * reference-counted object is deleted, and (3) the rest of the extra data
 * that was added with <tt>destroy_when==PRE_DESTROY</tt> is then deleted.
 * The order in which the objects are destroyed is not guaranteed.  Therefore,
 * clients should be careful not to add extra data that has deletion
 * dependancies (instead consider using nested ArrayRefCountPtr objects as extra
 * data which will guarantee the order of deletion).
 *
 * <b>Preconditions:</b><ul>
 * <li><tt>p->get() != NULL</tt> (throws <tt>std::logic_error</tt>)
 * <li> If this function has already been called with the same template
 *      type <tt>T1</tt> for <tt>extra_data</tt> and the same string <tt>name</tt>
 *      and <tt>force_unique==true</tt>, then an <tt>std::invalid_argument</tt>
 *      exception will be thrown.
 * </ul>
 *
 * Note, this function is made a non-member function to be consistent
 * with the non-member <tt>get_extra_data()</tt> functions.
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
void set_extra_data( const T1 &extra_data, const std::string& name, ArrayRefCountPtr<T2> *p
                     ,EPrePostDestruction destroy_when
#ifndef __sun
                     = POST_DESTROY
#endif
                     ,bool force_unique
#ifndef __sun
                     = true
#endif
	);
#ifdef __sun
template<class T1, class T2>
inline void set_extra_data( const T1 &extra_data, const std::string& name, ArrayRefCountPtr<T2> *p )
{ set_extra_data( extra_data, name, p, POST_DESTROY, true ); }
template<class T1, class T2>
inline void set_extra_data( const T1 &extra_data, const std::string& name, ArrayRefCountPtr<T2> *p, EPrePostDestruction destroy_when )
{ set_extra_data( extra_data, name, p, destroy_when, true ); }
#endif

/** \brief Get a non-const reference to extra data associated with a <tt>ArrayRefCountPtr</tt> object.
 *
 * @param  p    [in] Smart pointer object that extra data is being extraced from.
 * @param  name [in] Name of the extra data.
 *
 * @return Returns a non-const reference to the extra_data object.
 *
 * <b>Preconditions:</b><ul>
 * <li><tt>p.get() != NULL</tt> (throws <tt>std::logic_error</tt>)
 * <li><tt>name</tt> and <tt>T1</tt> must have been used in a previous
 *      call to <tt>set_extra_data()</tt> (throws <tt>std::invalid_argument</tt>).
 * </ul>
 *
 * Note, this function must be a non-member function since the client
 * must manually select the first template argument.
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
T1& get_extra_data( ArrayRefCountPtr<T2>& p, const std::string& name );

/** \brief Get a const reference to extra data associated with a <tt>ArrayRefCountPtr</tt> object.
 *
 * @param  p    [in] Smart pointer object that extra data is being extraced from.
 * @param  name [in] Name of the extra data.
 *
 * @return Returns a const reference to the extra_data object.
 *
 * <b>Preconditions:</b><ul>
 * <li><tt>p.get() != NULL</tt> (throws <tt>std::logic_error</tt>)
 * <li><tt>name</tt> and <tt>T1</tt> must have been used in a previous
 *      call to <tt>set_extra_data()</tt> (throws <tt>std::invalid_argument</tt>).
 * </ul>
 *
 * Note, this function must be a non-member function since the client
 * must manually select the first template argument.
 *
 * Also note that this const version is a false sense of security
 * since a client can always copy a const <tt>ArrayRefCountPtr</tt> object
 * into a non-const object and then use the non-const version to
 * change the data.  However, its presence will help to avoid some
 * types of accidental changes to this extra data.
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
const T1& get_extra_data( const ArrayRefCountPtr<T2>& p, const std::string& name );

/** \brief Get a pointer to non-const extra data (if it exists) associated
 * with a <tt>ArrayRefCountPtr</tt> object.
 *
 * @param  p    [in] Smart pointer object that extra data is being extraced from.
 * @param  name [in] Name of the extra data.
 *
 * @return Returns a non-const pointer to the extra_data object.
 *
 * <b>Preconditions:</b><ul>
 * <li><tt>p.get() != NULL</tt> (throws <tt>std::logic_error</tt>)
 * </ul>
 *
 * <b>Postconditions:</b><ul>
 * <li> If <tt>name</tt> and <tt>T1</tt> have been used in a previous
 *      call to <tt>set_extra_data()</tt> then <tt>return !=NULL</tt>
 *      and otherwise <tt>return == NULL</tt>.
 * </ul>
 *
 * Note, this function must be a non-member function since the client
 * must manually select the first template argument.
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
T1* get_optional_extra_data( ArrayRefCountPtr<T2>& p, const std::string& name );

/** \brief Get a pointer to const extra data (if it exists) associated with a <tt>ArrayRefCountPtr</tt> object.
 *
 * @param  p    [in] Smart pointer object that extra data is being extraced from.
 * @param  name [in] Name of the extra data.
 *
 * @return Returns a const pointer to the extra_data object if it exists.
 *
 * <b>Preconditions:</b><ul>
 * <li><tt>p.get() != NULL</tt> (throws <tt>std::logic_error</tt>)
 * </ul>
 *
 * <b>Postconditions:</b><ul>
 * <li> If <tt>name</tt> and <tt>T1</tt> have been used in a previous
 *      call to <tt>set_extra_data()</tt> then <tt>return !=NULL</tt>
 *      and otherwise <tt>return == NULL</tt>.
 * </ul>
 *
 * Note, this function must be a non-member function since the client
 * must manually select the first template argument.
 *
 * Also note that this const version is a false sense of security
 * since a client can always copy a const <tt>ArrayRefCountPtr</tt> object
 * into a non-const object and then use the non-const version to
 * change the data.  However, its presence will help to avoid some
 * types of accidental changes to this extra data.
 *
 * \relates ArrayRefCountPtr
 */
template<class T1, class T2>
const T1* get_optional_extra_data( const ArrayRefCountPtr<T2>& p, const std::string& name );

/** \brief Return a non-<tt>const</tt> reference to the underlying deallocator object.
 *
 * <b>Preconditions:</b><ul>
 * <li><tt>p.get() != NULL</tt> (throws <tt>std::logic_error</tt>)
 * <li> The deallocator object type used to construct <tt>p</tt> is same as <tt>Dealloc_T</tt>
 *      (throws <tt>std::logic_error</tt>)
 * </ul>
 *
 * \relates ArrayRefCountPtr
 */
template<class Dealloc_T, class T>
Dealloc_T& get_dealloc( ArrayRefCountPtr<T>& p );

/** \brief Return a <tt>const</tt> reference to the underlying deallocator object.
 *
 * <b>Preconditions:</b><ul>
 * <li><tt>p.get() != NULL</tt> (throws <tt>std::logic_error</tt>)
 * <li> The deallocator object type used to construct <tt>p</tt> is same as <tt>Dealloc_T</tt>
 *      (throws <tt>std::logic_error</tt>)
 * </ul>
 *
 * Note that the <tt>const</tt> version of this function provides only
 * a very ineffective attempt to avoid accidental changes to the
 * deallocation object.  A client can always just create a new
 * non-<tt>const</tt> <tt>ArrayRefCountPtr<T></tt> object from any
 * <tt>const</tt> <tt>ArrayRefCountPtr<T></tt> object and then call the
 * non-<tt>const</tt> version of this function.
 *
 * \relates ArrayRefCountPtr
 */
template<class Dealloc_T, class T>
const Dealloc_T& get_dealloc( const ArrayRefCountPtr<T>& p );

/** \brief Return a pointer to the underlying non-<tt>const</tt> deallocator
 * object if it exists.
 *
 * <b>Preconditions:</b><ul>
 * <li><tt>p.get() != NULL</tt> (throws <tt>std::logic_error</tt>)
 * </ul>
 *
 * <b>Postconditions:</b><ul>
 * <li> If the deallocator object type used to construct <tt>p</tt> is same as <tt>Dealloc_T</tt>
 *      then <tt>return!=NULL</tt>, otherwise <tt>return==NULL</tt>
 * </ul>
 *
 * \relates ArrayRefCountPtr
 */
template<class Dealloc_T, class T>
Dealloc_T* get_optional_dealloc( ArrayRefCountPtr<T>& p );

/** \brief Return a pointer to the underlying <tt>const</tt> deallocator
 * object if it exists.
 *
 * <b>Preconditions:</b><ul>
 * <li><tt>p.get() != NULL</tt> (throws <tt>std::logic_error</tt>)
 * </ul>
 *
 * <b>Postconditions:</b><ul>
 * <li> If the deallocator object type used to construct <tt>p</tt> is same as <tt>Dealloc_T</tt>
 *      then <tt>return!=NULL</tt>, otherwise <tt>return==NULL</tt>
 * </ul>
 *
 * Note that the <tt>const</tt> version of this function provides only
 * a very ineffective attempt to avoid accidental changes to the
 * deallocation object.  A client can always just create a new
 * non-<tt>const</tt> <tt>ArrayRefCountPtr<T></tt> object from any
 * <tt>const</tt> <tt>ArrayRefCountPtr<T></tt> object and then call the
 * non-<tt>const</tt> version of this function.
 *
 * \relates ArrayRefCountPtr
 */
template<class Dealloc_T, class T>
const Dealloc_T* get_optional_dealloc( const ArrayRefCountPtr<T>& p );

/** \brief Output stream inserter.
 *
 * The implementation of this function just print pointer addresses and
 * therefore puts not restrictions on the data types involved.
 *
 * \relates ArrayRefCountPtr
 */
template<class T>
std::ostream& operator<<( std::ostream& out, const ArrayRefCountPtr<T>& p );

} // end namespace Teuchos

#endif	// TEUCHOS_ARRAY_REFCOUNTPTR_DECL_HPP
