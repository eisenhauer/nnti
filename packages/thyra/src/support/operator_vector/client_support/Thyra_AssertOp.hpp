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

#ifndef THYRA_ASSERT_OP_HPP
#define THYRA_ASSERT_OP_HPP

#include "Thyra_OperatorVectorTypes.hpp"
#include "Thyra_VectorSpaceBase.hpp"
#include "Thyra_VectorBase.hpp"
#include "Thyra_LinearOpBase.hpp"
#include "Teuchos_TestForException.hpp"

namespace Thyra {

/* Utility struct for dumping vector space names, dimension etc.
 */
template<class Scalar>
struct dump_vec_spaces_t {
public:
  dump_vec_spaces_t(
    const Thyra::VectorSpaceBase<Scalar>& _vec_space1, const std::string &_vec_space1_name
    ,const Thyra::VectorSpaceBase<Scalar>& _vec_space2, const std::string &_vec_space2_name
    )
    :vec_space1(_vec_space1),vec_space1_name(_vec_space1_name)
    ,vec_space2(_vec_space2),vec_space2_name(_vec_space2_name)
    {}
  const Thyra::VectorSpaceBase<Scalar> &vec_space1;
  const std::string                    vec_space1_name;
  const Thyra::VectorSpaceBase<Scalar> &vec_space2;
  const std::string                    vec_space2_name;
}; // end dum_vec_spaces

/* Utility function for dumping vector space names, dimension etc.
 */
template<class Scalar>
inline dump_vec_spaces_t<Scalar> dump_vec_spaces(
  const Thyra::VectorSpaceBase<Scalar>& vec_space1, const std::string &vec_space1_name
  ,const Thyra::VectorSpaceBase<Scalar>& vec_space2, const std::string &vec_space2_name
  )
{
  return dump_vec_spaces_t<Scalar>(vec_space1,vec_space1_name,vec_space2,vec_space2_name);
}

// Notice!!!!!!!  Place a breakpoint in following function in order to halt the
// program just before an exception is thrown!

/* Utility ostream operator for dumping vector space names, dimension etc.
 */
template<class Scalar>
std::ostream& operator<<( std::ostream& o, const dump_vec_spaces_t<Scalar>& d )
{
  o << "Error, " << d.vec_space1_name << " at address " << &d.vec_space1
    << " described as " << d.vec_space1.description()
    << " with dimension " << d.vec_space1_name << ".dim() = " << d.vec_space1.dim()
    << " is not compatible with "
    << d.vec_space2_name  << " at address " << &d.vec_space2
    << " described as " << d.vec_space2.description()
    << " with dimension " << d.vec_space2_name << ".dim() = " << d.vec_space2.dim();
  return o;
}

/* Utility enum for selecting domain or range spaces
 */
enum EM_VS { VS_RANGE, VS_DOMAIN };

/** \brief Utility function for selecting domain or range spaces
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_support_code_utils_grp
 */
template<class Scalar>
const Thyra::VectorSpaceBase<Scalar>& linear_op_op(
  const Thyra::LinearOpBase<Scalar>&     M
  ,Thyra::ETransp                        M_trans
  ,EM_VS                                 M_VS
  )
{
  if(real_trans(M_trans) == NOTRANS && M_VS == VS_RANGE)
    return *M.range();
  if((real_trans(M_trans) == TRANS || M_trans == CONJTRANS) && M_VS == VS_RANGE)
    return *M.domain();
  if(real_trans(M_trans) == NOTRANS && M_VS == VS_DOMAIN)
    return *M.domain();
  // (M_trans == TRANS || M_trans == CONJTRANS) && M_VS == VS_DOMAIN
  return *M.range();
}

} // end namespace Thyra

/** \brief This macro just asserts that a LHS argument is set
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_support_code_grp
 */
#define THYRA_ASSERT_LHS_ARG(FUNC_NAME,LHS_ARG) \
  TEST_FOR_EXCEPTION( \
    (LHS_ARG) == NULL, std::invalid_argument \
    ,FUNC_NAME << " : Error!" \
    );

// Notice!!!!!!!  Setting a breakpoint at the line number that is printed by this macro
// and then trying to set the condition !isCompatible does not work (at least not
// in gdb).

/** \brief Helper assertion macro
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_support_code_utils_grp
 */
#define THYRA_ASSERT_VEC_SPACES_NAMES(FUNC_NAME,VS1,VS1_NAME,VS2,VS2_NAME) \
{ \
  const bool isCompatible = (VS1).isCompatible(VS2); \
  TEST_FOR_EXCEPTION( \
    !isCompatible, ::Thyra::Exceptions::IncompatibleVectorSpaces \
    ,FUNC_NAME << " : "	<< ::Thyra::dump_vec_spaces(VS1,VS1_NAME,VS2,VS2_NAME) \
    ) \
}

