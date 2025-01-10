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
int read_full(int fd, char *buff, uint32_t len); 
int send_full(int fd, const char *buff, uint32_t len);
#endif

