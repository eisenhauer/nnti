// @HEADER
// ***********************************************************************
// 
//    Thyra: Interfaces and Support for Abstract Numerical Algorithms
//                 Copyright (2004) Sandia Corporation
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
// @HEADER

#ifndef THYRA_MULTI_VECTOR_BASE_DECL_HPP
#define THYRA_MULTI_VECTOR_BASE_DECL_HPP

#include "Thyra_LinearOpBaseDecl.hpp"
#include "RTOpPack_RTOpT.hpp"

namespace Thyra {

/** \brief Interface for a collection of column vectors called a multi-vector.
 * 
 * \section Thyra_MVB_outline_sec Outline
 * 
 * <ul>
 * <li>\ref Thyra_MVB_intro_sec
 * <li>\ref Thyra_MVB_col_access_sec
 * <li>\ref Thyra_MVB_subviews_sec
 * <li>\ref Thyra_MVB_as_LO_sec
 *   <ul>
 *   <li>\ref Thyra_MVB_block_update_sec
 *   <li>\ref Thyra_MVB_block_inner_prod_sec
 *   </ul>
 * <li>\ref Thyra_MVB_RTOp_sec
 * <li>\ref Thyra_MVB_rtop_collection_sec
 * <li>\ref Thyra_MVB_expl_access_sec
 * <li>\ref Thyra_MVB_expl_access_utils_sec
 * <li>\ref Thyra_MVB_dev_notes_sec
 * </ul>
 * 
 * \section Thyra_MVB_intro_sec Introduction
 * 
 * The primary purpose for this interface is to allow for the convenient
 * aggregation of column vectors as a single matrix-like object.  Such an
 * orderly arrangement of column vectors into a single aggregate object allows
 * for better optimized linear algebra operations such as matrix-matrix
 * multiplication and the solution of linear systems for multiple right-hand
 * sides.  Every computing environment (serial, parallel, out-of-core etc.) 
 * should be able to define at least one reasonably efficient implementation
 * of this interface.
 * 
 * \section Thyra_MVB_col_access_sec Accessing the individual columns as vector views
 * 
 * The individual columns of a multi-vector can be access using the non-const
 * and const versions of the <tt>col()</tt> function.  For example, the
 * individual columns of one multi-vector can be copied to another as follows

 \code

  template<class Scalar>
  void copyOneColumnAtATime(
    const Thyra::MultiVectorBase<Scalar>   &X
    ,Thyra::MultiVectorBase<Scalar>        *Y
    )
  {
    for( int j = 1; j <= X.domain()->dim(); ++j )
      assign( &*Y->col(j), *X.col(j) );
  } 

 \endcode

 * In the above code fragment, the expression <tt>X.col(j)</tt> returns a
 * smart-pointer to a non-changeable column of <tt>X</tt> while the expression
 * <tt>Y->col(j)</tt> returns a smart-pointer to a changeable column of
 * <tt>Y</tt> which is being modified.
 *
 * <b>Note:</b> A modification to <tt>Y</tt> is not guaranteed to be committed
 * back to <tt>Y</tt> until the smart pointer returned from <tt>Y.col(j)</tt>
 * is deleted.
 * 
 * \section Thyra_MVB_subviews_sec Accessing collections of columns as multi-vector views
 * 
 * Another important aspect of this interface is the ability to allow clients
 * to access non-changeable and changeable <tt>MultiVectorBase</tt> views of
 * columns of a parent <tt>MultiVectorBase</tt> object.  These sub-views are
 * created with one of the overloaded <tt>subView()</tt> functions of which
 * there are two general forms.
 * 
 * The first form provides views of contiguous columns through the functions
 * <tt>subView(const Range1D&)</tt> and <tt>subView(const Range1D&)const</tt>.
 * For example, the following function shows how to copy the first three columns
 * of one multi-vector to the last three columns of another multi-vector.

 \code

 template<class Scalar>
 void copyThreeColumns(
   const Thyra::MultiVectorBase<Scalar> &X
   ,Thyra::MultiVectorBase<Scalar>      *Y
   )
 {
   const int m = Y->domain()->dim();
   assign( &*Y->subView(Range1D(m-2,m)), *X.subView(Range1D(1,3)) );
 }

 \endcode
 
 * <b>Note:</b> In the above example <tt>*Y</tt> can be the same multi-vector
 * as <tt>X</tt>.
 *
 * <!-- Warning! Do not reformat the below paragraph or the \ref links will break! --> 
 * The second form provides views of non-contiguous columns through
 * the functions
 * <tt>\ref Thyra_MVB_subView_noncontiguous_nonconst "subView(const int numCols, const int cols[])"</tt>
 * and
 * <tt>\ref Thyra_MVB_subView_noncontiguous_const "subView(const int numCols, const int cols[]) const"</tt>.
 * For example, the following function copies columns 1,
 * 3, and 5 from one multi-vector to columns 2, 4, and 6 of another
 * multi-vector.

 \code

 template<class Scalar>
 void copyThreeStaggeredColumns(
   const Thyra::MultiVectorBase<Scalar> &X
   ,Thyra::MultiVectorBase<Scalar>      *Y
   )
 {
   using Teuchos::arrayArg;
   assign( &*Y->subView(3,arrayArg<int>(2,4,6)()), ,*X.subView(3,arrayArg<int>(1,3,5)()) );
 }

 \endcode

 * <b>Note:</b> In the above example <tt>*Y</tt> can be the same multi-vector
 * as <tt>X</tt>.
 * 
 * In general, the first contiguous form of views will be more efficient that
 * the second non-contiguous form.  Therefore, user's should try to structure
 * their ANAs to use the contiguous form of multi-vector views if possible and
 * only result to the non-contiguous form of views when absolutely needed.
 * 
 * \section Thyra_MVB_as_LO_sec MultiVectorBase as a linear operator
 * 
 * The <tt>%MultiVectorBase</tt> interface is derived from the
 * <tt>LinearOpBase</tt> interface and therefore every
 * <tt>%MultiVectorBase</tt> object can be used as a linear operator which has
 * some interesting implications.  Since a linear operator can apply itself to
 * vectors ans multi-vectors and a multi-vector is a linear operator, this
 * means that a multi-vector can apply itself to other vectors and
 * multi-vectors.  There are several different use cases that this
 * functionality provides functionality for.  Two of the more important use
 * cases are block updates and block inner products.
 *
 * \subsection Thyra_MVB_block_update_sec Multi-vector block updates
 *
 * Let <tt>V</tt> and <tt>Y</tt> be multi-vector objects with the same vector
 * space with a very large number of rows <tt>m</tt> and a moderate number of
 * columns <tt>n</tt>.  Now, consider the block update of the form
 
 \verbatim

   Y = Y + V * B
 \endverbatim

 * where the multi-vector <tt>B</tt> is of dimension <tt>n x b</tt>.
 *
 * The following function shows how this block update might be performed.

 \code

 template<class Scalar>
 void myBlockUpdate(
   const Thyra::MultiVectorBase<Scalar> &V
   ,const int                           b
   ,Thyra::MultiVectorBase<Scalar>      *Y
   )
 {
   typedef Teuchos::ScalarTraits<Scalar> ST;
   // Create the multi-vector B used for the update
   Teuchos::RefCountPtr<Thyra::MultiVectorBase<Scalar> >
     B = createMembers(V.domain(),b);
   // Fill up B for the update
   ...
   // Do the update Y = V*B + Y
   V.apply(Thyra::NONCONJ_ELE,*B,Y,ST::one(),ST::one());
 }

 \endcode

 * In a block update, as demonstrated above, level-3 BLAS can be used to
 * provide a very high level of performance.  Note that in an SPMD program,
 * that <tt>B</tt> would be a locally replicated multi-vector and <tt>V</tt>
 * and <tt>Y</tt> would be distributed-memory multi-vectors.  In an SPMD
 * environment, there would be no global communication in a block update.
 *
 * \subsection Thyra_MVB_block_inner_prod_sec Multi-vector block inner products
 *
 * An important operation supported by the
 * <tt>LinearOpBase::applyTranspose()</tt> function is the block inner product
 * which takes the form
 
 \verbatim

   B = adjoint(V)*X
 \endverbatim

 * where <tt>V</tt> and <tt>X</tt> are tall, thin multi-vectors and <tt>B</tt>
 * is a small multi-vector.  In an SPMD environment, <tt>V</tt> and <tt>X</tt>
 * would be distributed-memory objects while <tt>B</tt> would be locally
 * replicated on each processor.  The following function shows how block inner
 * product would be performed:

 \code

 template<class Scalar>
 Teuchos::RefCountPtr<Thyra::MultiVectorBase<Scalar> >
 void myBlockInnerProd(
   const Thyra::MultiVectorBase<Scalar>                    &V
   ,const Thyra::MultiVectorBase<Scalar>                   &X
   )
 {
   // Create the multi-vector B used for the result
   Teuchos::RefCountPtr<Thyra::MultiVectorBase<Scalar> >
     B = createMembers(V.domain(),X.domain()->dim());
   // Do the inner product B = adjoint(V)*X
   V.applyTranspose(Thyra::CONJ_ELE,X,&*B);
   // Return the result
   return B;
 }

 \endcode

 * In an SPMD program, the above block inner product will use level-3 BLAS to
 * multiply the local elements of <tt>V</tt> and <tt>X</tt> and will then do a
 * single global reduction to assemble the product <tt>B</tt> on all of the
 * processors.
 *
 * \section Thyra_MVB_RTOp_sec Support for reduction/transformation operations
 * 
 * Another powerful feature of this interface is the ability to apply
 * reduction/transformation operators over a sub-set of rows and columns in a
 * set of multi-vector objects.  The behavior is identical to the client
 * extracting each column in a set of multi-vectors and calling
 * <tt>VectorBase::applyOp()</tt> individually on these columns.  However, the
 * advantage of using the multi-vector apply functions is that there may be
 * greater opportunities for increased performance in a number of respects.
 * Also, the intermediate reduction objects over a set of columns can be
 * reduced by a secondary reduction object.
 * 
 * \section Thyra_MVB_rtop_collection_sec Collection of pre-written RTOps and wrapper functions
 *
 * There already exists RTOp-based implementations of several standard vector
 * operations and some convenience functions that wrap these operators and
 * call <tt>applyOp()</tt>.  These wrapper functions can be found \ref
 * Thyra_Op_Vec_MultiVectorStdOps_grp "here".
 * 
 * \section Thyra_MVB_expl_access_sec Explicit multi-vector coefficient access
 * 
 * This interface also allows a client to extract a sub-set of elements in an
 * explicit form as non-changeable <tt>RTOpPack::SubMultiVectorT</tt> objects or
 * changeable <tt>RTOpPack::MutableSubMultiVectorT</tt> objects using the
 * <tt>getSubMultiVector()</tt> functions.  In general, this is a very bad
 * thing to do and should be avoided.  However, there are some situations
 * where this is needed, just as is the case for vectors (see \ref
 * Thyra_VB_expl_access_sec).  The default implementation of these explicit
 * access functions use sophisticated reduction/transformation operators with
 * the <tt>applyOp()</tt> function in order to extract and set the needed
 * elements.  Therefore, all <tt>%MultiVectorBase</tt> subclasses
 * automatically support these operations (even if it is a bad idea to use
 * them).
 * 
 * \section Thyra_MVB_expl_access_utils_sec Explicit multi-vector coefficient access utilities
 * 
 * Client code in general should not directly call the above described
 * explicit sub-multi-vector access functions but should instead use the
 * utility classes <tt>ExplicitMultiVectorView</tt> and
 * <tt>ExplicitMutableMultiVectorView</tt> since these are easier to use and
 * safer in the event that an exception is thrown.
 * 
 * \section Thyra_MVB_dev_notes_sec Notes for subclass developers
 * 
 * This is a fairly bare-bones interface class without much in the way of
 * default function implementations.  The subclass
 * <tt>MultiVectorDefaultBase</tt> uses a default multi-vector implementation
 * to provide overrides of many of the functions and should the the first
 * choice for subclasses implementations.
 * 
 * Note that through the magic of the <tt>applyOp()</tt> functions that this
 * interface is able to implement the pure virtual <tt>apply()</tt> function
 * from the <tt>LinearOpBase</tt> interface.  This implementation is not
 * optimal, but will be sufficient in many different contexts.
 * 
 * The <tt>applyOp()</tt> functions should only be overridden if the subclass
 * can do something more efficient than simply applying the
 * reduction/transformation operators one column at a time.
 * 
 * Functions that subclasses should almost never need (or want) to override
 * are the const version of <tt>col()</tt> or the the const versions of
 * <tt>subView()</tt>.
 * 
 * \ingroup Thyra_Op_Vec_fundamental_interfaces_code_grp
 */
template<class Scalar>
class MultiVectorBase : virtual public LinearOpBase<Scalar>
{
public:

