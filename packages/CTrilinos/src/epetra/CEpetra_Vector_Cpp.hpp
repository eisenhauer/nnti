#ifndef CEPETRA_VECTOR_CPP_HPP
#define CEPETRA_VECTOR_CPP_HPP

/*! @HEADER */
/*
************************************************************************

                CTrilinos:  C interface to Trilinos
                Copyright (2009) Sandia Corporation

Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
license for use of this work by or on behalf of the U.S. Government.

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA
Questions? Contact M. Nicole Lemaster (mnlemas@sandia.gov)

************************************************************************
*/
/*! @HEADER */


#include "CTrilinos_config.h"

#include "CTrilinos_enums.h"
#include "Teuchos_RCP.hpp"
#include "Epetra_Vector.h"


namespace CEpetra {


using Teuchos::RCP;


/*! get Epetra_Vector from non-const table using CT_Epetra_Vector_ID */
const RCP<Epetra_Vector>
getVector( CT_Epetra_Vector_ID_t id );

/*! get Epetra_Vector from non-const table using CTrilinos_Universal_ID_t */
const RCP<Epetra_Vector>
getVector( CTrilinos_Universal_ID_t id );

/*! get const Epetra_Vector from either the const or non-const table
 * using CT_Epetra_Vector_ID */
const RCP<const Epetra_Vector>
getConstVector( CT_Epetra_Vector_ID_t id );

/*! get const Epetra_Vector from either the const or non-const table
 * using CTrilinos_Universal_ID_t */
const RCP<const Epetra_Vector>
getConstVector( CTrilinos_Universal_ID_t id );

/*! store Epetra_Vector (owned) in non-const table */
CT_Epetra_Vector_ID_t
storeNewVector( Epetra_Vector *pobj );

/*! store Epetra_Vector in non-const table */
CT_Epetra_Vector_ID_t
storeVector( Epetra_Vector *pobj );

/*! store const Epetra_Vector in const table */
CT_Epetra_Vector_ID_t
storeConstVector( const Epetra_Vector *pobj );

/* remove Epetra_Vector from table using CT_Epetra_Vector_ID */
void
removeVector( CT_Epetra_Vector_ID_t *id );

/* remove Epetra_Vector from table using CTrilinos_Universal_ID_t */
void
removeVector( CTrilinos_Universal_ID_t *aid );

/* purge Epetra_Vector table */
void
purgeVector(  );

/* store Epetra_Vector in non-const table */
CTrilinos_Universal_ID_t
aliasVector( const Teuchos::RCP< Epetra_Vector > & robj );

/* store const Epetra_Vector in const table */
CTrilinos_Universal_ID_t
aliasConstVector( const Teuchos::RCP< const Epetra_Vector > & robj );

} // namespace CEpetra


#endif // CEPETRA_VECTOR_CPP_HPP


