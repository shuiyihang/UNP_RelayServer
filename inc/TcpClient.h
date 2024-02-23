#ifndef __TCPCLIENT_H__
#define __TCPCLIENT_H__

#include "TcpBase.h"

class TcpClient : public tcpBase
{
private:
    /* data */
    sockaddr_in m_servAddr;
public:
    int m_sockfd;
    TcpClient(const char* host,const unsigned short port);
    ~TcpClient() = default;

    int Connect();
    void Disconnect(const char* op);
    void Send(const char* msg,int n);
    int Receive(char* msg,int maxlen);
};

#endif