//
// File AnasaziPetra.hpp: interface for the AnasaziPetra class.
//
#ifndef ANASAZI_PETRA_HPP
#define ANASAZI_PETRA_HPP

#include "AnasaziMultiVec.hpp"
#include "AnasaziMatrix.hpp"
#include "AnasaziOperator.hpp"
#include "AnasaziCommon.hpp"
#include "AnasaziReturnType.hpp"

#include "Epetra_MultiVector.h"
#include "Epetra_Operator.h"
#include "Epetra_Map.h"
#include "Epetra_LocalMap.h"

//--------template class AnasaziPetraVec-------------------------------------
template <class TYPE>
class AnasaziPetraVec : public AnasaziMultiVec<TYPE>, public Epetra_MultiVector {
public:
// constructors
	AnasaziPetraVec(const Epetra_BlockMap&, TYPE *, const int, const int stride=0);
	AnasaziPetraVec(const Epetra_BlockMap&, const int);
	AnasaziPetraVec(Epetra_DataAccess CV, const Epetra_MultiVector& P_vec, int index[], int NumVecs );
	AnasaziPetraVec(const Epetra_MultiVector & P_vec);
	~AnasaziPetraVec();
	//
	//  member functions inherited from AnasaziMultiVec
	//
	//  the following is a virtual copy constructor returning
	//  a pointer to the pure virtual class. vector values are
	//  not copied; instead a new MultiVec is created containing
	//  a non-zero amount of columns.
	//
	AnasaziMultiVec<TYPE> * Clone ( const int numvecs );
	//
	//  the following is a virtual copy constructor returning
	//  a pointer to the pure virtual class. vector values are
	//  copied and a new stand-alone MultiVector is created.
	//  (deep copy).
	//
	AnasaziMultiVec<TYPE> * CloneCopy ();
	//
	//  the following is a virtual copy constructor returning
	//  a pointer to the pure virtual class. vector values are
	//  copied and a new stand-alone MultiVector is created
	//  where only selected columns are chosen.  (deep copy).
	//
	AnasaziMultiVec<TYPE> * CloneCopy ( int index[], int numvecs );
	//
	//  the following is a virtual view constructor returning
	//  a pointer to the pure virtual class. vector values are 
	//  shared and hence no memory is allocated for the columns.
	//
	AnasaziMultiVec<TYPE> * CloneView ( int index[], int numvecs);
	//
	//  this routine sets a subblock of the multivector, which
	//  need not be contiguous, and is given by the indices.
	//
	void SetBlock ( AnasaziMultiVec<TYPE>& A, int index[], int numvecs );
	//
	int GetNumberVecs () const { return NumVectors(); }
	int GetVecLength () const { return MyLength(); }
	//
	// *this <- alpha * A * B + beta * (*this)
	//
	void MvTimesMatAddMv ( TYPE alpha, AnasaziMultiVec<TYPE>& A, 
		AnasaziDenseMatrix<TYPE>& B, TYPE beta );
	//
	// *this <- alpha * A + beta * B
	//
	void MvAddMv ( TYPE alpha , AnasaziMultiVec<TYPE>& A, TYPE beta,
		AnasaziMultiVec<TYPE>& B);
	//
	// B <- alpha * A^T * (*this)
	//
	void MvTransMv ( TYPE alpha, AnasaziMultiVec<TYPE>& A, AnasaziDenseMatrix<TYPE>& B );
	//
	// alpha[i] = norm of i-th column of (*this)
	//
	void MvNorm ( TYPE* normvec);
	//
	// random vectors in i-th column of (*this)
	//
	void MvRandom();
        //
        // initializes each element of (*this) with alpha
        //
        void MvInit ( TYPE alpha );
	//
	// print (*this)
	//
	void MvPrint();
private:
};
//-------------------------------------------------------------

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

template<class TYPE>
AnasaziPetraVec<TYPE>::AnasaziPetraVec(const Epetra_BlockMap& Map, TYPE * array, 
		   				const int numvec, const int stride)
	: Epetra_MultiVector(Copy, Map, array, stride, numvec) 
{
}

template<class TYPE>
AnasaziPetraVec<TYPE>::AnasaziPetraVec(const Epetra_BlockMap& Map, const int numvec)
	: Epetra_MultiVector(Map, numvec) 
{
}

