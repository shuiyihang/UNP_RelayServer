#include "generator.h"

const char* tip_1 = "time: ";
const char* tip_2 = "cont: ";
const char* tip_3 = "T: ";
const char* tip_4 = "R: ";


#define HEAD_LEN    14

#define THR_NUMS 30

#define FORMAT_LEN  38 // 格式头
#define TAIL_LEN    1   //+换行




#ifdef ONE_SEND_DEBUG
#define MAX_OUT_NUMS    1
#else
#define MAX_OUT_NUMS    10          // 100 not work!!
#endif


struct sendForm
{
    int session;
    int talklen;
    TcpClient *clients;
};

int CLGenerator::m_out_client_nums = 0;

int CLGenerator::print_pos = 0;

void CLGenerator::init(int session,int talklen)
{
    m_sessions = session;
    m_talklen = talklen;
    m_running = true;
    m_norm_state = false;
    no_response_times = 0;
    m_w_file_pos = 0;
    srand(time(NULL));// 使用当前时间作为种子
    memset(m_history_clients,0,sizeof(int)*HISTORY_SIZE);
    m_test_nums = 0;

    pthread_mutex_init(&m_op_mutex,NULL);

    m_fp = fopen("../../log/record.txt","w+");
    if(m_fp == NULL)
    {
        err_sys(record.txt file open failed);
    }

    if(stat("../../data/data.txt",&m_src_file_stat) < 0)
    {
        err_sys(stat data.txt failed);
    }
    m_data_fd = open("../../data/data.txt",O_RDONLY);
    m_data_src_addr = (char*)mmap(NULL,m_src_file_stat.st_size,PROT_READ,MAP_PRIVATE,m_data_fd,0);

    if(MAP_FAILED == m_data_src_addr)
    {
        err_sys(mmap failed);
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

    utils.addfd(m_epollfd,m_pipefd[0],EPOLLIN);

    utils.u_pipefd = m_pipefd;
    // printf("recv fd:%d,send fd:%d\n",m_pipefd[0],m_pipefd[1]);
    
    utils.addsig(SIGPIPE,true,SIG_IGN);
    utils.addsig(SIGINT,false);
    utils.addsig(SIGALRM,false);

    m_thpool = new CLThreadPool(THR_NUMS);
    
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
            utils.addfd(m_epollfd,m_clients[i * 2].m_sockfd,EPOLLIN);//syh
            // utils.addfd(m_epollfd,m_clients[i * 2].m_sockfd,EPOLLIN|EPOLLONESHOT);
            // printf("sockfd:%d,idx:%d\n",m_clients[i * 2].m_sockfd,i*2);
        }

        ret = m_clients[i * 2 + 1].Connect();
        if(ret == -1)
        {
            err_sys(conncet error~);
        }else
        {
            m_map_table[m_clients[i * 2 + 1].m_sockfd] = i*2 + 1;
            utils.addfd(m_epollfd,m_clients[i * 2 + 1].m_sockfd,EPOLLIN);//syh
            // utils.addfd(m_epollfd,m_clients[i * 2 + 1].m_sockfd,EPOLLIN|EPOLLONESHOT);
            // printf("sockfd:%d,idx:%d\n",m_clients[i * 2+1].m_sockfd,i*2+1);
        }
        // usleep(1000);
    }
}

