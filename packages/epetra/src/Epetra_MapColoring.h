
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

#ifndef EPETRA_MAPCOLORING_H
#define EPETRA_MAPCOLORING_H
#include "Epetra_DistObject.h"
#include "Epetra_BlockMap.h"
class Epetra_HashTable;
class Epetra_Map;

//! Epetra_MapColoring: A class for coloring Epetra_Map and Epetra_BlockMap objects.

/*! This class allows the user to associate an integer value, i.e., a color, to each element of 
    an existing Epetra_Map or Epetra_BlockMap object.  Colors may be assigned at construction, or 
    via set methods.  Any elements that are not explicitly assigned a color are assigned the color 
    0 (integer zero).  Color information may be accessed in three basic ways:
  <ol> 
  <li> By local element ID (LID) - Returns the color of a specified LID, where the LID is associated
       with the Epetra_Map or BlockMap that was passed in to the Epetra_MapColoring constructor.
  <li> By global element ID (GID) - Returns the color of the specified GID.  There two methods
       for accessing GIDs, one assumes the request is for GIDs owned by the calling processor,
       the second allows arbitrary requested for GIDs, as long as the GID is defined on some processor
       for the Epetra_Map or Epetra_BlockMap.
  <li> By color groups - LIDs and GIDs are grouped by color so that all elements of a given color can
       be accessed.
  </ol>

*/

class Epetra_MapColoring: public Epetra_DistObject {
    
  public:

  //@{ \name Constructors/destructors.
  //! Epetra_MapColoring basic constructor.
  /*!
    \param In
            Map - An Epetra_Map or Epetra_BlockMap (Note: Epetra_BlockMap is a base class of
	    Epetra_Map, so either can be passed in to this constructor.
    \param In
            DefaultColor - The integer value to use as the default color for this map.  This constructor
	    will initially define the color of all map elements to the default color.

    \return Pointer to a Epetra_MapColoring object.

  */ 
  Epetra_MapColoring(const Epetra_BlockMap& Map, const int DefaultColor = 0);

  //! Epetra_MapColoring constructor.
  /*!
    \param In
            ColorList - Array of dimension Map.NumMyElements() containing the list of colors
            that should be assigned the map elements on this processor. If this argument is
	    set to 0 (zero), all elements will initially be assigned color 0 (zero).  Element
	    colors can be modified by using methods described below.
    
    \param In
            Map - An Epetra_Map or Epetra_BlockMap (Note: Epetra_BlockMap is a base class of
	    Epetra_Map, so either can be passed in to this constructor.

    \return Pointer to a Epetra_MapColoring object.

  */ 
  Epetra_MapColoring(int * ElementColors, const Epetra_BlockMap& Map, const int DefaultColor = 0);

  //! Epetra_MapColoring copy constructor.
  
  Epetra_MapColoring(const Epetra_MapColoring& Source);
  
  //! Epetra_MapColoring destructor.
  
  virtual ~Epetra_MapColoring();
  //@}
  
  //@{ \name Set Color methods.
  
  //! LID element color assignment method.
  /*! Allows color assignment of ith LID: colormap[i] = color
    \return MapColor(LID).
  */
  int& operator [] (int LID) {ListsAreValid_ = false; return ElementColors_[LID];};
  
  //! GID element color assignment method, Note:  Valid only for GIDs owned by calling processor.
  /*! Allows color assignment of specified GID \e only if the GID is owned by map on 
      the calling processor. If you are unsure about the ownership of a GID, check by using the MyGID()
      method.  MyGID(GID) returns true if the GID is owned by the calling processor.
      \return MapColor(GID).
  */
  int& operator () (int GID) {ListsAreValid_ = false; return ElementColors_[Map().LID(GID)];};
  //@}
  
  //@{ \name Local/Global color accessor methods.
  //! LID element color access method.
  /*! Returns color  of ith LID: colormap[i] = color
    \return MapColor[LID].
  */
  const int& operator [] (int LID) const { return ElementColors_[LID];};
  
  //! GID element color assignment method, Note:  Valid only for GIDs owned by calling processor.
  /*! Allows color assignment of specified GID \e only if the GID is owned by map on 
      the calling processor. If you are unsure about the ownership, check by using the MyGID()
      method on the map object.
    \return MapColor(GID).
  */
  const int& operator () (int GID) const {return ElementColors_[Map().LID(GID)];};
  //@}
  
