#ifndef IFPACK_AMESOS_H
#define IFPACK_AMESOS_H

#include "Ifpack_ConfigDefs.h"
#if defined(HAVE_IFPACK_AMESOS) && defined(HAVE_IFPACK_TEUCHOS)

#include "Ifpack_Preconditioner.h"
#include "Epetra_Operator.h"
#include "Teuchos_ParameterList.hpp"
class Epetra_Map;
class Epetra_Time;
class Epetra_Comm;
class Amesos_BaseSolver;
class Epetra_LinearProblem;
class Epetra_RowMatrix;

//! Ifpack_Amesos: a class to use Amesos' factorizations as preconditioners.
/*!
Class Ifpack_Amesos enables the use of Amesos' factorizations as 
Ifpack_Preconditioners.

Ifpack_Amesos is just a bare-bone wrap to Amesos. Currently, the
only parameter required recognized by SetParameters() is
\c "amesos: solver type" (defaulted to \c
"Amesos_Klu"), which defined the Amesos solver. The Teuchos list
in input to SetParameters() is copied, then the copied list is
used to set the parameters of the Amesos object.

This class works with matrices whose communicator contains only one
process, that is, either serial matrices, or Ifpack_LocalFilter'd matrices.

\warning The number of flops is NOT updated.

\author Marzio Sala, SNL 9214.

\date Last update Oct-04.

*/
class Ifpack_Amesos : public Ifpack_Preconditioner {
      
public:

  //@{ \name Constructors/Destructors.

  //! Constructor.
  Ifpack_Amesos(Epetra_RowMatrix* Matrix);

  //! Copy constructor.
  Ifpack_Amesos(const Ifpack_Amesos& rhs);

  //! Operator=.
  Ifpack_Amesos& operator=(const Ifpack_Amesos& rhs);

  //@{ \name Destructor.
  //! Destructor
  virtual ~Ifpack_Amesos();

  //@}

  //@{ \name Atribute set methods.

   //! If set true, transpose of this operator will be applied (not implemented).
    /*! This flag allows the transpose of the given operator to be used 
     * implicitly.  
      
    \param 
	   UseTranspose - (In) If true, multiply by the transpose of operator, 
	   otherwise just use operator.

    \return Integer error code, set to 0 if successful.  Set to -1 if this implementation does not support transpose.
  */

  virtual int SetUseTranspose(bool UseTranspose);
  //@}
  
  //@{ \name Mathematical functions.

