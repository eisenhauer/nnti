// $Id$ 
// $Source$ 
// @HEADER
// ***********************************************************************
// 
//                           Stokhos Package
//                 Copyright (2008) Sandia Corporation
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
// Questions? Contact Eric T. Phipps (etphipp@sandia.gov).
// 
// ***********************************************************************
// @HEADER

#include "Teuchos_TestForException.hpp"
#include "Stokhos_DynamicArrayTraits.hpp"

template <typename BasisT> 
Stokhos::OrthogPolyExpansion<BasisT>::
OrthogPolyExpansion(const Teuchos::RCP<BasisT>& basis) :
  sz(basis->size()),
  A(2*sz,2*sz),
  B(2*sz,2),
  piv(2*sz),
  Cijk(basis),
  lapack()
{
}

extern "C" {
  double dlange_(char*, int*, int*, double*, int*, double*);
}

template <typename BasisT>
typename Stokhos::OrthogPolyExpansion<BasisT>::ordinal_type
Stokhos::OrthogPolyExpansion<BasisT>::
solve(typename Stokhos::OrthogPolyExpansion<BasisT>::ordinal_type s,
      typename Stokhos::OrthogPolyExpansion<BasisT>::ordinal_type nrhs)
{
  ordinal_type info;
//   lapack.GESV(s, nrhs, A.values(), A.numRows(), &(piv[0]), b.values(), 
// 	      b.numRows(), &info);
  lapack.GETRF(s, s, A.values(), A.numRows(), &(piv[0]), &info);
  value_type norm, rcond;
  std::vector<ordinal_type> iwork(4*s);
  std::vector<value_type> work(4*s);
  norm = 1.0;
  ordinal_type n = A.numRows();
  char t = '1';
  norm = dlange_(&t, &s, &s, A.values(), &n, &work[0]);
  lapack.GECON('1', s, A.values(), A.numRows(), norm, &rcond, &work[0], 
	       &iwork[0], &info);
  std::cout << "condition number = " << 1.0/rcond << std::endl;
  lapack.GETRS('N', s, nrhs, A.values(), A.numRows(), &(piv[0]), B.values(), 
	       B.numRows(), &info);
  return info;
}

template <typename BasisT> 
void
Stokhos::OrthogPolyExpansion<BasisT>::
unaryMinus(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
	   const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  value_type* cc = c.coeff();
  const value_type* ca = a.coeff();
  unsigned int pa = a.size();

  for (unsigned int i=0; i<pa; i++)
    cc[i] = -ca[i];
}

template <typename BasisT> 
void
Stokhos::OrthogPolyExpansion<BasisT>::
plusEqual(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& val)
{
  c[0] += val;
}

template <typename BasisT> 
void
Stokhos::OrthogPolyExpansion<BasisT>::
minusEqual(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& val)
{
  c[0] -= val;
}

template <typename BasisT> 
void
Stokhos::OrthogPolyExpansion<BasisT>::
timesEqual(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& val)
{
  unsigned int pc = c.size();
  value_type* cc = c.coeff();
  for (unsigned int i=0; i<pc; i++)
    cc[i] *= val;
}

template <typename BasisT> 
void
Stokhos::OrthogPolyExpansion<BasisT>::
divideEqual(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& val)
{
  unsigned int pc = c.size();
  value_type* cc = c.coeff();
  for (unsigned int i=0; i<pc; i++)
    cc[i] /= val;
}

template <typename BasisT> 
void
Stokhos::OrthogPolyExpansion<BasisT>::
plusEqual(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
	  const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& x)
{
  unsigned int p = c.size();
  unsigned int xp = x.size();
  unsigned int pmin = xp < p ? xp : p;
  if (xp > p)
    c.resize(xp);

  value_type* cc = c.coeff();
  value_type* xc = x.coeff();
  for (unsigned int i=0; i<pmin; i++)
    cc[i] += xc[i];
  if (p < xp)
    for (unsigned int i=p; i<xp; i++)
      cc[i] = xc[i];
}

template <typename BasisT> 
void
Stokhos::OrthogPolyExpansion<BasisT>::
minusEqual(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
	   const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& x)
{
  unsigned int p = c.size();
  unsigned int xp = x.size();
  unsigned int pmin = xp < p ? xp : p;
  if (xp > p)
    c.resize(xp);

  value_type* cc = c.coeff();
  value_type* xc = x.coeff();
  for (unsigned int i=0; i<pmin; i++)
    cc[i] -= xc[i];
  if (p < xp)
    for (unsigned int i=p; i<xp; i++)
      cc[i] = -xc[i];
}

