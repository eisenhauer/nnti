// @HEADER
// ***********************************************************************
// 
//                    Teuchos: Common Tools Package
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

// Kris
// 06.18.03 -- Minor formatting changes
//          -- Changed calls to LAPACK objects to use new <OType, SType> templates
// 07.08.03 -- Move into Teuchos package/namespace
// 07.11.03 -- Added ScalarTraits for ARPREC mp_real
// 07.14.03 -- Fixed int rand() function (was set up to return a floating-point style random number)
// 07.17.03 -- Added squareroot() function

// NOTE: Before adding specializations of ScalarTraits, make sure that they do not duplicate 
// specializations already present in PyTrilinos (see packages/PyTrilinos/src/Teuchos_Traits.i)

// NOTE: halfPrecision and doublePrecision are not currently implemented for ARPREC, GMP or the ordinal types (e.g., int, char)

#ifndef _TEUCHOS_SCALARTRAITS_HPP_
#define _TEUCHOS_SCALARTRAITS_HPP_

/*! \file Teuchos_ScalarTraits.hpp
    \brief Defines basic traits for the scalar field type.
*/
 
#include "Teuchos_ConfigDefs.hpp"

#ifdef HAVE_TEUCHOS_ARPREC
#include <arprec/mp_real.h>
#endif

#ifdef HAVE_TEUCHOS_QD
#include <qd/qd_real.h>
#include <qd/dd_real.h>
#endif

#ifdef HAVE_TEUCHOS_GNU_MP
#include <gmp.h>
#include <gmpxx.h>
#endif

/*! \struct Teuchos::ScalarTraits
    \brief This structure defines some basic traits for a scalar field type.

    Scalar traits are an essential part of templated codes.  This structure offers
    the basic traits of the templated scalar type, like defining zero and one,
    and basic functions on the templated scalar type, like performing a square root.

    The functions in the templated base unspecialized struct are designed not to
    compile (giving a nice compile-time error message) and therefore specializations
    must be written for Scalar types actually used.

    \note 
     <ol>
       <li> The default defined specializations are provided for \c int, \c float, and \c double.
     	 <li> ScalarTraits can be used with the Arbitrary Precision Library ( \c http://crd.lbl.gov/~dhbailey/mpdist/ )
            by configuring Teuchos with \c --enable-teuchos-arprec and giving the appropriate paths to ARPREC.
            Then ScalarTraits has the specialization: \c mp_real.
     	 <li> If Teuchos is configured with \c --enable-teuchos-std::complex then ScalarTraits also has
            a parital specialization for all std::complex numbers of the form <tt>std::complex<T></tt>.
     </ol>
*/

