
/*! @HEADER */
/*
************************************************************************

                CTrilinos:  C interface to Trilinos
                Copyright (2009) Sandia Corporation

Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
license for use of this work by or on behalf of the U.S. Government.

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA
Questions? Contact M. Nicole Lemaster (mnlemas@sandia.gov)

************************************************************************
*/
/*! @HEADER */


#include "CTrilinos_config.h"


#ifdef HAVE_CTRILINOS_AZTECOO



/*! @file CAztecOO_StatusTestResNorm.h
 * @brief Wrappers for AztecOO_StatusTestResNorm */

/* True C header file! */


#ifndef CAZTECOO_STATUSTESTRESNORM_H
#define CAZTECOO_STATUSTESTRESNORM_H


#include "CEpetra_Operator.h"
#include "CEpetra_Vector.h"
#include "CEpetra_MultiVector.h"
#include "CTrilinos_enums.h"


#ifdef __cplusplus
extern "C" {
#endif



/*! @name AztecOO_StatusTestResNorm constructor wrappers */
/*@{*/

/*! @brief Wrapper for 
   AztecOO_StatusTestResNorm::AztecOO_StatusTestResNorm(const Epetra_Operator & Operator, const Epetra_Vector & LHS, const Epetra_Vector & RHS,double Tolerance)
*/
CT_AztecOO_StatusTestResNorm_ID_t AztecOO_StatusTestResNorm_Create ( 
  CT_Epetra_Operator_ID_t OperatorID, CT_Epetra_Vector_ID_t LHSID, 
  CT_Epetra_Vector_ID_t RHSID, double Tolerance );

/*@}*/

/*! @name AztecOO_StatusTestResNorm destructor wrappers */
/*@{*/

/*! @brief Wrapper for 
   virtual AztecOO_StatusTestResNorm::~AztecOO_StatusTestResNorm()
*/
void AztecOO_StatusTestResNorm_Destroy ( 
  CT_AztecOO_StatusTestResNorm_ID_t * selfID );

/*@}*/

/*! @name AztecOO_StatusTestResNorm member wrappers */
/*@{*/

/*! @brief Wrapper for 
   int AztecOO_StatusTestResNorm::DefineResForm( ResType TypeOfResidual, NormType TypeOfNorm, Epetra_Vector * Weights = 0)
*/
int AztecOO_StatusTestResNorm_DefineResForm ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID, 
  CT_ResType_E_t TypeOfResidual, CT_NormType_E_t TypeOfNorm, 
  CT_Epetra_Vector_ID_t WeightsID );

/*! @brief Wrapper for 
   int AztecOO_StatusTestResNorm::DefineScaleForm( ScaleType TypeOfScaling, NormType TypeOfNorm, Epetra_Vector * Weights = 0, double ScaleValue = 1.0)
*/
int AztecOO_StatusTestResNorm_DefineScaleForm ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID, 
  CT_ScaleType_E_t TypeOfScaling, CT_NormType_E_t TypeOfNorm, 
  CT_Epetra_Vector_ID_t WeightsID, double ScaleValue );

/*! @brief Wrapper for 
   int AztecOO_StatusTestResNorm::ResetTolerance(double Tolerance)
*/
int AztecOO_StatusTestResNorm_ResetTolerance ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID, double Tolerance );

/*! @brief Wrapper for 
   int AztecOO_StatusTestResNorm::SetMaxNumExtraIterations(int maxNumExtraIterations)
*/
int AztecOO_StatusTestResNorm_SetMaxNumExtraIterations ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID, 
  int maxNumExtraIterations );

/*! @brief Wrapper for 
   int AztecOO_StatusTestResNorm::GetMaxNumExtraIterations()
*/
int AztecOO_StatusTestResNorm_GetMaxNumExtraIterations ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID );

/*! @brief Wrapper for 
   bool AztecOO_StatusTestResNorm::ResidualVectorRequired() const
*/
boolean AztecOO_StatusTestResNorm_ResidualVectorRequired ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID );

/*! @brief Wrapper for 
   AztecOO_StatusType AztecOO_StatusTestResNorm::CheckStatus(int CurrentIter, Epetra_MultiVector * CurrentResVector, double CurrentResNormEst, bool SolutionUpdated)
*/
CT_AztecOO_StatusType_E_t AztecOO_StatusTestResNorm_CheckStatus ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID, int CurrentIter, 
  CT_Epetra_MultiVector_ID_t CurrentResVectorID, 
  double CurrentResNormEst, boolean SolutionUpdated );

/*! @brief Wrapper for 
   AztecOO_StatusType AztecOO_StatusTestResNorm::GetStatus() const
*/
CT_AztecOO_StatusType_E_t AztecOO_StatusTestResNorm_GetStatus ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID );

/*! @brief Wrapper for 
   void AztecOO_StatusTestResNorm::ResetStatus()
*/
void AztecOO_StatusTestResNorm_ResetStatus ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID );

/*! @brief Wrapper for 
   double AztecOO_StatusTestResNorm::GetTolerance() const
*/
double AztecOO_StatusTestResNorm_GetTolerance ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID );

/*! @brief Wrapper for 
   double AztecOO_StatusTestResNorm::GetTestValue() const
*/
double AztecOO_StatusTestResNorm_GetTestValue ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID );

/*! @brief Wrapper for 
   double AztecOO_StatusTestResNorm::GetResNormValue() const
*/
double AztecOO_StatusTestResNorm_GetResNormValue ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID );

/*! @brief Wrapper for 
   double AztecOO_StatusTestResNorm::GetScaledNormValue() const
*/
double AztecOO_StatusTestResNorm_GetScaledNormValue ( 
  CT_AztecOO_StatusTestResNorm_ID_t selfID );

/*@}*/


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* CAZTECOO_STATUSTESTRESNORM_H */

#endif /* HAVE_CTRILINOS_AZTECOO */


