#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include "basicThreadTests.h"
#include "interrupt.h"
#include "lock.h"
#include "mailbox.h"
#include "producerConsumer.h"
#include "ULT.h"
/*
 *  Using posix mutex for some tests of phases 1 and 2
 */
#include <pthread.h> 

static void mbWorker0(Mailbox *mb);
static void mbWorker1(Mailbox *mb);

static const int verbose = 0;
static const int vverbose = 0;

static void hello(char *msg);
static int fact(int n);
static void suicide();
static void finale();
static int setFlag(int val);
static void startAlarmHelper();
static void doPotato(int mySlot);
static int tryMovePotato(int mySlot);
static void spin(int secs);

typedef struct PCTestArgs{
  int count;
  int color;
  PC *pc;
  Mailbox *mb;
} PCTestArgs;

static void doPCTest(int nProd, 
                     int nCons, 
                     int nColors, 
                     PC *pc, 
                     Mailbox *mb);
static void pcTestProducer(PCTestArgs *args);
static void pcTestConsumer(PCTestArgs *args);



void basicThreadTests()
{
  Tid ret;
  Tid ret2;

  printf("Starting tests...\n");

  /*
   * Initial thread yields
   */
  ret = ULT_Yield(ULT_SELF);
  assert(ULT_isOKRet(ret));
  printf("Initial thread returns from Yield(SELF)\n");
  ret = ULT_Yield(0); /* See ULT.h -- initial thread must be Tid 0 */
  assert(ULT_isOKRet(ret));
  printf("Initial thread returns from Yield(0)\n");
  ret = ULT_Yield(ULT_ANY);
  assert(ret == ULT_NONE);
  printf("Initial thread returns from Yield(ANY)\n");
  ret = ULT_Yield(0xDEADBEEF);
  assert(ret == ULT_INVALID);
  printf("Initial thread returns from Yield(INVALID)\n");
  ret = ULT_Yield(16);
  assert(ret == ULT_INVALID);
  printf("Initial thread returns from Yield(INVALID2)\n");
  
  /*
   * Create a thread
   */
  ret = ULT_CreateThread((void (*)(void *))hello, "World");
  assert(ULT_isOKRet(ret));
  ret2 = ULT_Yield(ret);
  assert(ret2 == ret);

  /*
   * Create 10 threads
   */
  int ii;
  static const int NTHREAD = 10;
  Tid children[NTHREAD];
  char msg[NTHREAD][1024];
  for(ii = 0; ii < NTHREAD; ii++){
    ret = snprintf(msg[ii], 1023, "Hello from thread %d\n", ii);
    assert(ret > 0);
    children[ii] = ULT_CreateThread((void (*)(void *))hello, msg[ii]);
    assert(ULT_isOKRet(children[ii]));
  }
  for(ii = 0; ii < NTHREAD; ii++){
    ret = ULT_Yield(children[ii]);
    assert(ret == children[ii]);
  }


  /*
   * Destroy 11 threads we just created
   */
  ret = ULT_DestroyThread(ret2);
  assert(ret == ret2);
  for(ii = 0; ii < NTHREAD; ii++){
    ret = ULT_DestroyThread(children[ii]);
    assert(ret == children[ii]);
  }

  /*
   * Create maxthreads-1 threads
   */
  printf("Creating %d threads\n", ULT_MAX_THREADS-1);
  for(ii = 0; ii < ULT_MAX_THREADS-1; ii++){
    ret = ULT_CreateThread((void (*)(void *))fact, (void *)10);
    assert(ULT_isOKRet(ret));
  }
  /*
   * Now we're out of threads. Next create should fail.
   */
  ret = ULT_CreateThread((void (*)(void *))fact, (void *)10);
  assert(ret == ULT_NOMORE);
  /*
   * Now let them all run.
   */
  printf("Running %d threads\n", ULT_MAX_THREADS-1);
  for(ii = 0; ii < ULT_MAX_THREADS; ii++){
    ret = ULT_Yield(ii);
    if(ii == 0){ 
      /* 
       * Guaranteed that first yield will find someone. 
       * Later ones may or may not depending on who
       * stub schedules  on exit.
       */
      assert(ULT_isOKRet(ret));
    }
  }
  /*
   * They should have cleaned themselves up when
   * they finished running. Create maxthreads-1 threads.
   */
  printf("Creating %d threads\n", ULT_MAX_THREADS-1);
  for(ii = 0; ii < ULT_MAX_THREADS-1; ii++){
    ret = ULT_CreateThread((void (*)(void *))fact, (void *)10);
    assert(ULT_isOKRet(ret));
  }
  /*
   * Now destroy some explicitly and let the others run
   */
  printf("Destorying %d threads, running the rest\n",
         ULT_MAX_THREADS/2);
  for(ii = 0; ii < ULT_MAX_THREADS; ii+=2){
    ret = ULT_DestroyThread(ULT_ANY);
    assert(ULT_isOKRet(ret));
  }
  for(ii = 0; ii < ULT_MAX_THREADS; ii++){
    ret = ULT_Yield(ii);
  }
  printf("Trying some destroys even though I'm the only thread\n");
  ret = ULT_DestroyThread(ULT_ANY);
  assert(ret == ULT_NONE);
  ret = ULT_DestroyThread(42);
  assert(ret == ULT_INVALID);
  ret = ULT_DestroyThread(-42);
  assert(ret == ULT_INVALID);
  ret = ULT_DestroyThread(ULT_MAX_THREADS + 1000);
  assert(ret == ULT_INVALID);

  /*
   * Create a tread that destroys itself. 
   * Control should come back here after
   * that thread runs.
   */
  printf("Testing destroy self\n");
  int flag = setFlag(0);
  ret = ULT_CreateThread((void (*)(void *))suicide, NULL);
  assert(ULT_isOKRet(ret));
  ret = ULT_Yield(ret);
  assert(ULT_isOKRet(ret));
  flag = setFlag(0);
  assert(flag == 1); /* Other thread ran */
  /* That thread is gone now */
  ret = ULT_Yield(ret);
  assert(ret == ULT_INVALID);

}

