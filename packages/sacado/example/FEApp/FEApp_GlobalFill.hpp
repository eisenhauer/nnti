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

#ifndef FEAPP_GLOBALFILL_HPP
#define FEAPP_GLOBALFILL_HPP

#include <vector>

#include "Teuchos_RefCountPtr.hpp"

#include "FEApp_AbstractPDE.hpp"
#include "FEApp_AbstractQuadrature.hpp"
#include "FEApp_NodeBC.hpp"
#include "FEApp_Mesh.hpp"
#include "FEApp_AbstractInitPostOp.hpp"

namespace FEApp {

  template <typename ScalarT>
  class GlobalFill {
  public:
    
    //! Constructor
    GlobalFill(
      const Teuchos::RefCountPtr<const FEApp::Mesh>& elementMesh,
      const Teuchos::RefCountPtr<const FEApp::AbstractQuadrature>& quadRule,
      const Teuchos::RefCountPtr< FEApp::AbstractPDE<ScalarT> >& pdeEquations,
      const std::vector< Teuchos::RefCountPtr<FEApp::NodeBC> >& nodeBCs,
      bool is_transient);
  
    //! Destructor
    ~GlobalFill();

    //! Compute global fill
    void computeGlobalFill(FEApp::AbstractInitPostOp<ScalarT>& initPostOp);

  private:

    //! Private to prohibit copying
    GlobalFill(const GlobalFill&);

    //! Private to prohibit copying
    GlobalFill& operator=(const GlobalFill&);

  protected:
    
    //! Element mesh
    Teuchos::RefCountPtr<const FEApp::Mesh> mesh;

    //! Quadrature rule
    Teuchos::RefCountPtr<const FEApp::AbstractQuadrature> quad;

    //! PDE Equations
    Teuchos::RefCountPtr< FEApp::AbstractPDE<ScalarT> > pde;

    //! Node boundary conditions
    std::vector< Teuchos::RefCountPtr<FEApp::NodeBC> > bc;

    //! Are we transient?
    bool transient;

    //! Number of nodes per element
    unsigned int nnode;

    //! Number of PDE equations
    unsigned int neqn;

    //! Number of element-level DOF
    unsigned int ndof;

    //! Element solution variables
    std::vector<ScalarT> elem_x;

    //! Element time derivative variables
    std::vector<ScalarT>* elem_xdot;

    //! Element residual variables
    std::vector<ScalarT> elem_f;

    //! Node solution variables
    std::vector<ScalarT> node_x;

    //! Node time derivative variables
    std::vector<ScalarT>* node_xdot;

    //! Node residual variables
    std::vector<ScalarT> node_f;

  };

}

// Include implementation
#ifndef SACADO_ETI
#include "FEApp_GlobalFillImpl.hpp"
#endif 

#endif // FEAPP_GLOBALFILL_HPP
