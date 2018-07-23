#ifdef CS333_P3P4
#include "types.h"
#include "user.h"
#include "stat.h"

void
main(int argc, char *argv[])
{
  int a,b;
  a = fork();
  if (a!=0) {
    b = fork();
    if (b!=0) wait();
    else exit();
  } else sleep(100);
  exit();
}
#endif
