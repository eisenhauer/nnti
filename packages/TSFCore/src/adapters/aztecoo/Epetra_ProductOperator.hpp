// ///////////////////////////////////////////////
// Epetra_ProductOperator.hpp

#ifndef EPETRA_PRODUCT_OPERATOR_HPP
#define EPETRA_PRODUCT_OPERATOR_HPP

#include "Epetra_Operator.h"
#include "Teuchos_RefCountPtr.hpp"
#include "Teuchos_BLAS_types.hpp"

class Epetra_Vector;

namespace Epetra {

///
/** Implements <tt>Epetra_Operator</tt> as a product of one or
 * more <tt>Epetra_Operator</tt> objects.
 *
 * This class implements a product operator of the form:
 *
 \verbatim

 M = M[0]*M[1]*...*M[num_Op-1]

 \endverbatim
 *
 * and operator applications are performed one constituent operator at
 * a time as:
 *
 \verbatim
 
 Forward Mat-vec: Y = M * X

   T[k-1] = M[k]*T[k]

     for k = num_Op-1...0

       where: T[num_Op-1] = X (input vector)
       where: T[-1]       = Y (output vector)

 Adjoint Mat-vec: Y = M' * X

   T[k] = M[k]'*T[k-1]

     for k = 0...num_Op-1

       where: T[-1]       = X (input vector)
       where: T[num_Op-1] = Y (output vector)

 \endverbatim
 *
 * Likewise, the inverse can also be applied (if all of the
 * constituent operators support the inverse operation) as:
 *
 \verbatim
 
 Forward Inverse Mat-vec: Y = inv(M) * X

   T[k] = inv(M[k])*T[k-1]

     for k = 0...num_Op-1

     for k = 0...num_Op-1

       where: T[-1]       = X (input vector)
       where: T[num_Op-1] = Y (output vector)

 Adjoint Inverse Mat-vec: Y = inv(M') * X

   T[k] = inv(M[k]')*T[k-1]

     for k = num_Op-1...0

       where: T[num_Op-1] = X (input vector)
       where: T[-1]       = Y (output vector)

 \endverbatim
 *
 * Note that maps for the result of the inverse of an operator is the
 * same as the result of the adjoint of the operator and the map for
 * the result of the inverse of the adjoint is the same as for the
 * result the non-inverse forward opeator.
 *
 * The client constructs this object with a list of
 * <tt>Epetra_Operator</tt> objects an how the non-transposed operator
 * is to be viewed and if it is to be views as its inverse or not
 * (see <tt>initialize()</tt>).
 *
 * Note: The Epetra_Map objects returned from OperatorDomainMap() and
 * OperatorRangeMap() must always be with respect to the
 * non-transposed operator!  This is very strange behavior and is
 * totally undocumented in the Epetra_Operator interface but it seems
 * to be the case.
 */
class ProductOperator : public Epetra_Operator {
public:

	/** @name Public types */
	//@{

	///
	enum EApplyMode { APPLY_MODE_APPLY, APPLY_MODE_APPLY_INVERSE };

	//@}

	/** @name Constructors / initializers / accessors */
	//@{

	/// Construct to uninitialized
	ProductOperator();

	/// Calls <tt>initialize()</tt>.
	ProductOperator(
		const int                                      num_Op
		,const Teuchos::RefCountPtr<Epetra_Operator>   Op[]
		,const Teuchos::ETransp                        Op_trans[]
		,const EApplyMode                              Op_inverse[]
		);

