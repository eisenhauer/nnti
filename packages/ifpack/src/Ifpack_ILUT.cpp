/*@HEADER
// ***********************************************************************
// 
//       Ifpack: Object-Oriented Algebraic Preconditioner Package
//                 Copyright (2002) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
//@HEADER
*/

#include "Ifpack_ConfigDefs.h"
#ifdef HAVE_IFPACK_TEUCHOS
#include "Ifpack_Preconditioner.h"
#include "Ifpack_ILUT.h"
#include "Ifpack_Condest.h"
#include "Epetra_Comm.h"
#include "Epetra_Map.h"
#include "Epetra_RowMatrix.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_Vector.h"
#include "Epetra_MultiVector.h"
#include "Epetra_Util.h"
#include "Teuchos_ParameterList.hpp"
#include <functional>

//==============================================================================
// FIXME: allocate Comm_ and Time_ the first Initialize() call
Ifpack_ILUT::Ifpack_ILUT(const Epetra_RowMatrix* A) :
  A_(*A),
  Comm_(Comm()),
  L_(0),
  U_(0),
  Condest_(-1.0),
  Relax_(0.),
  Athresh_(0.0),
  Rthresh_(0.0),
  LevelOfFill_(1.0),
  IsInitialized_(false),
  IsComputed_(false),
  UseTranspose_(false),
  NumMyRows_(-1),
  NumInitialize_(0),
  NumCompute_(0),
  NumApplyInverse_(0),
  InitializeTime_(0.0),
  ComputeTime_(0.0),
  ApplyInverseTime_(0.0),
  Time_(Comm())
{ }

//==============================================================================
Ifpack_ILUT::~Ifpack_ILUT()
{

  if (L_)
    delete L_;

  if (U_)
    delete U_;

  IsInitialized_ = false;
  IsComputed_ = false;
}

//==========================================================================
int Ifpack_ILUT::SetParameters(Teuchos::ParameterList& List)
{

  LevelOfFill_ = List.get("fact: ilut level-of-fill",LevelOfFill());
  if (LevelOfFill_ <= 0.0)
    IFPACK_CHK_ERR(-2); // must be greater than 0.0

  Athresh_ = List.get("fact: absolute threshold", Athresh_);
  Rthresh_ = List.get("fact: relative threshold", Rthresh_);
  Relax_ = List.get("fact: relax value", Relax_);

  // set label
  sprintf(Label_, "IFPACK ILUT (fill=%f, relax=%f, athr=%f, rthr=%f)",
	  LevelOfFill(), RelaxValue(), AbsoluteThreshold(), 
	  RelativeThreshold());
  return(0);
}

//==========================================================================
int Ifpack_ILUT::Initialize()
{
  IsInitialized_ = false;
  Time_.ResetStartTime();

  if (Matrix().NumMyRows() != Matrix().NumMyCols())
    IFPACK_CHK_ERR(-2);
    
  NumMyRows_ = Matrix().NumMyRows();

  // delete previously allocated factorization
  if (L_)
    delete L_;
  if (U_)
    delete U_;

  // nothing else to do here
  IsInitialized_ = true;
  ++NumInitialize_;
  InitializeTime_ += Time_.ElapsedTime();

  return(0);
}

