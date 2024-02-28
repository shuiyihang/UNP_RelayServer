#ifndef __TCPBASE_H__
#define __TCPBASE_H__

#include <fstream>
#include <string.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/epoll.h>
#include <fcntl.h>
#define err_sys(msg) do { perror(#msg); exit(EXIT_FAILURE); } while (0)
#define err_msg(msg) do { perror(#msg);} while (0)
#define PORT    9877

class tcpBase
{

};

#endif