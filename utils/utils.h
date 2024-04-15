#ifndef __UTILS_H
#define __UTILS_H
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>

typedef void(*sig_func)(int);

class CLUtilsBase
{
public:
    void addfd(int epollfd,int fd,unsigned int events);
    void modfd(int epollfd,int fd,unsigned int ev);
    void delfd(int epollfd,int fd);
    void setnonblocking(int fd);
private:
};


class CLUtils:public CLUtilsBase
{
public:
    static int* u_pipefd;
private:

public:
    void addsig(int sig,bool restart=true,sig_func handle=sig_handler);
    
private:
    static void sig_handler(int sig);
};

#endif