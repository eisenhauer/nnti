// Kris
// 06.18.03 -- Minor formatting changes
//          -- Changed calls to LAPACK objects to use new <OType, SType> templates
// 07.08.03 -- Move into Teuchos package/namespace
// 07.11.03 -- Added ScalarTraits for ARPREC::mp_real
// 07.14.03 -- Fixed int rand() function (was set up to return a floating-point style random number)
// 07.17.03 -- Added squareroot() function

#ifndef _TEUCHOS_SCALARTRAITS_HPP_
#define _TEUCHOS_SCALARTRAITS_HPP_

#include "Teuchos_LAPACK.hpp"
#include "Teuchos_ConfigDefs.hpp"
#if ( defined(HAVE_COMPLEX) || defined(HAVE_COMPLEX_H) ) && defined(HAVE_TEUCHOS_EXPERIMENTAL)
#define ComplexFloat complex<float>
#define ComplexDouble complex<double>
#endif

#ifdef HAVE_TEUCHOS_ARPREC
#include "mp/mpreal.h"
#endif

namespace Teuchos {
  /** The Teuchos ScalarTraits file.
      
  For the most general type 'class T', we define aborting functions, 
  which should restrict implementations from using traits other than the
  specializations defined below.
  */
  
  template<class T>
  struct ScalarTraits 
  {
    typedef T magnitudeType;
    
    static inline int undefinedParameters()
    {
#ifndef TEUCHOS_NO_ERROR_REPORTS
      cerr << endl << "Teuchos::ScalarTraits: Machine parameters are undefined for this scalar type." << endl;
#endif
      return(-1);
    }
    
    static inline int unsupportedType()
    {
#ifndef TEUCHOS_NO_ERROR_REPORTS
      cerr << endl << "Teuchos::ScalarTraits: unsupported scalar type." << endl;
#endif
      return(-2);
    }
    
    static inline bool haveMachineParameters() { return false; };
    static inline magnitudeType eps()   { throw(undefinedParameters()); };
    static inline magnitudeType sfmin() { throw(undefinedParameters()); };
    static inline magnitudeType base()  { throw(undefinedParameters()); };
    static inline magnitudeType prec()  { throw(undefinedParameters()); };
    static inline magnitudeType t()     { throw(undefinedParameters()); };
    static inline magnitudeType rnd()   { throw(undefinedParameters()); };
    static inline magnitudeType emin()  { throw(undefinedParameters()); };
    static inline magnitudeType rmin()  { throw(undefinedParameters()); };
    static inline magnitudeType emax()  { throw(undefinedParameters()); };
    static inline magnitudeType rmax()  { throw(undefinedParameters()); };
    static inline magnitudeType magnitude(T a) { throw(unsupportedType()); };
    static inline T zero()                     { throw(unsupportedType()); };
    static inline T one()                      { throw(unsupportedType()); };
    static inline T random()                   { throw(unsupportedType()); };
    static inline const char* name()           { throw(unsupportedType()); };
    static inline magnitudeType squareroot(T x) { throw(unsupportedType()); };
  };
  
  template<>
  struct ScalarTraits<int>
  {
    typedef long long magnitudeType;
    
    static inline int undefinedParameters()
    {
#ifndef TEUCHOS_NO_ERROR_REPORTS
      cerr << endl << "Teuchos::ScalarTraits: Machine parameters are undefined for this scalar type." << endl;
#endif
      return(-1);
    }

