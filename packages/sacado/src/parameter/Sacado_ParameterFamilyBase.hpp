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

#ifndef SACADO_PARAMETERFAMILYBASE_HPP
#define SACADO_PARAMETERFAMILYBASE_HPP

#include "Sacado_ConfigDefs.h"
#include "Teuchos_RefCountPtr.hpp"

namespace Sacado {
  
  /*!
   * A class to store multiple template instantiations of a single templated
   * parameter.
   */
  template <typename EntryBase, template<typename> class EntryType>
  class ParameterFamilyBase  {

  public:
  
    //! Constructor
    ParameterFamilyBase(const std::string& name, 
			bool supports_ad, 
			bool supports_analytic);

    //! Destructor
    virtual ~ParameterFamilyBase();

    //! Get the name of the family
    std::string getName() const;

    //! Indicates whether parameter supports AD derivatives
    bool supportsAD() const;

    //! Indicates whether parameter supports analytic derivatives
    bool supportsAnalytic() const;

    //! Determine if family has an entry for the given type \c ValueType.
    template <typename ValueType>
    bool hasType() const;

    //! Add a new parameter using custom entry
    /*!
     * Returns true if successful in adding entry to library, false 
     * otherwise.
     */
    template <typename ValueType>
    bool addEntry(const Teuchos::RefCountPtr< EntryType<ValueType> >& entry);

    //! Gets the entry corresponding to type \em ValueType
    template <typename ValueType>
    Teuchos::RefCountPtr< EntryType<ValueType> > getEntry();

    //! Gets the entry corresponding to type \em ValueType
    template <typename ValueType>
    Teuchos::RefCountPtr< const EntryType<ValueType> > getEntry() const;

    //! Print the family
    void printFamily(std::ostream& os) const;

  protected: 

    //! Map of entries for a parameter name
    typedef std::map<std::string, Teuchos::RefCountPtr<EntryBase> > ValueMap;

    //! Const iterator for ValueMap
    typedef typename ValueMap::const_iterator const_iterator;

    //! Iterator for ValueMap
    typedef typename ValueMap::iterator iterator;
    
    //! Returns a string representation of type \em ValueType
    template <class ValueType> std::string getTypeName() const;

  private:

    //! Private to prohibit copying
    ParameterFamilyBase(const ParameterFamilyBase&);
    
    //! Private to prohibit copying
    ParameterFamilyBase& operator = (const ParameterFamilyBase&);

  protected:

    //! Family of parameter entries
    ValueMap family;

    //! Family name
    const std::string name;

    //! Family supports AD
    bool supports_ad;
    
    //! Family supports analytic derivatives
    bool supports_analytic;

  };
}

// Include template definitions
#include "Sacado_ParameterFamilyBaseImp.hpp"

#endif
