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
#ifndef XPETRA_EPETRAIMPORT_HPP
#define XPETRA_EPETRAIMPORT_HPP

/* this file is automatically generated - do not edit (see script/epetra.py) */

#include "Xpetra_EpetraConfigDefs.hpp"

#include "Xpetra_Import.hpp"

#include "Xpetra_EpetraMap.hpp"//TMP

#include "Epetra_Import.h"

namespace Xpetra {

  // TODO: move that elsewhere
  //   const Epetra_Import & toEpetra(const Import<int, int> &);

  RCP< const Import<int, int > > toXpetra(const Epetra_Import *import);
  //

  class EpetraImport
    : public Import<int, int>
  {

    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::DefaultNode::DefaultNodeType Node;
    //! The specialization of Map used by this class.
    typedef Map<LocalOrdinal,GlobalOrdinal,Node> map_type;

  public:

    //! @name Constructor/Destructor Methods
    //@{

    //! Construct an Import from the source and target Maps.
    EpetraImport(const Teuchos::RCP< const map_type > &source, const Teuchos::RCP< const map_type > &target);

    //! Constructor (with list of parameters).
    EpetraImport(const Teuchos::RCP< const map_type > &source, const Teuchos::RCP< const map_type > &target, const Teuchos::RCP< Teuchos::ParameterList > &plist);

    //! Copy constructor.
    EpetraImport(const Import< LocalOrdinal, GlobalOrdinal, Node > &import);

    //! Destructor.
    ~EpetraImport() { }

    //@}

    //! @name Import Attribute Methods
    //@{

    //! Number of initial identical IDs.
    size_t getNumSameIDs() const { XPETRA_MONITOR("EpetraImport::getNumSameIDs"); return import_->NumSameIDs(); }

    //! Number of IDs to permute but not to communicate.
    size_t getNumPermuteIDs() const { XPETRA_MONITOR("EpetraImport::getNumPermuteIDs"); return import_->NumPermuteIDs(); }

    //! List of local IDs in the source Map that are permuted.
    ArrayView< const LocalOrdinal > getPermuteFromLIDs() const;

    //! List of local IDs in the target Map that are permuted.
    ArrayView< const LocalOrdinal > getPermuteToLIDs() const;

    //! Number of entries not on the calling process.
    size_t getNumRemoteIDs() const;

    //! List of entries in the target Map to receive from other processes.
    ArrayView< const LocalOrdinal > getRemoteLIDs() const;

    //! Number of entries that must be sent by the calling process to other processes.
    size_t getNumExportIDs() const;

    //! List of entries in the source Map that will be sent to other processes.
    ArrayView< const LocalOrdinal > getExportLIDs() const;

    //! List of processes to which entries will be sent.
    ArrayView< const int > getExportPIDs() const;

    //! The Source Map used to construct this Import object.
    const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getSourceMap() const { XPETRA_MONITOR("EpetraImport::getSourceMap"); return toXpetra(import_->SourceMap()); }

    //! The Target Map used to construct this Import object.
    const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getTargetMap() const { XPETRA_MONITOR("EpetraImport::getTargetMap"); return toXpetra(import_->TargetMap()); }

    //@}

    //! @name I/O Methods
    //@{

    //! Print method.
    void print(std::ostream &os) const;

    //@}

    //! @name Xpetra specific
    //@{

    //! EpetraImport constructor to wrap a Epetra_Import object
    EpetraImport(const RCP<const Epetra_Import> &import) : import_(import) { }

    //! Get the underlying Epetra import
    RCP< const Epetra_Import> getEpetra_Import() const { return import_; }

    //@}

  private:

    RCP<const Epetra_Import> import_;

  }; // EpetraImport class

} // Xpetra namespace

#endif // XPETRA_EPETRAIMPORT_HPP
