// File AnasaziLOCA.hpp: interface for the AnasaziLOCA class.
//
#ifndef ANASAZI_LOCA_HPP
#define ANASAZI_LOCA_HPP

#include "AnasaziMatrix.hpp"
#include "AnasaziMultiVec.hpp"
#include "AnasaziCommon.hpp"
#include "AnasaziReturnType.hpp"
#include "NOX_Abstract_Vector.H"
#include "LOCA_Abstract_Group.H"
#include "NOX_Parameter_List.H"

namespace Anasazi {
enum DataAccess {Copy, View};
}
//
//--------template class AnasaziLOCAMat-----------------------
//
template <class TYPE> 
class AnasaziLOCAMat : public AnasaziMatrix<TYPE> {
public:
	AnasaziLOCAMat( NOX::Parameter::List&, NOX::Abstract::Group& );
	~AnasaziLOCAMat();
	Anasazi_ReturnType ApplyMatrix ( const AnasaziMultiVec<TYPE>& x, 
					AnasaziMultiVec<TYPE>& y ) const;
private:
	NOX::Parameter::List& locaParams;
	NOX::Abstract::Group& locaGroup;
};
//------------------------------------------------------------
//
//--------template class AnasaziLOCAVec-------------------------------------
//
template <class TYPE>
class AnasaziLOCAVec : public AnasaziMultiVec<TYPE> {
public:
	friend class AnasaziLOCAMat<TYPE>;
// constructors
	AnasaziLOCAVec(const NOX::Abstract::Vector& N_vec, int NumVecs );
	AnasaziLOCAVec(const vector< NOX::Abstract::Vector *> N_vecPtrs, 
			Anasazi::DataAccess type = Anasazi::Copy );
	AnasaziLOCAVec(const AnasaziLOCAVec<TYPE>& source, 
			Anasazi::DataAccess type = Anasazi::Copy );
	AnasaziLOCAVec(Anasazi::DataAccess type, const AnasaziLOCAVec<TYPE>& source, 
			int index[], int NumVecs); 

