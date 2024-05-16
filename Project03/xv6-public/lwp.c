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

extern struct proc* get_initproc();
extern void forkret(void);
extern void trapret(void);
extern void init_proc(struct proc*);

extern int nexttid;

// copy of wakeup1 in proc.c
static void
wakeup2(void *chan)
{
  for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
  return ;
}


struct proc*
alloc_thread(void)
{
  struct proc* curproc = myproc();
  struct proc* p = nullptr;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == UNUSED)
      goto found;
  return nullptr;

found:
  p->tid = nexttid++;
  p->pid = myproc()->pid;
  p->state = EMBRYO;
  p->parent = curproc;
  p->origin = (curproc->tid ? curproc->origin : curproc);
  p->pgdir = curproc->pgdir;

  if ((p->kstack = kalloc()) == 0)
  {
    init_proc(p);
    return nullptr;
  }

  char* sp = p->kstack + KSTACKSIZE - (sizeof *p->tf);
  p->tf = (struct trapframe*)sp;
  *p->tf = *curproc->tf;

  sp -= sizeof(uint);
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;
  return p;
}

int
alloc_ustack(struct proc* lwp)
{
  struct proc *ori = lwp->origin;
  ori->sz = PGROUNDUP(ori->sz);
  if ((ori->sz = allocuvm(ori->pgdir, ori->sz, ori->sz + 2*PGSIZE)))
  {
    clearpteu(ori->pgdir, (char*)(ori->sz - 2*PGSIZE));
    lwp->sz = ori->sz;
    return 0;
  }
  init_proc(lwp);
  return -1;
}

int
thread_create(thread_t* thread, void* (*start_routine)(void *), void *arg)
{
  struct proc* curproc = myproc();
  struct proc* lwp;

  acquire(&ptable.lock);
  if ((lwp = alloc_thread()) == 0)
    goto bad;
  
  struct proc *ori = lwp->origin;
  ori->sz = PGROUNDUP(ori->sz);
  if ((ori->sz = allocuvm(ori->pgdir, ori->sz, ori->sz + 2*PGSIZE)) == 0)
    goto bad;
  clearpteu(ori->pgdir, (char*)(ori->sz - 2*PGSIZE));
  lwp->sz = ori->sz;

  uint sp = lwp->sz - 2*sizeof(uint);
  uint st_val[2] = {0xFFFFFFFFU, (uint)arg};
  if (copyout(lwp->pgdir, sp, st_val, 2 * sizeof(uint)) < 0)
    goto bad;
  release(&ptable.lock);

  for (int fd = 0; fd < NOFILE; fd++)
    if (curproc->ofile[fd])
      lwp->ofile[fd] = filedup(curproc->ofile[fd]);
  lwp->cwd = idup(curproc->cwd);

  acquire(&ptable.lock);
  lwp->tf->eip = (uint)start_routine;
  lwp->tf->esp = sp;

  for (struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->pid == lwp->pid) 
      p->sz = lwp->sz;
  *thread = lwp->tid;
  lwp->state = RUNNABLE;
  release(&ptable.lock);
  return 0;

bad:
  init_proc(lwp);
  release(&ptable.lock);
  return -1;
}


void
thread_exit(void* retval)
{
  struct proc* lwp = myproc();
  if (lwp->tid == 0)
    panic("thread_exit - not lwp\n");
  // if initproc, it may not lwp

  for (int fd = 0; fd < NOFILE; fd++)
    if (lwp->ofile[fd])
      lwp->ofile[fd] = 0;

  begin_op();
  iput(lwp->cwd);
  end_op();
  lwp->cwd = 0;
  lwp->retval = retval;
  lwp->state = ZOMBIE;
  
  acquire(&ptable.lock);
  wakeup2(lwp->parent);
  for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if (p->parent == lwp)
    {
      p->parent = get_initproc();
      if (p->state == ZOMBIE)
        wakeup2(get_initproc());
    }
  }
  sched();
  panic("zombie exit");
}


int 
thread_join(thread_t thread, void** retval)
{  
  acquire(&ptable.lock);
  for (struct proc *lwp = myproc();;)
  {
    for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->tid != thread)
        continue;
      if (p->state == ZOMBIE)
      {
        *retval = p->retval;
        p->retval = nullptr;
        if (p->kstack)  // true
          kfree(p->kstack);
        p->kstack = nullptr;
        p->pid = p->tid = 0;
        p->parent = p->origin = nullptr;
        p->name[0] = 0;
        p->killed = false;
        p->state = UNUSED;
        release(&ptable.lock);
        return 0;
      }
    }
    sleep(lwp, &ptable.lock);
  }
  return -1;
}


// System call wrapper functions

int
sys_thread_create(void)
{
  thread_t* thread;
  void* (*start_rotine)(void *);
  void* arg;
  if (argptr(0, (void*)&thread, sizeof(thread)) < 0 ||\
      argptr(1, (void*)&start_rotine, sizeof(start_rotine)) < 0 ||\
      argptr(2, (void*)&arg, sizeof(arg)) < 0)
    return -1;
  return thread_create(thread, start_rotine, arg);
}

int
sys_thread_exit(void)
{
  void* ret_val;
  if (argptr(0, (void*)&ret_val, sizeof(ret_val)) < 0)
    return -1;
  thread_exit(ret_val);
  return 0;
}

int
sys_thread_join(void)
{
  thread_t thread;
  void** retval;
  if (argint(0, (int*)&thread) < 0 || argptr(1, (void*)&retval, sizeof(retval)) < 0)
    return -1;
  return thread_join(thread, retval);
}

