/* ========================================================================== */
/* === klu_solve ============================================================ */
/* ========================================================================== */

/* Solve Ax=b using the symbolic and numeric objects from klu_analyze
 * (or klu_analyze_given) and klu_factor.  Note that no iterative refinement is
 * performed.  Uses Numeric->Xwork as workspace (undefined on input and output),
 * of size 4n Entry's (note that columns 2 to 4 of Xwork overlap with
 * Numeric->Iwork).
 */

#include "klu_internal.h"

int KLU_solve
(
    /* inputs, not modified */
    klu_symbolic *Symbolic,
    klu_numeric *Numeric,
    int d,		    /* leading dimension of B */
    int nrhs,		    /* number of right-hand-sides */

    /* right-hand-side on input, overwritten with solution to Ax=b on output */
    double B [ ],	    /* size n*nrhs, in column-oriented form, with
			     * leading dimension d. */
    /* --------------- */
    klu_common *Common
)
{
    Entry x [4], offik, s ;
    double rs, *Rs ;
    Entry *Singleton, *Offx, *X, *Bz ;
    int *Q, *R, *Pnum, *Offp, *Offi ;
    int **Lbip, **Ubip, **Lblen, **Ublen ;
    Unit **LUbx, **Udiag ;
    int k1, k2, nk, k, block, pend, n, p, nblocks, chunk, nr, i ;

    if (Common == NULL)
    {
	return (FALSE) ;
    }
    Common->status = KLU_OK ;

    if (Numeric == NULL)
    {
	/* invalid Numeric object */
	Common->status = KLU_INVALID ;
	return (FALSE) ;
    }

    Bz = (Entry *) B ;

    /* ---------------------------------------------------------------------- */
    /* get the contents of the Symbolic object */
    /* ---------------------------------------------------------------------- */

    n = Symbolic->n ;
    nblocks = Symbolic->nblocks ;
    Q = Symbolic->Q ;
    R = Symbolic->R ;

    /* ---------------------------------------------------------------------- */
    /* get the contents of the Numeric object */
    /* ---------------------------------------------------------------------- */

    ASSERT (nblocks == Numeric->nblocks) ;
    Pnum = Numeric->Pnum ;
    Offp = Numeric->Offp ;
    Offi = Numeric->Offi ;
    Offx = (Entry *) Numeric->Offx ;
    Singleton = (Entry *) Numeric->Singleton ;

    Lbip = Numeric->Lbip ;
    Lblen = Numeric->Lblen ;
    Ubip = Numeric->Ubip ;
    Ublen = Numeric->Ublen ;
    LUbx = (Unit **) Numeric->LUbx ;
    Udiag = (Unit **) Numeric->Udiag ;

    Rs = Numeric->Rs ;
    X = (Entry *) Numeric->Xwork ;

    ASSERT (KLU_valid (n, Offp, Offi, Offx)) ;

    /* ---------------------------------------------------------------------- */
    /* solve in chunks of 4 columns at a time */
    /* ---------------------------------------------------------------------- */

    for (chunk = 0 ; chunk < nrhs ; chunk += 4)
    {

	/* ------------------------------------------------------------------ */
	/* get the size of the current chunk */
	/* ------------------------------------------------------------------ */

	nr = MIN (nrhs - chunk, 4) ;

	/* ------------------------------------------------------------------ */
	/* scale and permute the right hand side, X = P*(R\B) */
	/* ------------------------------------------------------------------ */

	if (Rs == NULL)
	{

	    /* no scaling */
	    switch (nr)
	    {

		case 1:

		    for (k = 0 ; k < n ; k++)
		    {
			X [k] = Bz [Pnum [k]] ;
		    }
		    break ;

		case 2:

		    for (k = 0 ; k < n ; k++)
		    {
			i = Pnum [k] ;
			X [2*k    ] = Bz [i      ] ;
			X [2*k + 1] = Bz  [i + d  ] ;
		    }
		    break ;

		case 3:

		    for (k = 0 ; k < n ; k++)
		    {
			i = Pnum [k] ;
			X [3*k    ] = Bz [i      ] ;
			X [3*k + 1] = Bz [i + d  ] ;
			X [3*k + 2] = Bz [i + d*2] ;
		    }
		    break ;

		case 4:

		    for (k = 0 ; k < n ; k++)
		    {
			i = Pnum [k] ;
			X [4*k    ] = Bz [i      ] ;
			X [4*k + 1] = Bz [i + d  ] ;
			X [4*k + 2] = Bz [i + d*2] ;
			X [4*k + 3] = Bz [i + d*3] ;
		    }
		    break ;
	    }

	}
	else
	{

	    switch (nr)
	    {

		case 1:

		    for (k = 0 ; k < n ; k++)
		    {
			SCALE_DIV_ASSIGN (X [k], Bz  [Pnum [k]], Rs [k]) ;
		    }
		    break ;

		case 2:

		    for (k = 0 ; k < n ; k++)
		    {
			i = Pnum [k] ;
			rs = Rs [k] ;
			SCALE_DIV_ASSIGN (X [2*k], Bz [i], rs) ;
			SCALE_DIV_ASSIGN (X [2*k + 1], Bz [i + d], rs) ;
		    }
		    break ;

		case 3:

		    for (k = 0 ; k < n ; k++)
		    {
			i = Pnum [k] ;
			rs = Rs [k] ;
			SCALE_DIV_ASSIGN (X [3*k], Bz [i], rs) ;
			SCALE_DIV_ASSIGN (X [3*k + 1], Bz [i + d], rs) ;
			SCALE_DIV_ASSIGN (X [3*k + 2], Bz [i + d*2], rs) ;
		    }
		    break ;

		case 4:

		    for (k = 0 ; k < n ; k++)
		    {
			i = Pnum [k] ;
			rs = Rs [k] ;
			SCALE_DIV_ASSIGN (X [4*k], Bz [i], rs) ;
			SCALE_DIV_ASSIGN (X [4*k + 1], Bz [i + d], rs) ;
			SCALE_DIV_ASSIGN (X [4*k + 2], Bz [i + d*2], rs) ;
			SCALE_DIV_ASSIGN (X [4*k + 3], Bz [i + d*3], rs) ;
		    }
		    break ;
	    }
	}

	/* ------------------------------------------------------------------ */
	/* solve X = (L*U + Off)\X */
	/* ------------------------------------------------------------------ */

	for (block = nblocks-1 ; block >= 0 ; block--)
	{

	    /* -------------------------------------------------------------- */
	    /* the block of size nk is from rows/columns k1 to k2-1 */
	    /* -------------------------------------------------------------- */

	    k1 = R [block] ;
	    k2 = R [block+1] ;
	    nk = k2 - k1 ;
	    PRINTF (("solve %d, k1 %d k2-1 %d nk %d\n", block, k1,k2-1,nk)) ;

	    /* solve the block system */
	    if (nk == 1)
	    {
		s = Singleton [block] ;
		switch (nr)
		{

		    case 1:
			DIV (X [k1], X [k1], s) ;
			break ;

		    case 2:
			DIV (X [2*k1], X [2*k1], s) ;
			DIV (X [2*k1 + 1], X [2*k1 + 1], s) ;
			break ;

		    case 3:
			DIV (X [3*k1], X [3*k1], s) ;
			DIV (X [3*k1 + 1], X [3*k1 + 1], s) ;
			DIV (X [3*k1 + 2], X [3*k1 + 2], s) ;
			break ;

		    case 4:
			DIV (X [4*k1], X [4*k1], s) ;
			DIV (X [4*k1 + 1], X [4*k1 + 1], s) ;
			DIV (X [4*k1 + 2], X [4*k1 + 2], s) ;
			DIV (X [4*k1 + 3], X [4*k1 + 3], s) ;
			break ;

		}
	    }
	    else
	    {
		KLU_lsolve (nk, Lbip [block], Lblen [block], LUbx [block], nr,
			X + nr*k1) ;
		KLU_usolve (nk, Ubip [block], Ublen [block], LUbx [block],
			Udiag [block], nr, X + nr*k1) ;
	    }

	    /* -------------------------------------------------------------- */
	    /* block back-substitution for the off-diagonal-block entries */
	    /* -------------------------------------------------------------- */

	    if (block > 0)
	    {
		switch (nr)
		{

		    case 1:

			for (k = k1 ; k < k2 ; k++)
			{
			    pend = Offp [k+1] ;
			    x [0] = X [k] ;
			    for (p = Offp [k] ; p < pend ; p++)
			    {
				MULT_SUB (X [Offi [p]], Offx [p], x [0]) ;
			    }
			}
			break ;

		    case 2:

			for (k = k1 ; k < k2 ; k++)
			{
			    pend = Offp [k+1] ;
			    x [0] = X [2*k    ] ;
			    x [1] = X [2*k + 1] ;
			    for (p = Offp [k] ; p < pend ; p++)
			    {
				i = Offi [p] ;
				offik = Offx [p] ;
				MULT_SUB (X [2*i], offik, x [0]) ;
				MULT_SUB (X [2*i + 1], offik, x [1]) ;
			    }
			}
			break ;

		    case 3:

			for (k = k1 ; k < k2 ; k++)
			{
			    pend = Offp [k+1] ;
			    x [0] = X [3*k    ] ;
			    x [1] = X [3*k + 1] ;
			    x [2] = X [3*k + 2] ;
			    for (p = Offp [k] ; p < pend ; p++)
			    {
				i = Offi [p] ;
				offik = Offx [p] ;
				MULT_SUB (X [3*i], offik, x [0]) ;
				MULT_SUB (X [3*i + 1], offik, x [1]) ;
				MULT_SUB (X [3*i + 2], offik, x [2]) ;
			    }
			}
			break ;

		    case 4:

			for (k = k1 ; k < k2 ; k++)
			{
			    pend = Offp [k+1] ;
			    x [0] = X [4*k    ] ;
			    x [1] = X [4*k + 1] ;
			    x [2] = X [4*k + 2] ;
			    x [3] = X [4*k + 3] ;
			    for (p = Offp [k] ; p < pend ; p++)
			    {
				i = Offi [p] ;
				offik = Offx [p] ;
				MULT_SUB (X [4*i], offik, x [0]) ;
				MULT_SUB (X [4*i + 1], offik, x [1]) ;
				MULT_SUB (X [4*i + 2], offik, x [2]) ;
				MULT_SUB (X [4*i + 3], offik, x [3]) ;
			    }
			}
			break ;
		}
	    }
	}

	/* ------------------------------------------------------------------ */
	/* permute the result, Bz  = Q*X */
	/* ------------------------------------------------------------------ */

	switch (nr)
	{

	    case 1:

		for (k = 0 ; k < n ; k++)
		{
		    Bz  [Q [k]] = X [k] ;
		}
		break ;

	    case 2:

		for (k = 0 ; k < n ; k++)
		{
		    i = Q [k] ;
		    Bz  [i      ] = X [2*k    ] ;
		    Bz  [i + d  ] = X [2*k + 1] ;
		}
		break ;

	    case 3:

		for (k = 0 ; k < n ; k++)
		{
		    i = Q [k] ;
		    Bz  [i      ] = X [3*k    ] ;
		    Bz  [i + d  ] = X [3*k + 1] ;
		    Bz  [i + d*2] = X [3*k + 2] ;
		}
		break ;

	    case 4:

		for (k = 0 ; k < n ; k++)
		{
		    i = Q [k] ;
		    Bz  [i      ] = X [4*k    ] ;
		    Bz  [i + d  ] = X [4*k + 1] ;
		    Bz  [i + d*2] = X [4*k + 2] ;
		    Bz  [i + d*3] = X [4*k + 3] ;
		}
		break ;
	}

	/* ------------------------------------------------------------------ */
	/* go to the next chunk of B */
	/* ------------------------------------------------------------------ */

	Bz  += d*4 ;
    }
    return (TRUE) ;
}
