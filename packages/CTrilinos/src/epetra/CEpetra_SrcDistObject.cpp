
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

#include "CEpetra_BlockMap_Cpp.hpp"
#include "CEpetra_SrcDistObject_Cpp.hpp"
#include "CEpetra_SrcDistObject.h"
#include "Epetra_SrcDistObject.h"
#include "Teuchos_RCP.hpp"
#include "CTrilinos_enums.h"
#include "CTrilinos_utils.hpp"
#include "CTrilinos_utils_templ.hpp"
#include "CTrilinos_TableRepos.hpp"


//
// Definitions from CEpetra_SrcDistObject.h
//


extern "C" {


CT_Epetra_SrcDistObject_ID_t Epetra_SrcDistObject_Degeneralize ( 
  CTrilinos_Universal_ID_t id )
{
    return CTrilinos::concreteType<CT_Epetra_SrcDistObject_ID_t>(id);
}

CTrilinos_Universal_ID_t Epetra_SrcDistObject_Generalize ( 
  CT_Epetra_SrcDistObject_ID_t id )
{
    return CTrilinos::abstractType<CT_Epetra_SrcDistObject_ID_t>(id);
}

void Epetra_SrcDistObject_Destroy ( 
  CT_Epetra_SrcDistObject_ID_t * selfID )
{
    CEpetra::removeSrcDistObject(selfID);
}

CT_Epetra_BlockMap_ID_t Epetra_SrcDistObject_Map ( 
  CT_Epetra_SrcDistObject_ID_t selfID )
{
    return CEpetra::storeConstBlockMap(&( CEpetra::getConstSrcDistObject(
        selfID)->Map() ));
}


} // extern "C"


//
// Definitions from CEpetra_SrcDistObject_Cpp.hpp
//


/* get Epetra_SrcDistObject from non-const table using CT_Epetra_SrcDistObject_ID */
const Teuchos::RCP<Epetra_SrcDistObject>
CEpetra::getSrcDistObject( CT_Epetra_SrcDistObject_ID_t id )
{
    return CTrilinos::tableRepos().get<Epetra_SrcDistObject>(
        CTrilinos::abstractType<CT_Epetra_SrcDistObject_ID_t>(id));
}

/* get Epetra_SrcDistObject from non-const table using CTrilinos_Universal_ID_t */
const Teuchos::RCP<Epetra_SrcDistObject>
CEpetra::getSrcDistObject( CTrilinos_Universal_ID_t id )
{
    return CTrilinos::tableRepos().get<Epetra_SrcDistObject>(id);
}

/* get const Epetra_SrcDistObject from either the const or non-const table
 * using CT_Epetra_SrcDistObject_ID */
const Teuchos::RCP<const Epetra_SrcDistObject>
CEpetra::getConstSrcDistObject( CT_Epetra_SrcDistObject_ID_t id )
{
    return CTrilinos::tableRepos().get<Epetra_SrcDistObject>(
        CTrilinos::abstractType<CT_Epetra_SrcDistObject_ID_t>(id));
}

/* get const Epetra_SrcDistObject from either the const or non-const table
 * using CTrilinos_Universal_ID_t */
const Teuchos::RCP<const Epetra_SrcDistObject>
CEpetra::getConstSrcDistObject( CTrilinos_Universal_ID_t id )
{
    return CTrilinos::tableRepos().getConst<Epetra_SrcDistObject>(id);
}

/* store Epetra_SrcDistObject (owned) in non-const table */
CT_Epetra_SrcDistObject_ID_t
CEpetra::storeNewSrcDistObject( Epetra_SrcDistObject *pobj )
{
    return CTrilinos::concreteType<CT_Epetra_SrcDistObject_ID_t>(
        CTrilinos::tableRepos().store<Epetra_SrcDistObject>(pobj, true));
}

/* store Epetra_SrcDistObject in non-const table */
CT_Epetra_SrcDistObject_ID_t
CEpetra::storeSrcDistObject( Epetra_SrcDistObject *pobj )
{
    return CTrilinos::concreteType<CT_Epetra_SrcDistObject_ID_t>(
        CTrilinos::tableRepos().store<Epetra_SrcDistObject>(pobj, false));
}

/* store const Epetra_SrcDistObject in const table */
CT_Epetra_SrcDistObject_ID_t
CEpetra::storeConstSrcDistObject( const Epetra_SrcDistObject *pobj )
{
    return CTrilinos::concreteType<CT_Epetra_SrcDistObject_ID_t>(
        CTrilinos::tableRepos().store<Epetra_SrcDistObject>(pobj, false));
}

/* remove Epetra_SrcDistObject from table using CT_Epetra_SrcDistObject_ID */
void
CEpetra::removeSrcDistObject( CT_Epetra_SrcDistObject_ID_t *id )
{
    CTrilinos_Universal_ID_t aid = 
        CTrilinos::abstractType<CT_Epetra_SrcDistObject_ID_t>(*id);
    CTrilinos::tableRepos().remove(&aid);
    *id = CTrilinos::concreteType<CT_Epetra_SrcDistObject_ID_t>(aid);
}

/* purge Epetra_SrcDistObject table */
void
CEpetra::purgeSrcDistObject(  )
{
    CTrilinos::tableRepos().purge<Epetra_SrcDistObject>();
}