  /** \brief . */
  using LinearOpBase<Scalar>::describe;
  
  /** @name Provide access to the columns as VectorBase objects */
  //@{

  /** \brief Return a non-changeable view of a constituent column vector.
   *
   * \param  j  [in] One-based index of the column to return a view for
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->domain().get()!=NULL && this->range().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> <tt>1 <= j && j <= this->domain()->dim()</tt> (throw <tt>std::invalid_argument</tt>)
   * </ul>
   *
   * <b>Postconditions:</b><ul>
   * <li> <tt>return.get() != NULL</tt>
   * <li> <tt>this->range()->isCompatible(*return->space()) == true</tt>
   * </ul>
   *
   * <b>Note:</b> This view is to be used immediately and then released.
   *
   * The default implementation of this function (which is the only
   * implementation needed by most subclasses) is based on the
   * non-const version <tt>col()</tt>.
   */
  virtual Teuchos::RefCountPtr<const VectorBase<Scalar> > col(Index j) const;

  /** \brief Return a changeable view of a constituent column vector.
   *
   * \param  j  [in] One-based index of the column to return a view for
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->domain().get()!=NULL && this->range().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> <tt>1 <= j && j <= this->domain()->dim()</tt> (throw <tt>std::invalid_argument</tt>)
   * </ul>
   *
   * <b>Postconditions:</b><ul>
   * <li> <tt>return.get() != NULL</tt>
   * <li> <tt>this->range()->isCompatible(*return->space()) == true</tt>
   * </ul>
   *
   * <b>Note:</b> This view is to be used immediately and then released.
   *
   * <b>Note:</b> <tt>*this</tt> is not guaranteed to be modified until the
   * smart pointer returned by this function is destroyed.
   */
  virtual Teuchos::RefCountPtr<VectorBase<Scalar> > col(Index j) = 0;

