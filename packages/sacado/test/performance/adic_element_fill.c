#include <math.h>
#include <stdlib.h>

typedef struct {
  int nqp;
  int nnode;
  InactiveDouble *w, *jac, **phi, **dphi;
  int *gid;
} ElemData;

void adic_element_fill(ElemData* e, 
		       unsigned int neqn,
		       const double* x, 
		       double* u, 
		       double* du, 
		       double* f) {
  /* Construct element solution, derivative */
  for (unsigned int qp=0; qp<e->nqp; qp++) {
    for (unsigned int eqn=0; eqn<neqn; eqn++) {
      u[qp*neqn+eqn] = 0.0;
      du[qp*neqn+eqn] = 0.0;
      for (unsigned int node=0; node<e->nnode; node++) {
	u[qp*neqn+eqn] += x[node*neqn+eqn] * e->phi[qp][node];
	du[qp*neqn+eqn] += x[node*neqn+eqn] * e->dphi[qp][node];
      }
    }
  }

  /* Compute sum of equations for coupling */
  double *s = malloc(e->nqp*neqn*sizeof(double));
  for (unsigned int qp=0; qp<e->nqp; qp++) {
    for (unsigned int eqn=0; eqn<neqn; eqn++) {
      s[qp*neqn+eqn] = 0.0;
      for (unsigned int j=0; j<neqn; j++) {
      	if (j != eqn)
      	  s[qp*neqn+eqn] += u[qp*neqn+j]; 
      }
    }
  }

  /* Evaluate element residual */
  for (unsigned int node=0; node<e->nnode; node++) {
    for (unsigned int eqn=0; eqn<neqn; eqn++) {
      unsigned int row = node*neqn+eqn;
      f[row] = 0.0;
      for (unsigned int qp=0; qp<e->nqp; qp++) {
	f[row] += 
	  e->w[qp]*e->jac[qp]*(-(1.0/(e->jac[qp]*e->jac[qp]))*
			     du[qp*neqn+eqn]*e->dphi[qp][node] + 
			     e->phi[qp][node]*(exp(u[qp*neqn+eqn]) + 
					      u[qp*neqn+eqn]*s[qp*neqn+eqn]));
      }
    }
  }

  free(s);
}