	~AnasaziLOCAVec();
	//
	//  member functions inherited from AnasaziMultiVec
	//
	//  the following is a virtual copy constructor returning
	//  a pointer to the pure virtual class. vector values are
	//  not copied; instead a new MultiVec is created containing
	//  a non-zero amount of columns.
	//
	virtual AnasaziMultiVec<TYPE> * Clone ( const int );
	//
	//  the following is a virtual copy constructor returning
	//  a pointer to the pure virtual class. vector values are
	//  copied and a new stand-alone MultiVector is created.
	//  (deep copy).
	//
	virtual AnasaziMultiVec<TYPE> * CloneCopy ();
	//
	//  Selective deep copy (or copy) constructor.
	//
	virtual AnasaziMultiVec<TYPE> * CloneCopy ( int [], int );
	//
	//  the following is a virtual view constructor returning
	//  a pointer to the pure virtual class. vector values are 
	//  shared and hence no memory is allocated for the columns.
	//
	virtual AnasaziMultiVec<TYPE> * CloneView ( int [], int );
	//
	virtual int GetNumberVecs () const;
	virtual int GetVecLength () const;
	//
	//  set a block of this multivec with the multivecs specified by
	//  the index.
	//
	virtual void SetBlock ( AnasaziMultiVec<TYPE>& A, int index[], 
		int NumVecs ); 
	//
	// *this <- alpha * A * B + beta * (*this)
	//
	virtual void MvTimesMatAddMv ( TYPE alpha, AnasaziMultiVec<TYPE>& A, 
		AnasaziDenseMatrix<TYPE>& B, TYPE beta );
	//
	// *this <- alpha * A + beta * B
	//
	virtual void MvAddMv ( TYPE alpha , AnasaziMultiVec<TYPE>& A, TYPE beta,
		AnasaziMultiVec<TYPE>& B);
	//
	// B <- alpha * A^T * (*this)
	//
	virtual void MvTransMv ( TYPE alpha, AnasaziMultiVec<TYPE>& A, 
		AnasaziDenseMatrix<TYPE>& B );
	//
	// alpha[i] = norm of i-th column of (*this)
	//
	virtual void MvNorm ( TYPE* normvec );
	//
	// random vectors in i-th column of (*this)
	//
	virtual	void MvRandom();
        //
        // initializes each element of (*this) with alpha
        //
        virtual void MvInit ( TYPE alpha );
	//
	// print (*this)
	//
	virtual void MvPrint();
	//
	// Return a pointer to specific NOX::Abstract::Vector for LOCA computation.
	// If index is not a valid index for this multivec, then nothing is done.
	// NOTE:  This method is not included in the AnasaziMultiVec virtual base class.
	//
	virtual void GetNOXVector( NOX::Abstract::Vector& Vec, int index );
	// 
private:
// Data container
 	vector< NOX::Abstract::Vector* > mvPtrs;
	Anasazi::DataAccess CV;
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

template<class TYPE>
AnasaziLOCAVec<TYPE>::AnasaziLOCAVec( const NOX::Abstract::Vector& N_vec, int NumVecs ) :
					mvPtrs(NumVecs), CV(Anasazi::Copy)
{
	for (int i=0; i<NumVecs; i++) {
		mvPtrs[i] = N_vec.clone(NOX::ShapeCopy);
		mvPtrs[i]->init(0.0);
//		cout<<"nox_vec_init "<<i<<"\t"<<
//			typeid(*(mvPtrs[i])).name()<<endl;
	}
}

template<class TYPE>
AnasaziLOCAVec<TYPE>::AnasaziLOCAVec( const vector< NOX::Abstract::Vector *> N_vecPtrs,
					Anasazi::DataAccess type ) : 
					mvPtrs(N_vecPtrs.size()), CV(type)
{
	int i;
	if (type == Anasazi::Copy) {
		for ( i=0; i<mvPtrs.size(); i++) {
			mvPtrs[i] = N_vecPtrs[i]->clone(NOX::DeepCopy);
		}
	} 
	else {
		for ( i=0; i<mvPtrs.size(); i++) {
			mvPtrs[i] = N_vecPtrs[i];
		}
	}
}

template<class TYPE>
AnasaziLOCAVec<TYPE>::AnasaziLOCAVec( const AnasaziLOCAVec<TYPE>& source, 
			Anasazi::DataAccess type ) : mvPtrs(source.mvPtrs.size()),
			CV(type)
{
	int i;

	if (type == Anasazi::Copy) {
		for (i=0; i<mvPtrs.size(); i++) {
			mvPtrs[i] = source.mvPtrs[i]->clone(NOX::DeepCopy);
		}
	}
	else {
		for (i=0; i<mvPtrs.size(); i++) {
			mvPtrs[i] = source.mvPtrs[i];
		}
	}
}

template<class TYPE>
AnasaziLOCAVec<TYPE>::AnasaziLOCAVec( Anasazi::DataAccess type, const AnasaziLOCAVec<TYPE>& source, 
					int index[], int NumVecs ): mvPtrs(NumVecs), CV(type)
{
	int i;

	if (type == Anasazi::Copy) {
		for ( i=0; i<NumVecs; i++ ) {
			mvPtrs[i] = source.mvPtrs[ index[i] ]->clone(NOX::DeepCopy);
//			cout<<"ALV_copy_init "<<i<<"\t"<<
//				typeid(*(mvPtrs[i])).name()<<endl;
		}
	} 
	else {
		for ( i=0; i<NumVecs; i++ ) {
			mvPtrs[i] = source.mvPtrs[ index[i] ];
//			cout<<"ALV_view_init "<<i<<"\t"<<
//				typeid(*(mvPtrs[i])).name()<<endl;
		}
	}
}

template<class TYPE>
AnasaziLOCAVec<TYPE>::~AnasaziLOCAVec()
{
	if (CV == Anasazi::Copy) {
		for (int i=0; i<mvPtrs.size(); i++) {
			delete mvPtrs[i];
		}
	}
}
//
//  member functions inherited from AnasaziMultiVec
//
//
//  Simulating a virtual copy constructor. If we could rely on the co-variance
//  of virtual functions, we could return a pointer to AnasaziLOCAVec<TYPE>
//  (the derived type) instead of a pointer to the pure virtual base class.
//
template<class TYPE>
AnasaziMultiVec<TYPE>* AnasaziLOCAVec<TYPE>::Clone ( const int NumVecs ) {
	AnasaziLOCAVec *ptr_alv = new AnasaziLOCAVec(*(mvPtrs[0]),NumVecs);
	return ptr_alv; // safe upcast.
}
	//
	//  the following is a virtual copy constructor returning
	//  a pointer to the pure virtual class. vector values are
	//  copied.
	//
template<class TYPE>
AnasaziMultiVec<TYPE>* AnasaziLOCAVec<TYPE>::CloneCopy() {
	AnasaziLOCAVec *ptr_alv = new AnasaziLOCAVec(*this);
	return ptr_alv; // safe upcast
}

template<class TYPE>
AnasaziMultiVec<TYPE>* AnasaziLOCAVec<TYPE>::CloneCopy ( int index[], int NumVecs ) {
	
	AnasaziLOCAVec *ptr_alv = new AnasaziLOCAVec( Anasazi::Copy, *this, index, NumVecs );
	return ptr_alv; // safe upcast.
}

template<class TYPE>
AnasaziMultiVec<TYPE>* AnasaziLOCAVec<TYPE>::CloneView ( int index[], int NumVecs ) {
	
	AnasaziLOCAVec *ptr_alv = new AnasaziLOCAVec( Anasazi::View, *this, index, NumVecs );
	return ptr_alv; // safe upcast.
}

template<class TYPE>
int AnasaziLOCAVec<TYPE>::GetNumberVecs () const {
	return mvPtrs.size();
}

template<class TYPE>
int AnasaziLOCAVec<TYPE>::GetVecLength () const {
	return mvPtrs[0]->length();
}

template<class TYPE>
void AnasaziLOCAVec<TYPE>::SetBlock( AnasaziMultiVec<TYPE>& A, int index[], int NumVecs ) {

	int i, ind;
	AnasaziLOCAVec *A_vec = dynamic_cast<AnasaziLOCAVec *>(&A); assert(A_vec!=NULL);
	int MyNumVecs = mvPtrs.size();
	for (i=0; i<NumVecs; i++) {
		ind = index[i];
		if (ind < MyNumVecs) {
			delete mvPtrs[ind];
			mvPtrs[ind] = A_vec->mvPtrs[i]->clone(NOX::DeepCopy);				
		}
	}
}
//
// *this <- alpha * A * B + beta * (*this)
//
template<class TYPE>
void AnasaziLOCAVec<TYPE>::MvTimesMatAddMv ( TYPE alpha, AnasaziMultiVec<TYPE>& A, 
						   AnasaziDenseMatrix<TYPE>& B, TYPE beta ) 
{
	int i,j;
	AnasaziLOCAVec *A_vec = dynamic_cast<AnasaziLOCAVec *>(&A); assert(A_vec!=NULL);
	int m = B.getrows();
	int n = B.getcols();
	int ldb = B.getld();
	TYPE *Bvals = B.getarray();  	
	AnasaziLOCAVec *temp_vec = new AnasaziLOCAVec(*(mvPtrs[0]),n);
	temp_vec->MvInit(0.0);
	TYPE one = 1.0;
//
//	*this <- alpha * A * B + beta *(*this)
//
	for (j=0; j<n; j++) {
		for (i=0; i<m; i++) {
			temp_vec->mvPtrs[j]->update(Bvals[j*ldb+i], *(A_vec->mvPtrs[i]),one);
		}				
		mvPtrs[j]->update(alpha,*(temp_vec->mvPtrs[j]),beta);
	}
	delete temp_vec;
}
//
// *this <- alpha * A + beta * B
//
template<class TYPE>
void AnasaziLOCAVec<TYPE>::MvAddMv ( TYPE alpha , AnasaziMultiVec<TYPE>& A, 
						   TYPE beta, AnasaziMultiVec<TYPE>& B) {
	const TYPE zero = 0.0;
	AnasaziLOCAVec *A_vec = dynamic_cast<AnasaziLOCAVec *>(&A); assert(A_vec!=NULL);
	AnasaziLOCAVec *B_vec = dynamic_cast<AnasaziLOCAVec *>(&B); assert(B_vec!=NULL);

	for (int i=0; i<mvPtrs.size(); i++) {
		mvPtrs[i]->update(alpha, *(A_vec->mvPtrs[i]), beta, *(B_vec->mvPtrs[i]), zero);
	}		
}
//
// dense B <- alpha * A^T * (*this)
//
template<class TYPE>
void AnasaziLOCAVec<TYPE>::MvTransMv ( TYPE alpha, AnasaziMultiVec<TYPE>& A,
						   AnasaziDenseMatrix<TYPE>& B) {
	int i,j;
	AnasaziLOCAVec *A_vec = dynamic_cast<AnasaziLOCAVec *>(&A); assert(A_vec!=NULL);
	int m = B.getrows();
	int n = B.getcols();
	int ldb = B.getld();
	TYPE *Bvals = B.getarray();  	

	for (j=0; j<n; j++) {
		for (i=0; i<m; i++) {
			Bvals[j*ldb + i] = alpha * mvPtrs[j]->dot(*(A_vec->mvPtrs[i]));
		}
	}
}
//
// array[i] = norm of i-th column of (*this)
//
template<class TYPE>
void AnasaziLOCAVec<TYPE>::MvNorm ( TYPE * normvec ) 
{
	if (normvec) {
		for (int i=0; i<mvPtrs.size(); i++) {
//			cout<<i<<"\t"<<typeid(*(mvPtrs[i])).name()<<endl;
			normvec[i] = mvPtrs[i]->norm();
		}
	}
}
//
// random vectors in i-th column of (*this)
//
template<class TYPE>
void AnasaziLOCAVec<TYPE>::MvRandom () 
{
	for (int i=0; i<mvPtrs.size(); i++) {
		mvPtrs[i]->random();
	}	
}
//
// initializes each element of (*this) with alpha
//
template<class TYPE>
void AnasaziLOCAVec<TYPE>::MvInit ( TYPE alpha ) 
{
	for (int i=0; i<mvPtrs.size(); i++) {
		mvPtrs[i]->init( alpha );
	}	
}
//
//  print multivectors
//
template<class TYPE>
void AnasaziLOCAVec<TYPE>::MvPrint() {
//	cout << *this << endl;
}
//
//  return individual NOX Vector
//
template<class TYPE>
void AnasaziLOCAVec<TYPE>::GetNOXVector( NOX::Abstract::Vector& Vec, int index ) 
{
	if (index < mvPtrs.size()) { Vec = *(mvPtrs[index]); }
}

///////////////////////////////////////////////////////////////
//
// implementation of the AnasaziLOCAMat class.
//
////////////////////////////////////////////////////////////////////
//
// AnasaziMatrix constructors
//
template <class TYPE>
AnasaziLOCAMat<TYPE>::AnasaziLOCAMat(NOX::Parameter::List& params, 
					NOX::Abstract::Group& group) :
					locaParams(params), locaGroup(group) {
//	cout << "ctor:AnasaziLOCAMat " << this << endl;
	}

template <class TYPE>
AnasaziLOCAMat<TYPE>::~AnasaziLOCAMat() {
//	cout << "dtor:AnasaziLOCAMat " << this << endl;
	}
//
// AnasaziMatrix matrix multiply
//
template <class TYPE>
Anasazi_ReturnType AnasaziLOCAMat<TYPE>::ApplyMatrix ( const AnasaziMultiVec<TYPE>& x, 
					  AnasaziMultiVec<TYPE>& y ) const {
	
	NOX::Abstract::Group::ReturnType res;
	AnasaziMultiVec<TYPE> &temp_x = const_cast<AnasaziMultiVec<TYPE> &>(x);
	AnasaziLOCAVec<TYPE> *x_vec = dynamic_cast<AnasaziLOCAVec<TYPE> *>(&temp_x); assert(x_vec!=NULL);
	AnasaziLOCAVec<TYPE> *y_vec = dynamic_cast<AnasaziLOCAVec<TYPE> *>(&y); assert(y_vec!=NULL);

	int NumVecs = x_vec->GetNumberVecs();
	for (int i=0; i<NumVecs; i++) {
		res = locaGroup.applyJacobianInverse(locaParams, *(x_vec->mvPtrs[i]), 
						*(y_vec->mvPtrs[i])); 
//		res = locaGroup.applyJacobian(*(x_vec->mvPtrs[i]), *(y_vec->mvPtrs[i])); 
	}
	if (res == NOX::Abstract::Group::Ok) {
	    return Ok;
	} else {
	    return Failed;
	}
}

#endif 
 // end of file ANASAZI_LOCA_HPP
