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
extern void forkret(void);
extern void trapret(void);

// same with wakeup1() in proc.c
static void
wakeup_lwp_parent(void *chan)
{
  for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
  return ;
}

void
cancel_alloc_lwp(struct proc *lwp)
{
  lwp->is_lwp = false;
  if (lwp->kstack)
  {
    kfree(lwp->kstack);
    lwp->kstack = nullptr;
  }
  lwp->state = UNUSED;
  return ;
}


// alloc lwp. return new (light weight) process
// ptable.lock acquired before called
// very similar with allocproc() in proc.c
struct proc*
alloc_lwp(void)
{
  struct proc *cur = myproc(), *p;
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == UNUSED)
      break;
  if (p->state != UNUSED)
    return nullptr;  // unused proc not found

  // allocate kernel stack
  if (!(p->kstack = kalloc()))
    return nullptr;
  char *sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  // above: similar with allocproc()
  // below: for lwp

  // share page table
  p->pgdir = cur->pgdir;

  // set lwp info
  p->parent = (cur->is_lwp ? cur : cur->parent);  // what if cur is initproc? :thinking:

  return p;
}


// very similar with exec (sz -> lwp->parent->sz)
int
alloc_userstack(struct proc *lwp)
{
  // allocate two pages at the next page boundary.
  // make the first inaccessible. use the second as the user stack.
  lwp->parent->sz = PGROUNDUP(lwp->parent->sz);
  if (!(lwp->parent->sz = allocuvm(lwp->pgdir, lwp->parent->sz, lwp->parent->sz + 2*PGSIZE)))
  {
    cancel_alloc_lwp(lwp);
    // unlike "bad" in exec.c, no freevm(pgdir): because pgdir shared.
    return -1;
  }
  clearpteu(lwp->parent->pgdir, (char*)(lwp->parent->sz - 2*PGSIZE));
  lwp->sz = lwp->parent->sz;
  return 0;
}


int
thread_create(thread_t *thread, void *(start_routine)(void*), void *arg)
{
  struct proc *cur = myproc(), *lwp;
  acquire(&ptable.lock);
  if ((lwp = alloc_lwp()) == nullptr || !alloc_userstack(lwp))
    goto bad;

  // parameters (similar with exec(); but one parameter {arg})
  uint ustack[2] = {0xffffffffU, (uint)arg};  // fake return PC, arg
  uint sp = lwp->sz -  2 * sizeof(uint);  // 2 uint data in stack
  if (!copyout(lwp->pgdir, sp, ustack, 2 * sizeof(uint)))
    goto bad;

  // set function data
  lwp->tf->eip = (uint)start_routine;
  lwp->tf->esp = sp;
  // share process memory size
  for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (lwp->parent->pid == p->parent->pid)
      p->sz = lwp->sz;
  // copy directory
  lwp->cwd = cur->cwd;
  // copy fd
  for (int fd = 0; fd < NOFILE; fd++)
    if (cur->ofile[fd])
      lwp->ofile[fd] = filedup(cur->ofile[fd]);

  // lwp metadata
  *thread = lwp->pid;
  lwp->is_lwp = true;
  lwp->pid = nextpid++;   // set pid (t_id) if lwp must be made
  lwp->state = RUNNABLE;  // set ready

  release(&ptable.lock);
  return 0;

bad:
  cancel_alloc_lwp(lwp);
  release(&ptable.lock);
  return -1;
}


// very similar with exit in proc.c
void
thread_exit(void *retval)
{
  struct proc *curlwp = myproc();
  if (!curlwp->is_lwp)
    panic("thread_exit: not lwp exit");
  if (curlwp == get_initproc())
    panic("init exiting");

  // set return value
  curlwp->return_value = retval;

  // close open files
  for (int fd = 0; fd < NOFILE; fd++)
  {
    if (curlwp->ofile[fd])
    {
      fileclose(curlwp->ofile[fd]);
      curlwp->ofile[fd] = nullptr;
    }
  }

  begin_op();
  iput(curlwp->cwd);
  end_op();
  curlwp->cwd = nullptr;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup_lwp_parent(curlwp->parent);

  // Pass abandoned children to init.
  for (struct proc *p = ptable.proc, *initproc = get_initproc(); p < &ptable.proc[NPROC]; p++)
  {
    if (p->parent == curlwp)
    {
      p->parent = initproc;
      if (p->state == ZOMBIE)
        wakeup_lwp_parent(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curlwp->state = ZOMBIE;
  sched();  // holding lock now. change to scheduler
  panic("zombie exit");
}

// very similar with wait()
// but not child proces
int
thread_join(thread_t thread, void **retval)
{
  struct proc *curproc = myproc();
  int havelwp;

  acquire(&ptable.lock);
  for (;;)
  {
    havelwp = false;
    for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent != curproc)
        continue;
      if (p->pid != thread)
        continue;
      havelwp = true;
      if (p->state == ZOMBIE)
      {
        // Found one.
        // pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        // freevm(p->pgdir);  // pgdir is shared!!
        p->pgdir = nullptr;   // instead do this.
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        p->is_lwp = 0;
        *retval = p->return_value;
        p->return_value = 0;
        release(&ptable.lock);
        return 0;
      }
    }

    // No point waiting if we don't have any lwp
    if (!havelwp || curproc->killed)
    {
      release(&ptable.lock);
      return -1;
    }

    // Wait for lwp to exit.
    sleep(curproc, &ptable.lock);  // release &ptable.lock?
  }
  return -1;  // not reachable
}


// ========== wrapper functions ==========

int
sys_thread_create(void)
{
  // parameters
  thread_t *thread;
  void *(*start_routine)(void*);
  void *arg;
  // check
  if (argptr(0, (void*)&thread, sizeof(thread)) < 0 ||\
      argptr(1, (void*)&start_routine, sizeof(start_routine)) < 0 ||\
      argptr(2, (void*)&arg, sizeof(arg)) < 0)
    return -2;
  return thread_create(thread, start_routine, arg);
}


int
sys_thread_exit(void)
{
  // parameter
  void *retval = nullptr;
  // check
  if (argptr(0, retval, sizeof(retval)) < 0)
    return -2;
  thread_exit(retval);
  return 0;
}


int
sys_thread_join(void)
{
  // parameters
  thread_t thread;
  void **retval;
  // check
  if (argint(0, (thread_t*)&thread) || /* << argint, not argptr! */\
      argptr(1, (void*)&retval, sizeof(retval)) < 0)
    return -2;
  return thread_join(thread, retval);
}


