#ifndef _PETRA_RDP_SPD_DENSEMATRIX_H_
#define _PETRA_RDP_SPD_DENSEMATRIX_H_

//! Petra_RDP_SPD_DenseMatrix: A class for constructing and using symmetric positive definite dense matrices.

/*! The Petra_RDP_SPD_DenseMatrix class enables the construction and use of real-valued, symmetric positive definite, 
    double-precision dense matrices.  It is built on the Petra_RDP_DenseMatrix class which in turn is built on the 
    BLAS and LAPACK via the Petra_BLAS and 
    Petra_LAPACK classes. 

The Petra_RDP_SPD_DenseMatrix class is intended to provide full-featured support for solving linear and eigen system
problems for symmetric positive definite matrices.  It is written on top of BLAS and LAPACK and thus has excellent
performance and numerical capabilities.  Using this class, one can either perform simple factorizations and solves or
apply all the tricks available in LAPACK to get the best possible solution for very ill-conditioned problems.

<b>Petra_RDP_SPD_DenseMatrix vs. Petra_LAPACK</b>

The Petra_LAPACK class provides access to most of the same functionality as Petra_RDP_SPD_DenseMatrix.
The primary difference is that Petra_LAPACK is a "thin" layer on top of LAPACK and Petra_RDP_SPD_DenseMatrix
attempts to provide easy access to the more sophisticated aspects of solving dense linear and eigensystems.
<ul>
<li> When you should use Petra_LAPACK:  If you are simply looking for a convenient wrapper around the Fortran LAPACK
     routines and you have a well-conditioned problem, you should probably use Petra_LAPACK directly.
<li> When you should use Petra_RDP_SPD_DenseMatrix: If you want to (or potentially want to) solve ill-conditioned 
     problems or want to work with a more object-oriented interface, you should probably use Petra_RDP_SPD_DenseMatrix.
     
</ul>

<b>Constructing Petra_RDP_SPD_DenseMatrix Objects</b>

There are three Petra_RDP_DenseMatrix constructors.  The first constructs a zero-sized object which should be made
to appropriate length using the Shape() or Reshape() functions and then filled with the [] or () operators. 
The second is a constructor that accepts user
data as a 2D array, the third is a copy constructor. The second constructor has
two data access modes (specified by the Petra_DataAccess argument):
<ol>
  <li> Copy mode - Allocates memory and makes a copy of the user-provided data. In this case, the
       user data is not needed after construction.
  <li> View mode - Creates a "view" of the user data. In this case, the
       user data is required to remain intact for the life of the object.
</ol>

\warning View mode is \e extremely dangerous from a data hiding perspective.
Therefore, we strongly encourage users to develop code using Copy mode first and 
only use the View mode in a secondary optimization phase.

<b>Setting vectors used for linear solves</b>

Setting the X and B vectors (which are Petra_RDP_DenseMatrix objects) used for solving linear systems 
is done separately from the constructor.  This allows
a single matrix factor to be used for multiple solves.  Similar to the constructor, the vectors X and B can
be copied or viewed using the Petra_DataAccess argument.

<b>Extracting Data from Petra_RDP_SPD_DenseMatrix Objects</b>

Once a Petra_RDP_SPD_DenseMatrix is constructed, it is possible to view the data via access functions.

\warning Use of these access functions cam be \e extremely dangerous from a data hiding perspective.


<b>Vector and Utility Functions</b>

Once a Petra_RDP_SPD_DenseMatrix is constructed, several mathematical functions can be applied to
the object.  Specifically:
<ul>
  <li> Factorizations.
  <li> Solves.
  <li> Condition estimates.
  <li> Equilibration.
  <li> Norms.
</ul>

The final useful function is Flops().  Each Petra_RDP_SPD_DenseMatrix object keep track of the number
of \e serial floating point operations performed using the specified object as the \e this argument
to the function.  The Flops() function returns this number as a double precision number.  Using this 
information, in conjunction with the Petra_Time class, one can get accurate parallel performance
numbers.

<b>Strategies for Solving Linear Systems</b>
In many cases, linear systems can be accurately solved by simply computing the Cholesky factorization
of the matrix and then performing a forward back solve with a given set of right hand side vectors.  However,
in some instances, the factorization may be very poorly conditioned and the simple approach may not work.  In
these situations, equilibration and iterative refinement may improve the accuracy, or prevent a breakdown in
the factorization. 

Petra_RDP_SPD_DenseMatrix will use equilibration with the factorization if, once the object
is constructed and \e before it is factored, you call the function FactorWithEquilibration(true) to force 
equilibration to be used.  If you are uncertain if equilibration should be used, you may call the function
ShouldEquilibrate() which will return true if equilibration could possibly help.  ShouldEquilibrate() uses
guidelines specified in the LAPACK User Guide, namely if SCOND < 0.1 and AMAX < Underflow or AMAX > Overflow, to 
determine if equilibration \e might be useful. 
 
Petra_RDP_SPD_DenseMatrix will use iterative refinement after a forward/back solve if you call
SolveToRefinedSolution(true).  It will also compute forward and backward error estimates if you call
EstimateSolutionErrors(true).  Access to the forward (back) error estimates is available via FERR() (BERR()).

Examples using Petra_RDP_SPD_DenseMatrix can be found in the Petra test directories.

*/
#include "Petra_Petra.h" 
#include "Petra_RDP_DenseMatrix.h"


//=========================================================================
class Petra_RDP_SPD_DenseMatrix : public Petra_RDP_DenseMatrix {

