#ifdef CS333_P2
#include "types.h"
#include "user.h"

#define MAX 50

int
main(void)
{
  // Set maximum number of entries in uproc array.
  int max = MAX;
  // There is no reason for the uproc table to have more entries than
  // there are processes in the proc table.

  // Allocate uproc table and check for malloc fail, getprocs fail.
  struct uproc *utable = malloc(sizeof(struct uproc)*max);
  if (utable == 0) // If malloc returned NULL
  {
    printf(2,"Error, malloc failed.\n");
    return -1;
  }

  // Numprocs should contain entries in utable
  int numprocs = getprocs(max, utable);
  if (numprocs < 1)
  {
    printf(2,"Error, getprocs failed. numprocs = %d\n", numprocs);
    free(utable);
    return -1;
  }

  // Print header
  printf(1,"PID\tGID\tPPID\tState\tName\tSeconds Elapsed\tCPU Time\tSize\n");

  for (int i = 0; i < numprocs; i++)
  {
    // Ignore unused, embryo processes;
    if (*(utable[i].state) == 0 || *(utable[i].state) == 1) continue;

    // Intermediate values for printinig time elapsed
    int secs = utable[i].elapsed_ticks/1000;
    int rem = utable[i].elapsed_ticks%1000;
    int cpu_secs = utable[i].CPU_total_ticks/1000;
    int cpu_rem = utable[i].CPU_total_ticks%1000;

    // Print each process
    printf(1,"%d\t%d\t%d\t%s\t%s\t%d.",
      utable[i].pid,
      utable[i].gid,
      utable[i].ppid,
      utable[i].state,
      utable[i].name,
      secs); // Time elapsed formatting
        if (rem < 100) printf(1,"0");
        if (rem < 10) printf(1,"0");
    printf(1,"%d\t%d.",
      rem,
      cpu_secs); // CPU time elapsed formatting
        if (cpu_rem < 100) printf(1,"0");
        if (cpu_rem < 10) printf(1,"0");
    printf(1,"%d\t%d\n",
      cpu_rem,
      utable[i].size);

  }
  free(utable);
//  exit();
  return 0;
}
#endif
