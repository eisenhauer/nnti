// @HEADER
//
// ***********************************************************************
//
//             Xpetra: A linear algebra interface package
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
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
// Questions? Contact
//                    Jeremie Gaidamour (jngaida@sandia.gov)
//                    Jonathan Hu       (jhu@sandia.gov)
//                    Ray Tuminaro      (rstumin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#ifndef XPETRA_IMPORT_HPP
#define XPETRA_IMPORT_HPP

/* this file is automatically generated - do not edit (see script/interfaces.py) */

#include <Kokkos_DefaultNode.hpp>
#include <Teuchos_Describable.hpp>
#include <Teuchos_as.hpp>
#include "Xpetra_Map.hpp"
#include <iterator>

namespace Xpetra {

  template <class LocalOrdinal, class GlobalOrdinal = LocalOrdinal, class Node = KokkosClassic::DefaultNode::DefaultNodeType>
  class Import
    : public Teuchos::Describable
  {

  public:

    //! @name Constructor/Destructor Methods
    //@{

    //! Destructor.
    virtual ~Import() { }

   //@}

    //! @name Import Attribute Methods
    //@{

    //! Number of initial identical IDs.
    virtual size_t getNumSameIDs() const = 0;

    //! Number of IDs to permute but not to communicate.
    virtual size_t getNumPermuteIDs() const = 0;

    //! List of local IDs in the source Map that are permuted.
    virtual ArrayView< const LocalOrdinal > getPermuteFromLIDs() const = 0;

    //! List of local IDs in the target Map that are permuted.
    virtual ArrayView< const LocalOrdinal > getPermuteToLIDs() const = 0;

    //! Number of entries not on the calling process.
    virtual size_t getNumRemoteIDs() const = 0;

    //! List of entries in the target Map to receive from other processes.
    virtual ArrayView< const LocalOrdinal > getRemoteLIDs() const = 0;

    //! Number of entries that must be sent by the calling process to other processes.
    virtual size_t getNumExportIDs() const = 0;

    //! List of entries in the source Map that will be sent to other processes.
    virtual ArrayView< const LocalOrdinal > getExportLIDs() const = 0;

    //! List of processes to which entries will be sent.
    virtual ArrayView< const int > getExportPIDs() const = 0;

    //! The Source Map used to construct this Import object.
    virtual const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getSourceMap() const = 0;

    //! The Target Map used to construct this Import object.
    virtual const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getTargetMap() const = 0;

    //@}

    //! @name I/O Methods
    //@{

    //! Print method.
    virtual void print(std::ostream &os) const = 0;

    //@}

  }; // Import class

} // Xpetra namespace

#define XPETRA_IMPORT_SHORT
#endif // XPETRA_IMPORT_HPP
