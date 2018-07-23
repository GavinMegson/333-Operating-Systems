#ifdef CS333_P2
#include "types.h"
#include "user.h"

int
main(int argc, char *argv[])
{
  // Set in time
  uint in = uptime();

  // Fork
  int pid = fork();

  // Check for error
  if (pid < 0)
  {
    printf(2, "Error: Process could not fork.");
    exit();
  }
  // If child, run next command
  if (pid == 0)
  {
    exec(argv[1], &argv[1]);
  }

  // If parent, wait for child and print elapsed time
  else
  {
    wait();

    uint out = uptime();
    uint elapsed = out - in;
    uint secs = elapsed/1000;
    uint rem = elapsed%1000;


      printf(1,"%s ran in %d.",
        argv[1],
        secs);
      if (rem < 100) printf(1,"0");
      if (rem < 10) printf(1,"0");
      printf(1,"%d seconds.\n",rem);

  }
  exit();
}

#endif
