// //////////////////////////////////////////////////////////
// MoochoSolver.hpp
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

#ifndef RSSQPP_RSQPPP_SOLVER_H
#define RSSQPP_RSQPPP_SOLVER_H

#include "MoochoPack/src/MoochoPackTypes.hpp"
#include "MoochoPack/src/NLPAlgoContainer.hpp"
#include "Teuchos_RefCountPtr.hpp"

namespace OptionsFromStreamPack {
	class OptionsFromStream;
}

namespace MoochoPack {

///
/** Universal interface to a MOOCHO solver.
 *
 * This class is designed to act as a simple encapsulation to several
 * other smaller components needed to solve an NLP.  This class is an
 * instance of the popular "Facade" design pattern (Design Patterns, 1995).
 * This class has a defualt implementation based on <tt>NLPAlgoConfigMamaJama</tt>
 * but the client can set different algorithm configuration objects (see
 * the requirments/specifications section below).
 *
 * There are two distinct activities associated with using a <tt>MoochoSolver</tt> object:
 * <ol>
 * <li> Algorithm configuration (i.e. configure an encapsulated
 *   <tt>NLPAlgoClientInterface</tt> object with an <tt>NLP</tt> and other objects).
 * <li> NLP solution (i.e. call <tt>NLPSolverClientInterface::find_min()</tt> on the
 *   encapuslaited solver object).
 * </ol>
 *
 * In the algorithm configuration phase, the client must, at a minimum, set the
 * NLP object for the NLP to be solved using <tt>this->set_nlp()</tt>.
 * The NLP object is needed so that the algorithm configuration object can adapt
 * the MOOCHO algorithm to the NLP in the best way possible.  The configuration
 * phase can also include setting a user defined track object(s) and a user
 * defined <tt>NLPAlgoConfig</tt> object.  An NLP is solved by calling
 * the method <tt>this->solve_nlp()</tt> which returns an <tt>enum</tt>
 * stating what happended and reporting the final point to the <tt>NLP</tt>
 * object.
 *
 * This class encapsulates a <tt>NLPAlgoClientInterface</tt> object and takes
 * over some of the drudgery of working with this interface.  In most cases
 * all the options that can be set to configuration object and other algorithmic
 * objects can be set using an <tt>OptionsFromStreamPack::OptionsFromStream</tt>
 * object by calling <tt>this->set_options()</tt>.
 *
 * Options specific to this class and the configuration object (down to the lower
 * algorithmic objects that it creates) can be set through an
 * <tt>OptionsFromStreamPack::OptionsFromStream</tt> object by passing it to
 * <tt>this->set_options()</tt>.  The files
 * <tt>\ref MoochoSolver_opts "Moocho.opt.MoochoSolver"</tt>,
 * <tt>\ref DecompositionSystemStateStepBuilderStd_opts "Moocho.opt.DecompositionSystemStateStepBuilderStd"</tt>
 * and <tt>\ref NLPAlgoConfigMamaJama_opts "Moocho.opt.NLPAlgoConfigMamaJama"</tt>
 * conatain the listing of these options as well as some documentation.
 * An options file <tt>Moocho.opt</tt> can be generated automatically
 * using the shell script <tt>generate_opt_file.x</tt>.
 *
 * <b>Requirements / Specifications</b>
 *
 * The requirements and specifications for this class are stated below.  More
 * detailed scenarios are shown elsewhere (??? where ???).
 * <ol>
 * <li> Base default implementation on <tt>NLPAlgoConfigMamaJama</tt> and require
 *   minimal effort to quickly solve an NLP.  This includes setting up standard
 *   <tt>IterationPack::AlgorithmTracker</tt> objects and taking care of
 *   exceptions etc.<br>
 *   <b>Enabler</b>: The client can simply not call <tt>this->set_config()</tt>
 *   or call <tt>this->set_config(NULL)</tt> which will result in the default
 *   configuration object being used.
 * <li> Allow clients the ability to insert a customized <tt>AlgorithmTracker</tt>
 *   object, in addition to the other standard track objects.<br>
 *   <b>Enabler</b>: An extra user defined track object can be set using
 *   <tt>this->set_extra_track()</tt> and can be unset using
 *   <tt>this->set_extra_track(NULL)</tt>.  Multiple track objects can be handled
 *   using the <tt>IterationPack::AlgorithmTrackerComposite</tt> subclass.
 * <li> Allow clients to set the targets for standard output streams (i.e.
 *   <tt>summary_out</tt>, <tt>journal_out</tt> and <tt>console_out</tt>)
 *   at all times (i.e. between successive solves) or just use default output
 *   files.<br>
 *   <b>Enabler</b>: Default output targets can be used by doing nothing.  User
 *   specified output streams can be set using <tt>this->set_console_out()</tt>,
 *   <tt>this->set_summary_out()</tt> and <tt>this->set_journal_out()</tt>.
 *   The same output files can be appended to for successive NLP solves by
 *   doing nothing.  The output files can be changed between NLP runs using
 *   <tt>this->set_console_out()</tt>, <tt>this->set_summary_out()</tt> and
 *   <tt>this->set_journal_out()</tt>.  Default output files can be overwritten
 *   between successive NLP solves by calling <tt>this->set_console_out(NULL)</tt>,
 *   <tt>this->set_summary_out(NULL)</tt> and <tt>this->set_journal_out(NULL)</tt>.
 * <li> Allow clients to set any special options in <tt>NLPAlgoConfigMamaJama</tt>
 *   beyond what can be set through the <tt>OptionsFromStream</tt> object (i.e. set
 *   a specialized <tt>BasisSystem</tt> object).<br>
 *   <b>Enabler</b>: This can be accomplished by having the client create a
 *   <tt>NLPAlgoConfigMamaJama</tt> object itself and then configure it using
 *   the published interface before explicitly setting it using <tt>this->set_config()</tt>.
 * <li> Allow clients to precisly control how an algorithm is configured and initialized
 *   beyond <tt>NLPAlgoConfigMamaJama</tt> and between NLP solves.<br>
 *   <b>Enabler</b>:  This can be accomplised by allowing the client to set a customized
 *   <tt>NLPAlgoConfig</tt> object.  Clients can simply modify algorithms created by
 *   <tt>NLPAlgoConfigMamaJama</tt> using delegation or subclassing (delegation is
 *   to be prefered).
 * <li> Allow clients to solve the same NLP (i.e. same dimensions, same structure etc)
 *   multiple times with the same configured MOOCHO algorithm.<br>
 *   <b>Enabler</b>:  This can be done By simply calling <tt>this->get_nlp()</tt> (if
 *   needed to access the NLP that was set using <tt>this->set_nlp()</tt>),
 *   modifying the NLP object in some way (i.e. a new initial point) and then calling
 *   <tt>this->solve_nlp()</tt>.
 * <li> Allow clients to configure a new MOOCHO algorithm with a potentially new NLP
 *   object (i.e. different dimensions, different structure etc).<br>
 *   <b>Enabler</b>: The client can just call <tt>this->set_uninitialized()</tt> which
 *   is equivalent to setting the state of the object after the default constructor.
 *   In this case the client will have to go through the entire reinitialization
 *   phase again.  Or, in order to use the same NLP, track and configuration objects
 *   but start off with a fresh algorithm configuration the client can just call
 *   <tt>this->set_nlp()</tt>.
 * </ol>
 *
 * ToDo: Finish documentation!
 */
class MoochoSolver {
public:

