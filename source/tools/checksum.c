/*
20130113
Jan Mojzis
Public domain.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "die.h"
#include "e.h"
#include "writeall.h"
#include "strtonum.h"
#include "fsyncfd.h"
#include "readblock.h"
#include "str.h"

void die_usage(void) {
  die_1(100, USAGE);
}

static void die_fatal(const char *trouble, const char *fn) {

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}

static unsigned char buf[4096];
static unsigned char h[BYTES];
static unsigned char out[2*BYTES + 1];
static unsigned long long inlen;
static unsigned char *in;

int main(int argc, char **argv) {

    struct stat st;
    long long i;
    sha_ctx ctx;
    long long bytes = -1;

    if (argv[0])
        if (argv[1]) {
            if (str_equal(argv[1], "-h")) die_usage();
            strtonum(&bytes, argv[1]);
        }
    if (bytes < 0 || bytes > BYTES) bytes = BYTES;

    if (fstat(0, &st) == 0) {
        in = mmap(0, st.st_size, PROT_READ, MAP_SHARED, 0, 0);
        if (in != MAP_FAILED) {
            sha(h, in, st.st_size);
            goto result;
        }
    }

    in = buf;
    sha_init(&ctx);

    for (;;) {
        inlen = readblock(0, buf, sizeof buf);
        if (inlen != sizeof buf) break;
        if (sha_block(&ctx, in, inlen) != 0) die_fatal("unable to compute hash", 0);
    }
    if (sha_last(&ctx, h, in, inlen) != 0) die_fatal("unable to compute hash", 0);

result:
    for (i = 0; i < bytes; ++i) {
        out[2 * i + 0] = "0123456789abcdef"[15 & (int) (h[i] >> 4)];
        out[2 * i + 1] = "0123456789abcdef"[15 & (int) (h[i] >> 0)];
    }
    out[2 * bytes] = '\n';

    if (writeall(1, out, 2 * BYTES + 1) == -1) die_fatal("unable to write output", 0);
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    _exit(0);
}
