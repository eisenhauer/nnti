/* ******************************************************************** */
/* See the file COPYRIGHT for a complete copyright notice, contact      */
/* person and disclaimer.                                               */        
/* ******************************************************************** */

#ifndef __MLOPERATOR_BLOCKMAT__
#define __MLOPERATOR_BLOCKMAT__

#include "ml_operator.h"
/*****************************************************************************/
/* structure to wrap ML subblock matrices into a large ML matrix.            */
/*****************************************************************************/

struct ML_Operator_blockmat_data {
  /* Ke functions for matvec & getrow */

  int (*Ke_matvec)(void *, int, double *, int, double *);
  int (*Ke_getrow)(void *,int,int*,int,int*,double*,int*);
  void *Ke_matvec_data, *Ke_getrow_data, *Ke_comm_data;
  double *Ke_diag;

  /* M functions for matvec, getrow. NOTE: it is assumed    */
  /* that M's communication is identical to Ke's.           */

  int (*M_matvec)(void *, int, double *, int, double *);
  int (*M_getrow)(void *,int,int*,int,int*,double*,int*);
  void *M_matvec_data, *M_getrow_data;
  double *M_diag;

  int N_Ke, Nghost;
  int *cols;         /* work vectors for block matrix getrow */
  double *vals;
};

#ifdef __cplusplus
extern "C" {
#endif

extern int ML_Operator_blockmat_matvec(void *, int , double *, int, double *);
extern int ML_Operator_blockmat_comm( double *x, void *data);
extern int ML_Operator_blockmat_getrow(void *, int, int *, int, int *, 
				       double *, int *);
extern int  ML_Operator_Gen_blockmat(ML_Operator *blockmat, 
				     ML_Operator *original1,
				     ML_Operator *original2);
extern void  ML_Operator_blockmatdata_Destroy(void *data);
#ifdef __cplusplus
}
#endif


#endif
