
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


#include "Epetra_HashTable.h"
#include "Epetra_Comm.h"
#include "Epetra_Map.h"
#include "Epetra_MapColoring.h"
//=============================================================================
Epetra_MapColoring::Epetra_MapColoring(int * ElementColors, const Epetra_BlockMap& Map,
				       const int DefaultColor)
  : Epetra_DistObject(Map, "Epetra::MapColoring"),
    DefaultColor_(DefaultColor),
    ColorIDs_(0),
    FirstColor_(0),
    NumColors_(0),
    Allocated_(false),
    ListsAreGenerated_(false),
    ListsAreValid_(false)
{
  Allocate(ElementColors, 1);
}
//=============================================================================
Epetra_MapColoring::Epetra_MapColoring(const Epetra_BlockMap& Map,
				       const int DefaultColor)
  : Epetra_DistObject(Map, "Epetra::MapColoring"),
    DefaultColor_(DefaultColor),
    ColorIDs_(0),
    FirstColor_(0),
    NumColors_(0),
    Allocated_(false),
    ListsAreGenerated_(false),
    ListsAreValid_(false)
{
  Allocate(&DefaultColor_, 0);
}
//=============================================================================
Epetra_MapColoring::Epetra_MapColoring(const Epetra_MapColoring& Source)
  : Epetra_DistObject(Source),
    DefaultColor_(Source.DefaultColor_),
    ColorIDs_(0),
    FirstColor_(0),
    NumColors_(0),
    Allocated_(Source.Allocated_),
    ListsAreGenerated_(false),
    ListsAreValid_(false)
{
  Allocate(Source.ElementColors_, 1);
}
//=========================================================================
Epetra_MapColoring::~Epetra_MapColoring(){


  if (Allocated_) delete [] ElementColors_;
  if (ListsAreGenerated_) DeleteLists();
}

//=========================================================================
int Epetra_MapColoring::DeleteLists()const {


  if (ListsAreGenerated_) {
    for (int i=0; i<NumColors_; i++) if (ColorLists_[i]!=0) delete [] ColorLists_[i];
    delete [] ColorLists_;
    delete [] ColorCount_;
    delete ColorIDs_;
    ListItem * CurItem = FirstColor_;
    while (CurItem!=0) {
      ListItem * NextItem = CurItem->NextItem;
      delete CurItem;
      CurItem = NextItem;
    }
  }
  return(0);
}

//=========================================================================
int Epetra_MapColoring::Allocate(int * ElementColors, int Increment)
{
  
  if (Allocated_) return(0);
  
  int NumMyElements = Map().NumMyElements();
  ElementColors_ = new int[NumMyElements];
  for (int i=0; i< NumMyElements; i++) ElementColors_[i] = ElementColors[i*Increment];
  Allocated_ = true;
  return(0);
}

//=========================================================================
int Epetra_MapColoring::GenerateLists() const {
  int NumMyElements = Map().NumMyElements();
  if (NumMyElements==0) return(0); // Nothing to do

  if (ListsAreGenerated_) DeleteLists();  // Delete any existing lists


  // Scan the ElementColors to determine how many colors we have
  NumColors_ = 1;
  FirstColor_ = new ListItem(ElementColors_[0]); // Initialize First color in list
  for (int i=1; i<NumMyElements; i++) if (!InItemList(ElementColors_[i])) NumColors_++;

  // Create hash table that maps color IDs to the integers 0,...NumColors_
  ColorIDs_ = new Epetra_HashTable(NumColors_);
  ListItem * CurItem = FirstColor_;
  for (int i=0; i<NumColors_; i++) {
    ColorIDs_->Add(CurItem->ItemValue, i);
    CurItem = CurItem->NextItem;
  }
  // Count the number of IDs of each color
  ColorCount_ = new int[NumColors_];
  for (int i=0; i<NumColors_; i++) ColorCount_[i] = 0;
  for (int i=0; i<NumMyElements; i++) ColorCount_[ColorIDs_->Get(ElementColors_[i])]++;

  // Finally build list of IDs grouped by color
  ColorLists_ = new int *[NumColors_];
  for (int i=0; i<NumColors_; i++) ColorLists_[i] = new int[ColorCount_[i]];

  ListsAreValid_ = true;

  return(0);
}
//=========================================================================
bool Epetra_MapColoring::InItemList(int ColorValue) const {
  bool ColorFound = false;
  ListItem * CurColor = 0;
  ListItem * NextColor = FirstColor_;
  while (ColorFound && NextColor!=0) {
    CurColor = NextColor;
    NextColor = CurColor->NextItem;
    if (ColorValue=CurColor->ItemValue) ColorFound = true;
  }

  if (!ColorFound) CurColor->NextItem = new ListItem(ColorValue);

  return(ColorFound);
}
//=========================================================================
int Epetra_MapColoring::NumElementsWithColor(int Color) const  {
  if (!ListsAreValid_) GenerateLists(); 
  int arrayIndex = ColorIDs_->Get(Color);
  return(ColorCount_[arrayIndex]);
}
//=========================================================================
int * Epetra_MapColoring::ColorLIDList(int Color) const  {
  if (!ListsAreValid_) GenerateLists(); 
  int arrayIndex = ColorIDs_->Get(Color);
  return(ColorLists_[arrayIndex]);
}
//=========================================================================
Epetra_Map * Epetra_MapColoring::GenerateMap(int Color) const {

  if (!ListsAreValid_) GenerateLists(); 
  int arrayIndex = ColorIDs_->Get(Color);
  int NumElements = ColorCount_[arrayIndex];
  int * ElementIDs = ColorLIDList(Color);
  Epetra_Map * map = new Epetra_Map(-1, NumElements, ElementIDs, 
				    Map().IndexBase(), Map().Comm());
  return(map);
}
//=========================================================================
Epetra_BlockMap * Epetra_MapColoring::GenerateBlockMap(int Color) const {

  if (!ListsAreValid_) GenerateLists(); 
  int arrayIndex = ColorIDs_->Get(Color);
  int NumElements = ColorCount_[arrayIndex];
  int * ColorElementIDs = ColorLIDList(Color);
  int * ColorElementSizes = new int[NumElements];
  int * MapElementSizes = Map().ElementSizeList();

  for (int i=0; i<NumElements; i++) 
    ColorElementSizes[i] = MapElementSizes[ColorElementIDs[i]];

  Epetra_BlockMap * map = new Epetra_BlockMap(-1, NumElements, ColorElementIDs, 
					      ColorElementSizes,
					      Map().IndexBase(), Map().Comm());

  delete [] ColorElementSizes;

  return(map);
}
//=========================================================================
void Epetra_MapColoring::Print(ostream& os) const {
  int MyPID = Map().Comm().MyPID();
  int NumProc = Map().Comm().NumProc();
  
  for (int iproc=0; iproc < NumProc; iproc++) {
    if (MyPID==iproc) {
      int NumMyElements1 =Map(). NumMyElements();
      int * MyGlobalElements1 = Map().MyGlobalElements();

      if (MyPID==0) {
	os.width(8);
	os <<  "     MyPID"; os << "    ";
	os.width(12);
	os <<  "GID  ";
	os.width(20);
	os <<  "Color  ";
	os << endl;
      }
      for (int i=0; i < NumMyElements1; i++) {
	os.width(10);
	os <<  MyPID; os << "    ";
	os.width(10);
	os << MyGlobalElements1[i] << "    ";
	os.width(20);
	os <<  ElementColors_[i];
	os << endl;
      }
      os << flush; 
    }

    // Do a few global ops to give I/O a chance to complete
    Map().Comm().Barrier();
    Map().Comm().Barrier();
    Map().Comm().Barrier();
  }
  return;
}
//=========================================================================
int Epetra_MapColoring::CheckSizes(const Epetra_DistObject& Source) {
  return(0);
}

