# Project3 Wiki

2020098240 이현빈

# 명세 요약

1. LWP(Light Weight Process)를 구현한다.
LWP는 일반적인 프로세스와 다르게 자원과 주소 공간 등을 공유하여 user-level multitasking을 가능하게 한다.
2. 다음과 같은 API를 제공해야 한다.
    1. `int thread_create(thread_t *thread, void *(*start_routine)(void*), void *arg)` 
        1. `start_routine`에서 시작하는 스레드를 실행한다.
        2. 생성된 스레드 id는 `thread`에   저장된다.
        3. `start_routine`의 parameter로 `arg`를 넘긴다.
        4. 성공적으로 만들어지면 0, 그렇지 않으면 0이 아닌 값을 반환한다.
    2. `void thread_exit(void *retval)`
        1. 스레드를 종료하고, `retval`로 값을 반환한다.
        2. `start_routine`의 마지막에 도달하여 스레드가 종료되는 경우는 **고려하지 않는다.**
    3. `int thread_join(theread_t thread, void **retval)`
        1. 스레드 id가 `thread`인 스레드의 종료를 기다리고, `thread_exit`을 통해 반환한 값을 `retval`로 가져온다.
        2. 스레드가 종료된 후 할당된 자원을 회수한다.
        3. 정상적으로 join된 경우 0, 그렇지 않으면 0이 아닌 값을 반환한다.
3. 다음의 기존 system call들은 아래와 같아야 한다.
    1. `fork`
    스레드에서 `fork`가 호출되면 기존의 `fork`를 문제 없이 실행해야 한다.
    2. `exec`
    기존 프로세스의 모든 스레드를 정리/종료하고, 그 중 하나의 스레드에서 새로운 프로세스를 시작한다.
    3. `sbrk` 
    여러 스레드가 메모리 할당을 요청할 때 할당된 공간이 서로 겹치면 안된다. 할당된 공간은 같은 프로세스의 모든 스레드가 공유한다.
    4. `kill`
    어떤 스레드가 `kill`되면 해당 프로세스의 모든 스레드를 정리/종료한다.
    5. `sleep`
    한 스레드가 `sleep`을 호출하면 해당 스레드만 `sleep`한다. - 특별한 변경점 없다고 예상
    6. `pipe`
    출력에 문제가 없도록 한다. - 특별한 변경점 없다고 예상
4. 스레드는 프로세스와 비슷하게 취급되어야 한다. 즉, 명시된 일부 경우를 제외한다면 프로세스처럼 동작해야 한다. 스레드 역시 프로세스들과 **같이** xv6의 기본 스케줄러인  Round Robin 방식으로 스케줄링된다.
5. [1~4번 명세와 독립] `pthread_lock_linux.c`의 `lock()`, `unlock()`을 수정하여 동기화 API를 구현한다.

이하  “스레드”를 “LWP”로 대신하여 작성한다.

# Design

1. LWP는 이름에서 알 수 있듯, 프로세스처럼 관리하는 것이 유리해 보인다. 프로세스와 함께 스케줄링되므로, 기존의 프로세스의 구조를 가져와 변경하는 것은 합리적이다.
    
    ```cpp
    struct proc {
    	// 생략 (기존 proc의 ㅇ)
      // for lwp
      int tid;                      // lwp ID (0 if 'origin')
      struct proc *origin;          // origin process
      void *retval;                 // return value for 'thread_exit'
    };
    ```
    
    기존의 프로세스를 재활용하므로, LWP인지 일반 프로세스인지 구분하기 위해 위와 같이 세 멤버 변수를 추가하였다.
    
    - `int tid`: LWP의 고유 id. 존재하는 모든 LWP의 tid는 다르다. 일반 프로세스의 경우 0이다.
        - 처음에는 LWP와 프로세스 모두 `pid`로 구분하려고 하였으나, 구현의 복잡도가 증가하여 같은 프로세스의 경우 `pid`는 모두 같고 `tid`로만 구분하게 하였다.
        - 해당 프로세스가 LWP인지 확인하기 위해서는 `proc->tid`를 검사하면 된다.
    - `struct proc *origin`: LWP를 생성한 프로세스. `pid`가 같고 `tid`가 0인 프로세스를 가리킨다. `tid`가 0인 프로세스는 해당 값이 `nullptr`이다.
        - 처음에는 `parent`를 이용하려고 하였으나, `tid`와 마찬가지로 구현의 복잡도가 크게 증가하여 `parent`와 별개의 변수를 선언하였다. (프로세스의 상태를 검사하는 조건문의 코드가 지나치게 길어진다.)
    - `void *retval`: `thread_exit`이 호출될 때 반환값을 프로세스에 임시로 저장하는 용도로 사용된다.
