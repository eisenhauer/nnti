/*
#@HEADER
# ************************************************************************
#
#                          Moertel FE Package
#                 Copyright (2006) Sandia Corporation
#
# Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
# license for use of this work by or on behalf of the U.S. Government.
#
# This library is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation; either version 2.1 of the
# License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
# USA
# Questions? Contact Michael Gee (mwgee@sandia.gov)
#
# ************************************************************************
#@HEADER
*/
/* ******************************************************************** */
/* See the file COPYRIGHT for a complete copyright notice, contact      */
/* person and disclaimer.                                               */
/* ******************************************************************** */
#include "mrtr_utils.H"
#include "mrtr_segment.H"
#include "mrtr_segment_linear1D.H"
#include "mrtr_segment_bilineartri.H"
#include "mrtr_segment_bilinearquad.H"
#include "mrtr_node.H"

#include <Epetra_Time.h>
#include <Epetra_Map.h>
#include <EpetraExt_Transpose_RowMatrix.h>
#include <EpetraExt_MatrixMatrix.h>

/*----------------------------------------------------------------------*
 | allocate a segment depending on the type                 mwgee 07/05|
 *----------------------------------------------------------------------*/
MOERTEL::Segment* MOERTEL::AllocateSegment(int type, int out)
{
  switch (type)
  {
    case MOERTEL::Segment::seg_Linear1D:
      {
        MOERTEL::Segment_Linear1D* tmp = new MOERTEL::Segment_Linear1D(out);
        return tmp;
      }
    break;
    case MOERTEL::Segment::seg_BiLinearTri:
      {
        MOERTEL::Segment_BiLinearTri* tmp = new MOERTEL::Segment_BiLinearTri(out);
        return tmp;
      }
    break;
    case MOERTEL::Segment::seg_BiLinearQuad:
      {
        MOERTEL::Segment_BiLinearQuad* tmp = new MOERTEL::Segment_BiLinearQuad(out);
        return tmp;
      }
    break;
    case MOERTEL::Segment::seg_none:
      cout << "***ERR*** MOERTEL::AllocateSegment:\n"
           << "***ERR*** type is func_none, cannot allocate.\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      exit(EXIT_FAILURE);
    break;
    default:
      cout << "***ERR*** MOERTEL::AllocateSegment:\n"
           << "***ERR*** type is unknown, cannot allocate new Segment\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      exit(EXIT_FAILURE);
    break;
  }

  return NULL;
}


/*----------------------------------------------------------------------*
 | allocate a function depending on the type                 mwgee 07/05|
 *----------------------------------------------------------------------*/
MOERTEL::Function* MOERTEL::AllocateFunction(MOERTEL::Function::FunctionType type, int out)
{
  switch (type)
  {
    case MOERTEL::Function::func_Constant1D:
      {
        MOERTEL::Function_Constant1D* tmp = new MOERTEL::Function_Constant1D(out);
        return tmp;
      }
    break;
    case MOERTEL::Function::func_Linear1D:
      {
        MOERTEL::Function_Linear1D* tmp = new MOERTEL::Function_Linear1D(out);
        return tmp;
      }
    break;
    case MOERTEL::Function::func_DualLinear1D:
      {
        MOERTEL::Function_DualLinear1D* tmp = new MOERTEL::Function_DualLinear1D(out);
        return tmp;
      }
    break;
    case MOERTEL::Function::func_LinearTri:
      {
        MOERTEL::Function_LinearTri* tmp = new MOERTEL::Function_LinearTri(out);
        return tmp;
      }
    break;
    case MOERTEL::Function::func_DualLinearTri:
      {
        MOERTEL::Function_DualLinearTri* tmp = new MOERTEL::Function_DualLinearTri(out);
        return tmp;
      }
    break;
    case MOERTEL::Function::func_none:
      cout << "***ERR*** MOERTEL::AllocateFunction:\n"
           << "***ERR*** type is func_none, cannot allocate.\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      exit(EXIT_FAILURE);
    break;
    default:
      cout << "***ERR*** MOERTEL::AllocateFunction:\n"
           << "***ERR*** type is unknown, cannot allocate new Function\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      exit(EXIT_FAILURE);
    break;
  }

  return NULL;
}

