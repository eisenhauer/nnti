#ifndef SPARSE_LU_HPP
#define SPARSE_LU_HPP
#include <stdio.h>
#include <fstream>
#include <iomanip>
#include <assert.h>
//#include "my_feti_sparse_solver.hpp"
#include "Epetra_LAPACK.h"
extern "C"{
  void metis_nodend(int* N,int XADJ[],int ADJD[],int* numflag,
		    int OPTIONS[],int PERM[],int INVP[]);
}
class CLAPS_sparse_lu {
 public:
  CLAPS_sparse_lu();
  ~CLAPS_sparse_lu();
  void cleanup();
  int factor(int N_, int NNZ_, int COLPTR[], int ROWIDX[], double ANZ[],
	     int scale_flag_ = 0);
  int sol(int NRHS, double RHS[], double SOL[], double TEMP[]);
 private: //
  void getnrm(int n, int colptr[], int rowidx[], 
	      double values[], double &anorm);
  void inpnv(int &n , int colptr[], int rowidx[], double values[], 
	     int perm[], int invp [], int &nsuper, int xsuper[], 
	     int xlindx[], int lindx[], int xlnz[], double lnz[],
	     int offset[]);
  int small_factor(int rowbeg[], int colidx[], double vals[]);
  int small_solve(int NRHS, double RHS[], double SOL[]);
  Epetra_LAPACK EL;
  int N, DEFBLK, NSUPER, NDEF, LBDEF, max_small, scale_flag;
  int *XSUPER;
  int *XLINDX;
  int *LINDX;
  int *XLNZ;
  int *PERM;
  int *INVP;
  int *IPROW;
  int *IPCOL;
  int *DEF;
  double* LNZ;
  double* NS;
  double *SCALE;
};
#endif // SPARSE_LU_HPP
  