/** \brief This is a very useful macro that should be used to validate
 * that two vector spaces are compatible.
 *
 * If the vector spaces are not compatible then a very helpful error
 * message is generated (in the std::exception class) along with the
 * file name and line number where this macro is called.  The error
 * message string embedded in the thrown exception gives the concrete
 * types of the two vector spaces involved as well as their dimensions.
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_support_code_grp
 */
#define THYRA_ASSERT_VEC_SPACES(FUNC_NAME,VS1,VS2)\
THYRA_ASSERT_VEC_SPACES_NAMES(FUNC_NAME,VS1,#VS1,VS2,#VS2)

/** \brief This macro validates that a linear operator and a vector space
 * for the domain vector are compatible.
 *
 * This macro is not recommended for casual users.
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_support_code_utils_grp
 */
#define THYRA_ASSERT_MAT_VEC_SPACES(FUNC_NAME,M,M_T,M_VS,VS) \
{ \
  std::ostringstream M_VS_name; \
  M_VS_name << "(" #M << ( (M_T) == Thyra::NOTRANS ? "" : "'" ) << ")" \
            << "." << ( (M_VS) == Thyra::VS_RANGE ? "range()" : "domain()" ); \
  THYRA_ASSERT_VEC_SPACES_NAMES( \
    FUNC_NAME \
    ,::Thyra::linear_op_op(M,M_T,M_VS),M_VS_name.str().c_str() \
    ,(VS),#VS \
    ) \
}

/** \brief This is a very useful macro that should be used to validate
 * that the spaces for the vector version of the
 * <tt>LinearOpBase::apply()</tt> function (or related operations).
 *
 * If the vector spaces are not compatible then a very helpful error
 * message is generated (in the std::exception class) along with the
 * file name and line number where this macro is called.  The error
 * message string embedded in the thrown exception gives the concrete
 * types of the vector spaces involved as well as their dimensions.
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_support_code_grp
 */
#define THYRA_ASSERT_LINEAR_OP_VEC_APPLY_SPACES(FUNC_NAME,M,M_T,X,Y) \
  THYRA_ASSERT_LHS_ARG(FUNC_NAME,Y); \
  THYRA_ASSERT_MAT_VEC_SPACES(FUNC_NAME,M,M_T,::Thyra::VS_RANGE,*(Y)->space()); \
  THYRA_ASSERT_MAT_VEC_SPACES(FUNC_NAME,M,M_T,::Thyra::VS_DOMAIN,*(X).space());

/** \brief This is a very useful macro that should be used to validate
 * that the spaces for the multi-vector version of the
 * <tt>LinearOpBase::apply()</tt> function (or related operations).
 *
 * If the vector spaces are not compatible then a very helpful error
 * message is generated (in the std::exception class) along with the
 * file name and line number where this macro is called.  The error
 * message string embedded in the thrown exception gives the concrete
 * types of the vector spaces involved as well as their dimensions.
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_support_code_grp
 */
#define THYRA_ASSERT_LINEAR_OP_MULTIVEC_APPLY_SPACES(FUNC_NAME,M,M_T,X,Y) \
  THYRA_ASSERT_LHS_ARG(FUNC_NAME,Y); \
  THYRA_ASSERT_VEC_SPACES(FUNC_NAME,*(X).domain(),*(Y)->domain()); \
  THYRA_ASSERT_MAT_VEC_SPACES(FUNC_NAME,M,M_T,::Thyra::VS_RANGE,*(Y)->range()); \
  THYRA_ASSERT_MAT_VEC_SPACES(FUNC_NAME,M,M_T,::Thyra::VS_DOMAIN,*(X).range());

/** \brief Helper assertion macro
 *
 * This macro is not recommended for casual users.
 *
 * \ingroup Thyra_Op_Vec_ANA_Development_support_code_utils_grp
 */
#define THYRA_ASSERT_MAT_MAT_SPACES(FUNC_NAME,M1,M1_T,M1_VS,M2,M2_T,M2_VS) \
{ \
  std::ostringstream M1_VS_name, M2_VS_name; \
  M1_VS_name << "(" #M1 << ( M1_T == ::Thyra::NOTRANS ? "" : "'" ) << ")" \
         << "." << ( M1_VS == ::Thyra::VS_RANGE ? "range()" : "domain()" ); \
  M2_VS_name << "(" #M2 << ( M2_T == ::Thyra::NOTRANS ? "" : "'" ) << ")" \
         << "." << ( M2_VS == ::Thyra::VS_RANGE ? "range()" : "domain()" ); \
  THYRA_ASSERT_VEC_SPACES_NAMES( \
    FUNC_NAME \
    ,::Thyra::linear_op_op(M1,M1_T,M1_VS),M1_VS_name.str().c_str() \
    ,::Thyra::linear_op_op(M2,M2_T,M2_VS),M2_VS_name.str().c_str() \
    ); \
}

#endif // THYRA_ASSERT_OP_HPP