	/** Public types */
	//@{

	///
	typedef Teuchos::RefCountPtr<
		NLPInterfacePack::NLP>                                       nlp_ptr_t; // full path needed by doxygen
	///
	typedef Teuchos::RefCountPtr<
		IterationPack::AlgorithmTracker>                        track_ptr_t; // full path needed by doxygen
	///
	typedef Teuchos::RefCountPtr<NLPAlgoConfig>    config_ptr_t;
	///
	typedef Teuchos::RefCountPtr<
		OptionsFromStreamPack::OptionsFromStream>                    options_ptr_t;
	///
	typedef Teuchos::RefCountPtr<std::ostream>       ostream_ptr_t;
	///
	enum ESolutionStatus {
		SOLVE_RETURN_SOLVED            =  0
		,SOLVE_RETURN_NLP_TEST_FAILED  = -1
		,SOLVE_RETURN_MAX_ITER         = -2
		,SOLVE_RETURN_MAX_RUN_TIME     = -3
		,SOLVE_RETURN_EXCEPTION        = -4
	};

	enum EConfigOptions {
		MAMA_JAMA
		,INTERIOR_POINT
	};

	//@}

	/** @name Initialization and algorithm configuration */
	//@{

	///
	/** Constructs to uninitialized.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>this->throw_exceptions() == false</tt>
	 * <li> ToDo: Fill these in!
	 * </ul>
	 */
	MoochoSolver();

