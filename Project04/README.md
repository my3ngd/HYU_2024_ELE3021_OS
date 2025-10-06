# Project4 Wiki

2020098240 이현빈

# 명세 요약

1. Copy-on-Write 구현
    
    운영체제에서 Copy-on-Write는 `fork` 실행 시 부모 프로세스를 복제하는 (=오버헤드가 높은) 기존의 방식과 달리, 프로세스 메모리를 복사하지 않고 참조하였다가, **수정될 경우에만 copy**하는 방식을 의미한다.
    
    `fork()`함수를 실행하면 다음과 같이 동작해야 한다.
    
    - 페이지 참조 카운터가 존재한다. 카운터는 참조하고 있는 프로세스의 수를 나타낸다.
    - 참조 카운터는 처음 프로세스에 의해 할당될 때 1이다.
    - 다른 프로세스가 페이지를 참조하면(`fork()`가 호출되면) 페이지 참조 카운터는 증가한다.
    - 프로세스가 페이지를 참조하지 않게 되면 페이지 참조 카운터는 감소한다.
    - 페이지의 참조 카운터가 0인 경우에만 free page로 전환할 수 있다.
    - 참조 카운터가 2 이상인 경우 (공유 페이지인 경우) 해당 페이지는 write가 불가하다.
    - 참조 카운터가 2 이상인 페이지에서 write를 시행하면 `T_PGFLT` trap과 trap handler를 설정해야 한다.
    - trap handler 함수(`CoW_handler()`)는 유저 메모리의 복사본을 만든다.
2. 함수 구현
    
    Copy-on-Write를 위해 아래 함수들을 구현해야 한다.
    
    - `void incr_refc(uint)`: 페이지 참조 카운터를 1 증가시킨다.
    - `void decr_refc(uint)`: 페이지 참조 카운터를 1 감소시킨다.
    - `int get_refc(uint)`: 페이지 참조 카운터를 반환한다.
    - `void CoW_handler(void)`: 유저 메모리의 복사본을 만든다. `T_PGFLT` trap에서 호출된다.
    
    또한 테스트를 위하여 아래 System call들을 구현해야 한다.
    
    - `int countfp(void)`: 시스템에 존재하는 free page의 수를 반환한다.
    - `int countvp(void)`: 현재 프로세스의 유저 메모리에 할당된 가상 페이지(logical page)의 수를 반환한다. 가상 주소 0부터 프로세스의 메모리 크기 `sz`까지의 페이지 수를 센다. 커널 주소 공간에 매핑된 페이지는 제외한다.
    - `int countpp(void)`: 현재 프로세스의 페이지 테이블에서 유효한 물리 주소가 할당된 PTE(page table entry)의 수를 반환한다. (반환값은 `countvp`와 동일해야 한다.)
    - `int countptp(void)`: 프로세스의 페이지 테이블에 의해 할당된 페이지 수를 반환한다. 이때 프로세스 내부 페이지 테이블을 저장하는데 사용된 모든 페이지와 페이지 디렉토리에 사용된 페이지의 수, 커널 페이지 테이블 매핑을 저장한 페이지 테이블을 모두 포함한다.

# Design

`fork()`시 동작을 변경하므로, `fork()`를 가장 먼저 살펴보아야 한다. `fork()`함수의 초반 부분 코드는 아래와 같다.

```cpp
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
...
```

자식 프로세스를 만드는 과정에서 `copyuvm()` 함수가 부모 프로세스의 메모리를 복사하고 있음을 알 수 있다. `copyuvm()`은 `vm.c`에 정의되어 있는데, `copyuvm()`함수를 수정하여 메모리를 새로 할당하고 복사하는 대신 참조하도록 수정할 것이다.

기존 `copyuvm()`은 아래와 같다.

```cpp
pde_t*
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  char *mem;

  if((d = setupkvm()) == 0)
    return 0;
  for(i = 0; i < sz; i += PGSIZE){
    if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if(!(*pte & PTE_P))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char*)P2V(pa), PGSIZE);
    if(mappages(d, (void*)i, PGSIZE, V2P(mem), flags) < 0) {
      kfree(mem);
      goto bad;
    }
  }
  return d;

bad:
  freevm(d);
  return 0;
}
```

여기서 `setupkvm()`, `walkpgdir()`, `memmove()`, `mappages()`에 대해 간단히 알아보자.

