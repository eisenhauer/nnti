/*! \@HEADER */
/*
************************************************************************

                CTrilinos:  C interface to Trilinos
                Copyright (2009) Sandia Corporation

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
Questions? Contact M. Nicole Lemaster (mnlemas\@sandia.gov)

************************************************************************
*/
/*! \@HEADER */


#include "CTrilinos_config.h"
#include "CEpetra_BlockMap.h"
#include "CEpetra_Map.h"
#include "CEpetra_Directory.h"
#include "CEpetra_Distributor.h"
#include "Epetra_Comm.h"
#include "CEpetra_Comm.h"
#include "CEpetra_Comm_Cpp.hpp"
#include "CEpetra_Distributor_Cpp.hpp"
#include "Teuchos_RCP.hpp"
#include "CTrilinos_enums.h"
#include "CTrilinos_exceptions.hpp"
#include "CTrilinos_utils.hpp"
#include "CTrilinos_utils_templ.hpp"

#include "CTrilinos_UnitTestHelpers.hpp"
#include "Teuchos_UnitTestHarness.hpp"


namespace {


/**********************************************************************
CT_Epetra_Comm_ID_t Epetra_Comm_Cast ( CTrilinos_Universal_ID_t id );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_Comm , Cast )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_Comm_ID_t selfID = UnitTest_Create_Comm());

  /* If no exception thrown, then test was successful */
}

/**********************************************************************
CT_Epetra_Comm_ID_t Epetra_Comm_Clone ( CT_Epetra_Comm_ID_t selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_Comm , Clone )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_Comm_ID_t selfID = UnitTest_Create_Comm());

  ECHO(CT_Epetra_Comm_ID_t dupID = Epetra_Comm_Clone(selfID));

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY(dupID.table, CT_Epetra_Comm_ID);
  TEST_EQUALITY_CONST(dupID.index, 1);
  TEST_EQUALITY_CONST(CTrilinos::isSameObject(selfID, dupID), false);
}

/**********************************************************************
void Epetra_Comm_Destroy ( CT_Epetra_Comm_ID_t * selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_Comm , Destroy )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_Comm_ID_t selfID = UnitTest_Create_Comm());

  ECHO(Epetra_Comm_Destroy(&selfID));

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY(selfID.table, CT_Invalid_ID);
  TEST_EQUALITY_CONST(selfID.index, -1);
}

/**********************************************************************
void Epetra_Comm_Barrier ( CT_Epetra_Comm_ID_t selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_Comm , Barrier )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_Comm_ID_t selfID = UnitTest_Create_Comm());

  ECHO(Epetra_Comm_Barrier(selfID));
}

/**********************************************************************
int Epetra_Comm_Broadcast_Double ( 
  CT_Epetra_Comm_ID_t selfID, double * MyVals, int Count, int Root );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_Comm , Broadcast_Double )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_Comm_ID_t selfID = UnitTest_Create_Comm());
  ECHO(int MyPID = Epetra_Comm_MyPID(selfID));

  ECHO(int Root = 0);
  ECHO(const int Count = 6);
  double tmpMyVals[Count] = {4.6, 2.6, 3.1, 7.7, -0.5, 1.0};
  double MyVals[Count];
  if (MyPID == Root) {
    for (int i=0; i<Count; i++) {
      MyVals[i] = tmpMyVals[i];
    }
  }

  ECHO(int ret = Epetra_Comm_Broadcast_Double(selfID, MyVals, Count, Root));
  TEST_EQUALITY_CONST(ret, 0);
  bool match = true;
  for (int i=0; i<Count; i++) {
    if (MyVals[i] != tmpMyVals[i]) match = false;
  }
  TEST_EQUALITY_CONST(match, true);
}

/**********************************************************************
int Epetra_Comm_Broadcast_Int ( 
  CT_Epetra_Comm_ID_t selfID, int * MyVals, int Count, int Root );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_Broadcast_Long ( 
  CT_Epetra_Comm_ID_t selfID, long * MyVals, int Count, int Root );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_Broadcast_Char ( 
  CT_Epetra_Comm_ID_t selfID, char * MyVals, int Count, int Root );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_GatherAll_Double ( 
  CT_Epetra_Comm_ID_t selfID, double * MyVals, double * AllVals, 
  int Count );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_Comm , GatherAll_Double )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_Comm_ID_t selfID = UnitTest_Create_Comm());
  ECHO(int NumProc = Epetra_Comm_NumProc(selfID));
  ECHO(int MyPID = Epetra_Comm_MyPID(selfID));

  ECHO(const int Count = 2);
  double MyVals[Count] = {4.6, 3.1};
  for (int i=0; i<Count; i++) {
    MyVals[i] += MyPID;
  }

  ECHO(int AllCount = NumProc * Count);
  double *AllVals = (double *)malloc(AllCount * sizeof(double));
  TEST_INEQUALITY_CONST(AllVals, 0);

  if (AllVals != NULL) {
    ECHO(int ret = Epetra_Comm_GatherAll_Double(selfID, MyVals, AllVals, Count));
    TEST_EQUALITY_CONST(ret, 0);
  }
}

/**********************************************************************
int Epetra_Comm_GatherAll_Int ( 
  CT_Epetra_Comm_ID_t selfID, int * MyVals, int * AllVals, int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_GatherAll_Long ( 
  CT_Epetra_Comm_ID_t selfID, long * MyVals, long * AllVals, 
  int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_SumAll_Double ( 
  CT_Epetra_Comm_ID_t selfID, double * PartialSums, 
  double * GlobalSums, int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_SumAll_Int ( 
  CT_Epetra_Comm_ID_t selfID, int * PartialSums, int * GlobalSums, 
  int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_SumAll_Long ( 
  CT_Epetra_Comm_ID_t selfID, long * PartialSums, long * GlobalSums, 
  int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_MaxAll_Double ( 
  CT_Epetra_Comm_ID_t selfID, double * PartialMaxs, 
  double * GlobalMaxs, int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_MaxAll_Int ( 
  CT_Epetra_Comm_ID_t selfID, int * PartialMaxs, int * GlobalMaxs, 
  int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_MaxAll_Long ( 
  CT_Epetra_Comm_ID_t selfID, long * PartialMaxs, long * GlobalMaxs, 
  int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_MinAll_Double ( 
  CT_Epetra_Comm_ID_t selfID, double * PartialMins, 
  double * GlobalMins, int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_MinAll_Int ( 
  CT_Epetra_Comm_ID_t selfID, int * PartialMins, int * GlobalMins, 
  int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_MinAll_Long ( 
  CT_Epetra_Comm_ID_t selfID, long * PartialMins, long * GlobalMins, 
  int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_ScanSum_Double ( 
  CT_Epetra_Comm_ID_t selfID, double * MyVals, double * ScanSums, 
  int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_ScanSum_Int ( 
  CT_Epetra_Comm_ID_t selfID, int * MyVals, int * ScanSums, 
  int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_ScanSum_Long ( 
  CT_Epetra_Comm_ID_t selfID, long * MyVals, long * ScanSums, 
  int Count );
 **********************************************************************/

