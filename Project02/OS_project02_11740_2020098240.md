# Project2 Wiki

2020098240 이현빈

# 명세 요약

1. xv6의 기존 스케줄링 방식인 Round-Robin 방식을 MLFQ(Multi-Level Feedback Queue)으로 수정한다. 이때 MLFQ는 아래와 같은 규칙을 가진다.
    1. MLFQ는 4개의 큐인 L0, L1, L2, L3으로 이루어지며 L0, L1, L2는 Round Robin, L3는 Priority Queue 방식으로 구현된다.
        1. 스케줄러는 L0의 RUNNABLE 상태의 프로세스를 스케줄링한다.
        2. L0에 RUNNABLE 상태인 프로세스가 없다면 L1에서 RUNNABLE 상태의 프로세스를 스케줄링한다.
        3. L1에 RUNNABLE 상태인 프로세스가 없다면 L2에서 RUNNABLE 상태의 프로세스를 스케줄링한다.
        4. L2에 RUNNABLE 상태인 프로세스가 없다면 L3에서 RUNNABLE 상태의 프로세스를 스케줄링한다.
    2. 각 큐는 `queue_level * 2 + 2` 의 time quantum을 가지며, time quantum을 모두 소비하면 아래와 같은 규칙으로 큐를 이동한다.
        1. 모든 프로세스는 처음에 L0에서 시작한다.
        2. L0에 있던 프로세스는 pid가 홀수인 경우 L1으로, 짝수인 경우 L2로 이동하며 time quantum을 초기화한다.
        3. L1, L2에 있던 프로세스는 L3로 이동하며 time quantum을 초기화한다.
        4. L3에 있던 프로세스는 time quantum을 초기화하며 Priority가 양수인 경우 1 감소시킨다.
    3. L3 큐는 우선순위 스케줄링을 하며, Priority가 높은 프로세스를 스케줄링한다.
        1. Priority는 0 이상 10 이하의 정수값을 가지며 클 수록 우선순위가 높다.
        2. 처음 프로세스를 실행하면 (별도의 system call이 없다면) Priority는 0이다.
        3. Priority 값은 L0, L1, L2 큐에서는 유효하지 않다. (L3에서만 사용됨)
        4. 만약 Priority 값이 가장 높은 프로세스가 두 개 이상이라면, 그 중 임의로 실행한다.
    4. Global tick이 100이 될 때마다 Priority Boosting한다.
        1. 충분히 긴 시간이 주어질 경우 모든 프로세스는 결국 L3에 모이게 되므로 Priority의 차이에 따른 Starvation을 유발할 수 있기 때문이다.
        2. Priority Boosting이 발생하면 모든 프로세스는 L0로 재조정되며, time quantum을 초기화한다.
2. MoQ (Monopoly Queue)를 구현한다.
    1. MoQ는 L0~L3 스케줄러와 달리 FCFS(First Come First Scheduling) 방식이다.
    2. MoQ에 속한 프로세스는 평소에 스케줄링되지 않는다.
        1. `monopolize` system call이 호출되면 스케줄링된다.
        2. `unmonopolize` system call이 호출되면 MoQ 스케줄링이 종료되어 MLFQ 스케줄링한다.
        이때, Global tick을 0으로 초기화한다.
    3. MoQ의 모든 프로세스들이 종료되면 `unmonopolize` system call을 자동으로 호출한다.
    4. MLFQ에 속한 프로세스들은 `setmonopoly` system call을 통해 MoQ로 이동한다.
    5. MoQ 스케줄링중에는 100 tick마다 발생하는 Priority Boosting이 발생하지 않는다.
3. 다음 system call들을 구현한다.
    1. `void yield(void)` : 자신이 점유한 CPU를 양보한다.
    2. `int getlev(void)` : 자신이 속한 큐의 레벨을 반환한다. MoQ에 속한 경우 99를 반환한다.
    3. `int setpriority(int pid, int priority)` : 특정 pid를 가지는 프로세스의 priority를 설정한다. 반환값은 아래와 같다.
        1. 0: 설정에 성공한 경우
        2. -1: pid를 가진 프로세스가 존재하지 않는 경우
        3. -2: 입력된 priority가 0 이상 10 이하의 정수가 아닌 경우
    4. `int setmonopoly(int pid, int password)` : password가 일치한다면 pid를 가진 프로세스를 MoQ로 이동시킨다. 반환값은 아래와 같다.
        1. {MoQ의 크기}: 암호가 일치하여 MoQ에 이동된 경우
        2. -1: pid를 가진 프로세스가 존재하지 않는 경우
        3. -2: 암호가 일치하지 않는 경우
        4. -3: 이미 MoQ에 존재하는 프로세스인 경우
        5. -4: 자신을 MoQ로 이동하려는 경우
    5. `void monopolize(void)` : MoQ 스케줄링을 시작한다.
    6. `void unmonopolize(void)` : MoQ 스케줄링을 종료한다.

명세에서 자세하게 언급하지 않는 내용들은 아래에서 자세하게 다룰 것이다.

명세에 의하면 큐의 관계를 아래 그림과 같이 요약할 수 있다.

![Untitled](Untitled.png)

