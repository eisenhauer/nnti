#ifndef CTRILINOS_UTILS_TEMPL_HPP
#define CTRILINOS_UTILS_TEMPL_HPP


#include <string>

#include "CEpetra_Distributor_Cpp.hpp"
#include "CEpetra_SerialComm_Cpp.hpp"
#include "CEpetra_BLAS_Cpp.hpp"
#include "CEpetra_Comm_Cpp.hpp"
#include "CEpetra_Operator_Cpp.hpp"
#include "CEpetra_MultiVector_Cpp.hpp"
#include "CEpetra_OffsetIndex_Cpp.hpp"
#include "CEpetra_Object_Cpp.hpp"
#include "CEpetra_RowMatrix_Cpp.hpp"
#include "CEpetra_CompObject_Cpp.hpp"
#include "CEpetra_Directory_Cpp.hpp"
#include "CEpetra_Flops_Cpp.hpp"
#include "CEpetra_SrcDistObject_Cpp.hpp"
#include "CEpetra_MpiComm_Cpp.hpp"
#include "CEpetra_CrsMatrix_Cpp.hpp"
#include "CEpetra_CrsGraph_Cpp.hpp"
#include "CEpetra_DistObject_Cpp.hpp"
#include "CEpetra_Vector_Cpp.hpp"
#include "CEpetra_Export_Cpp.hpp"
#include "CEpetra_Map_Cpp.hpp"
#include "CEpetra_BlockMap_Cpp.hpp"
#include "CEpetra_Import_Cpp.hpp"
#include "CEpetra_Time_Cpp.hpp"
#include "CEpetra_JadMatrix_Cpp.hpp"

#include "CTrilinos_config.h"
#include "CTrilinos_enums.h"
#include "CTrilinos_Table.hpp"
#include "CTrilinos_utils.hpp"


