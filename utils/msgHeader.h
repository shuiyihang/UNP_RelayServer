#ifndef __MSG_HEADER_H__
#define __MSG_HEADER_H__
#include <sys/time.h>


// 字节对齐？？
struct msgHeader
{
    int type;// 1-字符
    int subid;// 发送者id 5位
    timeval timestamp;//时间戳 字符最长20位
    long len;// 字符长度 10位
};


#endif