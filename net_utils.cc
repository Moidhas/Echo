#include "net_utils.h"

namespace  {

int send_full(int fd, const char *buff, uint32_t len) {
    while (len > 0) {
        uint32_t bytes = send(fd, buff, len, 0);
        if (bytes <= 0) return -1;
        buff += bytes;
        len -= bytes;
    }
    return 0;
}

int read_full(int fd, char *buff, uint32_t len) {
    while (len > 0) {
        uint32_t bytes = recv(fd, buff, len, 0);
        if (bytes <= 0) return -1;
        len -= bytes;
        buff += bytes;
    }

    return 0;
}

}

int send_msg(int fd, const std::string &msg) {
    uint32_t len = htonl(msg.size());
    char buffer[sizeof(uint32_t)];
    memcpy(buffer, &len, sizeof(uint32_t));
    if (send_full(fd, buffer, sizeof(uint32_t)) < 0 || send_full(fd, msg.c_str(), msg.size()) < 0) return -1;
    return 0;
}

int read_msg(int fd) {
    uint32_t len = 0;
    if (recv(fd, &len, sizeof(uint32_t), 0) <= 0) return -1;
    len = ntohl(len);
    char buff[len + 1];
    buff[len] = 0;
    if (read_full(fd, buff, len) < 0) return -1;

    std::cout << "msg: " << buff << std::endl;
    return 0;
}

