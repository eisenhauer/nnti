// @HEADER
// ***********************************************************************
// 
// Moocho: Multi-functional Object-Oriented arCHitecture for Optimization
//                  Copyright (2003) Sandia Corporation
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
// Questions? Contact Roscoe A. Bartlett (rabartl@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef DIRECT_SPARSE_SOLVER_MA28_SET_OPTIONS_H
#define DIRECT_SPARSE_SOLVER_MA28_SET_OPTIONS_H

#include "AbstractLinAlgPack_DirectSparseSolverMA28.hpp"
#include "OptionsFromStreamPack_SetOptionsFromStreamNode.hpp"
#include "OptionsFromStreamPack_SetOptionsToTargetBase.hpp"

namespace AbstractLinAlgPack {

///
/** Set options for DirectSparseSolverMA28 from
  * OptionsFromStream object.
  *
  * The options group is:
  *
  \begin{verbatim}
  options_group DirectSparseSolverMA28 {
      estimated_fillin_ratio = 10.0;
      u = 0.1;
      grow = true;
      tol = 0.0;
      nsrch = 4;
      lbig = false;
      print_ma28_outputs = false;
      output_file_name = NONE;
  }
  \end{verbatim}
  *
  * See MA28 documentation for a description of these options.
  */
class DirectSparseSolverMA28SetOptions
  : public OptionsFromStreamPack::SetOptionsFromStreamNode 
    , public OptionsFromStreamPack::SetOptionsToTargetBase<
      DirectSparseSolverMA28 >
{
public:

  ///
  DirectSparseSolverMA28SetOptions(
    DirectSparseSolverMA28* qp_solver = 0 );

protected:

  /// Overridden from SetOptionsFromStreamNode
  void setOption( int option_num, const std::string& option_value );

};	// end class DirectSparseSolverMA28SetOptions

}	// end namespace AbstractLinAlgPack 

#endif	// DIRECT_SPARSE_SOLVER_MA28_SET_OPTIONS_H