- `setupkvm()`: kernel virtual memory의 페이지 테이블을 생성하고 각 메모리의 권한을 설정한다.
- `walkpgdir()`: `va`가 가리키는 곳에 페이지 테이블이 없다면 만들고, 해당 페이지 테이블을 인덱싱하여 PTE를 반환한다.
- `memmove()`: 메모리를 복사한다.
- `mappages()`: 페이지 테이블을 생성하여 가상 주소와 물리 주소를 매핑한다.

따라서 `copyuvm()`은 `mem`을 `kalloc()`으로 새로 할당받고 `memmove()`하는 대신, 페이지를 읽기 권한으로 수정하고 `pa`를 매핑하면 된다. 마지막으로 해당되는 페이지들의 참조 카운터를 1 증가시킨다.

이때 참조 카운터는 `kalloc.c`의 `kmem` 구조체 내에 선언할 것이다. 추가로 명세에서는 free page의 개수를 세야 하므로, 이 값을 관리하는 변수도 추가한다.

참조 카운터를 수정하는 경우는 `kalloc()` , `kfree()`, `freerange()` 등이 있다. (`freerange()`에서는 0으로 초기화만 한다.)

# 구현

1. `copyuvm`
    
    가장 먼저 수정된 `copyuvm()`의 코드는 아래와 같다.
    
    ```cpp
    pde_t*
    copyuvm(pde_t *pgdir, uint sz)
    {
      pde_t *pde;
      pte_t *pte;
      uint pa, i, flags;
      // char *mem;   // unused variable
    
      if((pde = setupkvm()) == 0)
        return 0;
      for(i = 0; i < sz; i += PGSIZE){
        if((pte = walkpgdir(pgdir, (void *) i, 0)) == 0)
          panic("copyuvm: pte should exist");
        if(!(*pte & PTE_P))
          panic("copyuvm: page not present");
        *pte &= ~PTE_W;  // forbid write
        pa = PTE_ADDR(*pte);
        flags = PTE_FLAGS(*pte);
        // if((mem = kalloc()) == 0)
        //   goto bad;
        // memmove(mem, (char*)P2V(pa), PGSIZE);
        if(mappages(pde, (void*)i, PGSIZE, pa, flags) < 0)  // (V2P(mem) => pa)
          goto bad;
        incr_refc(pa);  // added
      }
      lcr3(V2P(pgdir)); // TLB flush
      return pde;
    
    bad:
      freevm(pde);
      return 0;
    }
    ```
    
    여기서 페이지 테이블 항목을 변경하였으므로 함수를 종료하기 전 TLB를 flush해야 한다.
    
    또한 `pa`의 참조 카운터를 1 증가시켜야 하므로 `incr_refc()`를 호출한다.
    
2. 페이지 참조 카운터 (`kmem.pgrefc[]`)
    
    `incr_refc()`등의 함수는 페이지 참조 카운터를 변경시킨다. 변경할 대상인 페이지 참조 카운터를 `kmem` 구조체 내에 정의한다.
    
    ```cpp
    struct {
      struct spinlock lock;
      int use_lock;
      struct run *freelist;
      int fpcnt;                  // added
      int pgrefc[PHYSTOP/PGSIZE]; // added
    } kmem;
    ```
    
    (여기서 `fpcnt`는 free page의 수를 나타낸다.)
    
    `pgrefc[]` 배열은 페이지 참조 카운터이다. 페이지 개수를 구하기 위해 `PHYSTOP/PGSIZE`를 계산하였다. (`PHYSTOP`은 물리 메모리의 크기이고, `PGSIZE`는 페이지 하나의 크기이다)
    
    이 값을 변경시키는 함수들을 아래와 같이 구현하였다.
    
    ```cpp
    void
    __incr_refc(uint pa)
    {
      kmem.pgrefc[pa/PGSIZE]++;
    }
    
    void
    __decr_refc(uint pa)
    {
      kmem.pgrefc[pa/PGSIZE]--;
    }
    
    int
    __get_refc(uint pa)
    {
      int res;
      res = kmem.pgrefc[pa/PGSIZE];
      if (res < 0)
        panic("negative refc");
      return res;
    }
    
    void
    incr_refc(uint pa)
    {
      acquire(&kmem.lock);
      __incr_refc(pa);
      release(&kmem.lock);
    }
    
    void
    decr_refc(uint pa)
    {
      acquire(&kmem.lock);
      __decr_refc(pa);
      release(&kmem.lock);
    }
    
    int
    get_refc(uint pa)
    {
      acquire(&kmem.lock);
      int res = __get_refc(pa);
      release(&kmem.lock);
      return res;
    }
    ```
    
    `__incr_refc()`, `__decr_refc()`, `__get_refc()`를 따로 구현한 이유는, 필요에 따라`kmem.lock`이 이미 `acquire`된 상황에 사용하기 위함이다. 아래에서 별 다른 구별 없이 사용하면 `kmem.lock`을 두 번 `acquire`하는 상황(=panic 발생)이 발생함을 쉽게 예측할 수 있다.
    
    위 함수들을 사용할 곳에 대해 살펴보자.
    
    - `copyuvm()`: 이미 존재하는 페이지를 참조하게 되면 참조 카운터를 1 증가시킨다. (구현완료)
    - `kfree()`: 페이지를 free할 때 참조 카운터만 1 감소하는지 실제로 free하는지 구분해야 한다.
    - `kalloc()`: 페이지를 할당하므로 해당 페이지의 참조 카운터를 1로 설정한다.
    - `CoW_handler()`: 페이지 참조를 종료하고 새로 할당 받으므로, 참조 카운터를 1 감소시킨다.
    
    `copyuvm()`은 구현되었으므로, 다음으로 `kfree()`를 구현할 것이다.
    
