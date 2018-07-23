#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "procdump.c"
#include "uproc.h"

#ifdef CS333_P3P4
struct StateLists {
  struct proc* ready[MAXPRIO+1];
  struct proc* readyTail[MAXPRIO+1];
  struct proc* free;
  struct proc* freeTail;
  struct proc* sleep;
  struct proc* sleepTail;
  struct proc* zombie;
  struct proc* zombieTail;
  struct proc* running;
  struct proc* runningTail;
  struct proc* embryo;
  struct proc* embryoTail;
};
#endif 


struct {
  struct spinlock lock;
  struct proc proc[NPROC];
#ifdef CS333_P3P4
  struct StateLists pLists;
  uint PromoteAtTime;
#endif
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);


#ifdef CS333_P3P4
static void initProcessLists(void);
static void initFreeList(void);
static int stateListAdd(struct proc** head, struct proc** tail, struct proc* p);
static int stateListRemove(struct proc** head, struct proc** tail, struct proc* p);
static int assertState(struct proc *p, enum procstate state);
//int setpriority(int pid, int priority);
#endif

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
#ifdef CS333_P3P4
  // Take unallocated free process off the free list
  p = ptable.pLists.free;
  int rc = stateListRemove(&ptable.pLists.free,&ptable.pLists.freeTail,p);
  if (rc == 0) goto found;
#else
  // take off free list
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
#endif
  release(&ptable.lock);
  return 0;

found:
#ifdef CS333_P3P4
  // add to embryo
  assertState(p, UNUSED);
  p->state = EMBRYO;
  p->budget = BUDGET;
  p->priority = 0; //All new processes start with priority 0
  rc = stateListAdd(&ptable.pLists.embryo,&ptable.pLists.embryoTail,p);
  if (rc != 0) panic("rc !=0 allocproc\n");
#endif
  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
#ifdef CS333_P3P4
    cprintf("Allocate kernel stack fail, allocproc\n");
    // if kalloc fails, turn embryo back into free
    // equivalent to p->state = UNUSED
    acquire(&ptable.lock);
    rc = stateListRemove(&ptable.pLists.embryo,&ptable.pLists.embryoTail,p);
    assertState(p,EMBRYO);
    p->state = UNUSED;
    rc += stateListAdd(&ptable.pLists.free,&ptable.pLists.freeTail,p);
    if (rc != 0) panic("Crazy things happening\n");
#endif
    p->state = UNUSED;
    release(&ptable.lock);
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
#ifdef CS333_P1
  p->start_ticks = ticks;
#endif
#ifdef CS333_P2
  p->cpu_ticks_total = 0;
  p->cpu_ticks_in = 0;
#endif
/*
#ifdef CS333_P3P4
  // set priority and budget to defaults
  p->priority = 0;
  p->budget = BUDGET;
#endif
*/
  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
#ifdef CS333_P3P4
  // Initialize StateLists, set promotion time
  getlock();
  ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
  initProcessLists();
  initFreeList();
  freelock();
#endif

  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  acquire(&ptable.lock);
#ifdef CS333_P3P4
  int rc = stateListRemove(&ptable.pLists.embryo,
    &ptable.pLists.embryoTail,p);
  assertState(p,EMBRYO);
  p->state = RUNNABLE;
  rc += stateListAdd(&ptable.pLists.ready[p->priority],
    &ptable.pLists.readyTail[p->priority],p);
  if (rc!=0) panic("userinit \n");
#endif
  p->state = RUNNABLE;
  release(&ptable.lock);

#ifdef CS333_P2
  // uid, gid, parent (parent of init is 0)
  p->uid = 0;
  p->gid = 0;
  p->parent = 0;
#endif
#ifdef CS333_P3P4
  p->next = 0;
#endif
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;

  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
#ifdef CS333_P3P4
  int rc;
#endif

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    acquire(&ptable.lock);
#ifdef CS333_P3P4
    rc = stateListRemove(&ptable.pLists.embryo,&ptable.pLists.embryoTail,np);
    assertState(np,EMBRYO);
    np->state = UNUSED;
    rc += stateListAdd(&ptable.pLists.free,&ptable.pLists.freeTail,np);
    if (rc != 0) panic("FORK: rc != 0");