2. 기존 API 분석
    
    구현해야 할 `thread_create`, `thread_exit`, `thread_join`은 모두 기존 프로세스의 동작과 유사하게 동작할 것임을 확인할 수 있다.
    
    - `thread_create`: LWP를 생성하는 동작은 프로세스를 생성하는 동작(`fork`)과 매우 유사하다. 또한 생성 후 `start_routine`부터 실행하는 동작은 `exec`의 후반부와 유사하다.
    - `thread_exit`: LWP를 종료하는 동작은 프로세스를 종료하는 동작(`exit`)과 매우 유사하다.
    - `thread_join`: LWP를 기다리는 동작은 프로세스를 기다리는 동작(`wait`)과 매우 유사하다.
    
    그러므로 세 API를 구현하기 전에, `fork`, `exec`, `exit`, `exec`의 동작을 먼저 분석하자.
    
    - `fork` 함수는 아래와 같다.
        
        ```cpp
        int
        fork(void)
        {
          int i, pid;
          struct proc *np;
          struct proc *curproc = myproc();
        
          // Allocate process.
          if ((np = allocproc()) == 0)
            return -1;
        // ..
        ```
        
        먼저, 프로세스를 할당받는다. 이때 `allocproc()`은 아래와 같다.
        
        ```cpp
        p->state = EMBRYO;
        p->pid = nextpid++;
        
        release(&ptable.lock);
        
        // Allocate kernel stack.
        if ((p->kstack = kalloc()) == 0)
        {
          p->state = UNUSED;
          return 0;
        }
        ```
        
        먼저 비어 있는 프로세스에 커널 스택을 할당받는다.
        
        ```cpp
        sp = p->kstack + KSTACKSIZE;
        
        // Leave room for trap frame.
        sp -= sizeof *p->tf;
        p->tf = (struct trapframe*)sp;
        
        // Set up new context to start executing at forkret,
        // which returns to trapret.
        sp -= 4;
        *(uint*)sp = (uint)trapret;
        ```
        
        stack point와 trap frame을 설정한다.
        
        ```cpp
        sp -= sizeof *p->context;
        p->context = (struct context*)sp;
        memset(p->context, 0, sizeof *p->context);
        p->context->eip = (uint)forkret;
        ```
        
        context를 설정한다.
        
        ```cpp
        // Copy process state from proc.
        if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0)
        {
          kfree(np->kstack);
          np->kstack = 0;
          np->state = UNUSED;
          return -1;
        }
        np->sz = curproc->sz;
        np->parent = curproc;
        *np->tf = *curproc->tf;
        np->tid = 0;
        ```
        
        프로세스의 커널 스택을 할당받으면 기존의 프로세스에서 프로세스 상태를 복사한다.
        
        ```cpp
        np->tf->eax = 0;
        
        for (i = 0; i < NOFILE; i++)
          if (curproc->ofile[i])
            np->ofile[i] = filedup(curproc->ofile[i]);
        np->cwd = idup(curproc->cwd);
        safestrcpy(np->name, curproc->name, sizeof(curproc->name));
        pid = np->pid;
        acquire(&ptable.lock);
        np->state = RUNNABLE;
        release(&ptable.lock);
        
        return pid;
        ```
        
        그 다음 파일을 열고, pid를 설정하며, state를 변경한다. (기타 작업)
        
    - `exec` 함수는 아래와 같다. 프로세스의 실행을 변경하는 부분만 작성한다.
        
        ```cpp
        int
        exec(char *path, char **argv)
        {
        	// ...
          sz = PGROUNDUP(sz);
          if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
            goto bad;
          clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
          sp = sz;
        ```
        
        `sz`(메모리 크기)를 유저 스택의 크기만큼 키운다. (유저 스택 할당)
        
        ```cpp
        // Push argument strings, prepare rest of stack in ustack.
        for(argc = 0; argv[argc]; argc++) {
          if(argc >= MAXARG)
            goto bad;
          sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
          if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
            goto bad;
          ustack[3+argc] = sp;
        }
        ustack[3+argc] = 0;
        
        ustack[0] = 0xffffffff;  // fake return PC
        ustack[1] = argc;
        ustack[2] = sp - (argc+1)*4;  // argv pointer
        
        sp -= (3+argc+1) * 4;
        if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
          goto bad;
        ```
        
        argument를 유저 스택에 추가한다. 이때 fake return PC가 존재하는데, `exec`된 프로세스는 기존의 프로세스와 별개가 되므로 불필요한 값이기에 임의의 값을 넣는다. (잘못 반환하는 경우를 대비하여 디버깅이 쉽도록 아무 값이 아닌 `0xffffff`를 넣은 것으로 이해하였다.)
        
        ```cpp
        // Commit to the user image.
        oldpgdir = curproc->pgdir;
        curproc->pgdir = pgdir;
        curproc->sz = sz;
        curproc->tf->eip = elf.entry;  // main
        curproc->tf->esp = sp;
        switchuvm(curproc);
        freevm(oldpgdir);
        return 0;
        ```
        
        프로세스의 정보를 수정하고 반환한다
        
    - `exit` 함수는 아래와 같다.
        
        ```cpp
        struct proc *curproc = myproc();
        if (curproc == initproc)
          panic("init exiting");
        // Close all open files.
        for (int fd = 0; fd < NOFILE; fd++)
        {
          if (curproc->ofile[fd])
          {
            fileclose(curproc->ofile[fd]);
            curproc->ofile[fd] = 0;
          }
        }
        ```
        
        열린 파일들을 닫는다.
        
        ```cpp
        begin_op();
        iput(curproc->cwd);
        end_op();
        curproc->cwd = 0;
        ```
        
        inode와 cwd를 수정한다.
        
        ```cpp
        acquire(&ptable.lock);
        
        // Parent might be sleeping in wait().
        wakeup1(curproc->parent);
        
        // Pass abandoned children to init.
        for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        {
          if (p->parent == curproc)
          {
            p->parent = initproc;
            if (p->state == ZOMBIE)
              wakeup1(initproc);
          }
        }
        
        // Jump into the scheduler, never to return.
        curproc->state = ZOMBIE;
        sched();
        panic("zombie exit");
        ```
        
        해당 프로세스는 종료될 예정이므로 부모 프로세스를 `wakeup`한 뒤, 자식 프로세스가 존재한다면 부모를 자신이 아닌 `initproc`으로 변경한다. (고아 프로세스) 마지막으로 자신의 상태를 `ZOMBIE`로 변경하고 스케줄러로 이동한다.
        
    - `wait` 함수는 아래와 같다.
        
        ```cpp
        for (;;)
        {
          // Scan through table looking for exited children.
          havekids = 0;
          for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
          {
            if(p->parent != curproc)
              continue;
            havekids = 1;
            if(p->state == ZOMBIE){
              // Found one.
              pid = p->pid;
              kfree(p->kstack);
              p->kstack = 0;
              freevm(p->pgdir);
              p->pid = 0;
              p->parent = 0;
              p->name[0] = 0;
              p->killed = 0;
              p->state = UNUSED;
              release(&ptable.lock);
              return pid;
            }
          }
        ```
        
        `ZOMBIE` 상태의 자식 프로세스를 발견하면, 해당 프로세스를 정리한다.
        
        ```cpp
        // No point waiting if we don't have any children.
        if(!havekids || curproc->killed){
          release(&ptable.lock);
          return -1;
        }
        ```
        
        자식 프로세스가 하나도 없었거나, 현재 프로세스가 죽었다면, `wait`에 실패한다.
        
