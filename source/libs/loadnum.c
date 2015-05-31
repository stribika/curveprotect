/*
20120329
Jan Mojzis
Public domain.
Based on public domain libraries from nacl-20110221
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "open.h"
#include "e.h"
#include "strtonum.h"
#include "loadnum.h"

static long long myread(int fd, char *x, long long xlen) {

    long long r;
    long long len = xlen;

    while (xlen > 0) {
        r = xlen;
        if (r > 1048576) r = 1048576;
        r = read(fd,x,r);
        if (r == 0) break;
        if (r <= 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            return -1;
        }
        x += r;
        xlen -= r;
    }
    return len - xlen;
}

int loadnum(const char *fn, long long *ret) {

    char buf[42]; /* space for sign + int128 + extra character + '\0'*/
    long long buflen;
    int fd;

    fd = open_read(fn);
    if (fd == -1) return 0;
    buflen = myread(fd, buf, sizeof buf - 1);
    close(fd);
    if (buflen == -1) return 0;
    buf[buflen] = 0;

    return strtonum(ret, buf);
}
