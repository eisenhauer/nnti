//@HEADER
// ***********************************************************************
//
//     EpetraExt: Epetra Extended - Linear Algebra Services Package
//                 Copyright (2011) Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Michael A. Heroux (maherou@sandia.gov)
//
// ***********************************************************************
//@HEADER

#ifndef EPETRAEXT_MMHELPERS_H
#define EPETRAEXT_MMHELPERS_H

#include "EpetraExt_ConfigDefs.h"
#include "Epetra_ConfigDefs.h"
#include "Epetra_DistObject.h"
#include "Epetra_Map.h"
#include "Teuchos_RCP.hpp"

#include <vector>
#include <set>
#include <map>


class Epetra_CrsMatrix;
class Epetra_Import;
class Epetra_Distributor;

namespace EpetraExt {
class LightweightCrsMatrix;

// Turn on to enable the special MMM timers
//#define ENABLE_MMM_TIMINGS

//#define HAVE_EPETRAEXT_DEBUG // for extra sanity checks

// ==============================================================
//struct that holds views of the contents of a CrsMatrix. These
//contents may be a mixture of local and remote rows of the
//actual matrix.
class CrsMatrixStruct {
public:
  CrsMatrixStruct();

  virtual ~CrsMatrixStruct();

  void deleteContents();

  int numRows;

  // The following class members get used in the transpose modes of the MMM
  // but not in the A*B mode.  
  int* numEntriesPerRow;
  int** indices;
  double** values;
  bool* remote;
  int numRemote;
  const Epetra_BlockMap* importColMap;

  // Maps and matrices (all modes)
  const Epetra_Map* origRowMap;
  const Epetra_Map* rowMap;
  const Epetra_Map* colMap;
  const Epetra_Map* domainMap;
  LightweightCrsMatrix* importMatrix;
  const Epetra_CrsMatrix *origMatrix;

  // The following class members are only used for A*B mode
  std::vector<int> targetMapToOrigRow;
  std::vector<int> targetMapToImportRow;
};

int dumpCrsMatrixStruct(const CrsMatrixStruct& M);

// ==============================================================
class CrsWrapper {
 public:
  virtual ~CrsWrapper(){}

  virtual const Epetra_Map& RowMap() const = 0;

  virtual bool Filled() = 0;

#ifndef EPETRA_NO_32BIT_GLOBAL_INDICES
  virtual int InsertGlobalValues(int GlobalRow, int NumEntries, double* Values, int* Indices) = 0;

  virtual int SumIntoGlobalValues(int GlobalRow, int NumEntries, double* Values, int* Indices) = 0;
#endif

#ifndef EPETRA_NO_64BIT_GLOBAL_INDICES
  virtual int InsertGlobalValues(long long GlobalRow, int NumEntries, double* Values, long long* Indices) = 0;

  virtual int SumIntoGlobalValues(long long GlobalRow, int NumEntries, double* Values, long long* Indices) = 0;
#endif
};

// ==============================================================
class CrsWrapper_Epetra_CrsMatrix : public CrsWrapper {
 public:
  CrsWrapper_Epetra_CrsMatrix(Epetra_CrsMatrix& epetracrsmatrix);
  virtual ~CrsWrapper_Epetra_CrsMatrix();

  const Epetra_Map& RowMap() const;

  bool Filled();

#ifndef EPETRA_NO_32BIT_GLOBAL_INDICES
  int InsertGlobalValues(int GlobalRow, int NumEntries, double* Values, int* Indices);
  int SumIntoGlobalValues(int GlobalRow, int NumEntries, double* Values, int* Indices);
#endif

#ifndef EPETRA_NO_64BIT_GLOBAL_INDICES
  int InsertGlobalValues(long long GlobalRow, int NumEntries, double* Values, long long* Indices);
  int SumIntoGlobalValues(long long GlobalRow, int NumEntries, double* Values, long long* Indices);
#endif

 private:
  Epetra_CrsMatrix& ecrsmat_;
};

// ==============================================================
template<typename int_type>
class CrsWrapper_GraphBuilder : public CrsWrapper {
 public:
  CrsWrapper_GraphBuilder(const Epetra_Map& emap);
  virtual ~CrsWrapper_GraphBuilder();

  const Epetra_Map& RowMap() const {return rowmap_; }

  bool Filled();

  int InsertGlobalValues(int_type GlobalRow, int NumEntries, double* Values, int_type* Indices);
  int SumIntoGlobalValues(int_type GlobalRow, int NumEntries, double* Values, int_type* Indices);

  std::map<int_type,std::set<int_type>*>& get_graph();