	///
	/** Set the NLP to be solved.
	 *
	 * Preconditions:<ul>
	 * <li> <tt>nlp.get() != NULL</tt> (throw <tt>std::invalid_argument</tt>).
	 * <li> This <tt>NLP</tt> object must be ready to have <tt>nlp->initialize()</tt>
	 *   called but <tt>nlp->is_initialized()</tt> can be \c false.
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li> <tt>this->get_nlp().get() == nlp.get()</tt>
	 * <li> This will cause the MOOCHO algorithm to be reconfigured before the NLP
	 *   is solved again.
	 * </ul>
	 */
	void set_nlp(const nlp_ptr_t& nlp);
	
	///
	/** Get the non-const smart pointer to the set NLP object.
	 *
	 * @return Returns the nlp object pointer set by <tt>this->set_nlp()</tt>.
	 */
	const nlp_ptr_t& get_nlp() const;
	
	/** Set an extra user defined <tt>AlgorithmTracker</tt> object to monitor the algorithm.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>this->get_track().get() == track.get()</tt>
	 * </ul>
	 */
	void set_track(const track_ptr_t& track);
	
	///
	/** Get the non-const smart pointer to the set <tt>AlgorithmTracker</tt> object.
	 *
	 * @return Returns the track object pointer set by <tt>this->set_track()</tt>.
	 */
	const track_ptr_t& get_track() const;

	///
	/** Set the algorithm configuration object.
	 *
	 * @param  config  [in] The algorithm configuration object to use the
	 *                 next time <tt>this->do_config_algo()</tt> or
	 *                 <tt>this->solve_nlp()</tt> are called.
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>config.get() == NULL</tt>] A <tt>NLPAlgoConfigMamaJama</tt>
	 *   object will be used to configure the MOOCHO algorithm the next time
	 *   that <tt>this->do_config_algo()</tt> or <tt>this->solve_nlp()</tt> are called.
	 * <li> [<tt>config.get() != NULL</tt>] The object <tt>*config</tt> will be used
	 *   to configure the MOOCHO algorithm the next time that <tt>this->do_config_algo()</tt>
	 *    or <tt>this->solve_nlp()</tt> are called.
	 * <li> A reconfiguration of the MOOCHO algorithm will be forced the next time that
	 *   <tt>this->do_config_algo()</tt> or <tt>this->solve_nlp()</tt> are called.
	 * <li> <tt>this->get_config().get() == config.get()</tt>
	 * </ul>
	 */
	void set_config( const config_ptr_t& config );

	///
	/** Return the configuration object being used.
	 *
	 * @return Returns <tt>return.get() == config.get()</tt> passed to the last call to
	 * <tt>this->set_config(config)</tt>.  
	 *
	 */
	const config_ptr_t& get_config() const;

	///
	/** Set the various options to use.
	 *
	 * @param  options  [in]  Smart pointer to the <tt>OptionsFromStream</tt> object
	 *                  to extract the options for <tt>this</tt> as well as for
	 *                  the configuration object from.  If <tt>options.get() != NULL</tt>
	 *                  then this object must not be destoryed until
	 *                  <tt>this->set_options(NULL)</tt> is called or <tt>this</tt>
	 *                  is destroyed.
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>options.get() == NULL</tt>] The file "Moocho.opt" will be looked for and
	 *   opened in the current directory.  If this file does not exist, then a default
	 *   set of options will be used.  If this file does exist then the options will be
	 *   read from this file.
	 * <li> [<tt>options.get() != NULL</tt>] The options will be read from <tt>*options</tt>.
	 * <li> A reconfiguration of the MOOCHO algorithm will be forced the next time that
	 *   <tt>this->do_config_algo()</tt> or <tt>this->solve_nlp()</tt> are called.
	 * <li> <tt>this->get_options().get() == options.get()</tt>
	 * </ul>
	 */
	void set_options( const options_ptr_t& options );

	///
	/** Get the <tt>OptionsFromStream</tt> object being used to extract the options from.
	 *
	 * ToDo: Finish documentation.
	 */
	const options_ptr_t& get_options() const;

