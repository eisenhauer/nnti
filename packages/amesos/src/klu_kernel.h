/* ========================================================================== */
/* === klu_kernel.h ========================================================= */
/* ========================================================================== */

/* This file should not be included in any user routine. */

typedef struct
{
    int j ;	    /* column j of L */
    int p ;	    /* Li [p..pend-1] currently being traversed */
    int pend ;

} WorkStackType ;


int klu_kernel
(
    /* input, not modified */
    int n,	    /* A is n-by-n */
    int Ap [ ],	    /* size n+1, column pointers for A */
    int Ai [ ],	    /* size nz = Ap [n], row indices for A */
    double Ax [ ],  /* size nz, values of A */
    int Q [ ],	    /* size n, optional input permutation */
    double tol,	    /* partial pivoting tolerance parameter */

    /* input and output */
    int *p_lsize,
    int *p_usize,

    /* output, not defined on input */
    int Lp [ ],	    /* size n+1 */
    int **p_Li,	    /* size lsize */
    double **p_Lx,  /* size lsize */
    int Up [ ],	    /* size n+1 */
    int **p_Ui,	    /* size usize */
    double **p_Ux,  /* size usize */
    int Pinv [ ],   /* size n */
    int P [ ],	    /* size n */
    int *p_noffdiag,	/* # of off-diagonal pivots chosen */

    /* workspace, not defined on input or output */
    double X [ ],   /* size n */
    int Stack [ ],  /* size n */
    int Flag [ ],  /* size n */

    /* workspace for non-recursive version only */
    WorkStackType WorkStack [ ],    /* size n */

    /* workspace for pruning only */
    int Lpend [ ],	/* size n workspace */
    int Lpruned [ ]	/* size n workspace */
) ;

/* ========================================================================== */
/* make sure debugging is turned off */
#ifndef NDEBUG
#define NDEBUG
#endif
/* To enable debugging, uncomment this line:
#undef NDEBUG
*/
/* ========================================================================== */

#ifdef MATLAB_MEX_FILE
#include "matrix.h"
#include "mex.h"
#define ASSERT(a) mxAssert(a, "")
#define ALLOCATE mxMalloc
#define _REALLOC mxRealloc
#define _FREE mxFree
#else
#include <stdio.h>
#include <assert.h>
#define ASSERT(a) assert(a)
#define ALLOCATE malloc
#define _REALLOC realloc
#define _FREE free
#endif

#include <stdlib.h>

#define REALLOCATE(p,type,size,ok) \
    { \
	type *pnew ; \
	size_t s ; \
	s = (size_t) ((sizeof (type)) * size) ; \
	pnew = (type *) _REALLOC ((void *) p, s) ; \
	ok = (pnew != (type *) NULL) ; \
	if (ok) \
	{ \
	    p = pnew ; \
	} \
    }

#define FREE(p,type) \
    { \
	if (p != (type *) NULL) \
	{ \
	    _FREE (p) ; \
	    p = (type *) NULL ; \
	} \
    }

#define SCALAR_IS_NAN(x) ((x) != (x))


#ifndef INT_MAX
#define INT_MAX 0x7fffffff
#endif
/* true if an integer (stored in double x) would overflow (or if x is NaN) */
#define INT_OVERFLOW(x) ((!((x) * (1.0+1e-8) <= (double) INT_MAX)) \
			|| SCALAR_IS_NAN (x))

#undef TRUE
#undef FALSE
#undef MAX
#undef MIN
#undef ABS
#undef PRINTF
#undef FLIP

#ifndef NDEBUG
#define PRINTF(s) { printf s ; } ;
#else
#define PRINTF(s)
#endif

#define TRUE 1
#define FALSE 0
#define MAX(a,b) (((a) > (b)) ?  (a) : (b))
#define MIN(a,b) (((a) < (b)) ?  (a) : (b))
#define ABS(a)   (((a) <  0 ) ? -(a) : (a))

/* FLIP is a "negation about -1", and is used to mark an integer i that is
 * normally non-negative.  FLIP (EMPTY) is EMPTY.  FLIP of a number > EMPTY
 * is negative, and FLIP of a number < EMTPY is positive.  FLIP (FLIP (i)) = i
 * for all integers i.  UNFLIP (i) is >= EMPTY. */
#define EMPTY (-1)
#define FLIP(i) (-(i)-2)
#define UNFLIP(i) ((i < EMPTY) ? FLIP (i) : (i))
