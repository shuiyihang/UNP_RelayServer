#include "session.h"
#include <cstdio>
#include <cstring>
#include "TcpBase.h"

int CLSession::m_session_count = 0;
int CLSession::m_epollfd = 0;

void CLSession::init()
{
    m_write_idx = 0;
    m_w_offset = 0;
    m_read_idx = 0;
    m_done = 0;
    m_occur_err = 0;
    m_peer_fd = -1;
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
        byte_read = recv(m_self_fd,m_read_buf+byte_read,READ_BUFFER_SIZE-m_read_idx,0);
        if(byte_read == -1)
        {
            // 线程不安全？
            // printf("errno:%d\n",errno);
            if(errno == EAGAIN)
            {
                break;
            }else
            {
                err_msg(CLSession::read_data);
            }
            return false;
        }else if(byte_read == 0)
        {
            if(READ_BUFFER_SIZE-m_read_idx == 0)
            {
                // printf("Read buffer full\n");
                break;
            }else
            {
                return false;
            }
            
        }
        m_read_idx += byte_read;
    }

    // m_read_buf[m_read_idx] = '\0';
    // printf("byte_read:%d\n",m_read_idx);
    // printf("recv:%s\n",m_read_buf);
    return true;
}
bool CLSession::write_data()
{
    int byte_send = send(m_self_fd,m_write_buf+m_w_offset,m_write_idx,0);
    if(byte_send == -1)
    {
        if(errno == EAGAIN)
        {
            // printf("set EPOLLOUT\n");
            utils.modfd(m_epollfd,m_self_fd,EPOLLOUT);// 重新发送
            return true;
        }else
        {
            err_msg(CLSession::write_data);
        }
        return false;
    }else
    {
        m_w_offset = byte_send;
        m_write_idx -= byte_send;
        if(m_write_idx == 0)
        {
            m_w_offset = 0;
            utils.modfd(m_epollfd,m_self_fd,EPOLLIN);// 重新修改为检测输入模式
        }else
        {
            utils.modfd(m_epollfd,m_self_fd,EPOLLOUT);// 再次发送
        }
    }

    return true;
}

void CLSession::process(void* arg)
{
    CLSession* handle = static_cast<CLSession*>(arg);
    if(handle->m_rw_st == 1)
    {
        CLSession* peer = handle - handle->m_self_fd + handle->m_peer_fd;

        if(0 == peer->m_write_idx)
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
                
                memcpy(peer->m_write_buf,handle->m_read_buf,handle->m_read_idx);
                peer->m_write_idx = handle->m_read_idx;

                peer->write_data();
            }
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