#include <unistd.h>
#include "localip.h"
#include "iptostr.h"
#include "writeall.h"
#include "die.h"
#include "e.h"


#define FATAL "netprintif: fatal: "

static void die_fatal(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}


char buf[256];
long long buflen = 0;

static void flush(void) {
    if (writeall(1, buf, buflen) == -1) die_fatal("unable to write output", 0);
    buflen = 0;
}

static void outs(const char *x) {

    long long i;

    for(i = 0; x[i]; ++i) {
        if (buflen >= sizeof buf) flush();
        buf[buflen++] = x[i];
    }
}

unsigned char ipbuf[1024];
long long ipbuflen;

int main() {

    long long j;

    ipbuflen = localip6(ipbuf, sizeof ipbuf);
    for (j = 0; j + 16 <= ipbuflen; j += 16) {
        outs(iptostr(0, ipbuf + j));
        outs("\n");
    }
    ipbuflen = localip4(ipbuf, sizeof ipbuf);
    for (j = 0; j + 16 <= ipbuflen; j += 16) {
        outs(iptostr(0, ipbuf + j));
        outs("\n");
    }
    flush();
    _exit(0);

}
