/*
// @HEADER
// ************************************************************************
//             FEI: Finite Element Interface to Linear Solvers
//                  Copyright (2005) Sandia Corporation.
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation, the
// U.S. Government retains certain rights in this software.
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
// Questions? Contact Alan Williams (william@sandia.gov) 
//
// ************************************************************************
// @HEADER
*/


#ifndef _fei_macros_hpp_
#define _fei_macros_hpp_


/*
 * ALL FEI source files must include this header, either directly or indirectly,
 * before any declaration or executable statement.
 *
 * Once this header has been included, all macros that matter to FEI code are
 * defined, except for "derivative" macros like FEI_OSTREAM which are defined
 * in response to other macros (see fei_iostream.hpp,fei_iosfwd.hpp).
 */


//Simulate bool support if the compiler being used doesn't have built-in bool
//(Is there still such a compiler as of 2007?)
//This should almost never be needed.
#ifdef FEI_SIMULATE_BOOL
#include "fei_bool.h"
#endif


//FEI_config.h contains macros defined by autoconf-configure. If you
//choose not to run configure, you can define the macro
// FEI_BYPASS_CONFIG_H when building fei, and when including fei headers
//from your client code. This way FEI_config.h (generated by configure)
//will not be included.
//Note that if you define FEI_BYPASS_CONFIG_H then you should also define
//appropriate macros that configure would have defined. The necessary ones
//appear below, where they are used to turn on corresponding FEI_ macros.

#ifndef FEI_BYPASS_CONFIG_H
#include "FEI_config.h"
#else

#ifndef HAVE_NO_MPI
#define HAVE_MPI
#endif

#endif

//
// React to various configure-defined macros by setting
// corresponding fei-specific macros.
// Note that we only define fei-specific macros for stuff that we fear may
// not always be present. Things that are assumed to always be present (such
// as <vector>, <string> etc) are included from various fei files without
// macro protection.
//

//If <time.h> is not available, define HAVE_NO_TIME_H and fei files will
//not attempt to include it.

#ifndef HAVE_NO_TIME_H
#define FEI_HAVE_TIME_H
//allows #include <time.h>
#endif

#ifndef HAVE_NO_IOSFWD
#define FEI_HAVE_IOSFWD
//allows #include <iosfwd>
#endif

//
//In most cases the C++ implementation should supply these headers:
// <iosfwd>, <iomanip>, <iostream>, <fstream>, <sstream>
//but some very old C++ implementations used to only supply these:
// <iomanip.h>, <iostream.h>, <fstream.h>, <sstream.h>
//Hopefully these days the 'dotless' headers are always available...
//
//Below, the 'dotless' headers are assumed to be available by default.
//To indicate that one or more of the 'dotless' headers are NOT available,
//define the macro HAVE_NO_'HEADER' where 'HEADER' is the header that isn't
//available. Then, we'll attempt to use the .h version of the header.
//

#include <stdexcept>

#ifdef HAVE_NO_IOMANIP
#define FEI_HAVE_IOMANIP_H
//allows #include <iomanip.h>
#else
#define FEI_HAVE_IOMANIP
//allows #include <iomanip>
#endif

#ifdef HAVE_NO_IOSTREAM
#define FEI_HAVE_IOSTREAM_H
//allows #include <iostream.h>
#else
#define FEI_HAVE_IOSTREAM
//allows #include <iostream>
#endif

#ifdef HAVE_NO_FSTREAM
#define FEI_HAVE_FSTREAM_H
//allows #include <fstream.h>
#else
#define FEI_HAVE_FSTREAM
//allows #include <fstream>
#endif

#ifdef HAVE_NO_SSTREAM
#define FEI_HAVE_SSTREAM_H
//allows #include <sstream.h>
#else
#define FEI_HAVE_SSTREAM
//allows #include <sstream>
#endif

#ifndef FEI_NO_STD_IOS_FMTFLAGS
#define FEI_HAVE_STD_IOS_FMTFLAGS
//see fei_iostream.hpp
#endif

#ifndef HAVE_MPI
#define FEI_SER
//if FEI_SER is defined, don't try to include <mpi.h>
#endif

#include "fei_version.h"

#endif // _fei_macros_hpp_