/**********************************************************************
int Epetra_Comm_MyPID ( CT_Epetra_Comm_ID_t selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_Comm , MyPID )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_Comm_ID_t selfID = UnitTest_Create_Comm());

  ECHO(int ret = Epetra_Comm_MyPID(selfID));

  /* Now check the result of the call to the wrapper function */
  ECHO(bool valid = (ret >= 0));
  TEST_EQUALITY_CONST(valid, true);
}

/**********************************************************************
int Epetra_Comm_NumProc ( CT_Epetra_Comm_ID_t selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_Comm , NumProc )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_Comm_ID_t selfID = UnitTest_Create_Comm());

  ECHO(int ret = Epetra_Comm_NumProc(selfID));

  /* Now check the result of the call to the wrapper function */
  ECHO(bool valid = (ret >= 1));
  TEST_EQUALITY_CONST(valid, true);
}

/**********************************************************************
CT_Epetra_Distributor_ID_t Epetra_Comm_CreateDistributor ( 
  CT_Epetra_Comm_ID_t selfID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_Comm , CreateDistributor )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_Comm_ID_t selfID = UnitTest_Create_Comm());

  ECHO(CT_Epetra_Distributor_ID_t disID = Epetra_Comm_CreateDistributor(selfID));

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY(disID.table, CT_Epetra_Distributor_ID);
  TEST_EQUALITY_CONST(disID.index, 0);

  /* Check more thoroughly */
  ECHO(Teuchos::RCP<Epetra_Distributor> r = CEpetra::getDistributor(disID));
  TEST_EQUALITY_CONST(r.is_null(), false);
}

/**********************************************************************
CT_Epetra_Directory_ID_t Epetra_Comm_CreateDirectory ( 
  CT_Epetra_Comm_ID_t selfID, CT_Epetra_BlockMap_ID_t MapID );
 **********************************************************************/

TEUCHOS_UNIT_TEST( Epetra_Comm , CreateDirectory )
{
  ECHO(CEpetra_Test_CleanSlate());

  ECHO(CT_Epetra_Comm_ID_t selfID = UnitTest_Create_Comm());

  ECHO(int NumGlobalElements = 9);
  ECHO(int IndexBase = 0);
  ECHO(CT_Epetra_BlockMap_ID_t MapID = Epetra_BlockMap_Cast(Epetra_Map_Abstract(
       Epetra_Map_Create(NumGlobalElements, IndexBase, selfID))));
  ECHO(CT_Epetra_Directory_ID_t dirID = Epetra_Comm_CreateDirectory(selfID, MapID));

  /* Now check the result of the call to the wrapper function */
  TEST_EQUALITY(dirID.table, CT_Epetra_Directory_ID);
  TEST_EQUALITY_CONST(dirID.index, 0);
}


/**********************************************************************/

//
// Template Instantiations
//


#ifdef TEUCHOS_DEBUG

#  define DEBUG_UNIT_TEST_GROUP( T ) \

#else

#  define DEBUG_UNIT_TEST_GROUP( T )

#endif


#define UNIT_TEST_GROUP( T ) \
  DEBUG_UNIT_TEST_GROUP( T )


} // namespace

