#include "PB_Utilities.hpp"

#include "Thyra_MultiVectorStdOps.hpp"
#include "Thyra_DefaultLinearOpSource.hpp"
#include "Thyra_DefaultInverseLinearOp.hpp"
#include "Thyra_DefaultZeroLinearOp.hpp"

#include "Teuchos_Array.hpp"

#include <cmath>

namespace PB {

using Teuchos::RCP;

// distance function...not parallel...entirely internal to this cpp file
inline double dist(int dim,double * coords,int row,int col)
{
   double value = 0.0;
   for(int i=0;i<dim;i++)
      value += std::pow(coords[dim*row+i]-coords[dim*col+i],2.0);

   // the distance between the two
   return std::sqrt(value);
}

/** \brief Build a graph Laplacian stenciled on a Epetra_CrsMatrix.
  *
  * This function builds a graph Laplacian given a (locally complete)
  * vector of coordinates and a stencil Epetra_CrsMatrix (could this be
  * a graph of Epetra_RowMatrix instead?). The resulting matrix will have
  * the negative of the inverse distance on off diagonals. And the sum
  * of the positive inverse distance of the off diagonals on the diagonal.
  * If there are no off diagonal entries in the stencil, the diagonal is
  * set to 0.
  *
  * \param[in]     dim     Number of physical dimensions (2D or 3D?).
  * \param[in]     coords  A vector containing the coordinates, with the <code>i</code>-th
  *                        coordinate beginning at <code>coords[i*dim]</code>.
  * \param[in]     stencil The stencil matrix used to describe the connectivity
  *                        of the graph Laplacian matrix.
  * \param[in,out] gl      The graph Laplacian matrix to be filled according
  *                        to the <code>stencil</code> matrix.
  *
  * \pre Assumes the <code>gl</code> argument is constructed to have a row map
  *      equivalent in size to <code>stencil</code>
  */
void buildGraphLaplacian(int dim,double * coords,const Epetra_CrsMatrix & stencil,Epetra_CrsMatrix & gl)
{
   // allocate an additional value for the diagonal, if neccessary
   double rowData[stencil.GlobalMaxNumEntries()+1];
   int rowInd[stencil.GlobalMaxNumEntries()+1];

   // loop over all the rows
   for(int j=0;j<gl.NumMyRows();j++) {
      int row = gl.GRID(j);
      double diagValue = 0.0;
      int diagInd = -1;
      int rowSz = 0;

      // extract a copy of this row...put it in rowData, rowIndicies
      stencil.ExtractGlobalRowCopy(row,stencil.MaxNumEntries(),rowSz,rowData,rowInd);
 
      // loop over elements of row
      for(int i=0;i<rowSz;i++) {
         int col = rowInd[i];

         // for nondiagonal entries
         if(row!=col) {
            double d = dist(dim,coords,row,col);
            rowData[i] = -1.0/d;
            diagValue += rowData[i];
         }
         else 
            diagInd = i;
      }
    
      // handle diagonal entry
      if(diagInd<0) { // diagonal not in row
         rowData[rowSz] = -diagValue;
         rowInd[rowSz] = row;
         rowSz++;
      }
      else { // diagonal in row
         rowData[diagInd] = -diagValue;
         rowInd[diagInd] = row;
      }

      // insert row data into graph Laplacian matrix
      gl.InsertGlobalValues(row,rowSz,rowData,rowInd);
   }

   gl.FillComplete();
}

//! Build an inverse operator using a factory and a linear operator
LinearOp buildInverse(const InverseFactory & factory,const LinearOp & A)
{
   // build and initialize inverse linear op with solve
   Teuchos::RCP<Thyra::LinearOpWithSolveBase<double> > invALOWS = factory->createOp();
   factory->initializeOp(Thyra::defaultLinearOpSource(A),&*invALOWS,Thyra::SUPPORT_SOLVE_FORWARD_ONLY);
   
   return Thyra::inverse<double>(invALOWS);
}

/** Using a prebuilt linear operator, use factory to build an inverse operator
  * given a new forward operator.
  */
void rebuildInverse(const InverseFactory & factory, const LinearOp & A, LinearOp & invA)
{
   TEUCHOS_ASSERT(false);
}

/** \brief Apply a linear operator to a multivector (think of this as a matrix
  *        vector multiply).
  *
  * Apply a linear operator to a multivector. This also permits arbitrary scaling
  * and addition of the result. This function gives
  *     
  *    \f$ y = \alpha A x + \beta y \f$
  *
  * \param[in]     A
  * \param[in]     x
  * \param[in,out] y
  * \param[in]     \alpha
  * \param[in]     \beta
  *
  */
void applyOp(const LinearOp & A,const MultiVector & x,MultiVector & y,double alpha,double beta)
{
   Thyra::apply(*A,Thyra::NONCONJ_ELE,*x,&*y,alpha,beta);
}

/** \brief Update the <code>y</code> vector so that \f$y = \alpha x+\beta y\f$
  */
void update(double alpha,const MultiVector & x,double beta,MultiVector & y)
{
   Teuchos::Array<double> scale;
   Teuchos::Array<Teuchos::Ptr<const Thyra::MultiVectorBase<double> > >  vec;

   // build arrays needed for linear combo
   scale.push_back(alpha);
   vec.push_back(x.ptr());

   // compute linear combination
   Thyra::linear_combination<double>(scale,vec,beta,y.ptr());
}

//! Get the strictly upper triangular protion of the matrix
BlockedLinearOp getUpperTriBlocks(const BlockedLinearOp & blo)
{
   int rows = blockRowCount(blo);

   TEUCHOS_ASSERT(rows==blockColCount(blo));

   RCP<const Thyra::ProductVectorSpaceBase<double> > range = blo->productRange();
   RCP<const Thyra::ProductVectorSpaceBase<double> > domain = blo->productDomain();

   // allocate new operator
   BlockedLinearOp upper = createNewBlockedOp();
 
   // build new operator 
   upper->beginBlockFill(rows,rows);

   for(int i=0;i<rows;i++) {
      // put zero operators on the diagonal
      // this gurantees the vector space of
      // the new operator are fully defined
      RCP<const Thyra::LinearOpBase<double> > zed 
            = Thyra::zero<double>(range->getBlock(i),domain->getBlock(i));
      upper->setBlock(i,i,zed);

      for(int j=i+1;j<rows;j++) {
         // get block i,j
         LinearOp uij = blo->getBlock(i,j);

         // stuff it in U
         if(uij!=Teuchos::null)
            upper->setBlock(i,j,uij);
      }
   }
   upper->endBlockFill();

   return upper;
}

}
