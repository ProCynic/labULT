#include <assert.h>

#include "interrupt.h"
#include "cond.h"
#include "list.h"
#include "ULT.h"

void Cond_Init(Cond *c, Lock *ls)
{
  assert(0); // TBD
  return;
}

void Cond_Wait(Cond *c, Lock *ls)
{
  assert(0); // TBD
  return;
}

void Cond_Signal(Cond *c, Lock *ls)
{
  assert(0); // TBD
  return;
}

void Cond_Broadcast(Cond *c, Lock *ls)
{
  assert(0); // TBD
  return;
}

/*
 * Return 0 on success. 
 * Return negative on failure.
 *    ERR_COND_BUSY -- threads are waiting on this cond
 */
int 
Cond_Destroy(Cond *c)
{
  assert(0); // TBD
  return 0;
}

