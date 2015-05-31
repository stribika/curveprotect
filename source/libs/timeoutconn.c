#include <poll.h>
#include "xsocket.h"
#include "e.h"
#include "milliseconds.h"
#include "blocking.h"
#include "timeoutconn.h"


int timeoutconn(int fd, unsigned char *ip, unsigned char *port, long long t) {

    long long tm;
    long long deadline;
    struct pollfd p;

    if (t <= 0) return 0;

    if (xsocket_connect(fd, xsocket_type(ip), ip, port, 0) == -1) {
        if ((errno != EWOULDBLOCK) && (errno != EINPROGRESS)) return -1;

        deadline = milliseconds() + t * 1000;

        p.fd = fd;
        p.events = POLLOUT | POLLERR;
        for (;;) {
            tm = deadline - milliseconds();
            if (tm <= 0) {
                errno = ETIMEDOUT;
                return -1;
            }
            if (tm > 1000000) tm = 1000000;
            poll(&p, 1, tm);
            if (p.revents) break;
        }
        if (!xsocket_connected(fd)) return -1;
    }
    blocking_disable(fd);
    return 0;
}
