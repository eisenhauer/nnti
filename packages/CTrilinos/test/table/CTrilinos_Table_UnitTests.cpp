/*! \@HEADER */
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
Questions? Contact M. Nicole Lemaster (mnlemas\@sandia.gov)

************************************************************************
*/
/*! \@HEADER */


#include "CTrilinos_config.h"
#include "CTrilinos_enums.h"
#include "CTrilinos_utils.hpp"
#include "CTrilinos_Table.hpp"
#include "Teuchos_RCP.hpp"

#include "Teuchos_UnitTestHarness.hpp"

#include "CEpetra_SerialComm.h"
#include "CEpetra_Comm.h"
#include "CEpetra_Vector.h"
#include "Epetra_SerialComm.h"
#include "Epetra_Comm.h"
#include "Epetra_Vector.h"


#define JOIN_SET_0(A, B, C) A ## B ## C
#define JOIN_SET(A, B, C)   JOIN_SET_0(A, B, C)

#define BUILD_CALL(A, F) JOIN_SET( A , _ , F )
#define CLASS_TYPE(A)    JOIN_SET( CT_ , A , _ID_t )
#define CLASS_ENUM(A)    JOIN_SET( CT_ , A , _ID )
#define CLASS_ESTR(A)    XSTRFY(CLASS_ENUM(A))
#define STRFY(A)         #A
#define XSTRFY(A)        STRFY(A)
#define CONSTRUCTOR(A)   A


#define T Epetra_SerialComm
#define T1 Epetra_SerialComm
#define T2 Epetra_Comm
#define T3 Epetra_SerialComm
#define T4 Epetra_Vector


namespace {


using Teuchos::null;
using Teuchos::RCP;
using Teuchos::rcp;
using Teuchos::RangeError;
using Teuchos::NullReferenceError;
using Teuchos::m_bad_cast;
using CTrilinos::CTrilinosTypeMismatchError;
using CTrilinos::Table;


/* Table::store() owned */

TEUCHOS_UNIT_TEST( Table, store )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(CTrilinos_Universal_ID_t id = table.store(new T, true));
  TEST_EQUALITY_CONST(id.index, 0);
  TEST_EQUALITY_CONST(id.is_const, FALSE);
  TEST_EQUALITY(id.table, CLASS_ENUM(T));
  TEST_EQUALITY_CONST(nonnull(table.get(id)), true);
  TEST_EQUALITY_CONST(is_null(table.get(id)), false);
  TEST_EQUALITY_CONST(nonnull(table.getConst(id)), true);
  TEST_EQUALITY_CONST(is_null(table.getConst(id)), false);
}

TEUCHOS_UNIT_TEST( Table, storeBase )
{
  ECHO(Table<T2> table(CONSTRUCTOR(CLASS_ENUM(T2))));
  ECHO(CTrilinos_Universal_ID_t id = table.store(new T1, true));
  TEST_EQUALITY_CONST(id.index, 0);
  TEST_EQUALITY_CONST(id.is_const, FALSE);
  TEST_EQUALITY(id.table, CLASS_ENUM(T2));
  TEST_EQUALITY_CONST(nonnull(table.get(id)), true);
  TEST_EQUALITY_CONST(is_null(table.get(id)), false);
  TEST_EQUALITY_CONST(nonnull(table.getConst(id)), true);
  TEST_EQUALITY_CONST(is_null(table.getConst(id)), false);
}

/* this should not compile -- ok */
/*
TEUCHOS_UNIT_TEST( Table, storeWrong )
{
  ECHO(Table<T4> table(CONSTRUCTOR(CLASS_ENUM(T4))));
  TEST_THROW(table.store(new T3, true), CTrilinosTypeMismatchError);
}
*/

TEUCHOS_UNIT_TEST( Table, storeNull )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(T* pobj = NULL);
  TEST_THROW(table.store(pobj, false), NullReferenceError); 
}


/* Table::store() non-owned */

