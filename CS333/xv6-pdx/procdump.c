#ifdef CS333_P3P4
static void
procdumpP3P4(struct proc p, char *state) {

// Intermediate values for printinig time elapsed
    int elapsedticks = ticks - p.start_ticks;
    int secs = elapsedticks/1000;
    int rem = elapsedticks%1000;
    int cpu_secs = p.cpu_ticks_total/1000;
    int cpu_rem = p.cpu_ticks_total%1000;

// Print each process
    cprintf("%d\t%s\t%d\t%d\t%d\t%s\t%d\t%d\t%d.",
      p.pid,
      p.name,
      p.uid,
      p.gid,
      p.parent ? p.parent->pid : p.pid,
      state,
      p.priority,
      p.sz,
      secs); // Time elapsed formatting
        if (rem < 100) cprintf("0");
        if (rem < 10) cprintf("0");
    cprintf("%d\t%d.",
      rem,
      cpu_secs); // CPU time elapsed formatting
        if (cpu_rem < 100) cprintf("0");
        if (cpu_rem < 10) cprintf("0");
    cprintf("%d\n",
      cpu_rem
      );
  return;
}
#elif CS333_P2
static void
procdumpP2(struct proc p, char *state) {

// Intermediate values for printinig time elapsed
    int elapsedticks = ticks - p.start_ticks;
    int secs = elapsedticks/1000;
    int rem = elapsedticks%1000;
    int cpu_secs = p.cpu_ticks_total/1000;
    int cpu_rem = p.cpu_ticks_total%1000;

// Print each process
    cprintf("%d\t%s\t%d\t%d\t%d\t%s\t%d\t%d.",
      p.pid,
      p.name,
      p.uid,
      p.gid,
      p.parent ? p.parent->pid : p.pid,
      state,
      p.sz,
      secs); // Time elapsed formatting
        if (rem < 100) cprintf("0");
        if (rem < 10) cprintf("0");
    cprintf("%d\t%d.",
      rem,
      cpu_secs); // CPU time elapsed formatting
        if (cpu_rem < 100) cprintf("0");
        if (cpu_rem < 10) cprintf("0");
    cprintf("%d\n",
      cpu_rem
      );
  return;
}
#elif CS333_P1
static void
procdumpP1(struct proc p, char *state) {
  cprintf("%d\t%s\t%s", p.pid, state, p.name);
  // Print time elapsed, 1 tic per millisecond
  unsigned int timenow = ticks;
  unsigned int secs_elapsed = (timenow - p.start_ticks) / 1000;
  unsigned int mill_elapsed = (timenow - p.start_ticks) % 1000;
  cprintf("\t%d.",secs_elapsed,mill_elapsed);
  if (mill_elapsed < 100) cprintf("0");
  if (mill_elapsed < 10) cprintf("0");
  cprintf("%d\t\n",mill_elapsed);
}
#endif