	///
	/** Set the error output and whether exceptions will be thrown from these functions or not.
	 *
	 * @param  throw_exceptions
	 *                [in] If \c true, then after printing the error message (see error_out) the
	 *                exception will be rethrown out of <tt>this->solve_nlp()</tt>.
	 * @param  error_out
	 *                [in] If <tt>error_out.get() != NULL</tt>, then the error messages from any thrown
	 *                <tt>std::exception</tt> will be printed to <tt>*error_out</tt>.  Otherwise, they
	 *                will be printed to <tt>std::cerr</tt>.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>this->get_error_out().get() == error_out.get()</tt>
	 * <li> <tt>this->throw_exceptions() == throw_exceptions()</tt>
	 * </ul>
	 */
	void set_error_handling(
		bool                    throw_exceptions
		,const ostream_ptr_t&   error_out
		);

	///
	/** Return if exceptions will be thrown out of <tt>this->solve_nlp()</tt>.
	 */
	bool throw_exceptions() const;

	///
	/** Return the <tt>std::ostream</tt> object used for error reporting on exceptions.
	 *
	 * If <tt>return.get() == NULL</tt> then this means that <tt>std::cerr</tt> wil be
	 * written to.
	 */
	const ostream_ptr_t& error_out() const;

	///
	/** Turn on and off console outputting.
	 */
	void do_console_outputting(bool);

	///
	/** Return if console outputting is performed or not.
	 */
	bool do_console_outputting() const;

	///
	/** Set the <tt>std::ostream</tt> object to use for console output
	 * by a <tt>MoochoTrackerConsoleStd</tt> object.
	 *
	 * @param  console_out  [in] Smart pointer to an <tt>std::ostream</tt>
	 *                      object that output appropriate for the console
	 *                      is sent to.  An obvious choice for this output
	 *                      stream is <tt>std::cout</tt> can can be set using
	 *                      <tt>this->set_console_out(rcp(&std::cout,false))</tt>.
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>summary_out.get() == NULL</tt>] The stream <tt>std::cout</tt>
	 *   will be written to with summary output from a <tt>MoochoTrackerConsoleStd</tt>
	 *   object the next time <tt>this->solve_nlp()</tt> is called.
	 * <li> [<tt>console_out.get() != NULL</tt>] Output appropriate for the
	 *   console will be sent to <tt>*console_out</tt> the next time
	 *   <tt>this->solve_nlp()</tt> is called by a <tt>MoochoTrackerConsoleStd</tt>
	 *   object
	 * <li> <tt>this->get_console_out().get() == console_out.get()</tt>
	 * </ul>
	 *
	 * ToDo: Discuss exactly what is printed to this output stream.
	 */
	void set_console_out( const ostream_ptr_t& console_out );

	///
	/** Get the non-const smart pointer to the set output stream for console outputting.
	 *
	 * @return Returns the console_out smart pointer object pointer set by
	 * the last call to <tt>this->set_console_out()</tt>.  Not that if
	 * <tt>console_out.get() == NULL</tt> on in the last call to
	 * <tt>this->set_console_out(console_out)</tt> this this method returns
	 * <tt>return.get() == NULL</tt> which is a flag that the stream
	 * \c std::cout is being written to and this function does not
	 * provide access to that <tt>std::ofstream</tt> object (the client
	 * can access that stream themselves).
	 */
	const ostream_ptr_t& get_console_out() const;
	
	///
	/** Turn on and off summary outputting.
	 */
	void do_summary_outputting(bool);

	///
	/** Return if summary outputting is performed or not.
	 */
	bool do_summary_outputting() const;

	///
	/** Set the <tt>std::ostream</tt> object to use for summary output.
	 *
	 * @param  summay_out  [in] Smart pointer to <tt>std::ostream</tt> object
	 *                     that summary output is sent to.
	 * 
	 * Postconditions:<ul>
	 * <li> [<tt>summary_out.get() == NULL</tt>] The file "MoochoSummary.out"
	 *   will be overwritten with summary output by a <tt>MoochoTrackerSummaryStd</tt>
	 *   object the next time <tt>this->solve_nlp()</tt> is called.
	 * <li> [<tt>summary_out.get() != NULL</tt>]  The stream <tt>*summary_out</tt> will
	 *   be written to with summary output by a <tt>MoochoTrackerSummaryStd</tt>
	 *   object the next time <tt>this->solve_nlp()</tt> is called.
	 * <li> <tt>this->get_summary_out().get() == summary_out.get()</tt>
	 * </ul>
	 *
	 * ToDo: Discuss exactly what is printed to this output stream.
	 */
	void set_summary_out( const ostream_ptr_t& summary_out );
	