void CLGenerator::run()
{
    bool timeout = false;
    
    alarm(TIMESLOT);
    while (m_running)
    {
        sendTask(this);
        int nready = epoll_wait(m_epollfd,m_events,MAX_EVENT_NUMBER,-1);
        for(int i = 0; i < nready;i++)
        {
            int tmpfd = m_events[i].data.fd;
            if(tmpfd == m_pipefd[0] && m_events[i].events & EPOLLIN)
            {
                // printf("deal_signal\n");
                deal_signal(timeout);
            }else if(m_events[i].events & EPOLLIN)
            {
                deal_read(tmpfd);
                m_norm_state = true;
            }else if(m_events[i].events & EPOLLOUT)
            {
                // printf("trigger sockfd:%d EPOLLOUT\n",tmpfd);
                deal_write(tmpfd);
                m_norm_state = true;// 有数据正常发送接收
            }else if(m_events[i].events & (EPOLLERR|EPOLLHUP))
            {
                utils.delfd(m_epollfd,tmpfd);
                close(tmpfd);
                printf("close fd in line: %d\n",__LINE__);
            }
        }

        if(timeout)
        {
            if(false == m_norm_state)
            {
                no_response_times++;
                if(no_response_times > 10)
                {
                    m_running = false;
                }
                auto now = std::chrono::system_clock::now();
                std::time_t now_time = std::chrono::system_clock::to_time_t(now);
                struct std::tm *local_time = std::localtime(&now_time);

                printf("%02d-%02d %02d:%02d:%02d: no data in/out,code seem enter while state\n",local_time->tm_mon + 1,
                        local_time->tm_mday,
                        local_time->tm_hour,
                        local_time->tm_min,
                        local_time->tm_sec);
            }
            m_norm_state = false;
            timeout = false;
            no_response_times = 0;
            alarm(TIMESLOT);
        }
        
    }

    m_thpool->thPoolWait();
    pthread_mutex_destroy(&m_op_mutex);
    m_thpool->thPoolDestroy();
    fclose(m_fp);

}

void CLGenerator::sendTask(void *arg)
{
    CLGenerator *handle = (CLGenerator*)arg;

    // while (handle->m_running)
    {
        int client_idx = rand()%(handle->m_sessions*2);
        timeval timestamp;
        
        int rand_ret = 0;
        for(int i = 0;i < HISTORY_SIZE;i++)
        {
            if(client_idx == handle->m_history_clients[i])
            {
                rand_ret = 1;
                break;
            }
        }

        // TODO会话数太少可能挑不出来
        if(rand_ret == 1)
        {
            return;
        }

        if(handle->m_clients[client_idx].m_io_state == 0)
        {
            if(m_out_client_nums < MAX_OUT_NUMS)
            {
                print_pos = 0;
                handle->m_history_clients[handle->m_test_nums%HISTORY_SIZE] = client_idx;
                handle->m_test_nums++;
                

                char* w_buf = handle->m_clients[client_idx].m_write_buf;
                gettimeofday(&timestamp,NULL);

                long now_us = timestamp.tv_sec*1e6 + timestamp.tv_usec;

                // 标识头部(2字节)|有效数据长度(hh,ll,hh,ll)|时间戳(hh,ll,hh,ll)|[数据]
                w_buf[0] = 0xEE;
                w_buf[1] = 0xFF;

                w_buf[2] = handle->m_talklen&0xFF;
                w_buf[3] = (handle->m_talklen>>8)&0xFF;
                w_buf[4] = (handle->m_talklen>>16)&0xFF;
                w_buf[5] = (handle->m_talklen>>24)&0xFF;

                w_buf[6] =  now_us&0xFF;
                w_buf[7] =  (now_us>>8)&0xFF;
                w_buf[8] =  (now_us>>16)&0xFF;
                w_buf[9] =  (now_us>>24)&0xFF;
                w_buf[10] = (now_us>>32)&0xFF;
                w_buf[11] = (now_us>>40)&0xFF;
                w_buf[12] = (now_us>>48)&0xFF;
                w_buf[13] = (now_us>>56)&0xFF;
               
                handle->m_clients[client_idx].m_write_idx = HEAD_LEN;

                handle->m_clients[client_idx].m_iv[0].iov_base = handle->m_clients[client_idx].m_write_buf;
                handle->m_clients[client_idx].m_iv[0].iov_len = HEAD_LEN;

                handle->m_clients[client_idx].m_iv[1].iov_base = handle->m_data_src_addr;
                handle->m_clients[client_idx].m_iv[1].iov_len = handle->m_talklen;


                auto now = std::chrono::system_clock::now();
                auto t_now_us = std::chrono::time_point_cast<std::chrono::microseconds>(now);
                auto us = t_now_us.time_since_epoch().count();
                // printf("time:%ld EPOLLOUT wait sockfd:%d m_test_nums:%ld\n",us,handle->m_clients[client_idx].m_sockfd,handle->m_test_nums);
                
                m_out_client_nums++;
                handle->m_clients[client_idx].m_io_state = 1;
                handle->utils.modfd(handle->m_epollfd,handle->m_clients[client_idx].m_sockfd,EPOLLOUT);
                if(handle->m_pipefd[0] == handle->m_clients[client_idx].m_sockfd)
                {
                    printf("errro!!!!\n");
                }
                // printf("wait sockfd:%d EPOLLOUT\n",handle->m_clients[client_idx].m_sockfd);
            }
        }
    }
    
}


