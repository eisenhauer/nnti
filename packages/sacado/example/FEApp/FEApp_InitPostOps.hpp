// $Id$ 
// $Source$ 
// @HEADER
// ***********************************************************************
// 
//                           Sacado Package
//                 Copyright (2006) Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
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
// Questions? Contact David M. Gay (dmgay@sandia.gov) or Eric T. Phipps
// (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#ifndef FEAPP_INITPOSTOPS_HPP
#define FEAPP_INITPOSTOPS_HPP

#include "Teuchos_RefCountPtr.hpp"
#include "Epetra_Vector.h"
#include "Epetra_CrsMatrix.h"

#include "FEApp_AbstractInitPostOp.hpp"

#include "Sacado_Fad_DFad.hpp"

namespace FEApp {

  //! Fill operator for residual
  class ResidualOp : public FEApp::AbstractInitPostOp<double> {
  public:
    
    //! Constructor
    /*!
     * Set xdot to Teuchos::null for steady-state problems
     */
    ResidualOp(
	    const Teuchos::RefCountPtr<const Epetra_Vector>& overlapped_xdot,
	    const Teuchos::RefCountPtr<const Epetra_Vector>& overlapped_x,
	    const Teuchos::RefCountPtr<Epetra_Vector>& overlapped_f);

    //! Destructor
    virtual ~ResidualOp();
    
    //! Evaulate element init operator
    virtual void elementInit(const FEApp::AbstractElement& e,
			     unsigned int neqn,
			     std::vector<double>* elem_xdot,
			     std::vector<double>& elem_x);

    //! Evaluate element post operator
    virtual void elementPost(const FEApp::AbstractElement& e,
			     unsigned int neqn,
			     std::vector<double>& elem_f);

    //! Evaulate node init operator
    virtual void nodeInit(const FEApp::NodeBC& bc,
			  unsigned int neqn,
			  std::vector<double>* node_xdot,
			  std::vector<double>& node_x);

    //! Evaluate node post operator
    virtual void nodePost(const FEApp::NodeBC& bc,
			  unsigned int neqn,
			  std::vector<double>& node_f);

  private:
    
    //! Private to prohibit copying
    ResidualOp(const ResidualOp&);

    //! Private to prohibit copying
    ResidualOp& operator=(const ResidualOp&);

  protected:

    //! Time derivative vector (may be null)
    Teuchos::RefCountPtr<const Epetra_Vector> xdot;

    //! Solution vector
    Teuchos::RefCountPtr<const Epetra_Vector> x;

    //! Residual vector
    Teuchos::RefCountPtr<Epetra_Vector> f;

  };

  //! Fill operator for Jacobian
  class JacobianOp : 
    public FEApp::AbstractInitPostOp< Sacado::Fad::DFad<double> > {
  public:

    //! Constructor
    /*!
     * Set xdot to Teuchos::null for steady-state problems
     */
    JacobianOp(
	    double alpha, double beta,
	    const Teuchos::RefCountPtr<const Epetra_Vector>& overlapped_xdot,
	    const Teuchos::RefCountPtr<const Epetra_Vector>& overlapped_x,
	    const Teuchos::RefCountPtr<Epetra_Vector>& overlapped_f,
	    const Teuchos::RefCountPtr<Epetra_CrsMatrix>& overlapped_jac);

    //! Destructor
    virtual ~JacobianOp();

    //! Evaulate element init operator
    virtual void elementInit(
			 const FEApp::AbstractElement& e,
			 unsigned int neqn,
			 std::vector< Sacado::Fad::DFad<double> >* elem_xdot,
			 std::vector< Sacado::Fad::DFad<double> >& elem_x);

    //! Evaluate element post operator
    virtual void elementPost(const FEApp::AbstractElement& e,
			     unsigned int neqn,
			     std::vector< Sacado::Fad::DFad<double> >& elem_f);

    //! Evaulate node init operator
    virtual void nodeInit(const FEApp::NodeBC& bc,
			  unsigned int neqn,
			  std::vector< Sacado::Fad::DFad<double> >* node_xdot,
			  std::vector< Sacado::Fad::DFad<double> >& node_x);

    //! Evaluate node post operator
    virtual void nodePost(const FEApp::NodeBC& bc,
			  unsigned int neqn,
			  std::vector< Sacado::Fad::DFad<double> >& node_f);

  private:
    
    //! Private to prohibit copying
    JacobianOp(const JacobianOp&);
    
    //! Private to prohibit copying
    JacobianOp& operator=(const JacobianOp&);

  protected:

    //! Coefficient of mass matrix
    double m_coeff;

    //! Coefficient of Jacobian matrix
    double j_coeff;

    //! Time derivative vector (may be null)
    Teuchos::RefCountPtr<const Epetra_Vector> xdot;

    //! Solution vector
    Teuchos::RefCountPtr<const Epetra_Vector> x;

    //! Residual vector
    Teuchos::RefCountPtr<Epetra_Vector> f;

    //! Jacobian matrix
    Teuchos::RefCountPtr<Epetra_CrsMatrix> jac;

  };

}

#endif // FEAPP_INITPOSTOPS_HPP
