#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include "CLEvent.h"

CLEvent::CLEvent(int bSemaphore)
{
    m_flag = 0;
    this->bSemaphore = bSemaphore;
    pthread_mutex_init(&m_mutex,0);
    pthread_cond_init(&m_cond,0);
}

CLEvent::~CLEvent()
{
    pthread_mutex_destroy(&m_mutex);
    pthread_cond_destroy(&m_cond);
}

void CLEvent::Set()
{
    pthread_mutex_lock(&m_mutex);

    m_flag++;
    pthread_cond_signal(&m_cond);
    pthread_mutex_unlock(&m_mutex);
}

void CLEvent::SetAll()
{
    pthread_mutex_lock(&m_mutex);

    m_flag++;
    pthread_cond_broadcast(&m_cond);
    pthread_mutex_unlock(&m_mutex);
}

void CLEvent::Wait()
{
    pthread_mutex_lock(&m_mutex);
    if(m_flag == 0){
        pthread_cond_wait(&m_cond,&m_mutex);
    }
    if(bSemaphore == true){
        m_flag--;
    }else{
        m_flag = 0;
    }
    pthread_mutex_unlock(&m_mutex);
}