template<class TYPE>
AnasaziPetraVec<TYPE>::AnasaziPetraVec(Epetra_DataAccess CV, const Epetra_MultiVector& P_vec, 
						int index[], int NumVecs )
	: Epetra_MultiVector(CV, P_vec, index, NumVecs) 
{
}

template<class TYPE>
AnasaziPetraVec<TYPE>::AnasaziPetraVec(const Epetra_MultiVector& P_vec)
	: Epetra_MultiVector(P_vec) 
{
}

template<class TYPE>
AnasaziPetraVec<TYPE>::~AnasaziPetraVec() 
{
}
//
//  member functions inherited from AnasaziMultiVec
//
//
//  Simulating a virtual copy constructor. If we could rely on the co-variance
//  of virtual functions, we could return a pointer to AnasaziPetraVec<TYPE>
//  (the derived type) instead of a pointer to the pure virtual base class.
//
template<class TYPE>
AnasaziMultiVec<TYPE>* AnasaziPetraVec<TYPE>::Clone ( const int NumVecs ) 
{
	AnasaziPetraVec * ptr_apv = new AnasaziPetraVec(Map(),NumVecs);
	return ptr_apv; // safe upcast.
}
//
//  the following is a virtual copy constructor returning
//  a pointer to the pure virtual class. vector values are
//  copied.
//
template<class TYPE>
AnasaziMultiVec<TYPE>* AnasaziPetraVec<TYPE>::CloneCopy() 
{
	AnasaziPetraVec *ptr_apv = new AnasaziPetraVec(*this);
	return ptr_apv; // safe upcast
}

template<class TYPE>
AnasaziMultiVec<TYPE>* AnasaziPetraVec<TYPE>::CloneCopy ( int index[], int numvecs ) 
{
	AnasaziPetraVec * ptr_apv = new AnasaziPetraVec(Copy, *this, index, numvecs );
	return ptr_apv; // safe upcast.
}

template<class TYPE>
AnasaziMultiVec<TYPE>* AnasaziPetraVec<TYPE>::CloneView ( int index[], int numvecs ) 
{
	AnasaziPetraVec * ptr_apv = new AnasaziPetraVec(View, *this, index, numvecs );
	return ptr_apv; // safe upcast.
}

template<class TYPE>
void AnasaziPetraVec<TYPE>::SetBlock(AnasaziMultiVec<TYPE>& A, int index[], int numvecs ) 
{	
	int i,j,ind;
	AnasaziPetraVec *A_vec = dynamic_cast<AnasaziPetraVec *>(&A); assert(A_vec);
	int MyNumVecs = (*this).GetNumberVecs();
	int VecLength = A.GetVecLength();

	// Set the vector values in the right order, careful that the index
	// doesn't go beyond the bounds of the multivector
	for ( j=0; j< numvecs; j++) {
		ind = index[j];
		if (ind < MyNumVecs) {
			for ( i=0; i<VecLength; i++) {
				(*this)[ind][i] = (*A_vec)[j][i];	
			}
		}
	}
}								
//
// *this <- alpha * A * B + beta * (*this)
//
template<class TYPE>
void AnasaziPetraVec<TYPE>::MvTimesMatAddMv ( TYPE alpha, AnasaziMultiVec<TYPE>& A, 
						   AnasaziDenseMatrix<TYPE>& B, TYPE beta ) 
{
	int info=0;
	const int izero=0;
	char* trans="N";
	Epetra_LocalMap LocalMap(B.getrows(), izero, Map().Comm());
	Epetra_MultiVector B_Pvec(Copy, LocalMap, B.getarray(), B.getld(), B.getcols());

	AnasaziPetraVec *A_vec = dynamic_cast<AnasaziPetraVec *>(&A); assert(A_vec);

	info = Multiply( *trans, *trans, alpha, *A_vec, B_Pvec, beta );

	assert(info==0);
}
//
// *this <- alpha * A + beta * B
//
template<class TYPE>
void AnasaziPetraVec<TYPE>::MvAddMv ( TYPE alpha , AnasaziMultiVec<TYPE>& A, 
						   TYPE beta, AnasaziMultiVec<TYPE>& B) 
{
	int info=0;
	const TYPE one =1.0;
	const TYPE zero = 0.0;

	AnasaziPetraVec *A_vec = dynamic_cast<AnasaziPetraVec *>(&A); assert(A_vec);
	AnasaziPetraVec *B_vec = dynamic_cast<AnasaziPetraVec *>(&B); assert(B_vec);

	info = Update( alpha, *A_vec, beta, *B_vec, zero ); assert(info==0);
}
//
// dense B <- alpha * A^T * (*this)
//
template<class TYPE>
void AnasaziPetraVec<TYPE>::MvTransMv ( TYPE alpha, AnasaziMultiVec<TYPE>& A,
						   AnasaziDenseMatrix<TYPE>& B) 
{
	int info=0;
	const int izero=0;
	const TYPE zero=0.0;
	//const TYPE one=1.0;
	char* trans1="T";
	char* trans2="N";

	AnasaziPetraVec *A_vec = dynamic_cast<AnasaziPetraVec *>(&A);

	if (A_vec) {

		Epetra_LocalMap LocalMap(B.getrows(), izero, Map().Comm());
		Epetra_MultiVector B_Pvec(View, LocalMap, B.getarray(), B.getld(), B.getcols());

		info = B_Pvec.Multiply( *trans1, *trans2, alpha, *A_vec, *this, zero ); 
		assert(info==0);
	}
}
//
// alpha[i] = norm of i-th column of (*this)
//
template<class TYPE>
void AnasaziPetraVec<TYPE>::MvNorm ( TYPE * normvec ) 
{
	int info=0;
	if (normvec) {
		info = Norm2(normvec);
		assert(info==0);
	}
}
//
// random vectors in i-th column of (*this)
//
template<class TYPE>
void AnasaziPetraVec<TYPE>::MvRandom () 
{
	int info=0;
	info = Random();
	assert(info==0);
}
//
// initializes each element of (*this) with alpha
//