/*----------------------------------------------------------------------*
 | do cross product                                          mwgee 10/05|
 *----------------------------------------------------------------------*/
bool MOERTEL::cross(double* out, const double* g1, const double* g2)
{
  out[0] = g1[1]*g2[2] - g1[2]*g2[1];
  out[1] = g1[2]*g2[0] - g1[0]*g2[2];
  out[2] = g1[0]*g2[1] - g1[1]*g2[0];
  return true;
}

/*----------------------------------------------------------------------*
 | do dot product                                            mwgee 10/05|
 *----------------------------------------------------------------------*/
double MOERTEL::dot(const double* g1, const double* g2, const int dim)
{
  double result=0.0;
  for (int i=0; i<dim; ++i) result+=g1[i]*g2[i];
  return result;
}

/*----------------------------------------------------------------------*
 | compute length of vector                                  mwgee 10/05|
 *----------------------------------------------------------------------*/
double MOERTEL::length(const double* g, const int dim)
{
  return sqrt(MOERTEL::dot(g,g,dim));
}

/*----------------------------------------------------------------------*
 | do 2x2 solve                                              mwgee 10/05|
 *----------------------------------------------------------------------*/
bool MOERTEL::solve22(const double A[][2], double* x, const double* b)
{
  double det = A[0][0]*A[1][1]-A[0][1]*A[1][0];
  if (abs(det)<1.0e-10)
  {
    cout << "***ERR*** MOERTEL::solve22:\n"
         << "***ERR*** Determinant is zero\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    exit(EXIT_FAILURE);
  }
  det = 1/det;
  x[0] = det*A[1][1]*b[0]-det*A[0][1]*b[1];
  x[1] = det*A[0][0]*b[1]-det*A[1][0]*b[0];
  return true;
}

/*----------------------------------------------------------------------*
 | do 3x3 solve                                              mwgee 10/05|
 *----------------------------------------------------------------------*/
bool MOERTEL::solve33(const double A[][3], double* x, const double* b)
{
  Epetra_SerialDenseMatrix AA(3,3);
  Epetra_SerialDenseMatrix XX(3,1);
  Epetra_SerialDenseMatrix BB(3,1);
  for (int i=0; i<3; ++i)
  {
    BB(i,0) = b[i];
    for (int j=0; j<3; ++j)
      AA(i,j) = A[i][j];
  }
  Epetra_SerialDenseSolver solver;
  solver.SetMatrix(AA);
  solver.SetVectors(XX,BB);
  solver.FactorWithEquilibration(true);
  solver.Factor();
  int err = solver.Solve();
  if (err)
  {
    cout << AA;
    cout << BB;
    cout << XX;
    cout << "***WRN*** MOERTEL::solve33:\n"
         << "***WRN*** Epetra_SerialDenseSolver::Solve returned " << err << "\n"
         << "***WRN*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    exit(EXIT_FAILURE);
  }
  for (int i=0; i<3; ++i)
    x[i] = XX(i,0);
  
  return true;
}

/*----------------------------------------------------------------------*
 | get the '10' digit from a pos. int                        mwgee 10/05|
 *----------------------------------------------------------------------*/
int MOERTEL::digit_ten(int number)
{
  number = abs(number);
  if (number<10) return 0;
  number /= 10; 
  number = number%10;
  return number;
}

/*----------------------------------------------------------------------*
 | swap 2 kinds                                              mwgee 10/05|
 | this template is given in mrtr_utils.H                               |
 *----------------------------------------------------------------------*/
// template<typename kind> void swap(kind& a, kind& b);