void CLGenerator::writeTask(void *arg)
{
    msgHeader header;
    arg_struct* param = static_cast<arg_struct*>(arg);

    CLGenerator* handle = param->trig;
    TcpClient* client = handle->m_clients + handle->m_map_table[param->fd];

    int bytes_have_send = 0;
    int bytes_to_send = client->m_iv[0].iov_len + client->m_iv[1].iov_len;
    while (1)
    {
        int w_bytes = writev(param->fd,client->m_iv,2);
        if(w_bytes < 0)
        {
            if(errno == EAGAIN)
            {
                printf("sockfd:%d EPOLLOUT line:%d\n",param->fd,__LINE__);
                handle->utils.modfd(handle->m_epollfd,param->fd,EPOLLOUT);
            }else
            {
                client->m_occur_err = true;
            }
            break;
        }

        bytes_have_send += w_bytes;
        bytes_to_send -= w_bytes;
        if(bytes_have_send >= client->m_iv[0].iov_len)
        {
            client->m_iv[0].iov_len = 0;
            client->m_iv[1].iov_base = handle->m_data_src_addr + (bytes_have_send-client->m_write_idx);
            client->m_iv[1].iov_len = client->m_iv[1].iov_len - (bytes_have_send-client->m_write_idx);
        }else
        {
            client->m_iv[0].iov_base = client->m_write_buf + bytes_have_send;
            client->m_iv[0].iov_len = client->m_iv[0].iov_len - bytes_have_send;
        }
        if(bytes_to_send <= 0)
        {

#ifndef  ONE_SEND_DEBUG
            client->m_io_state = 0;
            m_out_client_nums--;
#endif
            handle->utils.modfd(handle->m_epollfd,param->fd,EPOLLIN);

            auto now = std::chrono::system_clock::now();
            auto t_now_us = std::chrono::time_point_cast<std::chrono::microseconds>(now);
            auto us = t_now_us.time_since_epoch().count();

            // printf("time:%ld EPOLLIN sockfd:%d write done!\n",us,param->fd);
            break;
        }

    }
    client->m_done = true;
    
}