  //@}

  /** @name Multi-vector sub-views */
  //@{

  /** \brief Return a non-changeable sub-view of a contiguous set of columns of the this multi-vector.
   *
   * \anchor Thyra_MVB_subView_contiguous_const
   *
   * @param  colRng  [in] One-based range of columns to create a view of.  Note that it is valid for
   *                  <tt>colRng.full_range()==true</tt> in which case the view of the entire
   *                  multi-vector is taken.
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->domain().get()!=NULL && this->range().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> [<tt>!colRng.full_range()</tt>] <tt>colRng.ubound() <= this->domain()->dim()</tt> (throw <tt>std::invalid_argument</tt>)
   * </ul>
   *
   * <b>Postconditions:</b><ul>
   * <li> <tt>this->range()->isCompatible(*return->range()) == true</tt>
   * <li> <tt>return->domain()->dim() == RangePack::full_range(colRng,1,this->domain()->dim()).size()</tt>
   * <li> <tt>*return->col(1+k)</tt> represents the same column vector as <tt>this->col(colRng.lbound()+k)</tt>,
   *      for <tt>k=0...RangePack::full_range(colRng,1,this->domain()->dim()).ubound()-1</tt>
   * </ul>
   *
   * <b>Note:</b> This view is to be used immediately and then released.
   */
  virtual Teuchos::RefCountPtr<const MultiVectorBase<Scalar> > subView( const Range1D& colRng ) const = 0;
  