/*----------------------------------------------------------------------*
 | sort dlist                                                      10/05|
  dlist:           On input, values to be sorted. On output, sorted values
                   (i.e., dlist[i] <= dlist[i+1]).

  N:               length of vector 'dlist'.

  list2:           If on input,
                   a) list2 = NULL: it is unchanged on output,
                   b) list2 is a list associated with 'list':
                   on output, if dlist[k] on input is now element 'j' on output,
                   list2[j] on output is list2[k].
 *----------------------------------------------------------------------*/
void MOERTEL::sort(double* dlist, int N, int* list2)
{
  int    l, r, j, i, flag;
  int    RR2;
  double dRR, dK;

  if (N <= 1) return;

  l    = N / 2 + 1;
  r    = N - 1;
  l    = l - 1;
  dRR  = dlist[l - 1];
  dK   = dlist[l - 1];

  if (list2 != NULL) {
     RR2 = list2[l - 1];
     while (r != 0) {
        j = l;
        flag = 1;

        while (flag == 1) {
           i = j;
           j = j + j;

           if (j > r + 1)
              flag = 0;
           else {
              if (j < r + 1)
                 if (dlist[j] > dlist[j - 1]) j = j + 1;

              if (dlist[j - 1] > dK) {
                 dlist[ i - 1] = dlist[ j - 1];
                 list2[i - 1] = list2[j - 1];
              }
              else {
                 flag = 0;
              }
           }
        }
        dlist[ i - 1] = dRR;
        list2[i - 1] = RR2;

        if (l == 1) {
           dRR  = dlist [r];
           RR2 = list2[r];
           dK = dlist[r];
           dlist[r ] = dlist[0];
           list2[r] = list2[0];
           r = r - 1;
         }
         else {
            l   = l - 1;
            dRR  = dlist[ l - 1];
            RR2 = list2[l - 1];
            dK   = dlist[l - 1];
         }
      }
      dlist[ 0] = dRR;
      list2[0] = RR2;
   }
   else {
      while (r != 0) {
         j = l;
         flag = 1;
         while (flag == 1) {
            i = j;
            j = j + j;
            if (j > r + 1)
               flag = 0;
            else {
               if (j < r + 1)
                  if (dlist[j] > dlist[j - 1]) j = j + 1;
               if (dlist[j - 1] > dK) {
                  dlist[ i - 1] = dlist[ j - 1];
               }
               else {
                  flag = 0;
               }
            }
         }
         dlist[ i - 1] = dRR;
         if (l == 1) {
            dRR  = dlist [r];
            dK = dlist[r];
            dlist[r ] = dlist[0];
            r = r - 1;
         }
         else {
            l   = l - 1;
            dRR  = dlist[ l - 1];
            dK   = dlist[l - 1];
         }
      }
      dlist[ 0] = dRR;
   }
  return;
}


/*----------------------------------------------------------------------*
 |                                                                 08/05|
 |  modified version of the epetraext matrixmatrixadd                   |
 |  NOTE:                                                               |
 |  - A has to be FillComplete, B must NOT be FillComplete()            |
 *----------------------------------------------------------------------*/
