#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include "xsocket.h"

long long xsocket_getscopeid(const char *name) {
    return if_nametoindex(name);
}