  /** \brief Return a changeable sub-view of a contiguous set of columns of
   * the this multi-vector.
   *
   * \anchor Thyra_MVB_subView_contiguous_nonconst
   *
   * @param  colRng  [in] One-based range of columns to create a view of.  Note that it is valid for
   *                  <tt>colRng.full_range()==true</tt> in which case the view of the entire
   *                  multi-vector is taken.
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->domain().get()!=NULL && this->range().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> [<tt>!colRng.full_range()</tt>] <tt>colRng.ubound() <= this->domain()->dim()</tt> (throw <tt>std::invalid_argument</tt>)
   * </ul>
   *
   * <b>Postconditions:</b><ul>
   * <li> <tt>this->range()->isCompatible(*return->range()) == true</tt>
   * <li> <tt>return->domain()->dim() == RangePack::full_range(colRng,1,this->domain()->dim()).size()</tt>
   * <li> <tt>*return->col(1+k)</tt> represents the same column vector as <tt>this->col(colRng.lbound()+k)</tt>,
   *      for <tt>k=0...RangePack::full_range(colRng,1,this->domain()->dim()).ubound()-1</tt>
   * </ul>
   *
   * <b>Note:</b> This view is to be used immediately and then released.
   *
   * <b>Note:</b> <tt>*this</tt> is not guaranteed to be modified until the
   * smart pointer returned by this function goes out of scope.
   */
  virtual Teuchos::RefCountPtr<MultiVectorBase<Scalar> > subView( const Range1D& colRng ) = 0;

  /** \brief Return a non-changeable sub-view of a non-contiguous set of columns of this multi-vector.
   *
   * \anchor Thyra_MVB_subView_noncontiguous_const
   *
   * @param  numCols  [in] The number of columns to extract a view for.
   * @param  cols     [in] Array (length <tt>numCols</tt>) of the 1-based column indexes to use in the
   *                  returned view.
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->domain().get()!=NULL && this->range().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> <tt>numCols <= this->domain()->dim()</tt> (throw <tt>std::invalid_argument</tt>)
   * <li> <tt>1 <= cols[k] <= this->domain()->dim()</tt>, for <tt>k=0...numCols-1</tt> (throw <tt>std::invalid_argument</tt>)
   * <li> <tt>col[k1] != col[k2]</tt>, for all <tt>k1 != k2</tt> in the range <tt>[0,numCols-1]</tt>
   * </ul>
   *
   * <b>Postconditions:</b><ul>
   * <li> <tt>this->range()->isCompatible(*return->range()) == true</tt>
   * <li> <tt>return->domain()->dim() == numCols</tt>
   * <li> <tt>*return->col(k+1)</tt> represents the same column vector as <tt>this->col(cols[k])</tt>,
   *      for <tt>k=0...numCols-1</tt>
   * </ul>
   *
   * <b>Note:</b> This view is to be used immediately and then released.
   */
  virtual Teuchos::RefCountPtr<const MultiVectorBase<Scalar> > subView( const int numCols, const int cols[] ) const = 0;

  /** \brief Return a changeable sub-view of a non-contiguous set of columns of this multi-vector.
   *
   * \anchor Thyra_MVB_subView_noncontiguous_nonconst
   *
   * @param  numCols  [in] The number of columns to extract a view for.
   * @param  cols     [in] Array (length <tt>numCols</tt>) of the 1-based column indexes to use in the
   *                  returned view.
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->domain().get()!=NULL && this->range().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> <tt>1 <= numCols <= this->domain()->dim()</tt> (throw <tt>std::invalid_argument</tt>)
   * <li> <tt>1 <= cols[k] <= this->domain()->dim()</tt>, for <tt>k=0...numCols-1</tt> (throw <tt>std::invalid_argument</tt>)
   * <li> <tt>col[k1] != col[k2]</tt>, for all <tt>k1 != k2</tt> in the range <tt>[0,numCols-1]</tt>
   * </ul>
   *
   * <b>Postconditions:</b><ul>
   * <li> <tt>this->range()->isCompatible(*return->range()) == true</tt>
   * <li> <tt>return->domain()->dim() == numCols</tt>
   * <li> <tt>*return->col(k+1)</tt> represents the same column vector as <tt>this->col(cols[k])</tt>,
   *      for <tt>k=0...numCols-1</tt>
   * </ul>
   *
   * <b>Note:</b> This view is to be used immediately and then released.
   *
   * <b>Note:</b> <tt>*this</tt> is not guaranteed to be modified until the
   * smart pointer returned by this function goes out of scope.
   */
  virtual Teuchos::RefCountPtr<MultiVectorBase<Scalar> > subView( const int numCols, const int cols[] ) = 0;
  
