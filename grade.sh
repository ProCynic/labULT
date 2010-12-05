#!/bin/sh

echo
echo ====================================================
echo grade.sh does some very simple tests on the text
echo output of your program. Passing grade.sh is
echo not a guarantee that your code is correct or
echo that it will receive a good grade. You 
echo should also do your own tests.
echo
echo Attempting to mislead the grading script
echo is not permitted and will be treated as
echo academic dishonesty.
echo ====================================================
echo
make -k
passed=0
failed=0

echo "Test part 1"
./doTest > doTest.grade.out
if (cat doTest.grade.out; echo XYZ_BREAK_TOKEN_XYZ; cat doTest.expected) | awk -f checkAll.awk | grep "checkAll-OK" > /dev/null
then
   passed=`expr 1 + $passed`
   echo OK: Basic non-preemptive looks OK
else
   failed=`expr 1 + $failed`
   echo ***********
   (cat doTest.grade.out; echo XYZ_BREAK_TOKEN_XYZ; cat doTest.expected) | awk -f checkAll.awk
   echo Basic non-preemptive looks broken
   echo ***********
fi


echo "Test part 2 (this will take 10-15 seconds)"
./doTest2 | grep -v "AlarmHelper" > doTest2.grade.out
if (cat doTest2.grade.out; echo XYZ_BREAK_TOKEN_XYZ; cat doTest2.expected) | awk -f checkAll.awk | grep "checkAll-OK" > /dev/null
then
   passed=`expr 1 + $passed`
   echo OK: Basic tests still look OK
else
   failed=`expr 1 + $failed`
   echo ***********
   (cat doTest2.grade.out; echo XYZ_BREAK_TOKEN_XYZ; cat doTest2.expected) | awk -f checkAll.awk
   echo Basic preemptive looks broken
   echo ***********
fi