//==========================================================================
#include "float.h"
int Ifpack_ILUT::Compute() {

  if (!IsInitialized()) 
    IFPACK_CHK_ERR(Initialize());

  Time_.ResetStartTime();
  IsComputed_ = false;

  int NumMyRows_ = Matrix().NumMyRows();

  vector<int> Nnz(NumMyRows_);
  // will contain the nonzero values of U (including diagonal)
  vector<vector<int> > UI(NumMyRows_);
  vector<vector<double> > UV(NumMyRows_);

  // will contain the nonzero values of L
  vector<vector<int> > LI(NumMyRows_);
  vector<vector<double> > LV(NumMyRows_);

  // temp vectors for ExtractMyRowCopy()
  int Length = Matrix().MaxNumEntries();
  vector<int> RowIndices(Length);
  vector<double> RowValues(Length);

  // sizes of L and U pointers (a couple for each row)
  vector<int> L_size(NumMyRows_);
  vector<int> U_size(NumMyRows_);

  // cycle over all rows
  for (int i = 0 ; i < NumMyRows_ ; ++i) {

    int RowNnz;
    Matrix().ExtractMyRowCopy(i,Length,RowNnz,&RowValues[0],&RowIndices[0]);
    
    // count the nonzeros in L (without diagonal, stored in U)
    L_size[i] = 0;
    U_size[i] = 0;
    for (int j = 0 ; j < RowNnz ; ++j) {
      if (RowIndices[j] < i)
        L_size[i]++;
      else
        U_size[i]++;
    }

    LI[i].resize((int)(LevelOfFill() * L_size[i]));
    LV[i].resize((int)(LevelOfFill() * L_size[i]));
    UI[i].resize((int)(LevelOfFill() * (U_size[i] + 1)));
    UV[i].resize((int)(LevelOfFill() * (U_size[i] + 1)));

    int L_count = 0;
    int U_count = 0; 

    // diagonal element is always the first of U
    UI[i][U_count] = i;
    UV[i][U_count] = 0.0;
    ++U_count;

    for (int j = 0 ; j < RowNnz ; ++j) {
      int col = RowIndices[j];

      if (col < i) {
        LI[i][L_count] = col;
        LV[i][L_count] = RowValues[j];
        ++L_count;
      }
      else if (col == i) {
          UV[i][0] = RowValues[j];
      }
      else if (col < NumMyRows_) {
        UI[i][U_count] = col;
        double v = RowValues[j];
        // change the diagonal value, following SetParameters()
        UV[i][U_count] = Rthresh_ * v + EPETRA_SGN(v) * Athresh_;
        ++U_count;
      }
    }
  }

  // allocate the Crs matrices that will contain the factors
  L_ = new Epetra_CrsMatrix(Copy,Matrix().RowMatrixRowMap(),0);
  U_ = new Epetra_CrsMatrix(Copy,Matrix().RowMatrixRowMap(),0);
  if ((L_ == 0) || (U_ == 0))
    IFPACK_CHK_ERR(-5);

  // insert first row in U_
  IFPACK_CHK_ERR(U_->InsertGlobalValues(0,(int)U_size[0],
                                        &(UV[0][0]), &(UI[0][0])));

  // FIXME
  //double threshold_ = 0e-6;
  double rel_threshold_ = 1e-1;

  // ===================== //
  // Perform factorization //
  // ===================== //

  vector<double> tmp(NumMyRows_);
  for (int j = 0 ; j < NumMyRows_ ; ++j)
    tmp[j] = 0.0;
  vector<double> l_atmp(NumMyRows_);
  vector<double> u_atmp(NumMyRows_);
  vector<int> l_index(NumMyRows_);
  vector<int> u_index(NumMyRows_);

  double old_l_cutoff = 1.0;
  double old_u_cutoff = 1.0;

  // cycle over all rows 
  for (int i = 1 ; i < NumMyRows_ ; ++i) {

    double dropped_values = 0.0;

    // populate tmp with nonzeros of this row
    for (int j = 0 ; j < (int)L_size[i] ; ++j) {
      tmp[LI[i][j]] = LV[i][j];
    }
    for (int j = 0 ; j < (int)U_size[i] ; ++j) {
      tmp[UI[i][j]] = UV[i][j];
    }

    int first;
    if (L_size[i])
      first = LI[i][0];
    else
      first = i;

    double diag = UV[i][0];
    double drop = 0.0 * diag; // FIXME

    for (int k = first ; k < i ; ++k) {

      if (tmp[k] == 0.0)
        continue;

#if 0
      if (IFPACK_ABS(tmp[k]) < drop) {
        tmp[k] = 0.0;
        continue;
      }
#endif

      tmp[k] /= UV[k][0];

      for (int j = 0 ; j < (int)U_size[k] ; ++j) {
        int col = UI[k][j];
        if (col <= k)
          continue;
        double add = tmp[k] * UV[k][j];
        if (IFPACK_ABS(add) > drop)
          tmp[col] -= add;
      }
    }

    // track diagonal element and insert it
    UV[i][0] = tmp[i];
    double abs_diag = IFPACK_ABS(UV[i][0]);
    tmp[i] = 0.0;

    // estimate a good cut-off from previous line. This will
    // limitate the number of elements to order with sort().
    double this_l_cutoff = rel_threshold_ * old_l_cutoff * abs_diag;
    double this_u_cutoff = rel_threshold_ * old_u_cutoff * abs_diag;
    // get nonzeros in tmp, and absolute values in l_atmp and u_atmp
    int l_count = 0;
    int u_count = 0;
    for (int j = 0 ; j < i ; ++j) {
      double val = IFPACK_ABS(tmp[j]);
      if (val < this_l_cutoff || val < drop) {
        dropped_values += tmp[j];
        tmp[j] = 0.0;
      } else {
        // store in l pointer
        l_atmp[l_count] = val;
        l_index[l_count] = j;
        ++l_count;
      }
    }
    for (int j = i + 1 ; j < NumMyRows_ ; ++j) {
      double val = IFPACK_ABS(tmp[j]);
      if (val <= this_u_cutoff || val <= drop) {
        dropped_values += tmp[j];
        tmp[j] = 0.0;
      } else {
        u_atmp[u_count] = val;
        u_index[u_count] = j;
        ++u_count;
      }
    }

    int l_LOF = (int)(LevelOfFill() * L_size[i]);
    int u_LOF = (int)(LevelOfFill() * U_size[i]);
    double l_cutoff = 0.0;
    double u_cutoff = 0.0;
    // sort in ascending order the entries for this row
    if (l_count > l_LOF) {
      sort(l_atmp.begin(),l_atmp.begin() + l_count,greater<double>());
      l_cutoff = l_atmp[l_LOF];
    }
    if (u_count > u_LOF) {
      sort(u_atmp.begin(),u_atmp.begin() + u_count,greater<double>());
      u_cutoff = u_atmp[u_LOF];
    }

    int L_count = 0;
    int U_count = 1; // diagonal already inserted

    for (int kk = 0 ; kk < l_count ; ++kk) {
      int col = l_index[kk];
      double aval = IFPACK_ABS(tmp[col]);
      if (aval > l_cutoff) {
        LI[i][L_count] = col;
        LV[i][L_count] = tmp[col];
        ++L_count;
      }
      else
        dropped_values += tmp[col];
      tmp[col] = 0.0;
    }
    for (int kk = 0 ; kk < u_count ; ++kk) {
      int col = u_index[kk];
      double aval = IFPACK_ABS(tmp[col]);
      if (aval > u_cutoff) {
        UI[i][U_count] = col;
        UV[i][U_count] = tmp[col];
        ++U_count;
      }
      else
        dropped_values += tmp[col];
      tmp[col] = 0.0;
    }

    // relaxing dropped elements
    UV[i][0] += Relax_ * dropped_values;
    
    // reset the number in processed row
    L_size[i] = L_count;
    U_size[i] = U_count;
    if (L_size[i] > (int)LI[i].size())
      IFPACK_CHK_ERR(-4);
    if (U_size[i] > (int)UI[i].size())
      IFPACK_CHK_ERR(-4);

    old_l_cutoff = l_cutoff / abs_diag;
    old_u_cutoff = u_cutoff / abs_diag;

  }

  // insert unit diagonal in L_
  for (int i = 0 ; i < NumMyRows_ ; ++i) {
    double val = 1.0;
    IFPACK_CHK_ERR(L_->InsertGlobalValues(i,1,&val, &i));
  }

  // insert computed elements in L_
  for (int i = 1 ; i < NumMyRows_ ; ++i) {
#ifdef IFPACK_DEBUG
    for (int j = 0 ; j < (int)L_size[i] ; ++j) {
      if (LI[i][j] >= NumMyRows_) {
        cerr << "ERROR: LI[" << i << "][" << j << "] = "
          << LI[i][j] << " and NumMyRows = " << NumMyRows_ << endl;
        cerr << "(file " << __FILE__ << ", line " << __LINE__ << endl;
        exit(EXIT_FAILURE);
      }
    }
#endif
    IFPACK_CHK_ERR(L_->InsertGlobalValues(i,(int)L_size[i],
                                          &(LV[i][0]), &(LI[i][0])));
    LI[i].resize(0);
    LV[i].resize(0);
  }

  // insert computed elements in U_
  for (int i = 1 ; i < NumMyRows_ ; ++i) {
#ifdef IFPACK_DEBUG
    for (int j = 0 ; j < (int)U_size[i] ; ++j) {
      if (UI[i][j] >= NumMyRows_) {
        cerr << "ERROR: UI[" << i << "][" << j << "] = "
          << UI[i][j] << " and NumMyRows = " << NumMyRows_ << endl;
        cerr << "(file " << __FILE__ << ", line " << __LINE__ << endl;
        exit(EXIT_FAILURE);
      }
    }
#endif
    IFPACK_CHK_ERR(U_->InsertGlobalValues(i,(int)U_size[i],
                                          &(UV[i][0]), &(UI[i][0])));
    UI[i].resize(0);
    UV[i].resize(0);
  }

  IFPACK_CHK_ERR(L_->FillComplete());
  IFPACK_CHK_ERR(U_->FillComplete());

  IsComputed_ = true;
  ++NumCompute_;
  
  return(0);

}
  