  //@}

  /** @name Collective reduction/transformation operator apply functions */
  //@{

  /** \brief Apply a reduction/transformation operator column by column and
   * return an array of the reduction objects.
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->domain().get()!=NULL && this->range().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> See the preconditions for <tt>Thyra::applyOp()</tt>
   * </ul>
   *
   * See the documentation for the function <tt>Thyra::applyOp()</tt>
   * for a description of the arguments.
   *
   * This function is not to be called directly by the client but instead
   * through the nonmember function <tt>Thyra::applyOp()</tt>.
   *
   * It is expected that <tt>this</tt> will be one of the multi-vector
   * objects in <tt>multi_vecs[]</tt> or <tt>targ_multi_vecs[]</tt>.
   *
   * The default implementation calls <tt>VectorBase::applyOp()</tt> on
   * each column <tt>this->col(j)</tt> for <tt>j = 1
   * ... this->range()->dim()</tt>.
   */
  virtual void applyOp(
    const RTOpPack::RTOpT<Scalar>   &primary_op
    ,const int                      num_multi_vecs
    ,const MultiVectorBase<Scalar>* multi_vecs[]
    ,const int                      num_targ_multi_vecs
    ,MultiVectorBase<Scalar>*       targ_multi_vecs[]
    ,RTOpPack::ReductTarget*        reduct_objs[]
    ,const Index                    primary_first_ele
    ,const Index                    primary_sub_dim
    ,const Index                    primary_global_offset
    ,const Index                    secondary_first_ele
    ,const Index                    secondary_sub_dim
    ) const;

  /** \brief Apply a reduction/transformation operator column by column and
   * reduce the intermediate reduction objects into a single reduction object.
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->domain().get()!=NULL && this->range().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> See the preconditions for <tt>Thyra::applyOp()</tt>
   * </ul>
   *
   * See the documentation for the function <tt>Thyra::applyOp()</tt>
   * for a description of the arguments.
   *
   * This function is not to be called directly by the client but instead
   * through the nonmember function <tt>Thyra::applyOp()</tt>.
   *
   * It is expected that <tt>this</tt> will be one of the multi-vector
   * objects in <tt>multi_vecs[]</tt> or <tt>targ_multi_vecs[]</tt>.
   *
   * The default implementation calls <tt>applyOp()</tt> where an
   * array of reduction objects is taken.
   */
  virtual void applyOp(
    const RTOpPack::RTOpT<Scalar>   &primary_op
    ,const RTOpPack::RTOpT<Scalar>  &secondary_op
    ,const int                      num_multi_vecs
    ,const MultiVectorBase<Scalar>* multi_vecs[]
    ,const int                      num_targ_multi_vecs
    ,MultiVectorBase<Scalar>*       targ_multi_vecs[]
    ,RTOpPack::ReductTarget         *reduct_obj
    ,const Index                    primary_first_ele
    ,const Index                    primary_sub_dim
    ,const Index                    primary_global_offset
    ,const Index                    secondary_first_ele
    ,const Index                    secondary_sub_dim
    ) const;

  //@}

  /** @name Explicit sub-multi-vector access */
  //@{

