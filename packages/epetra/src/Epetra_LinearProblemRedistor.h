
/* Copyright (2001) Sandia Corportation. Under the terms of Contract 
 * DE-AC04-94AL85000, there is a non-exclusive license for use of this 
 * work by or on behalf of the U.S. Government.  Export of this program
 * may require a license from the United States Government. */


/* NOTICE:  The United States Government is granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable worldwide
 * license in ths data to reproduce, prepare derivative works, and
 * perform publicly and display publicly.  Beginning five (5) years from
 * July 25, 2001, the United States Government is granted for itself and
 * others acting on its behalf a paid-up, nonexclusive, irrevocable
 * worldwide license in this data to reproduce, prepare derivative works,
 * distribute copies to the public, perform publicly and display
 * publicly, and to permit others to do so.
 * 
 * NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT
 * OF ENERGY, NOR SANDIA CORPORATION, NOR ANY OF THEIR EMPLOYEES, MAKES
 * ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL LIABILITY OR
 * RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY
 * INFORMATION, APPARATUS, PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS
 * THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS. */

#ifndef EPETRA_LINEARPROBLEMREDISTOR_H
#define EPETRA_LINEARPROBLEMREDISTOR_H
#include "Epetra_Object.h"
class Epetra_Map;
class Epetra_LinearProblem;

//! Epetra_LinearProblemRedistor: A class for redistributing an Epetra_LinearProblem object.

/*! This class provides capabilities to redistribute an existing Epetra_LinearProblem object 
	  across a parallel distributed memory machine.  All or part of a linear problem object can
		be redistributed.   Reverse distributions, value updates and matrix transposition 
		are also supported.  Specification of the redistribution can be done by 
		<ol> 
		<li> providing a target	Epetra_Map object describing the new distribution, or 
		<li> by specifying the number of processors to use and stating whether or not
		     the problem should be completely replicated on all processors.
		</ol>

*/

class Epetra_LinearProblemRedistor: public Epetra_Object {
    
  public:

  //@{ \name Constructors/destructors.
  //! Epetra_LinearProblemRedistor constructor using pre-defined layout.
  /*!
    \param Problem (In) An existing Epetra_LinearProblem object.  The Epetra_RowMatrix, the LHS and RHS pointers
		       do not need to be defined before this constructor is called.
		\param RedistMap (In) An Epetra_Map describing the target layout of the redistribution.

    \return Pointer to a Epetra_LinearProblemRedistor object.

  */ 
  Epetra_LinearProblemRedistor(Epetra_LinearProblem * Problem, const Epetra_Map & RedistMap);

  //! Epetra_LinearProblemRedistor constructor specifying number of processor and replication bool.
  /*!
    \param Problem (In) An existing Epetra_LinearProblem object.  The Epetra_RowMatrix, the LHS and RHS pointers
		       do not need to be defined before this constructor is called.
		\param Replicate (In) A bool that indicates if the linear problem should be fully replicated on all processors.
		       If true, then a complete copy of the linear problem will be made on each processor.

    \return Pointer to a Epetra_LinearProblemRedistor object.
  */ 
  Epetra_LinearProblemRedistor(const Epetra_LinearProblem& Problem, const bool Replicate);

  //! Epetra_LinearProblemRedistor copy constructor.
  
  Epetra_LinearProblemRedistor(const Epetra_LinearProblemRedistor& Source);
  
  //! Epetra_LinearProblemRedistor destructor.
  
  virtual ~Epetra_LinearProblemRedistor();
  //@}
  
  //@{ \name Forward transformation methods.
  
