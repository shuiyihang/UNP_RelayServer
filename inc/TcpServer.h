
#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "TcpBase.h"
#include "thread_poll.h"

const int MAX_EVENT_NUMBER = 10000; //最大事件数

#define THR_NUMS    12


class TcpServer : public tcpBase
{
private:
    /* data */
    int m_listenfd;
    int m_epollfd;

    pthread_mutex_t mutex;

    CLThreadPool *thpool;

    epoll_event events[MAX_EVENT_NUMBER];

    static void dealwithread(void* tmpfd);
    void setnonblocking(int fd);
public:
    TcpServer(const unsigned short port);
    ~TcpServer()
    {
        pthread_mutex_destroy(&mutex);
    }


    void Run();
};

struct argStruct
{
    TcpServer *srv;
    int fd;
};

#endif