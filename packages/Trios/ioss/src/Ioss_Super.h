/*--------------------------------------------------------------------*/
/*    Copyright 2000 Sandia Corporation.                              */
/*    Under the terms of Contract DE-AC04-94AL85000, there is a       */
/*    non-exclusive license for use of this work by or on behalf      */
/*    of the U.S. Government.  Export of this program may require     */
/*    a license from the United States Government.                    */
/*--------------------------------------------------------------------*/

// -*- Mode: c++ -*-
#ifndef IOSS_Ioss_Super_h
#define IOSS_Ioss_Super_h

#include <Ioss_CodeTypes.h>
#include <Ioss_ElementTopology.h>
#include <Ioss_Super.h>
#include <string>

// STL Includes
#include <vector>

namespace Ioss {
  class ElementVariableType;

  class Super : public Ioss::ElementTopology {

  public:
    static void factory();
    ~Super();
    Super(const std::string &name, int node_count);

    static void make_super(const std::string &type);
    
    int spatial_dimension()           const;
    int parametric_dimension()       const;
    bool is_element()                 const {return true;}
    int order()               const;

    int number_corner_nodes() const;
    int number_nodes()        const;
    int number_edges()        const;
    int number_faces()        const;

    int number_nodes_edge(int edge= 0)   const;
    int number_nodes_face(int face= 0)   const;
    int number_edges_face(int face= 0)   const;

    Ioss::IntVector edge_connectivity(int edge_number) const;
    Ioss::IntVector face_connectivity(int face_number) const;
    Ioss::IntVector element_connectivity()             const;

    Ioss::IntVector face_edge_connectivity(int face_number) const;

    Ioss::ElementTopology* face_type(int face_number = 0) const;
    Ioss::ElementTopology* edge_type(int edge_number = 0) const;

  protected:

  private:
    int nodeCount;
    Ioss::ElementVariableType *storageType;
    Super(const Super&); // Do not implement
  };
}
#endif
