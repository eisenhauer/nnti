#include "RBGen_ISVD_MultiSDAUDV.h"

namespace RBGen {

  ISVD_MultiSDAUDV::ISVD_MultiSDAUDV() : IncSVDPOD(), ISVDUDV(), ISVDMultiSDA() {}

  void ISVD_MultiSDAUDV::Initialize( 
      const Teuchos::RCP< Teuchos::ParameterList >& params,
      const Teuchos::RCP< const Epetra_MultiVector >& init,
      const Teuchos::RCP< RBGen::FileIOHandler< Epetra_CrsMatrix > >& fileio) {
    IncSVDPOD::Initialize(params,init,fileio);
    ISVDUDV::Initialize(params,init,fileio);
    ISVDMultiSDA::Initialize(params,init,fileio);
  }

} // end of RBGen namespace
