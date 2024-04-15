#include "TcpServer.h"
#include <cassert>

TcpServer::TcpServer(const unsigned short port)
{
    int ret = 0;
    sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    m_listenfd = socket(AF_INET,SOCK_STREAM,0);
    bind(m_listenfd,(sockaddr*)&addr,sizeof(addr));
    listen(m_listenfd,10);

    m_epollfd = epoll_create(5);

    utils.addfd(m_epollfd,m_listenfd,EPOLLIN);

    ret = socketpair(PF_UNIX,SOCK_STREAM,0,m_pipefd);
    assert(ret != -1);
    utils.setnonblocking(m_pipefd[1]);

    sessions = new CLSession[MAX_FD];
    CLSession::m_epollfd = m_epollfd;

    utils.addfd(m_epollfd,m_pipefd[0],EPOLLIN | EPOLLRDHUP);

    utils.addsig(SIGPIPE,true,SIG_IGN);
    utils.addsig(SIGTERM,false);
    utils.addsig(SIGALRM,false);
    alarm(TIMESLOT);
    utils.u_pipefd = m_pipefd;

    thpool = new CLThreadPool(THR_NUMS);
    pthread_mutex_init(&mutex,NULL);

}

bool TcpServer::deal_signal(bool& timeout,bool& stop_srv)
{
    char signals[1024];
    int ret = recv(m_pipefd[0],signals,sizeof(signals),0);
    if(ret == -1)
    {
        return false;
    }else if(ret == 0)
    {
        return false;
    }else
    {
        for(int i = 0;i < ret;i++)
        {
            switch (signals[i])
            {
                case SIGALRM:
                {
                    timeout = true;
                    break;
                }
                case SIGTERM:
                {
                    stop_srv = true;
                    break;
                }
            }
        }
    }
    return true;
}

void TcpServer::deal_session_connect()
{
    // 接受新的连接
    sockaddr_in conaddr;
    socklen_t len = sizeof(conaddr);
    char hostname[128];

    int confd = accept(m_listenfd,(sockaddr*)&conaddr,&len);

    if(CLSession::m_session_count >= MAX_FD)
    {
        return;
    }

    pthread_mutex_lock(&mutex);
    utils.addfd(m_epollfd,confd,EPOLLIN | EPOLLET | EPOLLONESHOT);// 改用边缘触发模式
    pthread_mutex_unlock(&mutex);

    CLSession::m_session_count++;
    sessions[confd].init();
    sessions[confd].m_self_fd = confd;

    if(pairing == false)
    {
        pairfd = confd;
        pairing = true;
    }else
    {
        sessions[pairfd].m_peer_fd = confd;
        sessions[confd].m_peer_fd = pairfd;
        pairing = false;
    }
}
void TcpServer::deal_read(int sockfd)
{
    CLSession* handle = static_cast<CLSession*>(sessions+sockfd);
    handle->m_rw_st = 1;
    thpool->thPoolAddWork(CLSession::process,sessions+sockfd);
    while(1)
    {
        if(handle->m_done)
        {
            if(handle->m_occur_err)
            {
                utils.delfd(m_epollfd,handle->m_self_fd);
                utils.delfd(m_epollfd,handle->m_peer_fd);
                close(handle->m_self_fd);
                close(handle->m_peer_fd);

                auto now = std::chrono::system_clock::now();
                std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

                printf("close fd in line: %d,time:%ld\n",__LINE__,us);
                CLSession::m_session_count -= 2;
            }
            handle->m_done = 0;
            break;
        }
    }
}

void TcpServer::deal_write(int sockfd)
{
    // 写出错，关掉peer
    CLSession* handle = static_cast<CLSession*>(sessions+sockfd);
    handle->m_rw_st = 0;
    thpool->thPoolAddWork(CLSession::process,sessions+sockfd);
    while(1)
    {
        if(handle->m_done)
        {
            if(handle->m_occur_err)
            {
                utils.delfd(m_epollfd,handle->m_self_fd);
                utils.delfd(m_epollfd,handle->m_peer_fd);
                close(handle->m_self_fd);
                close(handle->m_peer_fd);
                printf("close fd in line: %d\n",__LINE__);
                CLSession::m_session_count -= 2;
            }
            handle->m_done = 0;
            break;
        }
    }
}
void TcpServer::Run()
{
    bool timeout = false;
    bool stop_srv = false;

    while (false == stop_srv)
    {
        int nready = epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
        if(nready == -1 && errno != EINTR)
        {
            err_sys(epoll error);
            break;
        }
        
        for(int i = 0;i < nready;i++)
        {
            int tmpfd = events[i].data.fd;
            if(tmpfd == m_listenfd)
            {
                deal_session_connect();
            }else if(tmpfd == m_pipefd[0] && events[i].events & EPOLLIN)
            {
                deal_signal(timeout,stop_srv);
            }else if(events[i].events & EPOLLIN)
            {
                deal_read(tmpfd);
            }else if(events[i].events & EPOLLOUT)
            {
                deal_write(tmpfd);
            }else if(events[i].events & (EPOLLERR|EPOLLHUP))
            {
                utils.delfd(m_epollfd,tmpfd);
                utils.delfd(m_epollfd,sessions[tmpfd].m_peer_fd);
                close(tmpfd);
                close(sessions[tmpfd].m_peer_fd);
                printf("close fd in line: %d\n",__LINE__);
                CLSession::m_session_count -= 2;
            }
        }
    }
    
   
    
}
