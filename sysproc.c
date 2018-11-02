#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "uproc.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

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

//Turn off the computer
int
sys_halt(void)
{
  cprintf("Shutting down ...\n");
  outw( 0x604, 0x0 | 0x2000);
  return 0;
}

#ifdef CS333_P1
// P1: date () to return the current UTC time
int
sys_date(void)
{
  struct rtcdate *d;
  if(argptr(0, (void*)&d, sizeof(struct rtcdate)) < 0)
    return -1;
  cmostime (d);
  return 0;
}
#endif
#ifdef CS333_P2
int
sys_getuid(void)
{
  return proc->uid;
}

int
sys_getgid(void)
{
  return proc->gid;
}

int
sys_getppid(void)
{
  // add test for "no" parent i.e. init process
  if(1 == proc->pid){
    return proc->pid;
  }
  return proc->parent->pid;
}

int
sys_setuid(void)
{
  int u;

  if(argint(0, &u) < 0)
    return -1;
  return setuid((uint)u);
}

int
sys_setgid(void)
{
  int g;

  if(argint(0, &g) < 0)
    return -1;
  return setgid((uint)g);
}

int
sys_getprocs(void)
{
  struct uproc *u;
  int max;
  if(argint(0, &max) < 0)
    return -1;
  if(argptr(1, (void *)&u, sizeof(struct uproc)*max) < 0)
    return -1;
  return getprocs((uint)max, u);
}
#endif
#ifdef CS333_P3P4
int
sys_setpriority(void)
{
  int prio, pid;
  if(argint(1, &prio) < 0)
    return -1;
  if(argint(0, &pid) < 0)
    return -1;
  return setpriority(pid, prio);
}
#endif
