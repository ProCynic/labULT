#include <assert.h>
#include "lock.h"
#include "mailbox.h"
#include "ULT.h"

static void sanity();


void 
mb_init(Mailbox *mb, int workers, int max)
{
  Lock_Init(&(mb->mutex));
  mb->maxCount = max;
  mb->counter = 0;
  mb->partial0 = 0;
  mb->partial1 = 0;
}

void
mb_increment(Mailbox *mb, int whichPartial)
{

  Lock_Acquire(&(mb->mutex));

  sanity(mb);
  ULT_Yield(ULT_ANY);
  sanity(mb);

  mb->counter++;

  ULT_Yield(ULT_ANY);
  ULT_Yield(ULT_ANY);


  ULT_Yield(ULT_ANY);
  ULT_Yield(ULT_ANY);
  if(whichPartial % 2 == 0){
    mb->partial0++;
  }
  else{
    mb->partial1++;
  }
  sanity(mb);

  Lock_Release(&(mb->mutex));
  return;
}


int
mb_checkDone(Mailbox *mb)
{
  int ret = 0;
  Lock_Acquire(&(mb->mutex));

  sanity(mb);
  ULT_Yield(ULT_ANY);
  sanity(mb);

  if(mb->counter == mb->maxCount){
    ret = 1;
  }
  Lock_Release(&(mb->mutex));
  return ret;
}


static void
sanity(Mailbox *mb)
{
  assert(mb->counter <= mb->maxCount);
  assert(mb->partial0 + mb->partial1 == mb->counter);
}