#endif
    np->state = UNUSED;
    release(&ptable.lock);
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));

  pid = np->pid;
#ifdef CS333_P2
  // copy over uid, gid
  np->uid = proc->uid;
  np->gid = proc->gid;
#endif
  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
#ifdef CS333_P3P4
  rc = stateListRemove(&ptable.pLists.embryo,&ptable.pLists.embryoTail,np);
  assertState(np,EMBRYO);
  np->state = RUNNABLE;
  // Copy over priority
  np->priority = proc->priority;
  rc += stateListAdd(&ptable.pLists.ready[np->priority],
    &ptable.pLists.readyTail[np->priority],np);
  if (rc !=0) panic("state list error FORK");
#endif
  np->state = RUNNABLE;
  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
#ifndef CS333_P3P4
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}
#else
void
exit(void)
{
  struct proc *p;
  int fd, rc;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init, one list at a time
  for(p = ptable.pLists.running; p != 0; p = p->next){
    if(p->parent == proc){
      p->parent = initproc;
    }
  }

  for (int i = 0; i < MAXPRIO+1; i++){
    for(p = ptable.pLists.ready[i]; p != 0; p = p->next){
      if(p->parent == proc){
        p->parent = initproc;
      }
    }
  }

  for(p = ptable.pLists.embryo; p != 0; p = p->next){
    if(p->parent == proc){
      p->parent = initproc;
    }
  }

  for(p = ptable.pLists.sleep; p != 0; p = p->next){
    if(p->parent == proc){
      p->parent = initproc;
    }
  }

  // Also call wakeup for zombie processes
  for(p = ptable.pLists.zombie; p != 0; p = p->next){
    if(p->parent == proc){
      p->parent = initproc;
      wakeup1(initproc);
    }
  }

  // Change state and jump into the scheduler, never to return.
  rc = stateListRemove(&ptable.pLists.running,
    &ptable.pLists.runningTail,proc);
  assertState(proc,RUNNING);
  proc->state = ZOMBIE;
  rc += stateListAdd(&ptable.pLists.zombie,
    &ptable.pLists.zombieTail,proc);
  if (rc!=0) panic("exit\n");

  sched();
  panic("zombie exit");
}
#endif

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
#ifndef CS333_P3P4
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#else
int
wait(void)
{
  struct proc *curr;
  int pid, rc;
  int havekids = 0;

  acquire(&ptable.lock);
  for(;;){
    // search zombie list for children
    curr = ptable.pLists.zombie;
    while (curr != 0) {
      if (curr->parent->pid == proc->pid) {
	rc = stateListRemove(&ptable.pLists.zombie,
	  &ptable.pLists.zombieTail,curr);
        pid = curr->pid;
        kfree(curr->kstack);
        curr->kstack = 0;
        assertState(curr,ZOMBIE);
        curr->state = UNUSED;
        freevm(curr->pgdir);
	curr->pid = 0;
        curr->parent = 0;
        curr->name[0] = 0;
        curr->killed = 0;
	rc += stateListAdd(&ptable.pLists.free,
	  &ptable.pLists.freeTail,curr);
	if (rc != 0) panic("wait error\n");
	release(&ptable.lock);
	return pid;
      }
      curr = curr->next;
    }
  // search all the other lists (except free) for
  // any children
  for (int i = 0; i<MAXPRIO+1; i++) {
    curr = ptable.pLists.ready[i];
    while (curr!=0) {
      if (curr->parent->pid==proc->pid) {
      havekids = 1;
      break;
      }
      curr = curr->next;
    }
  }

  curr = ptable.pLists.sleep;
  while (curr!=0) {
    if (curr->parent->pid==proc->pid) {
    havekids = 1;
    break;
    }
    curr = curr->next;
  }

  curr = ptable.pLists.running;
  while (curr!=0) {
    if (curr->parent->pid==proc->pid) {
    havekids = 1;
    break;
    }
    curr = curr->next;
  }
  curr = ptable.pLists.embryo;
  while (curr!=0) {
    if (curr->parent->pid==proc->pid) {
    havekids = 1;
    break;
    }
    curr = curr->next;
  }

  // If no children, leave
  if (havekids == 0) {
    release(&ptable.lock);
    return -1;
  }

  // If process killed, leave
  if (proc->killed) {
    release(&ptable.lock);
    return -1;
  }

  // If nothing else, sleep
  sleep(proc, &ptable.lock);
  }
}
#endif

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifndef CS333_P3P4
// original xv6 scheduler. Use if CS333_P3P4 NOT defined.
void
scheduler(void)
{
  struct proc *p;
  int idle;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      idle = 0;  // not idle this timeslice
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
#ifdef CS333_P2
      p->cpu_ticks_in = ticks;
#endif
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
  }
}

