#ifdef CS333_P3P4
int readydump(void)
{
  struct proc *head = ptable.pLists.ready;
  struct proc *tail = ptable.pLists.readyTail;

  if (head.next == 0)
    cprintf("No ready processes.");
  else
    cprintf("Ready processes: %d",head.pid);
  while (head.next != 0)
  {
    cprintf(" -> %d",head.pid);
    head = head.next;
  }
  cprintf("\n");
  return 0;
}

int freedump(void)
{
  struct proc *head = ptable.pLists.free;
  struct proc *tail = ptable.pLists.freeTail;
  int counter = 0;

  while (head.next != 0)
  {
    counter++;
    head = head.next;
  }
  cprintf("Free list size: %d processes.\n",counter);
  return 0;
}

int sleepdump(void)
{
  struct proc *head = ptable.pLists.sleep;
  struct proc *tail = ptable.pLists.sleepTail;

  if (head.next == 0)
    cprintf("No sleeping processes.");
  else
    cprintf("Sleeping processes: %d",head.pid);
  while (head.next != 0)
  {
    cprintf(" -> %d",head.pid);
    head = head.next;
  }
  cprintf("\n");
  return 0;
}

int zombiedump(void)
{
  struct proc *head = ptable.pLists.zombie;
  struct proc *tail = ptable.pLists.zombieTail;

  if (head.next == 0)
    cprintf("No zombie processes.");
  else
    cprintf("Zombie processes (PID, PPID): (%d,%d)",head.pid, head.parent->pid);
  while (head.next != 0)
  {
    cprintf(" -> (%d,%d)",head.pid,head.parent->pid);
    head = head.next;
  }
  cprintf("\n");
  return 0;
}

#endif
