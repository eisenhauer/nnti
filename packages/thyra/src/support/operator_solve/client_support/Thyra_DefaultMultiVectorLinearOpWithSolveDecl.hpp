// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
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
// ***********************************************************************
// @HEADER

#ifndef THYRA_MULTI_VECTOR_LINEAR_OP_WITH_SOLVE_DECL_HPP
#define THYRA_MULTI_VECTOR_LINEAR_OP_WITH_SOLVE_DECL_HPP


#include "Thyra_DefaultDiagonalLinearOpDecl.hpp"
#include "Thyra_LinearOpWithSolveBaseDecl.hpp"
#include "Thyra_SingleRhsLinearOpWithSolveBaseDecl.hpp"
#include "Thyra_DefaultMultiVectorProductVectorSpaceDecl.hpp"
#include "Teuchos_ConstNonconstObjectContainer.hpp"


namespace Thyra {


/** \brief Implicit concrete <tt>LinearOpWithSolveBase</tt> subclass that
 * takes a flattended out multi-vector and performs a multi-RHS solve with it.
 *
 * ToDo: Finish documentation!
 */
template<class Scalar>
class DefaultMultiVectorLinearOpWithSolve
  : virtual public LinearOpWithSolveBase<Scalar>, // Public interface
    virtual protected SingleRhsLinearOpWithSolveBase<Scalar> // Implementation detail
    // 2007/05/24: rabartl: ToDo: We could derive this class just from
    // SingleScalarLinearOpWithSolveBase but then that would require that we
    // have a multi-vector form Thyra::MultiVectorProductMultiVector which
    // does not exist yet and is not needed at this point.
{
public:

  /** @name Constructors/initializers/accessors */
  //@{

  /** \brief Construct to uninitialized. */
  DefaultMultiVectorLinearOpWithSolve();

  /** \brief . */
  void initialize(
    const Teuchos::RefCountPtr<LinearOpWithSolveBase<Scalar> > &lows,
    const Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecRange,
    const Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecDomain
    );

  /** \brief . */
  void initialize(
    const Teuchos::RefCountPtr<const LinearOpWithSolveBase<Scalar> > &lows,
    const Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecRange,
    const Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecDomain
    );
  
  /** \brief . */
  Teuchos::RefCountPtr<LinearOpWithSolveBase<Scalar> >
  getNonconstLinearOpWithSolve();
  
  /** \brief . */
  Teuchos::RefCountPtr<const LinearOpWithSolveBase<Scalar> >
  getLinearOpWithSolve() const;

  // 2007/05/24: rabartl: ToDo: Add a const version of the above function once
  // needed

  /** \brief . */
  void uninitialize();

  //@}

  /** @name Overridden from LinearOpBase */
  //@{

  /** \brief . */
  Teuchos::RefCountPtr< const VectorSpaceBase<Scalar> > range() const;

  /** \brief . */
  Teuchos::RefCountPtr< const VectorSpaceBase<Scalar> > domain() const;

  /** \brief . */
  Teuchos::RefCountPtr<const LinearOpBase<Scalar> > clone() const;

  //@}

protected:

  /** @name Overridden from SingleScalarLinearOpBase */
  //@{

  /** \brief . */
  bool opSupported(ETransp M_trans) const;

  //@}

  /** @name Overridden from SingleRhsLinearOpBase */
  //@{

  /** \brief . */
  void apply(
    const ETransp M_trans,
    const VectorBase<Scalar> &x,
    VectorBase<Scalar> *y,
    const Scalar alpha,
    const Scalar beta
    ) const;

  //@}

  /** @name Overridden from SingleScalarLinearOpWithSolveBase */
  //@{

  /** \brief . */
  bool solveSupportsTrans(ETransp M_trans) const;

  /** \brief . */
  bool solveSupportsSolveMeasureType(
    ETransp M_trans, const SolveMeasureType& solveMeasureType
    ) const;

  //@}

  /** @name Overridden from SingleRhsLinearOpWithSolveBase */
  //@{

  /** \brief . */
  SolveStatus<Scalar> solve(
    const ETransp M_trans,
    const VectorBase<Scalar> &b,
    VectorBase<Scalar> *x,
    const SolveCriteria<Scalar> *solveCriteria
    ) const;

  //@}

private:

  // //////////////////////////////
  // Private types

  typedef Teuchos::ConstNonconstObjectContainer<LinearOpWithSolveBase<Scalar> > CNLOWS;

  // //////////////////////////////
  // Private data members

  CNLOWS lows_;
  Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > multiVecRange_;
  Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > multiVecDomain_;

  // //////////////////////////////
  // Private member functions

  static void validateInitialize(
    const Teuchos::RefCountPtr<const LinearOpWithSolveBase<Scalar> > &lows,
    const Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecRange,
    const Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecDomain
    );
  

};


/** \brief Nonmember constructor function.
 *
 * \relates DefaultMultiVectorLinearOpWithSolve
 */
template<class Scalar>
Teuchos::RefCountPtr<DefaultMultiVectorLinearOpWithSolve<Scalar> >
multiVectorLinearOpWithSolve()
{
  return Teuchos::rcp(new DefaultMultiVectorLinearOpWithSolve<Scalar>());
}


/** \brief Nonmember constructor function.
 *
 * \relates DefaultMultiVectorLinearOpWithSolve
 */
template<class Scalar>
Teuchos::RefCountPtr<DefaultMultiVectorLinearOpWithSolve<Scalar> >
multiVectorLinearOpWithSolve(
  const Teuchos::RefCountPtr<LinearOpWithSolveBase<Scalar> > &lows,
  const Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecRange,
  const Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecDomain
  )
{
  Teuchos::RefCountPtr<DefaultMultiVectorLinearOpWithSolve<Scalar> >
    mvlows = Teuchos::rcp(new DefaultMultiVectorLinearOpWithSolve<Scalar>());
  mvlows->initialize(lows,multiVecRange,multiVecDomain);
  return mvlows;
}


/** \brief Nonmember constructor function.
 *
 * \relates DefaultMultiVectorLinearOpWithSolve
 */
template<class Scalar>
Teuchos::RefCountPtr<DefaultMultiVectorLinearOpWithSolve<Scalar> >
multiVectorLinearOpWithSolve(
  const Teuchos::RefCountPtr<const LinearOpWithSolveBase<Scalar> > &lows,
  const Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecRange,
  const Teuchos::RefCountPtr<const DefaultMultiVectorProductVectorSpace<Scalar> > &multiVecDomain
  )
{
  Teuchos::RefCountPtr<DefaultMultiVectorLinearOpWithSolve<Scalar> >
    mvlows = Teuchos::rcp(new DefaultMultiVectorLinearOpWithSolve<Scalar>());
  mvlows->initialize(lows,multiVecRange,multiVecDomain);
  return mvlows;
}


}	// end namespace Thyra


#endif	// THYRA_MULTI_VECTOR_LINEAR_OP_WITH_SOLVE_DECL_HPP