 public:
  //! Default constructor; defines a zero size object.
  /*!
    Petra_RDP_SPD_DenseMatrix objects defined by the default constructor should be sized with the Shape() 
    or Reshape() functions.  
    Values should be defined by using the [] or ()operators.
   */
  Petra_RDP_SPD_DenseMatrix(void);
  //! Set object values from two-dimensional array.
  /*!
    \param In 
           Petra_DataAccess - Enumerated type set to Copy or View.
    \param In
           A - Pointer to an array of double precision numbers.  The first vector starts at A.
	   The second vector starts at A+LDA, the third at A+2*LDA, and so on.
    \param In
           LDA - The "Leading Dimension", or stride between vectors in memory.
    \param In 
           NumRowsCols - Number of rows and columns in object.

	   See Detailed Description section for further discussion.
  */
  Petra_RDP_SPD_DenseMatrix(Petra_DataAccess CV, double *A, int LDA, int NumRowsCols);
  
  //! Petra_RDP_SPD_DenseMatrix copy constructor.
  
  Petra_RDP_SPD_DenseMatrix(const Petra_RDP_SPD_DenseMatrix& Source);
  
  //! Set dimensions of a Petra_RDP_SPD_DenseMatrix object; init values to zero.
  /*!
    \param In 
           NumRowsCols - Number of rows and columns in object.

	   Allows user to define the dimensions of a Petra_RDP_DenseMatrix at any point. This function can
	   be called at any point after construction.  Any values that were previously in this object are
	   destroyed and the resized matrix starts off with all zero values.

    \return Integer error code, set to 0 if successful.
  */
  int Shape(int NumRowsCols) {return(Petra_RDP_DenseMatrix::Shape(NumRowsCols,NumRowsCols));};
  
  //! Reshape a Petra_RDP_SPD_DenseMatrix object.
  /*!
    \param In 
           NumRowsCols - Number of rows and columns in object.

	   Allows user to define the dimensions of a Petra_RDP_DenseMatrix at any point. This function can
	   be called at any point after construction.  Any values that were previously in this object are
	   copied into the new shape.  If the new shape is smaller than the original, the upper left portion
	   of the original matrix (the principal submatrix) is copied to the new matrix.

    \return Integer error code, set to 0 if successful.
  */
  int Reshape(int NumRowsCols) {return(Petra_RDP_DenseMatrix::Reshape(NumRowsCols,NumRowsCols));};

  
  //! Petra_RDP_SPD_DenseMatrix destructor.  
  virtual ~Petra_RDP_SPD_DenseMatrix ();
  
  //! If Flag is true, causes all subsequent function calls to work with upper triangle \e this matrix, otherwise work with lower.
  void FactorUpperTriangle(bool Flag) {Upper_ = Flag; if (Flag) UPLO_ = 'U'; else UPLO_ = 'L'; return;};

  //! Computes the in-place Cholesky factorization of the matrix using the LAPACK routine \e DPOTRF.
  /*!
    \return Integer error code, set to 0 if successful.
  */
  int Factor(void);

  //! Computes the solution X to AX = B for the \e this matrix and the B provided to SetVectors()..
  /*!
    \return Integer error code, set to 0 if successful.
  */
  int Solve(void);

  //! Inverts the \e this matrix.
  /*! Note: This function works a little differently that DPOTRI in that it fills the entire
      matrix with the inverse, independent of the UPLO specification.

    \return Integer error code, set to 0 if successful. Otherwise returns the LAPACK error code INFO.
  */
  int Invert(void);

  //! Computes the scaling vector S(i) = 1/sqrt(A(i,i) of the \e this matrix.
  /*! 
    \return Integer error code, set to 0 if successful. Otherwise returns the LAPACK error code INFO.
  */
  int ComputeEquilibrateScaling(void);

  //! Equilibrates the \e this matrix.
  /*! 
    \return Integer error code, set to 0 if successful. Otherwise returns the LAPACK error code INFO.
  */
  int Equilibrate_A(void);

  //! Equilibrates the current RHS.
  /*! 
    \return Integer error code, set to 0 if successful. Otherwise returns the LAPACK error code INFO.
  */
  int Equilibrate_B(void);


  //! Apply Iterative Refinement.
  /*! 
    \return Integer error code, set to 0 if successful. Otherwise returns the LAPACK error code INFO.
  */
  int ApplyRefinement(void);

  //! Unscales the solution vectors if equilibration was used to solve the system.
  /*! 
    \return Integer error code, set to 0 if successful. Otherwise returns the LAPACK error code INFO.
  */
  int Unequilibrate_X(void);

  //! Returns the reciprocal of the 1-norm condition number of the \e this matrix.
  /*! 
    \param Value Out
           On return contains the reciprocal of the 1-norm condition number of the \e this matrix.
    
    \return Integer error code, set to 0 if successful. Otherwise returns the LAPACK error code INFO.
  */
  int ReciprocalConditionEstimate(double & Value);

  //! Returns true if upper triangle of \e this matrix has and will be used.
  bool Upper() {return(Upper_);};

  //! Returns true if the LAPACK general rules for equilibration suggest you should equilibrate the system.
  bool ShouldEquilibrate() {ComputeEquilibrateScaling(); return(ShouldEquilibrate_);};

  //! Ratio of smallest to largest equilibration scale factors for the \e this matrix (returns -1 if not yet computed).
  /*! If SCOND() is >= 0.1 and AMAX() is not close to overflow or underflow, then equilibration is not needed.
   */
  double SCOND() {return(SCOND_);};

  //! Returns the absolute value of the largest entry of the \e this matrix (returns -1 if not yet computed).
  double AMAX() {return(AMAX_);};  

 private:

  void CopyUPLOMat(bool Upper, double * A, int LDA, int NumRows);

  bool Upper_;

  char UPLO_;

  double SCOND_;

};

#endif /* _PETRA_RDP_SPD_DENSEMATRIX_H_ */