#else
void
scheduler(void)
{
  struct proc *p;
  int idle;

  for (;;) {
    sti();
    idle = 1;

    acquire(&ptable.lock);

    // Check ticks against promotion deadline
    if (ptable.PromoteAtTime <= ticks) {

      // If it is time to promote, promote relevant lists
      struct proc *curr;
      struct proc *next;

      // Running:
      curr = ptable.pLists.running;
      while (curr != 0) {
	next = curr->next;
	if (curr->priority < 0)
	  panic("priority < 0, scheduler, running\n");
	if (curr->priority > 0)
	  curr->priority -= 1;
	curr = next;
      }

      // Each priority list in runnable (except priority 0):
      for (int i = 1; i <= MAXPRIO; i++) {
	curr = ptable.pLists.ready[i];
	while (curr != 0) {
	  next = curr->next;
	  if (curr->priority != i)
	    panic("priority != i, scheduler, runnable lists\n");
	  else {
	    stateListRemove(&ptable.pLists.ready[i],
	      &ptable.pLists.readyTail[i], curr);
	    curr->priority -= 1;
	    stateListAdd(&ptable.pLists.ready[i-1],
	      &ptable.pLists.readyTail[i-1], curr);
	  }
	  curr = next;
	}
      }

      // Sleeping:
      curr = ptable.pLists.sleep;
      while (curr != 0) {
	next = curr->next;
	if (curr->priority < 0)
	  panic("priority < 0, scheduler, sleeping\n");
	if (curr->priority > 0)
	  curr->priority -= 1;
	curr = next;
      }

      // Adjust future promotion time
      ptable.PromoteAtTime += TICKS_TO_PROMOTE;
    }

//    if(ptable.pLists.ready != 0) {
    int j;
    int rc = 0;
    for (j = 0; j < MAXPRIO + 1; j++) {
      p = ptable.pLists.ready[j];
      if (p!=0)
	goto scheduler_noskip;
    }
    goto scheduler_skip;

scheduler_noskip:

      rc = stateListRemove(&ptable.pLists.ready[j],
        &ptable.pLists.readyTail[j],p); //Remove from ready list
      assertState(p,RUNNABLE);
      p->state = RUNNING;
      rc += stateListAdd(&ptable.pLists.running,
	&ptable.pLists.runningTail, p);
      if (rc != 0) panic("Panic - scheduler\n");

      proc = p;
      idle = 0;
      switchuvm(p);

      // This has been changed to happen before switch.
      p->cpu_ticks_in = ticks;


      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      proc = 0;



scheduler_skip:
    release(&ptable.lock);

    if (idle) {
      sti();
      hlt();
    }
  }
}
#endif

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;

#ifdef CS333_P2
  proc->cpu_ticks_total += ticks - proc->cpu_ticks_in;
#endif
#ifdef CS333_P3P4
  // update budget and check for demotion
  proc->budget -= ticks - proc->cpu_ticks_in;
  if (proc->budget < 1) {
    // If runnable, deal with now
    if (proc->priority < MAXPRIO) {
      if (proc->state == RUNNABLE) {
        int rc = stateListRemove(&ptable.pLists.ready[proc->priority],
	  &ptable.pLists.readyTail[proc->priority],proc);
        rc += stateListAdd(&ptable.pLists.ready[proc->priority+1],
	  &ptable.pLists.readyTail[proc->priority+1],proc);
        if (rc!=0)
	  panic("sched update priority\n");
      }
      // Because not every proc will be in ready
      proc->priority += 1;
    }
    proc->budget = BUDGET;
  }
