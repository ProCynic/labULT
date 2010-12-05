#ifndef _COND_H_
#define _COND_H_
#include "lock.h"

typedef struct Cond{
  /* Your job is to fill this in... */
} Cond;

void Cond_Init(Cond *c, Lock *l );
void Cond_Wait(Cond *c, Lock *l);
void Cond_Signal(Cond *c, Lock *l);
void Cond_Broadcast(Cond *c, Lock *l);
int Cond_Destroy(Cond *c);


static const int ERR_COND_BUSY = -1;
#endif
