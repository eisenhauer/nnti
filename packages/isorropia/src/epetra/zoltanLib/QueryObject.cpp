// @HEADER
// ***********************************************************************
//
//            Isorropia: Partitioning and Load Balancing Package
//              Copyright (2006) Sandia Corporation
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
//
// ***********************************************************************
// @HEADER

#include <QueryObject.hpp>
#include <Isorropia_EpetraCostDescriber.hpp>

#include <Epetra_CrsGraph.h>
#include <Epetra_Comm.h>
#include <Epetra_BlockMap.h>
#include <Epetra_Map.h>
#include <Epetra_Vector.h>
#include <Epetra_RowMatrix.h>

#include <map>

// if non-zero, print out query function responses
//#define DEBUG_QUERIES 1
// process to print out responses, or -1 for all
//#define DEBUG_PROC -1

namespace Isorropia{

namespace Epetra {

namespace ZoltanLib{

QueryObject::QueryObject( Teuchos::RCP<const Epetra_CrsGraph> graph,
	   Teuchos::RCP<const Isorropia::Epetra::CostDescriber> costs,
                                     int inputType) 
  : graph_(graph),
    rowMap_(&(graph->RowMap())),
    colMap_(&(graph->ColMap())),
    costs_(costs),
    haveGraph_(true) ,
    input_type_(inputType) 
{
  myProc_ = graph->Comm().MyPID();
  base_ = rowMap_->IndexBase();
  int rc = 0;
 
  if (input_type_ == graph_input_){

    // graph queries need to know processes owning my column entries
    fill_procmap();

    if (graph->NumMyDiagonals() > 0){
      // In graph partitioning, we need to omit graph diagonal entries
      // (self edges) from the query functions.

      int nRows = rowMap_->NumMyElements();
      int *rowGIDs = rowMap_->MyGlobalElements();

      for (int i=0; i < nRows; i++){

	int numEntries;
	int *idx;

	rc = graph->ExtractMyRowView(i, numEntries, idx);

	for (int j=0; j<numEntries; j++){
	  if (rowGIDs[i] == colMap_->GID(idx[j])){
	    graph_self_edges_.insert(rowGIDs[i]);
	  }
	}
      }
    }
  }
}

QueryObject::QueryObject( Teuchos::RCP<const Epetra_RowMatrix> matrix,
	     Teuchos::RCP<const Isorropia::Epetra::CostDescriber> costs,
                                 int inputType) 
  : matrix_(matrix),
    rowMap_((const Epetra_BlockMap*)&(matrix->RowMatrixRowMap())),
    colMap_((const Epetra_BlockMap*)&(matrix->RowMatrixColMap())),
    costs_(costs),
    haveGraph_(false),
    input_type_(inputType) 
{
  myProc_ = matrix->Comm().MyPID();
  base_ = rowMap_->IndexBase();

  if (input_type_ == graph_input_){

    // graph queries need to know processes owning my column entries
    fill_procmap();

    if (matrix->NumMyDiagonals() > 0){
      // In graph partitioning, we need to omit graph diagonal entries
      // (self edges) from the query functions.

      int *rowGIDs;
      int nRows;

      Epetra_Vector diagonal(matrix->RowMatrixRowMap());
      nRows = rowMap_->NumMyElements();
      rowGIDs = rowMap_->MyGlobalElements();
      matrix->ExtractDiagonalCopy(diagonal);

      for (int i=0; i < nRows; i++){
	if (diagonal[i] != 0){
	  graph_self_edges_.insert(rowGIDs[i]);
	}
      }
    }
  }
  else{
    input_type_ = hgraph_input_;
  }
}

QueryObject::QueryObject( Teuchos::RCP<const Epetra_MultiVector> coords,
                          Teuchos::RCP<const Epetra_MultiVector> weights)
  : coords_(coords),
    weights_(weights),
    map_(&(coords->Map())),
    haveGraph_(false)
{
  myProc_ = map_->Comm().MyPID();
  base_ = map_->IndexBase();
  input_type_ = geometric_input_;
}

QueryObject::~QueryObject()
{
}

// Create a map required by graph queries

void QueryObject::fill_procmap()
{
  int num_colmap_elems = colMap_->NumMyElements();

  int *colIDs = new int [num_colmap_elems];
  int *procIDs = new int [num_colmap_elems];
  int *tmp = new int [num_colmap_elems];

  colMap_->MyGlobalElements(colIDs);

  rowMap_->RemoteIDList(num_colmap_elems, colIDs, procIDs, tmp);

  delete [] tmp;

  int i;
  for(i=0; i<num_colmap_elems; ++i) {

    // map from global column ID to process owning the row
    // with that global ID (this is a square matrix)

    procmap_[colIDs[i]] = procIDs[i];
  }

  delete [] colIDs;
  delete [] procIDs;
}

bool QueryObject::haveVertexWeights()
{
  return costs_->haveGlobalVertexWeights();
}
bool QueryObject::haveGraphEdgeWeights()
{
  return costs_->haveGlobalGraphEdgeWeights();
}
bool QueryObject::haveHypergraphEdgeWeights()
{
  return costs_->haveGlobalHypergraphEdgeWeights();
}

// Static query functions.  These will call the query function of the
// appropriate QueryObject object.

int QueryObject::Number_Objects(void *data, int *ierr)
{
  int numObj = 0;

  QueryObject *zq = (QueryObject *)data;

  if (zq){
    numObj = zq->My_Number_Objects(ierr);
  }
  else{
    *ierr = ZOLTAN_FATAL;
  }

  return numObj;
}
void QueryObject::Object_List  ( void * data,
		   int num_gid_entries, int num_lid_entries,
		   ZOLTAN_ID_PTR global_ids, ZOLTAN_ID_PTR local_ids,
		   int weight_dim, float * object_weights, int * ierr )
{
  QueryObject *zq = (QueryObject *)data;

  if (zq){
    zq->My_Object_List(num_gid_entries, num_lid_entries,
		    global_ids, local_ids, weight_dim, object_weights, ierr);
  }
  else{
    *ierr = ZOLTAN_FATAL;
  }

  return;
}
void QueryObject::Number_Edges_Multi  ( void * data,
	     int num_gid_entries, int num_lid_entries,
	     int num_obj,
	     ZOLTAN_ID_PTR global_ids, ZOLTAN_ID_PTR local_ids,
	     int *num_edges, int * ierr )
{
  QueryObject *zq = (QueryObject *)data;

  if (zq){
    zq->My_Number_Edges_Multi(num_gid_entries, num_lid_entries, num_obj,
	     global_ids, local_ids, num_edges, ierr );
  }
  else{
    *ierr = ZOLTAN_FATAL;
  }

  return;
}
void QueryObject::Edge_List_Multi( void * data,
	       int num_gid_entries, int num_lid_entries, int num_obj,
	       ZOLTAN_ID_PTR global_ids, ZOLTAN_ID_PTR local_ids, int *num_edges,
	       ZOLTAN_ID_PTR neighbor_global_ids, int * neighbor_procs,
	       int weight_dim, float * edge_weights, int * ierr )
{
  QueryObject *zq = (QueryObject *)data;

  if (zq){
    zq->My_Edge_List_Multi(num_gid_entries, num_lid_entries, num_obj,
	       global_ids, local_ids, num_edges,
	       neighbor_global_ids,  neighbor_procs,
	       weight_dim, edge_weights, ierr );
  }
  else{
    *ierr = ZOLTAN_FATAL;
  }

  return;
}
void QueryObject::HG_Size_CS ( void * data,
	 int* num_lists, int* num_pins, int* format, int * ierr )
{
  QueryObject *zq = (QueryObject *)data;

  if (zq){
    zq->My_HG_Size_CS(num_lists, num_pins, format, ierr );
  }
  else{
    *ierr = ZOLTAN_FATAL;
  }
}
void QueryObject::HG_CS ( void * data,
	    int num_gid_entries, int num_row_or_col, int num_pins, int format,
	    ZOLTAN_ID_PTR vtxedge_GID, int* vtxedge_ptr, ZOLTAN_ID_PTR pin_GID,
				     int * ierr )
{
  QueryObject *zq = (QueryObject *)data;

  if (zq){
    zq->My_HG_CS(num_gid_entries, num_row_or_col, num_pins, format,
	    vtxedge_GID, vtxedge_ptr, pin_GID, ierr );
  }
  else{
    *ierr = ZOLTAN_FATAL;
  }
}
void QueryObject::HG_Size_Edge_Weights(void * data,
			    int* num_edges, int* ierr)
{
  QueryObject *zq = (QueryObject *)data;

  if (zq){
    zq->My_HG_Size_Edge_Weights(num_edges, ierr);
  }
  else{
    *ierr = ZOLTAN_FATAL;
  }
}
void QueryObject::HG_Edge_Weights(void * data,
      int num_gid_entries, int num_lid_entries, int num_edges, int edge_weight_dim,
      ZOLTAN_ID_PTR edge_GID, ZOLTAN_ID_PTR edge_LID, float* edge_weights, int* ierr)
{
  QueryObject *zq = (QueryObject *)data;

  if (zq){
    zq->My_HG_Edge_Weights(num_gid_entries, num_lid_entries, num_edges, 
         edge_weight_dim, edge_GID, edge_LID, edge_weights, ierr);
  }
  else{
    *ierr = ZOLTAN_FATAL;
  }
}

int QueryObject::Number_Geom(void *data, int *ierr)
{
  int dim=0;
  QueryObject *zq = (QueryObject *)data;

  if (zq){
    dim = zq->My_Number_Geom(ierr);
  }
  else{
    *ierr = ZOLTAN_FATAL;
  }
  return dim;
}

void QueryObject::Geom_Multi(void *data, int num_gid_entries, int num_lid_entries,
        int num_obj, ZOLTAN_ID_PTR gids, ZOLTAN_ID_PTR lids, int num_dim,
        double *geom_vec, int *ierr)
{
  QueryObject *zq = (QueryObject *)data;

  if (zq){
    zq->My_Geom_Multi(num_gid_entries, num_lid_entries,
        num_obj, gids, lids, num_dim, geom_vec, ierr);
  }
  else{
    *ierr = ZOLTAN_FATAL;
  }
}

// Member general query functions.

int QueryObject::My_Number_Objects(int * ierr )
{
  *ierr = ZOLTAN_OK;

  if (input_type_ == geometric_input_){
    return coords_->MyLength();
  }
  else if (input_type_ == hgraph2d_finegrain_input_){
    if (!haveGraph_)
    {
      return matrix_->NumMyNonzeros();
    }
    else
    {
      return graph_->NumMyNonzeros();
    }
  }
  else{
    return rowMap_->NumMyElements();  // graph or hypergraph
  }
}

void QueryObject::My_Object_List(int num_gid_entries, int num_lid_entries,
		  ZOLTAN_ID_PTR global_ids, ZOLTAN_ID_PTR local_ids,
		  int weight_dim, float * object_weights, int * ierr )
{
  *ierr = ZOLTAN_OK;
  int ngids = 0;

  if (input_type_ == geometric_input_){
    ngids = map_->NumMyElements();
    map_->MyGlobalElements( ((int *) global_ids) );
  }
  else if ((input_type_ == hgraph_input_) || (input_type_ == graph_input_)){
    ngids = rowMap_->NumMyElements();
    rowMap_->MyGlobalElements( ((int *) global_ids) );
  }
  else if(input_type_ == hgraph2d_finegrain_input_)
  {
    // if statement for now, maybe set function pointer for different function instead

    int numRows = rowMap_->NumMyElements();

    object_weights = 0;

    if (!haveGraph_) //matrix
    {
      int ngids = 0;
      for (int rowNum=0; rowNum<numRows; rowNum++)
      {
        int rowSize;
        matrix_->NumMyRowEntries(rowNum,rowSize);

        //MMW need to make this memory allocation more efficient
        int *tmprowCols = new int[rowSize];
        double *tmprowVals = new double[rowSize];
        int numEntries;

        matrix_->ExtractMyRowCopy (rowNum, rowSize, numEntries, tmprowVals, tmprowCols);

        for(int colIndx=0; colIndx<rowSize; colIndx++)
        {
          global_ids[ngids] = rowMap_->GID(rowNum);
          ngids++;
          global_ids[ngids]= colMap_->GID(tmprowCols[colIndx]);
          ngids++;
        }

	delete [] tmprowCols;
	delete [] tmprowVals;
      }
    }
    else // graph
    {
      int ngids = 0;
      for (int rowNum=0; rowNum<numRows; rowNum++)
      {
        int rowSize;
        rowSize = graph_->NumMyIndices(rowNum);

        int *tmprowCols = new int[rowSize];
        int numEntries;

        graph_->ExtractMyRowCopy (rowNum, rowSize, numEntries, tmprowCols);

        for(int colIndx=0;colIndx<rowSize;colIndx++)
        {
          global_ids[ngids] = rowMap_->GID(rowNum);
          ngids++;
          global_ids[ngids]= colMap_->GID(tmprowCols[colIndx]);
          ngids++;
        }

        delete [] tmprowCols;
      }
    }
    return;
  } // fine-grain hypergraph case


  if (ngids < 1){
    return;
  }

  for (int i=0; i<ngids; i++){
    local_ids[i] = i;
  }

  if (weight_dim >= 1){          // Note we only supply 1-D weights
    float *to_wgts = object_weights;
    if (input_type_ == geometric_input_){
      double *wgts=NULL;
      int ld=0;
      if (weights_.get() && (weights_->NumVectors() > 0)){
        weights_->ExtractView(&wgts, &ld);
        if (ld < ngids){
          *ierr = ZOLTAN_FATAL;
          std::cerr << "Proc:" << myProc_ << " Error: ";
          std::cerr << "My_Object_List: not enough object weights" << std::endl;
          return;
        }
        for (int i=0; i < ngids; i++){
          *to_wgts= static_cast<float>(wgts[i]);
          to_wgts += weight_dim;
        }
      }
    }
    else if (costs_->haveVertexWeights()){
      std::map<int, float> weightMap;
      std::map<int, float>::iterator curr;
      std::map<int, float>::iterator end = weightMap.end();
  
      int mapSize = costs_->getVertexWeights(weightMap);
      if (mapSize != ngids){
        *ierr = ZOLTAN_FATAL;
        std::cout << "Proc:" << myProc_ << " Error: ";
        std::cout << "QueryObject::My_Object_List, number of vertex weights" << std::endl;
        return;
      }
      for (int i=0; i < ngids; i++){
        curr = weightMap.find(global_ids[i]);
        if (curr == end){
          *ierr = ZOLTAN_FATAL;
          std::cout << "Proc:" << myProc_ << " Error: ";
          std::cout << "QueryObject::My_Object_List, missing vertex weight" << std::endl;
          return;
        }
        *to_wgts = curr->second;
        to_wgts += weight_dim;
      }
    }
  }
  return;
}

// member graph query functions

void QueryObject::My_Number_Edges_Multi(int num_gid_entries, int num_lid_entries,
	     int num_obj,
	     ZOLTAN_ID_PTR global_ids, ZOLTAN_ID_PTR local_ids,
	     int *num_edges, int * ierr )
{
  *ierr = ZOLTAN_OK;

  int num_indices, rc;

  if (!haveGraph_){
    for (int i=0; i<num_obj; i++){
      rc = matrix_->NumMyRowEntries(local_ids[i], num_indices);
      if (rc != 0){
	*ierr = ZOLTAN_FATAL;
	std::cout << "Proc:" << myProc_ << " Error: ";
	std::cout << "QueryObject::My_Number_Edges_Multi, local id not in matrix" << std::endl;
	return;
      }
      num_edges[i] = num_indices;
      if (graph_self_edges_.size() > 0){
	std::set<int>::iterator it = graph_self_edges_.find(global_ids[i]);
	if (it != graph_self_edges_.end()){
	  num_edges[i]--;
	}
      }
    }
  }
  else{
    for (int i=0; i<num_obj; i++){
      num_edges[i] = graph_->NumMyIndices(local_ids[i]);
      if (num_edges[i] < 0){
	*ierr = ZOLTAN_FATAL;
	std::cout << "Proc:" << myProc_ << " Error: ";
	std::cout << "QueryObject::My_Number_Edges_Multi, local id not in graph" << std::endl;
	return;
      }
      if (graph_self_edges_.size() > 0){
	std::set<int>::iterator it = graph_self_edges_.find(global_ids[i]);
	if (it != graph_self_edges_.end()){
	  num_edges[i]--;
	}
      }
    }
  }
  return;
}

void QueryObject::My_Edge_List_Multi(int num_gid_entries, int num_lid_entries, int num_obj,
	       ZOLTAN_ID_PTR global_ids, ZOLTAN_ID_PTR local_ids, int *num_edges,
	       ZOLTAN_ID_PTR neighbor_global_ids, int * neighbor_procs,
	       int weight_dim, float * edge_weights, int * ierr )
{
  int *gids=NULL;
  int num_indices, rc, tmpSize;
  double *tmp = NULL;

  *ierr = ZOLTAN_OK;

  if (num_obj < 1) {
    return;
  }

  int *nborProc = neighbor_procs;
  float *wgt = edge_weights;
  ZOLTAN_ID_PTR nborID = neighbor_global_ids;
  bool use_weights = false;
  std::map<int, float> wgtMap;
  std::map<int, float>::iterator wgtIter;
  std::map<int, int>::iterator procIter;

  if ((weight_dim >= 1) && costs_->haveGraphEdgeWeights()){
    use_weights = true;
  }

  if (!haveGraph_)
    tmpSize = matrix_->MaxNumEntries();
  else
    tmpSize = graph_->MaxNumIndices();

  if (tmpSize > 0){
    if (!haveGraph_)
      tmp = new double [tmpSize];

    gids = new int [tmpSize];

    if ((!haveGraph_ && !tmp) || !gids){
      *ierr = ZOLTAN_MEMERR;
      return;
    }
  }

  for (int i=0; (i<num_obj) && (*ierr == ZOLTAN_OK); i++){

    int self_edge = 0;
    if (graph_self_edges_.size() > 0){
      std::set<int>::iterator it = graph_self_edges_.find(global_ids[i]);
      if (it != graph_self_edges_.end()){
	self_edge = 1;
      }
    }

    if (!haveGraph_){
      rc = matrix_->ExtractMyRowCopy(local_ids[i], tmpSize, num_indices, tmp, gids);
    }
    else{
      rc = graph_->ExtractMyRowCopy(local_ids[i], tmpSize,  num_indices, gids);
    }

    if ((rc < 0) || (num_indices != (num_edges[i] + self_edge))){
      *ierr = ZOLTAN_FATAL;
      std::cout << "Proc:" << myProc_ << " Error: ";
      std::cout << "QueryObject::My_Edge_List_Multi, extracting row" << std::endl;
      break;
    }

    for (int j=0; j<num_indices; j++){
      gids[j] = colMap_->GID(gids[j]); // convert to global IDs
    }

    if (use_weights){
      rc = costs_->getGraphEdgeWeights(global_ids[i], wgtMap);
      if (rc != num_edges[i]){
	*ierr = ZOLTAN_FATAL;
	std::cout << "Proc:" << myProc_ << " Error: ";
	std::cout << "QueryObject::My_Edge_List_Multi, getting weights" << std::endl;
	break;
      }
    }
    for (int j=0; j < num_indices; j++){

      if (self_edge && (gids[j] == (int)global_ids[i])) continue;  // skip self edge

      *nborID++ = gids[j];                   // Global ID of neighbor

      procIter = procmap_.find(gids[j]);
      if (procIter == procmap_.end()){
	*ierr = ZOLTAN_FATAL;
	std::cout << "Proc:" << myProc_ << " Error: ";
	std::cout << "QueryObject::My_Edge_List_Multi, process ID map" << std::endl;
	break;
      }

      *nborProc++ = procIter->second;        // Process owning neighbor

      if (use_weights){
        wgtIter = wgtMap.find(gids[j]); 
        if (wgtIter == wgtMap.end()){
          *ierr = ZOLTAN_FATAL;
          std::cout << "Proc:" << myProc_ << " Error: ";
          std::cout << "QueryObject::My_Edge_List_Multi, weight map" << std::endl;
          break;
        }
        *wgt++ = (float)wgtIter->second;    // edge weight
      }
    }
  }
  if (gids) delete [] gids;
  if (tmp) delete [] tmp;

  return;
}

// member hypergraph query functions

void QueryObject::My_HG_Size_CS(int* num_lists, int* num_pins, int* format, int * ierr )
{
  *ierr = ZOLTAN_OK;

  *format = ZOLTAN_COMPRESSED_VERTEX;   // We will return row (vertex) lists

  if (haveGraph_){
    *num_lists = graph_->NumMyRows();       // Number of rows
    *num_pins = graph_->NumMyNonzeros();    // Total nonzeros in these rows
  }
  else{
    *num_lists = matrix_->NumMyRows();       // Number of rows
    *num_pins = matrix_->NumMyNonzeros();    // Total nonzeros in these rows
  }

  return;
}

void QueryObject::My_HG_CS (int num_gid_entries, int num_row_or_col, int num_pins, int format,
	    ZOLTAN_ID_PTR vtxedge_GID, int* vtxedge_ptr, ZOLTAN_ID_PTR pin_GID,
				     int * ierr )
{
  int num_indices, rc, npins;
  double *tmp=NULL;


  *ierr = ZOLTAN_OK;

  if (haveGraph_){
    npins = graph_->NumMyNonzeros();
  }
  else{
    npins = matrix_->NumMyNonzeros();
    int maxrow = matrix_->MaxNumEntries();
    if (maxrow > 0){
      tmp = new double [maxrow];
      if (!tmp){
	*ierr = ZOLTAN_MEMERR;
	return;
      }
    }
  }

  if ((format != ZOLTAN_COMPRESSED_VERTEX) ||
      (num_row_or_col != rowMap_->NumMyElements()) ||
      (num_pins != npins)){
    *ierr = ZOLTAN_FATAL;
    std::cout << "Proc:" << myProc_ << " Error: ";
    std::cout << "QueryObject::My_HG_CS, bad arguments" << std::endl;
    if (tmp) delete [] tmp;
    return;
  }

  int pin_start_pos = 0;
  int *gids = (int *)pin_GID;

  for (int i=0; i<num_row_or_col; i++){ 
    vtxedge_GID[i] = rowMap_->GID(i);
    vtxedge_ptr[i] = pin_start_pos;

    if (haveGraph_){
      rc = graph_->ExtractMyRowCopy(i, npins, num_indices, gids + pin_start_pos);
    }
    else{
      rc = matrix_->ExtractMyRowCopy(i, npins, num_indices, tmp, gids + pin_start_pos);
    }

    if (rc != 0){
      *ierr = ZOLTAN_FATAL;
      std::cout << "Proc:" << myProc_ << " Error: ";
      std::cout << "QueryObject::My_HG_CS, extracting row" << std::endl;
      break;
    }

    for (int i=pin_start_pos; i<pin_start_pos+num_indices; i++){
      gids[i] = colMap_->GID(gids[i]); // convert to global IDs
      if (gids[i] < base_){
	*ierr = ZOLTAN_FATAL;
	std::cout << "Proc:" << myProc_ << " Error: ";
	std::cout << "QueryObject::My_HG_CS, local ID not in column map" << std::endl;
	break;
      }
    }

    pin_start_pos += num_indices;
    npins -= num_indices;
  }

  if (tmp){
    delete [] tmp;
  }
}

void QueryObject::My_HG_Size_Edge_Weights( int* num_edges, int* ierr)
{
  *num_edges = costs_->getNumHypergraphEdgeWeights();

  *ierr = ZOLTAN_OK;
}

void QueryObject::My_HG_Edge_Weights(
      int num_gid_entries, int num_lid_entries, int num_edges, int edge_weight_dim,
      ZOLTAN_ID_PTR edge_GID, ZOLTAN_ID_PTR edge_LID, float* edge_weights, int* ierr)
{
  *ierr = ZOLTAN_OK;

  if ((num_edges == 0) || (edge_weight_dim == 0)) return;

  if (num_edges != costs_->getNumHypergraphEdgeWeights()){
    *ierr = ZOLTAN_FATAL;
    std::cerr << "Proc:" << myProc_ << " Error: ";
    std::cerr << "QueryObject::My_HG_Edge_Weights, bad arguments" << std::endl;
    return;
  }

  if (num_lid_entries > 0){
    for (int i=0; i<num_edges; i++){
      edge_LID[i] = i;
    }
  }

  costs_->getHypergraphEdgeWeights(num_edges, (int *)edge_GID, edge_weights);
}

// member geometric query functions

int QueryObject::My_Number_Geom(int *ierr)
{
  int dim = 0;
  *ierr = ZOLTAN_FATAL;

  if (coords_.get()){
    dim = coords_->NumVectors();
    *ierr = ZOLTAN_OK;
  }
  else{
    std::cerr << "in My_Number_Geom, no MultiVector present" << std::endl;
  }

  return dim;
}

void QueryObject::My_Geom_Multi(int num_gid_entries, int num_lid_entries,
        int num_obj, ZOLTAN_ID_PTR gids, ZOLTAN_ID_PTR lids, int num_dim,
        double *geom_vec, int *ierr)
{
  if ((num_obj < 1) || (num_dim < 1)){
    *ierr = ZOLTAN_OK;
    return;
  }

  if (coords_.get() && 
      (coords_->MyLength() >= num_obj) && 
      (coords_->NumVectors() >= num_dim)){

    double *coords;
    int stride;

    coords_->ExtractView(&coords, &stride);

    double **v = new double * [num_dim];
    for (int j=0; j < num_dim; j++){
      v[j] = coords + (stride * j);
    }

    for (int i=0; i<num_obj; i++){
      for (int j=0; j<num_dim; j++){
        *geom_vec++ = v[j][i];
      }
    }

    delete [] v;
    *ierr = ZOLTAN_OK;
  }
  else{
    std::cerr << "Error in My_Geom_Multi" << std::endl;
    *ierr = ZOLTAN_FATAL;
  }
}

} // end namespace ZoltanLib
} // end namespace Epetra
} // end namespace Isorropia
