#ifndef _ULT_H_
#define _ULT_H_
#include <ucontext.h>
#include "list.h"
#include "interrupt.h"


typedef int Tid;
#define ULT_MAX_THREADS 1024
#define ULT_MIN_STACK 32768


typedef struct ThrdCtlBlk{
  Tid tid;
  ucontext_t* ucp;
} ThrdCtlBlk;


/*
 * Tids between 0 and ULT_MAX_THREADS-1 may
 * refer to specific threads and negative Tids
 * are used for error codes or control codes.
 * The first thread to run (before ULT_CreateThread
 * is called for the first time) must be Tid 0.
 */
static const Tid ULT_ANY = -1;
static const Tid ULT_SELF = -2;
static const Tid ULT_INVALID = -3;
static const Tid ULT_NONE = -4;
static const Tid ULT_NOMORE = -5;
static const Tid ULT_NOMEMORY = -6;
static const Tid ULT_FAILED = -7;

static inline int ULT_isOKRet(Tid ret){
  return (ret >= 0  && ret < ULT_MAX_THREADS ? 1 : 0);
}



void init();
struct ThrdCtlBlk* initTCB(Tid tid);
Tid ULT_CreateThread(void (*fn)(void *), void *parg);
Tid ULT_Yield(Tid tid);
Tid ULT_DestroyThread(Tid tid);

/*
 * Hint: yield() puts a thread on the ready queue and then
 * suspends its execution. You might find a suspend(Queue *pq) call useful
 * for Locks where you want to suspend execution of a thread,
 * but want to store the thread on the Lock queue rather than
 * the ready queue. The semantics of suspend are: suspend execution 
 * of this thread, storing its context in the structure pointed
 * to by pq.
 *
 * I happen to have used the (somewhat archaic) list implementation 
 * provided in list.h. You are welcome to use something else for your
 * queues and change the arguments to ULT_Suspend accordintly.
 *
 * Eventually returns 0 on succes. 
 * Immediately returns ULT_NONE if no other ready threads.
 */
int ULT_Suspend(List_Links *lptr);
/*
 * Enable moves a thread from a lock's waiting queue to
 * the system ready queue.
 */
void ULT_Enable(List_Links *lptr);


typedef struct listElem {
  struct List_Links links;
  struct ThrdCtlBlk* tcb;
} listElem;

void queueAdd(List_Links* queue, ThrdCtlBlk* tcb);
struct ThrdCtlBlk* queuePop(List_Links* queue);
Tid queueFront(List_Links* queue);
struct ThrdCtlBlk* queueGet(List_Links* queue, Tid tid);
int queueContains(List_Links* queue, Tid tid);
void Stub(void (*fn)(void *), void *arg);
void freeZombie();


void queuePrint(); //debugging
void showThreads(); //debugging
void printZombie(); //debugging
void printReady(); //debugging

#endif