위 그림에서, 각 큐의 왼쪽이 back이고, 오른쪽이 front이다.

- L0에서 나온 프로세스는 L0, L1, L2에 들어갈 수 있다. (time quantum 소비 여부 등에 의해 결정)
- L1에서 나온 프로세스는 L1, L3에 들어갈 수 있다.
- L2에서 나온 프로세스는 L2, L3에 들어갈 수 있다.
- L3에서 나온 프로세스는 L3에 들어갈 수 있다.
- MoQ는 system call에 의해 기존 큐에서 제거되어 들어간다.

# Design

- 큐의 구현: `struct proc`의 멤버 변수에 pointer를 추가하여 관리할 수도 있으나, 구현의 편의를 위해 `struct proc* queue[Size]`처럼 단순 배열을 사용할 것이다. 또한, 프로세스가 큐의 레벨을 관리하므로, queue 구조체를 구현할 때, level과 time quantum을 멤버 변수로 추가한다. (MoQ를 제외한 큐는 level을 통해 time quantum을 계산할 수 있으나, 구현의 편의를 위해 나누어두었다.)

이때 편의를 위해 원형 큐 방식으로 구현한다. 프로세스 테이블의 크기가 `NPROC`이므로, 원형 큐의 크기는 `NPROC+1`이 될 것이다. (이유는 구현 파트에서 다룰 것이다.)

큐에는 다음과 같은 연산이 지원된다.
    - Push 연산: 큐의 가장 뒤에 프로세스를 추가한다.
    - Pop 연산: 큐의 가장 앞 프로세스를 제거한다.
    - Front 연산: 큐의 가장 앞 프로세스를 반환한다. (제거하지 않음)
    - Remove 연산: 큐에서 특정 프로세스를 제거한다. (MoQ로 이동할 때 필요함)

- 큐의 관리: 기존에 사용되던 `ptable.lock`을 사용하여 관리한다.
- 우선순위 큐 (L3): 트리 형태의 실제 우선순위 큐를 구현하는 대신, 위에서 구현한 배열 형태의 큐를 재활용할 것이다.

우선순위 큐에는 다음과 같은 연산이 지원된다.
    - Push 연산: 큐의 Push 연산과 동일하다.
    - Top 연산: 우선순위 큐에서 Priority 값이 가장 큰 프로세스 중 **가장 앞에 있는 프로세스**를 선택한다.
    이는 L3에서 우선순위가 같은 프로세스들이 여러 개인 경우 균등하게 실행하기 위함이다.
    - Remove 연산: 우선순위 큐에서 특정 프로세스를 제거한다. (큐에서 사용하는 연산과 같다.)
    
    우선순위 큐를 구현할 때 **항상 가장 앞의 원소의 Priority가 최댓값을 가지도록 관리**할 수도 있다. 이 경우 Top 연산의 시간 복잡도가 `O(1)`로 감소한다는 장점이 있다. 그러나 프로세스의 Priority가 실시간으로 변경될 수 있으므로 관리의 편의를 위해 위와 같이 구현하였다.
    이는 동시에 실행되는 프로세스의 최댓값의 크기(NPROC)가 64이므로, `O(NPROC)`과 `O(1)`의 실행 시간 차이가 유의미하지 않기 때문이다.
    
- Ticks: `tickslock` 락으로 관리된다. 참조하거나 변경할 때 acquire, release를 적절하게 해야 한다. 프로세스의 running time은 `proc` 구조체의 멤버 변수를 추가하여 구현하였다.
Global tick은 Monopoly 상태가 아니라면 100이 되면 0으로 변경해준다.
- Monopoly 상태 관리: `is_monopolized`를 전역 변수로 설정하여 관리한다. 두 가지 system call에 의해 변경된다.
- yield 처리: 아래와 같이 두 가지 방법을 고려할 수 있다.
    - 1 ticks마다 yield
    이 경우 큐가 Round Robin처럼 동작하는 것이 더 명확하다. 각 큐에 비슷한 시점에 도착한 프로세스들이 비슷한 시점에 종료된다. Context switch overhead가 높다는 단점이 있다.
    - time quantum을 모두 소비하였을 때와 interrupt가 발생하였을 때 yield
    이 경우 Round Robin 큐가 FCFS 큐처럼 동작한다. 이는 time quantum 만큼을 한 번에 처리하기 때문이다. 대신 time quantum을 모두 소비하지 못한 경우 같은 level의 큐로 다시 이동한다. Context switch overhead가 첫 번째 방식에 비해 항상 낮다.
    
    여기서는 두 번째 방법으로 구현할 것이다. (첫 번째 방법으로 구현하였을 때와 크게 다르지 않다.)
    
- Priority Boosting: Global tick이 100 이상인 경우 실행한다. 실행 이후 Global tick을 0으로 바꾼다.

`scheduler` 함수의 코드의 실행 순서는 아래와 같을 것이다.

1. Monopoly 상태인지 검사
monopolized 상태이면 MoQ 스케줄링 실행 후 1번으로 되돌아감
2. L0 큐의 크기가 1 이상인 경우 아래를 실행
    1. L0의 첫 번째 값이 RUNNABLE이라면 실행 후 1번으로 되돌아감 (자세한 구현은 후술)
    2. 그렇지 않다면 L0에서 제거 후 1번으로 되돌아감
