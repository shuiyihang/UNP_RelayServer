
#ifndef __TCPSERVER_H__
#define __TCPSERVER_H__

#include "TcpBase.h"
#include "thread_poll.h"
// #include "unordered_map"
#include "utils.h"
#include "session.h"

const int MAX_EVENT_NUMBER = 10000; //最大事件数

#define THR_NUMS    30


class TcpServer
{
private:
    /* data */
    int m_listenfd;
    int m_epollfd;

    bool pairing = false;
    int pairfd;
    // std::unordered_map<int,int>pairlist;

    pthread_mutex_t mutex;

    CLThreadPool *thpool;

    epoll_event events[MAX_EVENT_NUMBER];
public:
    int m_pipefd[2];
    CLUtils utils;
    CLSession* sessions;
private:
    void deal_session_connect();
    void deal_read(int sockfd);
    bool deal_signal(bool& timeout,bool& stop_srv);
    void deal_write(int sockfd);
    
public:
    TcpServer(const unsigned short port);
    ~TcpServer()
    {
        pthread_mutex_destroy(&mutex);
        thpool->thPoolWait();
        thpool->thPoolDestroy();
        delete[] sessions;
        printf("~TcpServer\n");
    }


    void Run();
};

#endif