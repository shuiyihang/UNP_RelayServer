
#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "TcpBase.h"
#include "thread_poll.h"
#include "unordered_map"

const int MAX_EVENT_NUMBER = 10000; //最大事件数

#define THR_NUMS    30


class TcpServer : public tcpBase
{
private:
    /* data */
    int m_listenfd;
    int m_epollfd;

    bool pairing = false;
    int pairfd;
    std::unordered_map<int,int>pairlist;

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
        thpool->thPoolWait();
        thpool->thPoolDestroy();
    }


    void Run();
};

struct argStruct
{
    TcpServer *srv;
    int fd;
};

#endif