3. L1, L2, L3에서도 마찬가지로 실행
- 이때, priority boosting은 trap으로 관리하므로, scheduler가 관여하지 않음

기존 `scheduler` 함수는 `ptable`을 그대로 사용하므로 문제되지 않지만, 큐를 직접 구현하였으므로 프로세스가 실행되면 큐(L0)에 직접 넣어주어야 한다. 즉 p→state가 RUNNABLE이 되는 부분을 찾아 L0에 push해야 한다.

해당 부분은 아래와 같다.

- `userinit`
- `fork`
- `yield` (이 경우에는 이미 큐에 존재하므로, 큐에 추가하지 않는다.)
    
    즉, 실행 중인(p→state == RUNNING) 프로세스를 큐에서 제거하지 않는 구현을 하였다.
    
- `wakeup` (wrapper function 내부에 있다)
- `kill` (sleep인지 검사하는 부분)

# Implement

우선, 수정해야 할 파일들은 아래와 같다.

- Makefile: 빌드할 파일이 추가됨
- proc.h: PCB에 멤버 변수 추가
- proc.c: 스케줄러 관련 연산 수정
- user.h: system call 추가
- usys.S: system call 추가
- syscall.h: system call 추가
- syscall.c: system call 추가
- defs.h: system call 및 기타 큐 관련 함수 정의 추가
- trap.c: timer 처리와 yield 호출 조건 수정

또한, 큐 구현을 위해 다음 파일을 추가하였다.

- queue.c: 큐/우선순위 큐의 기본 연산 지원 (push, pop, size 등)
- monopoly.c: MoQ system call 지원

아래는 각 파일에 수정/추가된 구현이다.

- user.h: 추가된 system call들을 정의한다.
    
    ```cpp
    void yield(void);
    int getlev(void);
    int setpriority(int, int);
    int setmonopoly(int, int);
    void monopolize(void);
    void unmonopolize(void);
    ```
    
- usys.S: 추가된 system call들을 정의한다.
    
    ```nasm
    SYSCALL(yield)
    SYSCALL(getlev)
    SYSCALL(setpriority)
    SYSCALL(setmonopoly)
    SYSCALL(monopolize)
    SYSCALL(unmonopolize)
    ```
    
- syscall.h
    
    ```cpp
    // Project 02
    #define SYS_yield           22
    #define SYS_getlev          23
    #define SYS_setpriority     24
    #define SYS_setmonopoly     25
    #define SYS_monopolize      26
    #define SYS_unmonopolize    27
    ```
    
- syscall.c
    
    ```cpp
    // 생략
    extern int sys_yield(void);
    extern int sys_getlev(void);
    extern int sys_setpriority(void);
    extern int sys_setmonopoly(void);
    extern int sys_monopolize(void);
    extern int sys_unmonopolize(void);
    
    static int (*syscalls[])(void) = {
    // 생략
    [SYS_yield]         sys_yield,
    [SYS_getlev]        sys_getlev,
    [SYS_setpriority]   sys_setpriority,
    [SYS_setmonopoly]   sys_setmonopoly,
    [SYS_monopolize]    sys_monopolize,
    [SYS_unmonopolize]  sys_unmonopolize,
    };
    ```
    
- defs.h
    
    ```cpp
    // project02
    ...
    // proc.c
    void            priority_boosting(void);  // 추가
    ...
    // queue.c
    void            init_queue(struct proc_queue*, int);
    int             q_empty(struct proc_queue*);
    int             q_runnable(struct proc_queue*);
    struct proc*    q_front(struct proc_queue*);
    int             q_exist(struct proc_queue*, struct proc*);
    void            q_clear(struct proc_queue*);
    void            q_push(struct proc_queue*, struct proc*);
    void            q_pop(struct proc_queue*);
    void            q_remove(struct proc_queue*, struct proc*);
    struct proc*    q_top(struct proc_queue*);
    int             q_size(struct proc_queue*);
    
    // monopoly.c
    int             setmonopoly(int, int);
    void            monopolize(void);
    void            unmonopolize(void);
    ```
    
- queue.c
    
    ```cpp
    // 생략
    
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
    ```
    
- monopoly.c
    
    ```cpp
    // 생략
    const int mypassword = 2020098240;  // monopolize에 필요한 비밀번호
    
    // 프로세스를 MoQ로 이동
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
    
          // remove from current queue
          if (p->queue_level == 0)  q_remove(&L0, p);
          if (p->queue_level == 1)  q_remove(&L1, p);
          if (p->queue_level == 2)  q_remove(&L2, p);
          if (p->queue_level == 3)  q_remove(&L3, p);
          q_push(&MQ, p);  // new queue_level(99) also determined
    
          release(&ptable.lock);
          return q_size(&MQ);
        }
      }
      // -1: pid not exist
      release(&ptable.lock);
      return -1;
    }
    
    // MoQ 활성화
    void
    monopolize(void)
    {
      is_monopolized = 1;
      return ;
    }
    
    // MoQ 비활성화
    void
    unmonopolize(void)
    {
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
    ```
    