TEUCHOS_UNIT_TEST( Table, storeShared )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(T *pobj = new T);
  ECHO(CTrilinos_Universal_ID_t id = table.store(pobj, false));
  TEST_EQUALITY_CONST(id.index, 0);
  TEST_EQUALITY(id.table, CLASS_ENUM(T));
  TEST_EQUALITY(id.is_const, FALSE);
  TEST_EQUALITY_CONST(nonnull(table.get(id)), true);
  TEST_EQUALITY_CONST(is_null(table.get(id)), false);
  TEST_EQUALITY_CONST(nonnull(table.getConst(id)), true);
  TEST_EQUALITY_CONST(is_null(table.getConst(id)), false);
  ECHO(table.remove(&id));
  ECHO(delete pobj);
}

TEUCHOS_UNIT_TEST( Table, storeConstShared )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(const T *pobj = new T);
  ECHO(CTrilinos_Universal_ID_t id = table.store(pobj, false));
  TEST_EQUALITY_CONST(id.index, 0);
  TEST_EQUALITY(id.table, CLASS_ENUM(T));
  TEST_EQUALITY(id.is_const, TRUE);
  TEST_EQUALITY_CONST(nonnull(table.getConst(id)), true);
  TEST_EQUALITY_CONST(is_null(table.getConst(id)), false);
  TEST_THROW(nonnull(table.get(id)), CTrilinosTypeMismatchError);
  TEST_THROW(is_null(table.get(id)), CTrilinosTypeMismatchError);
  ECHO(table.remove(&id));
  ECHO(delete pobj);
}

TEUCHOS_UNIT_TEST( Table, storeSharedBase )
{
  ECHO(Table<T2> table(CONSTRUCTOR(CLASS_ENUM(T2))));
  ECHO(T1 *pobj = new T1);
  ECHO(CTrilinos_Universal_ID_t id = table.store(pobj, false));
  TEST_EQUALITY_CONST(id.index, 0);
  TEST_EQUALITY(id.table, CLASS_ENUM(T2));
  TEST_EQUALITY(id.is_const, FALSE);
  TEST_EQUALITY_CONST(nonnull(table.get(id)), true);
  TEST_EQUALITY_CONST(is_null(table.get(id)), false);
  TEST_EQUALITY_CONST(nonnull(table.getConst(id)), true);
  TEST_EQUALITY_CONST(is_null(table.getConst(id)), false);
  ECHO(table.remove(&id));
  ECHO(delete pobj);
}

TEUCHOS_UNIT_TEST( Table, storeConstSharedBase )
{
  ECHO(Table<T2> table(CONSTRUCTOR(CLASS_ENUM(T2))));
  ECHO(const T1 *pobj = new T1);
  ECHO(CTrilinos_Universal_ID_t id = table.store(pobj, false));
  TEST_EQUALITY_CONST(id.index, 0);
  TEST_EQUALITY(id.table, CLASS_ENUM(T2));
  TEST_EQUALITY(id.is_const, TRUE);
  TEST_EQUALITY_CONST(nonnull(table.getConst(id)), true);
  TEST_EQUALITY_CONST(is_null(table.getConst(id)), false);
  TEST_THROW(nonnull(table.get(id)), CTrilinosTypeMismatchError);
  TEST_THROW(is_null(table.get(id)), CTrilinosTypeMismatchError);
  ECHO(table.remove(&id));
  ECHO(delete pobj);
}

/* this should not compile -- ok */
/*
TEUCHOS_UNIT_TEST( Table, storeSharedWrong )
{
  ECHO(Table<T4> table(CONSTRUCTOR(CLASS_ENUM(T4))));
  ECHO(T3 *pobj = new T3);
  TEST_THROW(table.store(pobj, false), CTrilinosTypeMismatchError);
  ECHO(delete pobj);
}
*/

