/**
 * TODO： 线程池类
*/
#include "thread_poll.h"
#include <unistd.h>

static volatile int threads_keepalive;

CLJobQueue::CLJobQueue():mcond(false)
{
    len = 0;
    front = rear = nullptr;
    pthread_mutex_init(&op_mutex,NULL);
}

CLJobQueue::~CLJobQueue()
{
    pthread_mutex_destroy(&op_mutex);
}


void CLJobQueue::push(job *newJob)
{
    pthread_mutex_lock(&op_mutex);
    newJob->next = nullptr;
    if(len == 0){
        front = rear = newJob;
    }else{
        rear->next = newJob;
        rear = newJob;
    }
    len++;
    mcond.Set();
    pthread_mutex_unlock(&op_mutex);
}

job* CLJobQueue::pop()
{
    job* p_job = nullptr;
    pthread_mutex_lock(&op_mutex);
    if(len > 1){
        p_job = front;
        front = p_job->next;
        len--;
        mcond.Set();// 因为是二进制信号量，这里很关键
    }else if(len == 1){
        p_job = front;
        front = rear = nullptr;
        len = 0;
    }
    pthread_mutex_unlock(&op_mutex);
    return p_job;
}



CLThreadPool::CLThreadPool(int num_threads)
{
    threads_keepalive = 1;

    working_nums = 0;
    alive_nums = 0;
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&idle,NULL);

    tid = (pthread_t*)calloc(num_threads,sizeof(pthread_t));
    for(int i = 0;i < num_threads;i++){
        pthread_create(&tid[i],NULL,thr_fn,(void*)this);
        pthread_detach(tid[i]);
    }
}

CLThreadPool::~CLThreadPool()
{
    threads_keepalive = 0;
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&idle);
    free(tid);
}

void* CLThreadPool::thr_fn(void* arg)
{
    CLThreadPool *thpool = (CLThreadPool*)arg;
    thpool->run();
    return NULL;
}

void CLThreadPool::run()
{
    pthread_mutex_lock(&mutex);
    alive_nums++;
    pthread_mutex_unlock(&mutex);

    while (threads_keepalive)
    {
        jQueue.mcond.Wait();

        if(threads_keepalive){
            pthread_mutex_lock(&mutex);
            working_nums++;
            pthread_mutex_unlock(&mutex);

            void (*func)(void*);
            void *arg;
            job *p_job = jQueue.pop();
            if(p_job){
                func = p_job->function;
                arg = p_job->arg;
                func(arg);
                free(p_job);
            }

            pthread_mutex_lock(&mutex);
            working_nums--;
            if(working_nums == 0){
                pthread_cond_signal(&idle);
            }
            pthread_mutex_unlock(&mutex);
        }

    }

    pthread_mutex_lock(&mutex);
    alive_nums--;
    pthread_mutex_unlock(&mutex);
    
}


void CLThreadPool::thPoolAddWork(void (*function)(void*),void* arg)
{
    job *p_job = (job*)calloc(1,sizeof(job));
    p_job->function = function;
    p_job->arg = arg;
    jQueue.push(p_job);
}

void CLThreadPool::thPoolWait()
{
    pthread_mutex_lock(&mutex);
    while(jQueue.len || working_nums){
        pthread_cond_wait(&idle,&mutex);
    }
    pthread_mutex_unlock(&mutex);
}

void CLThreadPool::thPoolDestroy()
{
    threads_keepalive = 0;

    time_t start,end;
    double tPassed = 0.0;
    time(&start);
    while (tPassed < 1.0 && alive_nums)
    {
        jQueue.mcond.SetAll();// 给所有空闲进程死的机会
        time(&end);
        tPassed = difftime(end,start);
    }
    
    while (alive_nums)
    {
        jQueue.mcond.SetAll();// 等工作结束
        sleep(1);
    }
    delete this;
}