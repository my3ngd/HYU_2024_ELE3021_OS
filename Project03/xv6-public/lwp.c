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

extern int nextpid;
extern int nexttid;
extern struct proc* get_initproc(void);
extern void forkret(void);
extern void trapret(void);

void
cancel_create(struct proc *p)
{
  p->pid = p->tid = 0;
  p->parent = p->origin = nullptr;
  p->killed = false;
  p->pgdir = nullptr;
  p->state = UNUSED;
  if (p->kstack != nullptr)
    kfree(p->kstack);
  p->kstack = nullptr;
  p->retval = nullptr;
  return ;
}

static void
wakeup2(void *chan)
{
  struct proc *p;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

struct proc*
alloclwp()
{
  struct proc* curproc = myproc();
  struct proc* p = nullptr;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == UNUSED)
      break;
  if (p == &ptable.proc[NPROC])
    return nullptr;
  p->tid = nextpid++;
  p->pid = myproc()->pid;
  p->state = EMBRYO;
  p->parent = curproc;
  p->origin = (curproc->tid ? curproc->origin : curproc);
  p->pgdir = curproc->pgdir;
  p->retval = nullptr;

  // copy of allocproc()
  // kstack
  if ((p->kstack = kalloc()) == nullptr)
  {
    cancel_create(p);
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


int thread_create(thread_t *thread, void *(*start_routine)(void*), void *arg)
{
  struct proc* curproc = myproc();
  struct proc* lwp;

  acquire(&ptable.lock);
  // kernel stack
  if ((lwp = alloclwp()) == nullptr)
    goto bad;
  // user stack

  struct proc *ori = lwp->origin;
  ori->sz = PGROUNDUP(ori->sz);
  if ((ori->sz = allocuvm(ori->pgdir, ori->sz, ori->sz + 2*PGSIZE)) == 0)
    goto bad;
  clearpteu(ori->pgdir, (char*)(ori->sz - 2*PGSIZE));
  lwp->sz = ori->sz;

  // function info
  uint sp = lwp->sz - 2*sizeof(uint);
  uint ustack[2] = {0xFFFFFFFF, (uint)arg};
  if (copyout(lwp->pgdir, sp, ustack, 2 * sizeof(uint)) < 0)
    goto bad;
  lwp->tf->eip = (uint)start_routine;
  lwp->tf->esp = sp;

  // share mem sz
  for (struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->pid == lwp->pid) 
      p->sz = lwp->sz;
  release(&ptable.lock);

  // copy fd
  for (int i = 0; i < NOFILE; i++)
    if (curproc->ofile[i])
      lwp->ofile[i] = filedup(curproc->ofile[i]);
  lwp->cwd = idup(curproc->cwd);

  // thread name
  safestrcpy(lwp->name, curproc->name, sizeof(curproc->name));

  acquire(&ptable.lock);
  *thread = lwp->tid;
  lwp->state = RUNNABLE;
  release(&ptable.lock);
  return 0;

bad:
  cancel_create(lwp);
  release(&ptable.lock);
  return -1;
}


void
thread_exit(void *retval)
{
  struct proc *lwp = myproc();
  if (lwp->tid == 0)
    panic("thread_exit: non lwp exit");
  
  // remove fds
  for (int fd = 0; fd < NOFILE; fd++)
    if (lwp->ofile[fd])
      lwp->ofile[fd] = nullptr;
      // but no fileclose(): other thread may use
  
  // remove dir
  begin_op();
  iput(lwp->cwd);
  end_op();
  lwp->cwd = nullptr;

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
  lwp->state = ZOMBIE;
  lwp->retval = retval;
  sched();
  panic("zombie exit");
  return ;
}


int
thread_join(thread_t thread, void **retval)
{
  struct proc *curproc = myproc();
  acquire(&ptable.lock);
  for (;;)
  {
    for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->tid != thread || p->state != ZOMBIE)
        continue;
      if (p->kstack)
        kfree(p->kstack);
      p->kstack = nullptr;
      p->pid = p->tid = 0;
      p->parent = p->origin = nullptr;
      p->killed = false;
      p->state = UNUSED;
      *retval = p->retval;
      p->retval = nullptr;
      release(&ptable.lock);
      return 0;
    }
    sleep(curproc, &ptable.lock);
  }
  return 0;  // not reachable
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