    //! Applies the matrix to an Epetra_MultiVector.
  /*! 
    \param
    X - (In) A Epetra_MultiVector of dimension NumVectors to multiply with matrix.
    \param 
    Y - (Out) A Epetra_MultiVector of dimension NumVectors containing the result.

    \return Integer error code, set to 0 if successful.
    */
    virtual int Apply(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

    //! Applies the preconditioner to X, returns the result in Y.
  /*! 
    \param 
    X - (In) A Epetra_MultiVector of dimension NumVectors to be preconditioned.
    \param 
    Y - (Out) A Epetra_MultiVector of dimension NumVectors containing result.

    \return Integer error code, set to 0 if successful.

    \warning In order to work with AztecOO, any implementation of this method 
    must support the case where X and Y are the same object.
    */
    virtual int ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

    //! Returns the infinity norm of the global matrix (not implemented)
    virtual double NormInf() const;
  //@}
  
  //@{ \name Atribute access functions

    //! Returns a character string describing the operator
    virtual const char * Label() const;

    //! Returns the current UseTranspose setting.
    virtual bool UseTranspose() const;

    //! Returns true if the \e this object can provide an approximate Inf-norm, false otherwise.
    virtual bool HasNormInf() const;

    //! Returns a pointer to the Epetra_Comm communicator associated with this operator.
    virtual const Epetra_Comm & Comm() const;

    //! Returns the Epetra_Map object associated with the domain of this operator.
    virtual const Epetra_Map & OperatorDomainMap() const;

    //! Returns the Epetra_Map object associated with the range of this operator.
    virtual const Epetra_Map & OperatorRangeMap() const;

  //@}

  //@{ \name Construction and application methods.
 
  //! Returns \c true is the preconditioner has been successfully initialized.
  virtual bool IsInitialized() const
  {
    return(IsInitialized_);
  }

  //! Initializes the preconditioners.
  /*! \return
   * 0 if successful, 1 if problems occurred.
   */
  virtual int Initialize();

  //! Returns \c true if the preconditioner has been successfully computed.
  virtual bool IsComputed() const
  {
    return(IsComputed_);
  }

  //! Computes the preconditioners.
  /*! \return
   * 0 if successful, 1 if problems occurred.
   */
  virtual int Compute();

  //! Sets all the parameters for the preconditioner.
  /*! Parameters currently supported:
   * - \c "amesos: solver type" : Specifies the solver type
   *   for Amesos. Default: \c Amesos_Klu.
   *
   * The input list will be copied, then passed to the Amesos
   * object through Amesos::SetParameters().
   */   
  virtual int SetParameters(Teuchos::ParameterList& List);

  //@}

  //@{ \name Query methods.

  //! Returns a const reference to the internally stored matrix.
  virtual const Epetra_RowMatrix& Matrix() const
  {
    return(*Matrix_);
  }

  //! Returns the estimated condition number, computes it if necessary.
  virtual double Condest(const Ifpack_CondestType CT = Ifpack_Cheap,
                         const int MaxIters = 1550,
                         const double Tol = 1e-9,
			 Epetra_RowMatrix* Matrix= 0);
  
  //! Returns the estimated condition number, never computes it.
  virtual double Condest() const
  {
    return(Condest_);
  }

  //! Returns the number of calls to Initialize().
  virtual int NumInitialize() const
  {
    return(NumInitialize_);
  }

  //! Returns the number of calls to Compute().
  virtual int NumCompute() const
  {
    return(NumCompute_);
  }

  //! Returns the number of calls to ApplyInverse().
  virtual int NumApplyInverse() const
  {
    return(NumApplyInverse_);
  }

  //! Returns the total time spent in Initialize().
  virtual double InitializeTime() const
  {
    return(InitializeTime_);
  }

  //! Returns the total time spent in Compute().
  virtual double ComputeTime() const
  {
    return(ComputeTime_);
  }

  //! Returns the total time spent in ApplyInverse().
  virtual double ApplyInverseTime() const
  {
    return(ApplyInverseTime_);
  }

  //! Returns the number of flops in the initialization phase.
  virtual double InitializeFlops() const
  {
    return(0.0);
  }

  //! Returns the total number of flops to computate the preconditioner.
  virtual double ComputeFlops() const
  {
    return(ComputeFlops_);
  }

  //! Returns the total number of flops to apply the preconditioner.
  virtual double ApplyInverseFlops() const
  {
    return(ApplyInverseFlops_);
  }

  // Returns a constant reference to the internally stored 
  virtual const Teuchos::ParameterList& List() const 
  {
    return(List_);
  }

  //! Prints on ostream basic information about \c this object.
  virtual std::ostream& Print(std::ostream& os) const;

  //@}

protected:
  
  //@{ \name Methods to get/set private data

  //! Sets the label.
  inline void SetLabel(const char* Label) 
  {
    Label_ = Label;
  }

  //! Sets \c IsInitialized_.
  inline void SetIsInitialized(const bool IsInitialized)
  {
    IsInitialized_ = IsInitialized;
  }

  //! Sets \c IsComputed_.
  inline void SetIsComputed(const int IsComputed)
  {
    IsComputed_ = IsComputed;
  }

  //! Sets \c NumInitialize_.
  inline void SetNumInitialize(const int NumInitialize)
  {
    NumInitialize_ = NumInitialize;
  }

  //! Sets \c NumCompute_.
  inline void SetNumCompute(const int NumCompute)
  {
    NumCompute_ = NumCompute;
  }

  //! Sets \c NumApplyInverse_.
  inline void SetNumApplyInverse(const int NumApplyInverse)
  {
    NumApplyInverse_ = NumApplyInverse;
  }

  //! Sets \c InitializeTime_.
  inline void SetInitializeTime(const double InitializeTime)
  {
    InitializeTime_ = InitializeTime;
  }

  //! Sets \c ComputeTime_.
  inline void SetComputeTime(const double ComputeTime)
  {
    ComputeTime_ = ComputeTime;
  }

  //! Sets \c ApplyInverseTime_.
  inline void SetApplyInverseTime(const double ApplyInverseTime)
  {
    ApplyInverseTime_ = ApplyInverseTime;
  }

  //! Sets \c ComputeFlops_.
  inline void SetComputeFlops(const double ComputeFlops)
  {
    ComputeFlops_ = ComputeFlops;
  }

  //! Sets \c ComputeFlops_.
  inline void SetApplyInverseFlops(const double ApplyInverseFlops)
  {
    ApplyInverseFlops_ = ApplyInverseFlops;
  }

  //! Set \c List_.
  inline void SetList(const Teuchos::ParameterList& List)
  {
    List_ = List;
  }
  //@}
  
private:

  //! Pointers to the matrix to be preconditioned.
  const Epetra_RowMatrix* Matrix_;

  //! Amesos solver, use to apply the inverse of the local matrix.
  Amesos_BaseSolver* Solver_;
  //! Linear problem required by Solver_.
  Epetra_LinearProblem* Problem_;
  //! Contains a copy of the input parameter list.
  Teuchos::ParameterList List_;

  //! Contains the label of \c this object.
  string Label_;
  //! If true, the preconditioner has been successfully initialized.
  bool IsInitialized_;
  //! If true, the preconditioner has been successfully computed.
  bool IsComputed_;

  //! Contains the number of successful calls to Initialize().
  int NumInitialize_;
  //! Contains the number of successful call to Compute().
  int NumCompute_;
  //! Contains the number of successful call to ApplyInverse().
  mutable int NumApplyInverse_;

  //! Contains the time for all successful calls to Initialize().
  double InitializeTime_;
  //! Contains the time for all successful calls to Compute().
  double ComputeTime_;
  //! Contains the time for all successful calls to ApplyInverse().
  mutable double ApplyInverseTime_;
  //! Time object.
  Epetra_Time* Time_;

  //! Contains the number of flops for Compute().
  double ComputeFlops_;
  //! Contain sthe number of flops for ApplyInverse().
  double ApplyInverseFlops_;

  //! Contains the estimated condition number.
  double Condest_;
};

#endif // HAVE_IFPACK_AMESOS && HAVE_IFPAC_TEUCHOS
#endif // IFPACK_AMESOS_H