- proc.h
    
    ```cpp
    // 생략
    
    // Per-process state
    struct proc {
      // 생략
      // 추가된 부분
      int priority;      // priority of this proc (in L3 queue)
      int queue_level;   // queue level of this proc (0 ~ 3)
      int ticks;         // used ticks
    };
    
    // process queue
    // L_i queue => time quantum = 2i + 2
    struct proc_queue {
      struct proc* queue[NPROC+1];
      int front, back;
      int level;
      int time_quantum;
    };
    ```
    

queue.c와 monopoly.c에는 많은 함수들이 정의되어 있으나, 대부분 어렵지 않게 구현할 수 있다. 이번 프로젝트에서 구현한 원형 큐를 그림으로 나타내면 아래와 같다.

![Untitled](Untitled%201.png)

빨간 색 화살표가 가리키는 곳이 `front`, 파란 색 화살표가 가리키는 곳이 `back`이다. C++의 iterator인 `std::begin()`, `std::end()`과 유사하다고 이해할 수 있다.

만약 네 번째 그림과 같이 `front`와 `back`이 같다면, 큐는 비어 있다는 것이다. 실제로는 두 값이 동일하더라도 가리키는 곳이 null pointer가 아니라면 큐가 완전히 차 있도록 하는 구현을 할 수 있지만, 간단하게 구현하기 위해 큐의 크기를 프로세스 개수의 최댓값보다 크게 잡아 간단하게 해결하였다.

즉, 세 번째 그림과 같은 상태는 프로세스 개수의 최댓값이 큐에 저장되어 있는 것이다.

일반 큐에서는 front와 back이 이름 그대로의 기능으로서 유효하지만, 우선순위 큐로 사용되는 경우에는 프로세스가 존재하는 범위를 나타내는 의미를 가진다. (C++의 경우 std::priority_queue는 트리 형태로 구현되기 때문에, front(혹은 begin)와 back(혹은 end)의 개념 대신 top만 존재한다.) 이는 구현한 큐를 우선순위 큐와 함께 사용하기 위함이다.

이제 scheduler 함수가 포함된 가장 중요한 proc.c를 살펴보자.

```cpp
void
userinit(void)
{
  // 생략
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  q_push(&L0, p);  // 추가된 부분

  release(&ptable.lock);
}
```

가장 먼저 `userinit`에서 `p->state`를 `RUNNABLE`로 변경하므로, L0에 추가해주어야 한다.

```cpp
int
fork(void)
{
  // 생략
  acquire(&ptable.lock);

  np->state = RUNNABLE;
  q_push(&L0, np);  // 추가된 부분

  release(&ptable.lock);

  return pid;
}
```

`fork`에서도 프로세스가 추가된 후 `RUNNABLE` 상태가 되므로 L0에 추가해준다.

```cpp
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->state == SLEEPING && p->chan == chan)
    {
      p->state = RUNNABLE;
      if (p->queue_level == 0)  q_push(&L0, p);
      if (p->queue_level == 1)  q_push(&L1, p);
      if (p->queue_level == 2)  q_push(&L2, p);
      if (p->queue_level == 3)  q_push(&L3, p);
    }
  }
}

```

```cpp
int
kill(int pid)
{
	//...
	if(p->state == SLEEPING)
	{
	  p->state = RUNNABLE;
    if (p->queue_level == 0)  q_push(&L0, p);
    if (p->queue_level == 1)  q_push(&L1, p);
    if (p->queue_level == 2)  q_push(&L2, p);
    if (p->queue_level == 3)  q_push(&L3, p);
	}
	// ...
}
```

`wakeup`과 `kill`에서도 `RUNNABLE`로 변경되므로 추가해주어야 한다. SLEEPING 상태인 프로세스는 큐에 존재하지 않으므로, 큐에 존재하지 않는다면 원래 있던 큐에 추가하는 작업만 추가하였다.

또한, `yield`의 경우 프로세스의 상태가 `RUNNABLE`로 변경되지만, 실행중이던(`RUNNING` 상태의) 프로세스는 scheduler에서 관리하므로 여기에서 추가하지 않는다.

```cpp
void
scheduler(void)
{
  struct proc *p;  // 선택된 프로세스
  struct cpu *c = mycpu();
  c->proc = nullptr;  // cpu를 점유한 프로세스
  
  for (;;)
  {
    sti();
    acquire(&ptable.lock);
    
    if (is_monopolized) { /* code */ }
    
    if (!q_empty(&L0)) { /* code */ }
    if (!q_empty(&L1)) { /* code */ }
    if (!q_empty(&L2)) { /* code */ }
    if (!q_empty(&L3)) { /* code */ }
    release(&ptable.lock);
  }
}
```

`scheduler` 함수는 크게 위와 같은 구조를 가진다.

스케줄러는 무한 반복문으로 진행된다. 운영체제가 동작하는 동안에는 프로세스 스케줄링이 반드시 발생하기 때문이다.

반복문 내에서 스케줄러는 프로세스 정보가 담긴 프로세스 테이블의 락(`ptable.lock`)을 얻어서(`acquire`) 프로세스 정보에 유일하게 접근함을 보장받고, 반복문의 마지막에 도달하는 모든 부분에서 락을 내려두어야(`release`) 한다.

