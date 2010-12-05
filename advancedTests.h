#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ULT.h"
#include "list.h"
#include "interrupt.h"
#include "mailbox.h"

static int fact(int n);
void advancedTests1();
void advancedTests2();
static void startAlarmHelper();

