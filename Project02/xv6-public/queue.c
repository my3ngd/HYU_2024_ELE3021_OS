#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

extern struct proc_queue MQ;
extern struct proc_queue L0;
extern struct proc_queue L1;
extern struct proc_queue L2;
extern struct proc_queue L3;

const int SZ = NPROC+1;

int
nxt(int i)
{
  return (i+1)%SZ;
}

// 큐 초기화
void
init_queue(struct proc_queue* q, int queue_level)
{
  q->level = queue_level;
  q->time_quantum = queue_level * 2 + 2;
  q->front = q->back = 0;
  for (int i = 0; i <= NPROC; i++)
    q->queue[i] = nullptr;
}

// 큐가 비어 있으면 1, 그렇지 않으면 0 반환
int
q_empty(struct proc_queue *q)
{
  return q->front == q->back;
}

// runnable 상태인 프로세스가 존재하면 1, 그렇지 않으면 0 반환
int
q_runnable(struct proc_queue *q)
{
  if (q_empty(q)) return 0;
  for (int i = q->front; i != q->back; i = nxt(i))
    if (q->queue[i]->state == RUNNABLE)
      return 1;
  return 0;
}

// 큐의 크기 반환
int
q_size(struct proc_queue *q)
{
  if (q->back < q->front)
    return q->back - q->front + SZ;
  return q->back - q->front;
}

// 큐에 프로세스가 있는 지 확인하여 있으면 1, 없으면 0 반환
int
q_exist(struct proc_queue *q, struct proc *p)
{
  for (int i = q->front; i != q->back; i = nxt(i))
    if (q->queue[i] == p)
      return 1;
  return 0;
}

// 큐의 마지막에 프로세스 추가
void
q_push(struct proc_queue *q, struct proc *p)
{
  if (q_exist(q, p))
    q_remove(q, p);
  p->queue_level = q->level;
  q->queue[q->back] = p;
  q->back = nxt(q->back);
  return ;
}

// 큐의 가장 앞 프로세스를 제거
void
q_pop(struct proc_queue *q)
{
  q->queue[q->front] = nullptr;
  q->front = nxt(q->front);
}

// 큐에서 특정 프로세스를 제거; MoQ 혹은 우선순위 큐 구현에 필요
void
q_remove(struct proc_queue *q, struct proc *p)
{
  int index = -1;
  for (int i = q->front; i != q->back; i = nxt(i))
  {
    if (q->queue[i] == p)
    {
      index = i;
      break;
    }
  }
  if (index == -1)
    return ;  // not found
  for (int i = index; i != q->back; i = nxt(i))
  {
    q->queue[i] = q->queue[nxt(i)];
    q->queue[nxt(i)] = nullptr;
  }
  q->back = (q->back+SZ-1)%SZ;
  return ;
}

// 큐의 가장 앞 원소 반환
struct proc*
q_front(struct proc_queue *q)
{
  if (q_empty(q))  // may not happen
    return nullptr;
  return q->queue[q->front];
}

// 큐에서 priority가 가장 높은 프로세스 중 첫 번째를 반환
struct proc*
q_top(struct proc_queue *q)
{
  if (q_empty(q))  // may not happen
    return nullptr;
  struct proc *res = q->queue[q->front];
  for (int i = q->front; i != q->back; i = nxt(i))
    if (res->priority < q->queue[i]->priority)
      res = q->queue[i];
  return res;
}

// system calls

// 프로세스가 속한 큐의 레벨 반환
int
getlev(void)
{
  return myproc()->queue_level;
}

// 프로세스의 priority 설정
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

// wrapper functions
int
sys_getlev(void)
{
  return getlev();
}

int
sys_setpriority(void)
{
  int arg_pid, arg_priority;
  if (argint(0, &arg_pid) < 0 || argint(1, &arg_priority) < 0) return 1;  // argint fails (1)
  return setpriority(arg_pid, arg_priority);  // return wrapped function's return value (0 ~ -2)
}