//=============================================================================
int Ifpack_ILUT::ApplyInverse(const Epetra_MultiVector& X, 
			     Epetra_MultiVector& Y) const
{

  if (!IsComputed())
    IFPACK_CHK_ERR(-2); // compute preconditioner first

  if (X.NumVectors() != Y.NumVectors()) 
    IFPACK_CHK_ERR(-3); // Return error: X and Y not the same size

  Time_.ResetStartTime();

  // NOTE: I do suppose that X and Y are two different vectors
  EPETRA_CHK_ERR(L_->Solve(false,false,false,X,Y));
  EPETRA_CHK_ERR(U_->Solve(true,false,false,Y,Y));

  ++NumApplyInverse_;
  ApplyInverseTime_ += Time_.ElapsedTime();

  return(0);

}
//=============================================================================
// This function finds X such that LDU Y = X or U(trans) D L(trans) Y = X for multiple RHS
int Ifpack_ILUT::Apply(const Epetra_MultiVector& X, 
		      Epetra_MultiVector& Y) const 
{

  return(-98);
}

//=============================================================================
double Ifpack_ILUT::Condest(const Ifpack_CondestType CT, 
                            const int MaxIters, const double Tol,
			    Epetra_RowMatrix* Matrix)
{
  if (!IsComputed()) // cannot compute right now
    return(-1.0);

  // NOTE: this is computing the *local* condest
  if (Condest_ == -1.0)
    Condest_ = Ifpack_Condest(*this, CT, MaxIters, Tol, Matrix);

  return(Condest_);
}