template<class TYPE>
void AnasaziPetraVec<TYPE>::MvInit( TYPE alpha )
{	
	int i,j;
	int MyNumVecs = (*this).GetNumberVecs();
	int MyVecLength = (*this).GetVecLength();

	// Set the vector values in the right order, careful that the index
	// doesn't go beyond the bounds of the multivector
	for ( j=0; j< MyNumVecs; j++) {
		for ( i=0; i<MyVecLength; i++) {
			(*this)[j][i] = alpha;	
		}
	}
}								
//
//  print multivectors
//
template<class TYPE>
void AnasaziPetraVec<TYPE>::MvPrint() 
{
	cout << *this << endl;
}

///////////////////////////////////////////////////////////////
//--------template class AnasaziPetraMat-----------------------
template <class TYPE> 
class AnasaziPetraMat : public virtual AnasaziMatrix<TYPE> {
public:
	AnasaziPetraMat(const Epetra_Operator& );
	~AnasaziPetraMat();
	Anasazi_ReturnType ApplyMatrix ( const AnasaziMultiVec<TYPE>& x, 
					AnasaziMultiVec<TYPE>& y ) const;
private:
	const Epetra_Operator & Epetra_Mat;
};
//-------------------------------------------------------------
//
// implementation of the AnasaziPetraMat class.
//
////////////////////////////////////////////////////////////////////
//
// AnasaziMatrix constructors
//
template <class TYPE>
AnasaziPetraMat<TYPE>::AnasaziPetraMat(const Epetra_Operator& Matrix) 
	: Epetra_Mat(Matrix) 
{
}

template <class TYPE>
AnasaziPetraMat<TYPE>::~AnasaziPetraMat() 
{
}

//
// AnasaziMatrix matrix multiply
//
template <class TYPE>
Anasazi_ReturnType AnasaziPetraMat<TYPE>::ApplyMatrix ( const AnasaziMultiVec<TYPE>& x, 
						  AnasaziMultiVec<TYPE>& y ) const 
{
	int info=0;
	AnasaziMultiVec<TYPE> & temp_x = const_cast<AnasaziMultiVec<TYPE> &>(x);
	Epetra_MultiVector* vec_x = dynamic_cast<Epetra_MultiVector* >(&temp_x);
	Epetra_MultiVector* vec_y = dynamic_cast<Epetra_MultiVector* >(&y);

	assert( vec_x && vec_y );
	//
	// Need to cast away constness because the member function Apply
	// is not declared const.
	//
	info=const_cast<Epetra_Operator&>(Epetra_Mat).Apply( *vec_x, *vec_y );
	if (info==0) { 
		return Ok; 
	} else { 
		return Failed; 
	}	
}

