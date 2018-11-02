#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#ifdef CS333_P2
#include "uproc.h"
#endif

#ifdef CS333_P3P4
struct StateLists {
  struct proc* ready[MAX + 1];
  struct proc* free;
  struct proc* sleep;
  struct proc* zombie;
  struct proc* running;
  struct proc* embryo;
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
#ifdef CS333_P1
static void displaytime(int r);
static void calcelapsedtime(uint time);
static void printprocinfo(const struct proc *p, const char *state);
#endif
#ifdef CS333_P2
static void calccputime(uint time);
#endif
#ifdef CS333_P3P4
static int insertHead(struct proc** sList, struct proc* p);
static int insertTail(struct proc** sList, struct proc* p);
static int removeFromStateList(struct proc** sList, struct proc* p);
static void assertState(struct proc* p, enum procstate state);
static void assertPrio(struct proc* p, int expectedPrio);
static void removeFromReadyList(struct proc** sList, struct proc* p, int expectedPrio);
static void prioadjust(void);
static int budgetupdate(struct proc *p);
#endif

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

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
  #ifndef CS333_P3P4
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  #else
  if(ptable.pLists.free){
      p = ptable.pLists.free;
      goto found;
    }
  #endif
  release(&ptable.lock);
  return 0;

found:
  #ifdef CS333_P3P4
  removeFromStateList(&ptable.pLists.free, p);
  assertState(p, UNUSED);
  #endif
  p->state = EMBRYO;
  #ifdef CS333_P3P4
  insertHead(&ptable.pLists.embryo, p);
  #endif
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    #ifdef CS333_P3P4
    acquire(&ptable.lock);
    removeFromStateList(&ptable.pLists.embryo, p);
    assertState(p, EMBRYO);
    #endif
    p->state = UNUSED;
    #ifdef CS333_P3P4
    insertHead(&ptable.pLists.free, p);
    release(&ptable.lock);
    #endif
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

  // Initalization of start_ticks to tick for P1
  #ifdef CS333_P1
  p->start_ticks = ticks;
  #endif
  // Initalization of cpu_ticks_in and cpu_ticks_total
  #ifdef CS333_P2
  p->cpu_ticks_total = 0;
  p->cpu_ticks_in = 0;
  #endif
  #ifdef CS333_P3P4
  p->prio = 0; // procs always start in the highest prio queue
  p->budget = BUDGET;
  #endif
  return p;
}