디자인 부분에서 보았던 것과 같이, 스케줄러는 다음의 순서로 실행된다.

1. MoQ 처리: 아래와 같이 작성할 수 있다.
    
    ```cpp
    if (is_monopolized)
    {
      if (q_empty(&MQ))
      {
        unmonopolize();
        release(&ptable.lock);
        continue;
      }
      p = q_front(&MQ);
      p->queue_level = 99;
      if (p->state == ZOMBIE || p->state == UNUSED)
      {
        q_remove(&MQ, p);
        release(&ptable.lock);
        continue;
      }
      run_proc(c, p);
      release(&ptable.lock);
      continue;
    }
    ```
    
    1. MoQ가 비어 있다면, `unmonopolize` system call을 호출하여 종료한다.
    2. MoQ의 첫 번째 원소가 실행 가능한 상태가 아니라면, pop한다. (단, MoQ의 경우 다른 큐와 달리 SLEEPING인 프로세스를 제거하지 않는다.)
    3. 프로세스를 실행한다.
    
    `continue`문 전에 `release(ptable.lock)`이 존재한다. 해당 위치에서 release를 하지 않으면 `continue` 이후에 다시 `acquire(ptable.lock)`을 마주치게 되므로 문제가 발생하게 된다.
    
2. L0 → L1 → L2 → L3: 각 순서대로 처리하므로, 위와 같은 순서로 처리한다.
    1. L0 처리 부분은 아래와 같다.
        
        ```cpp
        if (q_runnable(&L0))
        {
          p = q_front(&L0);
          q_pop(&L0);
          if (p->state != RUNNABLE)
          {
            release(&ptable.lock);
            continue;
          }
          run_proc(c, p);
          if (L0.time_quantum <= p->ticks)
          {
            p->ticks = 0;
            if (p->state == RUNNABLE)
            {
              if (p->pid % 2 == 1)
                q_push(&L1, p);
              else
                q_push(&L2, p);
            }
            release(&ptable.lock);
            continue;
          }
          if (p->state == RUNNABLE)
            q_push(&L0, p);
        
          release(&ptable.lock);
          continue;
        }
        ```
        
    2. L1 처리 부분은 아래와 같다.
        
        ```cpp
        if (q_runnable(&L1))
        {
          p = q_front(&L1);
          q_pop(&L1);
          if (p->state != RUNNABLE)
          {
            release(&ptable.lock);
            continue;
          }
          run_proc(c, p);
          if (L1.time_quantum <= p->ticks)
          {
            p->ticks = 0;
            if (p->state == RUNNABLE)
              q_push(&L3, p);
            release(&ptable.lock);
            continue;
          }
          if (p->state == RUNNABLE)
            q_push(&L1, p);
        
          release(&ptable.lock);
          continue;
        }
        ```
        
    3. L2 처리 부분은 아래와 같다. (L1과 매우 유사하다.)
        
        ```cpp
        if (q_runnable(&L2))
        {
          p = q_front(&L2);
          q_pop(&L2);
          if (p->state != RUNNABLE)
          {
            release(&ptable.lock);
            continue;
          }
          run_proc(c, p);
          if (L2.time_quantum <= p->ticks)
          {
            p->ticks = 0;
            if (p->state == RUNNABLE)
              q_push(&L3, p);
            release(&ptable.lock);
            continue;
          }
          if (p->state == RUNNABLE)
            q_push(&L2, p);
        
          release(&ptable.lock);
          continue;
        }
        ```
        
    4. L3 처리 부분은 아래와 같다.
        
        ```cpp
        if (q_runnable(&L3))
        {
          p = q_top(&L3);
          if (p->state != RUNNABLE)
          {
            q_remove(&L3, p);
            release(&ptable.lock);
            continue;
          }
          run_proc(c, p);
          if (L3.time_quantum <= p->ticks)
          {
            p->ticks = 0;
            if (0 < p->priority)
              p->priority--;
            release(&ptable.lock);
            continue;
          }
          release(&ptable.lock);
          continue;
        }
        ```
        
    
    또한 코드를 보면 `ptable.lock`과 `tickslock`을 인자로 넘기는 `acquire`, `release`함수를 호출하는 것을 볼 수 있다. xv6에서는 mutual exclusive(상호 배제)를 보장하기 위해 lock을 사용하며, critical section의 앞/뒤로 lock을 acquire/release하여 보호한다. 또한 자기 자신의 release를 기다리는 경우 (Deadlock)를 알기 쉽도록 두 번 acquire하면 panic이 발생하여 구분할 수 있다.
    
    아래 trap.c에서는 `priority_boosting()` 함수를 호출한다. `priority_boosting`은 아래와 같다.
    
    ```cpp
    void
    priority_boosting(void)
    {
      acquire(&ptable.lock);
      struct proc* p;
      for (int i = 0; i < q_size(&L0); i++)
      {
        p = q_front(&L0);
        p->ticks = 0;
        q_pop(&L0);
        q_push(&L0, p);
      }
      while (q_size(&L1))
      {
        p = q_front(&L1);
        p->ticks = 0;
        q_pop(&L1);
        q_push(&L0, p);
      }
      while (q_size(&L2))
      {
        p = q_front(&L2);
        p->ticks = 0;
        q_pop(&L2);
        q_push(&L0, p);
      }
      while (q_size(&L3))
      {
        p = q_front(&L3);
        p->ticks = 0;
        q_pop(&L3);
        q_push(&L0, p);
      }
      release(&ptable.lock);
      return ;
    }
    ```
    
    MoQ가 아닌 모든 큐에 존재하는 값(프로세스)들을 L0로 이동시킨다. 이때 L0에 있는 프로세스도 이미 사용한 tick을 초기화하기 위해 L0→L0 이동을 실행해주어야 한다. 다만 아래 L1, L2, L3와 같이 `while (q_size(&L0))` 를 사용하면 항상 실행하게 될 수 있으므로, for문을 사용하였다.
    
