/*
// @HEADER
// 
// ***********************************************************************
// 
//      Teko: A package for block and physics based preconditioning
//                  Copyright 2010 Sandia Corporation 
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
// Questions? Contact Eric C. Cyr (eccyr@sandia.gov)
// 
// ***********************************************************************
// 
// @HEADER
*/

#include "Teko_DiagnosticLinearOp.hpp"

#include "Teuchos_TimeMonitor.hpp"

namespace Teko {

/** \brief This constructor explicitly takes the linear operator
  *        that needs to be wrapped and a string for output that describes
  *        the diagnostics.
  */
DiagnosticLinearOp::DiagnosticLinearOp(Teuchos::RCP<std::ostream> & ostrm, const LinearOp & A,const std::string & diagnosticString)
   : outputStream_(ostrm), wrapOpA_(A), diagString_(diagnosticString), timer_(diagnosticString)
{
}

DiagnosticLinearOp::~DiagnosticLinearOp()
{
   double elapsedTime = totalTime();
   int applications = numApplications();

   (*outputStream_) << "DiagnosticLinearOp \"" << diagString_ << "\": "
                    << "elapsed = " << elapsedTime << ", "
                    << "applications = " << applications << ", ";
   if(applications>0)
      (*outputStream_) << "timer/app = " << elapsedTime / double(applications) << std::endl;
   else
      (*outputStream_) << "timer/app = " << "none" << std::endl;
}

/** @brief Perform a matrix vector multiply with this operator. 
  *
  * The <code>apply</code> function takes one vector as input 
  * and applies the inverse \f$ LDU \f$ decomposition. The result
  * is returned in \f$y\f$. If this operator is reprsented as \f$M\f$ then
  * \f$ y = \alpha M x + \beta y \f$ (ignoring conjugation!).
  *
  * @param[in]     x 
  * @param[in,out] y 
  * @param[in]     alpha (default=1)
  * @param[in]     beta  (default=0)
  */
void DiagnosticLinearOp::implicitApply(const MultiVector & x, MultiVector & y,
                                       const double alpha, const double beta) const
{
   // start timer on construction, end on destruction
   Teuchos::TimeMonitor monitor(timer_,false);  

   wrapOpA_->apply(Thyra::NOTRANS,*x,y.ptr(),alpha,beta);
}

} // end namespace Teko
