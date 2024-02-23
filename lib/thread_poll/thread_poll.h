#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H
#include <pthread.h>
#include "../locker/CLEvent.h"
#include <malloc.h>


typedef struct job
{
    void (*function)(void* arg);
    void* arg;
    job *next;
}job;

class CLJobQueue
{
public:
    CLJobQueue();
    ~CLJobQueue();
    void push(job *newJob);
    job* pop();
public:
    CLEvent mcond;
    int len;

private:
    pthread_mutex_t op_mutex;
    job *front;
    job *rear;
};


class CLThreadPool
{
public:
    CLThreadPool(int num_threads);
    ~CLThreadPool();

    void thPoolAddWork(void (*function)(void*),void* arg);
    void thPoolWait();
    void thPoolDestroy();

public:

private:
    static void* thr_fn(void* arg);
    void run();
    
private:
    CLJobQueue jQueue;
    pthread_t *tid;
    volatile int working_nums;
    volatile int alive_nums;
    
    pthread_mutex_t mutex;
    pthread_cond_t idle;
};

#endif