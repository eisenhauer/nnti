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

/** \file   Intrepid_CubatureTensorSorted.hpp
    \brief  Header file for the Intrepid::CubatureTensorSorted class.
    \author Created by D. Kouri and D. Ridzal.
*/

#ifndef INTREPID_CUBATURE_TENSORSORTED_HPP
#define INTREPID_CUBATURE_TENSORSORTED_HPP

#include "Intrepid_CubatureLineSorted.hpp"

namespace Intrepid {

/** \class Intrepid::CubatureTensorSorted
    \brief Utilizes 1D cubature (integration) rules contained in the library sandia_rules
           (John Burkardt, Scientific Computing, Florida State University) within Intrepid.
	   
	   This class contains ten rules:
	   \li Gauss-Chebyshev Type 1 - integration domain is [-1,+1] with weight w(x) = 1/sqrt(1-x^2)
	   \li Gauss-Chebyshev Type 2 - integration domain is [-1,+1] with weight w(x) = sqrt(1-x^2)
	   \li Clenshaw-Curtis        - integration domain is [-1,+1] with weight w(x) = 1
	   \li Fejer Type 2           - integration domain is [-1,+1] with weight w(x) = 1
	   \li Gauss-Legendre         - integration domain is [-1,+1] with weight w(x) = 1
	   \li Gauss-Patterson        - integration domain is [-1,+1] with weight w(x) = 1
	   \li Trapezoidal            - integration domain is [-1,+1] with weight w(x) = 1
	   \li Gauss-Hermite          - integration domain is (-oo,+oo) with weight w(x) = exp(-x^2)
	   \li Hermite-Genz-Keister   - integration domain is (-oo,+oo) with weight w(x) = exp(-x^2)
	   \li Gauss-Laguerre         - integration domain is [0,+oo) with weight w(x) = exp(-x)
*/

/* =========================================================================
                       Structure Definition - quadRule
   ========================================================================= */ 
//struct quadInfo { 
  /*
    The quadRule structure contains information about a multidimensional 
    tensor product quadrature rule.  
  */
  //  int dim;          // Number of Spatial Dimensions
  //  int maxlevel;     // Maximum Order of 1D rules
  //  int * rule;       // 1D rules defined in each dimension
  //  int * growth;     // 1D growth rules defined in each dimension
  //  int * np;         // Number of parameters for each 1D rule
  //  double * param;   // Parameters for each 1D rule
  //};

template<class Scalar, class ArrayPoint = FieldContainer<Scalar>, class ArrayWeight = ArrayPoint>
class CubatureTensorSorted : public Intrepid::Cubature<Scalar,ArrayPoint,ArrayWeight> {

private:  
  /** \brief Contains nodes of this cubature rule.
  */
  typename std::map<std::vector<Scalar>,int> points_; // keys = nodes, values = location of weights
  
  /** \brief Contains weights of this cubature rule.
  */
  std::vector<Scalar> weights_;

  /** \brief Contains the number of nodes for this cubature rule.
  */
  int numPoints_;
  
  /** \brief The degree of polynomials that are integrated
             exactly by this cubature rule.
  */
  std::vector<int> degree_;

  /** \brief Dimension of integration domain.
  */
  int dimension_;
  
public:

  ~CubatureTensorSorted() {}

  //  CubatureTensorSorted() {}

  CubatureTensorSorted(int numPoints = 0, int dimension = 1);

  /** \brief Constructor.

      \param cubLine       [in]   - 1D cubature rule.
  */
  CubatureTensorSorted(CubatureLineSorted<Scalar> & cubLine);
 
  /** \brief Constructor.

      \param dimension     [in]   - The number of spatial dimensions.
      \param numPoints1D   [in]   - The number of cubature points in each direction.
      \param rule1D        [in]   - The cubature rule for each direction.
      \param isNormalized  [in]   - Flag whether or not to normalize the cubature weights.
  */
  CubatureTensorSorted(int dimension, std::vector<int> numPoints1D, std::vector<EIntrepidBurkardt> rule1D, bool isNormalized);

  /** \brief Constructor.

      \param dimension     [in]   - The number of spatial dimensions.
      \param numPoints1D   [in]   - The number of cubature points in each direction.
      \param rule1D        [in]   - The cubature rule for each direction.
      \param isNormalized  [in]   - Flag whether or not to normalize the cubature weights.
  */
  CubatureTensorSorted(int dimension, std::vector<int> numPoints1D, std::vector<EIntrepidBurkardt> rule1D, std::vector<EIntrepidGrowth> growth1D, bool isNormalized);

  /** \brief Constructor.

      \param dimension     [in]   - The number of spatial dimensions.
      \param maxNumPoints  [in]   - The maximum number of cubature points in each direction.
      \param rule1D        [in]   - The cubature rule for each direction.
      \param growth1D      [in]   - Growth rule for each direction.
      \param isNormalized  [in]   - Flag whether or not to normalize the cubature weights.
  */
  CubatureTensorSorted(int dimension, int maxNumPoints, std::vector<EIntrepidBurkardt> rule1D, std::vector<EIntrepidGrowth> growth1D, bool isNormalized);

  // Access Operator		
  /** \brief Returns cubature points and weights
             (return arrays must be pre-sized/pre-allocated).

      \param cubPoints   [out]   - Array containing the cubature points.
      \param cubWeights  [out]   - Array of corresponding cubature weights.
  */
  void getCubature(ArrayPoint  & cubPoints,
		   ArrayWeight & cubWeights) const;

  /** \brief Returns the number of cubature points.
  */
  int getNumPoints() const;

  /** \brief Returns max. degree of polynomials that are integrated exactly.
             The return vector has size 1.
  */
  void getAccuracy(std::vector<int> & accuracy) const;

  /** \brief Returns dimension of domain of integration.
  */  
  int getDimension() const;

  /** \brief Initiate iterator at the beginning of data.
  */
  typename std::map<std::vector<Scalar>,int>::iterator begin();

  /** \brief Initiate iterator at the end of data.
  */
  typename std::map<std::vector<Scalar>,int>::iterator end();

  /** \brief Insert a node and weight into data near the iterator position.
  */
  void insert(typename std::map<std::vector<Scalar>,int>::iterator it, 	      
	      std::vector<Scalar> point, Scalar weight);

  /** \brief Get a specific node described by the iterator location.
  */
  std::vector<Scalar> getNode(typename std::map<std::vector<Scalar>,int>::iterator it);

  /** \brief Get a specific weight described by the integer location.
  */
  Scalar getWeight(int node);

  /** \brief Get a specific weight described by the iterator location.
  */
  Scalar getWeight(std::vector<Scalar> point);

  /** \brief Replace CubatureLineSorted values with 
             "this = alpha1*this+alpha2*cubRule2".
  */
  void update(Scalar alpha2, CubatureTensorSorted<Scalar> & cubRule2, Scalar alpha1); 

  /** \brief Normalize CubatureLineSorted weights.
  */
  void normalize();

};

template<class Scalar>
CubatureTensorSorted<Scalar> kron_prod(CubatureTensorSorted<Scalar> & rule1,
				       CubatureLineSorted<Scalar>   & rule2 );

} // Intrepid Namespace


// include templated definitions
#include <Intrepid_CubatureTensorSortedDef.hpp>

#endif