int MOERTEL::MatrixMatrixAdd(const Epetra_CrsMatrix& A, bool transposeA,double scalarA,
                             Epetra_CrsMatrix& B,double scalarB )
{
  //
  //This method forms the matrix-matrix sum B = scalarA * op(A) + scalarB * B, where

  if (!A.Filled())
  {
     cout << "***ERR*** MOERTEL::MatrixMatrixAdd:\n"
          << "***ERR*** FillComplete was not called on A\n"
          << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
     exit(EXIT_FAILURE);
  }
  if (B.Filled())
  {
     cout << "***ERR*** MOERTEL::MatrixMatrixAdd:\n"
          << "***ERR*** FillComplete was called on B\n"
          << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
     exit(EXIT_FAILURE);
  }
  
  //explicit tranpose A formed as necessary
  Epetra_CrsMatrix* Aprime = 0;
  EpetraExt::RowMatrix_Transpose* Atrans = 0;
  if( transposeA )
  {
    Atrans = new EpetraExt::RowMatrix_Transpose(false,NULL,false);
    Aprime = &(dynamic_cast<Epetra_CrsMatrix&>(((*Atrans)(const_cast<Epetra_CrsMatrix&>(A)))));
  }
  else
    Aprime = const_cast<Epetra_CrsMatrix*>(&A);
    
  B.Scale(scalarB);
  
  //Loop over B's rows and sum into
  int MaxNumEntries = EPETRA_MAX( Aprime->MaxNumEntries(), B.MaxNumEntries() );
  int NumEntries;
  vector<int> Indices(MaxNumEntries);
  vector<double> Values(MaxNumEntries);

  int NumMyRows = Aprime->NumMyRows();
  int Row, err;

  if( scalarA )
  {
    for( int i = 0; i < NumMyRows; ++i )
    {
      Row = Aprime->GRID(i);
      EPETRA_CHK_ERR(Aprime->ExtractGlobalRowCopy(Row,MaxNumEntries,NumEntries,&Values[0],&Indices[0]));
      if( scalarA != 1.0 )
        for( int j = 0; j < NumEntries; ++j ) Values[j] *= scalarA;
      for (int j=0; j<NumEntries; ++j)
      {
        err = B.SumIntoGlobalValues(Row,1,&Values[j],&Indices[j]);
        if (err)
        err = B.InsertGlobalValues(Row,1,&Values[j],&Indices[j]);
        if (err != 0 && err != 1)
        {
          cout << "***ERR*** MOERTEL::MatrixMatrixAdd:\n"
               << "***ERR*** InsertGlobalValues returned " << err << "\n"
               << "Row " << Row << " Col " << Indices[j] << endl
               << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
          exit(EXIT_FAILURE);
        }
      }
    }
  }

  Indices.clear();
  Values.clear();

  if( Atrans ) delete Atrans;

  return(0);
}



#if 0
/*----------------------------------------------------------------------*
 | Multiply matrices A*B                                     mwgee 01/06|
 *----------------------------------------------------------------------*/
