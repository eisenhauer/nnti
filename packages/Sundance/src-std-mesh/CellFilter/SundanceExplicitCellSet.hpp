/* @HEADER@ */
// ************************************************************************
// 
//                             Sundance
//                 Copyright 2011 Sandia Corporation
// 
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Kevin Long (kevin.long@ttu.edu)
// 

/* @HEADER@ */

#ifndef SUNDANCE_EXPLICITCELLSET_H
#define SUNDANCE_EXPLICITCELLSET_H

#include "SundanceDefs.hpp"
#include "SundanceCellSetBase.hpp"

namespace Sundance
{
using namespace Teuchos;

/** 
 * ExplicitCellSet is a cell set subtype where the cell LIDs
 * are stored explicitly in an STL set. 
 * 
 * @see CellFilter, CellSet, CellSetBase, CellIterator 
 **/
class ExplicitCellSet : public CellSetBase
{
public:

  /** Construct with a mesh, initializing to an empty set */
  ExplicitCellSet(const Mesh& mesh, int cellDim,
    const CellType& cellType);

  /** Construct with a set of cells */
  ExplicitCellSet(const Mesh& mesh, int cellDim,
    const CellType& cellType,
    const Set<int>& cellLIDs);

  /** Returns an iterator pointing to the first element
   * in the set. */
  virtual CellIterator begin() const ;

  /** Returns a past-the-end iterator */
  virtual CellIterator end() const ;

  /** Returns a modifiable reference to the set of cells */
  Set<int>& cells() {return cells_;}

  /** */
  bool internalLessThan(const CellSetBase* other) const ;

  /** \name Printable interface */
  //@{
  /** Print to a stream */
  virtual void print(std::ostream& os) const ;
  //@}

  /* Handleable interface */
  GET_RCP(CellSetBase);

private:

  /** The set of cell LIDs */
  Set<int> cells_;

      
};
}


#endif
