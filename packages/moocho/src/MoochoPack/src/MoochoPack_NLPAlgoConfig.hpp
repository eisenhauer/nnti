// ////////////////////////////////////////////////////////////////////////////
// rSQPAlgo_Config.h
//
// Copyright (C) 2001 Roscoe Ainsworth Bartlett
//
// This is free software; you can redistribute it and/or modify it
// under the terms of the "Artistic License" (see the web site
//   http://www.opensource.org/licenses/artistic-license.html).
// This license is spelled out in the file COPYING.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// above mentioned "Artistic License" for more details.

#ifndef RSQP_ALGO_CONFIG_H
#define RSQP_ALGO_CONFIG_H

#include "ReducedSpaceSQPPackTypes.h"

namespace OptionsFromStreamPack {
	class OptionsFromStream;
}

namespace ReducedSpaceSQPPack {

class rSQPAlgoContainer;
class rSQPAlgoInterface;

///
/** Interface for objects responsible for configuring an rSQP algorithm.
 *
 * Objects of this type configure a \c rSQPAlgoContainer object with an rSQP algorithm object,
 * configuring the algo with step, state etc. objects and initailizing the algorithm before the
 * interations start.
 */
class rSQPAlgo_Config {
public:

	/** @name Public types */
	//@{

	/// Thrown if NLP type is incompatible with this config
	class InvalidNLPType : public std::logic_error
	{public: InvalidNLPType(const std::string& what_arg) : std::logic_error(what_arg) {}};
	///
	typedef MemMngPack::ref_count_ptr<
		const OptionsFromStreamPack::OptionsFromStream>             options_ptr_t;

	//@}

	/** @name Constructors/destructors */
	//@{

	///
	virtual ~rSQPAlgo_Config() {}

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
	 *                \c rSQPSolverClientInterface may be used determine how the
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
		rSQPAlgoContainer*       algo_cntr
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
		rSQPAlgoInterface* algo
		) = 0;

	//@}

private:

#ifdef DOXYGEN_COMPILE // Strictly for doxygen diagrams
	///
	OptionsFromStreamPack::OptionsFromStream    *options;
#endif

};	// end class rSQPAlgo_Config

}	// end namespace ReducedSpaceSQPPack

#endif	// RSQP_ALGO_CONFIG_H
