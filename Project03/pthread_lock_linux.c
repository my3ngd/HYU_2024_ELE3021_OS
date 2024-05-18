#include <stdio.h>
#include <pthread.h>
#define true 1
#define false 0

int shared_resource = 0;

#define NUM_ITERS 100
#define NUM_THREADS 100
#define NUM_DUMMY_ITER 1000

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
    // printf("%d lock\n", tid);    // print lock statement
}

void unlock(int tid)
{
    // printf("%d unlock\n", tid);  // print lock statement
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
