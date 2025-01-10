#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h> 
#include <iostream>
#include <cstdint>
#include <netinet/in.h>
#include <sys/_types/_socklen_t.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/event.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <fcntl.h>
#include <cstring>
#include <errno.h>

#define PORT 49152
#define BACKLOG 5

static void die(const char *msg) {
    fprintf(stderr, "[%d] %s\n", errno, msg);
    abort();
}

struct Conn {
    int fd = -1;
    uint32_t len = 0;
    char *buffer = nullptr;

    Conn(int fd): fd{fd} {}
    Conn() {}

    void setBuffer(char *buff) {
        buffer = buff;
        start = buffer;
    }

    void clearBuffer() {
        delete []start;
        setBuffer(nullptr);
        len = 0;
    }

    ~Conn() {
        delete[] start;
    }

private: 
    char *start = nullptr;
};

int handleRead(Conn &conn, int bytes) {
    int rbytes = 0;
    if (conn.len ==  0) {
        if (bytes < 4) {
            return rbytes; 
        } else {
            rbytes = recv(conn.fd, &conn.len, sizeof(conn.len), 0);
            if (rbytes < 0) return rbytes;
            conn.len = ntohl(conn.len);
            conn.setBuffer(new char[conn.len]);
        }
    } 

    if (bytes - rbytes !=  0) {
        int rbyte = recv(conn.fd, conn.buffer, conn.len, 0);
        if (rbyte < 0) return rbyte;

        if (rbyte == conn.len) {
            std::cout << "msg: " << conn.buffer << std::endl;
            conn.clearBuffer();
        }

        rbytes += rbyte;
    }

    return rbytes;
}

int main() {
    int ret;
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(PORT);
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    if (bind(listenfd, (sockaddr *) &addr, sizeof(addr)) < 0) {
        die("bind()");
    }
    
    listen(listenfd, BACKLOG);
    char ipstr[INET_ADDRSTRLEN];

    int kq = kqueue();
    if (kq < 0) return - 1; 


    std::unordered_map<int, Conn> conns;
    struct kevent evSet; 
    struct kevent tevent;

    EV_SET(&evSet, listenfd, EVFILT_READ, EV_ADD, 0, BACKLOG, NULL);
    ret = kevent(kq, &evSet, 1, NULL, 0, NULL);
    if (ret < 0) die("kevent()");

    while (true) {
        ret = kevent(kq, NULL, 0, &tevent, 1, NULL);
        if (ret <= 0) {
            die("kevent()");
        } else {
            if (tevent.flags & EV_EOF) {
                // can still have pending data in the buffer. 
                int rbytes = handleRead(conns[tevent.ident], tevent.data);
                if (rbytes < 0) die("handleRead()");
                conns.erase(tevent.ident);
                close(tevent.ident);
            }

            if (tevent.ident == listenfd && tevent.filter == EVFILT_READ) {
                int connfd = accept(listenfd, NULL, NULL);
                if (connfd < 0) die("accept()");
                fcntl(connfd, F_SETFL, O_NONBLOCK);
                conns[connfd] = Conn{connfd};

                EV_SET(&evSet, connfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                ret = kevent(kq, &evSet, 1, NULL, 0, NULL);
                if (ret < 0) die("kevent()");
            } else if (tevent.filter == EVFILT_READ) {
                int rbytes = handleRead(conns[tevent.ident], tevent.data);
                if (rbytes < 0)  die("handleRead()");
            }
        }


    }

    close(listenfd);
}
