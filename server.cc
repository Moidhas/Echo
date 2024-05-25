#include "net_utils.h"
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

#define PORT 49152
#define BACKLOG 5

int socket_setup() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(PORT);
     
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr))) {
        return -1;
    }

    if (listen(fd, BACKLOG)) return -1;
    return fd;
}


int watch_loop(int kq) {
    if (kq < 0) return -1;
    int listenFd = socket_setup();
    if (listenFd < 0) return -1;
    struct kevent evSet;
    struct kevent eventList[BACKLOG];
    
    EV_SET(&evSet, listenFd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent(kq, &evSet, 1, NULL, 0, NULL) < 0) return -1;
    struct sockaddr s{};
    socklen_t len = sizeof(s);

    while(true) {
        int n = kevent(kq, NULL, 0, eventList, BACKLOG, NULL);
        if (n < 1) return -1;

        for (int i = 0; i < n; ++i) {
            int fd = eventList[i].ident;
            if (eventList[i].flags & EV_EOF) {
                EV_SET(&evSet, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                if (kevent(kq, &evSet, 1, NULL, 0, NULL) < 0) return -1;
                close(fd);
            } else if (fd == listenFd) {
                fd = accept(listenFd, &s, &len);
                if (fd >= 0) {
                    EV_SET(&evSet, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                    if (kevent(kq, &evSet, 1, NULL, 0, NULL) < 0) return -1;
                }
            } else {
                read_msg(fd);
            }
        }
    }

    close(kq);
    return 0;
}

int main() {
    int kq = kqueue();
    return watch_loop(kq);
}