  /** \brief Get a non-changeable explicit view of a sub-multi-vector.
   *
   * @param  rowRng   [in] The range of the rows to extract the sub-multi-vector view.
   * @param  colRng   [in] The range of the columns to extract the sub-multi-vector view.
   * @param  sub_mv   [in/out] View of the sub-multi_vector.  Prior to the
   *                  first call to this function, <tt>sub_mv->set_uninitialized()</tt> must be called.
   *                  Technically <tt>*sub_mv</tt> owns the memory but this memory can be freed
   *                  only by calling <tt>this->freeSubMultiVector(sub_mv)</tt>.
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->range().get()!=NULL && this->domain().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> [<tt>!rowRng.full_range()</tt>] <tt>rowRng.ubound() <= this->range()->dim()</tt>
   *      (<tt>throw std::out_of_range</tt>)
   * <li> [<tt>!colRng.full_range()</tt>] <tt>colRng.ubound() <= this->domain()->dim()</tt>
   *      (<tt>throw std::out_of_range</tt>)
   * </ul>
   *
   * <b>Postconditions:</b><ul>
   * <li> <tt>*sub_mv</tt> contains an explicit non-changeable view to the elements
   *      in the row and column ranges <tt>RangePack::full_range(rowRng,1,this->range()->dim())</tt>
   *      and <tt>RangePack::full_range(colRng,1,this->domain()->dim())</tt> respectively.
   * </ul>
   *
   * <b>Note:</b> This view is to be used immediately and then released with a
   * call to <tt>freeSubMultiVector()</tt>.
   *
   * Note that calling this operation might require some dynamic memory
   * allocations and temporary memory.  Therefore, it is critical that
   * <tt>this->freeSubMultiVector(sub_mv)</tt> be called by client in order to
   * clean up memory and avoid memory leaks after the sub-multi-vector view is
   * finished being used.
   *
   * <b>Heads Up!</b> Note that client code in general should not directly
   * call this function but should instead use the utility class
   * <tt>ExplicitMultiVectorView</tt> which will also take care of calling
   * <tt>freeSubMultiVector()</tt>.
   *
   * If <tt>this->getSubMultiVector(...,sub_mv)</tt> was previously
   * called on <tt>sub_mv</tt> then it may be possible to reuse this
   * memory if it is sufficiently sized.  The user is encouraged to
   * make multiple calls to
   * <tt>this->getSubMultiVector(...,sub_mv)</tt> before
   * <tt>this->freeSubMultiVector(sub_mv)</tt> to finally clean up all of
   * the memory.  Of course, the same <tt>sub_mv</tt> object must be
   * passed to the same multi-vector object for this to work correctly.
   *
   * This function has a default implementation based on the vector operation
   * <tt>VectorBase::getSubVector()</tt> called on the non-changeable vector
   * objects returned from <tt>col()</tt>.  Note that the footprint of the
   * reduction object (both internal and external state) will be
   * O(<tt>rowRng.size()*colRng.size()</tt>).  For serial applications this is
   * fairly reasonable and will not be a major performance penalty.  For
   * parallel applications, however, this is a terrible implementation and
   * must be overridden if <tt>rowRng.size()</tt> is large at all.  Although,
   * this function should not even be used in cases where the multi-vector is
   * very large.  If a subclass does override this function, it must also
   * override <tt>freeSubMultiVector()</tt> which has a default implementation
   * which is a companion to this function's default implementation.
   */
  virtual void getSubMultiVector(
    const Range1D                       &rowRng
    ,const Range1D                      &colRng
    ,RTOpPack::SubMultiVectorT<Scalar>  *sub_mv
    ) const;

  /** \brief Free a non-changeable explicit view of a sub-multi-vector.
   *
   * @param  sub_mv
   *				[in/out] The memory referred to by <tt>sub_mv->values()</tt>
   *				will be released if it was allocated and <tt>*sub_mv</tt>
   *              will be zeroed out using <tt>sub_mv->set_uninitialized()</tt>.
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->range().get()!=NULL && this->domain().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> <tt>sub_mv</tt> must have been passed through a call to 
   *      <tt>this->getSubMultiVector(...,sub_mv)</tt>
   * </ul>
    *
   * <b>Postconditions:</b><ul>
   * <li> See <tt>RTOpPack::SubMultiVectorT::set_uninitialized()</tt> for <tt>sub_mv</tt>
   * </ul>
   *
   * The sub-multi-vector view must have been allocated by
   * <tt>this->getSubMultiVector()</tt> first.
   *
   * This function has a default implementation which is a companion
   * to the default implementation for <tt>getSubMultiVector()</tt>.  If
   * <tt>getSubMultiVector()</tt> is overridden by a subclass then this
   * function must be overridden also!
   */
  virtual void freeSubMultiVector( RTOpPack::SubMultiVectorT<Scalar>* sub_mv ) const;

