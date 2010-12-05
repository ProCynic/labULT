#include <assert.h>
#include <stdlib.h>
#include "ULT.h"


List_Links* readyQueue;
List_Links* zombieList;
ThrdCtlBlk* currentTCB;
ThrdCtlBlk* threads[ULT_MAX_THREADS];
int first = 0;

void init() {
  int x;
  for(x = 0; x < ULT_MAX_THREADS; x++)
    threads[x] = NULL;
  readyQueue = malloc(sizeof(struct List_Links));
  List_Init(readyQueue);
  
  zombieList = malloc(sizeof(struct List_Links));
  List_Init(zombieList);
  
  ThrdCtlBlk* tmp = malloc(sizeof(struct ThrdCtlBlk));
  tmp->tid = 0;
  tmp->ucp = malloc(sizeof(ucontext_t));
  
  getcontext(tmp->ucp);
  
  tmp->ucp->uc_stack.ss_sp = malloc(ULT_MIN_STACK);
  tmp->ucp->uc_stack.ss_size = ULT_MIN_STACK;
  tmp->ucp->uc_stack.ss_flags = 0;
  threads[0] = tmp;
  currentTCB = tmp;  
}

Tid getTid() {
  Tid x;
  for(x = 1; x < ULT_MAX_THREADS; x++)
    if(threads[x] == NULL)
      return x;
  return ULT_NOMORE;
}

Tid ULT_CreateThread(void (*fn)(void *), void *parg)
{
  interruptsOff();
  if(!first){
    
    first =1;
    init();
  }
  
  Tid tid = getTid();  // find an unused tid
  
  if(tid == ULT_NOMORE) {
    return ULT_NOMORE;
  }
  ThrdCtlBlk* tcb = malloc(sizeof(ThrdCtlBlk));
  if(tcb == NULL) // out of heap space
    return ULT_NOMEMORY;
  
  tcb->ucp = malloc(sizeof(ucontext_t));
  if(tcb->ucp == NULL) // out of heap space
    return ULT_NOMEMORY;
  tcb->tid = tid;
  
  
  getcontext(tcb->ucp);
  
  
  tcb->ucp->uc_stack.ss_sp = malloc(ULT_MIN_STACK);
  tcb->ucp->uc_stack.ss_size = ULT_MIN_STACK;
  tcb->ucp->uc_stack.ss_flags = 0;
  
  
  makecontext(tcb->ucp,(void (*)(void))&Stub,2,fn,parg); // seems to be correct
  
  
  threads[tid] = tcb;
  queueAdd(readyQueue, tcb);
  interruptsOn();
  return tid;
}

Tid ULT_Yield(Tid wantTid)
{
  interruptsOff();
  if(!first){
    first =1;
    init();
  }
  
  if(wantTid == ULT_SELF)
    wantTid = currentTCB->tid; //wantTid = tid of current thread
  
  if(wantTid == ULT_ANY) {
    if(List_IsEmpty(readyQueue)) {
      interruptsOn();
      return ULT_NONE;
    }
    wantTid = queueFront(readyQueue);
  }
  
  volatile int done = 0; // things between here and if(!done) get executed twice on yield(self)
  
  
  queueAdd(readyQueue, currentTCB);  
  
  
  getcontext(currentTCB->ucp);  // save current context
  
  ThrdCtlBlk* tmp;
  
  if(!done) {
    
    done = 1;
    tmp = queueGet(readyQueue, wantTid);
    if(tmp == NULL) {
      queueGet(readyQueue, currentTCB->tid);  // will remove from readyQueue
      interruptsOn();
      return ULT_INVALID;
    }
    currentTCB = tmp;
    setcontext(currentTCB->ucp);
  }
  freeZombie();
  interruptsOn();
  return wantTid;
}