//=========================================================================
int Epetra_MapColoring::CopyAndPermute(const Epetra_DistObject& Source, int NumSameIDs, 
				       int NumPermuteIDs, int * PermuteToLIDs, 
				       int *PermuteFromLIDs) {

  const Epetra_MapColoring & A = dynamic_cast<const Epetra_MapColoring &>(Source);

  int * From = A.ElementColors();
  int *To = ElementColors_;

  // Do copy first
  if (NumSameIDs>0)
    if (To!=From) {
      for (int j=0; j<NumSameIDs; j++)
	To[j] = From[j];
    }
  // Do local permutation next
  if (NumPermuteIDs>0)
    for (int j=0; j<NumPermuteIDs; j++) 
      To[PermuteToLIDs[j]] = From[PermuteFromLIDs[j]];
  
  return(0);
}

//=========================================================================
int Epetra_MapColoring::PackAndPrepare(const Epetra_DistObject & Source, int NumExportIDs, int * ExportLIDs,
				      int Nsend, int Nrecv,
				      int & LenExports, char * & Exports, int & LenImports, 
				      char * & Imports, 
				      int & SizeOfPacket, Epetra_Distributor & Distor) {


  const Epetra_MapColoring & A = dynamic_cast<const Epetra_MapColoring &>(Source);

  int  * From = A.ElementColors();
  int * IntExports = 0;
  int * IntImports = 0;

  if (Nsend>LenExports) {
    if (LenExports>0) delete [] Exports;
    LenExports = Nsend;
    IntExports = new int[LenExports];
    Exports = (char *) IntExports;
  }

  if (Nrecv>LenImports) {
    if (LenImports>0) delete [] Imports;
    LenImports = Nrecv;
    IntImports = new int[LenImports];
    Imports = (char *) IntImports;
  }

  SizeOfPacket = sizeof(int); 

  int * ptr;

  if (NumExportIDs>0) {
    ptr = (int *) Exports;    
    for (int j=0; j<NumExportIDs; j++) *ptr++ = From[ExportLIDs[j]];
  }
  
  return(0);
}

//=========================================================================
int Epetra_MapColoring::UnpackAndCombine(const Epetra_DistObject & Source,
					 int NumImportIDs, int * ImportLIDs, 
					char * Imports, int & SizeOfPacket, 
					 Epetra_Distributor & Distor, 
					 Epetra_CombineMode CombineMode ) {
  int i, j, jj, k;
  
  if(    CombineMode != Add
      && CombineMode != Zero
      && CombineMode != Insert
      && CombineMode != AbsMax )
    EPETRA_CHK_ERR(-1); //Unsupported CombinedMode, will default to Zero

  if (NumImportIDs<=0) return(0);

  int * To = ElementColors_;

  int * ptr;
  // Unpack it...

  ptr = (int *) Imports;
    
  if (CombineMode==Add)
    for (j=0; j<NumImportIDs; j++) To[ImportLIDs[j]] += *ptr++; // Add to existing value
  else if(CombineMode==Insert)
    for (j=0; j<NumImportIDs; j++) To[ImportLIDs[j]] = *ptr++;
  else if(CombineMode==AbsMax)
    for (j=0; j<NumImportIDs; j++) To[ImportLIDs[j]] = EPETRA_MAX( To[ImportLIDs[j]],abs(*ptr++));
  
  return(0);
}

