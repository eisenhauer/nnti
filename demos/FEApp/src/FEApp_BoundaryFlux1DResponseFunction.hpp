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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
// USA
// Questions? Contact David M. Gay (dmgay@sandia.gov) or Eric T. Phipps
// (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#ifndef FEAPP_BOUNDARYFLUX1DRESPONSEFUNCTION_HPP
#define FEAPP_BOUNDARYFLUX1DRESPONSEFUNCTION_HPP

#include "FEApp_AbstractResponseFunction.hpp"
#include "Epetra_Map.h"
#include "Epetra_Import.h"
#include "Epetra_Vector.h"

namespace FEApp {

  /*!
   * \brief Reponse function representing the flux out of the boundary for
   * a 1D problem.
   */
  class BoundaryFlux1DResponseFunction : public AbstractResponseFunction {
  public:
  
    //! Default constructor
    BoundaryFlux1DResponseFunction(unsigned int left_gid,
                                   unsigned int right_gid,
                                   unsigned int eqn,
                                   unsigned int num_eqns,
                                   double grid_spacing,
                                   const Epetra_Map& dofMap);

    //! Destructor
    virtual ~BoundaryFlux1DResponseFunction();

    //! Get the number of responses
    virtual unsigned int numResponses() const;

    //! Evaluate responses
    virtual void 
    evaluateResponses(const Epetra_Vector* xdot,
                      const Epetra_Vector& x,
                      const Teuchos::Array<ParamVec>& p,
                      Epetra_Vector& g);

    //! Evaluate tangent = dg/dx*dx/dp + dg/dxdot*dxdot/dp + dg/dp
    virtual void 
    evaluateTangents(
	   const Epetra_Vector* xdot,
	   const Epetra_Vector& x,
	   const Teuchos::Array<ParamVec>& p,
	   const Teuchos::Array< Teuchos::RCP<ParamVec> >& deriv_p,
	   const Teuchos::Array< Teuchos::RCP<Epetra_MultiVector> >& dxdot_dp,
	   const Teuchos::Array< Teuchos::RCP<Epetra_MultiVector> >& dx_dp,
	   Epetra_Vector* g,
	   const Teuchos::Array< Teuchos::RCP<Epetra_MultiVector> >& gt);

    //! Evaluate gradient = dg/dx, dg/dxdot, dg/dp
    virtual void 
    evaluateGradients(
	  const Epetra_Vector* xdot,
	  const Epetra_Vector& x,
	  const Teuchos::Array<ParamVec>& p,
	  const Teuchos::Array< Teuchos::RCP<ParamVec> >& deriv_p,
	  Epetra_Vector* g,
	  Epetra_MultiVector* dg_dx,
	  Epetra_MultiVector* dg_dxdot,
	  const Teuchos::Array< Teuchos::RCP<Epetra_MultiVector> >& dg_dp);

    //! Initialize SG expansion data
    virtual void init_sg(
      const Teuchos::RCP<const Stokhos::OrthogPolyBasis<int,double> >& sg_basis,
      const Teuchos::RCP<const Stokhos::Quadrature<int,double> >& sg_quad,
      const Teuchos::RCP<Stokhos::OrthogPolyExpansion<int,double> >& sg_exp) {}

    //! Evaluate stochastic Galerkin responses
    virtual void 
    evaluateSGResponses(
      const Stokhos::EpetraVectorOrthogPoly* sg_xdot,
      const Stokhos::EpetraVectorOrthogPoly& sg_x,
      const Teuchos::Array<ParamVec>& p,
      const Teuchos::Array<int>& sg_p_index,
      const Teuchos::Array< Teuchos::Array<SGType> >& sg_p_vals,
      Stokhos::EpetraVectorOrthogPoly& sg_g);

    //! Evaluate SG tangent = dg/dx*dx/dp + dg/dxdot*dxdot/dp + dg/dp
    virtual void 
    evaluateSGTangents(
      const Stokhos::EpetraVectorOrthogPoly* sg_xdot,
      const Stokhos::EpetraVectorOrthogPoly& sg_x,
      const Teuchos::Array<ParamVec>& p,
      const Teuchos::Array<int>& sg_p_index,
      const Teuchos::Array< Teuchos::Array<SGType> >& sg_p_vals,
      const Teuchos::Array< Teuchos::RCP<ParamVec> >& deriv_p,
      const Teuchos::Array< Teuchos::RCP<Epetra_MultiVector> >& dxdot_dp,
      const Teuchos::Array< Teuchos::RCP<Epetra_MultiVector> >& dx_dp,
      Stokhos::EpetraVectorOrthogPoly* sg_g,
      const Teuchos::Array< Teuchos::RCP<Stokhos::EpetraMultiVectorOrthogPoly > >& sg_gt);

    //! Evaluate SG gradient = dg/dx, dg/dxdot, dg/dp
    virtual void 
    evaluateSGGradients(
      const Stokhos::EpetraVectorOrthogPoly* sg_xdot,
      const Stokhos::EpetraVectorOrthogPoly& sg_x,
      const Teuchos::Array<ParamVec>& p,
      const Teuchos::Array<int>& sg_p_index,
      const Teuchos::Array< Teuchos::Array<SGType> >& sg_p_vals,
      const Teuchos::Array< Teuchos::RCP<ParamVec> >& deriv_p,
      Stokhos::EpetraVectorOrthogPoly* sg_g,
      Stokhos::EpetraMultiVectorOrthogPoly* sg_dg_dx,
      Stokhos::EpetraMultiVectorOrthogPoly* sg_dg_dxdot,
      const Teuchos::Array< Teuchos::RCP<Stokhos::EpetraMultiVectorOrthogPoly > >& sg_dg_dp);

  private:

    //! Private to prohibit copying
    BoundaryFlux1DResponseFunction(const BoundaryFlux1DResponseFunction&);
    
    //! Private to prohibit copying
    BoundaryFlux1DResponseFunction& operator=(const BoundaryFlux1DResponseFunction&);

  protected:

    //! Grid spacing
    double grid_spacing;

    //! Map for bringint in boundary values
    Epetra_Map *boundaryMap;

    //! Importer for bringing in boundary values
    Epetra_Import *importer;

    //! Vector storing bounary values
    Epetra_Vector *bv;

  };

}

#endif // FEAPP_BOUNDARYFLUX1DRESPONSEFUNCTION_HPP