	///
	/** Get the non-const smart pointer to the set output stream for summary outputting.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>return.get() == summary_out.get()</tt> where <tt>summary_out</tt> was the
	 *   value last sent to <tt>this->set_summary_out(summary_out)</tt>.
	 * </ul>
	 *
	 * @return Returns the summary_out smart pointer object pointer set by
	 * the last call to <tt>this->set_summary_out()</tt>.  Not that if
	 * <tt>summary_out.get() == NULL</tt> on in the last call to
	 * <tt>this->set_summary_out(summary_out)</tt> this this method returns
	 * <tt>return.get() == NULL</tt> which is a flag that the file
	 * "MoochoSummary.out" is being written to and this function does not
	 * provide access to that <tt>std::ofstream</tt> object.
	 */
	const ostream_ptr_t& get_summary_out() const;

	///
	/** Turn on and off journal outputting.
	 */
	void do_journal_outputting(bool);

	///
	/** Return if journal outputting is performed or not.
	 */
	bool do_journal_outputting() const;

	///
	/** Set the <tt>std::ostream</tt> object to use for journal output by the
	 * MOOCHO step objects.
	 *
	 * @param  journal_out [in] Smart pointer to an <tt>std::ostream</tt> object
	 *                     that journal output will be set to.
	 *
	 * Note that if the option
	 * <tt>NLPSolverClientInterface::journal_output_level == PRINT_NOTHING</tt>
	 * in the last call to <tt>this->set_options(options)</tt> then no output
	 * is sent to this stream at all.
	 * 
	 * Postconditions:<ul>
	 * <li> [<tt>journal_out.get() == NULL</tt>] The file "MoochoJournal.out"
	 *   will be overwritten with journal output.
	 *   object the next time <tt>this->solve_nlp()</tt> is called.
	 * <li> [<tt>journal_out.get() != NULL</tt>]  The stream <tt>*journal_out</tt> will
	 *   be written to with journal output the next time <tt>this->solve_nlp()</tt>
	 *   is called.
	 * <li> <tt>this->get_journal_out().get() == journal_out.get()</tt>
	 * </ul>
	 */
	void set_journal_out( const ostream_ptr_t& journal_out );
	
	///
	/** Get the non-const smart pointer to the set output stream for journal outputting.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>return.get() == journal_out.get()</tt> where <tt>journal_out</tt> was the
	 *   value last sent to <tt>this->set_journal_out(journal_out)</tt>.
	 * </ul>
	 *
	 * @return Returns the journal_out smart pointer object pointer set by
	 * the last call to <tt>this->set_journal_out()</tt>.  Not that if
	 * <tt>journal_out.get() == NULL</tt> on in the last call to
	 * <tt>this->set_journal_out(journal_out)</tt> this this method returns
	 * <tt>return.get() == NULL</tt> which is a flag that the file
	 * "MoochoJournal.out" is being written to and this function does not
	 * provide access to that <tt>std::ofstream</tt> object.
	 */
	const ostream_ptr_t& get_journal_out() const;

	///
	/** Turn on and off algo outputting.
	 */
	void do_algo_outputting(bool);

	///
	/** Return if algo outputting is performed or not.
	 */
	bool do_algo_outputting() const;

	///
	/** Turn on and off the generation of a statistics file.
	 */
	void generate_stats_file(bool);

	///
	/** Return if a statistics file will be generated or not.
	 */
	bool generate_stats_file() const;