//=============================================================================
std::ostream&
Ifpack_ILUT::Print(std::ostream& os) const
{
  if (!Comm().MyPID()) {
    os << endl;
    os << "================================================================================" << endl;
    os << "Ifpack_ILUT: " << Label() << endl << endl;
    os << "Level-of-fill      = " << LevelOfFill() << endl;
    os << "Absolute threshold = " << AbsoluteThreshold() << endl;
    os << "Relative threshold = " << RelativeThreshold() << endl;
    os << "Relax value        = " << RelaxValue() << endl;
    os << "Condition number estimate = " << Condest() << endl;
    os << "Global number of rows            = " << A_.NumGlobalRows() << endl;
    if (IsComputed_) {
      os << "Number of nonzeros of L + U     = " << NumGlobalNonzeros() << endl;
      os << "nonzeros / rows                 = " 
        << 1.0 * NumGlobalNonzeros() / U_->NumGlobalRows() << endl;
    }
    os << endl;
    os << "Phase           # calls   Total Time (s)       Total MFlops     MFlops/s" << endl;
    os << "-----           -------   --------------       ------------     --------" << endl;
    os << "Initialize()    "   << std::setw(5) << NumInitialize() 
       << "  " << std::setw(15) << InitializeTime() 
       << "               0.0            0.0" << endl;
    os << "Compute()       "   << std::setw(5) << NumCompute() 
       << "  " << std::setw(15) << ComputeTime()
       << "  " << std::setw(15) << 1.0e-6 * ComputeFlops() 
       << "  " << std::setw(15) << 1.0e-6 * ComputeFlops() / ComputeTime() << endl;
    os << "ApplyInverse()  "   << std::setw(5) << NumApplyInverse() 
       << "  " << std::setw(15) << ApplyInverseTime()
       << "  " << std::setw(15) << 1.0e-6 * ApplyInverseFlops() 
       << "  " << std::setw(15) << 1.0e-6 * ApplyInverseFlops() / ApplyInverseTime() << endl;
    os << "================================================================================" << endl;
    os << endl;
  }

  return(os);
}
#endif // HAVE_IFPACK_TEUCHOS