- trap.c
global tick이 100 이상이고 MoQ 스케줄링을 하지 않는 경우 `priority_boosting`을 호출한다.
    
    ```cpp
    void
    trap(struct trapframe *tf)
    {
      // ...
      switch(tf->trapno){
      case T_IRQ0 + IRQ_TIMER:
        if(cpuid() == 0){
          acquire(&tickslock);
          ticks++;
          wakeup(&ticks);
          release(&tickslock);
        }
      // ...
      if(myproc() && myproc()->state == RUNNING
    	  && tf->trapno == T_IRQ0+IRQ_TIMER && !is_monopolized)
      {
        acquire(&tickslock);
        if (ticks%100 == 99)
        {
          release(&tickslock);
          yield();
          priority_boosting();
        }
        else if (myproc()->queue_level * 2 + 2 <= ++(myproc()->ticks))
        {
          release(&tickslock);
          yield();
        }
        else
          release(&tickslock);
      }
      // ...
    }
    ```
    
    위에서는 global tick(`ticks`)을 1 증가시킨다. (100이 되면 0으로 바꾸지 않는 이유는 후술)
    
    아래에서는 yield에 대한 규칙을 정의한다. 여기서 `priority_boosting()`을 호출하는데, 이는 윗부분에서 호출하였을 경우 CPU에 프로세스가 존재하는 상태로 Priority boost를 하기 때문이다.
    따라서 아래 부분에서 Priority boost 전에 `yield`를 호출하여 문제를 해결하였다.
    
    이외에도 time quantum을 모두 사용하면 `yield`를 호출해야 한다.
    
    - 여기서 `yield`는 `tickslock`의 영향을 받는 것으로 보인다. release 전에 `yield`를 호출하면 문제(panic)가 발생한다.
- Makefile
    
    Project01과 같은 방식으로, 아래 부분을 수정하였다. (test.c는 제공된 테스트이다.)
    
    ```makefile
    OBJS = \
        ...
        queue.o\
        monopoly.o\
    ...
    UPROGS=\
        ...
        _test\
    ...
    EXTRA=\
        ...
        printf.c umalloc.c test.c\
        ...    
    ```
    

# Tests & Result

### Tests

다음과 같은 테스트를 진행하였다. (테스트 코드는 수업에서 제공됨)

1. default 테스트: 여러 프로세스를 생성(fork) 후, 각 큐에 머무르는 시간 비율을 측정
    
    모든 프로세스가 자신에게 주어진 time quantum을 전부 쓰고 비슷한 시기에 낮은 레벨로 내려가기 때문에 서로 비슷한 시기에 끝나게 됨. L1 큐가 L2 큐보다 우선 순위가 높으므로, pid가 홀수인 프로세스가 먼저 끝나는 경향을 보임.
    
2. priority 테스트: priority를 설정하여 L3에서 끝나는 시간을 조작
    
    전체적인 시간 사용량은 결국 비슷해지기 때문에 끝나는 시간도 비슷하지만, pid가 큰 프로세스가 조금 더 먼저 끝나는 경향을 보임.
    
3. sleep 테스트
    
    pid가 큰 프로세스에게 더 높은 우선순위(priority)를 부여함. 각 프로세스는 루프를 돌 때마다 바로 sleep 시스템 콜을 호출하는데, sleep 상태에 있는 동안에는 스케줄링 되지 않고 다른 프로세스가 실행될 수 있기 때문에 거의 동시에 작업을 마무리하게 됨.
    
4. MoQ 테스트
    
    pid가 홀수인 프로세스를 MoQ에 추가하고 실행함.
    

실행 방법은 아래와 같다.

- Project02/xv6-public 디렉토리에서 아래 명령을 입력한다.
`make clean; make; make fs.img; ./bootxv6.sh`
    
    이때, `bootxv6.sh`는 아래와 같다.
    
    `qemu-system-i386 -nographic -serial mon:stdio -hdb fs.img xv6.img -smp 1 -m 512`
    

### Result

각 테스트 이후 아래와 같은 결과가 나타난다.

![Untitled](Untitled%202.png)

