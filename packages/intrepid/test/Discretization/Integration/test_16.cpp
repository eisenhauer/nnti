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


/** \file
\brief  Unit test (CubatureDirect): correctness of
        integration of monomials for 1D reference cells.
\author Created by P. Bochev and D. Ridzal.
*/

#include "Intrepid_CubatureTensorSorted.hpp"
//#include "Intrepid_CubatureLineSorted.hpp"
#include "Intrepid_Utils.hpp"
#include "Teuchos_oblackholestream.hpp"
#include "Teuchos_RCP.hpp"
#include "Teuchos_GlobalMPISession.hpp"

using namespace Intrepid;

/*
  Computes integrals of monomials over a given reference cell.
*/
long double evalQuad(std::vector<int> power, int dimension, 
		     std::vector<int> order, 
		     std::vector<EIntrepidBurkardt> rule) {

  CubatureTensorSorted<long double> lineCub(dimension,order,rule,false);
  order[0]--; order[1]--;
  CubatureTensorSorted<long double> lineCub1(dimension,order,rule,false);  
  lineCub.update(1.0,lineCub1,1.0); 

  int size = lineCub.getNumPoints();
  FieldContainer<long double> cubPoints(size,dimension);
  FieldContainer<long double> cubWeights(size);
  lineCub.getCubature(cubPoints,cubWeights);

  int mid  = size/2;
  long double Q = 0.0;
  if (size%2) {
    Q  = cubWeights(mid);
    for (int i=0; i<dimension; i++) {
      Q *= powl(cubPoints(mid,i),(long double)power[i]);
    }
  }

  for (int i=0; i<mid; i++) {
    long double value1 = cubWeights(i);
    long double value2 = cubWeights(size-i-1);
    for (int j=0; j<dimension; j++) {
      value1 *= powl(cubPoints(i,j),(long double)power[j]);
      value2 *= powl(cubPoints(size-i-1,j),(long double)power[j]);
    }
    Q += value1+value2;
  }
  return Q;
}

long double evalInt(int dimension, std::vector<int> power, 
		    std::vector<EIntrepidBurkardt> rule) {
  long double I = 1.0;

  for (int i=0; i<dimension; i++) {
    if (rule[i]==BURK_CLENSHAWCURTIS||rule[i]==BURK_FEJER2||
	rule[i]==BURK_LEGENDRE||rule[i]==BURK_PATTERSON || 
	rule[i]==BURK_TRAPEZOIDAL) {
      if (power[i]%2)
	I *= 0.0;
      else 
	I *= 2.0/((long double)power[i]+1.0);
    }
    else if (rule[i]==BURK_LAGUERRE) {
      I *= tgammal((long double)(power[i]+1));
    }
    else if (rule[i]==BURK_CHEBYSHEV1) {
      long double bot, top;
      if (!(power[i]%2)) {
	top = 1; bot = 1;
	for (int j=2;j<=power[i];j+=2) {
	  top *= (long double)(j-1);
	  bot *= (long double)j;
	}
	I *= M_PI*top/bot;
      }
      else {
	I *= 0.0;
      }
    }
    else if (rule[i]==BURK_CHEBYSHEV2) {
      long double bot, top;
      if (!(power[i]%2)) {
      top = 1; bot = 1;
      for (int j=2;j<=power[i];j+=2) {
	top *= (long double)(j-1);
	bot *= (long double)j;
      }
      bot *= (long double)(power[i]+2);
      I   *= M_PI*top/bot;
      }
      else {
	I *= 0.0;
      }
    }
    else if (rule[i]==BURK_HERMITE||rule[i]==BURK_GENZKEISTER) {
      if (power[i]%2) {
	I *= 0.0;
      }
      else {  
	long double value = 1.0;
	if ((power[i]-1)>=1) {
	  int n_copy = power[i]-1;
	  while (1<n_copy) {
	    value  *= (long double)n_copy;
	    n_copy -= 2;
	  }
	}
	I *= value*sqrt(M_PI)/powl(2.0,(long double)power[i]/2.0);
      }
    }
  }
  return I;
}

