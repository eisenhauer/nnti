// @HEADER
// ***********************************************************************
// 
// Moocho: Multi-functional Object-Oriented arCHitecture for Optimization
//                  Copyright (2003) Sandia Corporation
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
// Questions? Contact Roscoe A. Bartlett (rabartl@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef ALGORITHM_STATE_H
#define ALGORITHM_STATE_H

#include <limits>
#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <iosfwd>

#include "IterationPack_IterQuantity.hpp"
#include "Teuchos_RefCountPtr.hpp"

namespace IterationPack {

///
/** Abstacts a set of iteration quantities for an iterative algorithm.
  *
  * This object encapsulates a set of iteration quantity access objects.
  * The concrete types of the \c IterQuantity objects are expected to be subclasses
  * of \c IterQuantityAccess.  It is therefore up the the clients to determine
  * the concrete types of these iteration quantity access objects and to use
  * <tt>dynamic_cast<...></tt> (or static_cast<...> if you are sure) to access the
  * <tt>IterQuantityAccess<...></tt> object and therefore the iteration quantities themselves.
  * Each iteration quantity (IQ) access object must have a unique name associated with it.
  * IQ objects are given to the state object by clients through the \c set_iter_quant()
  * method at which point the IQ object will be given a unique id that will never change
  * change until the IQ object is removed using \c erase_iter_quant().  Memory management
  * is performed using the <tt>Teuchos::RefCountPtr</tt> smart reference
  * counting poiner class.
  * The id of any IQ object (\c iq_id) can be obtained from its name by calling
  * <tt>iq_id = get_iter_quant_id(iq_name)</tt>.  If an IQ object with the name \c iq_name
  * does not exist then <tt>get_iter_quant_id(iq_name)</tt> will return <tt>DOES_NOT_EXIST</tt>.
  * The IQ objects themselves can be accesed in <tt>O(log(num_iter_quant()))</tt> time using
  * <tt>iter_quant(iq_name)</tt> or in <tt>O(1)</tt> time using <tt>iter_quant(iq_id)</tt>.
  *  Therefore, the access of IQ objects using <tt>iq_id</tt> is an optimization for faster
  * access and the client should never have to lookup <tt>iq_name</tt> given <tt>iq_id</tt>.
  * The mapping only works from \c iq_name to \c iq_id, not the other way around.
  * It is guaranteed that as long as <tt>erase_iter_quant(iq_id)</tt> is not called that each
  * \c iq_id that  <tt>&iter_quant(iq_id) == &iter_quant( get_iter_quant(iq_name) )</tt> will \c true.
  * For \c iq_name, if <tt>get_iter_quant_id(iq_name) == DOES_NOT_EXIST</tt> then <tt>iter_quant(iq_name)</tt>
  * will throw the exception \c DoesNotExist.
  *
  * The \c next_iteration() operation is called by the algorithm to call
  * <tt>\ref IterQuantity::next_iteration "next_iteration()"</tt> on each of the IQ objects.
  *
  * The \c dump_iter_quant(out) operation prints out a list of all of the IQ objects of thier
  * \c iq_name, \c iq_name and concrete type.
  *
  * The default copy constructor, and assignment operator functions
  * are allowed since they have the proper semantics.
  */
class AlgorithmState {
public:

  /** @name Public types */
  //@{

  ///
  typedef size_t													iq_id_type;
  ///
  typedef Teuchos::RefCountPtr<IterQuantity>		IQ_ptr;
  ///
  enum { DOES_NOT_EXIST = INT_MAX }; // should not ever be this many insertions.

  /// Thrown if name or id does not exist
  class DoesNotExist : public std::logic_error
  {public: DoesNotExist(const std::string& what_arg) : std::logic_error(what_arg) {}};

  /// Thrown if name already exists
  class AlreadyExists : public std::logic_error
  {public: AlreadyExists(const std::string& what_arg) : std::logic_error(what_arg) {}};

  //@}

  ///
  virtual ~AlgorithmState() {}

  /** @name Constructors */
  //@{

  ///
  /** Construct with an initial guess for the number of iteration quantities.
    *
    * The iteration counter k is default constructed to zero.
    */
  explicit AlgorithmState(size_t reserve_size = 0);

  //@}

  /** @name Iteration counter */
  //@{

  ///
  void k(int k);
  ///
  int k() const;
  ///
  int incr_k();

  //@}

  /** @name Iteration quantity setup */
  //@{
  
  /// Return the number of iteration quantities.
  virtual size_t num_iter_quant() const;

  ///
  /** Inserts the iteration quantity through a RefCountPtr<...> object.
    *
    * Time = O(log(num_iter_quant)), Space = O(1).
    *
    * If an iteration quantity already exists with the name <tt>iq_name</tt> then
    * a <tt>AlreadyExists</tt> exception will be thrown.  Otherwise the function
    * will return the iq_id assigned to the inserted interation quantity.
    *
    * Preconditions: <ul>
    * <li> <tt>iq.get() != NULL</tt> (thorw <tt>std::invalid_argument</tt>)
    * <li> <tt>get_iter_quant_id(iq_name) == DOES_NOT_EXIST</tt> (throw <tt>AlreadyExists</tt>)
    * </ul>
    */
  virtual iq_id_type set_iter_quant(const std::string& iq_name, const IQ_ptr& iq);

  ///
  /** Removes the iteration quantity with name iq_name.
    *
    * Time = O(log(num_iter_quant)), Space = O(1).
    *
    * If <tt>get_iter_quant(iq_name).count() == 1</tt> then the IterQuantity object
    * pointed to will be deleted.  Subsequently, the iq_id returned from
    * <tt>set_iter_quant(...)</tt> when <tt>iq_name</tt> was set is no longer valid.
    *
    * Preconditions: <ul>
    * <li> <tt>get_iter_quant_id(iq_name) != DOES_NOT_EXIST</tt> (throw <tt>DoesNotExist</tt>)
    * </ul>
    */
  virtual void erase_iter_quant(const std::string& iq_name);