long bytes_to_long(const unsigned char *buff)
{
    long ret = 0;
    for(int i = 0;i < 8;i++)
    {
        ret |= ((long)buff[i]) << (8*i);
    }
    return ret;
}
void CLGenerator::recordTask(void *arg)
{
    msgHeader header;
    arg_struct* param = static_cast<arg_struct*>(arg);

    CLGenerator* handle = param->trig;
    TcpClient* client = handle->m_clients + handle->m_map_table[param->fd];

    int offset = 0;
    // 读一个缓冲数据
    size_t r_len;
    if(client->m_data_len > 0)
    {
        r_len = (client->m_data_len > READ_BUFFER_SIZE)?READ_BUFFER_SIZE:client->m_data_len;
    }else
    {
        r_len = READ_BUFFER_SIZE;
    }
    if(false == client->read_data(r_len))
    {
        client->m_done = true;
        client->m_occur_err = true;
        printf("CLGenerator::recordTask err %d\n",__LINE__);
        return;
    }

    char* r_buf;
    int deal_idx = 0;

    while(deal_idx < client->m_read_idx)
    {
        r_buf = client->m_read_buf + deal_idx;

        if(0 == client->m_wait_pure_data)
        {
            // 先解析头部
            unsigned char* h_buf = (unsigned char*)r_buf;
            if(0xEE == h_buf[0] && 0xFF == h_buf[1])
            {
                client->m_data_len = (h_buf[5]<<24)|(h_buf[4]<<16)|(h_buf[3]<<8)|h_buf[2];
                client->m_start_time_us = bytes_to_long(h_buf+6);
                client->m_wait_pure_data = 1;

                deal_idx += HEAD_LEN;
#ifdef ENABLE_CONT_SAVE
                // 互斥访问log文件
                pthread_mutex_lock(&handle->m_op_mutex);
                client->m_w_format_pos = handle->m_w_file_pos;
                client->m_w_file_pos = handle->m_w_file_pos + FORMAT_LEN;
                handle->m_w_file_pos += (FORMAT_LEN + client->m_data_len + TAIL_LEN);// 获取一个写的独占区域 FORMAT_LEN:标准格式提示
                pthread_mutex_unlock(&handle->m_op_mutex);
#endif
                // printf("match head,m_data_len:%d\n",client->m_data_len);
            }else
            {
                printf("parase head err\n");
                for(int i = 0;i < 30;i++)
                {
                    printf("%x ",h_buf[i]);
                }
                printf("\n");
                break;
            }
        }else
        {
            if(client->m_data_len > 0)
            {

                int need_len = (client->m_data_len > (client->m_read_idx - deal_idx)) ? (client->m_read_idx - deal_idx):client->m_data_len;

#ifdef ENABLE_CONT_SAVE

                pthread_mutex_lock(&handle->m_op_mutex);

                fseek(handle->m_fp,client->m_w_file_pos,SEEK_SET);
                fwrite(r_buf,sizeof(char),need_len,handle->m_fp);


                // printf("clinet:%d capture data\n",param->fd);
                // for(int i = 0;i < 60;i++)
                // {
                //     printf("%x ",r_buf[i]);
                // }
                // printf("\n");

                client->m_w_file_pos += need_len;
                client->m_data_len -= need_len;
                deal_idx += need_len;


                if(client->m_data_len <= 0)
                {
                    fprintf(handle->m_fp,"\n");
                    // 读完，加上标准格式提示
                    fseek(handle->m_fp,client->m_w_format_pos,SEEK_SET);
                    timeval end;
                    gettimeofday(&end,NULL);
                    long dbg_us = end.tv_sec*1e6 + end.tv_usec - client->m_start_time_us;
                    float interval = (float)(dbg_us*1e-6);
                    // T: 00010 R: 00011 time: 0.27802 cont:
                    fprintf(handle->m_fp,"%s%05d %s%05d %s%.5f %s",tip_3,1,tip_4,2,\
                                                            tip_1,interval,tip_2);
                    client->m_wait_pure_data = 0;
                }

                pthread_mutex_unlock(&handle->m_op_mutex);
#else
                client->m_data_len -= need_len;
                deal_idx += need_len;
                if(client->m_data_len <= 0)
                {
                    client->m_wait_pure_data = 0;
                }
#endif
            }

        }
    }

#ifdef ENABLE_CONT_SAVE
    fflush(handle->m_fp);// 防止日志丢失
#endif
    client->m_done = true;
}


void CLGenerator::deal_write(int sockfd)
{
    arg_struct param;
    param.fd = sockfd;
    param.trig = this;
    m_thpool->thPoolAddWork(CLGenerator::writeTask,&param);
    TcpClient* client = m_clients + m_map_table[sockfd];
    while (1)
    {
        if(client->m_done == true)
        {
            if(client->m_occur_err)
            {
                utils.delfd(m_epollfd,client->m_sockfd);
                close(client->m_sockfd);
            }
            client->m_done = false;
            break;
        }
    }
    
}
void CLGenerator::deal_read(int sockfd)
{
    arg_struct param;
    param.fd = sockfd;
    param.trig = this;
    m_thpool->thPoolAddWork(CLGenerator::recordTask,&param);
    TcpClient* client = m_clients + m_map_table[sockfd];
    // printf("detect sockfd:%d data in\n",sockfd);
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
                    // printf("time out!!!\n");
                    break;
                }
                case SIGINT:
                {
                    m_running = false;
                    break;
                }
            }
        }
    }
    return true;
}