int main(int argc, char *argv[]) {
  
  Teuchos::GlobalMPISession mpiSession(&argc, &argv);

  // This little trick lets us print to std::cout only if
  // a (dummy) command-line argument is provided.
  int iprint     = argc - 1;
  Teuchos::RCP<std::ostream> outStream;
  Teuchos::oblackholestream bhs; // outputs nothing
  if (iprint > 0)
    outStream = Teuchos::rcp(&std::cout, false);
  else
    outStream = Teuchos::rcp(&bhs, false);

  // Save the format state of the original std::cout.
  Teuchos::oblackholestream oldFormatState;
  oldFormatState.copyfmt(std::cout);
 
  *outStream \
  << "===============================================================================\n" \
  << "|                                                                             |\n" \
  << "|                         Unit Test (CubatureTensorSorted)                    |\n" \
  << "|                                                                             |\n" \
  << "|     1) Computing integrals of monomials in 2D                               |\n" \
  << "|                                                                             |\n" \
  << "|  Questions? Contact  Drew Kouri (dpkouri@sandia.gov) or                     |\n" \
  << "|                      Denis Ridzal (dridzal@sandia.gov).                     |\n" \
  << "|                                                                             |\n" \
  << "|  Intrepid's website: http://trilinos.sandia.gov/packages/intrepid           |\n" \
  << "|  Trilinos website:   http://trilinos.sandia.gov                             |\n" \
  << "|                                                                             |\n" \
  << "===============================================================================\n"\
  << "| TEST 12: integrals of monomials in 2D - Anisotropic but no growth rules     |\n"\
  << "===============================================================================\n";


  // internal variables:
  int dimension           = 2;
  int         errorFlag   = 0;
  long double reltol      = 1.0e+02*INTREPID_TOL;
  int         maxDeg      = 0;    
  long double analyticInt = 0;
  long double testInt     = 0;
  int         maxOrder    = 20;
  std::vector<int> power(2,0);
  std::vector<EIntrepidBurkardt> rule1(2,BURK_CLENSHAWCURTIS);
  std::vector<int> order(2,0);

  *outStream << "\nIntegrals of monomials on a reference line (edge):\n";
  // compute and compare integrals
  try {
    for (int i=0; i<=maxOrder; i++) {
      maxDeg = i-2;
      
      order[0] = i; order[1] = i;
      for (int j=0; j <= maxDeg; j++) {
	power[0] = j;
	for (int k=0; k <= maxDeg; k++) {
	  power[1] = k;
	  analyticInt = 2.0*evalInt(dimension, power, rule1);
	  testInt     = evalQuad(power,dimension,order,rule1);
	  
	  long double abstol  = (analyticInt == 0.0 ? reltol : std::fabs(reltol*analyticInt) );
	  long double absdiff = std::fabs(analyticInt - testInt);
	  *outStream << "Cubature order " << std::setw(2) << std::left << i 
		     << " integrating "
		     << "x^" << std::setw(2) << std::left << j << "y^" 
		     << std::setw(2) << std::left 
		     << k <<  ":" << "   " << std::scientific 
		     << std::setprecision(16) << testInt 
		     << "   " << analyticInt << "   " << std::setprecision(4) 
		     << absdiff << "   " 
		     << "<?" << "   " << abstol << "\n";
	  if (absdiff > abstol) {
	    errorFlag++;
	    *outStream << std::right << std::setw(104) << "^^^^---FAILURE!\n";
	  }
	} // end for k
	*outStream << "\n";
      } // end for j
      *outStream << "\n";
    } // end for i
  }
  catch (std::logic_error err) {
    *outStream << err.what() << "\n";
    errorFlag = -1;
  };


  if (errorFlag != 0)
    std::cout << "End Result: TEST FAILED\n";
  else
    std::cout << "End Result: TEST PASSED\n";

  // reset format state of std::cout
  std::cout.copyfmt(oldFormatState);

  return errorFlag;
}