void 
grandFinale()
{
  int ret;
  printf("For my grand finale, I will destroy myself\n");
  printf("while my talented assistant prints Done.\n");
  ret = ULT_CreateThread((void (*)(void *))finale, NULL);
  assert(ULT_isOKRet(ret));
  ULT_DestroyThread(ULT_SELF);
  assert(0);

}


static void
hello(char *msg)
{
  Tid ret;

  printf("Made it to hello() in called thread\n");
  printf("Message: %s\n", msg);
  ret = ULT_Yield(ULT_SELF);
  assert(ULT_isOKRet(ret));
  printf("Thread returns from first yield\n");

  ret = ULT_Yield(ULT_SELF);
  assert(ULT_isOKRet(ret));
  printf("Thread returns from second yield\n");

  while(1){
    ULT_Yield(ULT_ANY);
  }
  
}

static int
fact(int n){
  if(n == 1){
    return 1;
  }
  return n*fact(n-1);
}

static void suicide()
{
  int ret = setFlag(1);
  assert(ret == 0);
  ULT_DestroyThread(ULT_SELF);
  assert(0);
}

/*
 *  Using posix mutex for some tests of phases 1 and 2
 */
static pthread_mutex_t posix_mutex = PTHREAD_MUTEX_INITIALIZER;
static int flagVal;

static int setFlag(int val)
{
  int ret;
  int err;
  err = pthread_mutex_lock(&posix_mutex);
  assert(!err);
  ret = flagVal;
  flagVal = val;
  err = pthread_mutex_unlock(&posix_mutex);
  assert(!err);
  return ret;
}


static void finale()
{
  int ret;
  printf("Finale running\n");
  ret = ULT_Yield(ULT_ANY);
  assert(ret == ULT_NONE);
  ret = ULT_Yield(ULT_ANY);
  assert(ret == ULT_NONE);
  printf("Done.\n");
  /* 
   * Stub should exit cleanly if there are no threads left to run.
   */
  return; 
}


#define NPOTATO  10
static int potato[NPOTATO];
static Tid potatoTids[NPOTATO];
static const int tPotato = 30;

void preemptiveTests()
{

  static const int duration = 10;
  int ret;

  int ii;

  startAlarmHelper();

  spin(2);

  interruptsQuiet();

  potato[0] = 1;
  for(ii = 1; ii < NPOTATO; ii++){
    potato[ii] = 0;
  }

  printf("Running hot potato test. This will take %d seconds\n",
         duration);

  for(ii = 0; ii < NPOTATO; ii++){
    potatoTids[ii] = ULT_CreateThread((void (*)(void *))doPotato, (void *)ii);
    assert(ULT_isOKRet(potatoTids[ii]));
  }


  spin(duration);

  printf("Hot potato done. Cleaning up\n");

  for(ii = 0; ii < NPOTATO; ii++){
    ret = ULT_DestroyThread(ULT_ANY);
    assert(ULT_isOKRet(ret));
  }  

  printf("Done.\n");
}

