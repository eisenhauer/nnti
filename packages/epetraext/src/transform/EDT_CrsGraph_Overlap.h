//@HEADER
// ************************************************************************
// 
//          Trilinos: An Object-Oriented Solver Framework
//              Copyright (2001) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or (at your option)
// any later version.
//   
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//   
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// 
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
// 
// ************************************************************************
//@HEADER

#ifndef EDT_CRSGRAPH_OVERLAP_H
#define EDT_CRSGRAPH_OVERLAP_H

#include <Epetra_Transform.h>

class Epetra_BlockMap;
class Epetra_CrsGraph;

namespace EpetraExt {

///
/** Given an input Epetra_CrsGraph, a "overlapped" Epetra_CrsGraph is generated
 * including rows associated with off processor contributions.
 */

class CrsGraph_Overlap : public StructuralSameTypeTransform<Epetra_CrsGraph> {

  const int levelOverlap_;

  const bool squareLocalBlock_;

  Epetra_BlockMap * OverlapMap_;

 public:

  ///
  /** Destructor
   */
  ~CrsGraph_Overlap();

  ///
  /** Constructor
   * input parameter overlap - number of levels of overlap
   * input parameter squareLocalBlock - whether to force the overlapped local domains
   * to be square by excluding the new columns associated with the overlapped rows
   */
  CrsGraph_Overlap( int overlap, bool squareLocalBlock = false )
  : levelOverlap_(overlap),
    squareLocalBlock_(squareLocalBlock),
    OverlapMap_(0)
  {}

  ///
  /** Generates the overlapped Epetra_CrsGraph from the input Epetra_CrsGraph
   */
  NewTypeRef operator()( OriginalTypeRef orig );

};

} //namespace EpetraExt

#endif //EDT_CRSGRAPH_OVERLAP_H