TEUCHOS_UNIT_TEST( Table, storeSharedCastConst )
{
  ECHO(Table<T2> table(CONSTRUCTOR(CLASS_ENUM(T2))));
  ECHO(T1 *pobj = new T1);
  ECHO(CTrilinos_Universal_ID_t id = table.store(pobj, false));
  TEST_EQUALITY_CONST(id.index, 0);
  TEST_EQUALITY(id.table, CLASS_ENUM(T2));
  TEST_EQUALITY(id.is_const, TRUE);
  TEST_EQUALITY_CONST(nonnull(table.getConst(id)), true);
  TEST_EQUALITY_CONST(is_null(table.getConst(id)), false);
  TEST_THROW(nonnull(table.get(id)), CTrilinosTypeMismatchError);
  TEST_THROW(is_null(table.get(id)), CTrilinosTypeMismatchError);
  ECHO(table.remove(&id));
  ECHO(delete pobj);
}

TEUCHOS_UNIT_TEST( Table, storeSharedNull )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(T* pobj = NULL);
  TEST_THROW(table.store(pobj, false), NullReferenceError); 
}

TEUCHOS_UNIT_TEST( Table, storeConstSharedNull )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(const T* pobj = NULL);
  TEST_THROW(table.store(pobj, false), NullReferenceError); 
}


/* Table::remove() */

TEUCHOS_UNIT_TEST( Table, remove )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(CTrilinos_Universal_ID_t id = table.store(new T, true));
  TEST_EQUALITY_CONST(id.index, 0);
  TEST_EQUALITY(id.table, CLASS_ENUM(T));
  TEST_EQUALITY_CONST(id.is_const, FALSE);
  ECHO(table.remove(&id));
  TEST_EQUALITY_CONST(id.index, -1);
  TEST_EQUALITY(id.table, CLASS_ENUM(Invalid));
}

TEUCHOS_UNIT_TEST( Table, removeConst )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(const T* pobj = new T);
  ECHO(CTrilinos_Universal_ID_t id = table.store(pobj, false));
  TEST_EQUALITY_CONST(id.index, 0);
  TEST_EQUALITY(id.table, CLASS_ENUM(T));
  TEST_EQUALITY_CONST(id.is_const, TRUE);
  ECHO(table.remove(&id));
  TEST_EQUALITY_CONST(id.index, -1);
  TEST_EQUALITY(id.table, CLASS_ENUM(Invalid));
  ECHO(delete pobj);
}

#ifdef TEUCHOS_DEBUG

TEUCHOS_UNIT_TEST( Table, removeInvalid )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(CTrilinos_Universal_ID_t id);
  ECHO(id.index = -1);
  ECHO(id.table = CLASS_ENUM(T));
  ECHO(id.is_const = FALSE);
  TEST_THROW(table.remove(&id), RangeError);
}

#endif /* TEUCHOS_DEBUG */

TEUCHOS_UNIT_TEST( Table, removeWrong )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(CTrilinos_Universal_ID_t id1 = table.store(new T, true));
  ECHO(CTrilinos_Universal_ID_t id);
  ECHO(id.index = id1.index);
  ECHO(id.table = CLASS_ENUM(T4));
  ECHO(id.is_const = FALSE);
  TEST_THROW(table.remove(&id), CTrilinosTypeMismatchError);
}


/* Table::get() */

TEUCHOS_UNIT_TEST( Table, get )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(CTrilinos_Universal_ID_t id = table.store(new T, true));
  ECHO(RCP<T> rcpT = table.get(id));
  TEST_EQUALITY_CONST(nonnull(rcpT), true);
  TEST_EQUALITY_CONST(is_null(rcpT), false);
  ECHO(RCP<const T> rcpCT = table.getConst(id));
  TEST_EQUALITY_CONST(nonnull(rcpCT), true);
  TEST_EQUALITY_CONST(is_null(rcpCT), false);
}

#ifdef TEUCHOS_DEBUG

