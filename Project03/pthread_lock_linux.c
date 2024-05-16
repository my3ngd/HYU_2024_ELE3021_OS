#include <stdio.h>
#include <pthread.h>
#define true 1
#define false 0

int shared_resource = 0;

#define NUM_ITERS 10
#define NUM_THREADS 10
#define NUM_DUMMY_ITER 10000

void lock(int tid);
void unlock(int tid);

int turn;
int flag[NUM_THREADS];

void lock(int tid)
{
    int other = NUM_THREADS - tid - 1;
    flag[tid] = true;
    turn = tid;
    while (turn == tid && flag[other] == true);
}

void unlock(int tid)
{
    flag[tid] = false;
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