	///
	/** Setup with constituent operators.
	 *
	 * @param  num_Op  [in] Number of constinuent operators.
	 * @param  Op      [in] Array (length <tt>num_Op</tt>) of smart pointers
	 *                 to <tt>Epetra_Operator objects that implement each
	 *                 constituent operator.
	 * @param  Op_trans
	 *                 [in] Array (length <tt>num_Op</tt>) that determines
	 *                 the transpose mode of the constituent operator (see below).
	 * @param  Op_inverse
	 *                 [in] Array (length <tt>num_Op</tt>) that determines
	 *                 the inverse apply mode of the constituent operator (see below).
	 *
	 * Preconditions:<ul>
	 * <li><tt>num_Op > 0</tt>
	 * <li><tt>Op!=NULL</tt>
	 * <li><tt>Op[k].get()!=NULL</tt>, for <tt>k=0...num_Op-1</tt>
	 * <li><tt>Op_trans!=NULL</tt>
	 * <li><tt>Op_inverse!=NULL</tt>
	 * </ul>
	 *
	 * Postconditions:<ul>
	 * <li><tt>this->num_Op()==num_Op</tt>
	 * <li><tt>this->Op(k).get()==Op[k].get()</tt>, for <tt>k=0...num_Op-1</tt>
	 * <li><tt>this->Op_trans(k)==Op_trans[k]</tt>, for <tt>k=0...num_Op-1</tt>
	 * <li><tt>this->Op_inverse(k)==Op_inverse[k]</tt>, for <tt>k=0...num_Op-1</tt>
	 * </ul>
	 *
	 * The forward constituent operator <tt>T[k-1] = M[k]*T[k]</tt> described in the
	 * main documenatation above is defined as follows:
	 *
	 \verbatim
   
   Op[k]->SetUseTranspose( Op_trans[k]!=Teuchos::NO_TRANS );
   if( Op_inverse[k]==APPLY_MODE_APPLY )
     Op[k]->Apply( T[k], T[k-1] );
	 else   
     Op[k]->ApplyInverse( T[k], T[k-1] );

	 \endverbatim
	 *
	 * The inverse constituent operator <tt>T[k] = inv(M[k])*T[k-1]</tt> described in the
	 * main documenatation above is defined as follows:
	 *
	 \verbatim
   
   Op[k]->SetUseTranspose( Op_trans[k]!=Teuchos::NO_TRANS );
   if( Op_inverse[k]==APPLY_MODE_APPLY )
     Op[k]->ApplyInverse( T[k-1], T[k] );
	 else   
     Op[k]->Apply( T[k-1], T[k] );

	 \endverbatim
	 *
	 * The other transposed constituent operators <tt>M[k]'</tt> and
	 * <tt>inv(M[k]')</tt> are defined by simply changing the value of
	 * the transpose as <tt>Op[k]->SetUseTranspose(
	 * Op_trans[k]==Teuchos::NO_TRANS );</tt>.  Note, that
	 * <tt>Op[k]->SetUseTranspose(...)</tt> is called immediately before
	 * <tt>Op[k]->Apply(...)</tt> or <tt>Op[k]->ApplyInverse(...)</tt>
	 * is called to avoid tricky problems that could occur with multiple
	 * uses of the same operator.
	 */
	void initialize(
		const int                                      num_Op
		,const Teuchos::RefCountPtr<Epetra_Operator>   Op[]
		,const Teuchos::ETransp                        Op_trans[]
		,const EApplyMode                              Op_inverse[]
		);

	///
	/** Set to an uninitialized state and wipe out memory.
	 *
	 * Postconditions:<ul>
	 * <li><tt>this->num_Op()==0</tt>
	 * </ul>
	 */
 	void uninitialize(
		int                                      *num_Op
		,Teuchos::RefCountPtr<Epetra_Operator>   Op[]
		,Teuchos::ETransp                        Op_trans[]
		,EApplyMode                              Op_inverse[]
		);

	///
	/** Apply the kth aggregate operator M[k] correctly.
	 *
	 * @param  k  [in] Gives the index (zero-based) of the constituent operator
	 *            to apply.
	 * @param  Op_trans
	 *            [in] Determines if the transpose of the constituent operator
	 *            is to be applied.
	 * @param  Op_inverse
	 *            [in] Determines if the operator or its inverse (if supported)
	 *            should be applied.
	 * @param  X  [in] The input vector.
	 * @param  Y  [out] The output vector.
	 *
	 * Clients should call this function to correctly apply a constitient operator!
	 */
	void applyConstituent(
		const int                  k
		,Teuchos::ETransp          Op_trans
		,EApplyMode                Op_inverse
		,const Epetra_MultiVector  &X_k
		,Epetra_MultiVector        *Y_k
		) const;