#endif
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;

}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
#ifdef CS333_P3P4
  int rc = stateListRemove(&ptable.pLists.running,&ptable.pLists.runningTail,proc);
  assertState(proc,RUNNING);
//    cprintf("running head: %d running tail: %d proc: %d\n",ptable.pLists.running,ptable.pLists.runningTail,proc);
//    cprintf("rc %d name %s state %d priority %d PID %d PPID %d \n",rc,proc->name,proc->state,proc->priority,proc->pid,proc->parent->pid);
  proc->state = RUNNABLE;
  rc += stateListAdd(&ptable.pLists.ready[proc->priority],
    &ptable.pLists.readyTail[proc->priority],proc);
  if (rc != 0) {
//    cprintf("rc %d name %s state %d priority %d PID %d PPID %d \n",rc,proc->name,proc->state,proc->priority,proc->pid,proc->parent->pid);
    panic("Yield\n");
  }
#endif
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
// 2016/12/28: ticklock removed from xv6. sleep() changed to
// accept a NULL lock to accommodate.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){
    acquire(&ptable.lock);
    if (lk) release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
#ifdef CS333_P3P4
  int rc;
  rc = stateListRemove(&ptable.pLists.running,&ptable.pLists.runningTail,proc);
  assertState(proc,RUNNING);
  proc->state = SLEEPING;
  rc += stateListAdd(&ptable.pLists.sleep,&ptable.pLists.sleepTail,proc);
  if (rc!=0) panic("Sleep\n");
#endif
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}

//PAGEBREAK!
#ifndef CS333_P3P4
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}
#else
static void
wakeup1(void *chan)
{
  struct proc *p;
  struct proc *curr = ptable.pLists.sleep;
  int rc;

  while (curr != 0) {
    p = curr;
    curr = curr->next;
    if (p->chan == chan) {
      rc = stateListRemove(&ptable.pLists.sleep,&ptable.pLists.sleepTail,p);
      assertState(p,SLEEPING);
      p->state = RUNNABLE;
      rc += stateListAdd(&ptable.pLists.ready[p->priority],
	&ptable.pLists.readyTail[p->priority],p);
      if (rc != 0) panic("wakeup1\n");
    }
  }
}
#endif

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
#ifndef CS333_P3P4
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#else
int
kill(int pid)
{
  struct proc *curr;
  acquire(&ptable.lock);
  int rc;

  // search all lists (except free and embryo) for
  // any children
  curr = ptable.pLists.running;
  while (curr!=0) {
    if (curr->pid==pid) {
      curr->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    curr = curr->next;
  }
  curr = ptable.pLists.sleep;
  while (curr!=0) {
    if (curr->pid==pid) {
      curr->killed = 1;
      // If sleeping, make ready
      rc = stateListRemove(&ptable.pLists.sleep,&ptable.pLists.sleepTail,curr);
      assertState(curr,SLEEPING);
      curr->state = RUNNABLE;
      // Adding to priority 0 so it will kill quicker
      rc += stateListAdd(&ptable.pLists.ready[0],&ptable.pLists.readyTail[0],curr);
      if (rc!=0) panic("kill\n");
      release(&ptable.lock);
      return 0;
    }
    curr = curr->next;
  }
  for (int i = 0; i <MAXPRIO+1; i++){
    curr = ptable.pLists.ready[i];
    while (curr!=0) {
      if (curr->pid==pid) {
        curr->killed = 1;
        release(&ptable.lock);
        return 0;
      }
      curr = curr->next;
    }
  }
  curr = ptable.pLists.zombie;
  while (curr!=0) {
    if (curr->pid==pid) {
      curr->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    curr = curr->next;
  }
  curr = ptable.pLists.embryo;
  while (curr!=0) {
    if (curr->pid==pid) {
      curr->killed = 1;
      release(&ptable.lock);
      return 0;
    }
    curr = curr->next;
  }

  release(&ptable.lock);
  return -1;
}
#endif

static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
};

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  acquire(&ptable.lock);

// Code taken from Mark's email
#if defined(CS333_P3P4)
#define HEADER "\nPID\tName\tUID\tGID\tPPID\tState\tPriority\tSize\tTime Elapsed\tCPU Time Elapsed\tPCs\n"
#elif defined(CS333_P2)
#define HEADER "\nPID\tName         UID\tGID\tPPID\tState\tSize\tElapsed\tCPU\t PCs\n"
#elif defined(CS333_P1)
#define HEADER "\nPID\tName         Elapsed\tState\tSize\t PCs\n"
#else
#define HEADER ""
#endif

  cprintf(HEADER);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";

#if defined(CS333_P3P4)
    procdumpP3P4(*p, state);
#elif defined(CS333_P2)
    procdumpP2(*p, state);
#elif defined(CS333_P1)
    procdumpP1(*p, state);
#else
    cprintf("%d %s %s", p->pid, state, p->name);
#endif

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }

    cprintf("\n");
  }
  release(&ptable.lock);
}

