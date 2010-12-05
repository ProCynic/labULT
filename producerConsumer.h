#ifndef _PRODUCER_CONSUMER_H_
#define _PRODUCER_CONSUMER_H_

#include "list.h"
#include "lock.h"
#include "cond.h"

typedef struct Item{
  List_Links links;
  int color;
} Item;

typedef struct PC{
  Lock lock;
  Cond spaceAvail;
  Cond stuffAvail;
  List_Links list;
  int capacity;
  int used;
  int maxColor;
  int waitingP;
  int waitingC;
} PC;

void PC_Init(PC *pc, int cap, int maxColor);
void PC_Put(PC *pc, int color);
int PC_Get(PC *pc, int color);
void PC_Destroy(PC *pc);

#endif
