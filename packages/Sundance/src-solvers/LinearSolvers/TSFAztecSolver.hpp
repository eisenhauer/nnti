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

#ifndef TSFAZTECSOLVER_HPP
#define TSFAZTECSOLVER_HPP

#include "TSFConfigDefs.hpp"
#include "TSFLinearSolverBase.hpp"
#include "TSFHandleable.hpp"
#include "TSFPrintable.hpp"
#include "TSFDescribable.hpp"

#include "Teuchos_Array.hpp"
#include "Teuchos_RefCountPtr.hpp"
#include "Teuchos_ParameterList.hpp"
#include <map>

#include "AztecOO.h"

#define HAVE_ML

#define AZ_ml           101
#define AZ_ml_levels    102
#define AZ_ml_sym       103
#define AZ_ml_damping   104
#define AZ_recursive_iterate 105

namespace TSFExtended
{
  using namespace Teuchos;

  /**
   *
   */
  class AztecSolver : public LinearSolverBase<double>,
                      public Handleable<LinearSolverBase<double> >,
                      public Printable,
                      public Describable
  {
  public:
    /** */
    AztecSolver(const Teuchos::map<int, int>& aztecOptions,
                const Teuchos::map<int, double>& aztecParameters);

    /** */
    AztecSolver(const Teuchos::ParameterList& params);

    /** */
    virtual ~AztecSolver(){;}

    /** Change the convergence tolerance. */
    virtual void updateTolerance(const double& tol);

    /** \name Printable interface */
    //@{
    /** Write to a stream  */
    void print(ostream& os) const 
    {
      os << description() << endl;
    }
    //@}
    
    /** \name Describable interface */
    //@{
    /** Write a brief description */
    string description() const {return "AztecSolver";}
    //@}

    

    /** */
    virtual SolverState<double> solve(const LinearOperator<double>& op,
                                      const Vector<double>& rhs,
                                      Vector<double>& soln) const ;

    /** \name Handleable interface */
    //@{
    /** Return a ref count pointer to a newly created object */
    virtual RefCountPtr<LinearSolverBase<double> > getRcp() 
    {return rcp(this);}
    //@}


  protected:

  private:
    
    void setupML(Epetra_RowMatrix* A) const ;

    /** Aztec options */
    mutable Array<int> options_;

    /** Aztec parameters */
    mutable Array<double> parameters_;

    /** Flag indicating whether we are using ML preconditioning */
    bool useML_;

    /** Flag indicating whether we are doing a recursive solve
     *  with aztec (i.e., using recursiveIterate) */
    bool aztec_recursive_iterate_;

    /** Number of ML levels to use */
    mutable int mlLevels_;

    /** whether ML should assume symmetric system */
    bool mlSymmetric_;

    /** whether ML should use damping */
    bool mlUseDamping_;

    /** damping factor for ML */
    double mlDamping_;

    /** ML preconditioner object */
    mutable RefCountPtr<Epetra_Operator> prec_;

    /** Aztec status */
    mutable Array<double> aztec_status;

    /** Aztec proc_config */
    mutable Array<int> aztec_proc_config;


    /** Map from parameter name to AZTEC parameter identifier */
    static Teuchos::map<string,int>& paramMap() 
    {static Teuchos::map<string,int> rtn; return rtn;}
    
    /** Initialize the map from parameter names to AZTEC parameter ID codes */
    static void initParamMap();

    
  };
  
}

#endif
