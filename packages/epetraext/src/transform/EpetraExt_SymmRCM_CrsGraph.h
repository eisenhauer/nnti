//@HEADER
// ***********************************************************************
// 
//     EpetraExt: Epetra Extended - Linear Algebra Services Package
//                 Copyright (2001) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
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
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
//@HEADER
                                                                                                   
#ifndef EpetraExt_CRSGRAPH_SYMMRCM_H
#define EpetraExt_CRSGRAPH_SYMMRCM_H

#include <vector>

#include <EpetraExt_Transform.h>

class Epetra_Map;
class Epetra_CrsGraph;

namespace EpetraExt {

struct CrsGraph_SymmRCM : public StructuralSameTypeTransform<Epetra_CrsGraph> {

 public:

  ~CrsGraph_SymmRCM();

  CrsGraph_SymmRCM( bool BruteForce = false, int testLeafWidth = 5 )
  : bruteForce_(BruteForce),
    testLeafWidth_(testLeafWidth),
    RCMMap_(0),
    RCMColMap_(0)
  {}

  NewTypeRef operator()( OriginalTypeRef orig );

 private:

  Epetra_Map * RCMMap_;
  Epetra_Map * RCMColMap_;
  const int testLeafWidth_;
  const bool bruteForce_;

  class BFT {
    
   public:

     BFT( const vector< vector<int> > & adjlist,
          int root,
          int max_width,
          bool & failed );

     int Width() { return width_; }
     int Depth() { return depth_; }

     void NonNeighborLeaves( vector<int> & leaves,
                             const vector< vector<int> > & adjlist,
                             int count );
     void ReverseVector( vector<int> & ordered );

   private:

     bool failed_;
     int width_;
     int depth_;
     int nodes_;

     vector< vector<int> > levelSets_;
  };

};

} //namespace EpetraExt

#endif //EpetraExt_CRSGRAPH_SYMMRCM_H