#ifdef CS333_P3P4
static int
stateListAdd(struct proc** head, struct proc** tail, struct proc* p)
{
  if (*head == 0) {
    *head = p;
    *tail = p;
    p->next = 0;
  } else {
    (*tail)->next = p;
    *tail = (*tail)->next;
    (*tail)->next = 0;
  }

  return 0;
}

static int
stateListRemove(struct proc** head, struct proc** tail, struct proc* p)
{
  if (*head == 0 || *tail == 0 || p == 0) {
    return -1;
  }

  struct proc* current = *head;
  struct proc* previous = 0;

  if (current == p) {
    *head = (*head)->next;
    return 0;
  }

  while(current) {
    if (current == p) {
      break;
    }

    previous = current;
    current = current->next;
  }

  // Process not found, hit eject.
  if (current == 0) {
    return -1;
  }

  // Process found. Set the appropriate next pointer.
  if (current == *tail) {
    *tail = previous;
    (*tail)->next = 0;
  } else {
    previous->next = current->next;
  }

  // Make sure p->next doesn't point into the list.
  p->next = 0;

  return 0;
}

static void
initProcessLists(void) {
  for (int i = 0; i < MAXPRIO+1; i++) {
    ptable.pLists.ready[i] = 0;
    ptable.pLists.readyTail[i] = 0;
  }
  ptable.pLists.free = 0;
  ptable.pLists.freeTail = 0;
  ptable.pLists.sleep = 0;
  ptable.pLists.sleepTail = 0;
  ptable.pLists.zombie = 0;
  ptable.pLists.zombieTail = 0;
  ptable.pLists.running = 0;
  ptable.pLists.runningTail = 0;
  ptable.pLists.embryo = 0;
  ptable.pLists.embryoTail = 0;
}

static void
initFreeList(void) {
  if (!holding(&ptable.lock)) {
    panic("acquire the ptable lock before calling initFreeList\n");
  }

  struct proc* p;

  for (p = ptable.proc; p < ptable.proc + NPROC; ++p) {
    p->state = UNUSED;
    stateListAdd(&ptable.pLists.free, &ptable.pLists.freeTail, p);
  }
}

static int
assertState(struct proc *p, enum procstate state) {
  if (p->state != state)
    panic("Assert State failure.\n");
  return 0;
}

#endif
#ifdef CS333_P2
int
getlock(void) {
  acquire(&(ptable.lock));
  return 1;
}

int
freelock(void) {
  release(&(ptable.lock));
  return 1;
}

// Copy info from proc[pcount] into a uproc and return it
struct uproc
getuproc(int pcount) {
  struct uproc current;
    current.pid = ptable.proc[pcount].pid;
    current.uid = ptable.proc[pcount].uid;
    current.gid = ptable.proc[pcount].gid;
    current.ppid = ptable.proc[pcount].parent->pid;
    current.size = ptable.proc[pcount].sz;
    current.CPU_total_ticks = ptable.proc[pcount].cpu_ticks_total;
    current.elapsed_ticks = ptable.proc[pcount].cpu_ticks_total - 
        ptable.proc[pcount].cpu_ticks_in;  //calculated field

    strncpy(current.state, states[ptable.proc[pcount].state], STRMAX);
    strncpy(current.name, ptable.proc[pcount].name, STRMAX);

  return current;
}
#endif

