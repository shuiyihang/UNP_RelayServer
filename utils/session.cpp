#include "session.h"
#include <cstdio>
#include <cstring>

int CLSession::m_session_count = 0;
int CLSession::m_epollfd = 0;

void CLSession::init()
{
    m_write_idx = 0;
    m_read_idx = 0;
    m_done = 0;
    m_occur_err = 0;
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
}
bool CLSession::read_data()
{
    int byte_read = 0;
    m_read_idx = 0;
    while(true)
    {
        // __n为0时返回0
        byte_read = recv(m_self_fd,m_read_buf+byte_read,READ_BUFFER_SIZE-m_read_idx-1,0);
        if(byte_read == -1)
        {
            // 线程不安全？
            // printf("errno:%d\n",errno);
            if(errno == EAGAIN)
            {
                break;
            }
            return false;
        }else if(byte_read == 0)
        {
            return false;
        }
        m_read_idx += byte_read;
    }

    m_read_buf[m_read_idx] = '\0';
    // printf("byte_read:%d\n",m_read_idx);
    // printf("recv:%s\n",m_read_buf);
    return true;
}
bool CLSession::write_data()
{
    int byte_send = 0;
    byte_send = send(m_self_fd,m_write_buf+byte_send,m_write_idx,0);
    if(byte_send == -1)
    {
        if(errno == EAGAIN)
        {
            utils.modfd(m_epollfd,m_self_fd,EPOLLOUT);// 重新发送
            return true;
        }
        return false;
    }
    utils.modfd(m_epollfd,m_self_fd,EPOLLIN);// 重新修改为检测输入模式
    return true;
}

void CLSession::process(void* arg)
{
    CLSession* handle = static_cast<CLSession*>(arg);
    if(handle->m_rw_st == 1)
    {
        // read
        if(false == handle->read_data())
        {
            // 读出错，关闭连接
            handle->m_occur_err = true;
        }else
        {
            // 将peerfd改成out
            // 解析：直接将读到的数据复制到发送里
            CLSession* peer = handle - handle->m_self_fd + handle->m_peer_fd;
            memcpy(peer->m_write_buf,handle->m_read_buf,handle->m_read_idx);
            peer->m_write_idx = handle->m_read_idx;

            handle->utils.modfd(m_epollfd,handle->m_peer_fd,EPOLLOUT);
        }
        
    }else
    {
        if(false == handle->write_data())
        {
            handle->m_occur_err = true;
        }
    }
    handle->m_done = 1;
}