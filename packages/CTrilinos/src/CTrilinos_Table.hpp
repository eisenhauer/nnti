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


/*! @file CTrilinos_Table.hpp
 * @brief Table for storing Trilinos objects. */


#ifndef CTRILINOS_TABLE_HPP
#define CTRILINOS_TABLE_HPP


#include <string>
#include <typeinfo>

#include "Teuchos_RCP.hpp"
#include "Teuchos_SimpleObjectTable.hpp"
#include "Teuchos_Exceptions.hpp"
#include "CTrilinos_enums.h"
#include "CTrilinos_exceptions.hpp"


namespace CTrilinos
{

/* stringify the enum name -- defined in CTrilinos_utils.cpp */
std::string enum2str( CTrilinos_Table_ID_t ty );


/*! Table class for storing Trilinos objects internally. */
template <class T>
class Table
{
  public:

    /*! constructor -- use is_const = true if table will store
     * objects of type const T instead of T */
    Table(CTrilinos_Table_ID_t type);

    /*! destructor */
    ~Table();

    /*! retrieve the object */
    const Teuchos::RCP<T> get(CTrilinos_Universal_ID_t id);

    /*! cast the object to type T and store a copy */
    template <class TT>
    const Teuchos::RCP<TT> get(CTrilinos_Universal_ID_t id);

    /*! retrieve the object */
    const Teuchos::RCP<const T> getConst(CTrilinos_Universal_ID_t id);

    /*! cast the object to type T and store a copy */
    template <class TT>
    const Teuchos::RCP<const TT> getConst(CTrilinos_Universal_ID_t id);

    /*! store an object of type T */
    CTrilinos_Universal_ID_t store(T* pobj, bool owned);

    /*! store an object whose base class is T */
    template <class Told>
    CTrilinos_Universal_ID_t store(Told* pobj, bool owned);

    /*! store an object of type const T */
    CTrilinos_Universal_ID_t store(const T* pobj, bool owned);

    /*! store an object whose base class is T */
    template <class Told>
    CTrilinos_Universal_ID_t store(const Told* pobj, bool owned);

    /*! cast the object to type T and store a copy */
    template <class Told>
    CTrilinos_Universal_ID_t alias(const Teuchos::RCP<Told> & rold);

    /*! cast the object to type T and store a copy */
    template <class Told>
    CTrilinos_Universal_ID_t alias(const Teuchos::RCP<const Told> & rold);

    /*! remove an object from the table and invalidate the id struct */
    int remove(CTrilinos_Universal_ID_t * id);

    /*! dump the table's contents but keep it's properties */
    void purge();

    /*! whether or not this table is templated on the desired tyoe */
    bool isType(CTrilinos_Table_ID_t tab) { return (tab == ttab); }

  private:

    /*! build full exception msg on the fly */
    std::string typeMismatchMsg(CTrilinos_Universal_ID_t id, std::string act);

    /*! build full exception msg on the fly */
    std::string badCastMsg(std::string type, std::string act);

    /* tables for storing objects */
    Teuchos::SimpleObjectTable<T> sot;
    Teuchos::SimpleObjectTable<const T> csot;