  int get_max_row_length() { return max_row_length_; }

 private:
  std::map<int_type,std::set<int_type>*> graph_;
  const Epetra_Map& rowmap_;
  int max_row_length_;
};

// ==============================================================
template<typename int_type>
void insert_matrix_locations(CrsWrapper_GraphBuilder<int_type>& graphbuilder,
                              Epetra_CrsMatrix& C);

template<typename int_type>
void Tpack_outgoing_rows(const Epetra_CrsMatrix& mtx,
                        const std::vector<int_type>& proc_col_ranges,
                        std::vector<int_type>& send_rows,
                        std::vector<int>& rows_per_send_proc);

#ifndef EPETRA_NO_32BIT_GLOBAL_INDICES
void pack_outgoing_rows(const Epetra_CrsMatrix& mtx,
                        const std::vector<int>& proc_col_ranges,
                        std::vector<int>& send_rows,
                        std::vector<int>& rows_per_send_proc);
#endif

#ifndef EPETRA_NO_64BIT_GLOBAL_INDICES
void pack_outgoing_rows(const Epetra_CrsMatrix& mtx,
                        const std::vector<long long>& proc_col_ranges,
                        std::vector<long long>& send_rows,
                        std::vector<int>& rows_per_send_proc);
#endif

template<typename int_type>
std::pair<int_type,int_type> get_col_range(const Epetra_Map& emap);

template<typename int_type>
std::pair<int_type,int_type> get_col_range(const Epetra_CrsMatrix& mtx);

// ==============================================================
class LightweightMapData : Epetra_Data {
  friend class LightweightMap; 
 public:
  LightweightMapData();
  ~LightweightMapData();
  long long IndexBase_;
#ifndef EPETRA_NO_32BIT_GLOBAL_INDICES
  std::vector<int> MyGlobalElements_int_; 
  Epetra_HashTable<int> * LIDHash_int_;
#endif
#ifndef EPETRA_NO_64BIT_GLOBAL_INDICES
  std::vector<long long> MyGlobalElements_LL_; 
  Epetra_HashTable<long long> * LIDHash_LL_;
#endif

  // For "copy" constructor only...
  Epetra_Map * CopyMap_;
  
 };

class LightweightMap {
 public:
  LightweightMap();
#ifndef EPETRA_NO_32BIT_GLOBAL_INDICES
  LightweightMap(int NumGlobalElements,int NumMyElements, const int * MyGlobalElements, int IndexBase, bool GenerateHash=true);
#endif
#ifndef EPETRA_NO_64BIT_GLOBAL_INDICES
  LightweightMap(long long NumGlobalElements,int NumMyElements, const long long * MyGlobalElements, int IndexBase, bool GenerateHash=true);
  LightweightMap(long long NumGlobalElements,int NumMyElements, const long long * MyGlobalElements, long long IndexBase, bool GenerateHash=true);
#endif
  LightweightMap(const Epetra_Map & Map);
  LightweightMap(const LightweightMap & Map);
  ~LightweightMap();

  LightweightMap & operator=(const LightweightMap & map);
#ifndef EPETRA_NO_32BIT_GLOBAL_INDICES
  int LID(int GID) const;
  int GID(int LID) const;      
#endif

#ifndef EPETRA_NO_64BIT_GLOBAL_INDICES
  int  LID(long long GID) const;
#endif
  long long GID64(int LID) const;
  int NumMyElements() const;

#if defined(EPETRA_NO_32BIT_GLOBAL_INDICES) && defined(EPETRA_NO_64BIT_GLOBAL_INDICES)
  // default implementation so that no compiler/linker error in case neither 32 nor 64
  // bit indices present.
  int  LID(long long GID) const { return -1; }
#endif

#ifndef EPETRA_NO_32BIT_GLOBAL_INDICES
  int* MyGlobalElements() const; 
  int IndexBase() const {
    if(IndexBase64() == (long long) static_cast<int>(IndexBase64()))
      return (int) IndexBase64();
    throw "EpetraExt::LightweightMap::IndexBase: IndexBase cannot fit an int.";
  }
  void MyGlobalElementsPtr(int *& MyGlobalElementList) const;
#endif

#ifndef EPETRA_NO_64BIT_GLOBAL_INDICES
  long long* MyGlobalElements64() const;
  void MyGlobalElementsPtr(long long *& MyGlobalElementList) const;
#endif
  long long IndexBase64() const {return Data_->IndexBase_;}

  int MinLID() const;
  int MaxLID() const;

