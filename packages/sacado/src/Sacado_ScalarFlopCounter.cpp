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

#include <cassert>
#include <iomanip>
#include <vector>

#include "Sacado_ScalarFlopCounter.hpp"

// Initialization of static members
const char* 
Sacado::FlopCounterPack::FlopCounts::flopCountsNames[] = 
{
  "="
  ,"+"
  ,"+="
  ,"unary +"
  ,"-"
  ,"-="
  ,"unary -"
  ,"*"
  ,"*="
  ,"/"
  ,"/="
  ,">"
  ,">="
  ,"<"
  ,"<="
  ,"=="
  ,"exp"
  ,"log"
  ,"log10"
  ,"sqrt"
  ,"cos"
  ,"sin"
  ,"tan"
  ,"acos"
  ,"asin"
  ,"atan"
  ,"atan2"
  ,"cosh"
  ,"sinh"
  ,"tanh"
  ,"abs"
  ,"pow"
  ,"max"
  ,"min"
};
const char*
Sacado::FlopCounterPack::FlopCounts::summaryFlopCountsNames[] =
{
  "="
  ,"all +-"
  ,"all *"
  ,"all /"
  ,"<,>,=="
  ,"nonlinear"
};
unsigned int 
Sacado::FlopCounterPack::FlopCounts::flopGranularity = 100000000;

Sacado::FlopCounterPack::FlopCounts::FlopCounts()
{
  reset();
}

void 
Sacado::FlopCounterPack::FlopCounts::reset()
{
  std::fill_n( &partialFlopCounts[0], int(NUM_OPS), uint(0) );
  std::fill_n( &partialSummaryFlopCounts[0], int(NUM_SUMMARY_OPS), uint(0) );
  std::fill_n( &flopCounts[0], int(NUM_OPS), double(0) );
  std::fill_n( &summaryFlopCounts[0], int(NUM_SUMMARY_OPS), double(0) );
  totalFlopCount = 0.0;
}

void 
Sacado::FlopCounterPack::FlopCounts::finalize()
{
  for (int i=0; i<NUM_OPS; i++) {
    flopCounts[i] += static_cast<double>(partialFlopCounts[i]);
    partialFlopCounts[i] = 0;
  }
  for (int i=0; i<NUM_SUMMARY_OPS; i++) {
    summaryFlopCounts[i] += static_cast<double>(partialSummaryFlopCounts[i]);
    partialSummaryFlopCounts[i] = 0;
  }
  totalFlopCount = 0;
  for (int i=0; i<NUM_OPS; i++)
    totalFlopCount += flopCounts[i];
}

void
Sacado::FlopCounterPack::FlopCounts::increment(Sacado::FlopCounterPack::FlopCounts::EFlopType ft)
{
  ESummaryFlopType sft = getSummaryType(ft);
  if (partialFlopCounts[ft] > flopGranularity) {
    flopCounts[ft] += static_cast<double>(partialFlopCounts[ft]);
    partialFlopCounts[ft] =0;
  }
  if (partialSummaryFlopCounts[sft] > flopGranularity) {
    summaryFlopCounts[sft] += 
      static_cast<double>(partialSummaryFlopCounts[sft]);
    partialSummaryFlopCounts[sft] = 0;
  }
  ++partialFlopCounts[ft];
  ++partialSummaryFlopCounts[sft];
}

Sacado::FlopCounterPack::FlopCounts::ESummaryFlopType 
Sacado::FlopCounterPack::FlopCounts::getSummaryType(Sacado::FlopCounterPack::FlopCounts::EFlopType ft)
{
  switch(ft) {
    case ASSIGN:
      return SUMMARY_ASSIGN;
      break;
    case PLUS:
    case PLUS_ASSIGN:
    case UNARY_PLUS:
    case MINUS:
    case MINUS_ASSIGN:
    case UNARY_MINUS:
      return SUMMARY_PLUS_MINUS;
      break;
    case MULTIPLY:
    case MULTIPLY_ASSIGN:
      return SUMMARY_MULTIPLY;
      break;
    case DIVIDE:
    case DIVIDE_ASSIGN:
      return SUMMARY_DIVIDE;
      break;
    case EXP:
    case LOG:
    case LOG10:
    case SQRT:
    case COS:
    case SIN:
    case TAN:
    case ACOS:
    case ASIN:
    case ATAN:
    case ATAN2:
    case COSH:
    case SINH:
    case TANH:
    case ABS:
    case POW:
    case MAX:
    case MIN:
      return SUMMARY_NONLINEAR;
      break;
    case GREATER_THAN:
    case GREATER_THAN_EQUAL:
    case LESS_THAN:
    case LESS_THAN_EQUAL:
    case EQUAL:
      return SUMMARY_COMPARISON;
      break;
    default:
      assert(0);
  }
}

std::ostream& 
Sacado::FlopCounterPack::printCountersTable(const int n, 
					    const char* names[], 
					    const char* abbr[],
					    const FlopCounts counts[], 
					    std::ostream &out)
{
  assert( n >= 1 && names && abbr && counts );
  const int wo = 10;
  const int wc = 20;
  const char spacero[] = "----------";
  const char spacerc[] = "--------------------";
  // Print legend
  if(names) {
    out << "\nLegend\n------\n";
    for( int j = 0; j < n; ++j )
      out << "  " << abbr[j] << " = " << names[j] << std::endl;
    out << std::endl;
  }
  // Print table header
  out << std::left << "  " << std::setw(wo) << "op\\count";
  for( int j = 0; j < n; ++j ) out << "  " << std::setw(wc) << abbr[j];
  out << std::endl;
  out << std::right << "  " << std::setw(wo) << spacero;
  for( int j = 0; j < n; ++j ) out << "  " << std::setw(wc) << spacerc;
  out << std::endl;
  // Print rows of all operation counts
  for( int i = 0; i < FlopCounts::NUM_OPS; ++i ) {
    double theseFlops = 0;
    for( int j = 0; j < n; ++j ) theseFlops += counts[j].flopCounts[i];
    if(theseFlops) {
      out << "  " << std::setw(wo) << FlopCounts::flopCountsNames[i];
      for( int j = 0; j < n; ++j ) out << "  " << std::setw(wc) << counts[j].flopCounts[i];
      out << std::endl;
    }
  }
  out << std::right << "  " << std::setw(wo) << spacero;
  for( int j = 0; j < n; ++j ) out << "  " << std::setw(wc) << spacerc;
  out << std::endl;
  // Print summary rows
  std::vector<double> totalFlops(n);
  for( int i = 0; i < FlopCounts::NUM_SUMMARY_OPS; ++i ) {
    double theseFlops = 0;
    for( int j = 0; j < n; ++j ) {
      const double flops = counts[j].summaryFlopCounts[i];
      theseFlops += flops;
      totalFlops[j] += flops;
    }
    if(theseFlops) {
      out << "  " << std::setw(wo) << FlopCounts::summaryFlopCountsNames[i];
      for( int j = 0; j < n; ++j )
	out << "  " << std::setw(wc) << counts[j].summaryFlopCounts[i];
      out << std::endl;
    }
  }
  out << std::right << "  " << std::setw(wo) << spacero;
  for( int j = 0; j < n; ++j ) out << "  " << std::setw(wc) << spacerc;
  out << std::endl;
  // Print total flops
  out << "  " << std::setw(wo) << "all flops";
  for( int j = 0; j < n; ++j ) out << "  " << std::setw(wc) << totalFlops[j];
  out << std::endl;
  //
  return out;
}