    /* properties of the tables */
    CTrilinos_Table_ID_t ttab;  /* enum value for stored objects */
    std::string castmsg;        /* string for exception msgs */
    std::string mismsg;         /* string for exception msgs */
};


/* constructor -- use is_const = true if table will store
 * objects of type const T instead of T */
template <class T>
Table<T>::Table(CTrilinos_Table_ID_t tab)
  : ttab(tab)
{
    /* assemble exception error messages for future use */
    std::stringstream hs;
    hs << "[CTrilinos::Table<" << typeid(T).name() << ">]: ";

    std::stringstream ss1;
    ss1 << "Expected type " << typeid(T).name() << " but found ";
    castmsg = hs.str() + ss1.str();

    std::stringstream ss2;
    ss1 << "Expected type " << enum2str(tab) << " but found ";
    mismsg = hs.str() + ss2.str();
}

/* destructor */
template <class T>
Table<T>::~Table()
{
  purge();
}

/* retrieve the object */
template <class T>
const Teuchos::RCP<T> Table<T>::get(CTrilinos_Universal_ID_t id)
{
    if (id.is_const)
        throw CTrilinosTypeMismatchError(typeMismatchMsg(id, std::string("get()")));

    if (id.table == ttab)
        return sot.getRCP(id.index);

    throw CTrilinosTypeMismatchError(typeMismatchMsg(id, std::string("get()")));
    return Teuchos::null;
}

/* return a different type RCP */
template <class T>
template <class TT>
const Teuchos::RCP<TT> Table<T>::get(CTrilinos_Universal_ID_t id)
{
    if (id.is_const)
        throw CTrilinosTypeMismatchError(typeMismatchMsg(id, std::string("get()")));

    if (id.table == ttab)
        return Teuchos::rcp_dynamic_cast<TT,T>(sot.getRCP(id.index), true);

    throw CTrilinosTypeMismatchError(typeMismatchMsg(id, std::string("get()")));
    return Teuchos::null;
}

/* retrieve the object */
template <class T>
const Teuchos::RCP<const T> Table<T>::getConst(CTrilinos_Universal_ID_t id)
{
    if (id.table == ttab) {
        if (id.is_const)
            return csot.getRCP(id.index);
        else
            return sot.getRCP(id.index);
    }

    throw CTrilinosTypeMismatchError(typeMismatchMsg(id, std::string("getConst()")));
    return Teuchos::null;
}

/* retrieve the object */
template <class T>
template <class TT>
const Teuchos::RCP<const TT> Table<T>::getConst(CTrilinos_Universal_ID_t id)
{
    if (id.table == ttab) {
        if (id.is_const)
            return Teuchos::rcp_dynamic_cast<const TT,const T>(csot.getRCP(id.index), true);
        else
            return Teuchos::rcp_dynamic_cast<TT,T>(sot.getRCP(id.index), true);
    }

    throw CTrilinosTypeMismatchError(typeMismatchMsg(id, std::string("getConst()")));
    return Teuchos::null;
}

/* store an object of type T */
template <class T>
CTrilinos_Universal_ID_t Table<T>::store(T* pobj, bool owned)
{
    if (pobj == NULL)
        throw Teuchos::NullReferenceError("[CTrilinos::Table]: Cannot store NULL pointer");

    CTrilinos_Universal_ID_t id;
    id.table = CT_Invalid_ID;
    id.index = -1;
    id.is_const = false;

    if ((id.index = sot.storeNew(pobj, owned)) != -1)
        id.table = ttab;

    return id;
}

/* store an object whose base class is T */
template <class T>
template <class Told>
CTrilinos_Universal_ID_t Table<T>::store(Told* pobj, bool owned)
{ /* prevent adding wrong types */
    if (pobj == NULL)
        throw Teuchos::NullReferenceError("[CTrilinos::Table]: Cannot store NULL pointer");

    CTrilinos_Universal_ID_t id;
    id.table = CT_Invalid_ID;
    id.index = -1;
    id.is_const = false;

    T* pnew = dynamic_cast<T*>(pobj);
    if (pnew != NULL) {
        if ((id.index = sot.storeNew(pobj, owned)) != -1)
            id.table = ttab;
    } else {
        throw CTrilinosTypeMismatchError(badCastMsg(typeid(*pobj).name(), std::string("store()")));
    }

    return id;
}

/* store an object of type T */
template <class T>
CTrilinos_Universal_ID_t Table<T>::store(const T* pobj, bool owned)
{
    if (pobj == NULL)
        throw Teuchos::NullReferenceError("[CTrilinos::Table]: Cannot store NULL pointer");

    CTrilinos_Universal_ID_t id;
    id.table = CT_Invalid_ID;
    id.index = -1;
    id.is_const = true;

    if ((id.index = csot.storeNew(pobj, owned)) != -1)
        id.table = ttab;

    return id;
}

/* store an object whose base class is T */
template <class T>
template <class Told>
CTrilinos_Universal_ID_t Table<T>::store(const Told* pobj, bool owned)
{ /* prevent adding wrong types */
    if (pobj == NULL)
        throw Teuchos::NullReferenceError("[CTrilinos::Table]: Cannot store NULL pointer");

    CTrilinos_Universal_ID_t id;
    id.table = CT_Invalid_ID;
    id.index = -1;
    id.is_const = true;

    const T* pnew = dynamic_cast<const T*>(pobj);
    if (pnew != NULL) {
        if ((id.index = csot.storeNew(pobj, owned)) != -1)
            id.table = ttab;
    } else {
        throw CTrilinosTypeMismatchError(badCastMsg(typeid(*pobj).name(), std::string("storeConst()")));
    }

    return id;
}

/* cast the object to type T and store a copy */
template <class T>
template <class Told>
CTrilinos_Universal_ID_t Table<T>::alias(const Teuchos::RCP<Told> & rold)
{
    CTrilinos_Universal_ID_t newid;
    newid.table = CT_Invalid_ID;
    newid.index = -1;
    newid.is_const = false;

    newid.index = sot.storeCastedRCP(rold);

    if (newid.index != -1)
        newid.table = ttab;

    return newid;
}

/* cast the object to type T and store a copy */
template <class T>
template <class Told>
CTrilinos_Universal_ID_t Table<T>::alias(const Teuchos::RCP<const Told> & rold)
{
    CTrilinos_Universal_ID_t newid;
    newid.table = CT_Invalid_ID;
    newid.index = -1;
    newid.is_const = true;

    newid.index = csot.storeCastedRCP(rold);

    if (newid.index != -1)
        newid.table = ttab;

    return newid;
}

/* remove an object from the table and invalidate the id struct */
template <class T>
int Table<T>::remove(CTrilinos_Universal_ID_t * id)
{
    int ret = -1;

    if (id->table == ttab) {
        if (id->is_const)
            ret = (csot.removeRCP(id->index) < 0 ? -1 : 0);
        else
            ret = (sot.removeRCP(id->index) < 0 ? -1 : 0);
        if (ret == 0) id->table = CT_Invalid_ID;
    } else {
        throw CTrilinosTypeMismatchError(typeMismatchMsg(*id, std::string("remove()")));
    }

    return ret;
}

/* dump the table's contents but keep it's properties */
template <class T>
void Table<T>::purge()
{
    sot.purge();
    csot.purge();
}

/* build full exception msg on the fly */
template <class T>
std::string Table<T>::typeMismatchMsg(CTrilinos_Universal_ID_t id, std::string act)
{
    std::stringstream ss;
    ss << mismsg << enum2str(id.table);
    if (id.is_const) {
        ss << " (const)";
    } else {
        ss << " (non-const)";
    }
    ss << " when attempting to " << act << " at index " << id.index;

    return ss.str();
}

/* build full exception msg on the fly */
template <class T>
std::string Table<T>::badCastMsg(std::string type, std::string act)
{
    std::stringstream ss;
    ss << castmsg << type << " when attempting to " << act;

    return ss.str();
}


} // namespace CTrilinos


#endif // CTRILINOS_TABLE_HPP