namespace CTrilinos {


template <class T>
CTrilinos_Object_ID_t
cast( Table<const T> &destTable, CTrilinos_Object_ID_t id )
{
    CTrilinos_Object_ID_t newid;
    newid.type = CT_Invalid_ID;
    newid.index = -1;
    newid.is_const = id.is_const;

    if (id.is_const) {
        switch (id.type) {
        case CT_Epetra_Distributor_ID:
            newid = destTable.cast(CEpetra::getConstDistributor(id));
            break;
        case CT_Epetra_SerialComm_ID:
            newid = destTable.cast(CEpetra::getConstSerialComm(id));
            break;
        case CT_Epetra_BLAS_ID:
            newid = destTable.cast(CEpetra::getConstBLAS(id));
            break;
        case CT_Epetra_Comm_ID:
            newid = destTable.cast(CEpetra::getConstComm(id));
            break;
        case CT_Epetra_Operator_ID:
            newid = destTable.cast(CEpetra::getConstOperator(id));
            break;
        case CT_Epetra_MultiVector_ID:
            newid = destTable.cast(CEpetra::getConstMultiVector(id));
            break;
        case CT_Epetra_OffsetIndex_ID:
            newid = destTable.cast(CEpetra::getConstOffsetIndex(id));
            break;
        case CT_Epetra_Object_ID:
            newid = destTable.cast(CEpetra::getConstObject(id));
            break;
        case CT_Epetra_RowMatrix_ID:
            newid = destTable.cast(CEpetra::getConstRowMatrix(id));
            break;
        case CT_Epetra_CompObject_ID:
            newid = destTable.cast(CEpetra::getConstCompObject(id));
            break;
        case CT_Epetra_Directory_ID:
            newid = destTable.cast(CEpetra::getConstDirectory(id));
            break;
        case CT_Epetra_Flops_ID:
            newid = destTable.cast(CEpetra::getConstFlops(id));
            break;
        case CT_Epetra_SrcDistObject_ID:
            newid = destTable.cast(CEpetra::getConstSrcDistObject(id));
            break;
#ifdef HAVE_MPI
        case CT_Epetra_MpiComm_ID:
            newid = destTable.cast(CEpetra::getConstMpiComm(id));
            break;
#endif /* HAVE_MPI */
        case CT_Epetra_CrsMatrix_ID:
            newid = destTable.cast(CEpetra::getConstCrsMatrix(id));
            break;
        case CT_Epetra_CrsGraph_ID:
            newid = destTable.cast(CEpetra::getConstCrsGraph(id));
            break;
        case CT_Epetra_DistObject_ID:
            newid = destTable.cast(CEpetra::getConstDistObject(id));
            break;
        case CT_Epetra_Vector_ID:
            newid = destTable.cast(CEpetra::getConstVector(id));
            break;
        case CT_Epetra_Export_ID:
            newid = destTable.cast(CEpetra::getConstExport(id));
            break;
        case CT_Epetra_Map_ID:
            newid = destTable.cast(CEpetra::getConstMap(id));
            break;
        case CT_Epetra_BlockMap_ID:
            newid = destTable.cast(CEpetra::getConstBlockMap(id));
            break;
        case CT_Epetra_Import_ID:
            newid = destTable.cast(CEpetra::getConstImport(id));
            break;
        case CT_Epetra_Time_ID:
            newid = destTable.cast(CEpetra::getConstTime(id));
            break;
        case CT_Epetra_JadMatrix_ID:
            newid = destTable.cast(CEpetra::getConstJadMatrix(id));
            break;
        default:
            break;
        }
    } else {
        switch (id.type) {
        case CT_Epetra_Distributor_ID:
            newid = destTable.cast(CEpetra::getDistributor(id));
            break;
        case CT_Epetra_SerialComm_ID:
            newid = destTable.cast(CEpetra::getSerialComm(id));
            break;
        case CT_Epetra_BLAS_ID:
            newid = destTable.cast(CEpetra::getBLAS(id));
            break;
        case CT_Epetra_Comm_ID:
            newid = destTable.cast(CEpetra::getComm(id));
            break;
        case CT_Epetra_Operator_ID:
            newid = destTable.cast(CEpetra::getOperator(id));
            break;
        case CT_Epetra_MultiVector_ID:
            newid = destTable.cast(CEpetra::getMultiVector(id));
            break;
        case CT_Epetra_OffsetIndex_ID:
            newid = destTable.cast(CEpetra::getOffsetIndex(id));
            break;
        case CT_Epetra_Object_ID:
            newid = destTable.cast(CEpetra::getObject(id));
            break;
        case CT_Epetra_RowMatrix_ID:
            newid = destTable.cast(CEpetra::getRowMatrix(id));
            break;
        case CT_Epetra_CompObject_ID:
            newid = destTable.cast(CEpetra::getCompObject(id));
            break;
        case CT_Epetra_Directory_ID:
            newid = destTable.cast(CEpetra::getDirectory(id));
            break;
        case CT_Epetra_Flops_ID:
            newid = destTable.cast(CEpetra::getFlops(id));
            break;
        case CT_Epetra_SrcDistObject_ID:
            newid = destTable.cast(CEpetra::getSrcDistObject(id));
            break;
#ifdef HAVE_MPI
        case CT_Epetra_MpiComm_ID:
            newid = destTable.cast(CEpetra::getMpiComm(id));
            break;
#endif /* HAVE_MPI */
        case CT_Epetra_CrsMatrix_ID:
            newid = destTable.cast(CEpetra::getCrsMatrix(id));
            break;
        case CT_Epetra_CrsGraph_ID:
            newid = destTable.cast(CEpetra::getCrsGraph(id));
            break;
        case CT_Epetra_DistObject_ID:
            newid = destTable.cast(CEpetra::getDistObject(id));
            break;
        case CT_Epetra_Vector_ID:
            newid = destTable.cast(CEpetra::getVector(id));
            break;
        case CT_Epetra_Export_ID:
            newid = destTable.cast(CEpetra::getExport(id));
            break;
        case CT_Epetra_Map_ID:
            newid = destTable.cast(CEpetra::getMap(id));
            break;
        case CT_Epetra_BlockMap_ID:
            newid = destTable.cast(CEpetra::getBlockMap(id));
            break;
        case CT_Epetra_Import_ID:
            newid = destTable.cast(CEpetra::getImport(id));
            break;
        case CT_Epetra_Time_ID:
            newid = destTable.cast(CEpetra::getTime(id));
            break;
        case CT_Epetra_JadMatrix_ID:
            newid = destTable.cast(CEpetra::getJadMatrix(id));
            break;
        default:
            break;
        }
    }

    return newid;
}

template <class T>
CTrilinos_Object_ID_t
cast( Table<T> &destTable, CTrilinos_Object_ID_t id )
{
    CTrilinos_Object_ID_t newid;
    newid.type = CT_Invalid_ID;
    newid.index = -1;
    newid.is_const = id.is_const;

    if (id.is_const) {
        throw CTrilinosTypeMismatchError("[CTrilinos::cast]: casting away constness");
    } else {
        switch (id.type) {
        case CT_Epetra_Distributor_ID:
            newid = destTable.cast(CEpetra::getDistributor(id));
            break;
        case CT_Epetra_SerialComm_ID:
            newid = destTable.cast(CEpetra::getSerialComm(id));
            break;
        case CT_Epetra_BLAS_ID:
            newid = destTable.cast(CEpetra::getBLAS(id));
            break;
        case CT_Epetra_Comm_ID:
            newid = destTable.cast(CEpetra::getComm(id));
            break;
        case CT_Epetra_Operator_ID:
            newid = destTable.cast(CEpetra::getOperator(id));
            break;
        case CT_Epetra_MultiVector_ID:
            newid = destTable.cast(CEpetra::getMultiVector(id));
            break;
        case CT_Epetra_OffsetIndex_ID:
            newid = destTable.cast(CEpetra::getOffsetIndex(id));
            break;
        case CT_Epetra_Object_ID:
            newid = destTable.cast(CEpetra::getObject(id));
            break;
        case CT_Epetra_RowMatrix_ID:
            newid = destTable.cast(CEpetra::getRowMatrix(id));
            break;
        case CT_Epetra_CompObject_ID:
            newid = destTable.cast(CEpetra::getCompObject(id));
            break;
        case CT_Epetra_Directory_ID:
            newid = destTable.cast(CEpetra::getDirectory(id));
            break;
        case CT_Epetra_Flops_ID:
            newid = destTable.cast(CEpetra::getFlops(id));
            break;
        case CT_Epetra_SrcDistObject_ID:
            newid = destTable.cast(CEpetra::getSrcDistObject(id));
            break;
#ifdef HAVE_MPI
        case CT_Epetra_MpiComm_ID:
            newid = destTable.cast(CEpetra::getMpiComm(id));
            break;
#endif /* HAVE_MPI */
        case CT_Epetra_CrsMatrix_ID:
            newid = destTable.cast(CEpetra::getCrsMatrix(id));
            break;
        case CT_Epetra_CrsGraph_ID:
            newid = destTable.cast(CEpetra::getCrsGraph(id));
            break;
        case CT_Epetra_DistObject_ID:
            newid = destTable.cast(CEpetra::getDistObject(id));
            break;
        case CT_Epetra_Vector_ID:
            newid = destTable.cast(CEpetra::getVector(id));
            break;
        case CT_Epetra_Export_ID:
            newid = destTable.cast(CEpetra::getExport(id));
            break;
        case CT_Epetra_Map_ID:
            newid = destTable.cast(CEpetra::getMap(id));
            break;
        case CT_Epetra_BlockMap_ID:
            newid = destTable.cast(CEpetra::getBlockMap(id));
            break;
        case CT_Epetra_Import_ID:
            newid = destTable.cast(CEpetra::getImport(id));
            break;
        case CT_Epetra_Time_ID:
            newid = destTable.cast(CEpetra::getTime(id));
            break;
        case CT_Epetra_JadMatrix_ID:
            newid = destTable.cast(CEpetra::getJadMatrix(id));
            break;
        default:
            break;
        }
    }

    return newid;
}

/* convert struct from specific type to generic CTrilinos_Object_ID_t
 * but keep the content in tact */
template <typename T>
CTrilinos_Object_ID_t
abstractType( T id )
{
    CTrilinos_Object_ID_t newid;

    newid.type = id.type;
    newid.index = id.index;
    newid.is_const = id.is_const;

    return newid;
}

/* convert struct from generic CTrilinos_Object_ID_t to specific type
 * but keep the content in tact */
template <typename T>
T
concreteType( CTrilinos_Object_ID_t id )
{
    T newid;

    newid.type = id.type;
    newid.index = id.index;
    newid.is_const = id.is_const;

    return newid;
}

/* isSameObject(RCP, RCP) */
template <class T1, class T2>
bool
isSameObject( const Teuchos::RCP<T1> &rcp1, const Teuchos::RCP<T2> &rcp2 )
{
    return (rcp1.shares_resource(rcp2));
}

/* isSameObject(RCP, generic_id) */
template <class T>
bool
isSameObject( const Teuchos::RCP<T> &rcp, CTrilinos_Object_ID_t id )
{
    bool shares = false;

    if (id.is_const) {
        switch (id.type) {
        case CT_Epetra_Distributor_ID:
            shares = rcp.shares_resource(CEpetra::getConstDistributor(id));
            break;
        case CT_Epetra_SerialComm_ID:
            shares = rcp.shares_resource(CEpetra::getConstSerialComm(id));
            break;
        case CT_Epetra_BLAS_ID:
            shares = rcp.shares_resource(CEpetra::getConstBLAS(id));
            break;
        case CT_Epetra_Comm_ID:
            shares = rcp.shares_resource(CEpetra::getConstComm(id));
            break;
        case CT_Epetra_Operator_ID:
            shares = rcp.shares_resource(CEpetra::getConstOperator(id));
            break;
        case CT_Epetra_MultiVector_ID:
            shares = rcp.shares_resource(CEpetra::getConstMultiVector(id));
            break;
        case CT_Epetra_OffsetIndex_ID:
            shares = rcp.shares_resource(CEpetra::getConstOffsetIndex(id));
            break;
        case CT_Epetra_Object_ID:
            shares = rcp.shares_resource(CEpetra::getConstObject(id));
            break;
        case CT_Epetra_RowMatrix_ID:
            shares = rcp.shares_resource(CEpetra::getConstRowMatrix(id));
            break;
        case CT_Epetra_CompObject_ID:
            shares = rcp.shares_resource(CEpetra::getConstCompObject(id));
            break;
        case CT_Epetra_Directory_ID:
            shares = rcp.shares_resource(CEpetra::getConstDirectory(id));
            break;
        case CT_Epetra_Flops_ID:
            shares = rcp.shares_resource(CEpetra::getConstFlops(id));
            break;
        case CT_Epetra_SrcDistObject_ID:
            shares = rcp.shares_resource(CEpetra::getConstSrcDistObject(id));
            break;
#ifdef HAVE_MPI
        case CT_Epetra_MpiComm_ID:
            shares = rcp.shares_resource(CEpetra::getConstMpiComm(id));
            break;
#endif /* HAVE_MPI */
        case CT_Epetra_CrsMatrix_ID:
            shares = rcp.shares_resource(CEpetra::getConstCrsMatrix(id));
            break;
        case CT_Epetra_CrsGraph_ID:
            shares = rcp.shares_resource(CEpetra::getConstCrsGraph(id));
            break;
        case CT_Epetra_DistObject_ID:
            shares = rcp.shares_resource(CEpetra::getConstDistObject(id));
            break;
        case CT_Epetra_Vector_ID:
            shares = rcp.shares_resource(CEpetra::getConstVector(id));
            break;
        case CT_Epetra_Export_ID:
            shares = rcp.shares_resource(CEpetra::getConstExport(id));
            break;
        case CT_Epetra_Map_ID:
            shares = rcp.shares_resource(CEpetra::getConstMap(id));
            break;
        case CT_Epetra_BlockMap_ID:
            shares = rcp.shares_resource(CEpetra::getConstBlockMap(id));
            break;
        case CT_Epetra_Import_ID:
            shares = rcp.shares_resource(CEpetra::getConstImport(id));
            break;
        case CT_Epetra_Time_ID:
            shares = rcp.shares_resource(CEpetra::getConstTime(id));
            break;
        case CT_Epetra_JadMatrix_ID:
            shares = rcp.shares_resource(CEpetra::getConstJadMatrix(id));
            break;
        default:
            break;
        }
    } else {
        switch (id.type) {
        case CT_Epetra_Distributor_ID:
            shares = rcp.shares_resource(CEpetra::getDistributor(id));
            break;
        case CT_Epetra_SerialComm_ID:
            shares = rcp.shares_resource(CEpetra::getSerialComm(id));
            break;
        case CT_Epetra_BLAS_ID:
            shares = rcp.shares_resource(CEpetra::getBLAS(id));
            break;
        case CT_Epetra_Comm_ID:
            shares = rcp.shares_resource(CEpetra::getComm(id));
            break;
        case CT_Epetra_Operator_ID:
            shares = rcp.shares_resource(CEpetra::getOperator(id));
            break;
        case CT_Epetra_MultiVector_ID:
            shares = rcp.shares_resource(CEpetra::getMultiVector(id));
            break;
        case CT_Epetra_OffsetIndex_ID:
            shares = rcp.shares_resource(CEpetra::getOffsetIndex(id));
            break;
        case CT_Epetra_Object_ID:
            shares = rcp.shares_resource(CEpetra::getObject(id));
            break;
        case CT_Epetra_RowMatrix_ID:
            shares = rcp.shares_resource(CEpetra::getRowMatrix(id));
            break;
        case CT_Epetra_CompObject_ID:
            shares = rcp.shares_resource(CEpetra::getCompObject(id));
            break;
        case CT_Epetra_Directory_ID:
            shares = rcp.shares_resource(CEpetra::getDirectory(id));
            break;
        case CT_Epetra_Flops_ID:
            shares = rcp.shares_resource(CEpetra::getFlops(id));
            break;
        case CT_Epetra_SrcDistObject_ID:
            shares = rcp.shares_resource(CEpetra::getSrcDistObject(id));
            break;
#ifdef HAVE_MPI
        case CT_Epetra_MpiComm_ID:
            shares = rcp.shares_resource(CEpetra::getMpiComm(id));
            break;
#endif /* HAVE_MPI */
        case CT_Epetra_CrsMatrix_ID:
            shares = rcp.shares_resource(CEpetra::getCrsMatrix(id));
            break;
        case CT_Epetra_CrsGraph_ID:
            shares = rcp.shares_resource(CEpetra::getCrsGraph(id));
            break;
        case CT_Epetra_DistObject_ID:
            shares = rcp.shares_resource(CEpetra::getDistObject(id));
            break;
        case CT_Epetra_Vector_ID:
            shares = rcp.shares_resource(CEpetra::getVector(id));
            break;
        case CT_Epetra_Export_ID:
            shares = rcp.shares_resource(CEpetra::getExport(id));
            break;
        case CT_Epetra_Map_ID:
            shares = rcp.shares_resource(CEpetra::getMap(id));
            break;
        case CT_Epetra_BlockMap_ID:
            shares = rcp.shares_resource(CEpetra::getBlockMap(id));
            break;
        case CT_Epetra_Import_ID:
            shares = rcp.shares_resource(CEpetra::getImport(id));
            break;
        case CT_Epetra_Time_ID:
            shares = rcp.shares_resource(CEpetra::getTime(id));
            break;
        case CT_Epetra_JadMatrix_ID:
            shares = rcp.shares_resource(CEpetra::getJadMatrix(id));
            break;
        default:
            break;
        }
    }

    return shares;
}

/* isSameObject(RCP, specific_id) */
template <class T, typename S>
bool
isSameObject( const Teuchos::RCP<T> &rcp, S id )
{
    return isSameObject<T>(rcp, abstractType<S>(id));
}

/* isSameObject(specific_id, RCP) */
template <typename S, class T>
bool
isSameObject( S id, const Teuchos::RCP<T> &rcp )
{
    return isSameObject<T>(rcp, abstractType<S>(id));
}

/* isSameObject(specific_id, specific_id) */
template <typename S1, typename S2>
bool
isSameObject( S1 id1, S2 id2 )
{
    CTrilinos_Object_ID_t id1a = abstractType<S1>(id1);
    CTrilinos_Object_ID_t id2a = abstractType<S2>(id2);

    return isSameObject(id1a, id2a);
}


} // namespace CTrilinos


#endif