  ///
  /** Return the iteration quantity id (iq_id) for the iteration quantity.
    *
    * If an iteration quantity with the name <tt>iq_name</tt> does not exist, then
    * the value DOES_NOT_EXIST is returned.
    *
    * Time = O(log(num_iter_quant)), Space = O(1).
    */
  virtual iq_id_type get_iter_quant_id(const std::string& iq_name) const;

  ///
  /** Returns the RefCountPtr<...> for the iteration quantiy with iq_id
    *
    * If this iq_id does not correspond to a valid iteration quantity
    * object then a DoesNotExist exception will be thrown.  If iq_id
    * was returned from get_iter_quant_id(iq_name), this iq_id may become
    * invalid if a client called erase_iter_quant(iq_name) in the
    * mean time.
    *
    * Time = O(1), Space = O(1).
    */
  virtual IQ_ptr& get_iter_quant(iq_id_type iq_id);

  ///
  virtual const IQ_ptr& get_iter_quant(iq_id_type iq_id) const;

  //@}

  /** @name Iteration quantity access */
  //@{

  ///
  /** Iteration quantity encapsulation object access with via iq_name.
    *
    * Time = O(log(num_iter_quant())), Space = O(1).
    *
    * Preconditions: <ul>
    * <li> <tt>get_iter_quant_id(iq_name) != DOES_NOT_EXIST</tt> (throw <tt>DoesNotExist</tt>)
    * </ul>
    */
  virtual IterQuantity& iter_quant(const std::string& iq_name );
  ///
  virtual const IterQuantity& iter_quant(const std::string& iq_name ) const;
  ///
  /** Iteration quantity encapsulation object access via iq_id.
    *
    * Time = O(1), Space = O(1).
    *
    * If the IQ object with iq_id does not exist then a <tt>DoesNotExist</tt>
    * exception will be thrown.
    */
  virtual IterQuantity& iter_quant(iq_id_type iq_id);
  ///
  virtual const IterQuantity& iter_quant(iq_id_type iq_id) const;

  //@}

  /** @name Iteration incrementation */
  //@{

  ///
  /** iteration quantity forwarding.
    *
    */
  virtual void next_iteration(bool incr_k = true);

  //@}

  /** @name Miscellaneous */
  //@{

  ///
  /** iteration quantity information dumping.
    *
    * This function outputs a list with columns:
    *
    * iq_name		iq_id		concrete type
    *
    * for each iteration quantity object.
    */
  virtual void dump_iter_quant(std::ostream& out) const;

  //@}

private:
  // ///////////////////////////////////////////////////////////
  // Private types

  typedef std::vector<IQ_ptr>					iq_t;
  typedef std::map<std::string,iq_id_type>	iq_name_to_id_t;

  // ///////////////////////////////////////////////////////////
  // Private data members

  int k_;		// Iteration counter.

  iq_t					iq_;
  // Array of RefCountPtr objects that point to set iteration quantities.
  // The index into this array is the iq_id for an IQ object.  This array
  // is filled sequantially from the beginning using push_back(...).
  // When erase_iter_quant(...) is called the iq_[iq_id] is set to null which
  // reduces the reference count of the IQ object (possible deleing it if
  // there are no other references).  Then if the user tries to access and
  // IQ object with this abandonded iq_id, the dereferencing operator for
  // RefCountPtr<...> will throw an exception.
#ifdef DOXYGEN_COMPILE
  IterQuantity  *iteration_quantities;
#endif

  iq_name_to_id_t			iq_name_to_id_;
  // Mapping of an IQ name to its id.

  // ///////////////////////////////////////////////////////////
  // Private member functions
  
  ///
  iq_name_to_id_t::iterator find_and_assert(const std::string& iq_name);
  ///
  iq_name_to_id_t::const_iterator find_and_assert(const std::string& iq_name) const;

};	// end class AlgorithmState

// /////////////////////////////////////////////////////////////////////////////////
// Inline member definitions for AlgorithmState

inline
AlgorithmState::AlgorithmState(size_t reserve_size)
  : k_(0)
{	iq_.reserve(reserve_size); }

// iteration counter

inline
void AlgorithmState::k(int k)
{	k_ = k; }

inline
int AlgorithmState::k() const
{	return k_; }

inline
int AlgorithmState::incr_k()
{	return ++k_; }

// 

inline
size_t AlgorithmState::num_iter_quant() const {
  return iq_name_to_id_.size();
}

inline
AlgorithmState::iq_id_type AlgorithmState::get_iter_quant_id(
  const std::string& iq_name) const
{
  const iq_name_to_id_t::const_iterator itr = iq_name_to_id_.find(iq_name);
  return itr == iq_name_to_id_.end() ? DOES_NOT_EXIST : (*itr).second;
}

inline
AlgorithmState::IQ_ptr& AlgorithmState::get_iter_quant(iq_id_type iq_id) {
  return iq_.at(iq_id);
}

inline
const AlgorithmState::IQ_ptr& AlgorithmState::get_iter_quant(
  iq_id_type iq_id) const
{
  return iq_.at(iq_id);
}

inline
IterQuantity& AlgorithmState::iter_quant(const std::string& iq_name ) {
  return *iq_[(*find_and_assert(iq_name)).second];
}

inline
const IterQuantity& AlgorithmState::iter_quant(const std::string& iq_name ) const {
  return *iq_[(*find_and_assert(iq_name)).second];
}

}	// end namespace IterationPack 

#endif // ALGORITHM_STATE_H
