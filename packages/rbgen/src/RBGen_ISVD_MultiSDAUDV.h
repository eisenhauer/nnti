#ifndef RBGEN_ISVD_MULTISDAUDV_H
#define RBGEN_ISVD_MULTISDAUDV_H

#include "RBGen_ISVDMultiSDA.h"
#include "RBGen_ISVDUDV.h"

namespace RBGen {

  class ISVD_MultiSDAUDV : public virtual ISVDUDV, public virtual ISVDMultiSDA {
    public:
      //! @name Constructor/Destructor.
      //@{

      //! Default constructor.
      ISVD_MultiSDAUDV();

      //! Destructor.
      virtual ~ISVD_MultiSDAUDV() {};
      //@}

      //! @name Set Methods
      //@{

      //! Initialize the method with the given parameter list and snapshot set.
      void Initialize( const Teuchos::RefCountPtr< Teuchos::ParameterList >& params,
          const Teuchos::RefCountPtr< Epetra_MultiVector >& init,
          const Teuchos::RefCountPtr< RBGen::FileIOHandler< Epetra_CrsMatrix > >& fileio = Teuchos::null );

      //@}
  };

} // end of RBGen namespace

#endif // RBGEN_ISVD_MULTISDAUDV_H
