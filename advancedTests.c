#include "advancedTests.h"

int main(int argc, char* argv[]) {
  advancedTests1();
  advancedTests2();

  printf("Tests Passed\n");
}

extern List_Links* readyQueue;

void advancedTests1() {
  
  int x;
  int y;
  Tid ret;
  //create 500 threads
  for(x = 0; x < 500; x++) {
    ret = ULT_CreateThread((void (*)(void *))fact, (void *)10);
    assert(ret == x+1);
  }
  
  // create and destroy 10 threads 1000 times
  for(x = 0; x < 1000; x++) {
    for(y = 0; y < 10; y++){
      ret = ULT_CreateThread((void (*)(void *))fact, (void *)10);
      assert(ULT_isOKRet(ret));
    }
    for(y = 0; y < 10; y++) {
      ULT_DestroyThread(ULT_ANY);
      assert(ULT_isOKRet(ret));
    }
  }

  // destroy all threads but 0
  for(x = 1; x < ULT_MAX_THREADS; x++) {
    ret = ULT_DestroyThread(x);
    assert(ret == x || ret == ULT_INVALID);
  }

  // 0 only thread left
  ret = ULT_Yield(ULT_ANY);
  assert(ret == ULT_NONE);
}

void advancedTests2() {

  int x;
  Tid ret;
  registerHandler();
  interruptsQuiet();
  startAlarmHelper();

  // create ULT_MAX_THREADS threads
  for(x = 1; x < ULT_MAX_THREADS; x++) {
    ret = ULT_CreateThread((void (*)(void *))fact, (void *)10);
    assert(ret == x);
  }
  // execute all threads
  for(x = 1; x < ULT_MAX_THREADS; x++) {
    ret = ULT_Yield(x);
    assert(ret == x || ret == ULT_INVALID);
  }

  // make sure all threads finish
  assert(List_IsEmpty(readyQueue));

  ULT_DestroyThread(ULT_SELF);
}
    

static int fact(int n) {
  if(n == 1)
    return 1;
  return n*fact(n-1);
}

static void startAlarmHelper() {
  int ret;
  char command[1024];

  pid_t myPid = getpid();
  snprintf(command, 1024, "./alarmHelper %d &\n", myPid);
  ret = system(command);
  printf("Started alarmHelper to speed up ALARM interupts\n");
  return;
}
