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

#ifndef BFGS_UPDATE_STRATEGY_H
#define BFGS_UPDATE_STRATEGY_H

#include "MoochoPack_Types.hpp"
#include "MoochoPack_QuasiNewtonStats.hpp"
#include "Teuchos_StandardMemberCompositionMacros.hpp"

namespace MoochoPack {

/** \brief Strategy interface which contains the guts for a dampened BFGS update.
 *
 * This object can not change the flow of control or do anything fancy.  It just
 * performs the dampened update or skips it if the update is not sufficiently
 * positive definite.
 *
 * See the printed documentation generated by \c this->print_step().
 */
class BFGSUpdate_Strategy {
public:

  /** \brief <<std member comp>> members for whether to rescale
   * the initial identity Hessian or not.
   */
  STANDARD_MEMBER_COMPOSITION_MEMBERS( bool, rescale_init_identity );

  /** \brief <<std member comp>> members for whether to perform
    * dampended quasi-newton updating or not.
    */
  STANDARD_MEMBER_COMPOSITION_MEMBERS( bool, use_dampening );

  /** \brief . */
  enum ESecantTesting { SECANT_TEST_DEFAULT, SECANT_TEST_ALWAYS, SECANT_NO_TEST };

  /** \brief <<std member comp>> members how and if the secant property of the BFGS
    * update is tested.
    *
    * ToDo: Finish documentation.
    */
  STANDARD_MEMBER_COMPOSITION_MEMBERS( ESecantTesting, secant_testing );

  /** \brief <<std member comp>> members for the warning tolerance for
    * the check of the secant property.
    */
  STANDARD_MEMBER_COMPOSITION_MEMBERS( value_type, secant_warning_tol );

  /** \brief <<std member comp>> members for the error tolerance for
    * the check of the secant property.
    */
  STANDARD_MEMBER_COMPOSITION_MEMBERS( value_type, secant_error_tol );

  /** \brief . */
  BFGSUpdate_Strategy(
    bool               rescale_init_identity  = true
    ,bool              use_dampening          = true
    ,ESecantTesting    secant_testing         = SECANT_TEST_DEFAULT
    ,value_type        secant_warning_tol     = 1e-6
    ,value_type        secant_error_tol       = 1e-1
    );

  /** \brief Perform the BFGS update.
   *
   * The function performs a straight forward (possibly dampended) BFGS update
   * that strictly satisfies the secant property <tt>B * s_bfgs = y_bfgs</tt>.
   *
   * See the printed documentation generated by \c this->print_step().
   *
   * Preconditions:<ul>
   * <li> <tt>s_bfgs->size() == y_bfgs->size() == B->rows() == B->cols()</tt> (throws ???)
   * </ul>
   *
   * @param  s_bfgs  [in/w] Secant change vector on input.  May be modified as
   *                 modified as workspace.
   * @param  y_bfgs  [in/w] Secant change vector on input.  May be modified as
    *                 modified as workspace.
   * @param  first_update
   *                 [in] If true then this is the first update after <tt>B</tt> was
   *                 initialized to identity.  In this case <tt>B</tt> will be rescaled
   *                 as <tt>B = Iscale * I</tt> before the update is performed if
   *                 <tt>this->rescale_init_identity() == true</tt>.
   * @param  out     [out] Output stream journal data is written to.
   * @param  olevel  [in] Output level for printing to <tt>out</tt>.
   * @param  check_results
   *                 [in] Helps determine if the secant property is tested or not
   *                 after the update (see the printed documentation).
   * @param  B       [in/out] The matrix to be updated.  <tt>B</tt> must support the
   *                 <tt>MatrixSymSecant</tt> interface or an exception will be thrown.
   * @param  quasi_newton_stats
   *                 [out] The quasi-newton statistics object that is updated to
   *                 inform what happened durring the update.
   */
  void perform_update(
    VectorMutable      *s_bfgs
    ,VectorMutable     *y_bfgs
    ,bool                    first_update
    ,std::ostream            &out
    ,EJournalOutputLevel     olevel
    ,bool                    check_results
    ,MatrixSymOp         *B
    ,QuasiNewtonStats        * quasi_newton_stats 
    );
  
  /** \brief . */
  void print_step( std::ostream& out, const std::string& leading_str ) const;

}; // end class BFGSUpdate_Strategy

}  // end namespace MoochoPack

#endif // BFGS_UPDATE_STRATEGY_H
