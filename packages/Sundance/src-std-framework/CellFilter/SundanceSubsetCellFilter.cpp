/* @HEADER@ */
// ************************************************************************
// 
//                              Sundance
//                 Copyright (2005) Sandia Corporation
// 
// Copyright (year first published) Sandia Corporation.  Under the terms 
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government 
// retains certain rights in this software.
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
// Questions? Contact Kevin Long (krlong@sandia.gov), 
// Sandia National Laboratories, Livermore, California, USA
// 
// ************************************************************************
/* @HEADER@ */

#include "SundanceSubsetCellFilter.hpp"
#include "SundanceExplicitCellSet.hpp"
#include "SundanceExceptions.hpp"
#include "SundanceOrderedTuple.hpp"
#include "SundanceOut.hpp"

using namespace SundanceStdFwk;
using namespace SundanceStdFwk::Internal;
using namespace SundanceCore::Internal;
using namespace Teuchos;

SubsetCellFilter::SubsetCellFilter(const CellFilter& superset,
                                   const CellPredicate& predicate)
  : CellFilterBase(), superset_(superset), predicate_(predicate)
{;}


XMLObject SubsetCellFilter::toXML() const 
{
  XMLObject rtn("SubsetCellFilter");
  rtn.addAttribute("id", Teuchos::toString(id()));
  rtn.addChild(predicate_.toXML());
  return rtn;
}

bool SubsetCellFilter::lessThan(const CellFilterStub* other) const
{
  const SubsetCellFilter* S 
    = dynamic_cast<const SubsetCellFilter*>(other);

  TEST_FOR_EXCEPTION(S==0,
                     InternalError,
                     "argument " << other->toXML() 
                     << " to SubsetCellFilter::lessThan() should be "
                     "a SubsetCellFilter pointer.");

  return OrderedPair<CellFilter, CellPredicate>(superset_, predicate_)
    < OrderedPair<CellFilter, CellPredicate>(S->superset_, S->predicate_);
}

CellSet SubsetCellFilter::internalGetCells(const Mesh& mesh) const
{
  SUNDANCE_OUT(this->verbosity() > VerbLow,
                   "SubsetCellFilter::internalGetCells()");
  CellSet super = superset_.getCells(mesh);

  int dim = superset_.dimension(mesh);

  CellType cellType = mesh.cellType(dim);

  predicate_.setMesh(mesh, dim);

  ExplicitCellSet* rtn = new ExplicitCellSet(mesh, dim, cellType);

  Set<int>& cells = rtn->cells();

  const CellPredicateBase* pred = predicate_.ptr().get();

  
  for (CellIterator i=super.begin(); i != super.end(); i++)
    {
      int LID = *i;
      SUNDANCE_OUT(this->verbosity() > VerbMedium,
                   "SubsetCellFilter is testing " << LID);
      if (pred->test(LID)) 
        {
          SUNDANCE_OUT(this->verbosity() > VerbMedium,
                       "accepted " << LID);
          cells.insert(LID);
        }
      else
        {
          SUNDANCE_OUT(this->verbosity() > VerbMedium,
                       "rejected " << LID);
        }
    }

  return rtn;
}
