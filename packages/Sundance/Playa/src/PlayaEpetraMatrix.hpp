/* @HEADER@ */
//   
 /* @HEADER@ */

#ifndef PLAYA_EPETRAMATRIX_HPP
#define PLAYA_EPETRAMATRIX_HPP

#include "PlayaLoadableMatrix.hpp"
#include "PlayaLinearOperatorDecl.hpp"
#include "PlayaLinearOpWithSpacesDecl.hpp"
#include "PlayaRowAccessibleOp.hpp"
#include "PlayaPrintable.hpp"
#include "PlayaILUFactorizableOp.hpp"
#include "PlayaICCFactorizableOp.hpp"
#include "Epetra_CrsMatrix.h"
#include "PlayaLinearOpWithSpacesDecl.hpp"

namespace Playa
{
using namespace Teuchos;



/** Playa wrapper for epetra matrix */
class EpetraMatrix : public LinearOpWithSpaces<double>,
                     public LoadableMatrix<double>,
                     public RowAccessibleOp<double>,
                     public Printable,
                     public ILUFactorizableOp<double>,
                     public ICCFactorizableOp<double>
{
public:

  /** Construct an empty EpetraMatrix structured according to the graph 
   * argument */
  EpetraMatrix(const Epetra_CrsGraph& graph,
    const VectorSpace<double>& domain,
    const VectorSpace<double>& range);

  /** Wrap an existing Epetra CRS Matrix */
  EpetraMatrix(const RCP<Epetra_CrsMatrix>& mat,
    const VectorSpace<double>& domain,
    const VectorSpace<double>& range);

  /** Apply the operator */
  virtual void apply(
    Teuchos::ETransp applyType,
    const Vector<double>& in,
    Vector<double> out) const ;
  


  /** \name LoadableMatrix interface functions */
  //@{
  /** Insert a set of elements in a row, adding to any previously
   * existing values. 
   * @param globalRowIndex the global index of the row to which these
   * elements belong.
   * @param nElemsToInsert the number of elements being inserted in this
   * step
   * @param globalColumnIndices array of column indices. Must 
   * be nElemsToInsert in length. 
   * @param elements array of element values. Must be nElemsToInsert in
   * length
   */
  virtual void addToRow(int globalRowIndex,
    int nElemsToInsert,
    const int* globalColumnIndices,
    const double* elementValues) ;


  /** 
   * Add to a batch of elements
   */
  virtual void addToElementBatch(int numRows, 
    int rowBlockSize,
    const int* globalRowIndices,
    int numColumnsPerRow,
    const int* globalColumnIndices,
    const double* values,
    const int* skipRow);

  /** Set all elements to zero, preserving the existing structure */
  virtual void zero() ;
  
  //@}

  /** \name incomplete factorization preconditioning interface */
  //@{
  /** create an incomplete factorization. 
   * @param fillLevels number of levels of fill on the local processor
   * @param overlapFill number of levels of fill on remote processors
   * @param relaxationValue fraction of dropped values to be added to the
   * diagonal
   * @param relativeThreshold relative diagonal perutrbation
   * @param absoluteThreshold absolute diagonal perturbation
   * @param leftOrRight whether this preconditioner is to be applied
   * from the left or right 
   * @param rtn newly created preconditioner, returned 
   * by reference argument.
   */
  virtual void getILUKPreconditioner(int fillLevels,
    int overlapFill,
    double relaxationValue,
    double relativeThreshold,
    double absoluteThreshold,
    LeftOrRight leftOrRight,
    Preconditioner<double>& rtn) const ;
  //@}


   /** \name incomplete factorization preconditioning interface */
  //@{
  /** create an incomplete factorization. 
   * @param fillLevels number of levels of fill on the local processor
   * @param overlapFill number of levels of fill on remote processors
   * @param relaxationValue fraction of dropped values to be added to the
   * diagonal
   * @param relativeThreshold relative diagonal perutrbation
   * @param absoluteThreshold absolute diagonal perturbation
   * @param rtn newly created preconditioner, returned 
   * by reference argument.
   */
  virtual void getICCPreconditioner(int fillLevels,
    int overlapFill,
    double relaxationValue,
    double relativeThreshold,
    double absoluteThreshold,
    Preconditioner<double>& rtn) const ;

  
  /** \name Row access interface */
  //@{
  /** Get the specified row as defined by RowAccessible  */
  void getRow(const int& row, 
		Teuchos::Array<int>& indices, 
		Teuchos::Array<double>& values) const;
  //@}
  

  /** \name Diagnostic output */
  //@{
  /** Print the matrix */
  virtual void print(std::ostream& os) const ;
  //@}

  /** */
  std::ostream& describe(
    std::ostream                         &out
    ,const Teuchos::EVerbosityLevel      verbLevel
    ,const std::string                   leadingIndent
    , const std::string                   indentSpacer
    ) const 
    {
      out << leadingIndent << indentSpacer << this->description() << std::endl;
      return out;
    }
  /** */
  std::string description() const ;
  //@}

  /** */
  static Epetra_CrsMatrix& getConcrete(const LinearOperator<double>& A);

  /** */
  static RCP<const Epetra_CrsMatrix> getConcretePtr(const LinearOperator<double>& A);

  /** 
   * Read-only access to the underlying crs matrix. Needed by Ifpack and ML.
   */
  const Epetra_CrsMatrix* crsMatrix() const ;

private:

  Epetra_CrsMatrix* crsMatrix();

  RCP<Epetra_CrsMatrix> matrix_;

  const Epetra_Map& getRangeMap() const;
  const Epetra_Map& getDomainMap() const;
};

  

}

#endif