Epetra_CrsMatrix* MOERTEL::MatMatMult(Epetra_CrsMatrix& A, bool transA, 
                                      Epetra_CrsMatrix& B, bool transB)
{
  // transpose A if indicated
  Epetra_CrsMatrix* Atrans = &A;
  EpetraExt::RowMatrix_Transpose* transposerA = NULL;
  if (transA)
  {
    transposerA = new EpetraExt::RowMatrix_Transpose(false);
    Atrans = &(dynamic_cast<Epetra_CrsMatrix&>(((*transposerA)(const_cast<Epetra_CrsMatrix&>(A)))));
    if (!Atrans)
    {
      cout << "***ERR*** MOERTEL::MatMatMult:\n"
           << "***ERR*** transpose of A failed\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      return NULL;
    }
  }  
  
  // transpose B if indicated
  Epetra_CrsMatrix* Btrans = &B;
  EpetraExt::RowMatrix_Transpose* transposerB = NULL;
  if (transB)
  {
    transposerB = new EpetraExt::RowMatrix_Transpose(false);
    Btrans = &(dynamic_cast<Epetra_CrsMatrix&>(((*transposerB)(const_cast<Epetra_CrsMatrix&>(B)))));
    if (!Btrans)
    {
      cout << "***ERR*** MOERTEL::MatMatMult:\n"
           << "***ERR*** transpose of B failed\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      return NULL;
    }
  }
  
  // make sure FillComplete was called on the matrices
  if (!Atrans->Filled()) 
  {
    cout << "***ERR*** MOERTEL::MatMatMult:\n"
         << "***ERR*** FillComplete() was not called on matrix A\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return NULL;
  }
  if (!Btrans->Filled()) 
  {
    cout << "***ERR*** MOERTEL::MatMatMult:\n"
         << "***ERR*** FillComplete() was not called on matrix B\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return NULL;
  }
  
  // create an ML communicator
  ML_Comm* ml_comm;
  ML_Comm_Create(&ml_comm);
#ifdef EPETRA_MPI
#if 0
  // replace MPI_COMM_WORLD in ml_comm by a comm used by one of the matrices
  const Epetra_MpiComm* mpiecomm = dynamic_cast<const Epetra_MpiComm*>(&(Atrans->Comm()));
  if (!mpiecomm)
  {
    ML_Comm_Destroy(&ml_comm);
    cout << "***ERR*** MOERTEL::MatMatMult:\n"
         << "***ERR*** dynamic_cast from Epetra_Comm to Epetra_MpiComm failed\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return NULL;
  }
  MPI_Comm mpicomm = mpiecomm->GetMpiComm();
  ML_Comm_Set_UsrComm(ml_comm,mpicomm);
  // FIXME: These 2 lines are needed for compatibility with Trilinos 6.x and can be skipped later
  MPI_Comm_size(mpicomm,&(ml_comm->ML_nprocs));
  MPI_Comm_rank(mpicomm,&(ml_comm->ML_mypid));
#endif
#endif  
  // create ml operators from the matrices and one for the output
  ML_Operator* mlA = ML_Operator_Create(ml_comm);
  ML_Operator* mlB = ML_Operator_Create(ml_comm);
  ML_Operator* mlC = ML_Operator_Create(ml_comm);
  
  
  // wrap the input matrices as ML_Operator
  ML_Operator_WrapEpetraMatrix((Epetra_RowMatrix*)Atrans,mlA);
  ML_Operator_WrapEpetraMatrix((Epetra_RowMatrix*)Btrans,mlB);
  //ML_Operator_Print(mlA,"I");
  //ML_Operator_Print(mlB,"WT");
  
  // make the multiply
  ML_2matmult(mlA,mlB,mlC,ML_EpetraCRS_MATRIX);
  
  // Extract the epetra stuff from mlC and blow the rest
  Epetra_CrsMatrix* result = static_cast<Epetra_CrsMatrix*>(mlC->data);
  if (!result->Filled())
  {
    cout << "***ERR*** MOERTEL::MatMatMult:\n"
         << "***ERR*** FillComplete() was not called on result!\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
  }
  result->OptimizeStorage();
  
  // tidy up
  ML_Operator_Destroy(&mlA);  
  ML_Operator_Destroy(&mlB);  
  ML_Operator_Destroy(&mlC);  
  ML_Comm_Destroy(&ml_comm);
  
  if (transA)
    delete transposerA;
  if (transB)
    delete transposerB;
  return result;
}
#endif


/*----------------------------------------------------------------------*
 | Multiply matrices A*B                                     mwgee 01/06|
 *----------------------------------------------------------------------*/
Epetra_CrsMatrix* MOERTEL::MatMatMult(const Epetra_CrsMatrix& A, bool transA, 
                                      const Epetra_CrsMatrix& B, bool transB,
                                      int outlevel)
{
  // make sure FillComplete was called on the matrices
  if (!A.Filled()) 
  {
    cout << "***ERR*** MOERTEL::MatMatMult:\n"
         << "***ERR*** FillComplete() was not called on matrix A\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return NULL;
  }
  if (!B.Filled()) 
  {
    cout << "***ERR*** MOERTEL::MatMatMult:\n"
         << "***ERR*** FillComplete() was not called on matrix B\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return NULL;
  }
  
  // create resultmatrix with correct rowmap
  Epetra_CrsMatrix* C = NULL;
  if (!transA)
    C = new Epetra_CrsMatrix(Copy,A.OperatorRangeMap(),20,false);
  else
    C = new Epetra_CrsMatrix(Copy,A.OperatorDomainMap(),20,false);
  
  // make the multiply
  Epetra_Time time(A.Comm());
  if (outlevel>9)
    time.ResetStartTime();

  int err = EpetraExt::MatrixMatrix::Multiply(A,transA,B,transB,*C);
  if (err) cout << "MOERTEL: EpetraExt::MatrixMatrix::Multiply returned err = " << err << endl;

  if (outlevel>9 && A.Comm().MyPID()==0)
  {
    cout << "MOERTEL (Proc 0): Time for matrix-matrix product " << time.ElapsedTime() << " sec\n";
    fflush(stdout);
  }

  return C;
}