  //@{ \name Color Information Access Methods.
  //! Returns number of colors.
  int NumColors() const {if (!ListsAreValid_) GenerateLists(); return(NumColors_);};
  
  //! Array of length NumColors() containing List of color values used in this coloring.
  /*! Color values can be arbitrary integer values.  As a result, a user of a previously
      constructed MapColoring object may need to know exactly which color values are present.
      This array contains that information as a sorted list of integer values.
  */
  int * ListOfColors() const {if (!ListsAreValid_) GenerateLists(); return(ListOfColors_);};
  
  //! Returns default color.
  int DefaultColor() const {return(DefaultColor_);};
  
  //! Returns number of map elements on calling processor having specified Color
  int NumElementsWithColor(int Color) const;
  
  //! Returns pointer to array of Map LIDs associated with the specified color.
  /*! Returns a pointer to a list of Map LIDs associated with the specified color. 
    This is a purely local list with no information about other processors.  If there
    are no LIDs associated with the specified color, the pointer is set to zero.
  */
  int * ColorLIDList(int Color) const;
  
  //! Returns pointer to array of the colors associated with the LIDs on the calling processor.
  /*! Returns a pointer to the list of colors associated with the elements on this processor
    such that ElementColor[LID] is the color assigned to that LID.
  */
  int * ElementColors() const{if (!ListsAreValid_) GenerateLists(); return(ElementColors_);};
  
  //@}
  //@{ \name Epetra_Map and Epetra_BlockMap generators.
  //! Generates an Epetra_Map of the GIDs associated with the specified color.
  /*! This method will create an Epetra_Map such that on each processor the GIDs associated with
    the specified color will be part of the map on that processor.  Note that this
    method always generates an Epetra_Map, not an Epetra_BlockMap, even if the map associated
    with this map coloring is a block map.  Once the map is generated, the user is responsible for
    deleting it.
  */
  Epetra_Map * GenerateMap(int Color) const;
  
  //! Generates an Epetra_BlockMap of the GIDs associated with the specified color.
  /*! This method will create an Epetra_BlockMap such that on each processor the GIDs associated with
    the specified color will be part of the map on that processor.  Note that this
    method will generate an Epetra_BlockMap such that each element as the same element size as the
    corresponding element of map associated with the map coloring.  
    Once the map is generated, the user is responsible for
    deleting it.
  */
  Epetra_BlockMap * GenerateBlockMap(int Color) const;
  //@}
  
  //@{ \name I/O methods
  
  //! Print method
  virtual void Print(ostream & os) const;
  //@}
  
 private:
  int Allocate(int * ElementColors, int Increment);
  int GenerateLists() const;
  int DeleteLists() const;
  bool InItemList(int ColorValue) const;
  
   // Routines to implement Epetra_DistObject virtual methods

  int CheckSizes(const Epetra_DistObject& A);
  int CopyAndPermute(const Epetra_DistObject & Source, int NumSameIDs, 
			 int NumPermuteIDs, int * PermuteToLIDs, int * PermuteFromLIDs);

  int PackAndPrepare(const Epetra_DistObject & Source, int NumExportIDs, int * ExportLIDs,
				      int Nsend, int Nrecv,
				      int & LenExports, char * & Exports, int & LenImports, 
				      char * & Imports, 
				      int & SizeOfPacket, Epetra_Distributor & Distor);
  
  int UnpackAndCombine(const Epetra_DistObject & Source,
		       int NumImportIDs, int * ImportLIDs, 
		       char * Imports, int & SizeOfPacket, 
		       Epetra_Distributor & Distor, Epetra_CombineMode CombineMode );


  struct ListItem {
    ListItem * NextItem;
    int ItemValue;
    
    ListItem( const int itemValue = 0, ListItem * nextItem = 0)
      : ItemValue(itemValue), NextItem(nextItem){}
  };
  
  int DefaultColor_;
  mutable Epetra_HashTable * ColorIDs_;
  mutable ListItem * FirstColor_;
  mutable int NumColors_;
  mutable int * ListOfColors_;
  mutable int * ColorCount_;
  mutable int * ElementColors_;
  mutable int ** ColorLists_;
  bool Allocated_;
  mutable bool ListsAreGenerated_;
  mutable bool ListsAreValid_;
  
  
};

#endif /* EPETRA_MAPCOLORING_H */
