#include "net_utils.h"
#include <errno.h>

#define PORT 49152

int main() {
    sockaddr_in addr{};
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(PORT);
    addr.sin_family = AF_INET;

    int fd = socket(AF_INET, SOCK_STREAM, 0);

    int err = connect(fd, (sockaddr *) &addr, sizeof(addr));
    if (err < 0) {
        return errno;
    }
    
    std::string msg;
    while(getline(std::cin, msg)) {
        if (send_msg(fd, msg) < 0) break;
    }

    close(fd);
}