TEUCHOS_UNIT_TEST( Table, getInvalid )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(CTrilinos_Universal_ID_t id);
  ECHO(id.index = 0);
  ECHO(id.table = CLASS_ENUM(T));
  ECHO(id.is_const = FALSE);
  TEST_THROW(table.get(id), RangeError);
}

#endif /* TEUCHOS_DEBUG */

TEUCHOS_UNIT_TEST( Table, getWrong )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(CTrilinos_Universal_ID_t id1 = table.store(new T, true));
  ECHO(CTrilinos_Universal_ID_t id);
  ECHO(id.index = id1.index);
  ECHO(id.table = CLASS_ENUM(T4));
  ECHO(id.is_const = FALSE);
  TEST_THROW(table.get(id), CTrilinosTypeMismatchError);
}


/* Table::alias() */

TEUCHOS_UNIT_TEST( Table, alias )
{
  ECHO(Table<T1> table1(CONSTRUCTOR(CLASS_ENUM(T1))));
  ECHO(Table<T2> table2(CONSTRUCTOR(CLASS_ENUM(T2))));

  ECHO(CTrilinos_Universal_ID_t id1 = table1.store(new T1, true));
  ECHO(CTrilinos_Universal_ID_t id2 = table2.alias(table1.get(id1)));

  TEST_EQUALITY(id2.table, CLASS_ENUM(T2));
  TEST_EQUALITY_CONST(id2.index, 0);
}

TEUCHOS_UNIT_TEST( Table, aliasConst )
{
  ECHO(Table<T1> table1(CONSTRUCTOR(CLASS_ENUM(T1))));
  ECHO(Table<T2> table2(CONSTRUCTOR(CLASS_ENUM(T2))));

  ECHO(CTrilinos_Universal_ID_t id1 = table1.store(new const T1, true));
  ECHO(CTrilinos_Universal_ID_t id2 = table2.alias(table1.get(id1)));

  TEST_EQUALITY(id2.table, CLASS_ENUM(T2));
  TEST_EQUALITY_CONST(id2.index, 0);
}

TEUCHOS_UNIT_TEST( Table, aliasBad )
{
  ECHO(Table<T3> table3(CONSTRUCTOR(CLASS_ENUM(T3))));
  ECHO(Table<T4> table4(CONSTRUCTOR(CLASS_ENUM(T4))));

  ECHO(CTrilinos_Universal_ID_t id3 = table3.store(new T3, true));
  TEST_THROW(table4.alias(table3.get(id3)), m_bad_cast);
}


/* Table::purge() */

#ifdef TEUCHOS_DEBUG

TEUCHOS_UNIT_TEST( Table, purge )
{
  ECHO(Table<T> table(CONSTRUCTOR(CLASS_ENUM(T))));
  ECHO(CTrilinos_Universal_ID_t id = table.store(new T, true));
  TEST_EQUALITY_CONST(nonnull(table.get(id)), true);
  ECHO(table.purge());
  TEST_THROW(table.get(id), RangeError);
}

#endif /* TEUCHOS_DEBUG */


/* Table::isType() */

#ifdef TEUCHOS_DEBUG

TEUCHOS_UNIT_TEST( Table, isType )
{
  ECHO(Table<T3> table3(CONSTRUCTOR(CLASS_ENUM(T3))));
  ECHO(Table<T4> table4(CONSTRUCTOR(CLASS_ENUM(T4))));
  ECHO(CTrilinos_Universal_ID_t id = table3.store(new T3, true));
  TEST_EQUALITY_CONST(table3.isType(id.table), true);
  TEST_EQUALITY_CONST(table4.isType(id.table), false);
}

#endif /* TEUCHOS_DEBUG */


//
// Template Instantiations
//


#ifdef TEUCHOS_DEBUG

#  define DEBUG_UNIT_TEST_GROUP( TT ) \

#else

#  define DEBUG_UNIT_TEST_GROUP( TT )

#endif


#define UNIT_TEST_GROUP( TT ) \
  DEBUG_UNIT_TEST_GROUP( TT )


} // namespace