	///
	/** Set the <tt>std::ostream</tt> object to use for algorithm output.
	 *
	 * @param  algo_out  [in] Smart pointer to an <tt>std::ostream</tt> object
	 *                   that static algorithm output will be set to.
	 *
	 * Postconditions:<ul>
	 * <li> [<tt>algo_out.get() == NULL</tt>] The file "MoochoAlgo.out"
	 *   will be overwritten with algorithm info.
	 *   object the next time <tt>this->solve_nlp()</tt> is called.
	 * <li> [<tt>algo_out.get() != NULL</tt>]  The stream <tt>*algo_out</tt> will
	 *   be written to with algorithm info the next time <tt>this->solve_nlp()</tt>
	 *   is called.
	 * <li> <tt>this->get_algo_out().get() == algo_out.get()</tt>
	 * </ul>
	 *
	 * Note that if the option <tt>NLPSolverClientInterface::print_algo == false</tt>
	 * in the last call to <tt>this->set_options(options)</tt> then no output
	 * is sent to this stream at all.
	 *
	 * ToDo: Discuss exactly what is printed to this output stream.
	 */
	void set_algo_out( const ostream_ptr_t& algo_out );
	
	///
	/** Get the non-const smart pointer to the set output stream for algo outputting.
	 *
	 * Postconditions:<ul>
	 * <li> <tt>return.get() == algo_out.get()</tt> where <tt>algo_out</tt> was the
	 *   value last sent to <tt>this->set_algo_out(algo_out)</tt>.
	 * </ul>
	 *
	 * @return Returns the algo_out smart pointer object pointer set by
	 * the last call to <tt>this->set_algo_out()</tt>.  Not that if
	 * <tt>algo_out.get() == NULL</tt> on in the last call to
	 * <tt>this->set_algo_out(algo_out)</tt> this this method returns
	 * <tt>return.get() == NULL</tt> which is a flag that the file
	 * "MoochoAlgo.out" is being written to and this function does not
	 * provide access to that <tt>std::ofstream</tt> object.
	 */
	const ostream_ptr_t& get_algo_out() const;

	///
	/** Set all output streams to <tt>Teuchos::oblackholestream</tt> unless
	 * set otherwise.
	 *
	 * @param  send_all_output_to_black_hole
	 *               [in] If <tt>true</tt> then all output (i.e. journal, summary
	 *               console, algo etc.) will all be sent to a stream object
	 *               <tt>Teuchos::oblackholestream</tt>.  If <tt>false</tt>
	 *               then the default behavior is maintained.
	 *
	 * This feature is designed to be used in special situations such as
	 * in SPMD mode where only one process should do outputtting.  The
	 * root process should call <tt>send_all_output_to_black_hole(false)</tt>
	 * and all of the slave processes should call
	 * <tt>send_all_output_to_black_hole(true)</tt>.
	 *
	 * Note, this function must be called before <tt>solve_nlp()</tt>.
	 */
	void send_all_output_to_black_hole( bool send_all_output_to_black_hole );

	///
	/** Returns the current value of <tt>send_all_output_to_black_hole</tt> set by
	 * <tt>send_all_output_to_black_hole()</tt>.
	 *
	 */
  bool send_all_output_to_black_hole() const;

	//@}

	/** @name Solve the NLP */
	//@{

