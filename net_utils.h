#include <cstring>
#include <unistd.h>
#include <netinet/in.h> 
#include <sys/_types/_socklen_t.h>
#include <sys/socket.h> 
#include <iostream>

#ifndef __NET_UTILS_H__
#define __NET_UTILS_H__

int send_msg(int fd, const std::string &msg); 
int read_msg(int fd); 

#endif

