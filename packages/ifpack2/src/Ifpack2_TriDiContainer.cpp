/*@HEADER
// ***********************************************************************
//
//       Ifpack2: Tempated Object-Oriented Algebraic Preconditioner Package
//                 Copyright (2009) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
//@HEADER
*/

#include "Ifpack2_TriDiContainer_decl.hpp"

#ifdef HAVE_IFPACK2_EXPLICIT_INSTANTIATION

#include "Ifpack2_TriDiContainer_def.hpp"
#include "Ifpack2_ExplicitInstantiationHelpers.hpp"
#include "Ifpack2_ETIHelperMacros.h"


// Explicit instantiation macro for TriDiContainer.
// Only instantiate in the Ifpack2 namespace.

#define LCLINST(S, LO, GO) template class TriDiContainer<Tpetra::CrsMatrix< S , LO , GO>, S >;template class TriDiContainer<Tpetra::RowMatrix< S , LO , GO >, S >;

namespace Ifpack2 {

  IFPACK2_ETI_MANGLING_TYPEDEFS()

  IFPACK2_INSTANTIATE_SLG(LCLINST)

  // FIXME (mfh 24 May 2014) This will result in a duplicate symbol if
  // the default Node type is TPINode.  See the definition of LCLINST
  // above.
#  if defined(HAVE_KOKKOSCLASSIC_THREADPOOL) && defined(HAVE_TPETRA_INST_DOUBLE)

  template class TriDiContainer<Tpetra::CrsMatrix<double, int, int, KokkosClassic::TPINode>, double >;
  template class TriDiContainer<Tpetra::RowMatrix<double, int, int, KokkosClassic::TPINode>, double >;

#  endif // defined(HAVE_KOKKOSCLASSIC_THREADPOOL) && defined(HAVE_TPETRA_INST_DOUBLE)

}

#endif

