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

/** \file   Intrepid_ArrayToolsDefCloneScale.hpp
    \brief  Definition file for clone / scale operations of the array tools interface.
    \author Created by P. Bochev and D. Ridzal.
*/

namespace Intrepid {


template<class Scalar, class ArrayOutFields, class ArrayInFields>
void ArrayTools::cloneFields(ArrayOutFields &       outputFields,
                             const ArrayInFields &  inputFields) {

#ifdef HAVE_INTREPID_DEBUG
  TEST_FOR_EXCEPTION( ( (inputFields.rank() < 2) || (inputFields.rank() > 4) ), std::invalid_argument,
                      ">>> ERROR (ArrayTools::cloneFields): Input fields container must have rank 2, 3, or 4.");
  TEST_FOR_EXCEPTION( (outputFields.rank() != inputFields.rank()+1), std::invalid_argument,
                      ">>> ERROR (ArrayTools::cloneFields): The rank of the input fields container must be one less than the rank of the output fields container.");
  for (int i=0; i<inputFields.rank(); i++) {
    std::string errmsg  = ">>> ERROR (ArrayTools::cloneFields): Dimensions ";
    errmsg += (char)(48+i);
    errmsg += " and ";
    errmsg += (char)(48+i+1);
    errmsg += " of the input and output fields containers must agree!";
    TEST_FOR_EXCEPTION( (inputFields.dimension(i) != outputFields.dimension(i+1)), std::invalid_argument, errmsg );
  }
#endif

  // get sizes
  int invalRank      = inputFields.rank();
  int outvalRank     = outputFields.rank();
  int numCells       = outputFields.dimension(0);
  int numFields      = outputFields.dimension(1);
  int numPoints      = outputFields.dimension(2);
  int dim1Tens       = 0;
  int dim2Tens       = 0;
  if (outvalRank > 3) {
    dim1Tens = outputFields.dimension(3);
    if (outvalRank > 4) {
      dim2Tens = outputFields.dimension(4);
    }
  }

  switch(invalRank) {
    case 2: {
      for(int cl = 0; cl < numCells; cl++) {
        for(int bf = 0; bf < numFields; bf++) {
          for(int pt = 0; pt < numPoints; pt++) {
            outputFields(cl, bf, pt) = inputFields(bf, pt);
          } // P-loop
        } // F-loop
      } // C-loop
    }// case 2
    break;

    case 3: {
      for(int cl = 0; cl < numCells; cl++) {
        for(int bf = 0; bf < numFields; bf++) {
          for(int pt = 0; pt < numPoints; pt++) {
            for( int iVec = 0; iVec < dim1Tens; iVec++) {
              outputFields(cl, bf, pt, iVec) = inputFields(bf, pt, iVec);
            } // D1-loop
          } // P-loop
        } // F-loop
      } // C-loop
    }// case 3
    break;

    case 4: {
      for(int cl = 0; cl < numCells; cl++) {
        for(int bf = 0; bf < numFields; bf++) {
          for(int pt = 0; pt < numPoints; pt++) {
            for( int iTens1 = 0; iTens1 < dim1Tens; iTens1++) {
              for( int iTens2 = 0; iTens2 < dim2Tens; iTens2++) {
                outputFields(cl, bf, pt, iTens1, iTens2) = inputFields(bf, pt, iTens1, iTens2);
              } // D2-loop
            } // D1-loop
          } // P-loop
        } // F-loop
      } // C-loop
    }// case 4
    break;

    default:
      TEST_FOR_EXCEPTION( !( (invalRank == 2) || (invalRank == 3) || (invalRank == 4) ), std::invalid_argument,
                          ">>> ERROR (ArrayTools::cloneFields): This method is defined only for rank-2, 3 or 4 input containers.");
  }// invalRank

} // cloneFields


template<class Scalar, class ArrayOutFields, class ArrayInFactors, class ArrayInFields>
void ArrayTools::cloneScaleFields(ArrayOutFields &        outputFields,
                                  const ArrayInFactors &  inputFactors,
                                  const ArrayInFields &   inputFields) {

#ifdef HAVE_INTREPID_DEBUG
  TEST_FOR_EXCEPTION( (inputFactors.rank() != 2), std::invalid_argument,
                      ">>> ERROR (ArrayTools::cloneScaleFields): The rank of the input factors container must be 2.");
  TEST_FOR_EXCEPTION( ( (inputFields.rank() < 2) || (inputFields.rank() > 4) ), std::invalid_argument,
                      ">>> ERROR (ArrayTools::cloneScaleFields): Input fields container must have rank 2, 3, or 4.");
  TEST_FOR_EXCEPTION( (outputFields.rank() != inputFields.rank()+1), std::invalid_argument,
                      ">>> ERROR (ArrayTools::cloneScaleFields): The rank of the input fields container must be one less than the rank of the output fields container.");
  TEST_FOR_EXCEPTION( ( inputFactors.dimension(0) != outputFields.dimension(0) ), std::invalid_argument,
                      ">>> ERROR (ArrayTools::cloneScaleFields): Zeroth dimensions of input factors container and output fields container (numbers of integration domains) must agree!");
  TEST_FOR_EXCEPTION( ( inputFactors.dimension(1) != outputFields.dimension(1) ), std::invalid_argument,
                      ">>> ERROR (ArrayTools::cloneScaleFields): First dimensions of input factors container and output fields container (numbers of fields) must agree!");
  for (int i=0; i<inputFields.rank(); i++) {
    std::string errmsg  = ">>> ERROR (ArrayTools::cloneScaleFields): Dimensions ";
    errmsg += (char)(48+i);
    errmsg += " and ";
    errmsg += (char)(48+i+1);
    errmsg += " of the input and output fields containers must agree!";
    TEST_FOR_EXCEPTION( (inputFields.dimension(i) != outputFields.dimension(i+1)), std::invalid_argument, errmsg );
  }
#endif

  // get sizes
  int invalRank      = inputFields.rank();
  int outvalRank     = outputFields.rank();
  int numCells       = outputFields.dimension(0);
  int numFields      = outputFields.dimension(1);
  int numPoints      = outputFields.dimension(2);
  int dim1Tens       = 0;
  int dim2Tens       = 0;
  if (outvalRank > 3) {
    dim1Tens = outputFields.dimension(3);
    if (outvalRank > 4) {
      dim2Tens = outputFields.dimension(4);
    }
  }

  switch(invalRank) {
    case 2: {
      for(int cl = 0; cl < numCells; cl++) {
        for(int bf = 0; bf < numFields; bf++) {
          for(int pt = 0; pt < numPoints; pt++) {
            outputFields(cl, bf, pt) = inputFields(bf, pt) * inputFactors(cl, bf);
          } // P-loop
        } // F-loop
      } // C-loop
    }// case 2
    break;

    case 3: {
      for(int cl = 0; cl < numCells; cl++) {
        for(int bf = 0; bf < numFields; bf++) {
          for(int pt = 0; pt < numPoints; pt++) {
            for( int iVec = 0; iVec < dim1Tens; iVec++) {
              outputFields(cl, bf, pt, iVec) = inputFields(bf, pt, iVec) * inputFactors(cl, bf);
            } // D1-loop
          } // P-loop
        } // F-loop
      } // C-loop
    }// case 3
    break;

    case 4: {
      for(int cl = 0; cl < numCells; cl++) {
        for(int bf = 0; bf < numFields; bf++) {
          for(int pt = 0; pt < numPoints; pt++) {
            for( int iTens1 = 0; iTens1 < dim1Tens; iTens1++) {
              for( int iTens2 = 0; iTens2 < dim2Tens; iTens2++) {
                outputFields(cl, bf, pt, iTens1, iTens2) = inputFields(bf, pt, iTens1, iTens2) * inputFactors(cl, bf);
              } // D2-loop
            } // D1-loop
          } // P-loop
        } // F-loop
      } // C-loop
    }// case 4
    break;

    default:
      TEST_FOR_EXCEPTION( !( (invalRank == 2) || (invalRank == 3) || (invalRank == 4) ), std::invalid_argument,
                          ">>> ERROR (ArrayTools::cloneScaleFields): This method is defined only for rank-2, 3 or 4 input containers.");
  }// invalRank

} // cloneScaleFields


template<class Scalar, class ArrayInOutFields, class ArrayInFactors>
void ArrayTools::scaleFields(ArrayInOutFields &      inoutFields,
                             const ArrayInFactors &  inputFactors) {

#ifdef HAVE_INTREPID_DEBUG
  TEST_FOR_EXCEPTION( (inputFactors.rank() != 2), std::invalid_argument,
                      ">>> ERROR (ArrayTools::scaleFields): The rank of the input factors container must be 2.");
  TEST_FOR_EXCEPTION( ( (inoutFields.rank() < 3) || (inoutFields.rank() > 5) ), std::invalid_argument,
                      ">>> ERROR (ArrayTools::scaleFields): Input/output fields container must have rank 3, 4, or 5.");
  TEST_FOR_EXCEPTION( ( inputFactors.dimension(0) != inoutFields.dimension(0) ), std::invalid_argument,
                      ">>> ERROR (ArrayTools::scaleFields): Zeroth dimensions of input factors container and input/output fields container (numbers of integration domains) must agree!");
  TEST_FOR_EXCEPTION( ( inputFactors.dimension(1) != inoutFields.dimension(1) ), std::invalid_argument,
                      ">>> ERROR (ArrayTools::scaleFields): First dimensions (number of fields) of input factors and input/output fields containers must agree!");
#endif

  // get sizes
  int inoutRank      = inoutFields.rank();
  int numCells       = inoutFields.dimension(0);
  int numFields      = inoutFields.dimension(1);
  int numPoints      = inoutFields.dimension(2);
  int dim1Tens       = 0;
  int dim2Tens       = 0;
  if (inoutRank > 3) {
    dim1Tens = inoutFields.dimension(3);
    if (inoutRank > 4) {
      dim2Tens = inoutFields.dimension(4);
    }
  }

  switch(inoutRank) {
    case 3: {
      for(int cl = 0; cl < numCells; cl++) {
        for(int bf = 0; bf < numFields; bf++) {
          for(int pt = 0; pt < numPoints; pt++) {
            inoutFields(cl, bf, pt) = inoutFields(cl, bf, pt) * inputFactors(cl, bf);
          } // P-loop
        } // F-loop
      } // C-loop
    }// case 2
    break;

    case 4: {
      for(int cl = 0; cl < numCells; cl++) {
        for(int bf = 0; bf < numFields; bf++) {
          for(int pt = 0; pt < numPoints; pt++) {
            for( int iVec = 0; iVec < dim1Tens; iVec++) {
              inoutFields(cl, bf, pt, iVec) = inoutFields(cl, bf, pt, iVec) * inputFactors(cl, bf);
            } // D1-loop
          }// P-loop
        } // F-loop
      } // C-loop
    }// case 3
    break;

    case 5: {
      for(int cl = 0; cl < numCells; cl++) {
        for(int bf = 0; bf < numFields; bf++) {
          for(int pt = 0; pt < numPoints; pt++) {
            for( int iTens1 = 0; iTens1 < dim1Tens; iTens1++) {
              for( int iTens2 = 0; iTens2 < dim2Tens; iTens2++) {
                inoutFields(cl, bf, pt, iTens1, iTens2) = inoutFields(cl, bf, pt, iTens1, iTens2) * inputFactors(cl, bf);
              } // D2-loop
            } // D1-loop
          } // P-loop
        } // F-loop
      } // C-loop
    }// case 4
    break;

    default:
      TEST_FOR_EXCEPTION( !( (inoutRank == 3) || (inoutRank == 4) || (inoutRank == 5) ), std::invalid_argument,
                          ">>> ERROR (ArrayTools::cloneScaleFields): This method is defined only for rank-3, 4 or 5 input/output containers.");
  }// inoutRank

} // scaleFields


} // end namespace Intrepid
