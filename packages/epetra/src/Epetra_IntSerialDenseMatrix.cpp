
/* Copyright (2001) Sandia Corportation. Under the terms of Contract 
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this 
 * work by or on behalf of the U.S. Government.  Export of this program
 * may require a license from the United States Government. */


/* NOTICE:  The United States Government is granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable worldwide
 * license in ths data to reproduce, prepare derivative works, and
 * perform publicly and display publicly.  Beginning five (5) years from
 * July 25, 2001, the United States Government is granted for itself and
 * others acting on its behalf a paid-up, nonexclusive, irrevocable
 * worldwide license in this data to reproduce, prepare derivative works,
 * distribute copies to the public, perform publicly and display
 * publicly, and to permit others to do so.
 * 
 * NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT
 * OF ENERGY, NOR SANDIA CORPORATION, NOR ANY OF THEIR EMPLOYEES, MAKES
 * ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR
 * RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY
 * INFORMATION, APPARATUS, PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS
 * THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS. */


#include "Epetra_IntSerialDenseMatrix.h"
//=============================================================================
Epetra_IntSerialDenseMatrix::Epetra_IntSerialDenseMatrix()
	: Epetra_Object("Epetra::IntSerialDenseMatrix"),
		CV_(Copy),
		A_Copied_(false),
		M_(0),
		N_(0),
		LDA_(0),
		A_(0)
{
}

