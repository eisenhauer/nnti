
//@HEADER
// ***********************************************************************
// 
//                Komplex: Complex Linear Solver Package
//                 Copyright (2002) Sandia Corporation
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
//@HEADER

#ifndef KOMPLEX_LINEARPROBLEM_H
#define KOMPLEX_LINEARPROBLEM_H

#include "Teuchos_RCP.hpp"
#include <vector>
class Epetra_LinearProblem;
class Epetra_Map;
class Epetra_MultiVector;
class Epetra_Import;
class Epetra_Export;
class Epetra_MapColoring;
class Epetra_IntVector;
class Epetra_VbrMatrix;

//! Komplex_LinearProblem: A class for forming an equivalent real formulation of a complex valued problem.

/*! The Komplex_LinearProblem class takes a complex linear problem, separated into real and imaginary parts,
    and forms an equivalent real valued system of twice the dimension.  The resulting system can then be
    solved with any Trilinos solver that understands Epetra objects.

KOMPLEX solves a complex-valued linear system Ax = b by solving
an equivalent real-valued system of twice the dimension.  Specifically,
writing in terms of real and imaginary parts, we have

 \f[ (A_r + i*A_i)*(x_r + i*x_i) = (b_r + i*b_i) \f]

  or by separating into real and imaginary equations we have

\f[
  \left( \begin{array}{rr}
                                    A_r & -A_i\\
                                    A_i &  A_r
                             \end{array}
   \right)
   \left( \begin{array}{r}
                                    x_r\\
                                    x_i
                             \end{array}
   \right)
   =
   \left( \begin{array}{r}
                                    b_r\\
                                    b_i
                             \end{array}
   \right)
\f]
  which is a real-valued system of twice the size.  If we find xr and xi, we
  can form the solution to the original system as x = xr +i*xi.


KOMPLEX accept user linear systems in three forms with either global
or local index values.

1) The first form is true complex.  The user passes in an MSR or VBR
format matrix where the values are stored like Fortran complex
numbers.
Thus, the values array is of type double that is twice as long as the
number of complex values.  Each complex entry is stored with real part
followed by imaginary part (as in Fortran).

2) The second form stores real and imaginary parts separately, but the
pattern for each is identical.  Thus only the values of the imaginary
part are passed to the creation routines.

3) The third form accepts two real-valued matrices with no assumption
about the structure of the matrices.  Each matrix is multiplied by a
user-supplied complex constant.  This is the most general form.

Each of the above forms supports a global or local index set.  By this
we mean that the index values (stored in bindx) refer to the global
problem indices, or the local indices (for example after calling
AZ_transform).


*/    

class Komplex_LinearProblem {
      
 public:

  //@{ \name Constructors/Destructor.
  //! Komplex_LinearProblem constructor.
  /*! Constructs the Komplex operator from the user definition 
      of the complex-valued matrix C = (c0r+i*c0i)*A0 +(c1r+i*c1i)*A1.
      Using this general expression for the complex matrix allows easy formulation of a variety of common
      complex problems.

      The operator will be explicitly constructed as an Epetra_VbrMatrix object when the first call to
      SetKomplexOperator() is made.  Subsequent calls to this method will attempt to reuse the the existing
      KomplexVbrMatrix object if possible, rather than reconstructing from scratch.  If this is not possible (typically
      because the structure has changed) then a the previous KomplexVbrMatrix object will be deleted and a new one will be 
      constructed.

      \param c0r (In) The real part of the complex coefficient multiplying A0.
      \param c0i (In) The imag part of the complex coefficient multiplying A0.
      \param A0 (In) An Epetra_RowMatrix that is one of the matrices used to define the true complex operator.
      \param c1r (In) The real part of the complex coefficient multiplying A1.
      \param c1i (In) The imag part of the complex coefficient multiplying A1.
      \param A1 (In) An Epetra_RowMatrix that is the second of the matrices used to define the true complex operator.
      \param Xr (In) The real part of the complex valued LHS.  
      \param Xi (In) The imag part of the complex valued LHS.  
      \param Br (In) The real part of the complex valued RHS.  
      \param Bi (In) The imag part of the complex valued RHS.  
      
      \return Error code, set to 0 if no error.
  */
  Komplex_LinearProblem(double c0r, double c0i, const Epetra_RowMatrix & A0,
			double c1r, double c1i, const Epetra_RowMatrix & A1,
			const Epetra_MultiVector & Xr, const Epetra_MultiVector & Xi,
			const Epetra_MultiVector & Br, const Epetra_MultiVector & Bi);

