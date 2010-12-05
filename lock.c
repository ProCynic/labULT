#include <assert.h>

#include "interrupt.h"
#include "lock.h"
#include "list.h"
#include "ULT.h"


void Lock_Init(Lock *l)
{
  assert(0); // TBD
}


void Lock_Acquire(Lock *l)
{
  assert(0); // TBD
}



void Lock_Release(Lock *l)
{
  assert(0); // TBD
}


/*
 * Return 0 on success. 
 * Return negative on failure.
 *    ERR_LOCK_BUSY -- lock is held or threads are waiting on this lock
 */
int Lock_Destroy(Lock *l)
{
  assert(0); // TBD
  return 0;
}