	///
	/** Return the number of aggregate opeators.
	 *
	 * A return value of <tt>0</tt> is a flag that <tt>this</tt>
	 * is not initialized yet.
	 */
	int num_Op() const;

	///
	/** Access the kth operator (zero-based).
	 *
	 * Preconditions:<ul>
	 * <li><tt>0 <= k <= this->num_Op()-1</tt>
	 * </ul>
	 *
	 * Warning! This is the raw opeator passed into
	 * <tt>initialize(...)</tt>.  In order to apply
	 * the constituent operator <tt>M[k]</tt> you must
	 * call <tt>ApplyConstituent()</tt>.
	 */
	Teuchos::RefCountPtr<Epetra_Operator> Op(int k) const;

	///
	/** Access the transpose mode of the kth operator (zero-based).
	 *
	 * Preconditions:<ul>
	 * <li><tt>0 <= k <= this->num_Op()-1</tt>
	 * </ul>
	 */
	Teuchos::ETransp Op_trans(int k) const;

	///
	/** Access the inverse mode of the kth operator (zero-based).
	 *
	 * Preconditions:<ul>
	 * <li><tt>0 <= k <= this->num_Op()-1</tt>
	 * </ul>
	 */
  EApplyMode Op_inverse(int k) const;

	//@}

	/** @name Overridden from Epetra_Operator */
	//@{
	
	///
	int SetUseTranspose(bool UseTranspose);
	///
	int Apply(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;
	///
	int ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;
	///
	double NormInf() const;
	///
	char * Label() const;
	///
	bool UseTranspose() const;
	///
	bool HasNormInf() const;
	///
	const Epetra_Comm & Comm() const;
	///
	const Epetra_Map & OperatorDomainMap() const;
	///
	const Epetra_Map & OperatorRangeMap() const;

	//@}

private:

	// ///////////////////
	// Private types

	typedef std::vector<Teuchos::RefCountPtr<Epetra_Operator> >  Op_t;
	typedef std::vector<Teuchos::ETransp>                        Op_trans_t;
	typedef std::vector<EApplyMode>                              Op_inverse_t;
	typedef std::vector<Teuchos::RefCountPtr<Epetra_Vector> >    EV_t;

	// /////////////////////
	// Private data members

	bool          UseTranspose_;
	Op_t          Op_;
	Op_trans_t    Op_trans_;
	Op_inverse_t  Op_inverse_;

	mutable EV_t  range_vecs_;
	mutable EV_t  domain_vecs_;

	// //////////////////////////
	// Private member functions

	void assertInitialized() const;
	void validateIndex(int k) const;
	void initializeTempVecs(bool applyInverse) const;

}; // class ProductOperator

// //////////////////////////////
// Inline members

// public

inline
int ProductOperator::num_Op() const
{
	return Op_.size();
}

inline
Teuchos::RefCountPtr<Epetra_Operator>
ProductOperator::Op(int k) const
{
	validateIndex(k);
	return Op_[k];
}

inline
Teuchos::ETransp
ProductOperator::Op_trans(int k) const
{
	validateIndex(k);
	return Op_trans_[k];
}

inline
ProductOperator::EApplyMode
ProductOperator::Op_inverse(int k) const
{
	validateIndex(k);
	return Op_inverse_[k];
}


// private

inline
void ProductOperator::assertInitialized() const
{
	TEST_FOR_EXCEPTION(
		Op_.size()==0, std::logic_error
		,"Epetra::ProductOperator: Error, Client has not called initialize(...) yet!"
		);
}

inline
void ProductOperator::validateIndex(int k) const
{
	TEST_FOR_EXCEPTION(
		k < 0 || Op_.size()-1 < k, std::logic_error
		,"Epetra::ProductOperator: Error, k = "<<k<< " is not in the range [0,"<<Op_.size()-1<<"]!"
		);
}

} // namespace Epetra

#endif // EPETRA_PRODUCT_OPERATOR_HPP