static void spin(int secs)
{
  struct timeval start, end;
  int ret;

  ret = gettimeofday(&start, NULL);
  assert(!ret);
  while(1){
    ret = gettimeofday(&end, NULL);
    if(end.tv_sec - start.tv_sec >= secs){
      break;
    }
  }
}


static void
startAlarmHelper()
{
  int ret;
  char command[1024];

  pid_t myPid = getpid();
  snprintf(command, 1024, "./alarmHelper %d &\n", myPid);
  ret = system(command);
  printf("Started alarmHelper to speed up ALARM interupts\n");
  return;
}


static void
doPotato(int mySlot)
{
  int ret;
  int moved;
  int passes = 0;

  printf("Made it to doPotato %d\n", mySlot);
  while(1){
    assert(alarmIsEnabled());
    moved = tryMovePotato(mySlot);
    if(moved){
      passes++;
      printf("%d passes potato for %d-st/nd/rd time \n", mySlot, passes);
    }

    /*
     * Add some yields by some threads to scramble the list
     */
    if(mySlot > 4){
      int ii;
      for(ii = 0; ii < mySlot - 4; ii++){
        ret = ULT_Yield(ULT_ANY);
        assert(ULT_isOKRet(ret));
      }
    }
  }
}


static int
tryMovePotato(int mySlot)
{
  int err;
  int ret = 0;

  err = pthread_mutex_lock(&posix_mutex);
  assert(!err);

  if(potato[mySlot]){
    ret = 1;
    potato[mySlot] = 0;
    potato[(mySlot + 1) % NPOTATO] = 1;
  }

  err = pthread_mutex_unlock(&posix_mutex);
  assert(!err);
  return ret;
}


static const int MAXMAIL = 200000;

void 
lockTests()
{

  ULT_Yield(ULT_ANY);
  startAlarmHelper();
  interruptsQuiet();

  /*
   * One mailbox
   */
  Mailbox *mb = (Mailbox *)malloc(sizeof(Mailbox));
  assert(mb);
  mb_init(mb, 2, MAXMAIL);

  ULT_CreateThread((void (*)(void *))mbWorker0, mb);

  while(!mb_checkDone(mb)){
    ULT_Yield(ULT_ANY);
  }
  int jj;
  for(jj = 0; jj < 1000; jj++){
    ULT_Yield(ULT_ANY);
  }
  assert(mb_checkDone(mb));
  free(mb);


  printf("One mailbox, two threads OK\n");

  /*
   * Two mailboxen
   */
  mb = (Mailbox *)malloc(sizeof(Mailbox));
  assert(mb);
  mb_init(mb, 2, MAXMAIL);
  Mailbox *mb2 = (Mailbox *)malloc(sizeof(Mailbox));
  assert(mb2);
  mb_init(mb2, 2, MAXMAIL);

  ULT_CreateThread((void (*)(void *))mbWorker0, mb);
  ULT_CreateThread((void (*)(void *))mbWorker0, mb2);

  while(!mb_checkDone(mb)){
    ULT_Yield(ULT_ANY);
  }
  for(jj = 0; jj < 1000; jj++){
    ULT_Yield(ULT_ANY);
  }
  assert(mb_checkDone(mb));
  while(!mb_checkDone(mb2)){
    ULT_Yield(ULT_ANY);
  }
  for(jj = 0; jj < 1000; jj++){
    ULT_Yield(ULT_ANY);
  }
  assert(mb_checkDone(mb2));

  free(mb);
  free(mb2);
  printf("Two mailboxen, four threads OK\n");

  
  printf("lockTests OK\n");
}


static void
mbWorker0(Mailbox *mb)
{
  ULT_CreateThread((void (*)(void *))mbWorker1, mb);
  
  int myPart = MAXMAIL/2;
  int ii;
  for(ii = 0; ii < myPart; ii++){
    mb_increment(mb, 0);
    ULT_Yield(ULT_ANY);
    ULT_Yield(ULT_ANY);
  }
  return;
}

