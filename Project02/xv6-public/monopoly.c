#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"


extern int is_monopolized;
extern uint ticks;
extern struct proc_queue L0;
extern struct proc_queue L1;
extern struct proc_queue L2;
extern struct proc_queue L3;
extern struct proc_queue MQ;

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

const int mypassword = 2020098240;

// set process monopolize
int
setmonopoly(int pid, int password)
{
  struct proc* p;
  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      // -2: password fail
      if (password != mypassword)
      {
        release(&ptable.lock);
        return -2;
      }
      // -3: already in MoQ
      if (p->queue_level == 99)
      {
        release(&ptable.lock);
        return -3;
      }
      // -4: p is myproc
      if (myproc() == p)  // self move
      {
        release(&ptable.lock);
        return -4;
      }

      // all clear!
      // remove from current queue
      if (p->queue_level == 0)  q_remove(&L0, p);
      if (p->queue_level == 1)  q_remove(&L1, p);
      if (p->queue_level == 2)  q_remove(&L2, p);
      if (p->queue_level == 3)  q_remove(&L3, p);
      q_push(&MQ, p);  // new queue_level(99) also determined

      release(&ptable.lock);
      return MQ.size;
    }
  }
  // -1: pid not exist
  release(&ptable.lock);
  return -1;
}

// activate MoQ
void
monopolize(void)
{
  cprintf("[SYS] moq activated!\n");
  is_monopolized = 1;
  return ;
}

// deactivate MoQ
void
unmonopolize(void)
{
  cprintf("[SYS] moq deactivated!\n");
  is_monopolized = 0;
  acquire(&tickslock);
  ticks = 0;
  release(&tickslock);
  return ;
}

// wrapper functions

int
sys_setmonopoly(void)
{
  cprintf("[SYS] setmonopoly\n");
  int pid, password;
  if (argint(0, &pid) < 0 || argint(1, &password) < 0) return -5;  // argint fails (-5)
  return setmonopoly(pid, password);  // return wrapped function's return value ({size of MoQ} ~ -4)
}

int
sys_monopolize(void)
{
  monopolize();
  return 0;
}

int
sys_unmonopolize(void)
{
  unmonopolize();
  return 0;
}

