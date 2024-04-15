#pragma once

#include "utils.h"
#include "TcpClient.h"
#include "thread_poll.h"
#include "msgHeader.h"
#include "unordered_map"

class CLGenerator
{
public:
    int m_epollfd;
    int m_pipefd[2];
    CLUtils utils;
    TcpClient* m_clients;

    int m_sessions;
    int m_talklen;

    pthread_mutex_t m_op_mutex;
    FILE *m_fp;
    bool m_running;

private:
    epoll_event m_events[10];
    CLThreadPool* m_thpool;
    std::unordered_map<int,int>m_map_table;
public:
    ~CLGenerator()
    {
        delete[] m_clients;
        delete m_thpool;
        printf("~CLGenerator\n");
    }
    void init(int session,int talklen);
    void build_contact();
    static void recordTask(void *arg);
    static void* sendTask(void *arg);
    void run();

private:
    void deal_read(int sockfd);
    bool deal_signal(bool& timeout);

};


struct arg_struct
{
    CLGenerator *trig;
    int fd;
};