/* ***************************************************************** 
    MESQUITE -- The Mesh Quality Improvement Toolkit

    Copyright 2006 Sandia National Laboratories.  Developed at the
    University of Wisconsin--Madison under SNL contract number
    624796.  The U.S. Government and the University of Wisconsin
    retain certain rights to this software.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License 
    (lgpl.txt) along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 
    (2006) kraftche@cae.wisc.edu
   
  ***************************************************************** */


/** \file ElementPMeanP.hpp
 *  \brief 
 *  \author Jason Kraftcheck 
 */

#ifndef MSQ_ELEMENT_PMEAN_P_HPP
#define MSQ_ELEMENT_PMEAN_P_HPP

#include "Mesquite.hpp"
#include "PMeanPMetric.hpp"
#include "ElementQM.hpp"

namespace Mesquite {

class ElemSampleQM;

class ElementPMeanP : public ElementQM, public PMeanPMetric
{
public:

  ElementPMeanP( double p, ElemSampleQM* metric );
  
  virtual ~ElementPMeanP();
  
  ElemSampleQM* get_quality_metric() const 
    { return mMetric; }
  
  virtual msq_std::string get_name() const;

  virtual int get_negate_flag() const;

  virtual
  bool evaluate( PatchData& pd, 
                 size_t handle, 
                 double& value, 
                 MsqError& err );

  virtual
  bool evaluate_with_gradient( PatchData& pd,
                 size_t handle,
                 double& value,
                 msq_std::vector<size_t>& indices,
                 msq_std::vector<Vector3D>& gradient,
                 MsqError& err );

  virtual
  bool evaluate_with_Hessian( PatchData& pd,
                 size_t handle,
                 double& value,
                 msq_std::vector<size_t>& indices,
                 msq_std::vector<Vector3D>& gradient,
                 msq_std::vector<Matrix3D>& Hessian,
                 MsqError& err );
     
  virtual
  bool evaluate_with_Hessian_diagonal( PatchData& pd,
                 size_t handle,
                 double& value,
                 msq_std::vector<size_t>& indices,
                 msq_std::vector<Vector3D>& gradient,
                 msq_std::vector<SymMatrix3D>& Hessian_diagonal,
                 MsqError& err );
private:

  ElemSampleQM* mMetric;
  mutable msq_std::vector<size_t> mHandles;
};  


} // namespace Mesquite

#endif
