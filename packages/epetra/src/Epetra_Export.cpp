
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


#include "Epetra_Export.h"
#include "Epetra_BlockMap.h"
#include "Epetra_Distributor.h"
#include "Epetra_Comm.h"

//==============================================================================
// Epetra_Export constructor for a Epetra_BlockMap object
Epetra_Export::Epetra_Export( const Epetra_BlockMap &  SourceMap, const Epetra_BlockMap & TargetMap)
  : Epetra_Object("Epetra::Export"), 
    TargetMap_(TargetMap),
    SourceMap_(SourceMap),
    NumSameIDs_(0),
    NumPermuteIDs_(0),
    PermuteToLIDs_(0),
    PermuteFromLIDs_(0),
    NumRemoteIDs_(0),
    RemoteLIDs_(0),
    NumExportIDs_(0),
    ExportLIDs_(0),
    ExportPIDs_(0),
    NumSend_(0),
    NumRecv_(0),
    Distor_(0)
{

  int i;

  // Build three ID lists:
  // NumSameIDs - Number of IDs in TargetMap and SourceMap that are identical, up to the first
  //              nonidentical ID.
  // NumPermuteIDs - Number of IDs in SourceMap that must be indirectly loaded but are on this processor.
  // NumExportIDs - Number of IDs that are in SourceMap but not in TargetMap, and thus must be exported.

  int NumSourceIDs = SourceMap.NumMyElements();
  int NumTargetIDs = TargetMap.NumMyElements();

  int *TargetGIDs = 0;
  if (NumTargetIDs>0) {
    TargetGIDs = new int[NumTargetIDs];
    TargetMap.MyGlobalElements(TargetGIDs);
  }

  int * SourceGIDs = 0;
  if (NumSourceIDs>0) {
    SourceGIDs = new int[NumSourceIDs];
    SourceMap.MyGlobalElements(SourceGIDs);
  }

  int MinIDs = EPETRA_MIN(NumSourceIDs, NumTargetIDs);


  NumSameIDs_ = 0;
  for (i=0; i< MinIDs; i++) if (TargetGIDs[i]==SourceGIDs[i]) NumSameIDs_++; else break;


  // Find count of Source IDs that are truly remote and those that are local but permuted

  NumPermuteIDs_ = 0;
  NumExportIDs_ = 0;
  for (i=NumSameIDs_; i< NumSourceIDs; i++) 
    if (TargetMap.MyGID(SourceGIDs[i])) NumPermuteIDs_++; // Check if Source GID is a local Target GID
    else NumExportIDs_++; // If not, then it is remote



  // Define remote and permutation lists

  int * ExportGIDs;
  if (NumExportIDs_>0) {
    ExportLIDs_ = new int[NumExportIDs_];
    ExportGIDs = new int[NumExportIDs_];
  }
  if (NumPermuteIDs_>0)  {
    PermuteToLIDs_ = new int[NumPermuteIDs_];
    PermuteFromLIDs_ = new int[NumPermuteIDs_];
  }

  NumPermuteIDs_ = 0;
  NumExportIDs_ = 0;
  for (i=NumSameIDs_; i< NumSourceIDs; i++) {
    if (TargetMap.MyGID(SourceGIDs[i])) {
      PermuteFromLIDs_[NumPermuteIDs_] = i;
      PermuteToLIDs_[NumPermuteIDs_++] = TargetMap.LID(SourceGIDs[i]);
    }
    else {
      //NumSend_ +=SourceMap.ElementSize(i); // Count total number of entries to send
      NumSend_ +=SourceMap.MaxElementSize(); // Count total number of entries to send (currently need max)
      ExportGIDs[NumExportIDs_] = SourceGIDs[i];
      ExportLIDs_[NumExportIDs_++] = i;
    }
  }
     
  if ( NumExportIDs_>0 && !SourceMap.DistributedGlobal()) 
    ReportError("Warning in Epetra_Export: Serial Export has remote IDs. (Exporting from Subset of Source Map)", 1);

  // Test for distributed cases
  int ierr = 0;

  if (SourceMap.DistributedGlobal()) {

    if (NumExportIDs_>0) {
      ExportPIDs_ = new int[NumExportIDs_];
      ierr = TargetMap.RemoteIDList(NumExportIDs_, ExportGIDs, ExportPIDs_, 0); // Get remote PIDs
      if( ierr ) throw ReportError("Error in Epetra_BlockMap::RemoteIDList", ierr);

      //Get rid of IDs not in Target Map
      if(NumExportIDs_>0) {
        int cnt = 0;
        for( i = 0; i < NumExportIDs_; ++i )
          if( ExportPIDs_[i] == -1 ) ++cnt;
        if( cnt ) {
          int * NewExportGIDs = new int[NumExportIDs_-cnt];
          int * NewExportPIDs = new int[NumExportIDs_-cnt];
          cnt = 0;
          for( i = 0; i < NumExportIDs_; ++i )
            if( ExportPIDs_[i] != -1 ) {
              NewExportGIDs[cnt] = ExportGIDs[i];
              NewExportPIDs[cnt] = ExportPIDs_[i];
              ++cnt;
          }
          NumExportIDs_ = cnt;
          delete [] ExportGIDs;
          delete [] ExportPIDs_;
          ExportGIDs = NewExportGIDs;
          ExportPIDs_ = NewExportPIDs;
          ReportError("Warning in Epetra_Export: Source IDs not found in Target Map (Do you want to export from subset of Source Map?)", 1 );
        }
      }
    }
    
    Distor_ = SourceMap.Comm().CreateDistributor();
    
    // Construct list of exports that calling processor needs to send as a result
    // of everyone asking for what it needs to receive.
    
    bool Deterministic = true;
    ierr = Distor_->CreateFromSends( NumExportIDs_, ExportPIDs_,Deterministic, NumRemoteIDs_);
    if (ierr!=0) throw ReportError("Error in Epetra_Distributor.CreateFromSends()", ierr);
    
    // Use comm plan with ExportGIDs to find out who is sending to us and
    // get proper ordering of GIDs for remote entries 
    // (that we will convert to LIDs when done).
    
    if (NumRemoteIDs_>0) RemoteLIDs_ = new int[NumRemoteIDs_]; // Allocate space for LIDs in target that are
    // going to get something from off-processor.
    ierr = Distor_->Do(reinterpret_cast<char *> (ExportGIDs), 
		sizeof( int ),
		reinterpret_cast<char *> (RemoteLIDs_));
    if (ierr) throw ReportError("Error in Epetra_Distributor.Do()", ierr);

    // Remote IDs come in as GIDs, convert to LIDs
    for (i=0; i< NumRemoteIDs_; i++) {
      RemoteLIDs_[i] = TargetMap.LID(RemoteLIDs_[i]);
      //NumRecv_ += TargetMap.ElementSize(RemoteLIDs_[i]); // Count total number of entries to receive
      NumRecv_ += TargetMap.MaxElementSize(); // Count total number of entries to receive (currently need max)
    }

    if (NumExportIDs_>0) delete [] ExportGIDs;
  }
  if (NumTargetIDs>0) delete [] TargetGIDs;
  if (NumSourceIDs>0) delete [] SourceGIDs;
  
  return;
}

