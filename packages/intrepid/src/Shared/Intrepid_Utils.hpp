// @HEADER
// ************************************************************************
//
//                           Intrepid Package
//                 Copyright (2007) Sandia Corporation
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
// Questions? Contact Pavel Bochev (pbboche@sandia.gov) or
//                    Denis Ridzal (dridzal@sandia.gov).
//
// ************************************************************************
// @HEADER

/** \file   Intrepid_Utils.hpp
    \brief  Intrepid utilities.
    \author Created by P. Bochev and D. Ridzal.
*/

#ifndef INTREPID_UTILS_HPP
#define INTREPID_UTILS_HPP

#include "Intrepid_ConfigDefs.hpp"
#include "Intrepid_Types.hpp"
#include "Teuchos_Array.hpp"
#include "Teuchos_oblackholestream.hpp"
#include "Teuchos_RCP.hpp"

namespace Intrepid {

/******************************************************* START *************************************************************/

//--------------------------------------------------------------------------------------------//
//                                                                                            //
//  Declarations: Utility functions for handling external data in tests                        //
//                                                                                            //
//--------------------------------------------------------------------------------------------//
  
enum TypeOfExactData{
  INTREPID_UTILS_FRACTION=0,
  INTREPID_UTILS_SCALAR
};


/** \brief  Compares the values in the test matrix <var><b>testMat</b></var> to precomputed
            analytic values stored in a file, where the input matrix is an array of arrays.

    \param  testMat          [in]     -  test matrix
    \param  inputFile        [in]     -  input file
    \param  reltol           [in]     -  relative tolerance for equality comparisons
    \param  iprint           [in]     -  if 0, no output; if 1, details are printed
    \param  analyticDataType [in]     -  type of analytic data for comparison:
                                         \li if INTREPID_UTILS_FRACTION, analytic fractions are parsed and computed
                                         \li if INTREPID_UTILS_SCALAR, high-precision scalar data is read
    \return 0 if pass; error code otherwise
 */
template<class Scalar>
int compareToAnalytic(const Teuchos::Array< Teuchos::Array<Scalar> > testMat,
                      std::ifstream & inputFile,
                      Scalar reltol,
                      int iprint,
                      TypeOfExactData analyticDataType = INTREPID_UTILS_FRACTION);

/** \brief  Compares the values in the test matrix <var><b>testMat</b></var> to precomputed
            analytic values stored in a file, where the input matrix is a single contiguous
            array.

    \param  testMat          [in]     -  test matrix
    \param  inputFile        [in]     -  input file
    \param  reltol           [in]     -  relative tolerance for equality comparisons
    \param  iprint           [in]     -  if 0, no output; if 1, details are printed
    \param  analyticDataType [in]     -  type of analytic data for comparison:
                                         \li if INTREPID_UTILS_FRACTION, analytic fractions are parsed and computed
                                         \li if INTREPID_UTILS_SCALAR, high-precision scalar data is read
    \return 0 if pass; error code otherwise
 */
template<class Scalar>
int compareToAnalytic(const Scalar * testMat,
                      std::ifstream & inputFile,
                      Scalar reltol,
                      int iprint,
                      TypeOfExactData analyticDataType = INTREPID_UTILS_FRACTION);



/** \brief  Loads analytic values stored in a file into the matrix <var><b>testMat</b></var>,
            where the output matrix is an array of arrays.

    \param  testMat          [in]     -  test matrix
    \param  inputFile        [in]     -  input file
    \param  analyticDataType [in]     -  type of analytic data for comparison:
                                         \li if INTREPID_UTILS_FRACTION, analytic fractions are parsed and computed
                                         \li if INTREPID_UTILS_SCALAR, high-precision scalar data is read
 */
template<class Scalar>
void getAnalytic(Teuchos::Array< Teuchos::Array<Scalar> > & testMat,
                 std::ifstream & inputFile,
                 TypeOfExactData analyticDataType = INTREPID_UTILS_FRACTION);

/** \brief  Loads analytic values stored in a file into the matrix <var><b>testMat</b></var>,
            where the output matrix is a single contiguous array.

    \param  testMat          [in]     -  test matrix
    \param  inputFile        [in]     -  input file
    \param  analyticDataType [in]     -  type of analytic data for comparison:
                                         \li if INTREPID_UTILS_FRACTION, analytic fractions are parsed and computed
                                         \li if INTREPID_UTILS_SCALAR, high-precision scalar data is read
 */
template<class Scalar>
void getAnalytic(Scalar * testMat,
                 std::ifstream & inputFile,
                 TypeOfExactData analyticDataType = INTREPID_UTILS_FRACTION);


//--------------------------------------------------------------------------------------------//
//                                                                                            //
//  Definitions: Utility functions for handling external data in tests                         //
//                                                                                            //
//--------------------------------------------------------------------------------------------//

template<class Scalar>
int compareToAnalytic(const Teuchos::Array< Teuchos::Array<Scalar> > testMat,
                      std::ifstream & inputFile,
                      Scalar reltol,
                      int iprint,
                      TypeOfExactData analyticDataType = INTREPID_UTILS_FRACTION) {

  // This little trick lets us print to std::cout only if
  // iprint > 0.
  Teuchos::RCP<std::ostream> outStream;
  Teuchos::oblackholestream bhs; // outputs nothing
  if (iprint > 0)
    outStream = Teuchos::rcp(&std::cout, false);
  else
    outStream = Teuchos::rcp(&bhs, false);

  // Save the format state of the original std::cout.
  Teuchos::oblackholestream oldFormatState;
  oldFormatState.copyfmt(std::cout);

  std::string line;
  std::string chunk;
  Scalar testentry;
  Scalar abstol;
  Scalar absdiff;
  int i=0, j=0;
  int err = 0;

  while (! inputFile.eof() )
  {
    std::getline (inputFile,line);
    std::istringstream linestream(line);
    std::string chunk;
    j = 0;
    while( linestream >> chunk ) {
      int num1;
      int num2;
      std::string::size_type loc = chunk.find( "/", 0);
      if( loc != std::string::npos ) {
        chunk.replace( loc, 1, " ");
        std::istringstream chunkstream(chunk);
        chunkstream >> num1;
        chunkstream >> num2;
        testentry = (Scalar)(num1)/(Scalar)(num2);
        abstol = ( std::fabs(testentry) < reltol ? reltol : std::fabs(reltol*testentry) );
        absdiff = std::fabs(testentry - testMat[i][j]);
        if (absdiff > abstol) {
          err++;
          *outStream << "FAILURE --> ";
        }
        *outStream << "entry[" << i << "," << j << "]:" << "   "
                   << testMat[i][j] << "   " << num1 << "/" << num2 << "   "
                   << absdiff << "   " << "<?" << "   " << abstol << "\n";
      }
      else {
        std::istringstream chunkstream(chunk);
        if (analyticDataType == INTREPID_UTILS_FRACTION) {
          chunkstream >> num1;
          testentry = (Scalar)(num1);
        }
        else if (analyticDataType == INTREPID_UTILS_SCALAR)
          chunkstream >> testentry;
        abstol = ( std::fabs(testentry) < reltol ?reltol : std::fabs(reltol*testentry) );
        absdiff = std::fabs(testentry - testMat[i][j]);
        if (absdiff > abstol) {
          err++;
          *outStream << "FAILURE --> ";
        }
        *outStream << "entry[" << i << "," << j << "]:" << "   "
                   << testMat[i][j] << "   " << testentry << "   "
                   << absdiff << "   " << "<?" << "   " << abstol << "\n";
      }
      j++;
    }
    i++;
  }

  // reset format state of std::cout
  std::cout.copyfmt(oldFormatState);

  return err;
} // end compareToAnalytic



template<class Scalar>
int compareToAnalytic(const Scalar * testMat,
                      std::ifstream & inputFile,
                      Scalar reltol,
                      int iprint,
                      TypeOfExactData analyticDataType = INTREPID_UTILS_FRACTION) {

  // This little trick lets us print to std::cout only if
  // iprint > 0.
  Teuchos::RCP<std::ostream> outStream;
  Teuchos::oblackholestream bhs; // outputs nothing
  if (iprint > 0)
    outStream = Teuchos::rcp(&std::cout, false);
  else
    outStream = Teuchos::rcp(&bhs, false);

  // Save the format state of the original std::cout.
  Teuchos::oblackholestream oldFormatState;
  oldFormatState.copyfmt(std::cout);

  std::string line;
  std::string chunk;
  Scalar testentry;
  Scalar abstol;
  Scalar absdiff;
  int i=0, j=0, offset=0;
  int err = 0;

  while (! inputFile.eof() )
  {
    std::getline (inputFile,line);
    std::istringstream linestream(line);
    std::string chunk;
    j = 0;
    while( linestream >> chunk ) {
      int num1;
      int num2;
      std::string::size_type loc = chunk.find( "/", 0);
      if( loc != std::string::npos ) {
        chunk.replace( loc, 1, " ");
        std::istringstream chunkstream(chunk);
        chunkstream >> num1;
        chunkstream >> num2;
        testentry = (Scalar)(num1)/(Scalar)(num2);
        abstol = ( std::fabs(testentry) < reltol ? reltol : std::fabs(reltol*testentry) );
        absdiff = std::fabs(testentry - testMat[i*offset+j]);
        if (absdiff > abstol) {
          err++;
          *outStream << "FAILURE --> ";
        }
        *outStream << "entry[" << i << "," << j << "]:" << "   "
                   << testMat[i*offset+j] << "   " << num1 << "/" << num2 << "   "
                   << absdiff << "   " << "<?" << "   " << abstol << "\n";
      }
      else {
        std::istringstream chunkstream(chunk);
        if (analyticDataType == INTREPID_UTILS_FRACTION) {
          chunkstream >> num1;
          testentry = (Scalar)(num1);
        }
        else if (analyticDataType == INTREPID_UTILS_SCALAR)
          chunkstream >> testentry;
        abstol = ( std::fabs(testentry) < reltol ?reltol : std::fabs(reltol*testentry) );
        absdiff = std::fabs(testentry - testMat[i*offset+j]);
        if (absdiff > abstol) {
          err++;
          *outStream << "FAILURE --> ";
        }
        *outStream << "entry[" << i << "," << j << "]:" << "   "
                   << testMat[i*offset+j] << "   " << testentry << "   "
                   << absdiff << "   " << "<?" << "   " << abstol << "\n";
      }
      j++;
    }
    i++;
    offset = j;
  }

  // reset format state of std::cout
  std::cout.copyfmt(oldFormatState);

  return err;
} // end compareToAnalytic



template<class Scalar>
void getAnalytic(Teuchos::Array< Teuchos::Array<Scalar> > & testMat,
                 std::ifstream & inputFile,
                 TypeOfExactData analyticDataType = INTREPID_UTILS_FRACTION) {

  // Save the format state of the original std::cout.
  Teuchos::oblackholestream oldFormatState;
  oldFormatState.copyfmt(std::cout);

  std::string line;
  std::string chunk;
  Scalar testentry;
  int i=0, j=0;

  while (! inputFile.eof() )
  {
    std::getline (inputFile,line);
    std::istringstream linestream(line);
    std::string chunk;
    j = 0;
    while( linestream >> chunk ) {
      int num1;
      int num2;
      std::string::size_type loc = chunk.find( "/", 0);
      if( loc != std::string::npos ) {
        chunk.replace( loc, 1, " ");
        std::istringstream chunkstream(chunk);
        chunkstream >> num1;
        chunkstream >> num2;
        testentry = (Scalar)(num1)/(Scalar)(num2);
        testMat[i][j] = testentry;
      }
      else {
        std::istringstream chunkstream(chunk);
        if (analyticDataType == INTREPID_UTILS_FRACTION) {
          chunkstream >> num1;
          testentry = (Scalar)(num1);
        }
        else if (analyticDataType == INTREPID_UTILS_SCALAR)
          chunkstream >> testentry;
        testMat[i][j] = testentry;
      }
      j++;
    }
    i++;
  }

  // reset format state of std::cout
  std::cout.copyfmt(oldFormatState);
} // end getAnalytic



template<class Scalar>
void getAnalytic(Scalar * testMat,
                 std::ifstream & inputFile,
                 TypeOfExactData analyticDataType = INTREPID_UTILS_FRACTION) {

  // Save the format state of the original std::cout.
  Teuchos::oblackholestream oldFormatState;
  oldFormatState.copyfmt(std::cout);

  std::string line;
  std::string chunk;
  Scalar testentry;
  int i=0, j=0, offset=0;

  while (! inputFile.eof() )
  {
    std::getline (inputFile,line);
    std::istringstream linestream(line);
    std::string chunk;
    j = 0;
    while( linestream >> chunk ) {
      int num1;
      int num2;
      std::string::size_type loc = chunk.find( "/", 0);
      if( loc != std::string::npos ) {
        chunk.replace( loc, 1, " ");
        std::istringstream chunkstream(chunk);
        chunkstream >> num1;
        chunkstream >> num2;
        testentry = (Scalar)(num1)/(Scalar)(num2);
        testMat[i*offset+j] = testentry;
      }
      else {
        std::istringstream chunkstream(chunk);
        if (analyticDataType == INTREPID_UTILS_FRACTION) {
          chunkstream >> num1;
          testentry = (Scalar)(num1);
        }
        else if (analyticDataType == INTREPID_UTILS_SCALAR)
          chunkstream >> testentry;
        testMat[i*offset+j] = testentry;
      }
      j++;
    }
    i++;
    offset = j;
  }

  // reset format state of std::cout
  std::cout.copyfmt(oldFormatState);
} // end getAnalytic

/******************************************************* STOP *************************************************************/




/******************************************************* START *************************************************************/

//--------------------------------------------------------------------------------------------//
//                                                                                            //
//    Functions for orders, cardinality and etc. of differential operators.                   //
//                                                                                            //
//--------------------------------------------------------------------------------------------//


/** \brief  Returns the rank of fields in a function space of the specified type.

            Field rank is defined as the number of indices needed to specify function value and 
            equals 0, 1,or 2 for scalars, vectors and tensors, respectively. The scalar field
            spaces in Intrepid are FUNCTION_SPACE_HGRAD and FUNCTION_SPACE_HVOL. The vector field
            spaces are FUNCTION_SPACE_HCURL, FUNCTION_SPACE_HDIV and FUNCTION_SPACE_VECTOR_HGRAD.
            FUNCTION_SPACE_TENSOR_HGRAD contains rank-2 tensors.

    \param  spaceType         [in]     -  function space type
    \return rank of the fields in the specified function space
 */
int getFieldRank(const EFunctionSpace spaceType); 



/** \brief  Returns rank of an operator. 

            When an operator acts on a field of a certain rank, the result can be a field with the
            same or a different rank. Operator rank is defined the difference between the ranks of
            the output field and the input field:
            \verbatim
                          Rank(OPERATOR) = Rank(OPERATOR(FIELD)) - Rank(FIELD)
            \endverbatim
            Therefore, operator rank allows us to figure out the rank of the result: 
            \verbatim
                          Rank(OPERATOR(FIELD)) = Rank(FIELD) + Rank(OPERATOR)
           \endverbatim
           and provides means to size properly arrays for output results. The following table 
           summarizes operator ranks (~ denotes undefined, below slash means 3D).
           By default, in 1D any operator other than VALUE has rank 1, i.e., GRAD, CURL and DIV
           reduce to d/dx and Dk are the higher-order derivatives d^k/dx^k. Only scalar functions 
           are allowed in 1D.

           \verbatim
           |========|======|============================|=========|==========|==========|==========|
           | field  | rank |  FUNCTION_SPACE_[type]     |  VALUE  | GRAD, Dk |   CURL   |    DIV   |
           |--------|------|----------------------------|---------|----------|----------|----------|
           | scalar |   0  |  HGRAD, HVOL               |    0    |     1    | 3-dim/~  |     ~    |
           | vector |   1  |  HCURL, HDIV, VECTOR_HGRAD |    0    |     1    | dim - 3  |    -1    |
           | tensor |   2  |  TENSOR_HGRAD              |    0    |     1    | dim - 3  |    -1    |
           |--------|------|----------------------------|---------|----------|----------|----------|
           |   1D   |   0  |  HGRAD, HVOL only          |    0    |     1    |     1    |     1    |
           |=======================================================================================|
           \endverbatim

    \param  spaceType        [in]    - function space type
    \param  operatorType     [in]    - the operator acting on the specified function space
    \param  spaceDim         [in]    - spatial dimension
    \return rank of the operator as defined in the table
 */
int getOperatorRank(const EFunctionSpace spaceType,
                    const EOperator      operatorType,
                    const int            spaceDim);



/** \brief  Returns order of an operator.

    \param  operatorType       [in]    - type of the operator whose order we want to know
    \return result ranges from 0 (for OPERATOR_VALUE) to 10 (OPERATOR_D10)
 */
int getOperatorOrder(const EOperator operatorType);



/** \brief  Returns the ordinal of a partial derivative of order k based on the multiplicities of
            the partials dx, dy, and dz.

            By default, any implementation of Intrepid::Basis method returns partials of order k
            (specified by OPERATOR_Dk) as a multiset ordered by the lexicographical order of the
            partial derivatives multiplicities. For example, the 10 derivatives of order 3 in 3D 
            are enumerated as:
            \verbatim
            D3={(3,0,0),(2,1,0),(2,0,1),(1,2,0),(1,1,1),(1,0,2),(0,3,0),(0,2,1),(0,1,2),(0,0,3)}
            \endverbatim
            The enumeration formula for this lexicographical order is
<table>   
<tr> <td>\f$i(xMult)            = 0\f$                            </td> <td>in 1D (only 1 derivative)</td> </tr>
<tr> <td>\f$i(xMult,yMult)      =yMult\f$                         </td> <td>in 2D</td> </tr>
<tr> <td>\f$i(xMult,yMult,zMult)=zMult+\sum_{r = 0}^{k-xMult} r\f$</td> <td>in 3D</td> </tr>
</table>
           where the order k of Dk is implicitly defined by xMult + yMult + zMult. Space dimension is
           implicitly defined by the default values of the multiplicities of y and z derivatives.

    \param  xMult            [in]    - multiplicity of dx
    \param  yMult            [in]    - multiplicity of dy (default = -1)
    \param  zMult            [in]    - multiplicity of dz (default = -1)
    \return the ordinal of partial derivative of order k as function of dx, dy, dz multiplicities
 */
int getDkEnumeration(const int xMult,
                     const int yMult = -1,
                     const int zMult = -1);



/** \brief  Returns multiplicities of dx, dy, and dz based on the enumeration of the partial
            derivative, its order and the space dimension. Inverse of the getDkEnumeration() method.

    \param  partialMult      [out]    - array with the multiplicities f dx, dy and dz
    \param  derivativeEnum   [in]     - enumeration of the partial derivative
    \param  operatorType     [in]     - k-th partial derivative Dk
    \param  spaceDim         [in]     - space dimension
 */
void getDkMultiplicities(Teuchos::Array<int>&  partialMult,
                         const int             derivativeEnum,
                         const EOperator       operatorType,
                         const int             spaceDim);



/** \brief  Returns cardinality of Dk, i.e., the number of all derivatives of order k. 

            The set of all partial derivatives of order k is isomorphic to the set of all multisets 
            of cardinality k with elements taken from the sets {x}, {x,y}, and {x,y,z} in 1D, 2D, 
            and 3D respectively. For example, the partial derivative
            \f$\displaystyle D\{1,2,1\}f = \frac{d^4 f}{dx dy^2 dz}\f$  maps to the multiset
            \f$\{x, y, z\}\f$ with multiplicities \f$\{1,2,1\}\f$. The number of all such multisets 
            is given by the binomial coefficient
            \f[       \begin{pmatrix} spaceDim + k - 1 \\ spaceDim - 1 \end{pmatrix}              \f]
            Therefore:
    \li     in 1D: cardinality = 1\n
    \li     in 2D: cardinality = k + 1\n
    \li     in 3D: cardinality = (k + 1)*(k + 2)/2

    \param  operatorType     [in]     - k-th derivative operator Dk
    \param  spaceDim         [in]     - space dimension
    \return the number of all partial derivatives of order k
*/
int getDkCardinality(const EOperator operatorType,
                     const int       spaceDim);

/******************************************************* STOP *************************************************************/




/******************************************************* START *************************************************************/

//--------------------------------------------------------------------------------------------//
//                                                                                            //
//    Declarations: array utilities                                                           //
//                                                                                            //
//--------------------------------------------------------------------------------------------//

/** \brief  Compares two multi-dimensional arrays.

    \param  left             [in]   - left array argument
    \param  right            [in]   - right array argument
    \return  0  if arrays have the same rank and all their dimensions match
            -1  if arrays have different ranks
             i  where i>0 is the ordinal of the first non-matching dimension plus one
  */
template<class ArrayScalar>
int compareArrays(const ArrayScalar & left,
                  const ArrayScalar & right);



/** \brief  Checks the rank of a multi-dimensional array.

    \param  array           [in]  - input array
    \param  rank            [in]  - expected rank for that array
    \return  0  if array.rank() != rank
             1  if array.rank() == rank
 */
template<class ArrayScalar>
int validateRank(const ArrayScalar & array,
                 const int &         rank);



/** \brief  Checks dimension of a multi-dimensional array.

            If only lowerBound is specified this method checks for exact match of the specified 
            dimension and the value of lowerBound. If both lowerBound and upperBound are specified  
            the method checks that the dimension value is in the range specified by these values.

    \param  array           [in]  - input array
    \param  dim             [in]  - dimension that is being validated
    \param  lowerBound      [in]  - the lower bound for that dimension
    \param  upperBound      [in]  - the upper bound for that dimension (default = -1)
    \return  1  if ( upperBound = -1 && array.dimension(dim) == lowerBound ) ||
                   ( 0 <= lowerBound <= array.dimension(dim) <= upperBound ) 
             0  otherwise
            -1  if dim > array.rank() - 1 (invalid dimension)
            -2  if lowerBound > upperBound   (invalid range)
            -3  if upperBound == -1 and lowerBound < 0 (invalid dimension value)
  */
template<class ArrayScalar>
int validateDimension(const ArrayScalar & array,
                      const int &         dim,
                      const int &         lowerBound,
                      const int &         upperBound = -1 );

//--------------------------------------------------------------------------------------------//
//                                                                                            //
//    Definitions: array utilities                                                           //
//                                                                                            //
//--------------------------------------------------------------------------------------------//


template<class ArrayScalar>
int compareArrays(const ArrayScalar & left,
                  const ArrayScalar & right){
  int rankLeft = left.rank();
  
  if(rankLeft != right.rank() ) {
    return -1;
  }
  else {
    for(int i = 0; i < rankLeft; i++) {
      if(left.dimension(i) != right.dimension(i) ) {
        return i + 1;
      }
    }
  }
  return 0;
}



template<class ArrayScalar>
int validateRank(const ArrayScalar & array,
                 const int &         rank){
  if(array.rank() == rank){
    return 1;
  }
  else {
    return 0;
  }
}



template<class ArrayScalar>
int validateDimension(const ArrayScalar & array,
                      const int &         dim,
                      const int &         lowerBound,
                      const int &         upperBound){
  
  // Check dimension bounds and rank before attempting to validate the array
  if( upperBound == -1 && lowerBound < 0 ) {
    return -3;
  }
  if( lowerBound > upperBound ) {
    return -2;
  }
  if( array.rank() - 1 < dim ) {
    return -1;
  }
  
  // Now we can validate the dimension
  if( ( (upperBound == -1) && (array.dimension(dim) == lowerBound) ) ||
      ( (lowerBound <= array.dimension(dim) ) && ( array.dimension(dim) <= upperBound) ) ) {
    return 1;
  }
  else{
    return 0;
  }
}

/******************************************************* STOP *************************************************************/




/******************************************************* START *************************************************************/

//--------------------------------------------------------------------------------------------//
//                                                                                            //
//              Declarations:      Helper functions of the Basis class                        //
//                                                                                            //
//--------------------------------------------------------------------------------------------//

/** \brief  Fills <var>ordinalToTag_</var> and <var>tagToOrdinal_</var> by basis-specific tag data

    \param  tagToOrdinal     [out]  - Lookup table for the DoF's ordinal by its tag
    \param  ordinalToTag     [out]  - Lookup table for the DoF's tag by its ordinal
    \param  tags             [in]   - a set of basis-dependent tags in flat (rank-1) array format.
    \param  basisCard        [in]   - cardinality of the basis
    \param  tagSize          [in]   - number of fields in a DoF tag
    \param  posScDim         [in]   - position in the tag, counting from 0, of the subcell dim
    \param  posScOrd         [in]   - position in the tag, counting from 0, of the subcell ordinal
    \param  posDfOrd         [in]   - position in the tag, counting from 0, of DoF ordinal relative to the subcell
 */
void setOrdinalTagData(std::vector<std::vector<std::vector<int> > >   &tagToOrdinal,
                       std::vector<std::vector<int> >                 &ordinalToTag,
                       const int                                      *tags,
                       const int                                      basisCard,
                       const int                                      tagSize,
                       const int                                      posScDim,
                       const int                                      posScOrd,
                       const int                                      posDfOrd);

/******************************************************* STOP *************************************************************/



} // end namespace Intrepid

#endif