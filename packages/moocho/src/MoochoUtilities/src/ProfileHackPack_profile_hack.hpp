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

#ifndef PROFILE_HACK_H
#define PROFILE_HACK_H

#include "StopWatchPack_stopwatch.hpp"

namespace ProfileHackPack {

/** \defgroup ProfileHackPack_grp Poor-mans profiling helper tools.
 * \ingroup Misc_grp
 *
 * These are tools that allow a developer to get some profiling
 * results without fancy profiling tools included with the compiler.
 * These tools collect timings for an entire process and should work
 * properly in a multi-threaded application.  The only inteface
 * element that a developer should be concerned with is the
 * ProfileHackPack::ProfileTiming class.
 */
//@{

///
/** Set the name and time for a function or other section of code.
 *
 * This function can be called multiple times with the same <tt>func_name</tt>
 * in which case the number of function calls and total time for <tt>func_name</tt>
 * will be accumulated.
 */
void set_time( const char func_name[], double time_secs );

///
/** Print out the timing generated by calls to set_time().
 */
void print_timings( std::ostream& out );

///
/** Helper class that takes care of timing.
 *
 * Here is a simple program that uses these this class:
 \verbatim
 #include <iostream>

 int main() {
 #ifdef USE_PROFILE_HACK
     ProfileHackPack::ProfileTiming profile_timing("main()",&std::cout);
 #endif
     for( int i = 0; i < 10; ++i ) {
	     f();
	     g();
     }
     return 0;
     // When main() exists, the destructor for profile_timing will 
     // record the time and will print the total times and number of
	 // calls for "main()", "f()" and "g()" to <tt>std::cout</tt>.
 }

 void f() {
 #ifdef USE_PROFILE_HACK
     ProfileHackPack::ProfileTiming profile_timing("f()");
 #endif
     ...
     //  When f() exists, the destructor for profile_timing will record the
     // time for this function call and increment the number of calls to this
     // function.
 }

 void g() {
 #ifdef USE_PROFILE_HACK
     ProfileHackPack::ProfileTiming profile_timing("g()");
 #endif
     ...
     //  When g() exists, the destructor for profile_timing will record the
     // time for this function call and increment the number of calls to this
     // function.
 }

 \endverbatim
 *
 * ToDo: Show an example of the output that might be generated for the above program.
 *
 * In the above program, if <tt>USE_PROFILE_HACK</tt> is not defined, then there will
 * be no runtime performance penalty and no profiling results will be printed.
 *
 * This class collects timings for an entire process and should work
 * properly in a multi-threaded application.
 */
class ProfileTiming {
public:
	///
	/** 
	 *
	 * @param  func_name  The name of the function to be timed
	 * @param  out        If != NULL then the function times will
	 *                    be printed to this stream when object is destroyed.
	 */
	ProfileTiming( const std::string& func_name, std::ostream* out = NULL )
		: func_name_(func_name), out_(out)
	{
		timer_.reset();
		timer_.start();
	}
	///
	~ProfileTiming()
	{
		set_time( func_name_.c_str(), timer_.read() );
		if( out_ )
			print_timings( *out_ );
	}

private:
	///
	std::string                 func_name_;
	std::ostream                *out_;
	StopWatchPack::stopwatch    timer_;

	// Not defined and not to be called!
	ProfileTiming();
	ProfileTiming(const ProfileTiming&);
	ProfileTiming& operator=(const ProfileTiming&);

}; // end class ProfileTiming

//@}

} // end namespace ProfileHackPack

#endif // PROFILE_HACK_H
