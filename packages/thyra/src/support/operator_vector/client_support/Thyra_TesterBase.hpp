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


#ifndef THYRA_TESTERBASE_HPP
#define THYRA_TESTERBASE_HPP

#include "Thyra_LinearOperatorImpl.hpp"
#include "Thyra_TestSpecifier.hpp"
#include "Teuchos_ScalarTraits.hpp"


namespace Thyra
{
  using Teuchos::RefCountPtr;
  using Teuchos::ScalarTraits;

  /** */
  template <class Scalar>
  class TesterBase 
  {
  public:
    /** \brief Local typedef for promoted scalar magnitude */
    typedef typename ScalarTraits<Scalar>::magnitudeType ScalarMag;

    /** */
    TesterBase(const VectorSpace<Scalar>& space, int nCols,
               Teuchos::RefCountPtr<Teuchos::FancyOStream>& out)
      : space_(space), nCols_(nCols), out_(out) 
    {
      *out << "==========================================================================="
          << endl;
      *out << "       testing on type " 
          << Teuchos::ScalarTraits<Scalar>::name() << endl;
      *out << "==========================================================================="
          << endl;
    }

    /** */
    virtual ~TesterBase(){;}

    /** */
    virtual bool runAllTests() const = 0 ;


    /** */
    bool checkTest(const TestSpecifier<Scalar>& spec,
                   const ScalarMag& err, 
                   const string& testName) const ;

    /** */
    void randomizeVec(Vector<Scalar>& x) const ;

    /** */
    LinearOperator<Scalar> randomDenseOp() const ;

    /** */
    const VectorSpace<Scalar>& space() const {return space_;}

    /** */
    ostream& out() const {return *out_;}
    
  private:
    VectorSpace<Scalar> space_;
    int nCols_;
    mutable Teuchos::RefCountPtr<Teuchos::FancyOStream> out_;
  };

  template <class Scalar> 
  inline void TesterBase<Scalar>
  ::randomizeVec(Vector<Scalar>& x) const
  {
    typedef ScalarTraits<Scalar> ST;
    randomize(Scalar(-ST::one()),Scalar(+ST::one()),x.ptr().get());
    
  }

  template <class Scalar> 
  inline bool TesterBase<Scalar>
  ::checkTest(const TestSpecifier<Scalar>& spec,
              const ScalarMag& err, 
              const string& testName) const 
  {
    bool rtn = true;
    if (err > spec.errorTol())
      {
        *out_ << testName << " test FAILED: err=" << err << ", tol = " 
             << spec.errorTol() << endl;
        rtn = false;
      }
    else if (err > spec.warningTol())
      {
        *out_ << "WARNING: " << testName << " test err="
             << err << " could not beat tol = " 
             << spec.warningTol() << endl;
      }
    else
      {
        *out_ << "test " << testName << " PASSED with tol=" << spec.errorTol() << endl;
      }
    return rtn;
  }

  template <class Scalar>
  inline LinearOperator<Scalar> TesterBase<Scalar>
  ::randomDenseOp() const 
  {
    typedef ScalarTraits<Scalar> ST;
    RefCountPtr<MultiVectorBase<Scalar> > mv = space_.createMembers(nCols_);
    randomize(-ST::one(), ST::one(), &*mv);
    RefCountPtr<LinearOpBase<Scalar> > rtn = mv;
    return rtn;
  }

  
  
}
#endif
