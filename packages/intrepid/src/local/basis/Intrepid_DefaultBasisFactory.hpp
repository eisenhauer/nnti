// @HEADER
// ************************************************************************
//
//                           Intrepid Package
//                 Copyright (2007) Sandia Corporation
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
// Questions? Contact Pavel Bochev (pbboche@sandia.gov) or
//                    Denis Ridzal (dridzal@sandia.gov).
//
// ************************************************************************
// @HEADER

/** \file   Intrepid_DefaultBasisFactory.hpp
\brief  Header file for the abstract base class Intrepid::DefaultBasisFactory.
\author Created by P. Bochev and D. Ridzal.
*/

#ifndef INTREPID_DEFAULT_BASIS_FACTORY_HPP
#define INTREPID_DEFAULT_BASIS_FACTORY_HPP

#include "Intrepid_ConfigDefs.hpp"
#include "Intrepid_Basis.hpp"
#include "Teuchos_RCP.hpp"

/////   list of default basis includes   /////

#include "Intrepid_F0_QUAD_I1_FEM_DEFAULT.hpp"
#include "Intrepid_F0_TRI_C1_FEM_DEFAULT.hpp"
#include "Intrepid_F0_TRI_C2_FEM_DEFAULT.hpp"

///// end of list of default basis includes /////

///// FIAT-generated element includes here
#include "Intrepid_F0_TRI_C1_FEM_FIAT.hpp"
#include "Intrepid_F0_TRI_C2_FEM_FIAT.hpp"
#include "Intrepid_F0_TRI_C3_FEM_FIAT.hpp"
#include "Intrepid_F0_TRI_C4_FEM_FIAT.hpp"
#include "Intrepid_F0_TRI_C5_FEM_FIAT.hpp"
#include "Intrepid_F0_TRI_C6_FEM_FIAT.hpp"
#include "Intrepid_F0_TRI_C7_FEM_FIAT.hpp"
#include "Intrepid_F0_TRI_C8_FEM_FIAT.hpp"
#include "Intrepid_F0_TRI_C9_FEM_FIAT.hpp"
#include "Intrepid_F0_TRI_C10_FEM_FIAT.hpp"
///// end FIAT-generated element includes


namespace Intrepid {
  
/** \class Intrepid::DefaultBasisFactory
    \brief A factory class that generates specific instances of bases.
*/
template<class Scalar>
class DefaultBasisFactory {
  private:
    std::map<unsigned long, Teuchos::RCP<Basis<Scalar> > > basisMap_;

  public:
    
  /** \brief Default constructor.
  */
  DefaultBasisFactory() {
    /* List all basis keys with corresponding basis classes.
       Legend:
       F_ - two-digit field code (see EField) - UPPER LIMIT is 41 due to unsigned long integer standard!
       C_ - two-digit cell code (see ECell)
       R  - one-digit reconstruction space code (see EReconstructionSpace)
       D_ - two-digit degree
       B  - two-digit basis code (see EBasis)
       S  - one-digit coordinate system code (see ECoordinates)
       If any of the leading codes are zero (00 or 0), must leave blank!
    */

    /**** F_C_RD_B_S ************************************************************/
    basisMap_[   2001000] = Teuchos::rcp( new Basis_F0_TRI_C1_FEM_DEFAULT<Scalar>() );
    basisMap_[   2002000] = Teuchos::rcp( new Basis_F0_TRI_C2_FEM_DEFAULT<Scalar>() );
    basisMap_[   3101000] = Teuchos::rcp( new Basis_F0_QUAD_I1_FEM_DEFAULT<Scalar>() );

    // begin FIAT-generated bases
    basisMap_[   2002020] = Teuchos::rcp( new Basis_F0_TRI_C2_FEM_FIAT<Scalar>() );
    basisMap_[   2001020] = Teuchos::rcp( new Basis_F0_TRI_C1_FEM_FIAT<Scalar>() );
    basisMap_[   2005020] = Teuchos::rcp( new Basis_F0_TRI_C5_FEM_FIAT<Scalar>() );
    basisMap_[   2004020] = Teuchos::rcp( new Basis_F0_TRI_C4_FEM_FIAT<Scalar>() );
    basisMap_[   2003020] = Teuchos::rcp( new Basis_F0_TRI_C3_FEM_FIAT<Scalar>() );
    basisMap_[   2008020] = Teuchos::rcp( new Basis_F0_TRI_C8_FEM_FIAT<Scalar>() );
    basisMap_[   2007020] = Teuchos::rcp( new Basis_F0_TRI_C7_FEM_FIAT<Scalar>() );
    basisMap_[   2006020] = Teuchos::rcp( new Basis_F0_TRI_C6_FEM_FIAT<Scalar>() );
    basisMap_[   2010020] = Teuchos::rcp( new Basis_F0_TRI_C10_FEM_FIAT<Scalar>() );
    basisMap_[   2009020] = Teuchos::rcp( new Basis_F0_TRI_C9_FEM_FIAT<Scalar>() );
    // end FIAT-generated bases

  };

  /** \brief Destructor.
  */
  virtual ~DefaultBasisFactory() {};

  /** \brief Factory method.

      \param field       [in]    - Field type (FIELD_FORM_0, etc.).
      \param cell        [in]    - Cell type (CELL_TRI, CELL_QUAD, etc.).
      \param recSpace    [in]    - Reconstruction space type (RECONSTRUCTION_SPACE_COMPLETE, etc.).
      \param degree      [in]    - Polynomial degree.
      \param basisType   [in]    - Basis type (BASIS_FEM_DEFAULT, etc.).
      \param coordSys    [in]    - Coordinate system (COORDINATES_CARTESIAN, etc.).

      \return
              - RCP to basis with given specifications.
  */
  Teuchos::RCP<Basis<Scalar> > create(EField field,
                                      ECell cell,
                                      EReconstructionSpace recSpace,
                                      int degree,
                                      EBasis basisType, 
                                      ECoordinates coordSys);
    
};
  
}// namespace Intrepid

#include "Intrepid_DefaultBasisFactoryDef.hpp"

#endif








































































