// Set up first user process.
void
userinit(void)
{
  #ifdef CS333_P3P4
  int i;
  #endif
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  //initialize lists for projects 3/4
  #ifdef CS333_P3P4
  acquire(&ptable.lock);
  // initialize PromoteAtTime vaue
  ptable.PromoteAtTime = TICKS_TO_PROMOTE;
  // initialize all lists except free to zero
  ptable.pLists.free = 0;
  ptable.pLists.running = 0;
  ptable.pLists.sleep = 0;
  ptable.pLists.embryo = 0;
  ptable.pLists.zombie = 0;
  for(i = 0; i <= MAX; i++){
    ptable.pLists.ready[i] = 0;
  }
  // initialize free to contain all 64 procs
  struct proc *curr;
  for(curr = ptable.proc; curr < &ptable.proc[NPROC]; curr++){
    curr->state = UNUSED;
    insertHead(&ptable.pLists.free, curr);
  }
  release(&ptable.lock);
  #endif
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
  #ifdef CS333_P2
  p->uid = DEFAULTUID;
  p->gid = DEFAULTGID;
  #endif
  
  #ifdef CS333_P3P4
  acquire(&ptable.lock);
  removeFromStateList(&ptable.pLists.embryo, p);
  assertState(p, EMBRYO);
  #endif
  p->state = RUNNABLE;
  #ifdef CS333_P3P4
  ptable.pLists.ready[0] = p;
  ptable.pLists.ready[0]->next = 0;
  release(&ptable.lock);
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

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    #ifdef CS333_P3P4
    acquire(&ptable.lock);
    removeFromStateList(&ptable.pLists.embryo, np);
    assertState(np, EMBRYO);
    #endif
    np->state = UNUSED;
    #ifdef CS333_P3P4
    insertHead(&ptable.pLists.free, np);
    release(&ptable.lock);
    #endif
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;
  #ifdef CS333_P2
  np->uid = np->parent->uid;
  np->gid = np->parent->gid;
  #endif

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));
 
  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  #ifdef CS333_P3P4
  removeFromStateList(&ptable.pLists.embryo, np);
  assertState(np, EMBRYO);
  #endif
  np->state = RUNNABLE;
  #ifdef CS333_P3P4
  insertTail(&ptable.pLists.ready[0], np);
  #endif
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
 int fd, i;

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
  // I would love a better way to search all lists...
  for(i = 0; i <= MAX; i++){
    for(p = ptable.pLists.ready[i]; p; p = p->next){
      if(p->parent == proc){
	p->parent = initproc;
      }
    }
  }
  for(p = ptable.pLists.running; p; p = p->next){
    if(p->parent == proc){
      p->parent = initproc;
    }
  }
  for(p = ptable.pLists.sleep; p; p = p->next){
    if(p->parent == proc){
      p->parent = initproc;
    }
  }
  for(p = ptable.pLists.embryo; p; p = p->next){
    if(p->parent == proc){
      p->parent = initproc;
    }
  }
  for(p = ptable.pLists.zombie; p; p = p->next){
    if(p->parent == proc){
      p->parent = initproc;
      wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  removeFromStateList(&ptable.pLists.running, proc);
  assertState(proc, RUNNING);
  proc->state = ZOMBIE;
  insertHead(&ptable.pLists.zombie, proc);
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
  struct proc *p;
  int havekids, found, pid, i; // thanks taniya for the flag idea

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    found = 0;
    for(p = ptable.pLists.zombie; p; p = p->next){
      if(p->parent != proc)
        continue;
      havekids = 1;
      // Found one.
      pid = p->pid;
      kfree(p->kstack);
      p->kstack = 0;
      freevm(p->pgdir);
      removeFromStateList(&ptable.pLists.zombie, p);
      assertState(p, ZOMBIE);
      p->state = UNUSED;
      insertHead(&ptable.pLists.free, p);
      p->pid = 0;
      p->parent = 0;
      p->name[0] = 0;
      p->killed = 0;
      release(&ptable.lock);
      return pid;
    }
    for(i = 0; i <= MAX; i++){
      for(p = ptable.pLists.ready[i]; p && !found; p = p->next){
	if(p->parent != proc)
	  continue;
	havekids = 1;
	found = 1;
      }
    }
    for(p = ptable.pLists.running; p && !found; p = p->next){
      if(p->parent != proc)
      	continue;
      havekids = 1;
      found = 1;
    }
    
    for(p = ptable.pLists.sleep; p && !found; p = p->next){
      if(p->parent != proc)
	continue;
      havekids = 1;
      found = 1;
    }
    
    for(p = ptable.pLists.embryo; p && !found; p = p->next){
      if(p->parent != proc)
	continue;
      havekids = 1;
      found = 1;
    }
    
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}
#endif

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
 int idle, i;  // for checking if processor is idle

  for(;;){
    // Enable interrupts on this processor.
    sti();

    idle = 1;  // assume idle unless we schedule a process
    
    acquire(&ptable.lock);
    if(ticks >= ptable.PromoteAtTime)
      prioadjust();
    for(i = 0; i <= MAX; i++){
      if(ptable.pLists.ready[i]){
	p = ptable.pLists.ready[i];
	// Switch to chosen process.  It is the process's job
	// to release ptable.lock and then reacquire it
	// before jumping back to us.
	idle = 0;  // not idle this timeslice
	proc = p;
	switchuvm(p);
	/* removeFromStateList(&ptable.pLists.ready[i], p); */
	/* assertState(p, RUNNABLE); */
	removeFromReadyList(&ptable.pLists.ready[i], p, i);
	/* assertPrio(p, i); */
	p->state = RUNNING;
	insertHead(&ptable.pLists.running, p);
        #ifdef CS333_P2
	/* p->cpu_ticks_in++; <- correct??? */
	p->cpu_ticks_in = ticks;
        #endif
	swtch(&cpu->scheduler, proc->context);
	switchkvm();

	// Process is done running for now.
	// It should have changed its p->state before coming back.
	proc = 0;
	break;
      }
    }
    release(&ptable.lock);
    // if idle, wait for next interrupt
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
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  #ifdef CS333_P3P4
  removeFromStateList(&ptable.pLists.running, proc);
  assertState(proc, RUNNING);
  #endif
  proc->state = RUNNABLE;
  #ifdef CS333_P3P4
  //update budget before entering sched
  budgetupdate(proc);
  insertTail(&ptable.pLists.ready[proc->prio], proc);
  #endif
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
  removeFromStateList(&ptable.pLists.running, proc);
  assertState(proc, RUNNING);
  #endif
  proc->state = SLEEPING;
  #ifdef CS333_P3P4
  budgetupdate(proc);
  insertHead(&ptable.pLists.sleep, proc);
  #endif
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){ 
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}

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
  struct proc *p, *curr;

  for(p = ptable.pLists.sleep; p; /* curr = p->next, p = curr */){
    curr = p->next;
    if(p->chan == chan){
      removeFromStateList(&ptable.pLists.sleep, p);
      assertState(p, SLEEPING);
      p->state = RUNNABLE;
      insertTail(&ptable.pLists.ready[p->prio], p);
    }
    p = curr;
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
  int i;
  struct proc *p;

  acquire(&ptable.lock);
  for(i = 0; i <= MAX; i++){
    for(p = ptable.pLists.ready[i]; p; p = p->next){
      if(p->pid == pid){
	p->killed = 1;
	release(&ptable.lock);
	return 0;
      }
    }
  }
  for(p = ptable.pLists.running; p; p = p->next){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
  }
  for(p = ptable.pLists.embryo; p; p = p->next){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
  }
  for(p = ptable.pLists.zombie; p; p = p->next){
    if(p->pid == pid){
      p->killed = 1;
      release(&ptable.lock);
      return 0;
    }
  }
  for(p = ptable.pLists.sleep; p; p = p->next){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      removeFromStateList(&ptable.pLists.sleep, p);
      assertState(p, SLEEPING);
      p->state = RUNNABLE;
      insertTail(&ptable.pLists.ready[p->prio], p);
      release(&ptable.lock);
      return 0;
    }
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

  #if defined(CS333_P1) && !defined(CS333_P2)
  cprintf("\nPID\tState\tName\tElapsed\t PCs\n"); // header as of P1
  #endif
  #if defined(CS333_P2) && !defined(CS333_P3P4)
  //header for P2
  cprintf("\nPID\tName\t\tUID\tGID\tPPID\tElapsed\tCPU\tState\tSize\t PCs\n");
  #endif
  #ifdef CS333_P3P4
  cprintf("\nPID\tName\t\tUID\tGID\tPPID\tPrio\tElapsed\tCPU\tState\tSize\t PCs\n");
  #endif
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    #ifdef CS333_P1
    printprocinfo(p, state);
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
}

#ifdef CS333_P1
static void
printprocinfo(const struct proc *p, const char *state)
{
    #if defined(CS333_P1) && !defined(CS333_P2)
    cprintf("%d\t%s\t%s\t", p->pid, state, p->name);
    calcelapsedtime(p->start_ticks);
    #endif
    #ifdef CS333_P2
    cprintf("%d\t%s\t", p->pid, p->name);
    if(7 > strlen(p->name))
      cprintf("\t");
    cprintf("%d\t%d\t", p->uid, p->gid);
    if(1 == p->pid)
      cprintf("%d\t", p->pid);
    else
      cprintf("%d\t", p->parent->pid); //lock?
    #ifdef CS333_P3P4
    cprintf("%d\t", p->prio);
    #endif
    calcelapsedtime(p->start_ticks);
    calccputime (p->cpu_ticks_total);
    cprintf("%s\t%d\t", state, p->sz);
    #endif
}

// Calculate the elepsed time of a process
// print the result in the for x.xxx tabbed for
// alignement. To be used for the procdump() in P1
static void
calcelapsedtime(uint time)
{
  uint e,q,r;
  e = ticks - time;
  q = e/1000;
  r = e%1000;
  cprintf("%d", q);
  displaytime(r);
}

// Prints the decimals from ticks being converted to seconds
static void
displaytime(int r)
{
  // updated c print to fix issue with 0 dropping for output values
  // i.e 2030 was printed 2.30 but should now be 2.030
  if(100 > r && 9 < r) 
    cprintf(".0%d\t", r);
  else if(10 > r)
    cprintf(".00%d\t", r);
  else
    cprintf(".%d\t",r);
}
#endif 
#ifdef CS333_P2
int
setuid(uint uid)
{
  if (0 > uid || 32767 < uid) //change to const?
    return -1;
  acquire(&ptable.lock);
  proc->uid = uid;
  release(&ptable.lock);
  return 0;
}

int
setgid(uint gid)
{
  if (0 > gid || 32767 < gid)
    return -1;
  acquire(&ptable.lock);
  proc->gid = gid;
  release(&ptable.lock);
  return 0;
}

int
getprocs(uint max, struct uproc* table)
{
  struct proc *p;
  uint entities /* = 0 */;

  acquire(&ptable.lock);
  for(p = ptable.proc, entities = 0; p < &ptable.proc[NPROC] && entities < max; p++){
    if (p->state == RUNNABLE || p->state == RUNNING || p->state == SLEEPING)
      {

        table[entities].pid = p->pid;
	table[entities].uid = p->uid;
	table[entities].gid = p->gid;
	// make the following a func...
	if (1 == p->pid){
	  table[entities].ppid = p->pid;
	}
	else{
	  table[entities].ppid = p->parent->pid;
	}
	table[entities].elapsed_ticks = ticks - (p->start_ticks);
	table[entities].CPU_total_ticks = p->cpu_ticks_total;
        safestrcpy(table[entities].state, states[p->state], STRMAX);
	table[entities].size = p->sz;
        safestrcpy(table[entities].name, p->name, STRMAX);
	#ifdef CS333_P3P4
	table[entities].prio = p->prio;
	#endif
	entities++;
      }
  }
  release(&ptable.lock);
  if(0==entities)
    return -1;
  return entities;;
}

static void
calccputime (uint time)
{
  uint q, r;
  q = time / 1000;
  r = time % 1000;
  cprintf("%d", q);
  displaytime(r);
}
#endif
#ifdef CS333_P3P4
static int
insertHead(struct proc** sList, struct proc* p)
{
  if(!p)
    return -1;
  p->next = *sList;
  *sList = p;
  return 0;
}

static int
insertTail(struct proc** sList, struct proc* p)
{
  struct proc *curr = *sList;			     
  if(!p) // if no proc pointer
    return -1;
  if(!*sList){
    *sList = p;
    (*sList)->next = 0;
    return 0;
  }
  
  while(curr->next)
    curr = curr->next;
  curr->next = p;
  curr->next->next = 0;
  return 0;
}

static int
removeFromStateList(struct proc** sList, struct proc* p)
{
  struct proc *curr, *new;
  if (!*sList || !p)
    return -1;
  if(*sList == p){
	//list beginning
    new = *sList;
    *sList = (*sList)->next;
    new->next = 0;
    return 0;
  }
  for(curr = *sList; curr && curr != p; new = curr, curr = curr->next);
  if(!curr)
    return -1;
   // middle/end
   new->next = curr->next;
   curr->next = 0;
   return 0;
}

static void
assertState(struct proc* p, enum procstate state)
{
  if(p->state != state){
    cprintf("Error: Expected %s, but state was %s\n", states[state], states[p->state]);
    panic("Incorrect Proc State!\n");
  }
}

static void
assertPrio(struct proc* p, int expectedPrio)
{
  if(p->prio != expectedPrio){
    cprintf("Error: Expected Prio %d, but priority was %d\n", expectedPrio, p->prio);
    panic("Incorrect Proc Priority!\n");
  }
}

static void
removeFromReadyList(struct proc** sList, struct proc* p, int expectedPrio)
{
  // lock should already be held by other functions
  removeFromStateList(&ptable.pLists.ready[expectedPrio], p);
  assertPrio(p, expectedPrio);
  assertState(p, RUNNABLE);
}
  
void
printReadyList(void)
{
  int i;
  cprintf("Ready List Processes:\n");
  acquire(&ptable.lock);
  for(i = 0; i <= MAX; i++){
    cprintf("Prio %d: ", i);
    if(!ptable.pLists.ready[i])
      cprintf("EMPTY\n", i);
    else{
      struct proc *curr;
      for(curr = ptable.pLists.ready[i]; curr; curr = curr->next){
	cprintf("(%d, %d)", curr->pid, curr->budget);
	if(curr->next)
	  cprintf(" -> "); 
      }
      cprintf("\n");
    }
  }
  release(&ptable.lock);
}

void
printSleepList(void)
{
  cprintf("Sleep List Processes:\n");
  acquire(&ptable.lock);
  if(!ptable.pLists.sleep)
    cprintf("No Sleeping Processes\n");
  else{
    struct proc *curr;
    for(curr = ptable.pLists.sleep; curr; curr = curr->next){
      cprintf("%d", curr->pid);
      if(curr->next)
	  cprintf(" -> ");
    }
    cprintf("\n");
  }
  release(&ptable.lock);
}


void
printZombieList(void)
{
  cprintf("Zombie List Processes:\n");
  acquire(&ptable.lock);
  if(!ptable.pLists.zombie)
    cprintf("No Zombie Processes\n");
  else{
    struct proc *curr;
    for(curr = ptable.pLists.zombie; curr; curr = curr->next){
      cprintf("(PID%d,PPID%d)", curr->pid, curr->parent->pid);
      if(curr->next)
	  cprintf(" -> ");
    }
    cprintf("\n");
  }
  release(&ptable.lock);
}

void
freeListSize(void)
{
  int size = 0;
  struct proc *curr;
  acquire(&ptable.lock);
  for(curr = ptable.pLists.free; curr; curr = curr->next)
    size++;
  release(&ptable.lock);
  cprintf("Free List Size: %d processes\n", size);
}

int
setpriority(int pid, int priority)
{
  int i;
  struct proc *p;
  if(0 > priority || MAX < priority)
    return -1;
  acquire(&ptable.lock);
  for(i = 0; i <= MAX; i++){
    for(p = ptable.pLists.ready[i]; p; p = p->next)
      if(p->pid == pid){
	if(priority != i){
	  removeFromReadyList(&ptable.pLists.ready[i], p, i);
	  /* removeFromStateList(&ptable.pLists.ready[i], p); */
	  /* assertPrio(p, i); */
	  /* assertState(p, RUNNABLE); */
	  p->prio = priority;
	  p->budget = BUDGET;
	  insertTail(&ptable.pLists.ready[priority], p);
	}
	release(&ptable.lock);
	return 0;
      }
  }
  for(p = ptable.pLists.running; p; p = p->next)
    if(p->pid == pid){
      p->prio = priority;
      p->budget = BUDGET;
      release(&ptable.lock);
      return 0;
    }
  for(p = ptable.pLists.sleep; p; p = p->next)
    if(p->pid == pid){
      p->prio = priority;
      p->budget = BUDGET;
      release(&ptable.lock);
      return 0;
    }
  release(&ptable.lock);
  return -1;
}

static void
prioadjust(void)
{
  int i;
  struct proc *p;
  for(p = ptable.pLists.sleep; p; p = p->next)
    if(p->prio)
      p->prio--;
  for(p = ptable.pLists.running; p; p = p->next)
    if(p->prio)
      p->prio--;
  for(i = 1; i < MAX + 1; i++)
    for(p = ptable.pLists.ready[i]; p; p = p->next){
      removeFromReadyList(&ptable.pLists.ready[i], p, i);
      /* removeFromStateList(&ptable.pLists.ready[i], p); */
      /* assertPrio(p, i); */
      /* assertState(p, RUNNABLE); */
      p->prio -= 1;
      insertTail(&ptable.pLists.ready[p->prio], p);
    }
  ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
}

static int
budgetupdate(struct proc *p)
{
  p->budget = p->budget - (ticks - p->cpu_ticks_in);
  if(0 >= p->budget){
    if(MAX > p->prio && MAX != 0) // no need to change the bottom prio
      p->prio++;
    p->budget = BUDGET;
    return 0;
  }
  return 0;
}
#endif