Tid ULT_DestroyThread(Tid tid)
{
  interruptsOff();
  if(!first){
    first =1;
    init();
  }
  
  if(tid == ULT_ANY) {
    if(List_IsEmpty(readyQueue))
      return ULT_NONE;
    tid = queueFront(readyQueue);
  }
  
  Tid tmp;
  if(tid == ULT_SELF) {
    queueAdd(zombieList,currentTCB); //put ourself on zombie list
    threads[currentTCB->tid] = NULL; //remove entry in thread table
    // this is an exit point, but we're exiting to yield so we don't need to turn interrupts on
    tmp =  ULT_Yield(ULT_ANY);  //fix to return own tid or ULT_NONE
    interruptsOff();
    if(tmp == ULT_NONE) {
      interruptsOn();
      return tmp;
    }
    interruptsOn();
    return tid;
  }
  
  if(!ULT_isOKRet(tid) || !threads[tid]) {
    interruptsOn();
    return ULT_INVALID;
  }
  
  
  
  ThrdCtlBlk* tcb = threads[tid];
  queueGet(readyQueue, tid);  //removes from readyQueue if in readyQueue
  free(tcb->ucp);
  free(tcb);
  threads[tid] = NULL;
  interruptsOn();
  return tid;
}



// Helper Functions

void queueAdd(List_Links* queue, ThrdCtlBlk* tcb) {  
  listElem* elem = malloc(sizeof(listElem));
  elem->tcb = tcb;
  List_InitElement(&(elem->links));
  List_Insert(&(elem->links), List_Last(queue));  
}

// returns tcb of first element of queue, destructive
ThrdCtlBlk* queuePop(List_Links* queue) {
  listElem* first = (listElem*)List_First(queue);
  List_Remove(List_First(queue));
  free(first);
  return first->tcb;
}

// get tid of first element of queue
Tid queueFront(List_Links* queue) {
  listElem* first = (listElem*)List_First(queue);
  return first->tcb->tid;
}

/*
  ThrdCtlBlk* queueGet(List_Links* queue, Tid tid) {
  
  List_Links* itemPtr;
  listElem* elem;
  LIST_FORALL(queue, itemPtr) {
  elem = (listElem*) itemPtr;
  //   printf(" elem->tcb->tid is %d\n",elem->tcb->tid);
  if (elem->tcb->tid == tid) {
  List_Remove(itemPtr);
  return elem->tcb;
  }
  }
  return NULL;
  }
*/

// do not understand or trust LIST_FORALL
ThrdCtlBlk* queueGet(List_Links* queue, Tid tid) {
  List_Links* current;
  listElem* elem;
  ThrdCtlBlk* ret;
  if(List_IsEmpty(queue))
    return NULL;
  current=List_First(queue);
  while(current != queue) {
    elem = (listElem*)current;
    if(elem->tcb->tid == tid) {
      List_Remove(current);
      ret = elem->tcb;
      free(elem);
      return ret;
    }
    current = current->nextPtr;
  }
  return NULL;
}

void freeZombie() {
  ThrdCtlBlk* tcb;
  while(!List_IsEmpty(zombieList)) {
    tcb = queuePop(zombieList);
    queueGet(readyQueue, tcb->tid);
    free(tcb->ucp);
    free(tcb);
  }
  
}

void Stub(void (*fn)(void *), void *arg){
  // thread starts here
  interruptsOn();
  freeZombie();
  Tid ret;
  fn(arg); // call root function
  ret = ULT_DestroyThread(ULT_SELF);
  printf("ret is: %d\n",ret);
  assert(ret == ULT_NONE); // we should only get here if we are the last thread.
  exit(0); // all threads are done, so process should exit 
} 

//debugging function
void queuePrint(List_Links* queue) {
  List_Links* itemPtr;
  listElem* elem;
  LIST_FORALL(queue, itemPtr) {
    elem = (listElem*) itemPtr;
    printf("\t\t%d\n",elem->tcb->tid);
  }
}

//debugging function
void showThreads() {
  printf("\n\n\n");
  int x;
  for(x = 0; x < ULT_MAX_THREADS; x++)
    if(threads[x] != NULL)
      printf("remaining thread: %d\n",x);
}

void printZombie() {
  printf("Printing Zombie List\n");
  queuePrint(zombieList);
}

void printReady() {
  printf("Printing Ready Queue\n");
  queuePrint(readyQueue);
}
