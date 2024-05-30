#pragma once

#include "utils.h"
#include "TcpClient.h"
#include "thread_poll.h"
#include "msgHeader.h"
#include "unordered_map"
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/uio.h>

#include <chrono>

#define HISTORY_SIZE    5
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
    int m_data_fd;
    char *m_data_src_addr;
    struct stat m_src_file_stat;

    bool m_running;

    static int m_out_client_nums;// epoll out状态下的数量
    long long m_w_file_pos;


    static int print_pos;

    int m_history_clients[HISTORY_SIZE];
    long m_test_nums;
private:
    epoll_event m_events[100];
    CLThreadPool* m_thpool;
    std::unordered_map<int,int>m_map_table;
public:
    ~CLGenerator()
    {
        delete[] m_clients;
        delete m_thpool;
        if(m_data_src_addr)
        {
            munmap(m_data_src_addr,m_src_file_stat.st_size);
            m_data_src_addr = NULL;
        }
        printf("~CLGenerator\n");
    }
    void init(int session,int talklen);
    void build_contact();
    static void recordTask(void *arg);
    static void writeTask(void *arg);

    static void* sendTask(void *arg);
    void run();

private:
    void deal_read(int sockfd);
    bool deal_signal(bool& timeout);
    void deal_write(int sockfd);

};


struct arg_struct
{
    CLGenerator *trig;
    int fd;
};



#define ONE_SEND_DEBUG