- L0, L1, L2의 time quantum의 비율은 1:2:3이다. Test 1에서 L0, L1, L2에 머무른 시간 비율을 보면 많은 경우 1:2(L0:L1)와 1:3(L0:L2)의 비율을 이룬다고 볼 수 있다. 그렇지 않은 경우에는 낮은 level의 큐에서 작업이 종료되었거나, Priority Boost로 인해 비율이 올바르게 나타나지 않은 경우로 추측할 수 있다.
    - 테스트 프로그램의 NUM_LOOP 값(100000)을 키우면, time quantum의 비율이 1:2와 1:3에 가까워짐을 관찰할 수 있다.
    - 실행 결과는 global tick 등 다양한 원인에 의해 영향을 받을 수 있다.
- 프로그램의 실행 시간이 충분히 긴 경우, L3 스케줄링에 영향을 받는다. Test 2에서는 pid가 높은 프로세스의 priority를 높게 설정하였는데, 이때 pid가 높은 프로세스가 먼저 종료되는 경향이 있음을 관찰할 수 있다.
- Sleep 상태의 프로세스는 큐에서 제거하는 구현을 하였다. 다시 `wakeup`이 호출되면 원래 있던 큐로 돌아가게 된다.
- MoQ 스케줄링을 하는 프로세스는 대부분의 시간을 MoQ에서 보낸다. pid가 작은 순서대로 만들어지므로, MoQ에서는 pid가 작은 프로세스가 먼저 종료됨을 함께 관찰할 수 있다. 이후에는 자동으로 MoQ 스케줄링에서 MLFQ 스케줄링으로 전환되어 MLFQ 내의 프로세스들이 실행된다.

User program에서 문제가 발생하는 경우 강제 종료시키고 메시지를 출력하며, 이후에 스케줄링이 정상적으로 재개되어야 한다.

- 제공된 test의 마지막에 다음과 같은 코드를 추가하여 Divided by Zero를 유도하였다.
    
    ```cpp
      int n = 5, m = 0;
      n /= m;
      printf(1, "div by zero = %d\n", n);
    ```
    
    xv6에서는 아래와 같은 출력을 얻을 수 있었다.
    
    ![Untitled](Untitled%203.png)
    
    User program에서 문제가 발생하면, trap이 발생하였음을 알리고, user program이 종료되며, 다시 스케줄링이 정상적으로 재개된다.
    

# Trouble Shooting

코드를 작성하는 과정에서 다음과 같은 문제들이 발생하였다.

- 처음 스케줄러를 구현하였을 때, 쉘 프로그램이 실행되지 않는 문제가 있었다. 이는 기존 xv6에서 사용하던 스케줄러는 `ptable`에서 스스로 RUNNABLE 프로세스를 찾는 방식이기에 큐를 구현하지 않고도 Round Robin 방식을 구현하였지만, 새로 구현한 스케줄러는 별도의 큐를 사용하기에 프로세스가 실행되려면 직접 큐에 넣어주어야 했다.
처음에는 스케줄러가 작동할 때 ptable에서 RUNNABLE 프로세스이면서 큐에 없는 경우 L0에 추가하는 방식을 사용하였었는데, 이후에 필요할 때 프로세스를 큐에 넣는 방식으로 구현 방법을 변경하였다.
- 스케줄러가 동작하던 중 스케줄러의 반복문이 작동하지 않는 현상이 있었다. 스케줄러가 다음 스케줄링할 프로세스를 찾지 못했거나, 프로세스 종료 이후 스케줄러로 되돌아오지 못한 것이라고 판단하였다.
기타 오류를 수정하면서 스케줄러를 재작성하였고, 자연스럽게 수정되었다.
- system call에 return value로는 int만 가능하고, parameter를 직접 받을 수 없는 문제가 있었다. 먼저 system call의 return value는 wrapper function으로 해결하였으며, parameter를 받을 수 없는 문제는 `argint` 함수 등을 사용해 해결하였다. (system call들을 정의한 syscall.c에 parameter로 정수, 포인터, 문자열 등을 받을 수 있는 함수가 정의되어 있다.)
예를 들어 `int setmonopoly(int pid, int password)`는 아래와 같은 wrapper function을 정의하면 된다.
    
    ```cpp
    int
    sys_setmonopoly(void)
    {
      int pid, password;
      if (argint(0, &pid) < 0 || argint(1, &password) < 0) return -5;
      return setmonopoly(pid, password);
    }
    ```
    
    여기서 `argint`가 -1을 반환하는 경우 임의로 -5를 반환하였다. (명세에 나타나지 않음)
    

# 이외에 생각해볼 만한 것들 (명세 외)

다음 항목들은 명세에 명확히 제시되지 않았다.

- Monopolized 상태가 아니면서 Priority Boosting이 발생하였을 때 MoQ에 속한 프로세스들이 L0로 이동하는가?
    - 여기에서는 이동시키지 않았다. MoQ에서 나가는 경우는 오직 프로세스가 실행이 완료되어야 하는 경우라고 판단하였다. 후술하겠지만 SLEEPING인 경우에도 MoQ에서 제거하지 않는다. (즉, MoQ 스케줄링 중 프로세스가 SLEEPING 상태가 되더라도 RUNNABLE 상태가 될 때까지 기다린다.)