3. `kfree()` 
    
    구현된 `kfree()`는 아래와 같다.
    
    ```cpp
    void
    kfree(char *v)
    {
      struct run *r;
    
      if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
        panic("kfree");
    
      if(kmem.use_lock)
        acquire(&kmem.lock);
    
      if (__get_refc(V2P(v)))
        __decr_refc(V2P(v));
      if (!__get_refc(V2P(v))) {
        // Fill with junk to catch dangling refs.
        memset(v, 1, PGSIZE);
        r = (struct run*)v;
    
        r->next = kmem.freelist;
        kmem.freelist = r;
        kmem.fpcnt++;
      }
    
      if(kmem.use_lock)
        release(&kmem.lock);
    }
    ```
    
    우선 참조 카운터를 1 감소시킨다. 그 이후에 참조 카운터가 0인 경우에만 실제로 free page로 전환시킨다. (참조 카운터가 감소한 이후에도 0이 아니라면 하나 이상의 페이지가 보고 있으므로 free page로 전환하면 안된다)
    
    여기에는 `kmem.fpcnt`를 변경하는 부분도 포함되어 있다.
    
4. `kalloc()` 
    
    다음으로 `kalloc()`은 아래와 같다.
    
    ```cpp
    char*
    kalloc(void)
    {
      struct run *r;
    
      if(kmem.use_lock)
        acquire(&kmem.lock);
      r = kmem.freelist;
      if(r) {
        kmem.fpcnt--;
        kmem.pgrefc[V2P((char*)r)/PGSIZE] = 1;
        kmem.freelist = r->next;
      }
      if(kmem.use_lock)
        release(&kmem.lock);
      return (char*)r;
    }
    ```
    
    기존에는 `kmem.freelist`만 변경했었다. 수정된 이후에는 `kmem.fpcnt`와 페이지 참조 카운터를 1로 설정하는 부분이 추가되었다.
    
