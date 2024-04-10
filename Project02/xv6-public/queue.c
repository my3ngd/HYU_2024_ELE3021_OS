#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#define nullptr (void*)0

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

void
init_queue(struct proc_queue* q, int queue_level)
{
  q->queue_level = queue_level;
  q->time_quantum = (queue_level + 1) * 2;
  q->size = 0;
  q->front = q->back = nullptr;
  return ;
}

int
q_empty(struct proc_queue* q)
{
  return !q->size;
}

struct proc*
q_front(struct proc_queue* q)
{
  return q->front;
}

int
q_exist(struct proc_queue* q, struct proc* p)
{
  if (q_empty(q)) return 0;
  struct proc* cur = q->front;
  do  // loop need to run more than 1
  {
    if (cur == p) return 1;
    cur = cur->next;
  } while (cur != q->front);
  return 0;
}

void
q_clear(struct proc_queue* q)
{
  q->front = q->back = nullptr;
  q->size = 0;
}

void
q_push(struct proc_queue* q, struct proc* p)
{
  p->queue_level = q->queue_level;
  p->ticks = 0;
  q->size++;
  if (q_empty(q))
  {
    q->front = q->back = p;
    p->next = p;
    return ;
  }
  q->back->next = p;
  q->back = p;
  p->next = q->front;
  return ;
}

void
q_pop(struct proc_queue* q)
{
  if (q_empty(q)) return;  // may not happen
  struct proc* cur = q->front->next;
  q->front = cur;
  q->size--;
}

// rotate queue
void
q_setfront(struct proc_queue* q, struct proc* p)
{
  struct proc* pre = q->back;
  struct proc* cur = pre->next;  // is q->front
  while (cur != p)
  {
    pre = cur;
    cur = cur->next;
  }
  q->front = cur;
  q->back = pre;
  return ;
}

void
q_remove(struct proc_queue* q, struct proc* p)
{
  if (q_empty(q)) return ;  // not able to remove; q_empty queue
  if (q->size == 1)
  {
    if (q->front != p) return ;
    q->front = q->back = nullptr;
    q->size = 0;
    return ;
  }

  struct proc* pre = q->back;
  struct proc* cur = pre->next;

  // find p
  while (cur != p)
  {
    pre = cur;
    cur = cur->next;
    if (cur->next == q->front && cur != p)  // p is not in q
      return ;
  }

  // unlink
  if (cur == q->front)
  {
    q->front = cur->next;
    q->back->next = q->front;
  }
  else if (cur == q->back)
  {
    q->back = pre;
    q->back->next = q->front;
  }
  else
  {
    pre->next = cur->next;
  }
  q->size--;
  return ;
}

// for priority queue (L3)
struct proc*
q_top(struct proc_queue* pq)
{
  if (q_empty(pq)) return nullptr;
  struct proc* cur = pq->front;
  struct proc* res = cur;
  int max_priority = pq->front->priority;

  do
  {
    cur = cur->next;
    if (cur->priority > max_priority)
    {
      max_priority = cur->priority;
      res = cur;
    }
  }
  while (cur != pq->front);
  return res;
}

void
print_queue(struct proc_queue *q)
{
  cprintf("===========================================\n");
  cprintf("  size = %d\n", q->size);
  struct proc *p = nullptr;
  do
  {
    cprintf("> pid = %d\n", p->pid);
    p = p->next;
  } while (p != q->front);
  
  cprintf("===========================================\n");
}

// APIs

int
getlev(void)
{
  struct proc* curr_proc = myproc();
  return curr_proc->queue_level;
}

int
setpriority(int pid, int priority)
{
  if (priority < 0 || 10 < priority)
    return -2;
  struct proc* p = nullptr;
  acquire(&ptable.lock);  // because this function approaches to ptable
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->pid == pid)
    {
      p->priority = priority;
      release(&ptable.lock);
      return 0;
    }
  }
  // process at pid not found
  release(&ptable.lock);
  return -1;
}

// system call

int
sys_getlev(void)
{
  return getlev();
}

int
sys_setpriority(int pid, int priority)
{
  return setpriority(pid, priority);
}
