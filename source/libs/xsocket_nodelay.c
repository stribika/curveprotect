#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "xsocket.h"

int xsocket_tcpnodelay(int s) {

    int opt = 1;
    return setsockopt(s, IPPROTO_TCP, 1, &opt, sizeof opt); /* 1 == TCP_NODELAY */
}