	///
	/** Solve the NLP.
	 *
	 * Preconditions:<ul>
	 * <li> <tt>this->get_nlp() != NULL</tt> (throw <tt>std::logic_error</tt>)
	 * </ul>
	 *
	 * <b>Algorithm configuration:</b><br>
	 * The <tt>NLPAlgoConfig</tt> object used to configure an optimization algorithm is specified
	 * by <tt>*this->get_config()</tt>.  If <tt>this->get_config().get() == NULL</tt> then
	 * the default configuratiion class <tt>NLPAlgoConfigMamaJama</tt> is used.  This default
	 * configuration class is full featured and should have a good shot at solving any NLP thrown at it.
	 *
	 * <b>Specifying options:</b><br>
	 * The options used by the configuration object to configure the optimization algorithm as well
	 * as solver tolerances, maximum number of iterations etc are taken from the
	 * <tt>OptionsFromStreamPack::OptionsFromStream</tt> object returned from <tt>*this->get_options()</tt>.
	 * If <tt>this->get_options().get() == NULL</tt> then an attempt is made to open the file 'Moocho.opt'
	 * in the current directory.  If this file does not exist, then a default set of options is used
	 * which will be acceptable for most NLPs.  The files <tt>\ref MoochoSolver_opts "Moocho.opt.MoochoSolver"</tt>,
	 * <tt>\ref DecompositionSystemStateStepBuilderStd_opts "Moocho.opt.DecompositionSystemStateStepBuilderStd"</tt>
	 * and <tt>\ref NLPAlgoConfigMamaJama_opts "Moocho.opt.NLPAlgoConfigMamaJama"</tt> show which
	 * options can be used with this solver interface and a <tt>NLPAlgoConfigMamaJama</tt> configuration
	 * object respectively.  Other configuration classes will use a different set of options.  See the
	 * documentation for those configuration classes for details.
	 *
	 * <b>Outputting to streams:</b><ul>
	 * <li> [<tt>this->do_console_outputting() == true</tt>] Output will be set to <tt>*this->get_console_out()</tt>
	 *      (or <tt>std::cout</tt> if <tt>this->get_console_out().get() == NULL</tt>) by a
	 *      <tt>MoochoTrackerConsoleStd</tt> object.
	 * <li> [<tt>this->do_summary_outputting() == true</tt>] Output will be set to <tt>*this->get_summary_out()</tt>
	 *      (or the file 'MoochoSummary.out' in the current directory if <tt>this->get_summary_out().get()
	 *      == NULL</tt>) by a <tt>MoochoTrackerSummaryStd</tt> object.
	 * <li> [<tt>this->do_journal_outputting() == true</tt>] Output will be set to <tt>*this->get_journal_out()</tt>
	 *      (or the file 'MoochoJournal.out' in the current directory if <tt>this->get_journal_out().get()
	 *      == NULL</tt>) by the step objects in the optimization algorithm.
	 * <li> [<tt>this->do_algo_outputting() == true</tt>] Output will be set to <tt>*this->get_algo_out()</tt>
	 *      (or the file 'MoochoAlgo.out' in the current directory if <tt>this->get_algo_out().get()
	 *      == NULL</tt>) which contains information on how the optimization algorithm is configured and what
	 *      the algorithm is (if the option 'MoochoSolver::print_algo == true', see the options file
	 *      <tt>\ref MoochoSolver_opts "Moocho.opt.MoochoSolver"</tt>).
	 * </ul>
	 *
	 * If <tt>this->throw_exceptions() == false</tt> then any exceptions that may be thown
	 * internally will be caught, <tt>std::exception::what()</tt>  will be printed to
	 * <tt>*this->error_out()</tt> (or <tt>std::cerr</tt> if <tt>this->error_out().get() == NULL</tt>)
	 * and this method will return <tt>SOLVE_RETURN_EXCEPTION</tt>.
	 * If <tt>this->throw_exceptions() == true</tt>, then after the error has been reported, the
	 * exception will be rethrown out for the caller to deal with!
	 *
	 * Even if no exception is thrown, then a short one-line summary message will be printed to
	 * <tt>*this->error_out()</tt> (or <tt>std::cerr</tt> if <tt>this->error_out().get() == NULL</tt>)
	 * stating whether the NLP was solved or not.
	 *
	 * ToDo: Finish documentation!
	 *
	 * @return The solution status:<ul>
	 * <li> ToDo: Fill these in and discuss them!
	 * </ul>
	 */
	ESolutionStatus solve_nlp() const;

	//@}

	/** @name Get the underlying solver object */
	//@{

	///
	/** Get the underlying <tt>NLPSolverClientInterface</tt> object.
	 *
	 * If the algorithm has not already been configured it will be here using whatever
	 * <tt>NLPAlgoConfig</tt> object that it has (set using <tt>this->set_config*()</tt>
	 * or using the default).  Whatever options where set (i.e. using <tt>this->set_options()</tt>)
	 * will be used when this algorithm is configured and the NLP is solved.
	 *
	 * Preconditions:<ul>
	 * <li> <tt>this->get_nlp() != NULL</tt> (throw <tt>std::logic_error</tt>)
	 * </ul>
	 *
	 * Warning!  Do not try to modify the underlying solver object using this
	 * returned reference to the solver object.  Instead, use the above public
	 * interface functions.
	 *
	 * ToDo: Finish documentation!
	 */
	NLPSolverClientInterface& get_solver();

	///
	const NLPSolverClientInterface& get_solver() const;
	
	//@}

private:

	// //////////////////////////////////////
	// Private types

	///
	typedef Teuchos::RefCountPtr<NLPSolverClientInterface>    solver_ptr_t;
		