    static inline bool haveMachineParameters() { return false; };
    static inline magnitudeType eps()   { throw(undefinedParameters()); };
    static inline magnitudeType sfmin() { throw(undefinedParameters()); };
    static inline magnitudeType base()  { throw(undefinedParameters()); };
    static inline magnitudeType prec()  { throw(undefinedParameters()); };
    static inline magnitudeType t()     { throw(undefinedParameters()); };
    static inline magnitudeType rnd()   { throw(undefinedParameters()); };
    static inline magnitudeType emin()  { throw(undefinedParameters()); };
    static inline magnitudeType rmin()  { throw(undefinedParameters()); };
    static inline magnitudeType emax()  { throw(undefinedParameters()); };
    static inline magnitudeType rmax()  { throw(undefinedParameters()); };
    static inline magnitudeType magnitude(int a) { return abs(a); };
    static inline int zero()  { return 0; };
    static inline int one()   { return 1; };
    static inline int random() { return rand(); };
    static inline const char * name() { return "int"; };
    static inline int squareroot(int x) { return (int) sqrt((double) x); };
  };
  
  
  template<>
  struct ScalarTraits<float>
  {
    typedef float magnitudeType;
    static inline bool haveMachineParameters() { return true; };
    static inline float eps()   { LAPACK<int, float> lp; return lp.LAMCH('E'); };
    static inline float sfmin() { LAPACK<int, float> lp; return lp.LAMCH('S'); };
    static inline float base()  { LAPACK<int, float> lp; return lp.LAMCH('B'); };
    static inline float prec()  { LAPACK<int, float> lp; return lp.LAMCH('P'); };
    static inline float t()     { LAPACK<int, float> lp; return lp.LAMCH('N'); };
    static inline float rnd()   { LAPACK<int, float> lp; return lp.LAMCH('R'); };
    static inline float emin()  { LAPACK<int, float> lp; return lp.LAMCH('M'); };
    static inline float rmin()  { LAPACK<int, float> lp; return lp.LAMCH('U'); };
    static inline float emax()  { LAPACK<int, float> lp; return lp.LAMCH('L'); };
    static inline float rmax()  { LAPACK<int, float> lp; return lp.LAMCH('O'); };
    static inline magnitudeType magnitude(float a) { return fabs(a); };    
    static inline float zero()  { return(0.0); };
    static inline float one()   { return(1.0); };    
    static inline float random() { float rnd = (float) rand() / RAND_MAX; return (float)(-1.0 + 2.0 * rnd); };
    static inline const char* name() { return "float"; };
    static inline float squareroot(float x) { return sqrt(x); };
  };
  
  
  template<>
  struct ScalarTraits<double>
  {
    typedef double magnitudeType;
    static inline bool haveMachineParameters() { return true; };
    static inline magnitudeType magnitude(double a) { return fabs(a); };
    static inline double zero()  { return 0.0; };
    static inline double one()   { return 1.0; };
    static inline double eps()   { LAPACK<int, double> lp; return lp.LAMCH('E'); };
    static inline double sfmin() { LAPACK<int, double> lp; return lp.LAMCH('S'); };
    static inline double base()  { LAPACK<int, double> lp; return lp.LAMCH('B'); };
    static inline double prec()  { LAPACK<int, double> lp; return lp.LAMCH('P'); };
    static inline double t()     { LAPACK<int, double> lp; return lp.LAMCH('N'); };
    static inline double rnd()   { LAPACK<int, double> lp; return lp.LAMCH('R'); };
    static inline double emin()  { LAPACK<int, double> lp; return lp.LAMCH('M'); };
    static inline double rmin()  { LAPACK<int, double> lp; return lp.LAMCH('U'); };
    static inline double emax()  { LAPACK<int, double> lp; return lp.LAMCH('L'); };
    static inline double rmax()  { LAPACK<int, double> lp; return lp.LAMCH('O'); };
    static inline double random() { double rnd = (double) rand() / RAND_MAX; return (double)(-1.0 + 2.0 * rnd); };
    static inline const char* name() { return "double"; };
    static inline double squareroot(double x) { return sqrt(x); };
  };
  
#if ( defined(HAVE_COMPLEX) || defined(HAVE_COMPLEX_H) ) && defined(HAVE_TEUCHOS_EXPERIMENTAL)
  
  template<> 
  struct ScalarTraits<ComplexFloat>
  {
    typedef float magnitudeType;
    static inline bool haveMachineParameters() { return true; };
    static inline float eps()   { LAPACK<int, float> lp; return lp.LAMCH('E'); };
    static inline float sfmin() { LAPACK<int, float> lp; return lp.LAMCH('S'); };
    static inline float base()  { LAPACK<int, float> lp; return lp.LAMCH('B'); };
    static inline float prec()  { LAPACK<int, float> lp; return lp.LAMCH('P'); };
    static inline float t()     { LAPACK<int, float> lp; return lp.LAMCH('N'); };
    static inline float rnd()   { LAPACK<int, float> lp; return lp.LAMCH('R'); };
    static inline float emin()  { LAPACK<int, float> lp; return lp.LAMCH('M'); };
    static inline float rmin()  { LAPACK<int, float> lp; return lp.LAMCH('U'); };
    static inline float emax()  { LAPACK<int, float> lp; return lp.LAMCH('L'); };
    static inline float rmax()  { LAPACK<int, float> lp; return lp.LAMCH('O'); };
    static magnitudeType magnitude(ComplexFloat a) { return std::abs(a); };
    static inline ComplexFloat zero()  { return ComplexFloat(0.0, 0.0); };
    static inline ComplexFloat one()   { return ComplexFloat(1.0, 0.0); };
    static inline ComplexFloat random()
    {
      float rnd1 = ScalarTraits<magnitudeType>::random();
      float rnd2 = ScalarTraits<magnitudeType>::random();
      return ComplexFloat(rnd1, rnd2);
    };
    static inline const char* name() { return "std::complex<float>"; };

