/*
20121211
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include <poll.h>
#include <stdlib.h>
#include "byte.h"
#include "e.h"
#include "readinput.h"

#define MYBLOCK 8096

static long long myread(int fd, void *x, long long xlen) {

    long long r;

    if (xlen > 1048576) xlen = 1048576;

    for (;;) {
        r = read(fd, x, xlen);
        if (r != -1) break;
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            struct pollfd p;
            p.fd = fd;
            p.events = POLLIN;
            poll(&p, 1, -1);
            continue;
        }
        break;
    }
    return r;
}


int readinput(long long zerobytes, unsigned char **x, long long *xlen) {

    long long malen = zerobytes + MYBLOCK;
    long long mlen = zerobytes;
    unsigned char *m, *newm;
    long long r;

    /* allocate space */
    m = malloc(malen);
    if (!m) return -1;

    /* zerobytes */
    byte_zero(m, mlen);

    /* read data */
    for (;;) {
        if (mlen + 1 > malen) {
            malen = MYBLOCK + 2 * malen;
            newm = malloc(malen);
            if (!newm) return -1;
            if (m) {
                byte_copy(newm, mlen, m);
                free(m);
            }
            m = newm;
        }
        r = myread(0, m + mlen, malen - mlen);
        if (r == -1) return -1;
        if (r == 0) break;
        mlen += r;
    }
    *x = m;
    *xlen = mlen;
    return 0;
}
