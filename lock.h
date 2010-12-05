#ifndef _LOCK_H_
#define _LOCK_H_


typedef struct Lock{
  /* Your job is to fill this in... */
}Lock;

void Lock_Init(Lock *l);
void Lock_Acquire(Lock *l);
void Lock_Release(Lock *l);
int Lock_Destroy(Lock *l);


static const int ERR_LOCK_BUSY = -1;

#endif
