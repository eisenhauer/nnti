/* @HEADER@ */
/* @HEADER@ */

#include "SundanceTriangleWriter.hpp"
#include "SundanceExceptions.hpp"
#include "SundanceOut.hpp"
#include "SundanceTabs.hpp"


using namespace SundanceUtils;
using namespace SundanceStdMesh;
using namespace SundanceStdMesh::Internal;
using namespace Teuchos;
using namespace TSFExtended;

void TriangleWriter::write() const 
{
  string f = filename();
  if (nProc() > 1) f = f + Teuchos::toString(myRank());

  /* write header information on root proc only */
  if (nProc() > 1 && myRank()==0) writeHeader(filename());

  /* write local mesh on all procs */
  writePoints(f);
  writeCells(f);
  writeEdges(f);
  if (mesh().spatialDim() > 2)
    {
      writeFaces(f);
    }
  if (nProc() > 1) writeParallelInfo(f);
}

void TriangleWriter::writeHeader(const string& filename) const 
{
  string hdrfile = filename + ".hdr";
  ofstream os(hdrfile.c_str());

  os << nProc() << endl;
  for (int p=0; p<nProc(); p++) 
    {
      os << filename + Teuchos::toString(p) << endl;
    }

  os << pointScalarNames().length() << endl;
  for (int i=0; i<pointScalarNames().length(); i++)
    {
      os << i << " " << pointScalarNames()[i] << endl;
    }
  os << cellScalarNames().length() << endl;
  for (int i=0; i<cellScalarNames().length(); i++)
    {
      os << i << " " << cellScalarNames()[i] << endl;
    }
  
  for (int i=0; i<comments().length(); i++)
    {
      os << "# " << comments()[i] << endl;
    }
}


void TriangleWriter::writePoints(const string& filename) const 
{
  int nPts = mesh().numCells(0);
  int dim = mesh().spatialDim();
  int nAttr = pointScalarFields().length();
  int nBdryMarkers = 0;

  string nodefile = filename + ".node";
  ofstream os(nodefile.c_str());

  os << nPts << " " << dim << " " << nAttr << " " << nBdryMarkers << endl;

  for (int i=0; i<nPts; i++)
    {
      os << i+indexOffset_;
      const Point& x = mesh().nodePosition(i);
      for (int d=0; d<dim; d++)
        {
          os << " " << x[d];
        }
      /*
      for (int f=0; f<nAttr; f++)
        {
          double val = pointScalarFields()[f].probeAtMeshPoint(i);
          os << " " << val;
        }
      */
      for (int b=0; b<nBdryMarkers; b++)
        {
          SUNDANCE_ERROR("Boundary markers not supported yet");
        }
      os << endl;
    }
  
  for (int i=0; i<comments().length(); i++)
    {
      os << "# " << comments()[i] << endl;
    }
}

void TriangleWriter::writeFaces(const string& filename) const 
{
  string facefile = filename + ".face";
  ofstream os(facefile.c_str());

  int dim = 2;
  int nFaces = mesh().numCells(dim);

  os << nFaces << " 0" << endl;

  for (int c=0; c<nFaces; c++)
    {
      os << c + indexOffset_;
      int nNodes = 3;
      
      for (int i=0; i<nNodes; i++)
        {
          os << " " << mesh().facetLID(2,c,0,i) + indexOffset_;
        }
      os << endl;
    }
  
  for (int i=0; i<comments().length(); i++)
    {
      os << "# " << comments()[i] << endl;
    }
}

void TriangleWriter::writeEdges(const string& filename) const 
{
  string edgefile = filename + ".edge";
  ofstream os(edgefile.c_str());

  int dim = 1;
  int nEdges = mesh().numCells(dim);
  int nNodes = 2;

  os << nEdges << " 0" << endl;

  for (int c=0; c<nEdges; c++)
    {
      os << c + indexOffset_;
      for (int i=0; i<nNodes; i++)
        {
          os << " " << mesh().facetLID(1,c,0,i) + indexOffset_;
        }
      os << endl;
    }
  
  for (int i=0; i<comments().length(); i++)
    {
      os << "# " << comments()[i] << endl;
    }
}

void TriangleWriter::writeCells(const string& filename) const 
{
  string elefile = filename + ".ele";
  ofstream os(elefile.c_str());

  int dim = mesh().spatialDim();
  int nCells = mesh().numCells(dim);
  int nAttr = cellScalarFields().length();

  os << nCells << " " << dim+1 << " " << nAttr << endl;

  for (int c=0; c<nCells; c++)
    {
      os << c + indexOffset_;
      int nNodes = dim+1;
      
      for (int i=0; i<nNodes; i++)
        {
          os << " " << mesh().facetLID(dim,c,0,i) + indexOffset_;
        }
      /*
      for (int f=0; f<nAttr; f++)
        {
          os << " " << cellScalarFields()[f].average(cell).value();
        }
      */
      os << endl;
    }
  
  for (int i=0; i<comments().length(); i++)
    {
      os << "# " << comments()[i] << endl;
    }
}


void TriangleWriter::writeParallelInfo(const string& filename) const 
{
  string parfile = filename + ".par";
  ofstream os(parfile.c_str());

  int dim = mesh().spatialDim();
  int nCells = mesh().numCells(dim);
  int nEdges = mesh().numCells(1);
  int nPts = mesh().numCells(0);

  os << myRank() << " " << nProc() << endl;

  os << nPts << endl;
  for (int i=0; i<nPts; i++)
    {
      os << i << " " << mesh().mapLIDToGID(0,i) 
         << " " << mesh().ownerProcID(0,i) << endl;
    }

  os << nCells << endl;
  for (int c=0; c<nCells; c++)
    {
      os << c << " " << mesh().mapLIDToGID(dim,c) 
         << " " << mesh().ownerProcID(dim,c) << endl;
    }

  os << nEdges << endl;
  for (int c=0; c<nEdges; c++)
    {
      os << c << " " << mesh().mapLIDToGID(1,c) 
         << " " << mesh().ownerProcID(1,c) << endl;
    }

  if (dim > 2)
    {
      int nFaces = mesh().numCells(2);
      os << nFaces << endl;
      for (int c=0; c<nFaces; c++)
        {
          os << c << " " << mesh().mapLIDToGID(2,c) 
             << " " << mesh().ownerProcID(2,c) << endl;
        }
    }
  
  for (int i=0; i<comments().length(); i++)
    {
      os << "# " << comments()[i] << endl;
    }
}


