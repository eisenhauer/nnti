/* ***********************************************************************
// 
//           TSFExtended: Trilinos Solver Framework Extended
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
// **********************************************************************/

#ifndef TSFEPETRAVECTOR_HPP
#define TSFEPETRAVECTOR_HPP

#include "TSFConfigDefs.hpp"
#include "TSFPrintable.hpp"
#include "TSFVecDescribableByTypeID.hpp"
#include "TSFCoreEpetraVector.hpp"
#include "TSFIndexableVector.hpp"
#include "TSFVectorDecl.hpp"
#include "Epetra_FEVector.h"
#include "Epetra_Vector.h"


namespace TSFExtended
{
  using TSFCore::Index;
  using namespace Teuchos;
  /**
   * TSF extension of TSFCore::EpetraVector, implementing the LoadableVector
   * interface allowing an application to access elements. This class derives
   * from TSFCore::EpetraVector, so it can be used seamlessly in any 
   * TSFCore-based code.
   */
  class EpetraVector : public TSFCore::EpetraVector, 
                       public Handleable<TSFCore::Vector<double> >,
                       public IndexableVector<double>,
                       public VecDescribableByTypeID<double>,
                       public Printable
    {
    public:
      GET_RCP(TSFCore::Vector<double>);
      /** Construct with a smart pointer to an Epetra FE vector. */
      EpetraVector(const RefCountPtr<Epetra_Vector>& vec,
                   const RefCountPtr<const TSFCore::EpetraVectorSpace>& map);

      /** virtual dtor */
      virtual ~EpetraVector() {;}

      /** \name IndexableVector interface */
      //@{
      /** read the element at the given global index */
      virtual const double& operator[](Index globalIndex) const 
      {return getElement(globalIndex);}

      /** writable access to the element at the given global index */
      virtual double& operator[](Index globalIndex) ;
      //@}

      /** \name LoadableVector interface */
      //@{
      /** set a single element */
      void setElement(Index globalIndex, const double& value);

      /** add to a single element */
      void addToElement(Index globalIndex, const double& value);

      /** set a group of elements */
      void setElements(size_t numElems, const Index* globalIndices, 
                       const double* values);


      /** add to a group of elements */
      void addToElements(size_t numElems, const Index* globalIndices, 
                         const double* values);

      /** */
      void finalizeAssembly();
      //@}

      /** \name AccessibleVector interface */
      //@{
      /** */
      const double& getElement(Index globalIndex) const ;

      /** */
      void getElements(const Index* globalIndices, int numElems,
                       vector<Scalar>& elems) const ;
      //@}
      

      /** \name Printable interface */
      //@{
      /** Write to a stream  */
      void print(ostream& os) const 
      {
        epetra_vec()->Print(os);
      }
      //@}

      /** \name Describable interface */
      //@{
      /** Write a brief description */
//       string describe() const 
//       {
//         return "EpetraVector";
//       }b
      //@}


      /** Get a read-only Epetra_Vector */
      static const Epetra_Vector& getConcrete(const TSFExtended::Vector<double>& tsfVec);
      /** Get a read-write Epetra_Vector */
      static Epetra_Vector& getConcrete(TSFExtended::Vector<double>& tsfVec);
      /** Get a read-write Epetra_Vector pointer */
      static Epetra_Vector* getConcretePtr(TSFExtended::Vector<double>& tsfVec);
    };
  
}

#endif
