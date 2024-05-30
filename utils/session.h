#pragma once
#include <sys/socket.h>
#include <errno.h>
#include "utils.h"

#define WRITE_BUFFER_SIZE   1024
#define READ_BUFFER_SIZE    1024

class CLSession
{
public:
    void init();
    static void process(void* arg);
private:
    bool read_data();
    bool write_data();
public:
    static int m_session_count;
    static int m_epollfd;
    int m_rw_st;
    int m_peer_fd;
    int m_self_fd;
    int m_write_idx;
    int m_w_offset;
    char m_write_buf[WRITE_BUFFER_SIZE];

    int m_read_idx;
    char m_read_buf[READ_BUFFER_SIZE];

    int m_done;
    int m_occur_err;
private:
    CLUtils utils;
};