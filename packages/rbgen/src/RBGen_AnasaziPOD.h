#ifndef RBGEN_ANASAZI_POD_H
#define RBGEN_ANASAZI_POD_H

#include "RBGen_PODMethod.hpp"
#include "RBGen_Method.hpp"
#include "Epetra_MultiVector.h"
#include "Epetra_CrsMatrix.h"
#include "Teuchos_ParameterList.hpp"

/*! \file RBGen_AnasaziPOD.h
    \brief Provides POD method using Anasazi eigensolvers.
*/

namespace RBGen {
  
  //! Class for producing a basis using the Anasazi eigensolver
  class AnasaziPOD : public virtual Method<Epetra_MultiVector,Epetra_CrsMatrix>, public virtual PODMethod<double> {
    
  public:
    //! @name Constructor/Destructor.
    //@{

    //! Default constructor.
    AnasaziPOD();

    //! Destructor.
    virtual ~AnasaziPOD() {};
    //@}

    //! @name Computation Methods
    //@{

    //! Compute a basis for the provided snapshots.
    /*! Computes bases for the provided snapshots using Anasazi to solve the eigenproblem defined by \f$ A^T A \f$,
     * where \c A is the snapshot matrix.
     */
    void computeBasis();
    
    //! Append new snapshots to the set, and update the basis.
    /*! \note This method is not supported by AnasaziPOD. Calling it has no effect.
     *
     */
    void updateBasis( const Teuchos::RCP< Epetra_MultiVector >& update_ss ) {};

    //@}

    //! @name Get Methods
    //@{
    
    Teuchos::RCP<const Epetra_MultiVector> getBasis() const { return basis_; }

    std::vector<double> getSingularValues() const { return sv_; }

    double getCompTime() const { return comp_time_; }
    //@}
    
    //! @name Set Methods
    //@{
    
    //! Initialize the method with the given parameter list and snapshot set.
    void Initialize( const Teuchos::RCP< Teuchos::ParameterList >& params,
                     const Teuchos::RCP< const Epetra_MultiVector >& ss,
		     const Teuchos::RCP< RBGen::FileIOHandler< Epetra_CrsMatrix > >& fileio = Teuchos::null );

    //! Reset the snapshot set used to compute the reduced basis.
    void Reset( const Teuchos::RCP<Epetra_MultiVector>& new_ss ) { ss_ = new_ss; }

    //! Reset the operator used to weight the inner product.
    void ResetOp( const Teuchos::RCP<Epetra_CrsMatrix>& new_op ) { op_ = new_op; }

    //@}

    //! @name Status Methods
    //@{

    bool isInitialized() { return isInitialized_; }
 
    //@}

  private:

    // Is this object initialized.
    bool isInitialized_;

    // Is the inner (A'*A) or outer (A*A') product being used for the SVD computation
    bool isInner_;

    // Size of the basis that this method will compute.
    int basis_size_;

    // Computational time (in seconds, using wall clock).
    double comp_time_;

    // Pointers to the snapshots and reduced basis.
    Teuchos::RCP<const Epetra_MultiVector> ss_;
    Teuchos::RCP<Epetra_MultiVector> basis_;

    // Pointer to the inner product operator
    Teuchos::RCP<Epetra_CrsMatrix> op_;

    // Vector holding singular values.
    std::vector<double> sv_;
  };
  
} // end of RBGen namespace

#endif // RBGEN_ANASAZI_POD_H
