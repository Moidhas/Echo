#include <cstring>
#include <string>
#include <unistd.h>
#include <netinet/in.h> 
#include <sys/_types/_socklen_t.h>
#include <sys/socket.h> 
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <vector>

#define PORT 49152

// to make sure the whole message is always sent.
int sendFull(int fd, const char *buff, uint32_t len) {
    while (len > 0) {
        uint32_t bytes = send(fd, buff, len, 0);
        if (bytes <= 0) return bytes;
        buff += bytes;
        len -= bytes;
    }
    return 1;
}

// to make sure the whole message is always read.
int readFull(int fd, char *buff, uint32_t len) {
    while (len > 0) {
        uint32_t bytes = recv(fd, buff, len, 0);
        if (bytes <= 0) return bytes;
        buff += bytes;
        len -= bytes;
    }

    return 1;
}

int sendReq(int connfd, const std::string &req) {
    uint32_t len = htonl(req.size());
    char buff[sizeof(len) + req.size()];
    memcpy(buff, &len, sizeof(len));

    strcpy(&buff[4], req.c_str());
    return sendFull(connfd, buff, sizeof(buff));
}

int readRes(int connfd) {
    uint32_t len = 0;
    int rbytes = recv(connfd, &len, sizeof(uint32_t), 0);
    if (rbytes <= 0) return rbytes;

    len = ntohl(len);
    char buff[len + 1];
    buff[len] = '\0';

    rbytes = readFull(connfd, buff, len);
    if (rbytes <= 0) return rbytes;

    std::cout << "length: " << len << " res: " << buff << std::endl;
    return rbytes;
}

int main() {
    sockaddr_in addr{};
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(PORT);
    addr.sin_family = AF_INET;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return errno;
    }

    int err = connect(fd, (sockaddr *) &addr, sizeof(addr));
    if (err < 0) {
        return errno;
    }

    std::string req;
    while (std::getline(std::cin, req)) {
        if (sendReq(fd, req) <= 0) return -1;
        if (readRes(fd) <= 0) return -1;
    }

    close(fd);
}
