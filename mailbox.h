#ifndef _MAILBOX_H_
#define _MAILBOX_H_
#include "lock.h"

typedef struct Mailbox{
  Lock mutex;
  int counter;
  int maxCount;
  int partial0;
  int partial1;
} Mailbox;

void mb_init(Mailbox *mb, int workers, int maxCount);
void  mb_increment(Mailbox *mb, int whichPartial);
int mb_checkDone(Mailbox *mb);
#endif