3. LWP에서 `fork`, `exec`, `sbrk`, `kill`, `sleep`, `pipe`, `exit`이 실행될 수 있어야 한다.
    1. `fork`의 변경 사항
        1. 만약 부모를 LWP의 origin 프로세스로 설정한다면, 생성에는 문제가 없지만 종료 시 LWP가 자식 프로세스를 잃어버렸다고 판단할 수 있음에 주의해야 한다.
        2. 따라서 LWP에서 `fork`가 발생하는 경우, 별도의 추가 동작은 없다.
    2. `exec`의 변경 사항
        1. `exec`가 발생하면, 모든 LWP를 정리해야 한다. 그러므로 `exec` 함수 초반에 모든 LWP를 제거하는 함수를 만들어 호출한다.
        2. 이때 하나의 LWP(혹은 프로세스)를 남겨야 하는데, `exec`를 호출한 LWP가 남도록 설정하였다. 이를 위해 다음과 같은 함수를 구현한다.
            
            `thread_removeall(int pid)`: 현재 LWP를 origin과 교환한 이후, 자신을 origin으로 가지는 모든 LWP를 정리한다.
            
    3. `sbrk`의 변경 사항
        1. `sbrk`는 메모리 크기를 변경해준다. 기존과는 다르게 서로 다른 프로세스간(LWP도 프로세스이므로) 메모리 공유가 발생하므로, 여러 LWP가 동시에 `sbrk`를 호출할 경우 race condition이 발생할 수 있다. 따라서 이를 lock으로 보호해야 한다.
        2. `sbrk`는 `growproc`을 호출하는 system call이므로, `growproc`에 lock을 추가하여 보호할 것이다. `growproc`의 시작 부분에 `ptable.lock`을 acquire 하고 종료 부분에 `ptable.lock`을 release 한다.
    4. `kill`의 변경 사항
        1. `kill`이 호출되면 같은 `pid`를 가지는 모든 프로세스를 정리해야 한다.
        2. 만약 pid와 tid를 혼용하게 된다면 해당 부분의 조건문 구현이 상대적으로 복잡해진다.
        3. 하나의 프로세스도 지우지 못한 경우에만 -1을 반환한다.
    5. `sleep`의 변경 사항
        1. 변경 사항 없음
    6. `pipe`의 변경 사항
        1. 변경 사항 없음
    7. `exit`의 변경 사항
        1. `exit`은 `exec`와 같이 호출되면 모든 LWP를 정리해야 한다.
        2. 그러므로 `exec`에서 사용한 `thread_removeall(int pid)`를 재사용하여 `exit`이 호출된 LWP를 제외한 같은 `pid`를 가지는 모든 프로세스를 정리한다.

