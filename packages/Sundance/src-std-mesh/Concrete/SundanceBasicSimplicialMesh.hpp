/* @HEADER@ */
/* @HEADER@ */

#ifndef SUNDANCE_BASICSIMPLICIALMESH_H
#define SUNDANCE_BASICSIMPLICIALMESH_H


#ifndef DOXYGEN_DEVELOPER_ONLY


#include "SundanceDefs.hpp"
#include "SundanceMeshBase.hpp"
#include "SundanceBasicVertexSet.hpp"
#include "SundanceArrayOfTuples.hpp"
#include "SundanceCreatableMesh.hpp"
#include "Teuchos_Array.hpp"
#include "Teuchos_Hashtable.hpp"

namespace SundanceStdMesh
{
  namespace Internal
  {
    /**
     * A no-frills simplicial mesh implementation. 
     */
    class BasicSimplicialMesh : public CreatableMesh
    {
    public:
      /** */
      BasicSimplicialMesh(int dim, const MPIComm& comm);

      /** */
      virtual ~BasicSimplicialMesh(){;}

      /** 
       * Get the number of cells having dimension dim
       */
      virtual int numCells(int dim) const  ;

      /** 
       * Return the position of the i-th node
       */
      virtual Point nodePosition(int i) const {return points_[i];}

      /** 
       * Compute the jacobians of a batch of cells, returning the 
       * result via reference argument
       *
       * @param cellDim dimension of the cells whose Jacobians are to
       * be computed
       * @param cellLID local indices of the cells for which Jacobians
       * are to be computed
       * @param jBatch reference to the resulting Jacobian batch
       */
      virtual void getJacobians(int cellDim, const Array<int>& cellLID,
                                CellJacobianBatch& jBatch) const  ;

      /**
       * Map reference quadrature points to physical points on the
       * given cells. 
       */
      virtual void pushForward(int cellDim, const Array<int>& cellLID,
                               const Array<Point>& refQuadPts,
                               Array<Point>& physQuadPts) const ;

      

      /** 
       * Return the rank of the processor that owns the given cell
       */
      virtual int ownerProcID(int cellDim, int cellLID) const  ;

      /** 
       * Return the number of facets of the given cell
       */
      virtual int numFacets(int cellDim, int cellLID, 
                            int facetDim) const  ;

      /** 
       * Return the local ID of a facet cell
       * @param cellDim dimension of the cell whose facets are being obtained
       * @param cellLID local index of the cell whose
       * facets are being obtained
       * @param facetDim dimension of the desired facet
       * @param facetIndex index into the list of the cell's facets
       */
      virtual int facetLID(int cellDim, int cellLID,
                           int facetDim, int facetIndex) const  ;

      /** 
       * Return the number of maximal cofacets of the given cell
       */
      virtual int numCofacets(int cellDim, int cellLID) const  ;

      /** 
       * Return the local ID of a cofacet cell
       * @param cellDim dimension of the cell whose cofacets are being obtained
       * @param cellLID local index of the cell whose
       * cofacets are being obtained
       * @param cofacetIndex index into the list of the cell's facets
       */
      virtual int cofacetLID(int cellDim, int cellLID,
                             int cofacetIndex) const  ;

      /** 
       * Find the local ID of a cell given its global index
       */
      virtual int mapGIDToLID(int cellDim, int globalIndex) const  ;

    

      /** 
       * Find the global ID of a cell given its local index
       */
      virtual int mapLIDToGID(int cellDim, int localIndex) const  ;

      /**
       * Get the type of the given cell
       */
      virtual CellType cellType(int cellDim) const  ;


      /** Get the label of the given cell */
      virtual int label(int cellDim, int cellLID) const ;

      /** */
      virtual void estimateNumVertices(int numVertices);

      /** */
      virtual void estimateNumElements(int numElements);

      /** */
      virtual int addVertex(int globalIndex, const Point& x,
                            int ownerProcID, int label);

      /** */
      virtual int addElement(int globalIndex, const Array<int>& vertices,
                             int ownerProcID, int label);

      /** */
      virtual void assignIntermediateCellOwners(int cellDim) ;

      /** */
      virtual bool hasIntermediateGIDs(int dim) const 
      {
        if (dim==1) return hasEdgeGIDs_;
        return hasFaceGIDs_;
      }

    private:

      int addFace(int v1, int v2, int v3, 
                  int e1, int e2, int e3,
                  int& rotation);

      int addEdge(int v1, int v2);

      int getEdgeLIDFromVertLIDs(int v1, int v2) ;

      int getFaceLIDFromVertLIDs(int v1, int v2, int v3,
                                 int& rotation) ;

      void synchronizeGIDNumbering(int dim, int localCount) ;

      
      

      /** Number of cells for each dimension */
      Array<int> numCells_;
    
      /** coordinates of vertices */
      Array<Point> points_;

      /** pairs of vertex indices for the edges. */
      ArrayOfTuples<int> edgeVerts_;
    
      /** tuples of vertex indices for the faces */
      ArrayOfTuples<int> faceVerts_;
    
      /** tuples of edge indices for the faces */
      ArrayOfTuples<int> faceEdges_;
    
      /** tuples of vertex indices for the elements */
      ArrayOfTuples<int> elemVerts_;

      /** tuples of edge indices for the elements */
      ArrayOfTuples<int> elemEdges_;

      /** tuples of face indices for the elements */
      ArrayOfTuples<int> elemFaces_;

      /** tuples of face rotations for the elements */
      ArrayOfTuples<int> elemFaceRotations_;

      /** table for mapping vertex set -> face index */
      Hashtable<VertexSet, int> vertexSetToFaceIndexMap_;

      /** list of element cofacets for the edges */
      Array<Array<int> > edgeCofacets_;

      /** list of element cofacets for the faces */
      Array<Array<int> > faceCofacets_;

      /** list of edge cofacets for the vertices */
      Array<Array<int> > vertEdges_;

      /** list of maximal cofacets for the vertices */
      Array<Array<int> > vertCofacets_;

      /** list of edge partners for the vertices */
      Array<Array<int> > vertEdgePartners_;

      /** list of edge signs for the vertices */
      Array<Array<int> > vertEdgeSigns_;

      /** workspace for face vertices */
      Array<int> tmpFaceVerts_;

      /** workspace for face edges */
      Array<int> tmpFaceEdges_;

      /** map from local to global vertex indices */
      Array<Array<int> > LIDToGIDMap_;

      /** map from global to local vertex indices */
      Array<Hashtable<int, int> > GIDToLIDMap_;
    
      /** Array of labels for the points */
      Array<Array<int> > labels_;

      /** Array of processor IDs for the vertices */
      Array<Array<int> > ownerProcID_;


      /** utility to set face vertices */
      inline void setWorkFace(int v1, int v2, int v3)
      {
        tmpFaceVerts_[0] = v1;
        tmpFaceVerts_[1] = v2;
        tmpFaceVerts_[2] = v3;
      }

      Array<int*> base_;
      Array<int*> tmpBase_;

      bool hasEdgeGIDs_;
      bool hasFaceGIDs_;

    };
  }

}



#endif /* DOXYGEN_DEVELOPER_ONLY */

#endif
