#include "TcpClient.h"

#define MAXLEN 1024
int main()
{
    int n;
    int stdineof = 0;
    char sendline[MAXLEN],recvline[MAXLEN+1];

    TcpClient client("127.0.0.1",9877);
    if(client.Connect() < 0)
    {
        err_sys(connect error);
    }

    // 使用epoll
    int epollfd = epoll_create(5);
    epoll_event events[10];

    epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = fileno(stdin);
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fileno(stdin),&event);
    event.data.fd = client.m_sockfd;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,client.m_sockfd,&event);

    while (1)
    {
       int nready = epoll_wait(epollfd,events,10,-1);

    //    printf("client ready:%d\n",nready);
       if(nready == -1)
       {
        perror("epoll_wait");
        close(epollfd);
        exit(1);
       }
       for(int i = 0;i < nready;i++)
       {
        if(events[i].data.fd == fileno(stdin))
        {
            if((n = read(fileno(stdin),sendline,MAXLEN)) == 0)
            {
                client.Disconnect("w");
                stdineof = 1;
                epoll_ctl(epollfd,EPOLL_CTL_DEL,fileno(stdin),NULL);
                continue;
            }
            // printf("terminal:%s\n",sendline);
            client.Send(sendline,n);
        }else
        {
            if((n = client.Receive(recvline,MAXLEN)) == 0)
            {
                if(stdineof == 1)return 0;
                else
                {
                    err_sys(server terminated prematurely~);
                }
            }
            write(fileno(stdout),recvline,n);
        }

       }
    }
    
    return 0;
}