# 구현

위에서 분석한 API들을 바탕으로 새로 API를 구현한다.

- 구현된 `thread_create`는 아래와 같다.
    
    ```cpp
    int
    thread_create(thread_t* thread, void* (*start_routine)(void *), void *arg)
    {
      struct proc* curproc = myproc();
      struct proc* lwp;
    
      acquire(&ptable.lock);
      if ((lwp = alloclwp()) == 0)
        goto bad;
    ```
    
    먼저 `alloclwp`를 호출하여 커널 스택을 할당받는다. `allocproc`과 매우 유사하지만, `ptable.lock`을 얻은 상태로 호출되고 `tid`, `origin` 등을 설정하기 위해 `allocproc`을 재사용하지 않고 다른 함수로 구현하였다.
    
    ```cpp
    struct proc *ori = lwp->origin;
    ori->sz = PGROUNDUP(ori->sz);
    if ((ori->sz = allocuvm(ori->pgdir, ori->sz, ori->sz + 2*PGSIZE)) == 0)
      goto bad;
    clearpteu(ori->pgdir, (char*)(ori->sz - 2*PGSIZE));
    lwp->sz = ori->sz;
    for (struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      if (p->pid == lwp->pid) 
        p->sz = lwp->sz;
    
    uint sp = lwp->sz - 2*sizeof(uint);
    uint st_val[2] = {0xFFFFFFFFU, (uint)arg};
    if (copyout(lwp->pgdir, sp, st_val, 2 * sizeof(uint)) < 0)
      goto bad;
    release(&ptable.lock);
    ```
    
    그 다음 유저스택의 크기를 키운다. 커널 스택과 달리 유저 스택은 공유되므로, `origin`의 메모리 크기 `sz`를 함께 변경해준다. (같은 `pid`를 가지는 모든 프로세스에 `sz`를 공유한다.)
    
    여기서 처음 할당 받는 크기가 `2*PGSIZE`인 이유를 알아보았는데, “Stack guard page”라는 개념을 찾을 수 있었다.
    
    그리고 시작할 함수인 `start_routine`과 `arg`를 추가한다. 기존의 `exec`에서 보았던 것과 달리 `argc`(argument count)가 한 개인 경우만 존재하므로, `exec`의 것보다 간단하게 구현하였다.
    
    ```cpp
    for (int fd = 0; fd < NOFILE; fd++)
      if (curproc->ofile[fd])
        lwp->ofile[fd] = filedup(curproc->ofile[fd]);
    lwp->cwd = idup(curproc->cwd);
    
    acquire(&ptable.lock);
    lwp->tf->eip = (uint)start_routine;
    lwp->tf->esp = sp;
    *thread = lwp->tid;
    lwp->state = RUNNABLE;
    release(&ptable.lock);
    return 0;
    ```
    
    파일과 cwd를 설정한다. 마찬가지로 `origin`과 그것을 공유한다. eip와 esp를 설정하여 LWP의 시작 주소(함수)를 설정한다. 마지막으로 `thread`에 tid를 저장하고, 실행 가능한 상태로 변경한 후 종료한다.
    