  /** \brief Get a changeable explicit view of a sub-multi-vector.
   *
   * @param  rowRng   [in] The range of the rows to extract the sub-multi-vector view.
   * @param  colRng   [in] The range of the columns to extract the sub-multi-vector view.
   * @param  sub_mv   [in/out] Changeable view of the sub-multi-vector.  Prior to the
   *                  first call <tt>sub_mv->set_uninitialized()</tt> must
   *                  have been called for the correct behavior.  Technically
   *                  <tt>*sub_mv</tt> owns the memory but this memory must be committed
   *                  and freed only by calling <tt>this->commitSubMultiVector(sub_mv)</tt>.
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->range().get()!=NULL && this->domain().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> [<tt>!rowRng.full_range()</tt>] <tt>rowRng.ubound() <= this->range()->dim()</tt>
   *      (<tt>throw std::out_of_range</tt>)
   * <li> [<tt>!colRng.full_range()</tt>] <tt>colRng.ubound() <= this->domain()->dim()</tt>
   *      (<tt>throw std::out_of_range</tt>)
   * </ul>
    *
   * <b>Postconditions:</b><ul>
   * <li> <tt>*sub_mv</tt> contains an explicit changeable view to the elements
   *      in the row and column ranges <tt>full_range(rowRng,1,this->range()->dim())</tt>
   *      and <tt>full_range(colRng,1,this->domain()->dim())</tt> respectively.
   * </ul>
   *
   *
   * <b>Note:</b> This view is to be used immediately and then committed back
   * with a call to <tt>commitSubMultiVector()</tt>.
   *
   * Note that calling this operation might require some internal allocations
   * and temporary memory.  Therefore, it is critical that
   * <tt>this->commitSubMultiVector(sub_mv)</tt> is called to commit the
   * changed entries and clean up memory and avoid memory leaks after the
   * sub-multi-vector is modified.
   *
   * <b>Heads Up!</b> Note that client code in general should not directly
   * call this function but should instead use the utility class
   * <tt>ExplicitMutableMultiVectorView</tt> which will also take care of
   * calling <tt>commitSubMultiVector</tt>.
   *
   * If <tt>this->getSubMultiVector(...,sub_mv)</tt> was previously
   * called on <tt>sub_mv</tt> then it may be possible to reuse this
   * memory if it is sufficiently sized.  The user is encouraged to
   * make multiple calls to
   * <tt>this->getSubMultiVector(...,sub_mv)</tt> before
   * <tt>this->commitSubMultiVector(sub_mv)</tt> to finally clean up
   * all of the memory.  Of course the same <tt>sub_mv</tt> object
   * must be passed to the same multi-vector object for this to work
   * correctly.
   *
   * Changes to the underlying sub-multi-vector are not guaranteed to
   * become permanent until <tt>this->getSubMultiVector(...,sub_mv)</tt>
   * is called again, or <tt>this->commitSubMultiVector(sub_mv)</tt> is
   * called.
   *
   * This function has a default implementation based on the vector
   * operation <tt>VectorBase::getSubVector()</tt> called on the changeable
   * vector objects returned from <tt>col()</tt>.  Note that the
   * footprint of the reduction object (both internal and external
   * state) will be O(<tt>rowRng.size()*colRng.size()</tt>).  For
   * serial applications this is fairly reasonable and will not be a
   * major performance penalty.  For parallel applications, however,
   * this is a terrible implementation and must be overridden if
   * <tt>rowRng.size()</tt> is large at all.  Although, this function
   * should not even be used in case where the multi-vector is very
   * large.  If a subclass does override this function, it must also
   * override <tt>commitSubMultiVector()</tt> which has a default
   * implementation which is a companion to this function's default
   * implementation.
   */
  virtual void getSubMultiVector(
    const Range1D                                &rowRng
    ,const Range1D                               &colRng
    ,RTOpPack::MutableSubMultiVectorT<Scalar>    *sub_mv
    );

  /** \brief Commit changes for a changeable explicit view of a sub-multi-vector.
   *
   * @param sub_mv
   *				[in/out] The data in <tt>sub_mv->values()</tt> will be written
   *              back to internal storage and the memory referred to by
   *              <tt>sub_mv->values()</tt> will be released if it was allocated
   *				and <tt>*sub_mv</tt> will be zeroed out using
   *				<tt>sub_mv->set_uninitialized()</tt>.
   *
   * <b>Preconditions:</b><ul>
   * <li> <tt>this->range().get()!=NULL && this->domain().get()!=NULL</tt> (throw <tt>std::logic_error</tt>)
   * <li> <tt>sub_mv</tt> must have been passed through a call to 
   *      <tt>this->getSubMultiVector(...,sub_mv)</tt>
   * </ul>
   *
   * <b>Postconditions:</b><ul>
   * <li> See <tt>RTOpPack::MutableSubMultiVectorT::set_uninitialized()</tt> for <tt>sub_mv</tt>
   * <li> <tt>*this</tt> will be updated according the the changes made to <tt>sub_mv</tt>
   * </ul>
   *
   * The sub-multi-vector view must have been allocated by
   * <tt>this->getSubMultiVector()</tt> first.
   *
   * This function has a default implementation which is a companion
   * to the default implementation for <tt>getSubMultiVector()</tt>.  If
   * <tt>getSubMultiVector()</tt> is overridden by a subclass then this
   * function must be overridden also!
   */
  virtual void commitSubMultiVector( RTOpPack::MutableSubMultiVectorT<Scalar>* sub_mv );

  //@}

  /** @name Cloning */
  //@{

  /** \brief Clone the multi-vector object (if supported).
   *
   * The default implementation uses the vector space to create a
   * new multi-vector object and then uses a transformation operator
   * to assign the vector elements.  A subclass should only override
   * this function if it can do something more sophisticated
   * (i.e. lazy evaluation) but in general, this is not needed.
   */
  virtual Teuchos::RefCountPtr<MultiVectorBase<Scalar> > clone_mv() const;

  //@}

  /** @name Overridden functions from LinearOpBase */
  //@{

  /// This function is simply overridden to return <tt>this->clone_mv()</tt>.
  Teuchos::RefCountPtr<const LinearOpBase<Scalar> > clone() const;

