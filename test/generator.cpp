#include "TcpClient.h"
#include "thread_poll.h"
#include "msgHeader.h"

bool running = true;
pthread_mutex_t op_mutex;
FILE *fp = NULL;

const char* tip_1 = "time: ";
const char* tip_2 = "cont: ";
const char* tip_3 = "T: ";
const char* tip_4 = "R: ";

#define HEAD_LEN    45

#define THREAD_NUMS 30


struct sendForm
{
    int session;
    int talklen;
    TcpClient *clients;
};

void* sendTask(void *arg)
{
    // 定义头的格式
    // <1 s us len>
    sendForm *handle = (sendForm*)arg;
    char *send_string = (char*)malloc(HEAD_LEN + handle->talklen);

    srand(time(NULL));// 使用当前时间作为种子
    char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    while (running)
    {
        int len = rand()%handle->talklen;

        if(len == 0)len = 1;

        timeval timestamp;
        gettimeofday(&timestamp,NULL);
        int targetfd = handle->clients[rand()%(handle->session*2)].m_sockfd;

        int offset = sprintf(send_string,"<%03d %06d %010ld %010ld %010ld>",1,targetfd,timestamp.tv_sec,timestamp.tv_usec,len);// 返回写入的字符数

        len += offset;
        for(int i = offset;i < len;i++)
        {
            send_string[i] = charset[rand() % (sizeof(charset) - 1)];
        }
        send_string[len] = '\0';

        
        write(targetfd,send_string,len);// 阻塞式写
        // fprintf(stdout,"send:%s\n",send_string);
        // sleep(1);
        // usleep(5000);
    }
    if(send_string)
    {
        free(send_string);
        send_string = NULL;
    }
    
}


void recordTask(void *arg)
{
    int fd = *(int*)arg;
    msgHeader header;
    char ceshi[HEAD_LEN];
    read(fd,ceshi,HEAD_LEN);
    int match = sscanf(ceshi,"<%03d %06d %010ld %010ld %010ld>",&header.type,&header.subid,&header.timestamp.tv_sec,&header.timestamp.tv_usec,&header.len);

    char *buff = NULL;
    // printf("======%d========\n",__LINE__);
    if(match == 5 && header.type == 1)
    {
        // printf("correct dump:%s\n",ceshi);
        timeval end;
        gettimeofday(&end,NULL);
        long dbg_us = end.tv_usec - header.timestamp.tv_usec;
        long dbg_s = end.tv_sec - header.timestamp.tv_sec;
        float interval = (float)(dbg_s + dbg_us*1e-6);

        buff = (char*)malloc(header.len+1);

        int len = read(fd,buff,header.len);
        if(len != header.len)
        {
            printf("+++++read failed+++++\n");
        }
        buff[header.len] = '\0';

        // fprintf(stdout,"%s\n",buff);
        // 互斥访问log文件
        pthread_mutex_lock(&op_mutex);
        
        fprintf(fp,"%s%05d %s%05d %s%.5f %s%s\n",tip_3,header.subid,tip_4,fd,tip_1,interval,tip_2,buff);
        fflush(fp);// 防止日志丢失
        
        pthread_mutex_unlock(&op_mutex);

        free(buff);
        // printf("======%d========\n",__LINE__);
    }else{
        printf("error dump:%s\n",ceshi);
    }

    free(arg);
}

int main(int argc,char* argv[])
{

    int session = 5;// 建立的会话数
    int talklen = 50;// 会话长度
    if(argc == 3)
    {
        session = atoi(argv[1]);
        talklen = atoi(argv[2]);
    }else if(argc == 2)
    {
        session = atoi(argv[1]);
    }

    fp = fopen("../../log/record.txt","a+");
    if(fp == NULL)
    {
        err_sys(file open failed);
    }

    TcpClient *clients = new TcpClient[session * 2];
    if(clients == NULL)
    {
        err_sys(session nums too large ~);
    }

    int epollfd = epoll_create(5);
    epoll_event event;
    event.events = EPOLLIN | EPOLLET;

    int ret;
    for(int i = 0;i < session;i++)
    {
        ret = clients[i * 2].Connect();
        if(ret == -1)
        {
            err_sys(conncet error~);
        }else
        {
            event.data.fd = clients[i * 2].m_sockfd;
        }

        epoll_ctl(epollfd,EPOLL_CTL_ADD,clients[i * 2].m_sockfd,&event);

        ret = clients[i * 2 + 1].Connect();
        if(ret == -1)
        {
            err_sys(conncet error~);
        }else
        {
            event.data.fd = clients[i * 2 + 1].m_sockfd;
        }
        epoll_ctl(epollfd,EPOLL_CTL_ADD,clients[i * 2 + 1].m_sockfd,&event);
        // usleep(1000);
    }
    // 主进程接受数据并写入log，安排一个线程不断地从中挑选客户发数据给服务器

    event.data.fd = fileno(stdin);
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fileno(stdin),&event);

    pthread_mutex_init(&op_mutex,NULL);
    pthread_t send_thread;

    sendForm format;
    format.clients = clients;
    format.session = session;
    format.talklen = talklen;

    pthread_create(&send_thread,NULL,sendTask,(void*)&format);

    CLThreadPool thpoll(THREAD_NUMS);

    epoll_event events[100];
    while (running)
    {
        int nready = epoll_wait(epollfd,events,100,-1);
        // printf("client ready:%d\n",nready);
        for(int i = 0; i < nready;i++)
        {
            if(events[i].data.fd == fileno(stdin))
            {
                char exitCode;
                read(fileno(stdin),&exitCode,1);

                epoll_ctl(epollfd,EPOLL_CTL_DEL,fileno(stdin),NULL);
                for(int j = 0;j < session * 2;j++)
                {
                    clients[j].Disconnect("w");
                    epoll_ctl(epollfd,EPOLL_CTL_DEL,clients[j].m_sockfd,NULL);
                }
                running = false;
                // printf("======%d========\n",__LINE__);
                break;
            }else
            {
                // 分给线程池处理
                if(events[i].events & EPOLLIN)
                {
                    int *recvfd = (int*)malloc(sizeof(int));
                    *recvfd = events[i].data.fd;
                    thpoll.thPoolAddWork(recordTask,(void*)recvfd);
                }else if(events[i].events & EPOLLERR || events[i].events & EPOLLHUP)
                {
                    close(events[i].data.fd);
                }
            }
        }
    }
    
    thpoll.thPoolWait();
    pthread_join(send_thread,NULL);
    pthread_mutex_destroy(&op_mutex);
    fclose(fp);
    delete(clients);
    exit(0);
}