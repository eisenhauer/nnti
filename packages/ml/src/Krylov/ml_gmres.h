/* ******************************************************************** */
/* See the file COPYRIGHT for a complete copyright notice, contact      */
/* person and disclaimer.                                               */        
/* ******************************************************************** */

/* ******************************************************************** */
/* Functions for the GMRES Krylov solver                                */
/* ******************************************************************** */
/* Author        : Charles Tong (LLNL)                                  */
/* Date          : December, 1999                                       */
/* ******************************************************************** */

#include "ml_common.h"
#include "ml_krylov.h"

#ifndef __MLGMRES__
#define __MLGMRES__

#ifndef ML_CPP
#ifdef __cplusplus
extern "C" {
#endif
#endif

extern int ML_GMRES_Solve(ML_Krylov *,int length,double *rhs,double *sol);

#ifndef ML_CPP
#ifdef __cplusplus
}
#endif
#endif
#endif