- 구현된 `thread_exit`는 다음과 같다.
    
    ```cpp
    void
    thread_exit(void* retval)
    {
      struct proc* lwp = myproc();
      if (lwp->tid == 0)
        panic("thread_exit - not lwp\n");
      // if initproc, it may not lwp
    
      for (int fd = 0; fd < NOFILE; fd++)
        if (lwp->ofile[fd])
          lwp->ofile[fd] = nullptr;
    ```
    
    먼저 `ofile`을 닫는다. 실제로 `closefile`을 호출하지는 않는다. 다른 LWP와 공유되는 자원이기 때문에 모든 LWP와 프로세스가 종료되고 `closefile`을 호출해야 한다.
    
    ```cpp
    begin_op();
    iput(lwp->cwd);
    end_op();
    lwp->cwd = 0;
    lwp->retval = retval;
    lwp->state = ZOMBIE;
    ```
    
    그 다음으로는 inode, cwd를 설정하고, return value와 LWP 상태를 설정한다. 이때 `begin_op`와 `end_op`를 호출할 때 `ptable.lock`을 미리 얻은 경우 문제가 발생하여 이후에 lock을 얻었다.
    
    ```cpp
    acquire(&ptable.lock);
    wakeup1(lwp->parent);
    for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
      if (p->parent == lwp)
      {
        p->parent = get_initproc();
        if (p->state == ZOMBIE)
          wakeup1(get_initproc());
      }
    }
    sched();
    panic("zombie exit");
    ```
    
    마지막으로 LWP는 종료될 것이므로 부모 프로세스를 깨운다. 여기서 `wakeup1`는 `proc.c`의  `static`으로 정의된 것을 `static` 키워드를 제외하고 `extern`하여 불러왔다.
    
    만약 자식 프로세스가 존재한다면, `initproc`을 부모로 설정해준다. 여기서 `get_initproc()`은 `initproc`을 반환해주는 함수이다. (마찬가지로 `initproc`이  `static`하게 정의되어 있다.)
    
- `thread_join`은 아래와 같다.
    
    ```cpp
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
            init_proc(p);
            release(&ptable.lock);
            return 0;
          }
        }
        sleep(lwp, &ptable.lock);
      }
      return -1;
    }
    ```
    
    LWP 중 `ZOMBIE` 프로세스를 찾아 초기화한 후 `retval`에 값을 저장한다. (`tid`는 유일함이 보장되므로 `pid`가 같은지 검사하지 않는다.)
    
    이때 프로세스를 초기화하는 함수 `init_proc()`은 다음과 같다.
    
    ```cpp
    void
    init_proc(struct proc *p)
    {
      if (p->kstack)
        kfree(p->kstack);
      p->kstack = nullptr;
      p->pid = p->tid = 0;;
      p->parent = p->origin = nullptr;
      p->name[0] = 0;
      p->killed = false;
      p->state = UNUSED;
      p->pgdir = nullptr;
      p->retval = nullptr;
      return ;
    }
    ```
    
    `init_proc()`은 프로세스를 초기화하지만, 프로세스가 열어둔 파일을 닫거나 `freevm(pgdir)`을 시도하지 않는다. LWP에서 `init_proc`을 호출하였을 때 공유되는 자원을 함께 종료하면 같은 pid를 가지는 다른 프로세스나 LWP에 영향을 준다.
    
- 수정을 거친 `fork`은 다음과 같다.
    
    ```cpp
    int
    fork(void)
    {
      int i, pid;
      struct proc *np;
      struct proc *curproc = myproc();
    
      // Allocate process.
      if ((np = allocproc()) == 0)
        return -1;
    
      // Copy process state from proc.
      if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0)
      {
        kfree(np->kstack);
        np->kstack = nullptr;
        np->state = UNUSED;
        return -1;
      }
      np->sz = curproc->sz;
      np->parent = curproc;
      *np->tf = *curproc->tf;
      np->tid = 0;
    
      // Clear %eax so that fork returns 0 in the child.
      np->tf->eax = 0;
    
      for (i = 0; i < NOFILE; i++)
        if (curproc->ofile[i])
          np->ofile[i] = filedup(curproc->ofile[i]);
      np->cwd = idup(curproc->cwd);
      safestrcpy(np->name, curproc->name, sizeof(curproc->name));
      pid = np->pid;
      acquire(&ptable.lock);
      np->state = RUNNABLE;
      release(&ptable.lock);
    
      return pid;
    }
    ```
    
    여기서 추가된 것은 `np->tid = 0;` 뿐이지만, 다른 곳 또한 주목할 만하다.
    
    - `np->tid = 0;`은 `fork`된 프로세스가 LWP가 아님을 의미한다.
    - `np->parent = curproc;` 부분은 `fork`된 프로세스의 부모가 LWP인지 아닌지를 구분하지 않음을 의미한다. (구분했다면 `np->proc = curproc->tid ? curproc->origin : curproc;`이 되었을 것이다.)
    
    첫 번째는 쉽게 이해할 수 있다. 두 번째의 경우 만약 `fork`된 프로세스의 부모를 `origin`으로 설정하였다면 `exit`으로 프로세스를 종료하였을 때 잘못된 부모 프로세스를 인식하여 문제가 발생함을 확인할 수 있다.
    
    이외에는 특별한 변경점이 없다.
    