5. `CoW_handler()`
    
    `CoW_handler()`함수는 `trap.c` 에서 `T_PGFLT` trap이 발생하면 호출하는 trap handler이다. 먼저 `trap.c`를 수정할 것이다.
    
    ```cpp
    void
    trap(struct trapframe *tf)
    {
      // ...
      switch(tf->trapno){
      // ...
      case T_PGFLT:  // added
        CoW_handler();
        break;
      // ...
    }
    ```
    
    명세에 나온 대로, `T_PGFLT` trap을 처리하였다.
    
    `CoW_handler` 함수는 아래와 같이 동작한다.
    
    - page fault가 발생한 가상 주소를 `rcr2()`로 가져온다.
    - 올바른 가상 주소의 경우, 페이지를 복사한다.
        - 페이지를 복사할 때 참조 카운터를 감소시켜야 한다.
        - 페이지 참조 카운터가 1이 되었다면 공유가 종료된 것이므로 페이지를 쓸 수 있게 한다.
    
    구현한 `CoW_handler()`는 아래와 같다.
    
    ```cpp
    void
    CoW_handler(void)
    {
      uint va;
      if ((va = rcr2()) < 0) {
        myproc()->killed = 1;
        cprintf("CoW handler: invalid va\n");
        return ;
      }
    
      pte_t *pte = walkpgdir(myproc()->pgdir, (void*)va, 0);
      uint pa = PTE_ADDR(*pte);
      int refc = get_refc(pa);
    
      if (!pte || !(*pte & PTE_P))
        panic("CoW handler: page not present");
    
      if (1 < refc) {
        char *mem;
        if ((mem = kalloc()) == 0)
          panic("CoW handler: out of memory");
        memmove(mem, (char*)P2V(pa), PGSIZE);
        *pte = V2P(mem) | PTE_P | PTE_U | PTE_W;
        decr_refc(pa);
      } else if (refc == 1) {
        *pte |= PTE_W;
      }
      lcr3(V2P(myproc()->pgdir));
    }
    ```
    
    - `va`는 오류가 발생한 가상 주소이다. 이 값을 사용해 `pte`를 가져온다.
    - `pte`를 통해 페이지가 얼마나 참조되는지 확인한다. 참조 카운터가 `refc`에 저장된다.
        - `refc`의 값이 2 이상이라면 새 메모리를 할당해야 한다.
            1. `mem`에 `kalloc()`으로 메모리를 할당받는다.
            2. `memmove()`로 할당받은 `mem`에 `pa`의 내용을 복사한다.
            3. `pa`의 참조 카운터를 감소시킨다.
        - `refc`의 값이 1이라면, 페이지가 공유중이지 않으므로 `pte`에 쓰기 권한(`PTE_W`)를 추가해준다.
6. [System call] - `int countfp(void)`
    
    명세에 따르면 free page의 개수를 세는 system call을 구현해야 한다. 이를 위해 `kmem` 구조체에 `fpcnt`를 선언하였다. 다음과 같은 상황에서 변경된다.
    
    - `kinit1()`에서 0으로 초기화된다.
    - `kfree()`에서 실제로 페이지가 free되었을 때 1 증가한다.
    - `kalloc()`에서 실제로 페이지가 할당될 때 1 감소한다.
    
    따라서 `countfp()`는 `kmem.fpcnt`를 반환하면 된다. 구현된 system call은 아래와 같다.
    
    ```cpp
    int
    sys_countfp(void)
    {
      acquire(&kmem.lock);
      int res = kmem.fpcnt;
      release(&kmem.lock);
      return res;
    }
    ```
    
    여기에서 `kmem`을 critical section으로 보고 마찬가지로 `kmem.lock`을 얻었다.
    
7. [System call] - `int countvp(void)`
    
    명세에 따르면 유저 메모리에 할당된 가상 페이지 수를 반환해야 한다.
    
    ```cpp
    int
    sys_countvp(void)
    {
      struct proc *p = myproc();
      int cnt = 0;
      for (int i = 0; i < p->sz; i += PGSIZE) {
        pte_t *pte = walkpgdir(p->pgdir, (void*)i, 0);
        if (pte && (*pte & PTE_U))
          cnt++;
      }
      return cnt;
    }
    ```
    
    현재 프로세스의 메모리의 크기는 `myproc()->sz`에, 페이지 하나의 크기는 `PGSIZE`로 정의되어 있다. 페이지를 순회하면서 pte를 가져오고, 해당 테이블이 유저 메모리인 경우 개수를 더한다. 이때 페이지가 유저 메모리의 것인지는 `mmu.h`에 정의된 `PTE_U`를 사용한다.
    
8. [System call] - `int countpp(void)`
    
    명세에 따르면 유효한 물리 주소가 할당된 page table entry의 수를 반환해야 한다.
    
    ```cpp
    int
    sys_countpp(void)
    {
      struct proc *p = myproc();
      pde_t *pgdir = p->pgdir, *pde;
      int cnt = 0;
    
      for (int i = 0; i < NPDENTRIES; i++) {
        pde = &pgdir[i];
        if (*pde & PTE_U) {
          pte_t *pte = (pte_t*)P2V(PTE_ADDR(*pde));
          for (int j = 0; j < NPTENTRIES; j++)
            if (pte[j] & PTE_U)
              cnt++;
        }
      }
      return cnt;
    }
    ```
    
    Page directory에는 `NPDENTRIES`개의 pde가 존재한다. 유저 메모리의 pde에 대해 페이지 테이블을 탐색하고, 이 중 유저 메모리 영역에 있다면 개수를 더하고 마지막에 반환한다.
    
    명세에 따르면 demand paging이 없기 때문에 `countvp()`와 같은 결과를 내야 하며, 실제로 그렇게 동작함을 확인할 수 있다.
    
