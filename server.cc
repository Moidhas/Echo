#include <iostream>
#include <cassert>
#include <cstdint>
#include <unistd.h>
#include <netdb.h>
#include <sys/event.h>
#include <unordered_map>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <vector>

#define PORT 49152
#define BACKLOG 5

struct Conn {
    int fd = -1;
    // the buffers are all in network order. 
    std::vector<uint8_t> readBuffer;
    std::vector<uint8_t> writeBuffer;
    bool wantWrite = false;

    Conn(int fd): fd{fd} {}
    Conn() {}
};

static void die(const char *msg) {
    fprintf(stderr, "[%d] %s\n", errno, msg);
    abort();
}


bool handleRequest(Conn &conn) {
    if (conn.readBuffer.size() <  4) {
        return false;
    }

    uint32_t len;
    memcpy(&len, conn.readBuffer.data(), sizeof(len));
    len = ntohl(len);
    
    if (len + 4 > conn.readBuffer.size()) {
        return false;
    }

    auto msgEnd = conn.readBuffer.begin() + sizeof(len) + len;
    conn.writeBuffer.insert(conn.writeBuffer.end(), conn.readBuffer.begin(), msgEnd);
    conn.readBuffer.erase(conn.readBuffer.begin(), msgEnd);
    return true;
}

int handleRead(Conn &conn, int bytes) {
    uint8_t buffer[bytes];
    int rbytes = recv(conn.fd, buffer, sizeof(buffer), 0);

    if (rbytes > 0) {
        conn.readBuffer.insert(conn.readBuffer.end(), buffer, buffer + rbytes);
        while(handleRequest(conn)) {}
    }

    conn.wantWrite = (conn.writeBuffer.size() > 0);

    return rbytes;
}

// This should only be called when the writeBuffer actually has a full message in it. 
int handleWrite(Conn &conn) {
    assert(conn.wantWrite);
    assert(conn.writeBuffer.size() > 0); 
    int wbytes = send(conn.fd, conn.writeBuffer.data(), conn.writeBuffer.size(), 0);

    conn.writeBuffer.erase(conn.writeBuffer.begin(), conn.writeBuffer.begin() + wbytes);
    conn.wantWrite = (conn.writeBuffer.size() > 0);

    return wbytes;
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

    // event-loop
    while (true) {
        ret = kevent(kq, NULL, 0, &tevent, 1, NULL);
        if (ret <= 0) {
            die("kevent()");
        } else if (tevent.flags & EV_EOF) {
            conns.erase(tevent.ident);
            close(tevent.ident);
        } else if (tevent.ident == listenfd && tevent.filter == EVFILT_READ) {
            int connfd = acceptConn(listenfd, kq);
            if (connfd < 0) die("accept()");

            struct kevent changeList[2];
            EV_SET(&changeList[0], connfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
            EV_SET(&changeList[1], connfd, EVFILT_WRITE, EV_ADD | EV_DISABLE, 0, 0, NULL);
            ret = kevent(kq, changeList, 2, NULL, 0, NULL);
            if (ret < 0) die("kevent()");

            conns[connfd] = Conn{connfd};
        } else if (tevent.filter == EVFILT_READ) {
            Conn &conn = conns[tevent.ident];
            int rbytes = handleRead(conn, tevent.data);
            if (rbytes < 0)  die("handleRead()");
            if (conn.wantWrite) {
                EV_SET(&evSet, conn.fd, EVFILT_WRITE, EV_ENABLE, 0, 0, NULL);
                ret = kevent(kq, &evSet, 1, NULL, 0, NULL);
                if (ret < 0) die("kevent()");
            }
        } else if (tevent.filter == EVFILT_WRITE) {
            Conn &conn = conns[tevent.ident];
            int rbytes = handleWrite(conn);
            if (rbytes < 0)  die("handleWrite()");
            if (!conn.wantWrite) {
                EV_SET(&evSet, conn.fd, EVFILT_WRITE, EV_DISABLE, 0, 0, NULL);
                ret = kevent(kq, &evSet, 1, NULL, 0, NULL);
                if (ret < 0) die("kevent()");
            }
        }
    }

    close(listenfd);
}