///////////////////////////////////////////////////////////////
//--------template class AnasaziPetraStdOp---------------------
template <class TYPE> 
class AnasaziPetraStdOp : public virtual AnasaziOperator<TYPE> {
public:
	AnasaziPetraStdOp(const Epetra_Operator& );
	~AnasaziPetraStdOp();
	Anasazi_ReturnType ApplyOp ( const AnasaziMultiVec<TYPE>& x, 
					AnasaziMultiVec<TYPE>& y ) const;
private:
	const Epetra_Operator & Epetra_Op;
};
//-------------------------------------------------------------
//
// implementation of the AnasaziPetraStdOp class.
//
////////////////////////////////////////////////////////////////////
//
// AnasaziOperator constructors
//
template <class TYPE>
AnasaziPetraStdOp<TYPE>::AnasaziPetraStdOp(const Epetra_Operator& Op) 
	: Epetra_Op(Op)
{
}

template <class TYPE>
AnasaziPetraStdOp<TYPE>::~AnasaziPetraStdOp() 
{
}
//
// AnasaziOperator applications
//
template <class TYPE>
Anasazi_ReturnType AnasaziPetraStdOp<TYPE>::ApplyOp ( const AnasaziMultiVec<TYPE>& x, 
						  AnasaziMultiVec<TYPE>& y ) const 
{
	int info=0;
	AnasaziMultiVec<TYPE> & temp_x = const_cast<AnasaziMultiVec<TYPE> &>(x);
	Epetra_MultiVector* vec_x = dynamic_cast<Epetra_MultiVector* >(&temp_x);
	Epetra_MultiVector* vec_y = dynamic_cast<Epetra_MultiVector* >(&y);

	assert( vec_x && vec_y );
	//
	// Need to cast away constness because the member function Apply
	// is not declared const.
	//
	info=const_cast<Epetra_Operator&>(Epetra_Op).Apply( *vec_x, *vec_y );
	if (info==0) { 
		return Ok; 
	} else { 
		return Failed; 
	}	
}

///////////////////////////////////////////////////////////////
//--------template class AnasaziPetraGenOp---------------------
template <class TYPE> 
class AnasaziPetraGenOp : public virtual AnasaziOperator<TYPE> {
public:
	AnasaziPetraGenOp(const Epetra_Operator&, const Epetra_Operator& );
	~AnasaziPetraGenOp();
	Anasazi_ReturnType ApplyOp ( const AnasaziMultiVec<TYPE>& x, 
					AnasaziMultiVec<TYPE>& y ) const;
private:
	const Epetra_Operator & Epetra_AOp;
	const Epetra_Operator & Epetra_BOp;
};
//-------------------------------------------------------------
//
// implementation of the AnasaziPetraGenOp class.
//
////////////////////////////////////////////////////////////////////
//
// AnasaziOperator constructors
//
template <class TYPE>
AnasaziPetraGenOp<TYPE>::AnasaziPetraGenOp(const Epetra_Operator& AOp,
					const Epetra_Operator& BOp) 
	: Epetra_AOp(AOp), Epetra_BOp(BOp) 
{
}

template <class TYPE>
AnasaziPetraGenOp<TYPE>::~AnasaziPetraGenOp() 
{
}
//
// AnasaziOperator applications
//
template <class TYPE>
Anasazi_ReturnType AnasaziPetraGenOp<TYPE>::ApplyOp ( const AnasaziMultiVec<TYPE>& x, 
						  AnasaziMultiVec<TYPE>& y ) const 
{
	int info=0;
	AnasaziMultiVec<TYPE> & temp_x = const_cast<AnasaziMultiVec<TYPE> &>(x);
	Epetra_MultiVector* vec_x = dynamic_cast<Epetra_MultiVector* >(&temp_x);
	Epetra_MultiVector* vec_y = dynamic_cast<Epetra_MultiVector* >(&y);
	Epetra_MultiVector temp_y(*vec_y); 

	assert( vec_x && vec_y );
	//
	// Need to cast away constness because the member function Apply
	// is not declared const.
	//
	info=const_cast<Epetra_Operator&>(Epetra_BOp).Apply( *vec_x, temp_y );
	assert(info==0);
	info=const_cast<Epetra_Operator&>(Epetra_AOp).Apply( temp_y, *vec_y );
	if (info==0) { 
		return Ok; 
	} else { 
		return Failed; 
	}	
}

#endif 
 // end of file ANASAZI_PETRA_HPP
