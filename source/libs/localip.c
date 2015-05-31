/*
20131204
Jan Mojzis
Public domain.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>
#include "byte.h"
#include "localip.h"


long long localip4(unsigned char *buf, long long buflen) {

    long long len = 0;
    struct ifaddrs *ifaddr, *ifa;
    unsigned char *x;
    struct sockaddr_in *sin;


    if (getifaddrs(&ifaddr) == -1) return 0;

    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        if (ifa->ifa_addr->sa_family == PF_INET) {
            if (ifa->ifa_flags & IFF_UP && !(ifa->ifa_flags & IFF_LOOPBACK)) {
                sin = (struct sockaddr_in *)ifa->ifa_addr;
                x = (unsigned char *)&sin->sin_addr;
                if (len + 16 <= buflen) {
                    byte_copy(buf + len, 12, "\0\0\0\0\0\0\0\0\0\0\377\377");
                    byte_copy(buf + len + 12, 4, x);
                    len += 16;
                }
            }
        }
    }
    freeifaddrs(ifaddr);
    return len;
}


long long localip6(unsigned char *buf, long long buflen) {

    long long len = 0;
    struct ifaddrs *ifaddr, *ifa;
    unsigned char *x;
    struct sockaddr_in6 *sin;


    if (getifaddrs(&ifaddr) == -1) return 0;

    for (ifa = ifaddr; ifa; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;

        if (ifa->ifa_addr->sa_family == PF_INET6) {
            if (ifa->ifa_flags & IFF_UP && !(ifa->ifa_flags & IFF_LOOPBACK)) {
                sin = (struct sockaddr_in6 *)ifa->ifa_addr;
                x = (unsigned char *)&sin->sin6_addr;
                if (!byte_isequal(x, 2, "\376\200")) { /* XXX no link-local address prefix */
                    if (len + 16 <= buflen) {
                        byte_copy(buf + len, 16, x);
                        len += 16;
                    }
                }
            }
        }
    }
    freeifaddrs(ifaddr);
    return len;
}

