/*--------------------------------------------------------------------*/
/*    Copyright 2005 Sandia Corporation.                              */
/*    Under the terms of Contract DE-AC04-94AL85000, there is a       */
/*    non-exclusive license for use of this work by or on behalf      */
/*    of the U.S. Government.  Export of this program may require     */
/*    a license from the United States Government.                    */
/*--------------------------------------------------------------------*/

#ifndef _fei_fstream_hpp_
#define _fei_fstream_hpp_

#include "fei_macros.hpp"

//
//The stuff in this file somewhat protects us from the fact that some
//platforms may not put stream-related stuff in the std namespace,
//even though most do.
//These days (2007) perhaps all platforms do put everything in std and
//perhaps no platforms still have iostream.h without having <iostream>, etc.
//But historically we've had to account for these possibilities and I see
//little to be gained from removing this flexibility at this point.
//
//The basic mechanism here is to use macros that are defined differently
//for certain situations. An alternative approach would be to import
//symbols into our namespace, but we choose not to do that since it is
//a well-known sin to perform namespace pollution from within a header.
//

#ifdef FEI_HAVE_FSTREAM
#include <fstream>
#define FEI_IFSTREAM std::ifstream
#define FEI_OFSTREAM std::ofstream
#elif defined(FEI_HAVE_FSTREAM_H)
#include <fstream.h>
#define FEI_IFSTREAM ifstream
#define FEI_OFSTREAM ofstream
#endif


#endif

