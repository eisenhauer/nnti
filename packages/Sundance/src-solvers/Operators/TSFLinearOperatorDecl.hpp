/* @HEADER@ */
/* ***********************************************************************
// 
//           TSFExtended: Trilinos Solver Framework Extended
//                 Copyright (2004) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// **********************************************************************/
 /* @HEADER@ */

#ifndef TSFLINEAROPERATORDECL_HPP
#define TSFLINEAROPERATORDECL_HPP

#include "TSFConfigDefs.hpp"
#include "TSFHandle.hpp"
#include "TSFHandleable.hpp"
#include "Thyra_LinearOpBase.hpp"
#include "TSFSingleScalarTypeOp.hpp"
#include "TSFLoadableMatrix.hpp"
#include "Teuchos_TimeMonitor.hpp"
#include "TSFRowAccessibleOp.hpp"


namespace TSFExtended
{
  using Thyra::Index;
  using namespace Teuchos;
  using namespace Thyra;

  template <class Scalar>  class LinearSolver;
  template <class Scalar>  class VectorSpace;
  template <class Scalar>  class Vector;
  template <class Scalar>  class VectorType;

  /** 
   * User-level linear operator class
   */
  template <class Scalar>
  class LinearOperator : public Handle<SingleScalarTypeOp<Scalar> >
  {
  public:
    /** \name Constructors, Destructors, and Assignment Operators */
    //@{
    /** Empty constructor*/
    LinearOperator();

    /** Constructor with raw pointer */
    LinearOperator(Handleable<SingleScalarTypeOp<Scalar> >* rawPtr);


    /** Constructor with smart pointer */
    LinearOperator(const RefCountPtr<SingleScalarTypeOp<Scalar> >& smartPtr);
    //@}

    /** Return the domain */
    const VectorSpace<Scalar> domain() const ;

    /** Return the range */
    const VectorSpace<Scalar> range() const ;


    /** 
     * Compute
     * \code
     * out = beta*out + alpha*op*in;
     * \endcode
     **/
    void apply(const Vector<Scalar>& in,
	       Vector<Scalar>& out,
	       const Scalar& alpha = 1.0,
	       const Scalar& beta = 0.0) const ;

    /**  
     * Compute
     * \code
     * out = beta*out + alpha*op^T*in;
     * \endcode
     **/
    void applyTranspose(const Vector<Scalar>& in,
			Vector<Scalar>& out,
			const Scalar& alpha = 1.0,
			const Scalar& beta = 0.0) const ;

    /** 
     * Apply method written in terms of Thyra objects. This simplifies
     * the conversion from TSFCore
     */
    virtual void generalApply(
                              const Thyra::ETransp            M_trans
                              ,const Thyra::VectorBase<Scalar>    &x
                              ,Thyra::VectorBase<Scalar>          *y
                              ,const Scalar            alpha = 1.0
                              ,const Scalar            beta  = 0.0
                              ) const ;


    //       /** For the moment this does nothing*/
    LinearOperator<Scalar> form() const {return *this;}
      
      
    /** Get a stopwatch for timing vector operations */
    RefCountPtr<Time>& opTimer();

    /**
     * Return a TransposeOperator.
     */
    LinearOperator<Scalar> transpose() const ; 

    /**
     * Return an InverseOperator.
     */
    LinearOperator<Scalar> inverse(const LinearSolver<Scalar>& solver = LinearSolver<Scalar>()) const ;

    /** Operator composition */
    LinearOperator<Scalar> operator*(const LinearOperator<Scalar>& other) const ;

    /** Operator sum */
    LinearOperator<Scalar> operator+(const LinearOperator<Scalar>& other) const ;

    /** Return a Loadable Matrix  */
    RefCountPtr<LoadableMatrix<Scalar> > matrix();

    /** Get a row of the underlying matrix */     
    void getRow(const int& row, 
                Teuchos::Array<int>& indices, 
                Teuchos::Array<Scalar>& values) const ;
    

    /** \name  Block operations  */
    //@{
      
    /** return number of block rows */
    int numBlockRows() const;
      

    /** return number of block cols */
    int numBlockCols() const;
      

    /** get the (i,j)-th block */
    LinearOperator<Scalar> getBlock(const int &i, const int &j) const ;

    /** set the (i,j)-th block 
	If the domain and/or the range are not set, then we
	are building the operator
    */
    void setBlock(int i, int j, 
		  const LinearOperator<Scalar>& sub);

    /** Finialize the Block operator 
     *
     *@param zerofill bool to indicate if blocks that are not set
     *                should be automatically set to the zero operator.
     */
    void finalize(bool zerofill);

    //@}


    /** Form a Linear operator in the specified type.  The source
     *LinearOperator must be RowAccexxible and the target must be
     *Loadable.

     * @param type is the VectorType used to specify the type of
     * operator to be formed
     */

    LinearOperator<Scalar> form(const VectorType<Scalar>& type);

      

  private:
  };
}


#endif