template <typename BasisT> 
void
Stokhos::OrthogPolyExpansion<BasisT>::
timesEqual(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
	   const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& x)
{
#ifdef STOKHOS_DEBUG
  const char* func = "Stokhos::OrthogPolyExpansion::timesEqual()";
  TEST_FOR_EXCEPTION((x.size() != c.size()) && (x.size() != 1) && 
		     (c.size() != 1), std::logic_error,
		     func << ":  Attempt to assign with incompatible orders");
#endif
  unsigned int p = c.size();
  unsigned int xp = x.size();

  value_type* cc = c.coeff();
  const value_type* xc = x.coeff();
  
  if (p > 1 && xp > 1) {
#ifdef STOKHOS_DEBUG
    TEST_FOR_EXCEPTION(size() < p, std::logic_error,
		       func << ":  Expansion size (" << size() 
		       << ") is too small for computation (" << p 
		       << " needed).");
#endif

    // Copy c coefficients into temporary array
    value_type* tc = Stokhos::ds_array<value_type>::get_and_fill(cc,p);
    value_type tmp;
    for (unsigned int k=0; k<p; k++) {
      tmp = value_type(0.0);
      for (unsigned int i=0; i<p; i++) {
	for (unsigned int j=0; j<xp; j++)
	  tmp += Cijk.triple_value(i,j,k)*tc[i]*xc[j];
      }
      cc[k] = tmp / Cijk.norm_squared(k);
    }
  }
  else if (p > 1) {
    for (unsigned int i=0; i<p; i++)
      cc[i] *= xc[0];
  }
  else  {
    if (xp > p)
      c.resize(xp);
    for (int i=xp-1; i>=0; i--)
	cc[i] = cc[0]*xc[i];
  }
}

