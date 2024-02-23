#include "TcpServer.h"


TcpServer::TcpServer(const unsigned short port)
{
    sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    m_listenfd = socket(AF_INET,SOCK_STREAM,0);
    bind(m_listenfd,(sockaddr*)&addr,sizeof(addr));
    listen(m_listenfd,10);

    m_epollfd = epoll_create(5);

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = m_listenfd;
    epoll_ctl(m_epollfd,EPOLL_CTL_ADD,m_listenfd,&event);
    setnonblocking(m_listenfd);

    thpool = new CLThreadPool(THR_NUMS);
    pthread_mutex_init(&mutex,NULL);

}

void TcpServer::Run()
{

    while (1)
    {
        int nready = epoll_wait(m_epollfd,events,MAX_EVENT_NUMBER,-1);
        printf("server ready:%d\n",nready);
        if(nready == -1)
        {
            err_sys(epoll error);
            break;
        }
        
        for(int i = 0;i < nready;i++)
        {
            int tmpfd = events[i].data.fd;
            if(tmpfd == m_listenfd)
            {
                // 接受新的连接
                sockaddr_in conaddr;
                socklen_t len = sizeof(conaddr);
                char hostname[128];

                epoll_event event;
                int confd = accept(tmpfd,(sockaddr*)&conaddr,&len);
                
                event.events = EPOLLIN | EPOLLET;// 改用边缘触发模式
                event.data.fd = confd;

                pthread_mutex_lock(&mutex);
                epoll_ctl(m_epollfd,EPOLL_CTL_ADD,confd,&event);
                pthread_mutex_unlock(&mutex);

                setnonblocking(confd);
                printf("connect from %s,port %d\n",inet_ntop(AF_INET,&conaddr.sin_addr,hostname,sizeof(hostname)),\
                        ntohs(conaddr.sin_port));
            }else
            {
                // 处理接收到的数据
                if(events[i].events & EPOLLIN)
                {
                    argStruct arg;
                    arg.srv = this;
                    arg.fd = tmpfd;
                    // dealwithread(&arg);

                    thpool->thPoolAddWork(dealwithread,(void*)&arg);
                }
            }
        }
    }
    
   
    
}

void TcpServer::dealwithread(void* arg)
{
    #define MAXLEN 1024
    char buff[MAXLEN];
    argStruct *info = (argStruct*)arg;
    int fd = info->fd;
    pthread_mutex_t mutex = info->srv->mutex;
    int m_epollfd = info->srv->m_epollfd;
    
    // printf("======%d========\n",__LINE__);
    // 读数据，然后写回客户
    int n;
    if((n = read(fd,buff,MAXLEN)) < 0)
    {
        if(errno == ECONNRESET)
        {
            printf("======close fd:%d========\n",fd);
            close(fd);
            pthread_mutex_lock(&mutex);
            epoll_ctl(m_epollfd,EPOLL_CTL_DEL,fd,NULL);
            pthread_mutex_unlock(&mutex);
        }else{
            err_sys(read error~);
        }
    }else if(n == 0)
    {
        printf("======close fd:%d========\n",fd);
        close(fd);
        pthread_mutex_lock(&mutex);
        epoll_ctl(m_epollfd,EPOLL_CTL_DEL,fd,NULL);
        pthread_mutex_unlock(&mutex);
    }else{
        // printf("======%d========\n",__LINE__);
        write(fd,buff,n);
 
    }
}

void TcpServer::setnonblocking(int fd)
{
    int flags = fcntl(fd,F_GETFL);
    int option = flags | O_NONBLOCK;
    fcntl(fd,F_SETFL,option);
}