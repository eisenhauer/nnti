#ifndef XPETRA_EPETRAEXPORT_HPP
#define XPETRA_EPETRAEXPORT_HPP

/* this file is automatically generated - do not edit (see script/epetra.py) */

#include "Xpetra_EpetraConfigDefs.hpp"

#include "Xpetra_Export.hpp"

#include "Xpetra_EpetraMap.hpp"//TMP

#include "Epetra_Export.h"

namespace Teuchos { class ParameterList; }

// Note: 'export' is a reserved keyword in C++. Do not use 'export' as a variable name.

namespace Xpetra {

  // TODO: move that elsewhere
  const Epetra_Export & toEpetra(const Export<int, int> &);
  //

  class EpetraExport
    : public Export<int, int>
  {

    typedef int LocalOrdinal;
    typedef int GlobalOrdinal;
    typedef Kokkos::DefaultNode::DefaultNodeType Node;
    //! The specialization of Map used by this class.
    typedef Map<LocalOrdinal,GlobalOrdinal,Node> map_type;

  public:

    //! @name Constructor/Destructor Methods
    //@{

    //! Construct a Export object from the source and target Map.
    EpetraExport(const Teuchos::RCP< const map_type > &source, const Teuchos::RCP< const map_type > &target);

    //! Constructor (with list of parameters).
    EpetraExport(const Teuchos::RCP< const map_type > &source, const Teuchos::RCP< const map_type > &target, const Teuchos::RCP< Teuchos::ParameterList > &plist);

    //! Copy constructor.
    EpetraExport(const Export< LocalOrdinal, GlobalOrdinal, Node > &rhs);

    //! Destructor.
    ~EpetraExport() { }

    //@}

    //! @name Export Attribute Methods
    //@{

    //! Number of initial identical IDs.
    size_t getNumSameIDs() const { XPETRA_MONITOR("EpetraExport::getNumSameIDs"); return export_->NumSameIDs(); }

    //! Number of IDs to permute but not to communicate.
    size_t getNumPermuteIDs() const { XPETRA_MONITOR("EpetraExport::getNumPermuteIDs"); return export_->NumPermuteIDs(); }

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
    ArrayView< const int > getExportImageIDs() const;

    //! The source Map used to construct this Export.
    const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getSourceMap() const { XPETRA_MONITOR("EpetraExport::getSourceMap"); return toXpetra(export_->SourceMap()); }

    //! The target Map used to construct this Export.
    const RCP< const Map< LocalOrdinal, GlobalOrdinal, Node > >  getTargetMap() const { XPETRA_MONITOR("EpetraExport::getTargetMap"); return toXpetra(export_->TargetMap()); }

    //@}

    //! @name I/O Methods
    //@{

    //! Print the Export's data to the given output stream.
    void print(std::ostream &os) const;

    //@}

    //! @name Xpetra specific
    //@{

    //! EpetraExport constructor to wrap a Epetra_Export object
    EpetraExport(const RCP<const Epetra_Export> &exp) : export_(exp) {  }

    //! Get the underlying Epetra export
    RCP< const Epetra_Export> getEpetra_Export() const { return export_; }

    //@}
    
  private:
    
    RCP<const Epetra_Export> export_;

  }; // EpetraExport class

} // Xpetra namespace

#endif // XPETRA_EPETRAEXPORT_HPP
