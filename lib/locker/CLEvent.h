
#ifndef __CLEVENT_H__
#define __CLEVENT_H__

class CLEvent
{
public:
    CLEvent(int bSemaphore = true);
    virtual ~CLEvent();

    void Set();
    void Wait();
    void SetAll();

private:
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
    int m_flag;
    int bSemaphore; // 二进制信号量和计数信号量，分别用于实现互斥访问和资源池管理
};

#endif