- 수정을 거친 `exec`은 다음과 같다.
    
    ```cpp
    int
    exec(char *path, char **argv)
    {
      char *s, *last;
      int i, off;
      uint argc, sz, sp, ustack[3+MAXARG+1];
      struct elfhdr elf;
      struct inode *ip;
      struct proghdr ph;
      pde_t *pgdir, *oldpgdir;
      struct proc *curproc = myproc();
    
      thread_removeall(curproc->pid);
    
      // ...
    }
    ```
    
    `exec` 를 호출하면 기존에 실행하던 LWP를 모두 정리해야 한다. 따라서 `thread_removeall`을 정의하고 호출하였다. 이외의 변경사항은 없다.
    
    이때 `thread_removeall`은 아래와 같다.
    
    ```cpp
    void
    thread_removeall(int pid)
    {
      struct proc* curproc = myproc();
    
      acquire(&ptable.lock);
      swap_origin(curproc);
      for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; ++p)
      {
        if (p->pid == pid && p != curproc)
        {
          init_proc(p);
          for (int fd = 0; fd < NOFILE; fd++)
            if (p->ofile[fd])
              p->ofile[fd] = 0;
        }
      }
      release(&ptable.lock);
    }
    ```
    
    현재 실행중인 LWP(프로세스)를 `origin`으로 설정한다. 그리고 ptable에서 현재 프로세스를 제외하고 같은 pid를 가지는 모든 LWP를 찾아 삭제한다. 이때 현재 프로세스를 제외해야 `exec`의 나머지 부분을 실행할 수 있다. 이때 여기서 호출하는 함수 `swap_origin`은 아래와 같다.
    
    ```cpp
    void
    swap_origin(struct proc* curproc)
    {
      if (curproc->tid == 0)
        return;
      curproc->parent = curproc->origin->parent;
      curproc->origin = nullptr;
      for (struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; ++p)
        if (p != curproc && p->pid == curproc->pid)
          p->origin = curproc;
      curproc->tid = 0;
      return ;
    }
    ```
    
    입력으로 주어진 파라미터 `curproc`이 LWP라면 자신의 `origin`과 위치를 교환한다. 다른 LWP의 `origin`도 현재 프로세스로 수정해준다.
    
- `sbrk`는 `growproc`을 호출하는 system call이다. 따라서 `growproc`을 수정해야 한다. 수정을 거친 `growproc`는 다음과 같다.
    
    ```cpp
    int
    growproc(int n)
    {
      struct proc *curproc = myproc();
    
      acquire(&ptable.lock);   // because of access sz (sz shared: critical)
      uint sz = (curproc->tid ? curproc->origin->sz : curproc->sz);
    
      if (n > 0 && (sz =   allocuvm(curproc->pgdir, sz, sz + n)) == 0)  goto bad;
      if (n < 0 && (sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)  goto bad;
    
      for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
        if (p->pid == curproc->pid)
          p->sz = sz;
      // share size
      if (curproc->tid)
      {
        curproc->origin->sz = sz;
        for (struct proc* p = ptable.proc; p < &ptable.proc[NPROC]; p++)
          if (p->pid == curproc->pid) 
            p->sz = curproc->origin->sz;
      }
      else  curproc->sz = sz;
    
      release(&ptable.lock);
      switchuvm(curproc);
      return 0;
    bad:
      release(&ptable.lock);
      return -1;
    }
    ```
    
    기존의 `growproc`은 `ptable.lock`을 얻지 않았다. 이는 프로세스 메모리의 크기(`sz`)가 모든 프로세스마다 다르고, 이를 공유하지 않기 때문이다. 그러나 LWP를 도입하면 서로 다른 프로세스(LWP 또한 프로세스이므로)간 메모리와 그 크기를 공유하는 critical section이 되므로 lock으로 보호할 필요가 있다.
    
- 수정을 거친 `kill`은 다음과 같다.
    
    ```cpp
    int
    kill(int pid)
    {
      acquire(&ptable.lock);
      int is_killed = false;
      for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      {
        if (p->pid == pid)
        {
          is_killed = p->killed = true;
          // Wake process from sleep if necessary.
          if (p->state == SLEEPING)
            p->state = RUNNABLE;
        }
      }
      release(&ptable.lock);
      return is_killed - 1;
    }
    ```
    
    기존의 `kill` 과 다르게 하나의 프로세스(LWP)를 `kill`하면 같은 pid를 가지는 모든 프로세스(LWP)를 `kill`해야 한다. 기존에는 하나의 프로세스를 지운 뒤 바로 함수를 종료했지만, 수정 후에는 모든 프로세스를 검사한 후 함수를 종료해야 한다.
    하나 이상의 프로세스를 지웠다면 `is_killed`를 `true(1)`로 설정하여 `is_killed-1`을 반환하므로 실패시 -1, 성공하면 0을 반환한다.
    
