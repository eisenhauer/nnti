#ifndef __MLXYT__
#define __MLXYT__

#include "ml_operator.h"

#ifndef ML_CPP
#ifdef __cplusplus
extern "C" {
#endif
#endif



extern void setup_henry(ML *my_ml, int grid0, int **imapper, int **separator,
        int **sep_size, int *Nseparators, int *Nlocal, int *Nghost,
        ML_Operator **matvec_data);

int CSR_submv(ML_Operator *Amat, double p[], double ap[]);
int CSR_submatvec(ML_Operator *Amat, double p[], double ap[], int mask);
int ML_submv(ML_Operator *Amat, double p[], double ap[]);
int ML_submatvec(ML_Operator *Amat, double p[], double ap[], int mask);
int ML_Gen_CoarseSolverXYT( ML *ml, int i);
void ML_XYTfree(void *temp);
extern void ML_subexchange_bdry(double x[], ML_CommInfoOP *comm_info,
                      int start_location, int total_send, ML_Comm *comm,
                      int mask);
extern int ML_gpartialsum_int(int val, ML_Comm *comm);

extern int ML_Comm_subGappendInt(ML_Comm *com_ptr, int *vals, int *cur_length, 
                    int total_length,int sub_mask);

#ifndef ML_CPP
#ifdef __cplusplus
}
#endif
#endif

#endif