	// ////////////////////////////////////
	// Private data members

#ifndef DOXYGEN_COMPILE
	mutable NLPAlgoContainer solver_;          // Solver object.
#else
	mutable NLPAlgoContainer solver;
#endif
	mutable bool              reconfig_solver_; // If true then we must reconfigure the solver!
	mutable value_type        workspace_MB_;
	mutable value_type        obj_scale_;
	mutable bool              test_nlp_;
	mutable bool              print_algo_;
	mutable bool              algo_timing_;
	mutable bool              generate_stats_file_;
	mutable bool              print_opt_grp_not_accessed_;
	mutable bool              throw_exceptions_;
	mutable bool              do_console_outputting_;
	mutable bool              do_summary_outputting_;
	mutable bool              do_journal_outputting_;
	mutable bool              do_algo_outputting_;
	mutable int               configuration_;
#ifndef DOXYGEN_COMPILE
	nlp_ptr_t                 nlp_;
	track_ptr_t               track_;
	config_ptr_t              config_;
	options_ptr_t             options_;          // set by client
	ostream_ptr_t             error_out_;        // set by client
	ostream_ptr_t             algo_out_;         // set by client
	ostream_ptr_t             console_out_;      // set by client
	ostream_ptr_t             summary_out_;      // set by client
	ostream_ptr_t             journal_out_;      // set by client
	mutable options_ptr_t     options_used_;     // actually used (can be NULL)
	mutable ostream_ptr_t     error_out_used_;   // actually used (can't be NULL)
	mutable ostream_ptr_t     console_out_used_; // actually used (can be NULL if do_console_outputting == false)
	mutable ostream_ptr_t     summary_out_used_; // actually used (can be NULL if do_summary_outputting == false)
	mutable ostream_ptr_t     journal_out_used_; // actually used (can be NULL if do_journal_outputting == false)
	mutable ostream_ptr_t     algo_out_used_;    // actually used (can be NULL if do_algo_outputting == false)
	bool                      send_all_output_to_black_hole_;
#endif

	// ////////////////////////////////////
	// Private member functions

	///
	void update_solver() const;

}; // end class MoochoSolver

/** \defgroup MoochoSolver_opts Options for an MoochoSolver object.
 *
 * The following is the contents of the file <tt>Moocho.opt.MoochoSolver</tt> which
 * are options specific to the class <tt>MoochoPack::MoochoSolver</tt>.
 * For options specific to the <tt>%NLPAlgoConfigMamaJama</tt> configuration class
 * see the documentation for
 * <tt>MoochoPack::NLPAlgoConfigMamaJama</tt>.
 *
 * \verbinclude Moocho.opt.MoochoSolver
 */

/** \defgroup DecompositionSystemStateStepBuilderStd_opts Options for common builder object of type DecompositionSystemStateStepBuilderStd object.
 *
 * The following is the contents of the file <tt>Moocho.opt.DecompositionSystemStateStepBuilderStd</tt> which
 * are options that are shared by different specific configuration classes (for example, see
 * <tt>MoochoPack::NLPAlgoConfigMamaJama</tt>).
 *
 * \verbinclude Moocho.opt.DecompositionSystemStateStepBuilderStd
 */

// /////////////////////////////////////////
// Inline members

inline
void MoochoSolver::do_console_outputting(bool do_console_outputting)
{
	do_console_outputting_ = do_console_outputting;
}

inline
bool MoochoSolver::do_console_outputting() const
{
	return do_console_outputting_;
}

inline
void MoochoSolver::do_summary_outputting(bool do_summary_outputting)
{
	do_summary_outputting_ = do_summary_outputting;
}

inline
bool MoochoSolver::do_summary_outputting() const
{
	return do_summary_outputting_;
}

inline
void MoochoSolver::do_journal_outputting(bool do_journal_outputting)
{
	do_journal_outputting_ = do_journal_outputting;
}

inline
bool MoochoSolver::do_journal_outputting() const
{
	return do_journal_outputting_;
}

inline
void MoochoSolver::do_algo_outputting(bool do_algo_outputting)
{
	do_algo_outputting_ = do_algo_outputting;
}

inline
bool MoochoSolver::do_algo_outputting() const
{
	return do_algo_outputting_;
}

inline
void MoochoSolver::generate_stats_file(bool generate_stats_file)
{
	generate_stats_file_ = generate_stats_file;
}

inline
bool MoochoSolver::generate_stats_file() const
{
	return generate_stats_file_;
}

} // end namespace MoochoPack

#endif // RSSQPP_RSQPPP_SOLVER_H