  //! Komplex_LinearProblem Destructor
  virtual ~Komplex_LinearProblem();
  //@}
  //@{ \name Set methods.
  //! Update the values of the equivalent real valued system.
  /*! This method allows the values of an existing Komplex_LinearProblem object to be updated.  Note that
      the update that there is no change to the pattern of the matrices. 
    	   
    \return Error code, set to 0 if no error.
  */
  int UpdateValues(double c0r, double c0i, const Epetra_RowMatrix & A0,
		   double c1r, double c1i, const Epetra_RowMatrix & A1,
		   const Epetra_MultiVector & Xr, const Epetra_MultiVector & Xi,
		   const Epetra_MultiVector & Br, const Epetra_MultiVector & Bi);
  //@}
  //@{ \name Methods to extract complex system solution.
  //! Extrac a solution for the original complex-valued problem using the solution of the Komplex problem.
  /*! After solving the komplex linear system, this method can be called to extract the
      solution of the original problem, assuming the solution for the komplex system is valid.
    \param Xr (Out) An existing Epetra_MultiVector.  On exit it will contain the real part of the complex valued solution.  
    \param Xi (Out) An existing Epetra_MultiVector.  On exit it will contain the imag part of the complex valued solution. 
    
  */
  int ExtractSolution(Epetra_MultiVector & Xr, Epetra_MultiVector & Xi);
  //@}
  //@{ \name Attribute Access Methods.

  //! Returns pointer to the Epetra_LinearProblem object that defines the Komplex formulation.
  /*! The pointer returned from this method will contain the address of a fully-constructed Epetra_LinearProblem
      instance that can be used with any Trilinos preconditioner or solver.
  */
  Epetra_LinearProblem * KomplexProblem() const {return(KomplexProblem_.get());}

  //@}

 protected:
  int ProcessValues(double c0r, double c0i, const Epetra_RowMatrix & A0,
		    double c1r, double c1i, const Epetra_RowMatrix & A1,
		    const Epetra_MultiVector & Xr, const Epetra_MultiVector & Xi,
		    const Epetra_MultiVector & Br, const Epetra_MultiVector & Bi,
		    bool firstTime);
  
  int TestMaps (const Epetra_RowMatrix & A0, const Epetra_RowMatrix & A1,
		const Epetra_MultiVector & Xr, const Epetra_MultiVector & Xi,
		const Epetra_MultiVector & Br, const Epetra_MultiVector & Bi);
  
  int ConstructKomplexMaps(const Epetra_Map & A0DomainMap, const Epetra_Map & A0RangeMap,
			   const Epetra_Map & A0RowMap);
  
  int MakeKomplexMap(const Epetra_Map & Map, Teuchos::RCP<Epetra_Map> & KMap);
  
  int InitMatrixAccess(const Epetra_RowMatrix & A0, const Epetra_RowMatrix & A1);
 
  int GetRow(int Row, const Epetra_RowMatrix & A0, const Epetra_RowMatrix & A1,
	     int & NumIndices0, double * & Values0, int * & Indices0,
	     int & NumIndices1, double * & Values1, int * & Indices1);

  int PutRow(int Row, int & NumIndices, double * Values, int * Indices, bool firstTime); 

  Teuchos::RCP<Epetra_LinearProblem> KomplexProblem_;
  Teuchos::RCP<Epetra_CrsMatrix> KomplexMatrix_;
  Teuchos::RCP<Epetra_MultiVector> KomplexRHS_;
  Teuchos::RCP<Epetra_MultiVector> KomplexLHS_;
  
  Teuchos::RCP<Epetra_Map> KomplexMatrixRowMap_;
  Teuchos::RCP<Epetra_Map> KomplexMatrixColMap_;
  Teuchos::RCP<Epetra_Map> KomplexMatrixDomainMap_;
  Teuchos::RCP<Epetra_Map> KomplexMatrixRangeMap_;
  
  const Epetra_CrsMatrix * CrsA0_;
  const Epetra_CrsMatrix * CrsA1_;
  bool A0A1AreCrs_;
  
  std::vector<int> Indices0_;
  std::vector<double> Values0_;
  int MaxNumMyEntries0_;
  std::vector<int> Indices1_;
  std::vector<double> Values1_;
  int MaxNumMyEntries1_;	
  std::vector<int> IndicesK_;
  std::vector<double> ValuesK_;
  int MaxNumMyEntriesK_;	
  
 private:
  //! Copy constructor (defined as private so it is unavailable to user).
  Komplex_LinearProblem(const Komplex_LinearProblem & Problem){};
};
#endif /* KOMPLEX_LINEARPROBLEM_H */