//=============================================================================
Epetra_IntSerialDenseMatrix::Epetra_IntSerialDenseMatrix(int NumRows, int NumCols)
  : Epetra_Object("Epetra::IntSerialDenseMatrix"),
		CV_(Copy),
    A_Copied_(false),
    M_(0),
    N_(0),
    LDA_(0),
    A_(0)
{
	if(NumRows < 0)
		throw ReportError("NumRows = " + toString(NumRows) + ". Should be >= 0", -1);
	if(NumCols < 0)
		throw ReportError("NumCols = " + toString(NumCols) + ". Should be >= 0", -1);

	int errorcode = Shape(NumRows, NumCols);
	if(errorcode != 0)
		throw ReportError("Shape returned non-zero (" + toString(errorcode) + ").", -2);
}
//=============================================================================
Epetra_IntSerialDenseMatrix::Epetra_IntSerialDenseMatrix(Epetra_DataAccess CV, int* A, int LDA, 
																												 int NumRows, int NumCols)
  : Epetra_Object("Epetra::IntSerialDenseMatrix"),
		CV_(CV),
		A_Copied_(false),
		M_(NumRows),
		N_(NumCols),
		LDA_(LDA),
		A_(A)
{
	if(A == 0)
		throw ReportError("Null pointer passed as A parameter.", -3);
	if(NumRows < 0)
		throw ReportError("NumRows = " + toString(NumRows) + ". Should be >= 0", -1);
	if(NumCols < 0)
		throw ReportError("NumCols = " + toString(NumCols) + ". Should be >= 0", -1);
	if(LDA < 0)
		throw ReportError("LDA = " + toString(LDA) + ". Should be >= 0", -1);

  if(CV == Copy) {
    LDA_ = M_;
		const int newsize = LDA_ * N_;
		if(newsize > 0) {
			A_ = new int[newsize];
			CopyMat(A, LDA, M_, N_, A_, LDA_);
			A_Copied_ = true;
		}
		else {
			A_ = 0;
		}
  }
}
//=============================================================================
Epetra_IntSerialDenseMatrix::Epetra_IntSerialDenseMatrix(const Epetra_IntSerialDenseMatrix& Source)
  : Epetra_Object(Source),
		CV_(Source.CV_),
    A_Copied_(false),
    M_(Source.M_),
    N_(Source.N_),
    LDA_(Source.LDA_),
    A_(Source.A_)
{
	if(CV_ == Copy) {
		LDA_ = M_;
		const int newsize = LDA_ * N_;
		if(newsize > 0) {
			A_ = new int[newsize];
			CopyMat(Source.A_, Source.LDA_, M_, N_, A_, LDA_);
			A_Copied_ = true;
		}
		else {
			A_ = 0;
			A_Copied_ = false;
		}
	}
}
//=============================================================================
int Epetra_IntSerialDenseMatrix::Reshape(int NumRows, int NumCols) {
	if(NumRows < 0 || NumCols < 0)
		return(-1);

	int* A_tmp = 0;
	const int newsize = NumRows * NumCols;

	if(newsize > 0) {
		// Allocate space for new matrix
		A_tmp = new int[newsize];
		for(int k = 0; k < newsize; k++) 
			A_tmp[k] = 0; // Zero out values
		int M_tmp = EPETRA_MIN(M_, NumRows);
		int N_tmp = EPETRA_MIN(N_, NumCols);
		if(A_ != 0) 
			CopyMat(A_, LDA_, M_tmp, N_tmp, A_tmp, NumRows); // Copy principal submatrix of A to new A
	}
  
  CleanupData(); // Get rid of anything that might be already allocated  
  M_ = NumRows;
  N_ = NumCols;
  LDA_ = M_;
	if(newsize > 0) {
		A_ = A_tmp; // Set pointer to new A
		A_Copied_ = true;
	}

  return(0);
}
//=============================================================================
int Epetra_IntSerialDenseMatrix::Shape(int NumRows, int NumCols) {
	if(NumRows < 0 || NumCols < 0)
		return(-1);

  CleanupData(); // Get rid of anything that might be already allocated
  M_ = NumRows;
  N_ = NumCols;
  LDA_ = M_;
	const int newsize = LDA_ * N_;
	if(newsize > 0) {
		A_ = new int[newsize];
		for(int k = 0; k < newsize; k++)
			A_[k] = 0; // Zero out values
		A_Copied_ = true;
	}

  return(0);
}
//=============================================================================
Epetra_IntSerialDenseMatrix::~Epetra_IntSerialDenseMatrix()
{
  CleanupData();
}
//=============================================================================
void Epetra_IntSerialDenseMatrix::CleanupData()
{
  if(A_Copied_)
		delete[] A_; 
	A_ = 0; 
	A_Copied_ = false;
	M_ = 0;
	N_ = 0;
	LDA_ = 0;
}
//=============================================================================
Epetra_IntSerialDenseMatrix& Epetra_IntSerialDenseMatrix::operator = (const Epetra_IntSerialDenseMatrix& Source) {
  if(this == &Source)
		return(*this); // Special case of source same as target
	if((CV_ == View) && (Source.CV_ == View) && (A_ == Source.A_))
		return(*this); // Special case of both are views to same data.

	if(strcmp(Label(), Source.Label()) != 0)
		throw ReportError("operator= type mismatch (lhs = " + string(Label()) + 
											", rhs = " + string(Source.Label()) + ").", -5);
	
	if(Source.CV_ == View) {
		if(CV_ == Copy) { // C->V only
			CleanupData();
			CV_ = View;
		}
		M_ = Source.M_; // C->V and V->V
		N_ = Source.N_;
		LDA_ = Source.LDA_;
		A_ = Source.A_;
	}
	else {
		if(CV_ == View) { // V->C
			CV_ = Copy;
			M_ = Source.M_;
			N_ = Source.N_;
			LDA_ = Source.M_;
			const int newsize = LDA_ * N_;
			if(newsize > 0) {
				A_ = new int[newsize];
				A_Copied_ = true;
			}
			else {
				A_ = 0;
				A_Copied_ = false;
			}
		}
		else { // C->C
			if((Source.M_ <= LDA_) && (Source.N_ == N_)) { // we don't need to reallocate
				M_ = Source.M_;
				N_ = Source.N_;
			}
			else { // we need to allocate more space (or less space)
				CleanupData();
				M_ = Source.M_;
				N_ = Source.N_;
				LDA_ = Source.M_;
				const int newsize = LDA_ * N_;
				if(newsize > 0) {
					A_ = new int[newsize];
					A_Copied_ = true;
				}
			}
		}
		CopyMat(Source.A_, Source.LDA_, M_, N_, A_, LDA_); // V->C and C->C
	}
	
  return(*this);
}

//=============================================================================
int Epetra_IntSerialDenseMatrix::MakeViewOf(const Epetra_IntSerialDenseMatrix& Source) {
	if(strcmp(Label(), Source.Label()) != 0)
		return(-1);

	if(CV_ == Copy) {
		CleanupData();
		CV_ = View;
	}
	M_ = Source.M_;
	N_ = Source.N_;
	LDA_ = Source.LDA_;
	A_ = Source.A_;

	return(0);
}