  //@}

private:
  
#ifdef DOXYGEN_COMPILE
  VectorBase<Scalar>  *columns; // Doxygen only
#endif	

}; // end class MultiVectorBase<Scalar>

/** \defgroup Thyra_Op_Vec_MultiVectorBase_support_grp Support functions for MultiVectorBase interface

These functions allow a client to use a <tt>MultiVectorBase</tt> object more easily
in simpler use cases.

\ingroup Thyra_Op_Vec_fundamental_interfaces_code_grp

*/
//@{

/** \brief Apply a reduction/transformation operator column by column and
 * return an array of the reduction objects.
 *
 * ToDo: Finish documentation!
 */
template<class Scalar>
inline
void applyOp(
  const RTOpPack::RTOpT<Scalar>   &primary_op
  ,const int                      num_multi_vecs
  ,const MultiVectorBase<Scalar>* multi_vecs[]
  ,const int                      num_targ_multi_vecs
  ,MultiVectorBase<Scalar>*       targ_multi_vecs[]
  ,RTOpPack::ReductTarget*        reduct_objs[]
  ,const Index                    primary_first_ele
#ifndef __sun
                                                         = 1
#endif
  ,const Index                    primary_sub_dim
#ifndef __sun
                                                         = 0
#endif
  ,const Index                    primary_global_offset
#ifndef __sun
                                                         = 0
#endif
  ,const Index                    secondary_first_ele
#ifndef __sun
                                                         = 1
#endif
  ,const Index                    secondary_sub_dim
#ifndef __sun
                                                         = 0
#endif
  )
{
  if(num_multi_vecs)
    multi_vecs[0]->applyOp(
      primary_op
      ,num_multi_vecs,multi_vecs,num_targ_multi_vecs,targ_multi_vecs
      ,reduct_objs,primary_first_ele,primary_sub_dim,primary_global_offset
      ,secondary_first_ele,secondary_sub_dim
      );
  else if(num_targ_multi_vecs)
    targ_multi_vecs[0]->applyOp(
      primary_op
      ,num_multi_vecs,multi_vecs,num_targ_multi_vecs,targ_multi_vecs
      ,reduct_objs,primary_first_ele,primary_sub_dim,primary_global_offset
      ,secondary_first_ele,secondary_sub_dim
      );
}

#ifdef __sun
template<class Scalar>
inline
void applyOp(
  const RTOpPack::RTOpT<Scalar>   &primary_op
  ,const int                      num_multi_vecs
  ,const MultiVectorBase<Scalar>* multi_vecs[]
  ,const int                      num_targ_multi_vecs
  ,MultiVectorBase<Scalar>*       targ_multi_vecs[]
  ,RTOpPack::ReductTarget*        reduct_objs[]
  )
{
  applyOp(
          primary_op
          ,num_multi_vecs,multi_vecs,num_targ_multi_vecs,targ_multi_vecs
          ,reduct_objs,1,0,0,1,0
          );
}
#endif

/** \brief Apply a reduction/transformation operator column by column and reduce the intermediate
 * reduction objects into one reduction object.
 *
 * ToDo: Finish documentation!
 */
template<class Scalar>
inline
void applyOp(
  const RTOpPack::RTOpT<Scalar>   &primary_op
  ,const RTOpPack::RTOpT<Scalar>  &secondary_op
  ,const int                      num_multi_vecs
  ,const MultiVectorBase<Scalar>* multi_vecs[]
  ,const int                      num_targ_multi_vecs
  ,MultiVectorBase<Scalar>*       targ_multi_vecs[]
  ,RTOpPack::ReductTarget         *reduct_obj
  ,const Index                    primary_first_ele
#ifndef __sun
                                                         = 1
#endif
  ,const Index                    primary_sub_dim
#ifndef __sun
                                                         = 0
#endif
  ,const Index                    primary_global_offset
#ifndef __sun
                                                         = 0
#endif
  ,const Index                    secondary_first_ele
#ifndef __sun
                                                         = 1
#endif
  ,const Index                    secondary_sub_dim
#ifndef __sun
                                                         = 0
#endif
  )
{
  if(num_multi_vecs)
    multi_vecs[0]->applyOp(
      primary_op,secondary_op
      ,num_multi_vecs,multi_vecs,num_targ_multi_vecs,targ_multi_vecs
      ,reduct_obj,primary_first_ele,primary_sub_dim,primary_global_offset
      ,secondary_first_ele,secondary_sub_dim
      );
  else if(num_targ_multi_vecs)
    targ_multi_vecs[0]->applyOp(
      primary_op,secondary_op
      ,num_multi_vecs,multi_vecs,num_targ_multi_vecs,targ_multi_vecs
      ,reduct_obj,primary_first_ele,primary_sub_dim,primary_global_offset
      ,secondary_first_ele,secondary_sub_dim
      );
}

#ifdef __sun

template<class Scalar>
inline
void applyOp(
  const RTOpPack::RTOpT<Scalar>   &primary_op
  ,const RTOpPack::RTOpT<Scalar>  &secondary_op
  ,const int                      num_multi_vecs
  ,const MultiVectorBase<Scalar>* multi_vecs[]
  ,const int                      num_targ_multi_vecs
  ,MultiVectorBase<Scalar>*       targ_multi_vecs[]
  ,RTOpPack::ReductTarget         *reduct_obj
  )
{
  applyOp(
      primary_op,secondary_op
      ,num_multi_vecs,multi_vecs,num_targ_multi_vecs,targ_multi_vecs
      ,reduct_obj,1,0,0,1,0
      );
}

#endif // __sun

//@}

} // namespace Thyra

#endif // THYRA_MULTI_VECTOR_BASE_DECL_HPP
