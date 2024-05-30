#ifndef __TCPCLIENT_H__
#define __TCPCLIENT_H__

#include "TcpBase.h"

const int MAX_EVENT_NUMBER = 10000; //最大事件数

#define WRITE_BUFFER_SIZE   1024
#define READ_BUFFER_SIZE    1024

class TcpClient
{
private:

    sockaddr_in m_servAddr;

public:
    int m_write_idx;
    char m_write_buf[WRITE_BUFFER_SIZE];
    int m_read_idx;
    char m_read_buf[READ_BUFFER_SIZE];
    int m_sockfd;

    int m_io_state;// in or out
    struct iovec m_iv[2];
    int m_done;
    int m_occur_err;

    int m_wait_pure_data;// 等待接收数据
    int m_data_len;
    long m_start_time_us;
    long long m_w_file_pos;
    long long m_w_format_pos;
public:
    TcpClient(const char* host = "127.0.0.1",const unsigned short port = PORT);
    ~TcpClient() = default;

    int Connect();
    void Disconnect(const char* op);
    void Send(const char* msg,int n);
    int Receive(char* msg,int maxlen);

    bool read_data(size_t len);
    bool write_data();
};

#endif