/* ******************************************************************** */
/* See the file COPYRIGHT for a complete copyright notice, contact      */
/* and disclaimer.                                                      */
/* ******************************************************************** */

/* ******************************************************************** */
/* Declaration of the ML_Smoother structure                             */
/* ******************************************************************** */
/* Author        : Charles Tong (LLNL) and Raymond Tuminaro (SNL)       */
/* Date          : March, 1999                                          */
/* ******************************************************************** */

#ifndef __MLSMOOTHER__
#define __MLSMOOTHER__

/* ******************************************************************** */
/* data structure type definition                                       */
/* ******************************************************************** */

typedef struct ML_SmootherFunc_Struct ML_SmootherFunc;
typedef struct ML_Smoother_Struct ML_Smoother;
typedef struct ML_Sm_BGS_Data_Struct ML_Sm_BGS_Data;
typedef struct ML_Sm_ILUT_Data_Struct ML_Sm_ILUT_Data;
typedef struct ML_Sm_Schwarz_Data_Struct ML_Sm_Schwarz_Data;

/* ******************************************************************** */
/* local include files                                                  */
/* ******************************************************************** */

#include "ml_defs.h"
#include "ml_memory.h"
#include "ml_1level.h"
#include "ml_operator.h"
#include "ml_comminfoop.h"
#include <math.h>
#ifdef SUPERLU
#include "dsp_defs.h"
#include "util.h"
#elif DSUPERLU
#include "mpi.h"
#include "superlu_ddefs.h"
#endif

/* ******************************************************************** */
/* data definition for the ML_Smoother Class                            */
/* ******************************************************************** */
/* -------------------------------------------------------------------- */
/* These data structures define the smoother object.                    */
/* -------------------------------------------------------------------- */

struct ML_SmootherFunc_Struct 
{
   int ML_id;
   int (*internal)(void *, int, double *, int, double *);
   int (*external)(void *, int, double *, int, double *);
   void *data;
};

struct ML_Smoother_Struct 
{
   int                     ML_id;
   struct ML_1Level_Struct *my_level;
   int                     ntimes;
   int                     init_guess;
   double                  omega;
   double                  tol;
   ML_SmootherFunc         *smoother;
   void                    (*data_destroy)(void *);
   double                  build_time, apply_time;
   char                    *label;
};

struct ML_Sm_BGS_Data_Struct 
{
   double ** blockfacts;
   int    ** perms;
   int    blocksize;
   int    *blocklengths;
   int    *blockmap;
   int    Nblocks;
};

struct ML_Sm_ILUT_Data_Struct 
{
   int           Nrows;
   int           *mat_ia;
   int           *mat_ja;
   double        *mat_aa;
   ML_CommInfoOP *getrow_comm;
   int           fillin;
   double        threshold;
};

struct ML_Sm_Schwarz_Data_Struct 
{
   int           Nrows;
   int           **bmat_ia;
   int           **bmat_ja;
   double        **bmat_aa;
   int           **aux_bmat_ia;
   int           **aux_bmat_ja;
   double        **aux_bmat_aa;
   ML_CommInfoOP *getrow_comm;
   int           nblocks;
   int           *blk_info;
   int           *blk_size;
   int           **blk_indices;
   int           **perm_r;
   int           **perm_c;
#ifdef SUPERLU
   SuperMatrix   **slu_Amat;
   SuperMatrix   **slu_Lmat;
   SuperMatrix   **slu_Umat;
#endif
};

/* ******************************************************************** */
/* ******************************************************************** */
/*      User Interface Proto-types                                      */
/* ******************************************************************** */
/* ******************************************************************** */

