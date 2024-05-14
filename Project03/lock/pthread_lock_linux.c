#include <stdio.h>
#include <pthread.h>

int shared_resource = 0;

#define NUM_ITERS 10
#define NUM_THREADS 10

void lock();
void unlock();

void lock()
{
    ;
}

void unlock()
{
    ;
}


int increase_with_delay(int x)
{
    for (int i = 0, delay_value = 0; i < 1000; i++)
        delay_value += i;
    return ++x;
}


void* thread_func(void* arg)
{
    int tid = *(int*)arg;
    lock();

    for (int i = 0; i < NUM_ITERS; i++)
        shared_resource = increase_with_delay(shared_resource);

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