#include "TcpClient.h"

TcpClient::TcpClient(const char* host,const unsigned short port)
{
    m_done = false;
    m_occur_err = false;


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

    m_data_len = 0;
    m_io_state = 0;
    m_wait_pure_data = 0;

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

bool TcpClient::read_data(size_t len)
{
    int byte_read = 0;
    // These calls return the number of bytes received, or -1 if an error occurred
    // The value 0 may also be returned if the requested number of bytes to receive from a stream socket was 0
    assert(len <= READ_BUFFER_SIZE);

    byte_read = recv(m_sockfd,m_read_buf,len,0);
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
    m_read_idx = byte_read;
    return true;
}