#ifdef __cplusplus
extern "C" {
#endif
extern  int ML_Smoother_Create(ML_Smoother **, ML_1Level *level_ptr );
extern  int ML_Smoother_Init(ML_Smoother *, ML_1Level *level_ptr);
extern  int ML_Smoother_Destroy(ML_Smoother **);
extern  int ML_Smoother_Clean(ML_Smoother *);
extern  int ML_Smoother_Set_Label( ML_Smoother *smoo, char *label);
extern  int ML_Smoother_Apply(ML_Smoother *,int,double *,int,double*,int);
extern  int ML_Smoother_Set(ML_Smoother *, int, void *,
                 int (*internal)(void*,int,double*,int,double *),
                 int (*external)(void*,int,double*,int,double *),
                 int, double);
extern  int ML_Smoother_Jacobi(void *, int, double *x, int, double *);
extern  int ML_Smoother_GaussSeidel(void *, int, double *, int, double *);
extern  int ML_Smoother_SGS(void *, int, double *, int, double *);
extern  int ML_Smoother_BlockGS(void *, int, double *, int, double *);
extern  int ML_Smoother_ParaSails(void *, int, double *, int, double *);
extern  int ML_Smoother_ParaSailsSym(void *, int, double *, int, double *);
extern  int ML_Smoother_ParaSailsTrans(void *, int, double *, int, double *);
extern  int ML_Smoother_VBlockJacobi(void *,int,double *x,int, double *);
extern  int ML_Smoother_VBlockKrylovJacobi(void *,int,double*,int,double*);
extern  int ML_Smoother_VBlockSGS(void *, int, double *x, int, double *);
extern  int ML_Smoother_VBlockSGSSequential(void*,int,double*,int,double*);
extern  int ML_Smoother_OverlappedILUT(void *,int,double *x,int,double *);
extern  int ML_Smoother_VBlockAdditiveSchwarz(void *,int,double*,int,double*);
extern  int ML_Smoother_VBlockMultiplicativeSchwarz(void *,int,double*,int,double*);

/* ******************************************************************** */
/* ******************************************************************** */
/* priviate functions                                                   */
/* ******************************************************************** */
/* ******************************************************************** */

extern  int ML_Smoother_Create_BGS_Data(ML_Sm_BGS_Data **data);
extern  int ML_Smoother_Destroy_BGS_Data(ML_Sm_BGS_Data **data);
extern void ML_Smoother_Clean_BGS_Data(void *data);
extern  int ML_Smoother_Create_ILUT_Data(ML_Sm_ILUT_Data **data);
extern  int ML_Smoother_Destroy_ILUT_Data(ML_Sm_ILUT_Data **data);
extern  int ML_Smoother_Gen_BGSFacts(ML_Sm_BGS_Data **, ML_Operator *,int); 
extern  int ML_Smoother_Gen_VBGSFacts(ML_Sm_BGS_Data**,ML_Operator*,int,int*); 
extern  int ML_Smoother_Create_Schwarz_Data(ML_Sm_Schwarz_Data **data);
extern  int ML_Smoother_Destroy_Schwarz_Data(ML_Sm_Schwarz_Data **data);
extern void ML_Smoother_Clean_ParaSails(void *data);

extern  int ML_Smoother_ILUTDecomposition(ML_Sm_ILUT_Data *, ML_Operator *, 
                    ML_Comm *, int, int *,int*,double *,int *, int *,int);
extern  int ML_Smoother_VBlockSchwarzDecomposition(ML_Sm_Schwarz_Data *, 
                    ML_Operator *, ML_Comm *, int, int *,int*,double *,int *, 
                    int *,int);

extern  int ML_Smoother_GetOffProcRows(ML_CommInfoOP *, ML_Comm *, 
                  ML_Operator *,int,int *,int,int *,int *,int **,double **);
extern  int ML_Smoother_GetRowLengths(ML_CommInfoOP *, ML_Comm *, 
                  ML_Operator *, int *, int **);
extern  int ML_Smoother_ComposeOverlappedMatrix(ML_Operator *, ML_Comm *,
                  int *, int **, int **, double **, int **, int **, int *);

/* -------------------------------------------------------------------- */
/* Ray's functions                                                      */
/* -------------------------------------------------------------------- */

extern  int ML_MSR_SGSextra(void *, int , double *, int , double *);
extern void ML_MSR_GSextra_Clean(void *data);
extern  int ML_Smoother_BackGS(void *, int, double *, int, double *);
extern void ML_Smoother_Clean_OrderedSGS(void *data);
extern  int ML_Smoother_Gen_Ordering(ML_Operator *Amat, int **data_ptr);
extern  int ML_Smoother_OrderedSGS(void *sm,int inlen,double x[],int outlen,
                                   double rhs[]);
extern  int ML_Smoother_MSR_SGS(void *, int, double *, int, double *);
extern void ML_Smoother_Clean_MSR_GS(void *data);

#ifdef __cplusplus
}
#endif
#endif