- 수정을 거친 `exit`은 다음과 같다.
    
    ```cpp
    void
    exit(void)
    {
      struct proc *curproc = myproc();
    
      if (curproc == initproc)
        panic("init exiting");
      thread_removeall(curproc->pid);
      // Close all open files.
      for (int fd = 0; fd < NOFILE; fd++)
      {
        if (curproc->ofile[fd])
        {
          fileclose(curproc->ofile[fd]);
          curproc->ofile[fd] = nullptr;
        }
      }
    
      begin_op();
      iput(curproc->cwd);
      end_op();
      curproc->cwd = nullptr;
    
      acquire(&ptable.lock);
    
      // Parent might be sleeping in wait().
      wakeup1(curproc->parent);
    
      // Pass abandoned children to init.
      for (struct proc *p = ptable.proc; p < &ptable.proc[NPROC]; p++)
      {
        if (p->parent == curproc)
        {
          p->parent = initproc;
          if (p->state == ZOMBIE)
            wakeup1(initproc);
        }
      }
    
      // Jump into the scheduler, never to return.
      curproc->state = ZOMBIE;
      sched();
      panic("zombie exit");
    }
    ```
    
    `exit` 함수는 `exec`와 마찬가지로 `thread_removeall`을 호출하여 자신과 `pid`가 같은 모든 프로세스를 지우고, 마지막에는 자신을 종료하게 하였다.
    

# Trouble Shooting

구현 상 다음과 같은 어려움이 있었다.

- `pgdir`을 잘못 수정하여 xv6가 재부팅되는 현상
- `initproc`이 필요한 상황(고아 프로세스의 부모를 설정하기 위함)에서 `extern`으로 가져올 수 없던 현상
- `inode` 관련 설정 시 `ptable.lock`을 얻은 상태이면 정상적으로 동작하지 않던 현상
- 처음 구현할 때 LWP를 원본 프로세스의 자식으로 설정하였는데, 구현하는 과정에서 `p->parent`를 구분하는 과정의 코드가 복잡해짐 ⇒ 원본 프로세스와 부모 프로세스를 별도로 설정
- `sbrk`를 찾을 수 없었는데, `sys_sbrk`를 확인해보니 `sbrk` 대신 `growproc`이 있음
- `pid`와 `tid`를 혼용하려고 했으나, 같은 프로세스에서 생성된 LWP를 조사하는 과정 등 구현의 복잡함이 있음

---

# Locking

주어진 skeleton code는 다음과 같다. (실행과 무관한 수정이 있음)

```cpp
#include <stdio.h>
#include <pthread.h>
#define true 1
#define false 0

int shared_resource = 0;

#define NUM_ITERS 100
#define NUM_THREADS 10

void lock(){}
void unlock(){}

void* thread_func(void* arg)
{
    int tid = *(int*)arg;
    
    lock();
    
    for (int i = 0; i < NUM_ITERS; i++)
        shared_resource++;
    
    unlock();
    
    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int tids[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++)
    {
        tids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &tids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("shared: %d\n", shared_resource);

    return 0;
}
```

위 코드를 실행하여도 실제로는 race condition을 관측하는 것이 쉽지 않았다. 이는 `NUM_ITERS`와 `NUM_THREADS`가 충분히 크지 않고, `shared_resource++` 연산에 소요되는 시간이 너무 짧아 그렇다고 판단하여 아래와 같이 수정하였다.

```cpp
#include <stdio.h>
#include <pthread.h>
#define true 1
#define false 0

int shared_resource = 0;

#define NUM_ITERS 100
#define NUM_THREADS 100
#define NUM_DUMMY_ITER 10000

void lock(int tid);
void unlock(int tid);

// test and set
int flag;

void lock(){}
void unlock(){}

int update_shared_resource(int before)
{
    for (int dummy = 0, i = 0; i < NUM_DUMMY_ITER; i++)
        dummy += i;
    return ++before;
}

void* thread_func(void* arg)
{
    int tid = *(int*)arg;
    
    lock();
    
    for (int i = 0; i < NUM_ITERS; i++)
        shared_resource = update_shared_resource(shared_resource);
    
    unlock();
    
    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int tids[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++)
    {
        tids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &tids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("shared: %d\n", shared_resource);

    return 0;
}
```

위와 같이 `update_shared_resource`라는 함수를 구현하였다. 이 함수는 `shared_resource`를 1 증가시키지만, 그 연산의 시간을 크게 지연시켰다. 지연시킨 후에는 race condition을 아주 쉽게 관찰할 수 있었다.

Race condition을 해결하기 위해 lock을 구현하는 방법은 크게 두 가지로 분류된다.