template <typename BasisT> 
void
Stokhos::OrthogPolyExpansion<BasisT>::
divideEqual(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
	    const OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& x)
{
  const char* func = "Stokhos::OrthogPolyExpansion::divideEquals()";

#ifdef STOKHOS_DEBUG
  TEST_FOR_EXCEPTION((x.size() != c.size()) && (x.size() != 1) && 
		     (c.size() != 1), std::logic_error,
		     func << ":  Attempt to assign with incompatible sizes");
#endif

  unsigned int p = c.size();
  unsigned int xp = x.size();

  value_type* cc = c.coeff();
  const value_type* xc = x.coeff();
  
  if (xp > 1) {
    if (xp > p)
      c.resize(xp);

#ifdef STOKHOS_DEBUG
    TEST_FOR_EXCEPTION(size() < xp, std::logic_error,
		       func << ":  Expansion size (" << size() 
		       << ") is too small for computation (" << xp
		       << " needed).");
#endif
    
    // Fill A
    for (unsigned int i=0; i<xp; i++) {
      for (unsigned int j=0; j<xp; j++) {
	A(i,j) = 0.0;
	for (unsigned int k=0; k<xp; k++) {
	  A(i,j) += Cijk.triple_value(i,j,k)*xc[k];
	}
	A(i,j) /= Cijk.norm_squared(i);
      }
    }
    
    // Fill B
    for (unsigned int i=0; i<xp; i++)
      B(i,0) = cc[i];

    // Solve system
    int info = solve(xp, 1);

    TEST_FOR_EXCEPTION(info < 0, std::logic_error,
		       func << ":  Argument " << info 
		            << " for solve had illegal value");
    TEST_FOR_EXCEPTION(info > 0, std::logic_error,
		       func << ":  Diagonal entry " << info 
		            << " in LU factorization is exactly zero");

    // Get coefficients
    for (unsigned int i=0; i<xp; i++)
      cc[i] = B(i,0);
  }
  else {
    for (unsigned int i=0; i<p; i++)
      cc[i] /= xc[0];
  }
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
plus(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
#ifdef STOKHOS_DEBUG
  const char* func = "Stokhos::OrthogPolyExpansion::plus()";
  TEST_FOR_EXCEPTION((a.size() != b.size()) && (a.size() != 1) && 
		     (b.size() != 1), 
		     std::logic_error,
		     func << ":  Arguments have incompatible sizes!");
#endif

  unsigned int pa = a.size();
  unsigned int pb = b.size();
  unsigned int pc = pa > pb ? pa : pb;
  if (pc != c.size())
    c.resize(pc);

  const value_type* ca = a.coeff();
  const value_type* cb = b.coeff();
  value_type* cc = c.coeff();

  if (pa > 1 && pb > 1) {
    for (unsigned int i=0; i<pc; i++)
      cc[i] = ca[i] + cb[i];
  }
  else if (pa > 1) {
    cc[0] = ca[0] + cb[0];
    for (unsigned int i=1; i<pc; i++)
      cc[i] = ca[i];
  }
  else if (pb >= 1) {
    cc[0] = ca[0] + cb[0];
    for (unsigned int i=1; i<pc; i++)
      cc[i] = cb[i];
  }
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
plus(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
     const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& a, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  unsigned int pc = b.size();
  if (pc != c.size())
    c.resize(pc);

  const value_type* cb = b.coeff();
  value_type* cc = c.coeff();

  cc[0] = a + cb[0];
  for (unsigned int i=1; i<pc; i++)
    cc[i] = cb[i];
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
plus(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, 
     const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& b)
{
  unsigned int pc = a.size();
  if (pc != c.size())
    c.resize(pc);

  const value_type* ca = a.coeff();
  value_type* cc = c.coeff();

  cc[0] = ca[0] + b;
  for (unsigned int i=1; i<pc; i++)
    cc[i] = ca[i];
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
minus(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
#ifdef STOKHOS_DEBUG
  const char* func = "Stokhos::OrthogPolyExpansion::minus()";
  TEST_FOR_EXCEPTION((a.size() != b.size()) && (a.size() != 1) && 
		     (b.size() != 1), 
		     std::logic_error,
		     func << ":  Arguments have incompatible sizes!");
#endif

  unsigned int pa = a.size();
  unsigned int pb = b.size();
  unsigned int pc = pa > pb ? pa : pb;
  if (pc != c.size())
    c.resize(pc);

  const value_type* ca = a.coeff();
  const value_type* cb = b.coeff();
  value_type* cc = c.coeff();

  if (pa > 1 && pb > 1) {
    for (unsigned int i=0; i<pc; i++)
      cc[i] = ca[i] - cb[i];
  }
  else if (pa > 1) {
    cc[0] = ca[0] - cb[0];
    for (unsigned int i=1; i<pc; i++)
      cc[i] = ca[i];
  }
  else if (pb >= 1) {
    cc[0] = ca[0] - cb[0];
    for (unsigned int i=1; i<pc; i++)
      cc[i] = -cb[i];
  }
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
minus(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
      const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& a, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  unsigned int pc = b.size();
  if (pc != c.size())
    c.resize(pc);

  const value_type* cb = b.coeff();
  value_type* cc = c.coeff();

  cc[0] = a - cb[0];
  for (unsigned int i=1; i<pc; i++)
    cc[i] = -cb[i];
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
minus(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& b)
{
  unsigned int pc = a.size();
  if (pc != c.size())
    c.resize(pc);

  const value_type* ca = a.coeff();
  value_type* cc = c.coeff();

  cc[0] = ca[0] - b;
  for (unsigned int i=1; i<pc; i++)
    cc[i] = ca[i];

  return c;
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
times(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
#ifdef STOKHOS_DEBUG
  const char* func = "Stokhos::OrthogPolyExpansion::times()";
  TEST_FOR_EXCEPTION((a.size() != b.size()) && (a.size() != 1) && 
		     (b.size() != 1), 
		     std::logic_error,
		     func << ":  Arguments have incompatible sizes!");
#endif

  unsigned int pa = a.size();
  unsigned int pb = b.size();
  unsigned int pc = pa > pb ? pa : pb;
  if (pc != c.size())
    c.resize(pc);

  const value_type* ca = a.coeff();
  const value_type* cb = b.coeff();
  value_type* cc = c.coeff();

  if (pa > 1 && pb > 1) {
#ifdef STOKHOS_DEBUG
    TEST_FOR_EXCEPTION(size() < pc, std::logic_error,
		       func << ":  Expansion size (" << size()
		       << ") is too small for computation (" << pc
		       << " needed).");
#endif

    value_type tmp;
    for (unsigned int k=0; k<pc; k++) {
      tmp = value_type(0.0);
      for (unsigned int i=0; i<pa; i++) {
	for (unsigned int j=0; j<pb; j++) {
	  tmp += Cijk.triple_value(i,j,k)*ca[i]*cb[j];
	}
      }
      cc[k] = tmp / Cijk.norm_squared(k);
    }
  }
  else if (pa > 1) {
    for (unsigned int i=0; i<pc; i++)
      cc[i] = ca[i]*cb[0];
  }
  else if (pb >= 1) {
    for (unsigned int i=0; i<pc; i++)
      cc[i] = ca[0]*cb[i];
  }
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
times(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
      const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& a, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  unsigned int pc = b.size();
  if (pc != c.size())
    c.resize(pc);

  const value_type* cb = b.coeff();
  value_type* cc = c.coeff();

  for (unsigned int i=0; i<pc; i++)
    cc[i] = a*cb[i];
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
times(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, 
      const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& b)
{
  unsigned int pc = a.size();
  if (pc != c.size())
    c.resize(pc);

  const value_type* ca = a.coeff();
  value_type* cc = c.coeff();

  for (unsigned int i=0; i<pc; i++)
    cc[i] = ca[i]*b;

  return c;
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
divide(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
       const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, 
       const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  const char* func = "Stokhos::OrthogPolyExpansion::divide()";

#ifdef STOKHOS_DEBUG
  TEST_FOR_EXCEPTION((a.size() != b.size()) && (a.size() != 1) && 
		     (b.size() != 1), 
		     std::logic_error,
		     func << ":  Arguments have incompatible sizes!");
#endif

  unsigned int pa = a.size();
  unsigned int pb = b.size();
  unsigned int pc = pa > pb ? pa : pb;
  if (pc != c.size())
    c.resize(pc);

  const value_type* ca = a.coeff();
  const value_type* cb = b.coeff();
  value_type* cc = c.coeff();

  if (pb > 1) {

#ifdef STOKHOS_DEBUG
    TEST_FOR_EXCEPTION(size() < pc, std::logic_error,
		       func << ":  Expansion size (" << size() 
		       << ") is too small for computation (" << pc
		       << " needed).");
#endif

    // Fill A
    for (unsigned int i=0; i<pc; i++) {
      for (unsigned int j=0; j<pc; j++) {
	A(i,j) = 0.0;
	for (unsigned int k=0; k<pb; k++) {
	  A(i,j) += Cijk.triple_value(i,j,k)*cb[k];
	}
	A(i,j) /= Cijk.norm_squared(i);
      }
    }
    
    // Fill B
    for (unsigned int i=0; i<pa; i++)
      B(i,0) = ca[i];
    for (unsigned int i=pa; i<pc; i++)
      B(i,0) = value_type(0.0);

    // Solve system
    int info = solve(pc, 1);

    TEST_FOR_EXCEPTION(info < 0, std::logic_error,
		       func << ":  Argument " << info 
		            << " for solve had illegal value");
    TEST_FOR_EXCEPTION(info > 0, std::logic_error,
		       func << ":  Diagonal entry " << info 
		            << " in LU factorization is exactly zero");

    // Get coefficients
    for (unsigned int i=0; i<pc; i++)
      cc[i] = B(i,0);
  }
  else {
    for (unsigned int i=0; i<pa; i++)
      cc[i] = ca[i]/cb[0];
  }
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
divide(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
       const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& a, 
       const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  const char* func = "Stokhos::OrthogPolyExpansion::divide()";

  unsigned int pc = b.size();
  if (pc != c.size())
    c.resize(pc);

  const value_type* cb = b.coeff();
  value_type* cc = c.coeff();

  if (pc > 1) {
#ifdef STOKHOS_DEBUG
    TEST_FOR_EXCEPTION(size() < pc, std::logic_error,
		       func << ":  Expansion size (" << size()
		       << ") is too small for computation (" << pc
		       << " needed).");
#endif
   
    // Fill A
    for (unsigned int i=0; i<pc; i++) {
      for (unsigned int j=0; j<pc; j++) {
	A(i,j) = 0.0;
	for (unsigned int k=0; k<pc; k++) {
	  A(i,j) += Cijk.triple_value(i,j,k)*cb[k];
	}
	A(i,j) /= Cijk.norm_squared(i);
      }
    }

    // Fill B
    B(0,0) = a;
    for (unsigned int i=1; i<pc; i++)
      B(i,0) = value_type(0.0);

    // Solve system
    int info = solve(pc, 1);

    TEST_FOR_EXCEPTION(info < 0, std::logic_error,
		       func << ":  Argument " << info 
		            << " for solve had illegal value");
    TEST_FOR_EXCEPTION(info > 0, std::logic_error,
		       func << ":  Diagonal entry " << info 
		            << " in LU factorization is exactly zero");

    // Get coefficients
    for (unsigned int i=0; i<pc; i++)
      cc[i] = B(i,0);
  }
  else 
    cc[0] = a / cb[0];
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
divide(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
       const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, 
       const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& b)
{
  unsigned int pc = a.size();
  if (pc != c.size())
    c.resize(pc);

  const value_type* ca = a.coeff();
  value_type* cc = c.coeff();

  for (unsigned int i=0; i<pc; i++)
    cc[i] = ca[i]/b;
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
exp(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  const char* func = "Stokhos::OrthogPolyExpansion::exp()";

  unsigned int pc = a.size();
  if (pc != c.size())
    c.resize(pc);

#ifdef STOKHOS_DEBUG
  TEST_FOR_EXCEPTION(size() < pc, std::logic_error,
		     func << ":  Expansion size (" << size() 
		     << ") is too small for computation (" << pc
		     << " needed).");
#endif

  const value_type* ca = a.coeff();
  value_type* cc = c.coeff();

  const typename tp_type::basis_type& basis = Cijk.getBasis();
  value_type psi_0 = basis.evaluateZero(0);

  // Fill A and B
  for (unsigned int i=1; i<pc; i++) {
    B(i-1,0) = 0.0;
    for (unsigned int j=1; j<pc; j++) {
      A(i-1,j-1) = Cijk.double_deriv(i-1,j);
      for (unsigned int k=1; k<pc; k++)
	A(i-1,j-1) -= ca[k]*Cijk.triple_deriv(i-1,j,k);
      B(i-1,0) += ca[j]*Cijk.double_deriv(i-1,j);
    }
    B(i-1,0) *= psi_0;
  }

  // Solve system
  int info = solve(pc-1, 1);

  TEST_FOR_EXCEPTION(info < 0, std::logic_error,
		     func << ":  Argument " << info 
		     << " for solve had illegal value");
  TEST_FOR_EXCEPTION(info > 0, std::logic_error,
		     func << ":  Diagonal entry " << info 
		     << " in LU factorization is exactly zero");

  // Compute order-0 coefficient
  value_type s = psi_0 * ca[0];
  value_type t = psi_0;
  for (unsigned int i=1; i<pc; i++) {
    s += basis.evaluateZero(i) * ca[i];
    t += basis.evaluateZero(i) * B(i-1,0);
  }
  s = std::exp(s);
  cc[0] = (s/t);

  // Compute remaining coefficients
  for (unsigned int i=1; i<pc; i++)
    cc[i] = B(i-1,0) * cc[0];
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
log(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
   const char* func = "Stokhos::OrthogPolyExpansion::log()";

  unsigned int pc = a.size();
  if (pc != c.size())
    c.resize(pc);

#ifdef STOKHOS_DEBUG
  TEST_FOR_EXCEPTION(size() < pc, std::logic_error,
		     func << ":  Expansion size (" << size() 
		     << ") is too small for computation (" << pc
		     << " needed).");
#endif

  const value_type* ca = a.coeff();
  value_type* cc = c.coeff();

  const typename tp_type::basis_type& basis = Cijk.getBasis();
  value_type psi_0 = basis.evaluateZero(0);

  // Fill A and B
  for (unsigned int i=1; i<pc; i++) {
    B(i-1,0) = 0.0;
    for (unsigned int j=1; j<pc; j++) {
      A(i-1,j-1) = 0.0;
      for (unsigned int k=0; k<pc; k++)
	A(i-1,j-1) += ca[k]*Cijk.triple_deriv(i-1,k,j);
      B(i-1,0) += ca[j]*Cijk.double_deriv(i-1,j); 
    }
  }

  // Solve system
  int info = solve(pc-1, 1);

  TEST_FOR_EXCEPTION(info < 0, std::logic_error,
		     func << ":  Argument " << info 
		     << " for solve had illegal value");
  TEST_FOR_EXCEPTION(info > 0, std::logic_error,
		     func << ":  Diagonal entry " << info 
		     << " in LU factorization is exactly zero");

  // Compute order-0 coefficient
  value_type s = psi_0 * ca[0];
  value_type t = value_type(0.0);
  for (unsigned int i=1; i<pc; i++) {
    s += basis.evaluateZero(i) * ca[i];
    t += basis.evaluateZero(i) * B(i-1,0);
  }
  cc[0] = (std::log(s) - t) / psi_0;

  // Compute remaining coefficients
  for (unsigned int i=1; i<pc; i++)
    cc[i] = B(i-1,0);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
log10(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  log(c,a);
  divide(c,a,std::log(10.0));
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
sqrt(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  log(c,a);
  timesEqual(c,value_type(0.5));
  exp(c,c);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
pow(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a,
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  log(c,a);
  timesEqual(c,b);
  exp(c,c);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
pow(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& a, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  times(c,std::log(a),b);
  exp(c,c);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
pow(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, 
    const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& b)
{
  log(c,a);
  timesEqual(c,b);
  exp(c,c);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
sincos(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& s, 
       Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
       const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  const char* func = "Stokhos::OrthogPolyExpansion::sincos()";
  unsigned int pc = a.size();

#ifdef STOKHOS_DEBUG
  TEST_FOR_EXCEPTION(size() < pc, std::logic_error,
		     func << ":  Expansion size (" << size() 
		     << ") is too small for computation (" << pc 
		     << " needed).");
#endif

  if (s.size() != pc)
    s.resize(pc);
  if (c.size() != pc)
    c.resize(pc);

  const value_type* ca = a.coeff();
  value_type* cs = s.coeff();
  value_type* cc = c.coeff();

  const typename tp_type::basis_type& basis = Cijk.getBasis();
  value_type psi_0 = basis.evaluateZero(0);
  unsigned int offset = pc-1;

  // Fill A and b
  B.putScalar(value_type(0.0));
  value_type tmp, tmp2;
  for (unsigned int i=1; i<pc; i++) {
    tmp2 = value_type(0.0);
    for (unsigned int j=1; j<pc; j++) {
      tmp = Cijk.double_deriv(i-1,j);
      A(i-1,j-1) = tmp;
      A(i-1+offset,j-1+offset) = tmp;
      tmp = value_type(0.0);
      for (unsigned int k=1; k<pc; k++)
         tmp += ca[k]*Cijk.triple_deriv(i-1,j,k);
      A(i-1+offset,j-1) = tmp;
      A(i-1,j-1+offset) = -tmp;
      tmp2 += ca[j]*Cijk.double_deriv(i-1,j);
    }
    B(i-1,0) = tmp2*psi_0;
    B(i-1+offset,1) = tmp2*psi_0;
  }

  // Solve system
  int info = solve(2*pc-2, 2);

  TEST_FOR_EXCEPTION(info < 0, std::logic_error,
		     func << ":  Argument " << info 
		     << " for solve had illegal value");
  TEST_FOR_EXCEPTION(info > 0, std::logic_error,
		     func << ":  Diagonal entry " << info 
		     << " in LU factorization is exactly zero");

  // Compute order-0 coefficients
  value_type t = psi_0 * ca[0];
  value_type a00 = psi_0;
  value_type a01 = value_type(0.0);
  value_type a10 = value_type(0.0);
  value_type a11 = psi_0;
  value_type b0 = B(0,0);
  value_type b1 = B(1,0);
  for (unsigned int i=1; i<pc; i++) {
    t += basis.evaluateZero(i) * ca[i];
    a00 -= basis.evaluateZero(i) * B(i-1,1);
    a01 += basis.evaluateZero(i) * B(i-1,0);
    a10 -= basis.evaluateZero(i) * B(i-1+offset,1);
    a11 += basis.evaluateZero(i) * B(i-1+offset,0);
  }
  A(0,0) = a00;
  A(0,1) = a01;
  A(1,0) = a10;
  A(1,1) = a11;
  B(0,0) = std::sin(t);
  B(1,0) = std::cos(t);

  info = solve(2, 1);

  TEST_FOR_EXCEPTION(info < 0, std::logic_error,
		     func << ":  Argument " << info 
		     << " for (2x2) solve had illegal value");
  TEST_FOR_EXCEPTION(info > 0, std::logic_error,
		     func << ":  Diagonal entry " << info 
		     << " in (2x2) LU factorization is exactly zero");
  cs[0] = B(0,0);
  cc[0] = B(1,0);

  // Compute remaining coefficients
  B(0,0) = b0;
  B(1,0) = b1;
  for (unsigned int i=1; i<pc; i++) {
    cs[i] = cc[0]*B(i-1,0) - cs[0]*B(i-1,1);
    cc[i] = cc[0]*B(i-1+offset,0) - cs[0]*B(i-1+offset,1);
  }
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
sin(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& s, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  OrthogPolyApprox<value_type> c(s);
  sincos(s, c, a);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
cos(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  OrthogPolyApprox<value_type> s(c);
  sincos(s, c, a);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
tan(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& t, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  OrthogPolyApprox<value_type> c(t);
  
  sincos(t, c, a);
  divideEqual(t,c);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
sinhcosh(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& s, 
	 Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
	 const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  const char* func = "Stokhos::OrthogPolyExpansion::sinhcosh()";
  unsigned int pc = a.size();

#ifdef STOKHOS_DEBUG
  TEST_FOR_EXCEPTION(size() < pc, std::logic_error,
		     func << ":  Expansion size (" << size() 
		     << ") is too small for computation (" << pc
		     << " needed).");
#endif

  if (s.size() != pc)
    s.resize(pc);
  if (c.size() != pc)
    c.resize(pc);

  const value_type* ca = a.coeff();
  value_type* cs = s.coeff();
  value_type* cc = c.coeff();

  const typename tp_type::basis_type& basis = Cijk.getBasis();
  value_type psi_0 = basis.evaluateZero(0);
  unsigned int offset = pc-1;

  // Fill A and b
  B.putScalar(value_type(0.0));
  value_type tmp, tmp2;
  for (unsigned int i=1; i<pc; i++) {
    tmp2 = value_type(0.0);
    for (unsigned int j=1; j<pc; j++) {
      tmp = Cijk.double_deriv(i-1,j);
      A(i-1,j-1) = tmp;
      A(i-1+offset,j-1+offset) = tmp;
      tmp = value_type(0.0);
      for (unsigned int k=1; k<pc; k++)
         tmp += ca[k]*Cijk.triple_deriv(i-1,j,k);
      A(i-1+offset,j-1) = -tmp;
      A(i-1,j-1+offset) = -tmp;
      tmp2 += ca[j]*Cijk.double_deriv(i-1,j);
    }
    B(i-1,0) = tmp2*psi_0;
    B(i-1+offset,1) = tmp2*psi_0;
  }

  // Solve system
  int info = solve(2*pc-2, 2);

  TEST_FOR_EXCEPTION(info < 0, std::logic_error,
		     func << ":  Argument " << info 
		     << " for solve had illegal value");
  TEST_FOR_EXCEPTION(info > 0, std::logic_error,
		     func << ":  Diagonal entry " << info 
		     << " in LU factorization is exactly zero");

  // Compute order-0 coefficients
  value_type t = psi_0 * ca[0];
  value_type a00 = psi_0;
  value_type a01 = value_type(0.0);
  value_type a10 = value_type(0.0);
  value_type a11 = psi_0;
  value_type b0 = B(0,0);
  value_type b1 = B(1,0);
  for (unsigned int i=1; i<pc; i++) {
    t += basis.evaluateZero(i) * ca[i];
    a00 += basis.evaluateZero(i) * B(i-1,1);
    a01 += basis.evaluateZero(i) * B(i-1,0);
    a10 += basis.evaluateZero(i) * B(i-1+offset,1);
    a11 += basis.evaluateZero(i) * B(i-1+offset,0);
  }
  A(0,0) = a00;
  A(0,1) = a01;
  A(1,0) = a10;
  A(1,1) = a11;
  B(0,0) = std::sinh(t);
  B(1,0) = std::cosh(t);
  info = solve(2, 1);
  TEST_FOR_EXCEPTION(info < 0, std::logic_error,
		     func << ":  Argument " << info 
		     << " for (2x2) solve had illegal value");
  TEST_FOR_EXCEPTION(info > 0, std::logic_error,
		     func << ":  Diagonal entry " << info 
		     << " in (2x2) LU factorization is exactly zero");
  cs[0] = B(0,0);
  cc[0] = B(1,0);

  // Compute remaining coefficients
  B(0,0) = b0;
  B(1,0) = b1;
  for (unsigned int i=1; i<pc; i++) {
    cs[i] = cc[0]*B(i-1,0) + cs[0]*B(i-1,1);
    cc[i] = cc[0]*B(i-1+offset,0) + cs[0]*B(i-1+offset,1);
  }
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
sinh(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& s, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  OrthogPolyApprox<value_type> c(s);
  sinhcosh(s, c, a);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
cosh(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  OrthogPolyApprox<value_type> s(c);
  sinhcosh(s, c, a);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
tanh(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& t, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  OrthogPolyApprox<value_type> c(t);
  
  sinhcosh(t, c, a);
  divideEqual(t,c);
}

template <typename BasisT>
template <typename OpT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
quad(const OpT& quad_func,
     Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a,
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  const char* func = "Stokhos::OrthogPolyExpansion::quad()";
  unsigned int pc = a.size();
  if (pc != c.size())
    c.resize(pc);

  const value_type* ca = a.coeff();
  const value_type* cb = b.coeff();
  value_type* cc = c.coeff();

#ifdef STOKHOS_DEBUG
  TEST_FOR_EXCEPTION(size() < pc, std::logic_error,
		     func << ":  Expansion size (" << size()
		     << ") is too small for computation (" << pc
		     << " needed).");
#endif

  const typename tp_type::basis_type& basis = Cijk.getBasis();
  value_type psi_0 = basis.evaluateZero(0);
    
  // Fill A and B
  for (unsigned int i=1; i<pc; i++) {
    B(i-1,0) = 0.0;
    for (unsigned int j=1; j<pc; j++) {
      A(i-1,j-1) = 0.0;
      for (unsigned int k=0; k<pc; k++)
	A(i-1,j-1) += cb[k]*Cijk.triple_deriv(i-1,k,j);
      B(i-1,0) += ca[j]*Cijk.double_deriv(i-1,j);
    }
  }
  
  // Solve system
  int info = solve(pc-1, 1);

  TEST_FOR_EXCEPTION(info < 0, std::logic_error,
		     func << ":  Argument " << info 
		     << " for solve had illegal value");
  TEST_FOR_EXCEPTION(info > 0, std::logic_error,
		     func << ":  Diagonal entry " << info 
		     << " in LU factorization is exactly zero");

  // Compute order-0 coefficient
  value_type s = psi_0 * ca[0];
  value_type t = value_type(0.0);
  for (unsigned int i=1; i<pc; i++) {
    s += basis.evaluateZero(i) * ca[i];
    t += basis.evaluateZero(i) * B(i-1,0);
  }
  cc[0] = (quad_func(s) - t) / psi_0;

  // Get coefficients
  for (unsigned int i=1; i<pc; i++)
    cc[i] = B(i-1,0);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
acos(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  times(c,a,a);
  minus(c,value_type(1.0),c);
  sqrt(c,c);
  timesEqual(c,value_type(-1.0));
  quad(acos_quad_func(), c, a, c);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
asin(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  times(c,a,a);
  minus(c,value_type(1.0),c);
  sqrt(c,c);
  quad(asin_quad_func(), c, a, c);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
atan(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  times(c,a,a);
  plusEqual(c,value_type(1.0));
  quad(atan_quad_func(), c, a, c);
}

// template <typename BasisT>
// Hermite<value_type>
// atan2(const Hermite<value_type>& a,
//       const Hermite<value_type>& b)
// {
//   Hermite<value_type> c = atan(a/b);
//   c.fastAccessCoeff(0) = atan2(a.coeff(0),b.coeff(0));
// }

// template <typename BasisT>
// Hermite<value_type>
// atan2(const T& a,
//       const Hermite<value_type>& b)
// {
//   Hermite<value_type> c = atan(a/b);
//   c.fastAccessCoeff(0) = atan2(a,b.coeff(0));
// }

// template <typename BasisT>
// Hermite<value_type>
// atan2(const Hermite<value_type>& a,
//       const T& b)
// {
//   Hermite<value_type> c = atan(a/b);
//   c.fastAccessCoeff(0) = atan2(a.coeff(0),b);
// }

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
acosh(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  times(c,a,a);
  minusEqual(c,value_type(1.0));
  sqrt(c,c);
  quad(acosh_quad_func(), c, a, c);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
asinh(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  times(c,a,a);
  plusEqual(c,value_type(1.0));
  sqrt(c,c);
  quad(asinh_quad_func(), c, a, c);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
atanh(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
      const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  times(c,a,a);
  minus(c,value_type(1.0),c);
  quad(atanh_quad_func(), c, a, c);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
fabs(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
     const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  if (a[0] >= 0)
    c = a;
  else
    unaryMinus(c,a);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
abs(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a)
{
  if (a[0] >= 0)
    c = a;
  else
    unaryMinus(c,a);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
max(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a,
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  if (a[0] >= b[0])
    c = a;
  else
    c = b;
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
max(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& a, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  if (a >= b[0])
    c = OrthogPolyApprox<value_type>(b.size(), a);
  else
    c = b;
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
max(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, 
    const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& b)
{
  if (a[0] >= b)
    c = a;
  else
    c = OrthogPolyApprox<value_type>(a.size(), b);
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
min(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a,
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  if (a[0] <= b[0])
    return c = a;
  else
    return c = b;
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
min(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& a, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& b)
{
  if (a <= b[0])
    c = OrthogPolyApprox<value_type>(b.size(), a);
  else
    c = b;
}

template <typename BasisT>
void
Stokhos::OrthogPolyExpansion<BasisT>::
min(Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& c, 
    const Stokhos::OrthogPolyApprox<typename Stokhos::OrthogPolyExpansion<BasisT>::value_type >& a, 
    const typename Stokhos::OrthogPolyExpansion<BasisT>::value_type& b)
{
  if (a[0] <= b)
    c = a;
  else
    c = OrthogPolyApprox<value_type>(a.size(), b);
}
