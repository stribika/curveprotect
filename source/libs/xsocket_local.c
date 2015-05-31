/*
20131208
Jan Mojzis
Public domain.
*/

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "e.h"
#include "byte.h"
#include "hasipv6.h"
#include "xsocket.h"


static int xsocket_local6(int fd, unsigned char *ip, unsigned char *port, long long *id) {

#ifdef HASIPV6
    struct sockaddr_in6 sa;
    socklen_t salen = sizeof sa;

    byte_zero(&sa, sizeof sa);
    if (getsockname(fd, (struct sockaddr *)&sa, &salen) == -1) return -1;
    if (((struct sockaddr *)&sa)->sa_family != PF_INET6) { errno = EPROTO; return -1; }
    if (ip) byte_copy(ip, 16, &sa.sin6_addr);
    if (port) byte_copy(port, 2, &sa.sin6_port);
    if (id) *id = sa.sin6_scope_id;
    return 0;
#else
    errno = EPROTONOSUPPORT;
    return -1;
#endif

}

static int xsocket_local4(int fd, unsigned char *ip, unsigned char *port, long long *id) {

    struct sockaddr_in sa;
    socklen_t salen = sizeof sa;

    byte_zero(&sa, sizeof sa);
    if (getsockname(fd, (struct sockaddr *)&sa, &salen) == -1) return -1;
    if (((struct sockaddr *)&sa)->sa_family != PF_INET) { errno = EPROTO; return -1; }
    if (ip) byte_copy(ip, 12, "\0\0\0\0\0\0\0\0\0\0\377\377");
    if (ip) byte_copy(ip + 12, 4, &sa.sin_addr);
    if (port) byte_copy(port, 2, &sa.sin_port);
    if (id) *id = 0;
    return 0;
}

int xsocket_local(int fd, int type, unsigned char *ip, unsigned char *port, long long *id) {

    if (type == XSOCKET_V4) {
        return xsocket_local4(fd, ip, port, id);
    }
    if (type == XSOCKET_V6) {
        return xsocket_local6(fd, ip, port, id);
    }
    errno = EPROTO;
    return -1;
}
