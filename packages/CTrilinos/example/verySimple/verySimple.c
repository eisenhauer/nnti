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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include "CTrilinos_enums.h"
#include "CTrilinos_flex_enums.h"
#include "CTrilinos_table_man.h"
#include "CEpetra_SerialComm.h"
#include "CEpetra_Comm.h"
#include "CEpetra_Map.h"
#include "CEpetra_BlockMap.h"
#include "CEpetra_Vector.h"
#include "CEpetra_MultiVector.h"

/*! @file verySimple.c
 * This is a very simple example of how to use the CTrilinos interface
 * to Epetra.  This example follows from the Epetra example by the same
 * name.
 */

int main( int argc, char* argv[] )
{
  
  /*
   * Data declarations (how old-school is this!)
   */

  int numGlobalElements, indexBase, numGlobalElements_rtn, numMyElements;

  CT_Epetra_SerialComm_ID_Flex_t commID;
  CT_Epetra_Map_ID_Flex_t mapID;
  CT_Epetra_Vector_ID_Flex_t xID, bID;

  double bnorm, xnorm, expected_bnorm, expected_xnorm, bnorm_err, xnorm_err, err_tol;

  /* Since C doesn't support bool, CTrilinos offers a boolean enum
   * that has FALSE=0, TRUE=1 */
  boolean success = TRUE;

  /*
   * Executable code
   */
  
  /* Create an Epetra_SerialComm and cast to an Epetra_Comm so that
   * it can be passed to functions expecting the latter */
  commID.Epetra_SerialComm = Epetra_SerialComm_Create();
  CT_Migrate(&commID.universal, CT_Epetra_Comm_ID);

  /* Create an Epetra_Map and cast to an Epetra_BlockMap so that
   * a) it can be passed to functions expecting the latter and
   * b) methods implemented only in BlockMap can be invoked on the Map */
  numGlobalElements = 4;
  indexBase = 0;  /* use indexBase = 0 unless you know what you're doing! */
  mapID.Epetra_Map = Epetra_Map_Create(numGlobalElements, indexBase, commID.Epetra_Comm);
  CT_Migrate(&mapID.universal, CT_Epetra_BlockMap_ID);

  /* Check the properties of the map */
  numGlobalElements_rtn = Epetra_BlockMap_NumGlobalElements(mapID.Epetra_BlockMap);
  printf( "NumGlobalElements = %d\n", numGlobalElements_rtn );
  assert( numGlobalElements == numGlobalElements_rtn );

  numMyElements = Epetra_BlockMap_NumMyElements(mapID.Epetra_BlockMap);
  printf( "NumMyElements = %d\n", numMyElements);
  
  /* Create an Epetra_Vector and cast to an Epetra_MultiVector so that
   * methods implemented only in MultiVector can be invoked on the Vector */
  xID.Epetra_Vector = Epetra_Vector_Create(mapID.Epetra_BlockMap, TRUE);  /* zero this one */
  CT_Migrate(&xID.universal, CT_Epetra_MultiVector_ID);

  /* Do the same thing, but do not initialize this one to zero */
  bID.Epetra_Vector = Epetra_Vector_Create(mapID.Epetra_BlockMap, FALSE);
  CT_Migrate(&bID.universal, CT_Epetra_MultiVector_ID);

  /* Do some vector operations */
  Epetra_MultiVector_PutScalar(bID.Epetra_MultiVector, 2.0);
  Epetra_MultiVector_Update_WithA(xID.Epetra_MultiVector, 2.0, bID.Epetra_MultiVector, 0.0); /* x = 2*b */

  Epetra_MultiVector_Norm2(bID.Epetra_MultiVector, &bnorm);
  Epetra_MultiVector_Norm2(xID.Epetra_MultiVector, &xnorm);

  printf( "2 norm of x = %f\n", xnorm );
  printf( "2 norm of b = %f\n", bnorm );

  /* Test the expected value */
  err_tol = 0.0;
  expected_bnorm = sqrt( 2.0 * 2.0 * numGlobalElements );
  expected_xnorm = sqrt( 4.0 * 4.0 * numGlobalElements );
  bnorm_err = fabs( expected_bnorm - bnorm ) / expected_bnorm;
  xnorm_err = fabs( expected_xnorm - xnorm ) / expected_xnorm;
  printf( "error in 2 norm of x = %f\n", bnorm_err );
  printf( "error in 2 norm of b = %f\n", xnorm_err );

  if (bnorm_err > err_tol) success = FALSE;
  if (xnorm_err > err_tol) success = FALSE;

  /* Clean up memory (in reverse order)! */
  Epetra_MultiVector_Destroy(&xID.Epetra_MultiVector);
  Epetra_MultiVector_Destroy(&bID.Epetra_MultiVector);

  Epetra_BlockMap_Destroy(&mapID.Epetra_BlockMap);
  Epetra_Comm_Destroy(&commID.Epetra_Comm);

  /* This should throw an exception and print an error message
   * since the object has already been destroyed! */
  /* Epetra_BlockMap_NumGlobalElements(mapID.Epetra_BlockMap); */

  if (success == TRUE)
    printf( "\nEnd Result: TEST PASSED\n" );
  else
    printf( "\nEnd Result: TEST FAILED\n" );
  
  return ( (success == TRUE) ? 0 : 1 );
}