#ifdef CS333_P3P4
void
readydump(void)
{
  getlock();
  for (int i = 0; i < MAXPRIO + 1; i++)
  {
    struct proc *head = ptable.pLists.ready[i];

    if (head == 0)
      cprintf("No ready processes at priority %d.",i);
    else
    {
      cprintf("Ready processes at priority %d: %d (budget %d)",i,head->pid,head->budget);
      while (head->next != 0)
      {
        cprintf(" -> %d (%d)",head->pid,head->budget);
        head = head->next;
      }
    }
    cprintf("\n");
  }
  freelock();
}

void
freedump(void)
{
  getlock();
  struct proc *head = ptable.pLists.free;
  int counter = 0;

  while (head != 0)
  {
    counter++;
    head = head->next;
  }
  cprintf("Free list size: %d processes.\n",counter);
  freelock();
}

void
sleepdump(void)
{
  getlock();
  struct proc *head = ptable.pLists.sleep;

  if (head == 0)
    cprintf("No sleeping processes.");
  else
  {
    cprintf("Sleeping processes: %d",head->pid);
    while (head->next != 0)
    {
      head = head->next;
      cprintf(" -> %d",head->pid);
    }
  }
  cprintf("\n");
  freelock();
}

void
zombiedump(void)
{
  getlock();
  struct proc *head = ptable.pLists.zombie;

  if (head == 0)
    cprintf("No zombie processes.");
  else
  {
    cprintf("Zombie processes (PID, PPID): (%d,%d)",head->pid, head->parent->pid);
    while (head->next != 0)
    {
      cprintf(" -> (%d,%d)",head->pid,head->parent->pid);
      head = head->next;
    }
  }
  cprintf("\n");
  freelock();
}

int
setpriority(int pid, int priority)
{
  if (priority < 0 || priority > MAXPRIO)
    return -1;

  int rc = 0;

  acquire(&ptable.lock);
  // Running
  struct proc *curr = ptable.pLists.running;
  while (curr != 0)
  {
    if (curr->pid == pid)
    {
      if (curr->priority == priority)
      {
	curr->budget = BUDGET;
	release(&ptable.lock);
	return 0;
      }
      rc = stateListRemove(&ptable.pLists.running,
	&ptable.pLists.runningTail,curr);
      assertState(curr,RUNNING);
      curr->priority = priority;
      curr->budget = BUDGET;
      rc += stateListAdd(&ptable.pLists.ready[priority],
	&ptable.pLists.readyTail[priority],curr);
      if (rc != 0)
	panic("setpriority running\n");
      release(&ptable.lock);
      return 0;
    }
  curr = curr->next;
  }
  // Ready
  for (int i = 0; i < MAXPRIO+1; i++)
  {
    curr = ptable.pLists.ready[i];
    while (curr != 0)
    {
      if (curr->pid == pid)
      {
        if (curr->priority == priority)
        {
	  curr->budget = BUDGET;
	  release(&ptable.lock);
	  return 0;
        }
        rc = stateListRemove(&ptable.pLists.ready[i],
	  &ptable.pLists.readyTail[i],curr);
        assertState(curr,RUNNABLE);
        curr->priority = priority;
	curr->budget = BUDGET;
        rc += stateListAdd(&ptable.pLists.ready[priority],
	  &ptable.pLists.readyTail[priority],curr);
        if (rc != 0)
	  panic("setpriority ready\n");
        release(&ptable.lock);
        return 0;
      }
    curr = curr->next;
    }
  }
  // Sleep
  curr = ptable.pLists.sleep;
  while (curr != 0)
  {
    if (curr->pid == pid)
    {
      if (curr->priority == priority)
      {
	curr->budget = BUDGET;
	release(&ptable.lock);
	return 0;
      }
/*      rc = stateListRemove(&ptable.pLists.sleep,
	&ptable.pLists.sleepTail,curr);
      assertState(curr,SLEEPING);
      curr->priority = priority;
      curr->budget = BUDGET;
      rc += stateListAdd(&ptable.pLists.ready[priority],
	&ptable.pLists.readyTail[priority],curr);
      if (rc != 0)
	panic("setpriority sleep\n");
*/
      curr->priority = priority;
      curr->budget = BUDGET;
      release(&ptable.lock);
      return 0;
    }
  curr = curr->next;
  }


  release(&ptable.lock);
  return -1;
}

#endif
