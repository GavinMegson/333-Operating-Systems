#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "uproc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      return -1;
    }
    sleep(&ticks, (struct spinlock *)0);
  }
  return 0;
}

// return how many clock tick interrupts have occurred
// since start. 
int
sys_uptime(void)
{
  uint xticks;
  
  xticks = ticks;
  return xticks;
}

//Turn of the computer
int
sys_halt(void){
  cprintf("Shutting down ...\n");
  outw( 0x604, 0x0 | 0x2000);
  return 0;
}

#ifdef CS333_P1
//Get date
int
sys_date(void){
  struct rtcdate *d;
  if(argptr(0, (void *)&d, sizeof(struct rtcdate))<0)
    return -1;
  cmostime(d);
  cprintf("Year: %d Month: %d Day: %d Time: %d:%d (GMT)\n",
    d->year,d->month,d->day,d->hour,d->minute);
  return 0;
}
#endif
#ifdef CS333_P2
// Return user id of current process
uint
sys_getuid(void){
  return proc->uid;
}

// return group id of current process
uint
sys_getgid(void){
  return proc->gid;
}
// return process id of parent process, or self for init
uint
sys_getppid(void){
  return proc->parent ? proc->parent->pid : proc->pid;
}

// set uid - returns -1 if id out of range
int
sys_setuid(void){
  uint id;
  int temp;

  argint(0,&temp);
  id = (uint) temp;

  if (id < 32768) {
    proc->uid = id;
    return 0;
  } else {
    return -1;
  }
}

// set gid - returns -1 if out of range
int
sys_setgid(void){
  uint id;
  int temp;

  argint(0,&temp);
  id = (uint) temp;

   if (id < 32768) {
     proc->gid = id;
     return 0;
   } else {
     return -1;
  }
}

// Populates uproc table
int
sys_getprocs(void) {
  // Get arguments
  int max;
  struct uproc *utable = 0;
  argint(0, (int *) &max);
  argptr(1, (void *) utable, sizeof(struct uproc ) * 50);

  if (max < 1) return -1;
  int counter = 0; // Number of threads

  // Acquire lock/wait for lock (spinlock)
  // acquire(ptable.lock); call into function in proc.c
  getlock();

  // Create uproc array entry for each proc[] entry sizeof(ptable.proc?
  for (int pcount = 0, ucount = 0; ucount < max && pcount < NPROC; pcount++) {

    // This helper function populates a uproc with the ptable[pcount] entry
    struct uproc current = getuproc(pcount);

    if (*current.state == RUNNING || *current.state == SLEEPING || *current.state == RUNNABLE
      || *current.state == ZOMBIE) { // Not UNUSED (0) or EMBRYO (1)

      // Copy data from process struct into uproc struct, and store in new array
      utable[ucount] = current;
      ucount += 1;
      counter += 1;
    } else {
      continue;
    }
  }
  // release(ptable.lock); call into function in proc.c
  freelock();
  return counter;
}
#endif
#ifdef CS333_P3P4
int setpriority(int pid, int priority);

int
sys_setpriority(void)
{
  int pid, priority;
  argint(0,&pid);
  argint(1,&priority);
  return setpriority(pid,priority);
}
#endif
