#include "CTrilinos_config.h"


#ifdef HAVE_CTRILINOS_AMESOS


#include "CAmesos_BaseSolver_Cpp.hpp"
#include "CEpetra_LinearProblem_Cpp.hpp"
#include "CTeuchos_ParameterList_Cpp.hpp"
#include "CAmesos_Cpp.hpp"
#include "CAmesos.h"
#include "Amesos.h"
#include "Teuchos_RCP.hpp"
#include "CTrilinos_enums.h"
#include "CTrilinos_utils_templ.hpp"
#include "CTrilinos_Table.hpp"


namespace {


using Teuchos::RCP;
using CTrilinos::Table;


/* table to hold objects of type Amesos */
Table<Amesos>& tableOfAmesoss()
{
    static Table<Amesos>
        loc_tableOfAmesoss(CT_Amesos_ID, "CT_Amesos_ID", false);
    return loc_tableOfAmesoss;
}

/* table to hold objects of type const Amesos */
Table<const Amesos>& tableOfConstAmesoss()
{
    static Table<const Amesos>
        loc_tableOfConstAmesoss(CT_Amesos_ID, "CT_Amesos_ID", true);
    return loc_tableOfConstAmesoss;
}


} // namespace


//
// Definitions from CAmesos.h
//


extern "C" {


CT_Amesos_ID_t Amesos_Create (  )
{
    return CTrilinos::concreteType<CT_Amesos_ID_t>(
        tableOfAmesoss().store(new Amesos()));
}

void Amesos_Destroy ( CT_Amesos_ID_t * selfID )
{
    CTrilinos_Object_ID_t aid
        = CTrilinos::abstractType<CT_Amesos_ID_t>(*selfID);
    if (selfID->is_const) {
        tableOfConstAmesoss().remove(&aid);
    } else {
        tableOfAmesoss().remove(&aid);
    }
    *selfID = CTrilinos::concreteType<CT_Amesos_ID_t>(aid);
}

CT_Amesos_BaseSolver_ID_t Amesos_CreateSolver ( 
  CT_Amesos_ID_t selfID, const char * ClassType, 
  CT_Epetra_LinearProblem_ID_t LinearProblemID )
{
    return CAmesos::storeBaseSolver(CAmesos::getAmesos(
        selfID)->Create(ClassType, *CEpetra::getConstLinearProblem(
        LinearProblemID)));
}

boolean Amesos_Query ( 
  CT_Amesos_ID_t selfID, const char * ClassType )
{
    return CAmesos::getAmesos(selfID)->Query(ClassType);
}

CT_Teuchos_ParameterList_ID_t Amesos_GetValidParameters (  )
{
    return CTeuchos::storeParameterList(new Teuchos::ParameterList(
        Amesos::GetValidParameters()));
}


} // extern "C"


//
// Definitions from CAmesos_Cpp.hpp
//


/* get Amesos from non-const table using CT_Amesos_ID */
const Teuchos::RCP<Amesos>
CAmesos::getAmesos( CT_Amesos_ID_t id )
{
    CTrilinos_Object_ID_t aid
            = CTrilinos::abstractType<CT_Amesos_ID_t>(id);
    return tableOfAmesoss().get(aid);
}

/* get Amesos from non-const table using CTrilinos_Object_ID_t */
const Teuchos::RCP<Amesos>
CAmesos::getAmesos( CTrilinos_Object_ID_t id )
{
    return tableOfAmesoss().get(id);
}

/* get const Amesos from either the const or non-const table
 * using CT_Amesos_ID */
const Teuchos::RCP<const Amesos>
CAmesos::getConstAmesos( CT_Amesos_ID_t id )
{
    CTrilinos_Object_ID_t aid
            = CTrilinos::abstractType<CT_Amesos_ID_t>(id);
    if (id.is_const) {
        return tableOfConstAmesoss().get(aid);
    } else {
        return tableOfAmesoss().get(aid);
    }
}

/* get const Amesos from either the const or non-const table
 * using CTrilinos_Object_ID_t */
const Teuchos::RCP<const Amesos>
CAmesos::getConstAmesos( CTrilinos_Object_ID_t id )
{
    if (id.is_const) {
        return tableOfConstAmesoss().get(id);
    } else {
        return tableOfAmesoss().get(id);
    }
}

/* store Amesos in non-const table */
CT_Amesos_ID_t
CAmesos::storeAmesos( Amesos *pobj )
{
    return CTrilinos::concreteType<CT_Amesos_ID_t>(
            tableOfAmesoss().storeShared(pobj));
}

/* store const Amesos in const table */
CT_Amesos_ID_t
CAmesos::storeConstAmesos( const Amesos *pobj )
{
    return CTrilinos::concreteType<CT_Amesos_ID_t>(
            tableOfConstAmesoss().storeShared(pobj));
}

/* dump contents of Amesos and const Amesos tables */
void
CAmesos::purgeAmesosTables(  )
{
    tableOfAmesoss().purge();
    tableOfConstAmesoss().purge();
}



#endif /* HAVE_CTRILINOS_AMESOS */


