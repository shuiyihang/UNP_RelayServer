#include "generator.h"

const char* tip_1 = "time: ";
const char* tip_2 = "cont: ";
const char* tip_3 = "T: ";
const char* tip_4 = "R: ";

#define HEAD_LEN    45

#define THR_NUMS 30


struct sendForm
{
    int session;
    int talklen;
    TcpClient *clients;
};

void CLGenerator::init(int session,int talklen)
{
    m_sessions = session;
    m_talklen = talklen;
    m_running = true;

    m_fp = fopen("../../log/record.txt","w+");
    if(m_fp == NULL)
    {
        err_sys(file open failed);
    }

    m_clients = new TcpClient[m_sessions * 2];
    if(m_clients == NULL)
    {
        err_sys(session nums too large ~);
    }

    m_epollfd = epoll_create(5);

    int ret;
    ret = socketpair(PF_UNIX,SOCK_STREAM,0,m_pipefd);
    assert(ret != -1);
    utils.setnonblocking(m_pipefd[1]);
    utils.addfd(m_epollfd,m_pipefd[0],EPOLLIN | EPOLLRDHUP);
    utils.addsig(SIGPIPE,true,SIG_IGN);
    utils.addsig(SIGTERM,false);
    utils.u_pipefd = m_pipefd;

    m_thpool = new CLThreadPool(THR_NUMS);
    pthread_mutex_init(&m_op_mutex,NULL);
}

void CLGenerator::build_contact()
{
    int ret;
    for(int i = 0;i < m_sessions;i++)
    {
        ret = m_clients[i * 2].Connect();
        if(ret == -1)
        {
            err_sys(conncet error~);
        }else
        {
            m_map_table[m_clients[i * 2].m_sockfd] = i*2;
            utils.addfd(m_epollfd,m_clients[i * 2].m_sockfd,EPOLLIN | EPOLLONESHOT);
            // printf("sockfd:%d,idx:%d\n",m_clients[i * 2].m_sockfd,i*2);
        }

        ret = m_clients[i * 2 + 1].Connect();
        if(ret == -1)
        {
            err_sys(conncet error~);
        }else
        {
            m_map_table[m_clients[i * 2 + 1].m_sockfd] = i*2 + 1;
            utils.addfd(m_epollfd,m_clients[i * 2 + 1].m_sockfd,EPOLLIN | EPOLLONESHOT);
            // printf("sockfd:%d,idx:%d\n",m_clients[i * 2+1].m_sockfd,i*2+1);
        }
        // usleep(1000);
    }
}

void CLGenerator::run()
{
    bool timeout = false;
    
    pthread_t send_thread;
    pthread_create(&send_thread,NULL,sendTask,this);

    while (m_running)
    {
        int nready = epoll_wait(m_epollfd,m_events,100,-1);
        for(int i = 0; i < nready;i++)
        {
            int tmpfd = m_events[i].data.fd;
            if(tmpfd == m_pipefd[0] && m_events[i].events & EPOLLIN)
            {
                deal_signal(timeout);
            }else if(m_events[i].events & EPOLLIN)
            {
                deal_read(tmpfd);
            }else if(m_events[i].events & (EPOLLERR|EPOLLHUP))
            {
                utils.delfd(m_epollfd,tmpfd);
                close(tmpfd);
                printf("close fd in line: %d\n",__LINE__);
            }
        }
    }

    m_thpool->thPoolWait();
    pthread_join(send_thread,NULL);
    pthread_mutex_destroy(&m_op_mutex);
    m_thpool->thPoolDestroy();
    fclose(m_fp);

}

void* CLGenerator::sendTask(void *arg)
{
    // 定义头的格式
    // <1 s us len>
    CLGenerator *handle = (CLGenerator*)arg;
    char *send_string = (char*)malloc(HEAD_LEN + handle->m_talklen);

    srand(time(NULL));// 使用当前时间作为种子
    char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    while (handle->m_running)
    {
        int len = rand()%handle->m_talklen;

        if(len == 0)len = 1;

        timeval timestamp;
        gettimeofday(&timestamp,NULL);
        int targetfd = handle->m_clients[rand()%(handle->m_sessions*2)].m_sockfd;

        int offset = sprintf(send_string,"<%03d %06d %010ld %010ld %010ld>",1,targetfd,timestamp.tv_sec,timestamp.tv_usec,len);// 返回写入的字符数

        len += offset;
        for(int i = offset;i < len;i++)
        {
            send_string[i] = charset[rand() % (sizeof(charset) - 1)];
        }
        send_string[len] = '\0';

        
        write(targetfd,send_string,len);// TODO
        // fprintf(stdout,"send:%s\n",send_string);
        // sleep(1);
        usleep(5000);
    }
    if(send_string)
    {
        free(send_string);
        send_string = NULL;
    }
    
}
void CLGenerator::recordTask(void *arg)
{
    msgHeader header;
    arg_struct* param = static_cast<arg_struct*>(arg);

    CLGenerator* handle = param->trig;
    TcpClient* client = handle->m_clients + handle->m_map_table[param->fd];

    int offset = 0;
    // 读取所有数据
    if(false == client->read_data())
    {
        client->m_done = true;
        client->m_occur_err = true;
        printf("err %d\n",__LINE__);
        return;
    }
    // 互斥访问log文件
    pthread_mutex_lock(&handle->m_op_mutex);

    // 解析，记录
    while(5 == sscanf(client->m_read_buf+offset,"<%03d %06d %010ld %010ld %010ld>",\
                &header.type,&header.subid,\
                &header.timestamp.tv_sec,&header.timestamp.tv_usec,\
                &header.len))
    {

        timeval end;
        gettimeofday(&end,NULL);
        long dbg_us = end.tv_usec - header.timestamp.tv_usec;
        long dbg_s = end.tv_sec - header.timestamp.tv_sec;
        float interval = (float)(dbg_s + dbg_us*1e-6);

        
        memcpy(client->m_write_buf,client->m_read_buf+HEAD_LEN+offset,header.len);
        client->m_write_buf[header.len] = '\0';
        fprintf(handle->m_fp,"%s%05d %s%05d %s%.5f %s%s\n",tip_3,header.subid,tip_4,param->fd,\
                                                tip_1,interval,tip_2,client->m_write_buf);
        offset += (HEAD_LEN + header.len);
    }
    fflush(handle->m_fp);// 防止日志丢失
    pthread_mutex_unlock(&handle->m_op_mutex);
    client->m_done = true;
    handle->utils.modfd(handle->m_epollfd,client->m_sockfd,EPOLLIN|EPOLLONESHOT);
}


void CLGenerator::deal_read(int sockfd)
{
    arg_struct param;
    param.fd = sockfd;
    param.trig = this;
    m_thpool->thPoolAddWork(CLGenerator::recordTask,&param);
    TcpClient* client = m_clients + m_map_table[sockfd];
    while (1)
    {
        if(client->m_done == true)
        {
            if(client->m_occur_err)
            {
                utils.delfd(m_epollfd,client->m_sockfd);
                close(client->m_sockfd);
                
                auto now = std::chrono::system_clock::now();
                std::time_t now_c = std::chrono::system_clock::to_time_t(now);
                auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();

                printf("close fd in line: %d,time:%ld\n",__LINE__,us);
            }
            client->m_done = false;
            // printf("end work\n");
            break;
        }
    }
    
}

bool CLGenerator::deal_signal(bool& timeout)
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
                    m_running = false;
                    break;
                }
            }
        }
    }
    return true;
}

