#include "TcpClient.h"
TcpClient::TcpClient(const char* host,const unsigned short port)
{
    bzero(&m_servAddr,sizeof(sockaddr_in));
    m_sockfd = socket(AF_INET,SOCK_STREAM,0);

    m_servAddr.sin_family = AF_INET;
    m_servAddr.sin_port = htons(port);
    inet_pton(AF_INET,host,&m_servAddr.sin_addr);
}

int TcpClient::Connect()
{
    int ret;
    ret = connect(m_sockfd,(sockaddr*)&m_servAddr,sizeof(m_servAddr));
    return ret;
}

void TcpClient::Disconnect(const char* op)
{
    int len = strlen(op);
    int how;
    if(len == 2)
    {
        how = SHUT_RDWR;
    }else
    {
        if(op[0] == 'r')
        {
            how = SHUT_RD;
        }else
        {
            how = SHUT_WR;
        }
    }
    shutdown(m_sockfd,how);
}

void TcpClient::Send(const char* msg,int n)
{
    write(m_sockfd,msg,n);
}

int TcpClient::Receive(char* msg,int maxlen)
{
    int ret;
    ret = read(m_sockfd,msg,maxlen);
    return ret;
}

bool TcpClient::read_data()
{
    int byte_read = 0;
    byte_read = recv(m_sockfd,m_read_buf,READ_BUFFER_SIZE,0);
    if(byte_read == -1)
    {
        if(errno != EAGAIN)
        {
            err_msg(read_data);
            return false;
        }
    }else if(byte_read == 0)
    {
        printf("read_data 0\n");
        return false;
    }

    return true;
}