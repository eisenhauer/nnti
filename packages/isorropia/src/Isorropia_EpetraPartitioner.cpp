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

#include <Isorropia_EpetraPartitioner.hpp>
#include <Isorropia_Zoltan_Repartition.hpp>
#include <Isorropia_Exception.hpp>
#include <Isorropia_Epetra_utils.hpp>

#include <Teuchos_RefCountPtr.hpp>

#ifdef HAVE_EPETRA
#include <Epetra_Comm.h>
#include <Epetra_Map.h>
#include <Epetra_Import.h>
#include <Epetra_Vector.h>
#include <Epetra_MultiVector.h>
#include <Epetra_CrsGraph.h>
#include <Epetra_CrsMatrix.h>
#include <Epetra_LinearProblem.h>
#endif

/** Isorropia is the namespace that contains isorropia's declarations
  for classes and functions.
*/
namespace Isorropia {

#ifdef HAVE_EPETRA

EpetraPartitioner::
EpetraPartitioner(Teuchos::RefCountPtr<const Epetra_CrsGraph> input_graph,
                  const Teuchos::ParameterList& paramlist)
  : input_map_(),
    input_graph_(input_graph),
    paramlist_(),
    partitioning_already_computed_(false)
{
  input_map_ = Teuchos::rcp(&(input_graph->RowMap()), false);
  weights_ = Teuchos::rcp(Epetra_Utils::create_row_weights_nnz(*input_graph));
  paramlist_ = paramlist;
}

EpetraPartitioner::
EpetraPartitioner(Teuchos::RefCountPtr<const Epetra_RowMatrix> input_matrix,
                  const Teuchos::ParameterList& paramlist)
  : input_map_(),
    input_graph_(),
    paramlist_(),
    partitioning_already_computed_(false)
{
  input_map_ = Teuchos::rcp(&(input_matrix->RowMatrixRowMap()),false);
  weights_ = Teuchos::rcp(Epetra_Utils::create_row_weights_nnz(*input_matrix));
  paramlist_ = paramlist;
}

EpetraPartitioner::~EpetraPartitioner()
{
}

void EpetraPartitioner::setParameters(const Teuchos::ParameterList& paramlist)
{
  paramlist_ = paramlist;
}

void EpetraPartitioner::compute_partitioning()
{
  std::string bal_package_str("Balancing package");
  std::string bal_package = paramlist_.get(bal_package_str, "none_specified");
  int err = 0;
  if (bal_package == "Zoltan" || bal_package == "zoltan") {
#ifdef HAVE_ISORROPIA_ZOLTAN
    if (input_graph_.get() == NULL) {
      std::string str1("compute_partitioning ERROR: input_graph NULL, possibily ");
      std::string str2("because Epetra_RowMatrix input to Partitioner.");
      throw Isorropia::Exception(str1+str2);
    }

    err = Isorropia_Zoltan::repartition(*input_graph_, paramlist_,
                                        myNewElements_, exports_, imports_);
#else
    throw Isorropia::Exception("Zoltan requested, but zoltan not enabled.");
#endif
  }
  else {
    err = Isorropia::Epetra_Utils::repartition(*input_map_,
                                               *weights_,
                                               myNewElements_,
                                               exports_, imports_);
  }

  if (err != 0) {
    throw Isorropia::Exception("error in repartitioning");
  }

  partitioning_already_computed_ = true;
}

bool EpetraPartitioner::partitioning_already_computed() const
{
  return partitioning_already_computed_;
}

int EpetraPartitioner::newPartitionNumber(int myElem) const
{
  std::map<int,int>::const_iterator iter = exports_.find(myElem);
  if (iter != exports_.end()) {
    return(iter->second);
  }

  return( input_graph_->RowMap().Comm().MyPID() );
}

int EpetraPartitioner::numElemsInPartition(int partition) const
{
  int myPart = input_graph_->RowMap().Comm().MyPID();
  if (partition != myPart) {
    throw Isorropia::Exception("EpetraPartitioner::numElemsInPartition not implemented for non-local partitions.");
  }

  return(myPart);
}

void
EpetraPartitioner::elemsInPartition(int partition, int* elementList, int len) const
{
  int myPart = input_graph_->RowMap().Comm().MyPID();
  if (partition != myPart) {
    throw Isorropia::Exception("error in Epetra_Map::MyGlobalElements");
  }

  int length = len;
  if (myNewElements_.size() < length) length = myNewElements_.size();

  for(int i=0; i<length; ++i) {
    elementList[i] = myNewElements_[i];
  }
}

#endif //HAVE_EPETRA

}//namespace Isorropia
