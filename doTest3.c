#include <assert.h>
#include <stdio.h>
#include "interrupt.h"
#include "ULT.h"
#include "basicThreadTests.h"

int main(int argc, char **argv)
{
  registerHandler();


  lockTests();
  condTests();

  printf("Done.\n");

  return 0;
}