//==============================================================================
// Epetra_Export copy constructor 
Epetra_Export::Epetra_Export(const Epetra_Export & Exporter)
  : Epetra_Object(Exporter), 
     TargetMap_(Exporter.TargetMap_),
    SourceMap_(Exporter.SourceMap_),
    NumSameIDs_(Exporter.NumSameIDs_),
    NumPermuteIDs_(Exporter.NumPermuteIDs_),
    PermuteToLIDs_(0),
    PermuteFromLIDs_(0),
    NumRemoteIDs_(Exporter.NumRemoteIDs_),
    RemoteLIDs_(0),
    NumExportIDs_(Exporter.NumExportIDs_),
    ExportLIDs_(0),
    ExportPIDs_(0),
    NumSend_(Exporter.NumSend_),
    NumRecv_(Exporter.NumRecv_),
    Distor_(0)
{
  int i;
  if (NumPermuteIDs_>0) {
    PermuteToLIDs_ = new int[NumPermuteIDs_];
    PermuteFromLIDs_ = new int[NumPermuteIDs_];
    for (i=0; i< NumPermuteIDs_; i++) {
      PermuteToLIDs_[i] = Exporter.PermuteToLIDs_[i];
      PermuteFromLIDs_[i] = Exporter.PermuteFromLIDs_[i];
    }
  }

  if (NumRemoteIDs_>0) {
    RemoteLIDs_ = new int[NumRemoteIDs_];
    for (i=0; i< NumRemoteIDs_; i++) RemoteLIDs_[i] = Exporter.RemoteLIDs_[i];
  }

  TargetMap().Comm().Barrier();
  if (NumExportIDs_>0) {
    ExportLIDs_ = new int[NumExportIDs_];
    ExportPIDs_ = new int[NumExportIDs_];
    for (i=0; i< NumExportIDs_; i++) {
      ExportLIDs_[i] = Exporter.ExportLIDs_[i];
      ExportPIDs_[i] = Exporter.ExportPIDs_[i];
    }
  }

  if (Exporter.Distor_!=0) Distor_ = Exporter.Distor_->Clone();

}

//==============================================================================
// Epetra_Export destructor 
Epetra_Export::~Epetra_Export()
{
  if( Distor_ != 0 ) delete Distor_;

  if (RemoteLIDs_ != 0) delete [] RemoteLIDs_;
  if (PermuteToLIDs_ != 0) delete [] PermuteToLIDs_;
  if (PermuteFromLIDs_ != 0) delete [] PermuteFromLIDs_;

  if( ExportPIDs_ != 0 ) delete [] ExportPIDs_; // These were created by GSPlan
  if( ExportLIDs_ != 0 ) delete [] ExportLIDs_;
}
//=============================================================================
void Epetra_Export::Print(ostream & os) const
{

  os << endl << endl << "Source Map:" << endl << endl;
  SourceMap_.Print(os);
  
  os << endl << endl << "Target Map:" << endl << endl;
  TargetMap_.Print(os);
  
  os << endl << endl << "Distributor:" << endl << endl;
  if (Distor_==0) os << "  Is empty...." << endl;
  else Distor_->Print(os);
  
  os << "Number of Same IDs = " << NumSameIDs_ << endl;
  
  os << "Epetra_Export Print Needs attention!!!!" << endl;
  return;
}
