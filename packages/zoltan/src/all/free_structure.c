#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "lb_const.h"
#include "rcb_const.h"


void LB_Free_Structure(
LB *lb)				/* load balance object */
{
/*
 * Free any persistent memory associated with a method.
 */

  switch (lb->Method) {

    case RCB:
      LB_RCB_Free_Structure(lb);
      break;

/*
 * Add calls to additional method-specific free routines here.
 */

    default: {
    }
  }
}