9. [System call] - `int countptp(void)`
    
    명세에 따르면 페이지 테이블에 할당된 페이지 수를 반환해야 한다. 이때 페이지 디렉토리에 사용된 개수도 함께 더해야 한다.
    
    ```cpp
    int
    sys_countptp(void)
    {
      struct proc *p = myproc();
      pde_t *pgdir = p->pgdir, *pde;
      int cnt = 1;  // pgdir
    
      for (int i = 0; i < NPDENTRIES; i++) {
        pde = &pgdir[i];
        if (*pde & PTE_P)
          cnt++;
      }
      return cnt;
    }
    ```
    
    여기에서 반환할 페이지의 개수 `cnt`가 기본적으로 1로 설정되었다. 이는 프로세스의 페이지 디렉토리에 사용된 페이지이다. (xv6에는 프로세스마다 한 개의 `pgdir`가 있으며, `pgdir`의 각 entry인 `pde`에는 여러 `pte`의 정보가 담겨 있다.) 페이지 디렉토리를 돌면서 `PTE_P` (present 여부)만 검사하여 개수를 더한다.
    

![page_struct](/HYU_2024_ELE3021_OS/Project04/resources/img1.png)

페이지 구조를 요약하면 위 그림과 같으며, system call들의 역할은 아래처럼 설명할 수 있다.

- `countvp()`: 검은 색 (페이지) 중 유저 메모리에 할당된 것
- `countpp()`: (논리적으로 `countvp()`와 동일)
- `countptp()`: 파란 색(페이지 디렉토리)과 빨간 색(페이지 테이블)에 필요한 페이지의 수

# Test

주어진 테스트케이스를 아래와 같이 통과함을 볼 수 있었다.

![test](/HYU_2024_ELE3021_OS/Project04/resources/img2.png)

`countvp()`, `countpp()`, `countptp()` 모두 예제 테스트 코드 출력 결과와 동일한 값을 얻었다.

# Trouble Shooting

- 명세에 따르면 `incr_refc()`, `decr_refc()`, `get_refc()`에서 “적절한 lock”을 얻어야 한다고 한다. 처음에는 `kmem.lock`을 활용하기로 하였는데, 세 함수를 사용하려는 많은 곳에서 이미 `kmem.lock`을 얻은 상태임을 볼 수 있었다.
    - `kfree()`
        
        ```cpp
        void
        kfree(char *v)
        {
          // ...
          if(kmem.use_lock)
            acquire(&kmem.lock);
        
          if (__get_refc(V2P(v)))    // not get_refc(...)
        	    __decr_refc(V2P(v));   // not decr_refc(...)
          if (!__get_refc(V2P(v))) { // not get_refc(...)
            // ...
          }
        
          if(kmem.use_lock)
            release(&kmem.lock);
        }
        ```
        
        위 코드에서, `kmem.use_lock`에 해당하는 경우, `kmem.lock`을 얻는다. 이때 `get_refc()`, `decr_refc()`를 바로 사용하면 `kmem.lock`을 또 얻으려 하게 되며 `panic`이 발생한다. 그래서 위와 같이 `__get_refc()`, `__decr_refc()`를 별도로 구현하고 사용하였다. `CoW_handler()`에서는 `kmem.lock`을 `acquire`, `release`하는 `get_refc()`와 `decr_refc()`를 사용한다.
        
- `ls`를 입력하여 Copy-on-Write된 프로그램이 `T_PGFLT`를 거쳐 `CoW_handler()`가 정상적으로 실행되는지 확인하였을 때, 아래 사진과 같이 프로그램이 정상적으로 실행되지만 종료되지 않는 것처럼 보이는 현상이 발생했다.
    
    ![pgflt](/HYU_2024_ELE3021_OS/Project04/resources/img3.png)
    
    확인해본 결과 `kfree()`함수에서 페이지를 free page로 만드는 과정에서 `memset`을 하는데, 참조 카운터가 0인 경우에만 `memset`을 실행해야 함에도, 항상 실행되고 있었다. 수정 후에는 정상적으로 동작함을 확인할 수 있었다.