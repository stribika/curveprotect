#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "xsocket.h"

int xsocket_ipoptionskill(int fd) {
    return setsockopt(fd, IPPROTO_IP, 1, (char *)0, 0); /* 1 == IP_OPTIONS */
}