/* This is the default structure used by ScalarTraits<T> to produce a compile time
	error when the specialization does not exist for type <tt>T</tt>.
*/
namespace Teuchos {

template<class T>
struct UndefinedScalarTraits
{
  //! This function should not compile if there is an attempt to instantiate!
  static inline T notDefined() { return T::this_type_is_missing_a_specialization(); }
};

template<class T>
struct ScalarTraits
{
  //! Mandatory typedef for result of magnitude
  typedef T magnitudeType;
  //! Typedef for half precision
  typedef T halfPrecision;
  //! Typedef for double precision
  typedef T doublePrecision;
  //! Determines if scalar type is std::complex
  static const bool isComplex = false;
  //! Determines if scalar type is an ordinal type
  static const bool isOrdinal = false;
  //! Determines if scalar type supports relational operators such as <, >, <=, >=.
  static const bool isComparable = false;
  //! Determines if scalar type have machine-specific parameters (i.e. eps(), sfmin(), base(), prec(), t(), rnd(), emin(), rmin(), emax(), rmax() are supported)
  static const bool hasMachineParameters = false;
  //! Returns relative machine precision.
  static inline magnitudeType eps()   { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns safe minimum (sfmin), such that 1/sfmin does not overflow.
  static inline magnitudeType sfmin() { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the base of the machine.
  static inline magnitudeType base()  { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns \c eps*base.
  static inline magnitudeType prec()  { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the number of (base) digits in the mantissa.
  static inline magnitudeType t()     { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns 1.0 when rounding occurs in addition, 0.0 otherwise
  static inline magnitudeType rnd()   { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the minimum exponent before (gradual) underflow.
  static inline magnitudeType emin()  { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the underflow threshold - \c base^(emin-1)
  static inline magnitudeType rmin()  { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the largest exponent before overflow.
  static inline magnitudeType emax()  { return UndefinedScalarTraits<T>::notDefined(); }
  //! Overflow theshold - \c (base^emax)*(1-eps)
  static inline magnitudeType rmax()  { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the magnitudeType of the scalar type \c a.
  static inline magnitudeType magnitude(T a) { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns representation of zero for this scalar type.
  static inline T zero()                     { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns representation of one for this scalar type.
  static inline T one()                      { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the real part of the scalar type \c a.
  static inline magnitudeType real(T a) { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the imaginary part of the scalar type \c a.
  static inline magnitudeType imag(T a) { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the conjugate of the scalar type \c a.
  static inline T conjugate(T a) { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns a number that represents NaN.
  static inline T nan()                      { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns <tt>true</tt> if <tt>x</tt> is NaN or Inf.
  static inline bool isnaninf(const T& x)     { return UndefinedScalarTraits<T>::notDefined(); }
  //! Seed the random number generator returned by <tt>random()</tt>.
  static inline void seedrandom(unsigned int s) { int i; T t = &i; }
  //! Returns a random number (between -one() and +one()) of this scalar type.
  static inline T random()                   { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the name of this scalar type.
  static inline std::string name()           { (void)UndefinedScalarTraits<T>::notDefined(); return 0; }
  //! Returns a number of magnitudeType that is the square root of this scalar type \c x. 
  static inline T squareroot(T x) { return UndefinedScalarTraits<T>::notDefined(); }
  //! Returns the result of raising one scalar \c x to the power \c y.
  static inline T pow(T x, T y) { return UndefinedScalarTraits<T>::notDefined(); }
};
  
#ifndef DOXYGEN_SHOULD_SKIP_THIS


void throwScalarTraitsNanInfError( const std::string &errMsg );


#define TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR( VALUE, MSG ) \
  if (isnaninf(VALUE)) { \
    std::ostringstream omsg; \
    omsg << MSG; \
    Teuchos::throwScalarTraitsNanInfError(omsg.str());	\
  }


template<>
struct ScalarTraits<char>
{
  typedef char magnitudeType;
  typedef char halfPrecision;
  typedef char doublePrecision;
  static const bool isComplex = false;
  static const bool isOrdinal = true;
  static const bool isComparable = true;
  static const bool hasMachineParameters = false;
  // Not defined: eps(), sfmin(), base(), prec(), t(), rnd(), emin(), rmin(), emax(), rmax()
  static inline magnitudeType magnitude(char a) { return static_cast<char>(std::fabs(static_cast<double>(a))); }
  static inline char zero()  { return 0; }
  static inline char one()   { return 1; }
  static inline char conjugate(char x) { return x; }
  static inline char real(char x) { return x; }
  static inline char imag(char) { return 0; }
  static inline bool isnaninf(char ) { return false; }
  static inline void seedrandom(unsigned int s) { 
    std::srand(s); 
#ifdef __APPLE__
    // throw away first random number to address bug 3655
    // http://software.sandia.gov/bugzilla/show_bug.cgi?id=3655
    random();
#endif
  }
  //static inline char random() { return (-1 + 2*rand()); } // RAB: This version should be used to be consistent with others
  static inline char random() { return std::rand(); } // RAB: This version should be used for an unsigned char, not char
  static inline std::string name() { return "char"; }
  static inline char squareroot(char x) { return (char) std::sqrt((double) x); }
  static inline char pow(char x, char y) { return (char) std::pow((double)x,(double)y); }
};

template<>
struct ScalarTraits<short int>
{
  typedef short int magnitudeType;
  typedef short int halfPrecision;
  typedef short int doublePrecision;
  static const bool isComplex = false;
  static const bool isOrdinal = true;
  static const bool isComparable = true;
  static const bool hasMachineParameters = false;
  // Not defined: eps(), sfmin(), base(), prec(), t(), rnd(), emin(), rmin(), emax(), rmax()
  static inline magnitudeType magnitude(short int a) { return static_cast<short int>(std::fabs(static_cast<double>(a))); }
  static inline short int zero()  { return 0; }
  static inline short int one()   { return 1; }
  static inline short int conjugate(short int x) { return x; }
  static inline short int real(short int x) { return x; }
  static inline short int imag(short int) { return 0; }
  static inline void seedrandom(unsigned int s) { 
    std::srand(s); 
#ifdef __APPLE__
    // throw away first random number to address bug 3655
    // http://software.sandia.gov/bugzilla/show_bug.cgi?id=3655
    random();
#endif
  }
  //static inline int random() { return (-1 + 2*rand()); }  // RAB: This version should be used to be consistent with others
  static inline short int random() { return std::rand(); }             // RAB: This version should be used for an unsigned int, not int
  static inline std::string name() { return "short int"; }
  static inline short int squareroot(short int x) { return (short int) std::sqrt((double) x); }
  static inline short int pow(short int x, short int y) { return (short int) std::pow((double)x,(double)y); }
};

template<>
struct ScalarTraits<int>
{
  typedef int magnitudeType;
  typedef int halfPrecision;
  typedef int doublePrecision;
  static const bool isComplex = false;
  static const bool isOrdinal = true;
  static const bool isComparable = true;
  static const bool hasMachineParameters = false;
  // Not defined: eps(), sfmin(), base(), prec(), t(), rnd(), emin(), rmin(), emax(), rmax()
  static inline magnitudeType magnitude(int a) { return static_cast<int>(std::fabs(static_cast<double>(a))); }
  static inline int zero()  { return 0; }
  static inline int one()   { return 1; }
  static inline int conjugate(int x) { return x; }
  static inline int real(int x) { return x; }
  static inline int imag(int) { return 0; }
  static inline bool isnaninf(int) { return false; }
  static inline void seedrandom(unsigned int s) { 
    std::srand(s); 
#ifdef __APPLE__
    // throw away first random number to address bug 3655
    // http://software.sandia.gov/bugzilla/show_bug.cgi?id=3655
    random();
#endif
  }
  //static inline int random() { return (-1 + 2*rand()); }  // RAB: This version should be used to be consistent with others
  static inline int random() { return std::rand(); }             // RAB: This version should be used for an unsigned int, not int
  static inline std::string name() { return "int"; }
  static inline int squareroot(int x) { return (int) std::sqrt((double) x); }
  static inline int pow(int x, int y) { return (int) std::pow((double)x,(double)y); }
};

template<>
struct ScalarTraits<long int>
{
  typedef long int magnitudeType;
  typedef long int halfPrecision;
  typedef long int doublePrecision;
  static const bool isComplex = false;
  static const bool isOrdinal = true;
  static const bool isComparable = true;
  static const bool hasMachineParameters = false;
  // Not defined: eps(), sfmin(), base(), prec(), t(), rnd(), emin(), rmin(), emax(), rmax()
  static inline magnitudeType magnitude(long int a) { return static_cast<long int>(std::fabs(static_cast<double>(a))); }
  static inline long int zero()  { return 0; }
  static inline long int one()   { return 1; }
  static inline long int conjugate(long int x) { return x; }
  static inline long int real(long int x) { return x; }
  static inline long int imag(long int) { return 0; }
  static inline void seedrandom(unsigned int s) { 
    std::srand(s); 
#ifdef __APPLE__
    // throw away first random number to address bug 3655
    // http://software.sandia.gov/bugzilla/show_bug.cgi?id=3655
    random();
#endif
  }
  //static inline int random() { return (-1 + 2*rand()); }  // RAB: This version should be used to be consistent with others
  static inline long int random() { return std::rand(); }             // RAB: This version should be used for an unsigned int, not int
  static inline std::string name() { return "long int"; }
  static inline long int squareroot(long int x) { return (long int) std::sqrt((double) x); }
  static inline long int pow(long int x, long int y) { return (long int) std::pow((double)x,(double)y); }
};

template<>
struct ScalarTraits<size_t>
{
  typedef size_t magnitudeType;
  typedef size_t halfPrecision;
  typedef size_t doublePrecision;
  static const bool isComplex = false;
  static const bool isOrdinal = true;
  static const bool isComparable = true;
  static const bool hasMachineParameters = false;
  // Not defined: eps(), sfmin(), base(), prec(), t(), rnd(), emin(), rmin(), emax(), rmax()
  static inline magnitudeType magnitude(size_t a) { return a; }
  static inline size_t zero()  { return 0; }
  static inline size_t one()   { return 1; }
  static inline size_t conjugate(size_t x) { return x; }
  static inline size_t real(size_t x) { return x; }
  static inline size_t imag(size_t) { return 0; }
  static inline void seedrandom(unsigned int s) { 
    std::srand(s); 
#ifdef __APPLE__
    // throw away first random number to address bug 3655
    // http://software.sandia.gov/bugzilla/show_bug.cgi?id=3655
    random();
#endif
  }
  //static inline int random() { return (-1 + 2*rand()); }  // RAB: This version should be used to be consistent with others
  static inline size_t random() { return std::rand(); }             // RAB: This version should be used for an unsigned int, not int
  static inline std::string name() { return "size_t"; }
  static inline size_t squareroot(size_t x) { return (size_t) std::sqrt((double) x); }
  static inline size_t pow(size_t x, size_t y) { return (size_t) std::pow((double)x,(double)y); }
};

#ifdef HAVE_TEUCHOS_LONG_LONG_INT
template<>
struct ScalarTraits<long long int>
{
  typedef long long int magnitudeType;
  typedef long long int halfPrecision;
  typedef long long int doublePrecision;
  static const bool isComplex = false;
  static const bool isOrdinal = true;
  static const bool isComparable = true;
  static const bool hasMachineParameters = false;
  // Not defined: eps(), sfmin(), base(), prec(), t(), rnd(), emin(), rmin(), emax(), rmax()
  static inline magnitudeType magnitude(long long int a) { return static_cast<long long int>(std::fabs(static_cast<double>(a))); }
  static inline long long int zero()  { return 0; }
  static inline long long int one()   { return 1; }
  static inline long long int conjugate(long long int x) { return x; }
  static inline long long int real(long long int x) { return x; }
  static inline long long int imag(long long int) { return 0; }
  static inline void seedrandom(unsigned int s) { 
    std::srand(s); 
#ifdef __APPLE__
    // throw away first random number to address bug 3655
    // http://software.sandia.gov/bugzilla/show_bug.cgi?id=3655
    random();
#endif
  }
  //static inline int random() { return (-1 + 2*rand()); }  // RAB: This version should be used to be consistent with others
  static inline long long int random() { return std::rand(); }             // RAB: This version should be used for an unsigned int, not int
  static inline std::string name() { return "long long int"; }
  static inline long long int squareroot(long long int x) { return (long long int) std::sqrt((double) x); }
  static inline long long int pow(long long int x, long long int y) { return (long long int) std::pow((double)x,(double)y); }
};
#endif // HAVE_TEUCHOS_LONG_LONG_INT

#ifndef __sun
extern const float flt_nan;
#endif
 
template<>
struct ScalarTraits<float>
{
  typedef float magnitudeType;
  typedef float halfPrecision; // should become IEEE754-2008 binary16 or fp16 later, perhaps specified at configure according to architectural support
  typedef double doublePrecision;
  static const bool isComplex = false;
  static const bool isOrdinal = false;
  static const bool isComparable = true;
  static const bool hasMachineParameters = true;
  static inline float eps()   {
    return std::numeric_limits<float>::epsilon();
  }
  static inline float sfmin() {
    return std::numeric_limits<float>::min();
  }
  static inline float base()  {
    return static_cast<float>(std::numeric_limits<float>::radix);
  }
  static inline float prec()  {
    return eps()*base();
  }
  static inline float t()     {
    return static_cast<float>(std::numeric_limits<float>::digits);
  }
  static inline float rnd()   {
    return ( std::numeric_limits<float>::round_style == std::round_to_nearest ? one() : zero() );
  }
  static inline float emin()  {
    return static_cast<float>(std::numeric_limits<float>::min_exponent);
  }
  static inline float rmin()  {
    return std::numeric_limits<float>::min();
  }
  static inline float emax()  {
    return static_cast<float>(std::numeric_limits<float>::max_exponent);
  }
  static inline float rmax()  {
    return std::numeric_limits<float>::max();
  }
  static inline magnitudeType magnitude(float a)
    {
#ifdef TEUCHOS_DEBUG
      TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR(
        a, "Error, the input value to magnitude(...) a = " << a << " can not be NaN!" );
#endif      
      return std::fabs(a);
    }    
  static inline float zero()  { return(0.0f); }
  static inline float one()   { return(1.0f); }    
  static inline float conjugate(float x)   { return(x); }    
  static inline float real(float x) { return x; }
  static inline float imag(float) { return zero(); }
  static inline float nan() {
#ifdef __sun
    return 0.0f/std::sin(0.0f);
#else
    return flt_nan;
#endif
  }
  static inline bool isnaninf(float x) { // RAB: 2004/05/28: Taken from NOX_StatusTest_FiniteValue.C
    const float tol = 1e-6f; // Any (bounded) number should do!
    if( !(x <= tol) && !(x > tol) ) return true;                 // IEEE says this should fail for NaN
    float z=0.0f*x; if( !(z <= tol) && !(z > tol) ) return true;  // Use fact that Inf*0 = NaN
    return false;
  }
  static inline void seedrandom(unsigned int s) { 
    std::srand(s); 
#ifdef __APPLE__
    // throw away first random number to address bug 3655
    // http://software.sandia.gov/bugzilla/show_bug.cgi?id=3655
    random();
#endif
  }
  static inline float random() { float rnd = (float) std::rand() / RAND_MAX; return (-1.0f + 2.0f * rnd); }
  static inline std::string name() { return "float"; }
  static inline float squareroot(float x)
    {
#ifdef TEUCHOS_DEBUG
      TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR(
        x, "Error, the input value to squareroot(...) x = " << x << " can not be NaN!" );
#endif
      errno = 0;
      const float rtn = std::sqrt(x);
      if (errno)
        return nan();
      return rtn;
    }
  static inline float pow(float x, float y) { return std::pow(x,y); }
};

#ifndef __sun
extern const double dbl_nan;
#endif
 
template<>
struct ScalarTraits<double>
{
  typedef double magnitudeType;
  typedef float halfPrecision;
  /* there are different options as to how to double "double"
     - QD's DD (if available)
     - ARPREC
     - GNU MP
     - a true hardware quad

     in the shortterm, this should be specified at configure time. I have inserted a configure-time option (--enable-teuchos-double-to-dd) 
     which uses QD's DD when available. This must be used alongside --enable-teuchos-qd.
   */
#if defined(HAVE_TEUCHOS_DOUBLE_TO_QD)
  typedef dd_real doublePrecision;
#elif defined(HAVE_TEUCHOS_DOUBLE_TO_ARPREC)
  typedef mp_real doublePrecision;
#else
  typedef double doublePrecision;     // don't double "double" in this case
#endif
  static const bool isComplex = false;
  static const bool isOrdinal = false;
  static const bool isComparable = true;
  static const bool hasMachineParameters = true;
  static inline double eps()   {
    return std::numeric_limits<double>::epsilon();
  }
  static inline double sfmin() {
    return std::numeric_limits<double>::min();
  }
  static inline double base()  {
    return std::numeric_limits<double>::radix;
  }
  static inline double prec()  {
    return eps()*base();
  }
  static inline double t()     {
    return std::numeric_limits<double>::digits;
  }
  static inline double rnd()   {
    return ( std::numeric_limits<double>::round_style == std::round_to_nearest ? double(1.0) : double(0.0) );
  }
  static inline double emin()  {
    return std::numeric_limits<double>::min_exponent;
  }
  static inline double rmin()  {
    return std::numeric_limits<double>::min();
  }
  static inline double emax()  {
    return std::numeric_limits<double>::max_exponent;
  }
  static inline double rmax()  {
    return std::numeric_limits<double>::max();
  }
  static inline magnitudeType magnitude(double a)
    {
#ifdef TEUCHOS_DEBUG
      TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR(
        a, "Error, the input value to magnitude(...) a = " << a << " can not be NaN!" );
#endif      
      return std::fabs(a);
    }
  static inline double zero()  { return 0.0; }
  static inline double one()   { return 1.0; }
  static inline double conjugate(double x)   { return(x); }    
  static inline double real(double x) { return(x); }
  static inline double imag(double) { return(0); }
  static inline double nan() {
#ifdef __sun
    return 0.0/std::sin(0.0);
#else
    return dbl_nan;
#endif
  }
  static inline bool isnaninf(double x) { // RAB: 2004/05/28: Taken from NOX_StatusTest_FiniteValue.C
    const double tol = 1e-6; // Any (bounded) number should do!
    if( !(x <= tol) && !(x > tol) ) return true;                  // IEEE says this should fail for NaN
    double z=0.0*x; if( !(z <= tol) && !(z > tol) ) return true;  // Use fact that Inf*0 = NaN
    return false;
  }
  static inline void seedrandom(unsigned int s) { 
    std::srand(s); 
#ifdef __APPLE__
    // throw away first random number to address bug 3655
    // http://software.sandia.gov/bugzilla/show_bug.cgi?id=3655
    random();
#endif
  }
  static inline double random() { double rnd = (double) std::rand() / RAND_MAX; return (double)(-1.0 + 2.0 * rnd); }
  static inline std::string name() { return "double"; }
  static inline double squareroot(double x)
    {
#ifdef TEUCHOS_DEBUG
      TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR(
        x, "Error, the input value to squareroot(...) x = " << x << " can not be NaN!" );
#endif      
      errno = 0;
      const double rtn = std::sqrt(x);
      if (errno)
        return nan();
      return rtn;
    }
  static inline double pow(double x, double y) { return std::pow(x,y); }
};

#ifdef HAVE_TEUCHOS_QD
bool operator&&(const dd_real &a, const dd_real &b);
bool operator&&(const qd_real &a, const qd_real &b);

template<>
struct ScalarTraits<dd_real>
{
  typedef dd_real magnitudeType;
  typedef double halfPrecision;
  typedef qd_real doublePrecision;
  static const bool isComplex = false;
  static const bool isOrdinal = false;
  static const bool isComparable = true;
  static const bool hasMachineParameters = true;
  static inline dd_real eps()   { return std::numeric_limits<dd_real>::epsilon(); }
  static inline dd_real sfmin() { return std::numeric_limits<dd_real>::min(); }
  static inline dd_real base()  { return std::numeric_limits<dd_real>::radix; }
  static inline dd_real prec()  { return eps()*base(); }
  static inline dd_real t()     { return std::numeric_limits<dd_real>::digits; }
  static inline dd_real rnd()   { return ( std::numeric_limits<dd_real>::round_style == std::round_to_nearest ? dd_real(1.0) : dd_real(0.0) ); }
  static inline dd_real emin()  { return std::numeric_limits<dd_real>::min_exponent; }
  static inline dd_real rmin()  { return std::numeric_limits<dd_real>::min(); }
  static inline dd_real emax()  { return std::numeric_limits<dd_real>::max_exponent; }
  static inline dd_real rmax()  { return std::numeric_limits<dd_real>::max(); }
  static inline magnitudeType magnitude(dd_real a)
  {
#ifdef TEUCHOS_DEBUG
      TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR(
        a, "Error, the input value to magnitude(...) a = " << a << " can not be NaN!" );
#endif      
      return abs(a);
  }
  static inline dd_real zero()  { return dd_real(0.0); }
  static inline dd_real one()   { return dd_real(1.0); }
  static inline dd_real conjugate(dd_real x)   { return(x); }    
  static inline dd_real real(dd_real x) { return x ; }
  static inline dd_real imag(dd_real) { return zero(); }
  static inline dd_real nan() { return dd_real::_nan; }
  static inline bool isnaninf(dd_real x) { return isnan(x) || isinf(x); }
  static inline void seedrandom(unsigned int s) {
    // ddrand() uses std::rand(), so the std::srand() is our seed
    std::srand(s); 
#ifdef __APPLE__
    // throw away first random number to address bug 3655
    // http://software.sandia.gov/bugzilla/show_bug.cgi?id=3655
    random();
#endif
  }
  static inline dd_real random() { return ddrand(); }
  static inline std::string name() { return "dd_real"; }
  static inline dd_real squareroot(dd_real x)
  {
#ifdef TEUCHOS_DEBUG
      TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR(
        x, "Error, the input value to squareroot(...) x = " << x << " can not be NaN!" );
#endif      
      return sqrt(x);
  }
  static inline dd_real pow(dd_real x, dd_real y) { return pow(x,y); }
};

template<>
struct ScalarTraits<qd_real>
{
  typedef qd_real magnitudeType;
  typedef dd_real halfPrecision;
  typedef qd_real doublePrecision;
  static const bool isComplex = false;
  static const bool isOrdinal = false;
  static const bool isComparable = true;
  static const bool hasMachineParameters = true;
  static inline qd_real eps()   { return std::numeric_limits<qd_real>::epsilon(); }
  static inline qd_real sfmin() { return std::numeric_limits<qd_real>::min(); }
  static inline qd_real base()  { return std::numeric_limits<qd_real>::radix; }
  static inline qd_real prec()  { return eps()*base(); }
  static inline qd_real t()     { return std::numeric_limits<qd_real>::digits; }
  static inline qd_real rnd()   { return ( std::numeric_limits<qd_real>::round_style == std::round_to_nearest ? qd_real(1.0) : qd_real(0.0) ); }
  static inline qd_real emin()  { return std::numeric_limits<qd_real>::min_exponent; }
  static inline qd_real rmin()  { return std::numeric_limits<qd_real>::min(); }
  static inline qd_real emax()  { return std::numeric_limits<qd_real>::max_exponent; }
  static inline qd_real rmax()  { return std::numeric_limits<qd_real>::max(); }
  static inline magnitudeType magnitude(qd_real a)
  {
#ifdef TEUCHOS_DEBUG
      TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR(
        a, "Error, the input value to magnitude(...) a = " << a << " can not be NaN!" );
#endif      
      return abs(a);
  }
  static inline qd_real zero()  { return qd_real(0.0); }
  static inline qd_real one()   { return qd_real(1.0); }
  static inline qd_real conjugate(qd_real x)   { return(x); }    
  static inline qd_real real(qd_real x) { return x ; }
  static inline qd_real imag(qd_real) { return zero(); }
  static inline qd_real nan() { return qd_real::_nan; }
  static inline bool isnaninf(qd_real x) { return isnan(x) || isinf(x); }
  static inline void seedrandom(unsigned int s) {
    // qdrand() uses std::rand(), so the std::srand() is our seed
    std::srand(s); 
#ifdef __APPLE__
    // throw away first random number to address bug 3655
    // http://software.sandia.gov/bugzilla/show_bug.cgi?id=3655
    random();
#endif
  }
  static inline qd_real random() { return qdrand(); }
  static inline std::string name() { return "qd_real"; }
  static inline qd_real squareroot(qd_real x)
  {
#ifdef TEUCHOS_DEBUG
      TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR(
        x, "Error, the input value to squareroot(...) x = " << x << " can not be NaN!" );
#endif      
      return sqrt(x);
  }
  static inline qd_real pow(qd_real x, qd_real y) { return pow(x,y); }
};

#endif  // HAVE_TEUCHOS_QD

#ifdef HAVE_TEUCHOS_GNU_MP

extern gmp_randclass gmp_rng; 


/* Regarding halfPrecision, doublePrecision and mpf_class: 
   Because the precision of an mpf_class float is not determined by the data type, 
   there is no way to fill the typedefs for this object. 

   Instead, we could create new data classes (e.g., Teuchos::MPF128, Teuchos::MPF256) for 
   commonly used levels of precision, and fill out ScalarTraits for these. This would allow us
   to typedef the promotions and demotions in the appropriate way. These classes would serve to 
   wrap an mpf_class object, calling the constructor for the appropriate precision, exposing the 
   arithmetic routines but hiding the precision-altering routines.
   
   Alternatively (perhaps, preferably), would could create a single class templated on the precision (e.g., Teuchos::MPF<N>). 
   Then we have a single (partially-specialized) implementation of ScalarTraits. This class, as above, must expose all of the 
   operations expected of a scalar type; however, most of these can be trivially stolen from the gmpcxx.h header file

   CGB/RAB, 01/05/2009
*/
template<>
struct ScalarTraits<mpf_class>
{
  typedef mpf_class magnitudeType;
  typedef mpf_class halfPrecision;
  typedef mpf_class doublePrecision;
  static const bool isComplex = false;
  static const bool hasMachineParameters = false;
  // Not defined: eps(), sfmin(), base(), prec(), t(), rnd(), emin(), rmin(), emax(), rmax()
  static magnitudeType magnitude(mpf_class a) { return std::abs(a); }
  static inline mpf_class zero() { mpf_class zero = 0.0; return zero; }
  static inline mpf_class one() { mpf_class one = 1.0; return one; }    
  static inline mpf_class conjugate(mpf_class x) { return x; }
  static inline mpf_class real(mpf_class x) { return(x); }
  static inline mpf_class imag(mpf_class x) { return(0); }
  static inline bool isnaninf(mpf_class x) { return false; } // mpf_class currently can't handle nan or inf!
  static inline void seedrandom(unsigned int s) { 
    unsigned long int seedVal = static_cast<unsigned long int>(s);
    gmp_rng.seed( seedVal );	
  }
  static inline mpf_class random() { 
    return gmp_rng.get_f(); 
  }
  static inline std::string name() { return "mpf_class"; }
  static inline mpf_class squareroot(mpf_class x) { return std::sqrt(x); }
  static inline mpf_class pow(mpf_class x, mpf_class y) { return pow(x,y); }
  // Todo: RAB: 2004/05/28: Add nan() and isnaninf() functions when needed!
};

#endif  // HAVE_TEUCHOS_GNU_MP

#ifdef HAVE_TEUCHOS_ARPREC

/* See discussion above for mpf_class, regarding halfPrecision and doublePrecision. Something similar will need to be done
   for ARPREC. */
template<>
struct ScalarTraits<mp_real>
{
  typedef mp_real magnitudeType;
  typedef double halfPrecision;
  typedef mp_real doublePrecision;
  static const bool isComplex = false;
  static const bool isComparable = true;
  static const bool isOrdinal = false;
  static const bool hasMachineParameters = false;
  // Not defined: eps(), sfmin(), base(), prec(), t(), rnd(), emin(), rmin(), emax(), rmax()
  static magnitudeType magnitude(mp_real a) { return abs(a); }
  static inline mp_real zero() { mp_real zero = 0.0; return zero; }
  static inline mp_real one() { mp_real one = 1.0; return one; }    
  static inline mp_real conjugate(mp_real x) { return x; }
  static inline mp_real real(mp_real x) { return(x); }
  static inline mp_real imag(mp_real x) { return zero(); }
  static inline bool isnaninf(mp_real x) { return false; } // ToDo: Change this?
  static inline void seedrandom(unsigned int s) { 
    long int seedVal = static_cast<long int>(s);
    srand48(seedVal);
  }
  static inline mp_real random() { return mp_rand(); }
  static inline std::string name() { return "mp_real"; }
  static inline mp_real squareroot(mp_real x) { return sqrt(x); }
  static inline mp_real pow(mp_real x, mp_real y) { return pow(x,y); }
  // Todo: RAB: 2004/05/28: Add nan() and isnaninf() functions when needed!
};
  
#endif // HAVE_TEUCHOS_ARPREC
 
#ifdef HAVE_TEUCHOS_COMPLEX

// Partial specialization for std::complex numbers templated on real type T
template<class T> 
struct ScalarTraits<
  std::complex<T>
>
{
  typedef std::complex<T>  ComplexT;
  typedef std::complex<typename ScalarTraits<T>::halfPrecision> halfPrecision;
  typedef std::complex<typename ScalarTraits<T>::doublePrecision> doublePrecision;
  typedef typename ScalarTraits<T>::magnitudeType magnitudeType;
  static const bool isComplex = true;
  static const bool isOrdinal = ScalarTraits<T>::isOrdinal;
  static const bool isComparable = false;
  static const bool hasMachineParameters = true;
  static inline magnitudeType eps()          { return ScalarTraits<magnitudeType>::eps(); }
  static inline magnitudeType sfmin()        { return ScalarTraits<magnitudeType>::sfmin(); }
  static inline magnitudeType base()         { return ScalarTraits<magnitudeType>::base(); }
  static inline magnitudeType prec()         { return ScalarTraits<magnitudeType>::prec(); }
  static inline magnitudeType t()            { return ScalarTraits<magnitudeType>::t(); }
  static inline magnitudeType rnd()          { return ScalarTraits<magnitudeType>::rnd(); }
  static inline magnitudeType emin()         { return ScalarTraits<magnitudeType>::emin(); }
  static inline magnitudeType rmin()         { return ScalarTraits<magnitudeType>::rmin(); }
  static inline magnitudeType emax()         { return ScalarTraits<magnitudeType>::emax(); }
  static inline magnitudeType rmax()         { return ScalarTraits<magnitudeType>::rmax(); }
  static magnitudeType magnitude(ComplexT a)
    {
#ifdef TEUCHOS_DEBUG
      TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR(
        a, "Error, the input value to magnitude(...) a = " << a << " can not be NaN!" );
#endif      
      return std::abs(a);
    }
  static inline ComplexT zero()              { return ComplexT(ScalarTraits<magnitudeType>::zero(),ScalarTraits<magnitudeType>::zero()); }
  static inline ComplexT one()               { return ComplexT(ScalarTraits<magnitudeType>::one(),ScalarTraits<magnitudeType>::zero()); }
  static inline ComplexT conjugate(ComplexT a){ return ComplexT(a.real(),-a.imag()); }
  static inline magnitudeType real(ComplexT a) { return a.real(); }
  static inline magnitudeType imag(ComplexT a) { return a.imag(); }
  static inline ComplexT nan()               { return ComplexT(ScalarTraits<magnitudeType>::nan(),ScalarTraits<magnitudeType>::nan()); }
  static inline bool isnaninf(ComplexT x)    { return ScalarTraits<magnitudeType>::isnaninf(x.real()) || ScalarTraits<magnitudeType>::isnaninf(x.imag()); }
  static inline void seedrandom(unsigned int s) { ScalarTraits<magnitudeType>::seedrandom(s); }
  static inline ComplexT random()
    {
      const T rnd1 = ScalarTraits<magnitudeType>::random();
      const T rnd2 = ScalarTraits<magnitudeType>::random();
      return ComplexT(rnd1,rnd2);
    }
  static inline std::string name() { return std::string("std::complex<")+std::string(ScalarTraits<magnitudeType>::name())+std::string(">"); }
  // This will only return one of the square roots of x, the other can be obtained by taking its conjugate
  static inline ComplexT squareroot(ComplexT x)
    {
#ifdef TEUCHOS_DEBUG
      TEUCHOS_SCALAR_TRAITS_NAN_INF_ERR(
        x, "Error, the input value to squareroot(...) x = " << x << " can not be NaN!" );
#endif
      typedef ScalarTraits<magnitudeType>  STMT;
      const T r  = x.real(), i = x.imag();
      const T a  = STMT::squareroot((r*r)+(i*i));
      const T nr = STMT::squareroot((a+r)/2);
      const T ni = STMT::squareroot((a-r)/2);
      return ComplexT(nr,ni);
    }
  static inline ComplexT pow(ComplexT x, ComplexT y) { return pow(x,y); }
};

#endif //  HAVE_TEUCHOS_COMPLEX

#endif // DOXYGEN_SHOULD_SKIP_THIS

} // Teuchos namespace

#endif // _TEUCHOS_SCALARTRAITS_HPP_
