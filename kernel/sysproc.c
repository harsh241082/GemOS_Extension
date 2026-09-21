#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "vmstats.h"
#include "disk_sched.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_pause(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kkill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
// A1: Print hello from the kernel
uint64
sys_hello(void)
{
  printf("Hello from the kernel!\n");
  return 0;
}

// A2: Get process ID (independent implementation)
uint64
sys_getpid2(void)
{
  return myproc()->pid;
}

// B1: Get parent process ID
uint64
sys_getppid(void)
{
  struct proc *p = myproc();
  struct proc *parent;
  int ppid;

  // Need to acquire p->lock to safely access parent pointer
  acquire(&p->lock);
  parent = p->parent;
  acquire(&parent->lock);
  ppid = parent->pid;
  release(&parent->lock);
  release(&p->lock);

  return ppid;
}

// B2: Get number of child processes (not zombies)
uint64
sys_getnumchild(void)
{
  struct proc *p = myproc();
  struct proc *child;
  int count = 0;

  // Traverse process table to find children
  extern struct proc proc[NPROC];
  
  for(child = proc; child < &proc[NPROC]; child++) {
    acquire(&child->lock);
    if(child->parent == p && child->state != ZOMBIE) {
      count++;
    }
    release(&child->lock);
  }

  return count;
}

// C2: Get system call count of current process
uint64
sys_getsyscount(void)
{
  return myproc()->syscall_count;
}

// C3: Get system call count of a child process by PID
uint64
sys_getchildsyscount(void)
{
  int child_pid;
  struct proc *p = myproc();
  struct proc *child;

  argint(0, &child_pid);

  // Traverse process table to find the child with given PID
  extern struct proc proc[NPROC];
  
  for(child = proc; child < &proc[NPROC]; child++) {
    acquire(&child->lock);
    if(child->pid == child_pid && child->parent == p) {
      int count = child->syscall_count;
      release(&child->lock);
      return count;
    }
    release(&child->lock);
  }

  return -1;  // PID is not a child of the current process
}

// PA2: Get current MLFQ level of calling process
uint64
sys_getlevel(void)
{
  struct proc *p = myproc();
  int level;

  acquire(&p->lock);
  level = p->level;
  release(&p->lock);

  return level;
}

// PA2: Get detailed MLFQ information for a process
uint64
sys_getmlfqinfo(void)
{
  int pid;
  uint64 uaddr;
  struct mlfqinfo info;
  struct proc *p;

  argint(0, &pid);
  argaddr(1, &uaddr);

  // Find the process with the given pid.
  extern struct proc proc[NPROC];
  int found = 0;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->pid == pid && p->state != UNUSED) {
      info.level = p->level;
      for(int i = 0; i < MLFQ_LEVELS; i++) {
        info.ticks[i] = p->ticks[i];
      }
      info.times_scheduled = p->times_scheduled;
      info.total_syscalls = p->syscall_count;
      found = 1;
      release(&p->lock);
      break;
    }
    release(&p->lock);
  }

  if(!found)
    return -1;

  if(copyout(myproc()->pagetable, uaddr, (char *)&info, sizeof(info)) < 0)
    return -1;

  return 0;
}

uint64
sys_setdisksched(void)
{
  int policy;
  
  argint(0, &policy);
  
  return setdisksched(policy);
}

uint64
sys_getvmstats(void)
{
  int pid;
  uint64 uaddr;
  struct vmstats info;
  struct proc *p;

  argint(0, &pid);
  argaddr(1, &uaddr);

  // Find the process with the given pid.
  extern struct proc proc[NPROC];
  int found = 0;

  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->pid == pid && p->state != UNUSED) {
      info.page_faults = p->page_faults;
      info.pages_evicted = p->pages_evicted;
      info.pages_swapped_in = p->pages_swapped_in;
      info.pages_swapped_out = p->pages_swapped_out;
      info.resident_pages = p->resident_pages;
      found = 1;
      release(&p->lock);
      break;
    }
    release(&p->lock);
  }

  if(!found)
    return -1;

  if(copyout(myproc()->pagetable, uaddr, (char *)&info, sizeof(info)) < 0)
    return -1;

  return 0;
}