  bool GlobalIndicesInt() const { return IsInt; }
  bool GlobalIndicesLongLong() const { return IsLongLong; }
 private:
  void CleanupData();
  LightweightMapData *Data_;
  bool IsLongLong;
  bool IsInt;
  //Epetra_BlockMapData* Data_;

#ifndef EPETRA_NO_32BIT_GLOBAL_INDICES
  void Construct_int(int NumGlobalElements,int NumMyElements, const int * MyGlobalElements, long long IndexBase, bool GenerateHash=true);
#endif
#ifndef EPETRA_NO_64BIT_GLOBAL_INDICES
  void Construct_LL(long long NumGlobalElements,int NumMyElements, const long long * MyGlobalElements, long long IndexBase, bool GenerateHash=true);
#endif
};


// ==============================================================
class RemoteOnlyImport {
 public:
  RemoteOnlyImport(const Epetra_Import & Importer, LightweightMap & RemoteOnlyTargetMap);
  ~RemoteOnlyImport();

  int NumSameIDs() {return 0;}

  int NumPermuteIDs() {return 0;} 

  int NumRemoteIDs() {return NumRemoteIDs_;}

  int NumExportIDs() {return NumExportIDs_;}

  int* ExportLIDs() {return ExportLIDs_;}

  int* ExportPIDs() {return ExportPIDs_;}

  int* RemoteLIDs() {return RemoteLIDs_;}  

  int* PermuteToLIDs() {return 0;}

  int* PermuteFromLIDs() {return 0;}

  int NumSend() {return NumSend_;}
  
  Epetra_Distributor & Distributor() {return *Distor_;}  

  const Epetra_BlockMap & SourceMap() const {return *SourceMap_;}
  const LightweightMap & TargetMap() const {return *TargetMap_;}

 private:
  int NumSend_;
  int NumRemoteIDs_;
  int NumExportIDs_;
  int * ExportLIDs_;
  int * ExportPIDs_;
  int * RemoteLIDs_;
  Epetra_Distributor* Distor_;
  const Epetra_BlockMap* SourceMap_;
  const LightweightMap  *TargetMap_;
};
   
// ==============================================================
class LightweightCrsMatrix {
 public:
  LightweightCrsMatrix(const Epetra_CrsMatrix & A, RemoteOnlyImport & RowImporter);
  LightweightCrsMatrix(const Epetra_CrsMatrix & A, Epetra_Import & RowImporter);
  ~LightweightCrsMatrix();

  // Standard crs data structures
  std::vector<int>    rowptr_;
  std::vector<int>    colind_;
  std::vector<double> vals_;

#ifndef EPETRA_NO_64BIT_GLOBAL_INDICES
  // Colind in LL-GID space (if needed)
  std::vector<long long>   colind_LL_;
#endif

  // Epetra Maps
  bool                     use_lw;
  LightweightMap           *RowMapLW_;
  Epetra_BlockMap          *RowMapEP_;
  LightweightMap           ColMap_;
  Epetra_Map               DomainMap_;


  // List of owning PIDs (from the DomainMap) as ordered by entries in the column map.
  std::vector<int>    ColMapOwningPIDs_;

  // For building the final importer for C
  std::vector<int>    ExportLIDs_;
  std::vector<int>    ExportPIDs_;

 private: 

  template <typename ImportType, typename int_type>
  void Construct(const Epetra_CrsMatrix & A, ImportType & RowImporter);

  // Templated versions of MakeColMapAndReindex (to prevent code duplication)
  template <class GO>
  int MakeColMapAndReindex(std::vector<int> owningPIDs,std::vector<GO> Gcolind);

  template<typename int_type>
  std::vector<int_type>& getcolind();

  template<typename ImportType, typename int_type>
  int PackAndPrepareReverseComm(const Epetra_CrsMatrix & SourceMatrix, ImportType & RowImporter,
				std::vector<int> &ReverseSendSizes, std::vector<int_type> &ReverseSendBuffer);

  template<typename ImportType, typename int_type>
  int MakeExportLists(const Epetra_CrsMatrix & SourceMatrix, ImportType & RowImporter,
		      std::vector<int> &ReverseRecvSizes, const int_type *ReverseRecvBuffer,
		      std::vector<int> & ExportPIDs, std::vector<int> & ExportLIDs);

};

#ifndef EPETRA_NO_32BIT_GLOBAL_INDICES
template<> inline std::vector<int>& LightweightCrsMatrix::getcolind() { return colind_; }
#endif
#ifndef EPETRA_NO_64BIT_GLOBAL_INDICES
template<> inline std::vector<long long>& LightweightCrsMatrix::getcolind() { return colind_LL_; }
#endif

}//namespace EpetraExt

#endif

