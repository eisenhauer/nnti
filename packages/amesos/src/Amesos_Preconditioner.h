#ifndef AMESOS_PRECONDITIONER_H
#define AMESOS_PRECONDITIONER_H

#include "Epetra_Operator.h"
#include "Teuchos_ParameterList.hpp"
class Epetra_Map;
class Epetra_Comm;
class Amesos_BaseSolver;
class Epetra_LinearProblem;
class Epetra_RowMatrix;
class Amesos_LocalRowMatrix;

class Amesos_Preconditioner : public Epetra_Operator {
      
public:

  Amesos_Preconditioner(char* SolverType,
			Epetra_RowMatrix* Matrix,
			const bool LocalizeMatrix = true);

  Amesos_Preconditioner(char* SolverType,
			Epetra_RowMatrix* Matrix,
			Teuchos::ParameterList& List,
			const bool LocalizeMatrix = true);
  //@{ \name Destructor.
  //! Destructor
  virtual ~Amesos_Preconditioner();
  //@}

  //@{ \name Atribute set methods.

    //! If set true, transpose of this operator will be applied.
    /*! This flag allows the transpose of the given operator to be used implicitly.  Setting this flag
        affects only the Apply() and ApplyInverse() methods.  If the implementation of this interface 
	does not support transpose use, this method should return a value of -1.
      
    \param In
	   UseTranspose -If true, multiply by the transpose of operator, otherwise just use operator.

    \return Integer error code, set to 0 if successful.  Set to -1 if this implementation does not support transpose.
  */
    virtual int SetUseTranspose(bool UseTranspose);
  //@}
  
  //@{ \name Mathematical functions.

    //! Returns the result of a Epetra_Operator applied to a Epetra_MultiVector X in Y.
    /*! 
    \param In
	   X - A Epetra_MultiVector of dimension NumVectors to multiply with matrix.
    \param Out
	   Y -A Epetra_MultiVector of dimension NumVectors containing result.

    \return Integer error code, set to 0 if successful.
  */
    virtual int Apply(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

    //! Returns the result of a Epetra_Operator inverse applied to an Epetra_MultiVector X in Y.
    /*! 
    \param In
	   X - A Epetra_MultiVector of dimension NumVectors to solve for.
    \param Out
	   Y -A Epetra_MultiVector of dimension NumVectors containing result.

    \return Integer error code, set to 0 if successful.

    \warning In order to work with AztecOO, any implementation of this method must 
              support the case where X and Y are the same object.
  */
    virtual int ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

    //! Returns the infinity norm of the global matrix.
    /* Returns the quantity \f$ \| A \|_\infty\f$ such that
       \f[\| A \|_\infty = \max_{1\lei\lem} \sum_{j=1}^n |a_{ij}| \f].

       \warning This method must not be called unless HasNormInf() returns true.
    */ 
    virtual double NormInf() const;
  //@}
  
  //@{ \name Atribute access functions

    //! Returns a character string describing the operator
    virtual char * Label() const;

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

    virtual bool IsLocalized() const
    {
      return(IsLocalized_);
    }

private:

    int Amesos_Preconditioner::Compute(char* SolverType,
				       Epetra_RowMatrix* Matrix,
				       Teuchos::ParameterList& List,
				       const bool LocalizeMatrix);

    Epetra_RowMatrix* Matrix_;
    Amesos_LocalRowMatrix* LocalizedMatrix_;
    Amesos_BaseSolver* Solver_;
  Epetra_LinearProblem* Problem_;
  string Label_;
  bool IsLocalized_;

};

#endif /* AMESOS_PRECONDITIONER_H */
