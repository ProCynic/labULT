#include <assert.h>
#include <stdlib.h>
#include "producerConsumer.h"
#include "list.h"
#include "lock.h"
#include "cond.h"

static void addItem(int color, List_Links *list);
static void removeItem(int color, List_Links *list);

void
PC_Init(PC *pc, int cap, int maxColor)
{
  Lock_Init(&pc->lock);
  Cond_Init(&pc->spaceAvail, &pc->lock);
  Cond_Init(&pc->stuffAvail, &pc->lock);
  List_Init(&(pc->list));
  pc->capacity = cap;
  pc->used = 0;
  pc->maxColor = maxColor;
  pc->waitingP = 0;
  pc->waitingC = 0;
  return;
}

void
PC_Put(PC *pc, int color)
{
  Lock_Acquire(&pc->lock);
  assert(color <= pc->maxColor);
  while(pc->capacity == pc->used){
    pc->waitingP++;
    Cond_Wait(&pc->spaceAvail, &pc->lock);
    pc->waitingP--;
  }
  pc->used++;
  addItem(color, &(pc->list));
  assert(((Item *)List_Last(&(pc->list)))->color == color);
  if(pc->used == 1){
    /*
     * Went from empty to non-empty. Signal
     */
    if(pc->maxColor == 0){
      Cond_Signal(&pc->stuffAvail, &pc->lock);
    }
    else{
      Cond_Broadcast(&pc->stuffAvail, &pc->lock);
    }
  }
  assert(((Item *)List_Last(&(pc->list)))->color == color);
  Lock_Release(&pc->lock);
}

int
PC_Get(PC *pc, int color)
{
  Lock_Acquire(&pc->lock);
  assert(color <= pc->maxColor);
  while(List_IsEmpty(&(pc->list)) 
        || (((Item *)List_First(&(pc->list)))->color != color)){
    pc->waitingC++;
    Cond_Wait(&pc->stuffAvail, &pc->lock);
    pc->waitingC--;
  }
  removeItem(color, &(pc->list));
  pc->used--;
  Cond_Signal(&pc->spaceAvail, &pc->lock);
  /* 
   * We just took top item off -- another getter may be
   * able to proceed
   */
  Cond_Broadcast(&pc->stuffAvail, &pc->lock);
  Lock_Release(&pc->lock);
  return color;
}


void
PC_Destroy(PC *pc)
{
  Lock_Acquire(&pc->lock);
  assert(List_IsEmpty(&(pc->list)));
  Lock_Release(&pc->lock);
}


static void
addItem(int color, List_Links *list)
{
  Item *i;
  i = (Item *)malloc(sizeof(Item));
  assert(i);
  List_Init((List_Links *)i);
  i->color = color;
  List_Insert((List_Links *)i, LIST_ATREAR(list));
  assert(((Item *)List_Last(list))->color == color);
  return;
}

static void
removeItem(int color, List_Links *list)
{
  Item *i;
  assert(!List_IsEmpty(list));
  assert(((Item *)List_First(list))->color == color);
  i = (Item *)List_First(list);
  List_Remove((List_Links *)i);
  assert(i->color == color);
  free(i);
  return;
}