- Sleep 상태인 프로세스는 어떻게 처리하는가?
    1. 처음 구현하였을 때는 SLEEPING 상태인 경우 큐에서 제거하고, 다시 RUNNABLE 상태가 되었을 때 L0 큐에 추가하는 구현을 했다. 그러나 이 경우 프로세스가 time quantum을 모두 사용하지도 않았고, priority boosting이 없었음에도 속한 큐가 변경된다. MoQ에서 SLEEPING 상태인 경우에는 system call을 호출하지 않았음에도 L0로 강제로 이동함을 볼 수 있었다.
    2. 두 번째로는 모든 경우에 대해 큐에서 제거하지 않았다. 대신 SLEEPING 상태인 프로세스를 스케줄링하기 전에, 같은 레벨 큐의 가장 마지막으로 보냈다.
    3. 세 번째로는 큐에서 RUNNABLE 상태가 아닌 경우 모두 제거하였다. (RUNNING의 경우 제거 후 실행하였으므로, RUNNING 프로세스 또한 없다고 볼 수 있다.) 대신 `wakeup`, `kill` 함수에서 SLEEPING 프로세스가 RUNNABLE이 될 때 원래 큐에 다시 추가해주었다.
    이때 MoQ의 경우는 그렇지 않게 처리하였다.
- Zombie, Unused 상태인 프로세스는 어떻게 처리하는가?
    - 큐에서 제거하였다. MoQ에서도 해당 프로세스가 두 경우에 해당할 경우에는 문제가 발생할 수 있다고 생각하여 제거하였다.
- context switch는 언제 실행하는가?
    1. 매 tick마다 발생하게 할 수 있다.
    2. 혹은 time quantum을 모두 사용하였거나, interrupt 등에 의해 발생할 수 있다.
        
        이 경우 (L0, L1, L2에서는) time quantum을 모두 사용하였을 때와 달리 원래 있던 큐의 마지막으로 이동한다. (time quantum을 모두 사용해야 level이 다른 큐로 이동)
        
    
    첫 번째 방식은 context switch에 의한 overhead가 높다.
    
    두 번째 방식은 Round Robin이지만 FCFS와 매우 유사하다.
    
    - 처음 구현하였을 때 매 tick마다 context switch가 발생하게 하였으나, context switch의 overhead가 스케줄링 overhead의 대부분을 차지하므로 이를 줄이기 위해 time quantum을 모두 사용하였거나 interrupt 등으로만 yield 되도록 하였다.
    - 두 번째 방식에서 프로세스가 실행 중일 때 priority boosting이 발생하면 어떻게 해야 하는가?
        1. yield를 호출한 다음 boosting을 실행한다.
        2. 무시하고 priority boosting을 실행한다.
        
        첫 번째의 경우 예측한 대로 동작하지만, yield와 tickslock을 함께 관리해야 하므로 구현이 까다롭다. (lock의 이중 acquire에 의한 panic 발생)
        
        두 번째의 경우 실행 중인 프로세스는 boosting의 영향을 받지 않지만, 구현이 간단하다. 여기에서는 두 번째 방법을 선택하였다.
        
- Global tick을 초기화할 때, 다른 문제가 발생하지 않는가?
    - `sysproc.c`에는 `sleep(void)`함수의 wrapper function인 `sys_sleep(void)`가 구현되어 있다. 아래는 코드이다.
        
        ```cpp
        int
        sys_sleep(void)
        {
          int n;
          uint ticks0;
        
          if(argint(0, &n) < 0)
            return -1;
          acquire(&tickslock);
          ticks0 = ticks;
          while(ticks - ticks0 < n){
            if(myproc()->killed){
              release(&tickslock);
              return -1;
            }
            sleep(&ticks, &tickslock);
          }
          release(&tickslock);
          return 0;
        }
        ```
        
        여기서 while문의 조건에 `ticks - tick0 < n` 은 지나간 ticks를 센다는 의미이다. 만약 global tick을 0으로 초기화한다면 아래와 같은 상황에 문제가 발생할 수 있다.
        
        - ticks0 + n이 충분히 큰 경우 (ticks의 최댓값인 99보다 큰 경우) 영원히 종료되지 않는다.
        - 충분히 크지 않은 경우에도 종료되더라도 의도한 시간만큼 sleep하지 않 것이다.
        
        명세에 의하면 “Global tick이 100 ticks가 될 때마다 모든 프로세스들은 L0 큐로 재조정됩니다.”라고 하였다. 이를 “100 ticks 주기로 priority boost를 실행한다”고 이해하여 구현하였다. 따라서 100 ticks이 되면 prioirty boost를 실행한 후 0으로 만드는 대신, 매 100 ticks마다 priority boosting을 실행하였다.
        
        - 만약 100 ticks마다 ticks를 0으로 초기화하면, test program에서 `sleep(99);` 코드는 통과하는 반면, `sleep(101)` 코드는 통과하지 못하는 것을 관찰할 수 있다.
        - 다만 `unmonopolize()`를 호출하였을 때 global ticks를 0으로 초기화해야 한다는 것은 명확하다. 여기에서는 `ticks = 0`을 하였다.
            
            이 경우 `sleep` 함수가 `unmonopolize` 호출 전에 시작하여 종료되기 전 `unmonopolize`가 호출될 경우 global ticks가 0이 되므로 원래보다 오랜 시간을 기다려야 할 것이다.