    // This will only return one of the square roots of x, the other can be obtained by taking its conjugate
    static inline ComplexFloat squareroot(ComplexFloat x)
    {
      float r = x.real(), i = x.imag();
      float a = sqrt((r * r) + (i * i));
      float nr = sqrt((a + r) / 2);
      float ni = sqrt((a - r) / 2);
      ComplexFloat result = ComplexFloat(nr, ni);
      return result;
    };

  };
  
  template<>
  struct ScalarTraits<ComplexDouble>
  {
    typedef double magnitudeType;
    static inline bool haveMachineParameters() { return true; };
    static inline double eps()   { LAPACK<int, double> lp; return lp.LAMCH('E'); };
    static inline double sfmin() { LAPACK<int, double> lp; return lp.LAMCH('S'); };
    static inline double base()  { LAPACK<int, double> lp; return lp.LAMCH('B'); };
    static inline double prec()  { LAPACK<int, double> lp; return lp.LAMCH('P'); };
    static inline double t()     { LAPACK<int, double> lp; return lp.LAMCH('N'); };
    static inline double rnd()   { LAPACK<int, double> lp; return lp.LAMCH('R'); };
    static inline double emin()  { LAPACK<int, double> lp; return lp.LAMCH('M'); };
    static inline double rmin()  { LAPACK<int, double> lp; return lp.LAMCH('U'); };
    static inline double emax()  { LAPACK<int, double> lp; return lp.LAMCH('L'); };
    static inline double rmax()  { LAPACK<int, double> lp; return lp.LAMCH('O'); };
    static magnitudeType magnitude(ComplexDouble a) { return std::abs(a); };
    static inline ComplexDouble zero()  {return ComplexDouble(0.0,0.0); };
    static inline ComplexDouble one()   {return ComplexDouble(1.0,0.0); };    
    static inline ComplexDouble random()
    {
      double rnd1 = ScalarTraits<magnitudeType>::random();
      double rnd2 = ScalarTraits<magnitudeType>::random();
      return ComplexDouble(rnd1, rnd2);
    };
    static inline const char* name() { return "std::complex<double>"; };

    // This will only return one of the square roots of x, the other can be obtained by taking its conjugate
    static inline ComplexDouble squareroot(ComplexDouble x)
    {
      double r = x.real(), i = x.imag();
      double a = sqrt((r * r) + (i * i));
      double nr = sqrt((a + r) / 2);
      double ni = sqrt((a - r) / 2);
      ComplexDouble result = ComplexDouble(nr, ni);
      return result;
    };
  };

#endif  //  HAVE_COMPLEX || HAVE_COMPLEX_H

#ifdef HAVE_TEUCHOS_ARPREC

  template<>
  struct ScalarTraits<mp_real>
  {

    static inline int undefinedParameters()
    {
#ifndef TEUCHOS_NO_ERROR_REPORTS
      cerr << endl << "Teuchos::ScalarTraits: Machine parameters are undefined for this scalar type." << endl;
#endif
      return(-1);
    }

    typedef mp_real magnitudeType;
    static inline bool haveMachineParameters() { return false; };
    static inline mp_real eps()   { throw(undefinedParameters()); };
    static inline mp_real sfmin() { throw(undefinedParameters()); };
    static inline mp_real base()  { throw(undefinedParameters()); };
    static inline mp_real prec()  { throw(undefinedParameters()); };
    static inline mp_real t()     { throw(undefinedParameters()); };
    static inline mp_real rnd()   { throw(undefinedParameters()); };
    static inline mp_real emin()  { throw(undefinedParameters()); };
    static inline mp_real rmin()  { throw(undefinedParameters()); };
    static inline mp_real emax()  { throw(undefinedParameters()); };
    static inline mp_real rmax()  { throw(undefinedParameters()); };

    static magnitudeType magnitude(mp_real a) { return abs(a); };
    static inline mp_real zero() { mp_real zero = 0.0; return zero; };
    static inline mp_real one() { mp_real one = 1.0; return one; };    
    static inline mp_real random() { return mp_rand(); };
    static inline const char* name() { return "mp_real"; };
    static inline mp_real squareroot(mp_real x) { return sqrt(x); };
  };
  
#endif

} // Teuchos namespace

#endif // _TEUCHOS_SCALARTRAITS_HPP_
