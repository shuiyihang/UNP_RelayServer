#include "utils.h"
#include <signal.h>
#include <cstring>
#include <stdio.h>

int* CLUtils::u_pipefd = nullptr;

void CLUtilsBase::setnonblocking(int fd)
{
    int flags = fcntl(fd,F_GETFL);
    int option = flags | O_NONBLOCK;
    fcntl(fd,F_SETFL,option);
}
void CLUtilsBase::modfd(int epollfd,int fd,unsigned int ev)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLRDHUP;//syh
    // event.events = ev|EPOLLRDHUP|EPOLLONESHOT;
    epoll_ctl(epollfd,EPOLL_CTL_MOD,fd,&event);
}
void CLUtilsBase::addfd(int epollfd,int fd,unsigned int events)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}
void CLUtilsBase::delfd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}
void CLUtils::sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    int ret = send(u_pipefd[1],(char*)&msg,1,0);
    errno = save_errno;
    printf("sig:%d,ret:%d,fd:%d\n",sig,ret,u_pipefd[1]);
}

void CLUtils::addsig(int sig,bool restart,sig_func handle)
{
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handle;
    if(restart)
    {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL) != -1);
}