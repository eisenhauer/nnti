#ifndef PHX_DATA_LAYOUT
#define PHX_DATA_LAYOUT

#include <iostream>

namespace PHX{


  /*! \brief A pure virtual class to distinguish a unique data layout in a cell.

      The DataLayout class is used to (1) specify the array size of a
      an algebraic type in a single cell, and (2) to differentiate
      FieldTags that have the same name, but have different
      DataLayouts in the FieldManager.  For example suppose we want to
      store density at both the nodes and the quadrature points in a
      cell.  If we use the same string name for the FieldTag, the
      DataLayout will differentiate the objects.  We could probably
      just use an enumerated type here, but the DataLayout class
      allows users to derive and pass in auxiliary data via the tag.

  */
  class DataLayout {

  public:

    DataLayout() {}

    virtual ~DataLayout() {}

    virtual std::size_t size() const = 0;

    virtual bool operator==(const DataLayout& left) const = 0;

    virtual const std::type_info& getAlgebraicTypeInfo() const = 0;

    virtual void print(std::ostream& os, int indent = 0) const = 0;

  };

  std::ostream& operator<<(std::ostream& os, const PHX::DataLayout& t);
  
}

#endif
