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

#ifndef RSQP_ALGO_CONFIG_H
#define RSQP_ALGO_CONFIG_H

#include "MoochoPack_Types.hpp"

namespace OptionsFromStreamPack {
  class OptionsFromStream;
}

namespace MoochoPack {

class NLPAlgoContainer;
class NLPAlgoInterface;

///
/** Interface for objects responsible for configuring an rSQP algorithm.
 *
 * Objects of this type configure a \c NLPAlgoContainer object with an rSQP algorithm object,
 * configuring the algo with step, state etc. objects and initailizing the algorithm before the
 * interations start.
 */
class NLPAlgoConfig {
public:

  /** @name Public types */
  //@{

  /// Thrown if NLP type is incompatible with this config
  class InvalidNLPType : public std::logic_error
  {public: InvalidNLPType(const std::string& what_arg) : std::logic_error(what_arg) {}};
  ///
  typedef Teuchos::RefCountPtr<
    const OptionsFromStreamPack::OptionsFromStream>             options_ptr_t;

  //@}

  /** @name Constructors/destructors */
  //@{

  ///
  virtual ~NLPAlgoConfig() {}

  //@}

  /** @name Set options */
  //@{

  ///
  /** Set the <tt>OptionsFromStreamPack::OptionsFromStream</tt> to extract the
   * options for the configuration from.
   *
   * @param  options  [in] Smart pointer to an <tt>OptionsFromStream</tt> object that
   *                  the solver will extract options from.  If <tt>options.get() != NULL</tt>
   *                  then this object must not be destroyed until <tt>this->set_options(other_options)</tt>
   *                  is called where <tt>other_options.get() != options.get()</tt> or \c this is
   *                  destoryed.  If <tt>options.get() == NULL</tt> then a default set of options will
   *                  be used.
   *
   * Postconditions:<ul>
   * <li> <tt>this->get_options().get() = options.get()</tt>
   * </ul>
   *
   * It is allowed for options in the underlying <tt>*options</tt> object to be modified
   * without recalling <tt>this->set_options()</tt> again.
   */
  virtual void set_options( const options_ptr_t& options ) = 0;

  ///
  /** Get the <tt>OptionsFromStream</tt> object being used to extract the options from.
   */
  virtual const options_ptr_t& get_options() const = 0;

  //@}

  /** @name Algorithm configuration and initialization */
  //@{

  ///
  /** Configure the rSQP algorithm container with an rSQP algorithm object.
   *
   * @param  algo_cntr
   *                [in/out] On input, \c algo_cntr may or may not already have
   *                a configured algorithm.  The options set from the interface
   *                \c NLPSolverClientInterface may be used determine how the
   *                algorithm is to be configured.  On output, \c algo_cntr will
   *                have a configured algorithm (ready to call
   *                \c algo_cntr->algo().interface_print_algorithm(...)).
   * @param  trase_out
   *               [in/out] If <tt>trase_out != NULL</tt> on input, then \c *trase_out
   *               will recieve a discription about the logic of how the algorithm is
   *               configured.
   *
   * Preconditions:<ul>
   * <li> <tt>algo_cntr != NULL</tt> (throw <tt>???</tt>)
   * <li> <tt>algo_cntr->get_nlp().get() != NULL</tt> (throw <tt>???</tt>)
   * <li> <tt>algo_cntr->get_track().get() != NULL</tt> (throw <tt>???</tt>)
   * </ul>
   *
   * Postconditions:<ul>
   * <li> <tt>algo_cntr->get_algo().get() != NULL</tt> (throw <tt>???</tt>)
   * </ul>
   *
   * Note that if the type of NLP return by <tt>algo_cntr->nlp()</tt> is not supported
   * by this configuration object, then a <tt>InvalidNLPType</tt> exception will be
   * thrown.
   */
  virtual void config_algo_cntr(
    NLPAlgoContainer*       algo_cntr
    ,std::ostream*           trase_out = 0
    ) = 0;

  ///
  /** Initialize the rSQP algorithm object for the start of SQP iterations.
   *
   * @param  algo_cntr
   *                [in/out] On input, \c algo_cntr must already have
   *                a configured algorithm.   On output, \c algo_cntr will
   *                be ready to solve the NLP (i.e. ready to call
   *                \c algo_cntr->algo().dispatch(...)).
   *
   * Preconditions:<ul>
   * <li> <tt>algo_cntr != NULL</tt> (throw <tt>???</tt>)
   * <li> <tt>algo_cntr->get_nlp().get() != NULL</tt> (throw <tt>???</tt>)
   * <li> <tt>algo_cntr->get_track().get() != NULL</tt> (throw <tt>???</tt>)
   * <li> <tt>algo_cntr->get_algo().get() != NULL</tt> (throw <tt>???</tt>)
   * </ul>
   * 
   * Postconditions:<ul>
   * <li> <tt>algo_cntr</tt> ready to solve the NLP.
   * </ul>
   */
  virtual void init_algo(
    NLPAlgoInterface* algo
    ) = 0;

  //@}

private:

#ifdef DOXYGEN_COMPILE // Strictly for doxygen diagrams
  ///
  OptionsFromStreamPack::OptionsFromStream    *options;
#endif

};	// end class NLPAlgoConfig

}	// end namespace MoochoPack

#endif	// RSQP_ALGO_CONFIG_H
