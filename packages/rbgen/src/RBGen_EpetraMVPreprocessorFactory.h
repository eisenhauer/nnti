#ifndef RBGEN_EPETRAMV_PREPROCESSOR_FACTORY_HPP
#define RBGEN_EPETRAMV_PREPROCESSOR_FACTORY_HPP

#include "RBGen_PreprocessorFactory.hpp"
#include "RBGen_NoPreprocessor.hpp"
#include "RBGen_MSPreprocessor.h"

// Forward declaration of Epetra_MultiVector.
class Epetra_MultiVector;

namespace RBGen {

  class EpetraMVPreprocessorFactory : public virtual PreprocessorFactory<Epetra_MultiVector> {

  public:
    //@{ @name Constructor/Destructor.

    //! Default constructor.
    EpetraMVPreprocessorFactory();

    //! Destructor.
    virtual ~EpetraMVPreprocessorFactory() {};
    //@}

    //@{ @name Factory methods

    Teuchos::RefCountPtr<Preprocessor<Epetra_MultiVector> > create( const Teuchos::ParameterList& params );

    //@}

  private:

    // Available preprocessing types
    std::vector<std::string> preproc_types;

  };
 
} // end of RBGen namespace

#endif