//=============================================================================
void Epetra_IntSerialDenseMatrix::CopyMat(int* Source, int Source_LDA, int NumRows, int NumCols, 
																					int* Target, int Target_LDA) 
{
  int i, j;
  int* targetPtr = Target;
  int* sourcePtr = 0;
  for(j = 0; j < NumCols; j++) { // for each column
    targetPtr = Target + j * Target_LDA; // set pointers to correct stride
		sourcePtr = Source + j * Source_LDA;
    for(i = 0; i < NumRows; i++) // for each row
			*targetPtr++ = *sourcePtr++; // copy element (i,j) and increment pointer to (i,j+1)
  }
  return;
}
//=============================================================================
int Epetra_IntSerialDenseMatrix::OneNorm() {
	int anorm = 0;
	int* ptr = 0;
	for(int j = 0; j < N_; j++) {
		int sum = 0;
		ptr = A_ + j*LDA_;
		for(int i = 0; i < M_; i++) 
			sum += abs(*ptr++);
		anorm = EPETRA_MAX(anorm, sum);
	}
	return(anorm);
}
//=============================================================================
int Epetra_IntSerialDenseMatrix::InfNorm() {	
	int anorm = 0;
	int* ptr = 0;
	// Loop across columns in inner loop.  Most expensive memory access, but 
	// requires no extra storage.
	for(int i = 0; i < M_; i++) {
		int sum = 0;
		ptr = A_ + i;
		for(int j = 0; j < N_; j++) {
			sum += abs(*ptr);
			ptr += LDA_;
		}
		anorm = EPETRA_MAX(anorm, sum);
	}
	return(anorm);
}
//=========================================================================
int& Epetra_IntSerialDenseMatrix::operator () (int RowIndex, int ColIndex)  {
  if(RowIndex >= M_ || RowIndex < 0) 
		throw ReportError("Row index = " + toString(RowIndex) + 
											" Out of Range 0 - " + toString(M_-1),-1);
  if(ColIndex >= N_ || ColIndex < 0) 
		throw ReportError("Column index = " + toString(ColIndex) + 
											" Out of Range 0 - " + toString(N_-1),-2);
  return(A_[ColIndex*LDA_ + RowIndex]);
}
//=========================================================================
const int& Epetra_IntSerialDenseMatrix::operator () (int RowIndex, int ColIndex) const  {
  if(RowIndex >= M_ || RowIndex < 0) 
		throw ReportError("Row index = " + toString(RowIndex) + 
											" Out of Range 0 - " + toString(M_-1),-1);
  if(ColIndex >= N_ || ColIndex < 0) 
		throw ReportError("Column index = " + toString(ColIndex) + 
											" Out of Range 0 - " + toString(N_-1),-2);
	return(A_[ColIndex * LDA_ + RowIndex]);
}
//=========================================================================
const int* Epetra_IntSerialDenseMatrix::operator [] (int ColIndex) const  {
  if(ColIndex >= N_ || ColIndex < 0) 
		throw ReportError("Column index = " + toString(ColIndex) + 
											" Out of Range 0 - " + toString(N_-1),-2);
  return(A_ + ColIndex * LDA_);
}
//=========================================================================
int* Epetra_IntSerialDenseMatrix::operator [] (int ColIndex)  {
  if(ColIndex >= N_ || ColIndex < 0) 
		throw ReportError("Column index = " + toString(ColIndex) + 
											" Out of Range 0 - " + toString(N_-1),-2);
  return(A_+ ColIndex * LDA_);
}
//=========================================================================
void Epetra_IntSerialDenseMatrix::Print(ostream& os) const {
	if(CV_ == Copy)
		os << "Data access mode: Copy" << endl;
	else
		os << "Data access mode: View" << endl;
	if(A_Copied_)
		os << "A_Copied: yes" << endl;
	else
		os << "A_Copied: no" << endl;
	os << "Rows(M): " << M_ << endl;
	os << "Columns(N): " << N_ << endl;
	os << "LDA: " << LDA_ << endl;
	if(M_ == 0 || N_ == 0)
		os << "(matrix is empty, no values to display)" << endl;
	else
		for(int i = 0; i < M_; i++) {
			for(int j = 0; j < N_; j++){
				os << (*this)(i,j) << " ";
			}
			os << endl;
		}
}
//=========================================================================
int Epetra_IntSerialDenseMatrix::Random() {
	// Generate random numbers drawn from a uniform distribution on
	// the interval [0,maxint) using a multiplicative congruential generator
	// with modulus 2^31 - 1.

	const int maxint = 100;

  const double a = 16807.0;
	const double BigInt = 2147483647.0;
	double seed = rand(); // Use POSIX standard random function

	double randdouble;
	int randint;

	int* arrayPtr = 0;

  for(int j = 0; j < N_; j++) {
    arrayPtr = A_ + (j * LDA_);
    for(int i = 0; i < M_; i++) {
			seed = fmod(a * seed, BigInt);
			randdouble = (seed / BigInt);
			randint = int(randdouble * maxint);
			*arrayPtr++ = randint;
		}
	}
	
	return(0);
}