/*----------------------------------------------------------------------*
 | strip out zeros from a matrix                             m.gee 01/06|
 *----------------------------------------------------------------------*/
Epetra_CrsMatrix* MOERTEL::StripZeros(Epetra_CrsMatrix& A, double eps)
{
  Epetra_CrsMatrix* out = new Epetra_CrsMatrix(Copy,A.RowMap(),10,false);
  for (int lrow=0; lrow<A.NumMyRows(); ++lrow)
  {
    int grow = A.GRID(lrow); 
    if (grow<0) 
    { 
      cout << "***ERR*** MOERTEL::StripZeros:\n"
           << "***ERR*** Cannot gind global row indes from local row index\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      delete out;
      return NULL;
    }
    int numentries;
    int* lindices;
    double* values;
    int err  = A.ExtractMyRowView(lrow,numentries,values,lindices);
    if (err) 
    { 
      cout << "***ERR*** MOERTEL::StripZeros:\n"
           << "***ERR*** A.ExtractMyRowView returned " << err << endl
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      delete out;
      return NULL;
    }
    for (int j=0; j<numentries; ++j)
    {
      int lcol = lindices[j];  
      int gcol = A.GCID(lcol); 
      if (gcol<0) { cout << "ERROR: gcol<0 \n"; exit(0); }
      if (abs(values[j])<eps)
        continue;
      int err = out->InsertGlobalValues(grow,1,&values[j],&gcol);
      if (err != 0 && err != 1) 
      { 
        cout << "***ERR*** MOERTEL::StripZeros:\n"
             << "***ERR*** out->InsertGlobalValues returned " << err << endl
             << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
        delete out;
        return NULL;
      }
    }
  }
  out->FillComplete(A.OperatorDomainMap(),A.OperatorRangeMap());
  return out;
}

/*----------------------------------------------------------------------*
 | print matrix                                              m.gee 01/06|
 *----------------------------------------------------------------------*/
bool MOERTEL::Print_Matrix(string name, Epetra_CrsMatrix& A, int ibase)
{
  char mypidc[100];
  sprintf(mypidc,"%d",A.Comm().MyPID());
  name = name + mypidc + ".mtx";
  char* nameptr = &name[0];
  FILE* out = fopen(nameptr,"w");
  if (!out)
  {
    cout << "***ERR*** MOERTEL::Print_Matrix:\n"
         << "***ERR*** Cannot open file " << name << "\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return false;
  }
  
  // write global and local dimensions of this operator
  fprintf(out,"%d %d 0\n",A.RangeMap().NumGlobalElements(),A.DomainMap().NumGlobalElements());
  for (int lrow=0; lrow<A.NumMyRows(); ++lrow)
  {
    int grow = A.GRID(lrow); 
    if (grow<0) 
    { 
      cout << "***ERR*** MOERTEL::Print_Matrix:\n"
           << "***ERR*** Cannot gind global row index from local row index\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      return false;
    }
    int numentries;
    int* lindices;
    double* values;
    int err  = A.ExtractMyRowView(lrow,numentries,values,lindices);
    if (err) 
    { 
      cout << "***ERR*** MOERTEL::Print_Matrix:\n"
           << "***ERR*** A.ExtractMyRowView returned " << err << endl
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      delete out;
      return false;
    }
    for (int j=0; j<numentries; ++j)
    {
      int lcol = lindices[j];  
      int gcol = A.GCID(lcol); 
      if (gcol<0) { cout << "ERROR: gcol<0 \n"; exit(0); }
      fprintf(out," %d   %d   %20.10e\n",grow+ibase,gcol+ibase,values[j]);
    }
  }
  fflush(out);
  fclose(out);
  cout << "Epetra_CrsMatrix is written to file " << name << endl;
  fflush(stdout);
  return true;
}