  //! Generate a new Epetra_LinearProblem as a redistribution of the one passed into the constructor.
  /*! Constructs a new Epetra_LinearProblem that is a copy of the one passed in to the constructor.
		  The new problem will have redistributed copies of the RowMatrix, LHS and RHS from the original
			problem.  If any of these three objects are 0 pointers, then the corresponding pointer will be
			zero in the redistributed object.  

			The redistributed matrix will constructed as an Epetra_CrsMatrix.  The LHS and RHS will be Epetra_MultiVector
			objects.

			Two bools can be set when calling this method.  The first,
			ConstructTranspose, will cause the Redistribute method to construct the transpose of the original
			row matrix.  The second, MakeDataContiguous, forces the memory layout of the output matrix, RHS and LHS
			to be stored so that it is compatible with Fortran.  In particular, the Epetra_CrsMatrix is stored so 
			that value from row to row are contiguous, as are the indices.  This is compatible with the Harwell-Boeing
			compressed row and compressed column format.  The RHS and LHS are created so that there is a constant stride between
			the columns of the multivector.  This is compatible with Fortran 2D array storage.

    \param ConstructTranspose (In) Causes the output matrix to be transposed.  This feature can be used
		       to support solvers that need the matrix to be stored in column format.  This option
					 has no impact on the LHS and RHS of the output problem.
		\param MakeDataContiguous (In) Causes the output matrix, LHS and RHS to be stored in a form compatible with
		       Fortran-style solvers.  The output matrix will be compatible with the Harwell-Boeing compressed
					 column format.  The RHS and LHS will be stored such that the last value in column j of the 
					 multivector is stored next to the first value in column j+1.
		\param RedistProblem (Out) The redistributed Epetra_LinearProblem.  The RowMatrix, LHS and RHS that are generated
		       as part of this problem will be destroyed when the Epetra_LinearProblemRedistor object is destroyed.

		\return Integer error code, 0 if no errors, positive value if one or more of the input Rowmatrix, 
		        LHS or RHS pointers were 0.  Negative if some other fatal error occured.
					 
  */
  int CreateRedistProblem(const bool ConstructTranspose, const bool MakeDataContiguous, 
													Epetra_LinearProblem *& RedistProblem);

	
  //! Update the values of an already-redistributed problem.
  /*! Updates the values of an already-redistributed problem.  This method allows updating 
		  the redistributed problem without
		  allocating new storage.

    \param ProblemWithNewValues (In) The values from ProblemWithNewValues will be copied into the RedistProblem.  The
		       ProblemWithNewValues object must be identical in structure to the Epetra_LinearProblem object used to create
					 this instance of Epetra_LinearProblemRedistor.

		\return Integer error code, 0 if no errors, positive value if one or more of the input Rowmatrix, 
		        LHS or RHS pointers were 0.  Negative if some other fatal error occured.
					 
  */
  int UpdateValues(Epetra_LinearProblem * ProblemWithNewValues);
  //@}
  
  //@{ \name Reverse transformation methods.
  //! Update LHS of original Linear Problem object.
  /*! Copies the values from the LHS of the RedistProblem Object into the LHS of the original linear problem.  If the
		  RedistProblem is replicated, the LHS will be taken from processor 0 only.  If the RedistMap was passed in by the
			calling routine, it is the responsibility of the caller to make sure that the RedistMap is a bijective (1-to-1 and
			onto) map onto the RowMatrixRowMap of the original Epetra_RowMatrix object.  If this map is not bijective, the
			UpdateOriginalLHS() method may have indeterminant behavior.
			
    \return Error code, returns 0 if no error.
  */
   int UpdateOriginalLHS();
  //@}
  
  //@{ \name Attribute accessor methods.
  //! Returns const reference to the Epetra_Map that describes the layout of the RedistLinearProblem.
  const Epetra_Map & RedistMap() const;
  
  //! Returns const reference to the Epetra_Export object used to redistribute the original linear problem.
  /*! The RedistExporter object can be used to redistribute other Epetra_DistObject objects whose maps are compatible with
		  the original linear problem map, or with the RedistMap().
  */
  const Epetra_Export & RedistExporter() const;
  //@}
  
  //@{ \name Utility methods

	//! Extract the redistributed problem data in a form usable for other codes that require Harwell-Boeing format.
	/*! This method extract data from the linear problem for use with other packages, such as SuperLU, that require
		  the matrix, rhs and lhs in Harwell-Boeing format.  Note that the arrays returned by this method are owned by the
			Epetra_LinearProblemRedistor class, and they will be deleted when the owning class is destroyed.
	*/
	int ExtractHbData(int & M, int & N, int & nz, int * & ptr, int * & ind, double * & val, int & Nrhs, double * & rhs);
  //@}
  
  //@{ \name I/O methods
  
  //! Print method
  virtual void Print(ostream & os) const;
  //@}
  
 private: 
	int GenerateRedistMap();

	Epetra_LinearProblem * OrigProblem_;
	Epetra_LinearProblem * RedistProblem_;
	Epetra_Map * RedistMap_;

	bool Replicate_;
	bool ConstructTranspose_;
	bool MakeDataContiguous_;
	bool MapGenerated_;

	int ptr_;
		

};

#endif /* EPETRA_LINEARPROBLEMREDISTOR_H */
