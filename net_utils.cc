#include "net_utils.h"

int send_full(int fd, const char *buff, uint32_t len) {
    // to make sure the whole message is always sent.
    while (len > 0) {
        uint32_t bytes = send(fd, buff, len, 0);
        if (bytes <= 0) return bytes;
        buff += bytes;
        len -= bytes;
    }
    return 1;
}

int read_full(int fd, char *buff, uint32_t len) {
    // to make sure the whole message is always read.
    while (len > 0) {
        uint32_t bytes = recv(fd, buff, len, 0);
        if (bytes <= 0) return bytes;
        buff += bytes;
        len -= bytes;
    }

    return 1;
}

int send_msg(int connfd, const std::string &msg) {
    uint32_t len = htonl(msg.size());
    char buffer[sizeof(uint32_t)];
    memcpy(buffer, &len, sizeof(uint32_t));

    int hasClosed = send_full(connfd, buffer, sizeof(uint32_t));
    if (hasClosed <= 0) return hasClosed;

    hasClosed = send_full(connfd, msg.c_str(), msg.size());
    if (hasClosed <= 0) return hasClosed;

    return 1;
}

int read_msg(int connfd) {
    uint32_t len = 0;
    int hasClosed = recv(connfd, &len, sizeof(uint32_t), 0);
    if (hasClosed <= 0) return hasClosed;

    len = ntohl(len);
    char buff[len + 1];
    buff[len] = '\0';


    hasClosed = read_full(connfd, buff, len);
    if (hasClosed <= 0) return hasClosed;

    std::cout << "msg: " << buff << std::endl;
    return 1;
}