/*----------------------------------------------------------------------*
 | print matrix                                              m.gee 02/06|
 *----------------------------------------------------------------------*/
bool MOERTEL::Print_Vector(string name, Epetra_Vector& v, int ibase)
{
  char mypidc[100];
  sprintf(mypidc,"%d",v.Comm().MyPID());
  name = name + mypidc + ".vec";
  char* nameptr = &name[0];
  FILE* out = fopen(nameptr,"w");
  if (!out)
  {
    cout << "***ERR*** MOERTEL::Print_Vector:\n"
         << "***ERR*** Cannot open file " << name << "\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return false;
  }
  //fprintf(out,"%d %d\n",v.GlobalLength(),v.MyLength());
  for (int lrow=0; lrow<v.MyLength(); ++lrow)
  {
    int grow = v.Map().GID(lrow); 
    if (grow<0) 
    { 
      cout << "***ERR*** MOERTEL::Print_Vector:\n"
           << "***ERR*** Cannot gind global row index from local row index\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      return false;
    }
    //fprintf(out," %d   %20.10e\n",grow+ibase,v[lrow]);
    fprintf(out,"  %20.10e\n",v[lrow]);
  }
  fflush(out);
  fclose(out);
  cout << "Epetra_Vector is written to file " << name << endl;
  fflush(stdout);
  return true;
}


/*----------------------------------------------------------------------*
 | print matrix                                              m.gee 04/06|
 *----------------------------------------------------------------------*/
bool MOERTEL::Print_Graph(string name, Epetra_CrsGraph& A, int ibase)
{
  char mypidc[100];
  sprintf(mypidc,"%d",A.Comm().MyPID());
  name = name + mypidc + ".mtx";
  char* nameptr = &name[0];
  FILE* out = fopen(nameptr,"w");
  if (!out)
  {
    cout << "***ERR*** MOERTEL::Print_Graph:\n"
         << "***ERR*** Cannot open file " << name << "\n"
         << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
    return false;
  }
  
  // write global and local dimensions of this operator
  fprintf(out,"%d %d 0\n",A.RangeMap().NumGlobalElements(),A.DomainMap().NumGlobalElements());
  for (int lrow=0; lrow<A.NumMyRows(); ++lrow)
  {
    int grow = A.GRID(lrow); 
    if (grow<0) 
    { 
      cout << "***ERR*** MOERTEL::Print_Graph:\n"
           << "***ERR*** Cannot gind global row index from local row index\n"
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      return false;
    }
    int numentries;
    int* lindices;
    int err  = A.ExtractMyRowView(lrow,numentries,lindices);
    if (err) 
    { 
      cout << "***ERR*** MOERTEL::Print_Graph:\n"
           << "***ERR*** A.ExtractMyRowView returned " << err << endl
           << "***ERR*** file/line: " << __FILE__ << "/" << __LINE__ << "\n";
      delete out;
      return false;
    }
    for (int j=0; j<numentries; ++j)
    {
      int lcol = lindices[j];  
      int gcol = A.GCID(lcol); 
      if (gcol<0) { cout << "ERROR: gcol<0 \n"; exit(0); }
      fprintf(out," %d   %d   %20.10e\n",grow+ibase,gcol+ibase,1.0);
    }
  }
  fflush(out);
  fclose(out);
  cout << "Epetra_CrsGraph is written to file " << name << endl;
  fflush(stdout);
  return true;
}


