#include <unistd.h>
#include <iostream>
#include <netdb.h>
#include <sys/event.h>
#include <unordered_map>
#include <fcntl.h>
#include <errno.h>
#include <cstring>

#define PORT 49152
#define BACKLOG 5

static void die(const char *msg) {
    fprintf(stderr, "[%d] %s\n", errno, msg);
    abort();
}

struct Conn {
    int fd = -1;
    uint32_t len = 0;
    uint32_t posn = 0;
    char *buffer = nullptr;

    Conn(int fd): fd{fd} {}
    Conn() {}

    void newBuffer(uint32_t length) {
        delete[] start;

        len = length;
        posn = 0;

        buffer = new char[len + 1];
        buffer[len] = '\0';
        start = buffer;
    }

    void clearBuffer() {
        delete []start;

        len = 0;
        posn = 0;
        buffer = nullptr;
        start = buffer;
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
            conn.newBuffer(ntohl(conn.len));
        }
    } 

    if (bytes - rbytes !=  0) {
        int rbyte = recv(conn.fd, conn.buffer, conn.len - conn.posn, 0);
        if (rbyte < 0) return rbyte;

        conn.posn += rbyte;

        rbytes += rbyte;
    }

    return rbytes;
}

// Accepts the connection file-descriptor.
int acceptConn(int fd, int kq) {
    int connfd = accept(fd, NULL, NULL);
    if (connfd >= 0) fcntl(connfd, F_SETFL, O_NONBLOCK); 
    return connfd; 
}

int setupListenSocket() {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(PORT);
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(listenfd, F_SETFL, O_NONBLOCK);

    int ret = bind(listenfd, (sockaddr *) &addr, sizeof(addr));
    if (ret < 0) return ret;
    
    listen(listenfd, BACKLOG);
    return listenfd; 
}



int main() {
    int ret;
    std::unordered_map<int, Conn> conns;

    int listenfd = setupListenSocket();
    if (listenfd < 0) die("setupListenSocket()");

    
    int kq = kqueue();
    if (kq < 0) die("kevent()");
    struct kevent evSet; 
    struct kevent tevent;
    EV_SET(&evSet, listenfd, EVFILT_READ, EV_ADD, 0, BACKLOG, NULL);
    ret = kevent(kq, &evSet, 1, NULL, 0, NULL);
    if (ret < 0) die("kevent()");

    // event loop.
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
                int connfd = acceptConn(listenfd, kq);
                if (connfd < 0) die("accept()");

                EV_SET(&evSet, connfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                ret = kevent(kq, &evSet, 1, NULL, 0, NULL);
                if (ret < 0) die("kevent()");

                conns[connfd] = Conn{connfd};
            } else if (tevent.filter == EVFILT_READ) {
                Conn &conn = conns[tevent.ident];

                int rbytes = handleRead(conn, tevent.data);
                if (rbytes < 0)  die("handleRead()");

                if (conn.posn  == conn.len) {
                    std::cout << "msg: " << conn.buffer << std::endl;
                    conn.clearBuffer();
                }
            }
        }


    }

    close(listenfd);
}