1. Atomic hardware의 지원을 받아 atomic 연산을 통해 해결
2. Peterson’s algorithm과 같은 알고리즘만 사용하여 해결

Peterson’s algorithm은 두 개의 프로세스일 경우에는 쉽게 적용할 수 있었지만, 세 개 이상의 경우에는 그렇지 않다. 그래서 이 과제에서 atomic hardware의 지원을 받고자 하였다.

어셈블리어의 `xchg` 명령은 레지스터와 레지스터 혹은 레지스터와 메모리의 값을 atomic하게 교환한다. 이를 이용하여 test-and-set을 이용한 lock을 구현할 수 있다.

```cpp
int flag;

void lock(int tid)
{
    __asm__(
        "mylock: movl $1, %%eax;"   // assign 1 to eax
        "xchg %%eax, %0;"           // "atomic swap" %eax and [flag: mapped]
        "test %%eax, %%eax;"        // set ZF 1 if %eax not zero
        "jnz mylock;"               // jump to [mylock] if not ZF
        : "+m" (flag)               // output
        :                           // input
        : "%eax"                    // used register list
    );
    // printf("%d lock\n", tid);    // print lock statement
}

void unlock(int tid)
{
    // printf("%d unlock\n", tid);  // print lock statement
    flag = false;
}
```

위와 같이 inline-assembly 코드를 작성하였다. 여기서 사용된 명령들에 대한 정보는 아래와 같다.

- `mylock: movl $1, %%eax`: EAX 레지스터에 상수 1을 대입한다. 해당 명령은 `mylock`으로 label된다.
- `xchg %%eax, %0`: EAX 레지스터와 `flag`의 값을 교환한다. `%0`은 변수에 대한 mapping이다.
- `test %%eax, %%eax`: EAX 레지스터와 EAX 레지스터의 값을 AND 연산한다. 다시 말해 EAX 레지스터의 값이 0인지 검사한다. 검사 결과는 ZF(Zero Flag)에 영향을 준다.
- `jnz mylock`: ZF의 값이 0이 아니면 `mylock`으로 `jump`한다.

여기서, `__asm__`은 아래와 같이 사용할 수 있다.

```cpp
__asm__(
    "assembly code"
    "assembly code"
    "..."
    : "mode" (variable)  // output
    : ""                 // input
    : ""                 // used register list
);
```

완성된 코드는 아래와 같으며, 결과의 자세한 확인을 위해 `lock`과 `unlock` 내에 있는 `printf`가 동작하도록 설정하였다.

```cpp
#include <stdio.h>
#include <pthread.h>
#define true 1
#define false 0

int shared_resource = 0;

#define NUM_ITERS 100
#define NUM_THREADS 100
#define NUM_DUMMY_ITER 10000

void lock(int tid);
void unlock(int tid);

// test and set
int flag;

void lock(int tid)
{
    __asm__(
        "mylock: movl $1, %%eax;"   // assign 1 to eax
        "xchg %%eax, %0;"           // "atomic swap" %eax and [flag: mapped]
        "test %%eax, %%eax;"        // set ZF 1 if %eax not zero
        "jnz mylock;"               // jump to [mylock] if not ZF
        : "+m" (flag)               // output
        :                           // input
        : "%eax"                    // used register list
    );
    printf("%d lock\n", tid);       // print lock statement
}

void unlock(int tid)
{
    printf("%d unlock\n", tid);     // print lock statement
    flag = false;
}

int update_shared_resource(int before)
{
    for (int dummy = 0, i = 0; i < NUM_DUMMY_ITER; i++)
        dummy += i;
    return ++before;
}

void* thread_func(void* arg)
{
    int tid = *(int*)arg;
    
    lock(tid);
    
    for (int i = 0; i < NUM_ITERS; i++)
        shared_resource = update_shared_resource(shared_resource);
    
    unlock(tid);
    
    pthread_exit(NULL);
}

int main()
{
    pthread_t threads[NUM_THREADS];
    int tids[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++)
    {
        tids[i] = i;
        pthread_create(&threads[i], NULL, thread_func, &tids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    printf("shared: %d\n", shared_resource);

    return 0;
}
```

위 코드를 실행하면 다음과 같은 결과를 얻는다.

```bash
0 lock
0 unlock
3 lock
3 unlock
6 lock
6 unlock
5 lock
5 unlock
2 lock
2 unlock
4 lock
4 unlock
1 lock
1 unlock
... (생략)
18 lock
18 unlock
14 lock
14 unlock
95 lock
95 unlock
shared: 10000
```

실행 순서는 다를 수 있으나, `NUM_THREADS`와 `NUM_ITERS`을 수정하여도 순차적으로 lock을 얻고 놓는 과정을 반복함을 볼 수 있다.