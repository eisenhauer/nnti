//@HEADER
/*
************************************************************************

              Isorropia: Partitioning and Load Balancing Package
                Copyright (2006) Sandia Corporation

Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
license for use of this work by or on behalf of the U.S. Government.

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA
Questions? Contact Alan Williams (william@sandia.gov)
                or Erik Boman    (egboman@sandia.gov)

************************************************************************
*/
//@HEADER

#include <ispatest_lbeval_utils.hpp>

#ifdef HAVE_MPI
#include <mpi.h>
#endif

#ifdef HAVE_EPETRA
#include <Epetra_Comm.h>
#include <Epetra_BlockMap.h>
#include <Epetra_CrsGraph.h>
#include <Epetra_CrsMatrix.h>

#ifdef HAVE_MPI
#include <Epetra_MpiComm.h>
#else
#include <Epetra_SerialComm.h>
#endif

#endif

#include <map>
#include <set>
#include <iostream>

namespace ispatest {

#ifdef HAVE_EPETRA

static double compute_balance(const Epetra_Comm &comm, int myRows, int nwgts, float *wgts);
static void printMatrix(const char *txt, int *myA, int *myX, int *myB,
                        int numRows, int numCols, const Epetra_Comm &comm);
static int make_my_A(const Epetra_RowMatrix &matrix, int *myA, const Epetra_Comm &comm);

/******************************************************************
  Compute graph metrics
******************************************************************/

int compute_graph_metrics(const Epetra_RowMatrix &matrix,
            Isorropia::Epetra::CostDescriber &costs,
            double &balance, int &numCuts, double &cutWgt, double &cutn, double &cutl)
{
  const Epetra_BlockMap &rowmap = matrix.RowMatrixRowMap();
  const Epetra_BlockMap &colmap = matrix.RowMatrixColMap();

  int maxEdges = colmap.NumMyElements();

  std::vector<std::vector<int> > myRows(rowmap.NumMyElements());

  if (maxEdges > 0){
    int numEdges = 0;
    int *nborLID  = new int [maxEdges];
    double *tmp = new double [maxEdges];

    for (int i=0; i<rowmap.NumMyElements(); i++){

      matrix.ExtractMyRowCopy(i, maxEdges, numEdges, tmp, nborLID);
      std::vector<int> cols(numEdges);
      for (int j=0; j<numEdges; j++){
        cols[j] = nborLID[j];
      }
      myRows[i] = cols;
    }
    delete [] nborLID;
  }
 
  return compute_graph_metrics(rowmap, colmap, myRows, costs,
                               balance, numCuts, cutWgt, cutn, cutl);

}

int compute_graph_metrics(const Epetra_CrsGraph &graph,
            Isorropia::Epetra::CostDescriber &costs,
            double &balance, int &numCuts, double &cutWgt, double &cutn, double &cutl)
{
  const Epetra_BlockMap &rowmap = graph.RowMap();
  const Epetra_BlockMap &colmap = graph.ColMap();

  int maxEdges = colmap.NumMyElements();

  std::vector<std::vector<int> > myRows(rowmap.NumMyElements());

  if (maxEdges > 0){
    int numEdges = 0;
    int *nborLID  = new int [maxEdges];
    for (int i=0; i<rowmap.NumMyElements(); i++){

      graph.ExtractMyRowCopy(i, maxEdges, numEdges, nborLID);
      std::vector<int> cols(numEdges);
      for (int j=0; j<numEdges; j++){
        cols[j] = nborLID[j];
      }
      myRows[i] = cols;
    }
    delete [] nborLID;
  }
 
  return compute_graph_metrics(rowmap, colmap, myRows, costs,
                               balance, numCuts, cutWgt, cutn, cutl);

}

int compute_graph_metrics(const Epetra_BlockMap &rowmap,
                          const Epetra_BlockMap &colmap,
                          std::vector<std::vector<int> > &rows,
                          Isorropia::Epetra::CostDescriber &costs,
            double &balance, int &numCuts, double &cutWgt, double &cutn, double &cutl)
{
  const Epetra_Comm &comm  = rowmap.Comm();
  int myProc = comm.MyPID();
  int myRows = rowmap.NumMyElements();
  int rc;
  int *vgid = NULL;
  float *vwgt = NULL;

  // Compute the balance

  int numVWgts = costs.getNumVertices();

  if ((numVWgts > 0) && (numVWgts != myRows)){
    std::cerr << numVWgts << " row weights for " << myRows << "rows" << std::endl;
    return -1;
  }

  if (numVWgts > 0){
    vgid = new int [numVWgts];
    vwgt = new float [numVWgts];

    costs.getVertexWeights(numVWgts, vgid, vwgt);

    delete [] vgid;
  }

  balance = compute_balance(comm, myRows, numVWgts, vwgt);

  if (vwgt) delete [] vwgt;

  // Compute the measures based on cut edges

  int haveEdgeWeights = costs.haveGraphEdgeWeights();

  int localNumCuts = 0;
  double localCutWgt = 0.0;
  double localCutn = 0.0;
  double localCutl = 0.0;

  int maxEdges = colmap.NumMyElements();

  if (maxEdges > 0){
    std::map<int, float> weightMap;

    // Get the processes owning my vertices neighbors

    int numCols = colmap.NumMyElements();
    const int *colGIDs = colmap.MyGlobalElements();
    int *nborProc_GID = new int [numCols];
    int *nborRow_LID = new int [numCols];

    rc = rowmap.RemoteIDList(numCols, colGIDs, nborProc_GID, nborRow_LID);

    if (rc != 0){
      std::cout << "Error obtaining remote process ID list";
      std::cout << std::endl;
      delete [] nborProc_GID;
      delete [] nborRow_LID;
      return -1;
    }

    std::map<int, int> colProc;
    std::map<int, int>::iterator procIter;

    for (int j=0; j<numCols; j++){

      // map from column GID to process owning row with that GID 
      //   (matrix is square)

      colProc[colGIDs[j]] = nborProc_GID[j];
    }
    delete [] nborProc_GID;
    delete [] nborRow_LID;

    for (int i=0; i < rowmap.NumMyElements(); i++){
      int vtxGID = rowmap.GID(i);

      if (haveEdgeWeights){
        costs.getGraphEdgeWeights(vtxGID, weightMap);
      }

      int numEdges = rows[i].size();

      if (numEdges > 0){

        // get processes that own my neighbors
  
        std::set<int> nbors;
        float heWeight = 0.0;

        for (int j=0; j < numEdges; j++){

          int nborGID = colGIDs[rows[i][j]];

          if (nborGID == vtxGID) continue;  // skip self edges

          procIter = colProc.find(nborGID);
          if (procIter == colProc.end()){
            std::cout << "process owning column is missing";
            std::cout << std::endl;
            return -1;
          }
          int procNum = procIter->second;

          float wgt = 1.0;
          if (haveEdgeWeights){
            std::map<int, float>::iterator curr = weightMap.find(nborGID);
            if (curr == weightMap.end()){
              std::cout << "Graph edge weights do not match matrix";
              std::cout << std::endl;
              return -1;
            }
            wgt = curr->second;
          }
    
          if (procNum != myProc){
            localNumCuts++;            // number of graph edges that are cut 
            nbors.insert(procNum);     // count number of neighboring processes
            localCutWgt += wgt;        // sum of weights of cut edges
          }
          heWeight += wgt;             // implied hyperedge weight
        }
        int numNbors = nbors.size();

        if (numNbors > 0){
          // sum of the implied hyperedge weights of cut hyperedges
          localCutn += heWeight;   

          // sum of (number of partitions - 1) weighted by the 
          // implied hyperedge weight
          localCutl += (numNbors * heWeight);
        }
      }
    } // next vertex in my partition
  }

  double lval[4], gval[4];

  lval[0] = (double)localNumCuts;
  lval[1] = localCutWgt;
  lval[2] = localCutn;
  lval[3] = localCutl;

  comm.SumAll(lval, gval, 4);

  numCuts = (int)gval[0];
  cutWgt = gval[1];
  cutn   = gval[2];
  cutl   = gval[3];

  return 0;
}

/******************************************************************
  Compute hypergraph metrics
******************************************************************/

int compute_hypergraph_metrics(const Epetra_RowMatrix &matrix,
            Isorropia::Epetra::CostDescriber &costs,
            double &balance, double &cutn, double &cutl)  // output
{
  return compute_hypergraph_metrics(matrix.RowMatrixRowMap(), matrix.RowMatrixColMap(),
                                     matrix.NumGlobalCols(),
                                     costs, balance, cutn, cutl);
}
int compute_hypergraph_metrics(const Epetra_CrsGraph &graph,
            Isorropia::Epetra::CostDescriber &costs,
            double &balance, double &cutn, double &cutl)  // output
{
  return compute_hypergraph_metrics(graph.RowMap(), graph.ColMap(),
                                     graph.NumGlobalCols(),
                                     costs, balance, cutn, cutl);
}
int compute_hypergraph_metrics(const Epetra_BlockMap &rowmap, const Epetra_BlockMap &colmap,
            int numGlobalColumns,
            Isorropia::Epetra::CostDescriber &costs,
            double &balance, double &cutn, double &cutl)  // output
{
  const Epetra_Comm &comm  = rowmap.Comm();
#ifdef HAVE_MPI
  const Epetra_MpiComm* mpiComm =
    dynamic_cast<const Epetra_MpiComm*>(&comm);

  MPI_Comm mcomm = mpiComm->Comm();
#endif
  int nProcs = comm.NumProc();
  int myProc = comm.MyPID();
  int myRows = rowmap.NumMyElements();
  int rc;
  int *vgid = NULL;
  float *vwgt = NULL;

  int numVWgts = costs.getNumVertices();

  if ((numVWgts > 0) && (numVWgts != myRows)){
    std::cout << "length of row (vertex) weights array is not equal to number of rows";
    std::cout << std::endl;
    return -1;
  }

  if (numVWgts > 0){
    vgid = new int [numVWgts];
    vwgt = new float [numVWgts];

    costs.getVertexWeights(numVWgts, vgid, vwgt);

    delete [] vgid;
  }

  balance = compute_balance(comm, myRows, numVWgts, vwgt);

  if (vwgt) delete [] vwgt;

  /* Compute cutl and cutn. 
   */

  int totalHEWeights = 0; 

  int numHEWeights = costs.getNumHypergraphEdgeWeights();

  comm.SumAll(&numHEWeights, &totalHEWeights, 1);
 
  if ((totalHEWeights > 0) && (totalHEWeights <  numGlobalColumns)){
    if (myProc == 0)
      std::cerr << "Must supply either no h.e. weights or else supply at least one for each column" << std::endl;
      return -1;
  }

  int *heGIDs = NULL;
  float *heWeights = NULL;

  if (numHEWeights){
    heGIDs = new int [numHEWeights];
    heWeights = new float [numHEWeights];

    costs.getHypergraphEdgeWeights(numHEWeights, heGIDs, heWeights);
  }

  // Create a map from column global IDs to edge weight.  We assume each
  // edge weight is supplied by only one process.  We don't do the
  // ZOLTAN_EDGE_WEIGHT_OP operation.  TODO

  std::map<int, double> heWgt;
  std::map<int, double>::iterator heWgtIter;

  if (numHEWeights){
    for (int j=0; j<numHEWeights; j++){
      heWgt[heGIDs[j]] = heWeights[j];
    }
    delete [] heGIDs;
    delete [] heWeights;
  }

  // Create a set containing all the columns in my rows.  We assume all
  // the rows are in the same partition.

  int numMyCols = colmap.NumMyElements();

  std::set<int> colGIDS;
  std::set<int>::iterator gidIter;

  for (int j=0; j<numMyCols; j++){
    colGIDS.insert(colmap.GID(j));
  }
  
  /* Divide columns among processes, then each process computes its
   * assigned columns' cutl and cutn.
   */
  int ncols = numGlobalColumns / nProcs;
  int leftover = numGlobalColumns - (nProcs * ncols);
  std::vector<int> colCount(nProcs, 0);
  for (int i=0; i<nProcs; i++){
    colCount[i] = ncols;
    if (i < leftover) colCount[i]++;
  }
  int *colTotals = NULL;
  double *colWeights = NULL;
  if (colCount[myProc] > 0){
    colTotals = new int [colCount[myProc]];
    if (totalHEWeights > 0){
      colWeights = new double [colCount[myProc]];
    } 
  }
  int *colLocal= new int [ncols + 1];
  double *localWeights = NULL;
  if (totalHEWeights > 0){
    localWeights = new double [ncols + 1];
  }

  int base = colmap.IndexBase();
  int colStart = base;

  for (int i=0; i<nProcs; i++){

    // All processes send info to the process reponsible
    // for the next group of columns

    int ncols = colCount[i];
    int colEnd = colStart + ncols;
    for (int j=colStart,k=0; j < colEnd; j++,k++){
      gidIter = colGIDS.find(j);
      if (gidIter != colGIDS.end()){
        colLocal[k] = 1;     // column j has rows in my partition
      }
      else{
        colLocal[k] = 0;
      }
      if (totalHEWeights > 0){
        heWgtIter = heWgt.find(j);
        if (heWgtIter != heWgt.end()){
          // I have the edge weight for column j
          localWeights[k] = heWgtIter->second;
        }
        else{
          localWeights[k] = 0.0;
        }
      }
      
    }
#ifdef HAVE_MPI
    rc = MPI_Reduce(colLocal, colTotals, ncols, MPI_INT, MPI_SUM, i, mcomm);
    if (totalHEWeights > 0){
      rc = MPI_Reduce(localWeights, colWeights, ncols, MPI_DOUBLE, MPI_SUM, i, mcomm);
    }
#else
    memcpy(colTotals, colLocal, ncols * sizeof(int));
    if (totalHEWeights > 0){
      memcpy(colWeights, localWeights, ncols * sizeof(double));
    }
#endif
    colStart = colEnd;
  }

  delete [] colLocal;
  if (localWeights) delete [] localWeights;

  double localCutN=0;
  double localCutL=0;
  double ewgt = 1.0;

  for (int j=0; j<colCount[myProc]; j++){
    if (totalHEWeights > 0){
      ewgt = colWeights[j];
    }
    if (colTotals[j] > 1){
      localCutL += (colTotals[j] - 1) * ewgt; // # of cuts in columns/edges
      localCutN += ewgt;                      // # of cut columns/edges
    }
  }
  if (colTotals) delete [] colTotals;
  if (colWeights) delete [] colWeights;

  comm.SumAll(&localCutN, &cutn, 1);
  comm.SumAll(&localCutL, &cutl, 1);

  return 0;
}
static double compute_balance(const Epetra_Comm &comm, int myRows, int nwgts, float *wgts)
{
  int nProcs = comm.NumProc();
  int myProc = comm.MyPID();
  double weightTotal, balance;

  /* Proportion of weight desired in each partition.  For now we
     have no interface to specify unequal partitions.
   */
  double partSize = 1.0 / nProcs;
  std::vector<double> partSizes(nProcs, partSize);

  /* Sum of my row weights.  
   */
  double weightLocal = 0.0;

  if (nwgts > 0){
    for (int i=0; i<myRows; i++){
      weightLocal += wgts[i];
    }
  }
  else{
    weightLocal += myRows;   // default weight of each vertex is 1.0
  }

  comm.SumAll(&weightLocal, &weightTotal, 1);

  /* My degree of imbalance
   */
  double goalWeight = partSizes[myProc] * weightTotal;
  double imbalance = 1.0;
  if (weightLocal >= goalWeight)
    imbalance += (weightLocal - goalWeight) / goalWeight;
  else
    imbalance += (goalWeight - weightLocal) / goalWeight;

  comm.MaxAll(&imbalance, &balance, 1);

  return balance;
}
// Print out the matrix showing the partitioning, for debugging purposes.  
// This only works for small example matrices and 10 or fewer processes.

void show_matrix(const char *txt, const Epetra_CrsGraph &graph, const Epetra_Comm &comm)
{
  int me = comm.MyPID();

  if (comm.NumProc() > 10){
    if (me == 0){
      std::cout << txt << std::endl;
      std::cout << "Printed matrix format only works for 10 or fewer processes" << std::endl;
    }
    return;
  }

  const Epetra_BlockMap &rowmap = graph.RowMap();
  const Epetra_BlockMap &colmap = graph.ColMap();

  int myRows = rowmap.NumMyElements();
  int numRows = graph.NumGlobalRows();
  int numCols = graph.NumGlobalCols();
  int base = rowmap.IndexBase();

  int *myA = new int [numRows * numCols];
  memset(myA, 0, sizeof(int) * numRows * numCols);

  int *myIndices;

  int *myRowGIDs = rowmap.MyGlobalElements();

  for (int i=0; i< myRows; i++){
    int myRowLID = rowmap.LID(myRowGIDs[i]);

    int numEntries = graph.NumMyIndices(myRowLID);

    if (numEntries > 0){
      int rc = graph.ExtractMyRowView(myRowLID, numEntries, myIndices);
      if (rc){
        std::cout << txt << std::endl;
        std::cout << "extract graph error" << std::endl;
        return;
      }

      int *row = myA + (numCols * (myRowGIDs[i] - base));

      for (int j=0; j < numEntries; j++){
        int gid = colmap.GID(myIndices[j]);
        row[gid-base] = me+1;
      }
    }
  }

  printMatrix(txt, myA, NULL, NULL, numRows, numCols, comm);

  delete [] myA;
}

void show_matrix(const char *txt, const Epetra_RowMatrix &matrix, const Epetra_Comm &comm)
{
  if (comm.NumProc() > 10){
    if (comm.MyPID() == 0){
      std::cout << txt << std::endl;
      std::cout << "Printed matrix format only works for 10 or fewer processes" << std::endl;
    }
    return;
  }

  int numRows = matrix.NumGlobalRows();
  int numCols = matrix.NumGlobalCols();

  int *myA = new int [numRows * numCols];

  int rc = make_my_A(matrix, myA, comm);

  printMatrix(txt, myA, NULL, NULL, numRows, numCols, comm);

  delete [] myA;
}
void show_matrix(const char *txt, const Epetra_LinearProblem &problem, const Epetra_Comm &comm)
{
  int me = comm.MyPID();

  if (comm.NumProc() > 10){
    if (me == 0){
      std::cout << txt << std::endl;
      std::cout << "Printed matrix format only works for 10 or fewer processes" << std::endl;
    }
    return;
  }

  Epetra_RowMatrix *matrix = problem.GetMatrix();
  Epetra_MultiVector *lhs = problem.GetLHS();
  Epetra_MultiVector *rhs = problem.GetRHS();

  int numRows = matrix->NumGlobalRows();
  int numCols = matrix->NumGlobalCols();

  int *myA = new int [numRows * numCols];

  int rc = make_my_A(*matrix, myA, comm);

  int *myX = new int [numCols];
  int *myB = new int [numRows];

  memset(myX, 0, sizeof(int) * numCols);
  memset(myB, 0, sizeof(int) * numRows);

  const Epetra_BlockMap &lhsMap = lhs->Map();
  const Epetra_BlockMap &rhsMap = rhs->Map();

  int base = lhsMap.IndexBase();

  for (int j=0; j < lhsMap.NumMyElements(); j++){
    int colGID = lhsMap.GID(j);
    myX[colGID - base] = me + 1;
  }

  for (int i=0; i < rhsMap.NumMyElements(); i++){
    int rowGID = rhsMap.GID(i);
    myB[rowGID - base] = me + 1;
  }

  printMatrix(txt, myA, myX, myB, numRows, numCols, comm);

  delete [] myA;
  delete [] myX;
  delete [] myB;
}

static int make_my_A(const Epetra_RowMatrix &matrix, int *myA, const Epetra_Comm &comm)
{
  int me = comm.MyPID();

  const Epetra_BlockMap &rowmap = matrix.RowMatrixRowMap();
  const Epetra_BlockMap &colmap = matrix.RowMatrixColMap();

  int myRows = matrix.NumMyRows();
  int numRows = matrix.NumGlobalRows();
  int numCols = matrix.NumGlobalCols();
  int base = rowmap.IndexBase();
  int maxRow = matrix.MaxNumEntries();

  memset(myA, 0, sizeof(int) * numRows * numCols);

  int *myIndices = new int [maxRow];
  double *tmp = new double [maxRow];

  int rowLen = 0;

  for (int i=0; i< myRows; i++){

    int rc = matrix.ExtractMyRowCopy(i, maxRow, rowLen, tmp, myIndices);

    if (rc){
      if (me == 0){
        std::cout << "Error in make_my_A" << std::endl;
      }
       return 1;
    }

    int *row = myA + (numCols * (rowmap.GID(i) - base));

    for (int j=0; j < rowLen; j++){

      int colGID = colmap.GID(myIndices[j]);
      
      row[colGID - base] = me + 1;
    }
  }

  if (maxRow){
    delete [] myIndices;
    delete [] tmp;
  }
  return 0;
}

static void printMatrix(const char *txt, int *myA, int *myX, int *myB,
                        int numRows, int numCols, const Epetra_Comm &comm)
{
  int me = comm.MyPID();

  int *A = new int [numRows * numCols];
  int *x = NULL;
  int *b = NULL;

  comm.SumAll(myA, A, numRows * numCols);

  if (myX){
    x = new int [numCols];
    comm.SumAll(myX, x, numCols);
  }
  if (myB){
    b = new int [numRows];
    comm.SumAll(myB, b, numRows);
  }

  if (me == 0){
    std::cout << txt << std::endl;

    std::cout << "  ";
    for (int j=0; j<numCols; j++){
      std::cout << j%10 ;
    }
    if (x)
      std::cout << "    LHS";
    if (b)
      std::cout << "        RHS";

    std::cout << std::endl;

    int *row = A;
 
    for (int i=0; i < numRows; i++, row += numCols){
      std::cout << i%10 << " ";
      for (int j=0; j < numCols; j++){
        if (row[j] > 0){
          std::cout << row[j]-1;
        }
        else{
          std::cout << " ";
        }
      }
      std::cout << " " << i%10 ;

      if (x){
        std::cout << "   " << x[i]-1;
      }
      if ((i == 0) && b){
        std::cout << "    =  [";
        for (int j=0; j<numRows; j++){
          std::cout << b[j]-1;
        }
        std::cout << "]";
      }
      std::cout << std::endl;
    
    }
    std::cout << "  ";
    for (int j=0; j<numCols; j++){
      std::cout << j%10 ;
    }

    int columnsRemaining = numCols - numRows;
    int next_x = numRows;

    if ((columnsRemaining > 0) && x){
      std::cout << "     " << x[next_x++] - 1 << std::endl;
      columnsRemaining--;
      int pad = numCols + 7;
      while(columnsRemaining){ 
        for( int i=0; i < pad; i++){
          std::cout << " ";
        }
        std::cout << x[next_x++] - 1 << std::endl;
        columnsRemaining--;
      }
    }
    std::cout << std::endl;
  }

  delete [] A;
  if (x) delete [] x;
  if (b) delete [] b;
}

#endif // HAVE_EPETRA
}
