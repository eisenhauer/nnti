#include "CTrilinos_config.h"

#include "CEpetra_Object_Cpp.hpp"
#include "CEpetra_Object.h"
#include "Epetra_Object.h"
#include "Teuchos_RCP.hpp"
#include "CTrilinos_enums.h"
#include "CTrilinos_utils.hpp"
#include "CTrilinos_Table.hpp"


namespace {


using Teuchos::RCP;
using CTrilinos::Table;


Table<Epetra_Object>& tableOfObjects()
{
    static Table<Epetra_Object>
        loc_tableOfObjects(CT_Epetra_Object_ID, "CT_Epetra_Object_ID");
    return loc_tableOfObjects;
}


} // namespace


//
// Definitions from CEpetra_Object.h
//


extern "C" {


CT_Epetra_Object_ID_t Epetra_Object_Cast ( 
  CTrilinos_Object_ID_t id )
{
    return CTrilinos::concreteType<CT_Epetra_Object_ID_t>(
        CTrilinos::cast(tableOfObjects(), id));
}

CTrilinos_Object_ID_t Epetra_Object_Abstract ( 
  CT_Epetra_Object_ID_t id )
{
    return CTrilinos::abstractType<CT_Epetra_Object_ID_t>(id);
}

CT_Epetra_Object_ID_t Epetra_Object_Create ( 
  int TracebackModeIn, boolean set_label )
{
    return CTrilinos::concreteType<CT_Epetra_Object_ID_t>(
        tableOfObjects().store(new Epetra_Object(
        TracebackModeIn, set_label)));

}

CT_Epetra_Object_ID_t Epetra_Object_Create_WithLabel ( 
  const char * const Label, int TracebackModeIn )
{
    return CTrilinos::concreteType<CT_Epetra_Object_ID_t>(
        tableOfObjects().store(new Epetra_Object(
        Label, TracebackModeIn)));

}

CT_Epetra_Object_ID_t Epetra_Object_Duplicate ( 
  CT_Epetra_Object_ID_t ObjectID )
{
    const Teuchos::RCP<Epetra_Object> 
        pObject = CEpetra::getObject(ObjectID);

    return CTrilinos::concreteType<CT_Epetra_Object_ID_t>(
        tableOfObjects().store(new Epetra_Object(
        *pObject)));

}

void Epetra_Object_Destroy ( CT_Epetra_Object_ID_t * selfID )
{
    CTrilinos_Object_ID_t id =
        CTrilinos::abstractType<CT_Epetra_Object_ID_t>(*selfID);
    tableOfObjects().remove(&id);
    *selfID = CTrilinos::concreteType<CT_Epetra_Object_ID_t>(id);
}

void Epetra_Object_SetLabel ( 
  CT_Epetra_Object_ID_t selfID, const char * const Label )
{
    CEpetra::getObject(selfID)->SetLabel(Label);
}

const char * Epetra_Object_Label ( CT_Epetra_Object_ID_t selfID )
{
    return CEpetra::getObject(selfID)->Label();
}

int Epetra_Object_ReportError ( 
  CT_Epetra_Object_ID_t selfID, const char * const Message, 
  int ErrorCode )
{
    return CEpetra::getObject(selfID)->ReportError(
        CTrilinos::cptr2str(Message), ErrorCode);
}


} // extern "C"


//
// Definitions from CEpetra_Object_Cpp.hpp
//


const Teuchos::RCP<Epetra_Object>
CEpetra::getObject( CT_Epetra_Object_ID_t id )
{
    return tableOfObjects().get(
        CTrilinos::abstractType<CT_Epetra_Object_ID_t>(id));
}

const Teuchos::RCP<Epetra_Object>
CEpetra::getObject( CTrilinos_Object_ID_t id )
{
    return tableOfObjects().get(id);
}

CT_Epetra_Object_ID_t
CEpetra::storeObject( const Epetra_Object *pobj )
{
    return CTrilinos::concreteType<CT_Epetra_Object_ID_t>(
        tableOfObjects().storeCopy(pobj));
}

void
CEpetra::purgeObjectTable(  )
{
    tableOfObjects().purge();
}