static void
mbWorker1(Mailbox *mb)
{
  int myPart = MAXMAIL - MAXMAIL/2;
  int ii;
  for(ii = 0; ii < myPart; ii++){
    mb_increment(mb, 1);
    ULT_Yield(ULT_ANY);
  }
  return;
}


void condTests()
{
  int tryProd;
  int tryCons;
  int tryColor;
  int tryBuf;

  int nProd;
  int nCons;
  int nColors;
  int nBuf;

  for(tryProd = 0; tryProd < 2; tryProd++){
    if(tryProd == 0){
      nProd = 1;
    }
    else{
      nProd = 10;
    }
    for(tryCons = 0; tryCons < 2; tryCons++){
      if(tryCons == 0){
        nCons = 1;
      }
      else{
        nCons = 10;
      }
      for(tryColor = 0; tryColor < 2; tryColor++){
        if(tryColor == 0 || nProd == 1 || nCons == 1){
          nColors = 1;
        }
        else{
          nColors = 5;
        }
        for(tryBuf = 0; tryBuf < 3; tryBuf++){
          if(tryBuf == 0){
            nBuf = 1;
          }
          else if(tryBuf == 1){
            nBuf = 5;
          }
          else{
            nBuf = 50;
          }
          
          printf("condTests: nProd=%d nCons=%d nColors=%d nBuf=%d start\n",
                 nProd, nCons, nColors, nBuf);
          Mailbox *mb = (Mailbox *)malloc(sizeof(Mailbox));
          assert(mb);
          mb_init(mb, 1, nProd + nCons);
          PC *pc = (PC *)malloc(sizeof(PC));
          assert(pc);
          PC_Init(pc, nBuf, nColors);
          doPCTest(nProd, nCons, nColors, pc, mb);
          while(!mb_checkDone(mb)){
            /* Real code would make mb_checkDone block... */
          }
          if(verbose){
            printf("Free mb and pc\n");
          }
          free(mb);
          free(pc);
          printf("condTests: nProd=%d nCons=%d nColors=%d nBuf=%d OK\n",
                 nProd, nCons, nColors, nBuf);
        }
      }
    }
  }
}

static const int OP_COUNT = 10000;


static void
doPCTest(int nProd, int nCons, int nColors, PC *pc, Mailbox *mb)
{
  int ii;
  int ret;
  for(ii = 0; ii < nProd; ii++){
    PCTestArgs *args = (PCTestArgs *)malloc(sizeof(PCTestArgs));
    assert(args);
    args->count = OP_COUNT * nCons / nProd;
    args->color = ii % nColors;
    args->pc = pc;
    args->mb = mb;
    if(verbose){
      printf("Creating producer %d %d\n", args->count, args->color);
    }
    ret = ULT_CreateThread(( void (*)(void *))pcTestProducer, args);
  }
  for(ii = 0; ii < nCons; ii++){
    PCTestArgs *args = (PCTestArgs *)malloc(sizeof(PCTestArgs));
    assert(args);
    args->count = OP_COUNT;
    args->color = ii % nColors;
    args->pc = pc;
    args->mb = mb;
    if(verbose){
      printf("Creating consumer %d %d\n", args->count, args->color);
    }
    ret = ULT_CreateThread(( void (*)(void *))pcTestConsumer, args);
  }
  if(verbose){
    printf("All consumers and producers created\n");
  }
}


static void
pcTestProducer(PCTestArgs *args)
{
  int ii, jj;
  for(ii = 0; ii < args->count; ii++){
    if(vverbose){
      printf("producer putting color %d\n", args->color);
    }
    PC_Put(args->pc, args->color);
    if(vverbose){
      printf("producer done putting color %d\n", args->color);
    }
    for(jj = 0; jj < args->color + 1; jj++){
      ULT_Yield(ULT_ANY);
    }
  }
  mb_increment(args->mb, 0);
  free(args);
}

static void
pcTestConsumer(PCTestArgs *args)
{
  int ii, jj;
  for(ii = 0; ii < args->count; ii++){
    if(vverbose){
      printf("consumer getting color %d\n", args->color);
    }
    PC_Get(args->pc, args->color);
    if(vverbose){
      printf("consumer done getting color %d\n", args->color);
    }
    for(jj = 0; jj < args->color + 1; jj++){
      ULT_Yield(ULT_ANY);
    }
  }
  mb_increment(args->mb, 0);
  free(args);
}


