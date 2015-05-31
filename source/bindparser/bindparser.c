/*
20131221
Jan Mojzis
Public domain.
*/

#include <unistd.h>
#include "numtostr.h"
#include "writeall.h"
#include "fsyncfd.h"
#include "die.h"
#include "warn.h"
#include "e.h"
#include "uint16_unpack_big.h"
#include "uint32_unpack_big.h"
#include "fsyncfd.h"
#include "env.h"
#include "common/errcode.h"
#include "zscanner/file_loader.h"
#include "zscanner/scanner.h"

#define FATAL "bindparser: fatal: "
#define WARNING "bindparser: warning: "

#define USAGE "\
\n\
bindparser: usage:\n\
\n\
 name:\n\
   bindparser - parse zone files in bind format\n\
\n\
 syntax:\n\
   bindparser origin file <input >output\n\
\n\
 description:\n\
   bindparser converts bind-style zonefile to djbdns format\n\
\n\
"

void die_usage(const char *s) {
  if (s) die_4(100, USAGE, FATAL, s, "\n");
  die_1(100, USAGE);
}


file_loader_t *fl = 0;

static void cleanup(void) {
    if (fl) file_loader_free(fl);
}

static void die_fatal(const char *trouble, const char *fn) {

    cleanup();

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}

static char buf[4096];
static long long buflen = 0;

static void flush(void) {
    if (writeall(1, buf, buflen) == -1) die_fatal("unable to write output", 0);
    buflen = 0;
}

static void die(void) {

    flush();
    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    cleanup();
    _exit(0);
}


static void out(const char *x, long long len) {

    long long i;

    for(i = 0; i < len; ++i) {
        if (buflen >= sizeof buf) flush();
        buf[buflen++] = x[i];
    }
}

static void outs(const char *x) {

    long long i;

    for(i = 0; x[i]; ++i) {
        if (buflen >= sizeof buf) flush();
        buf[buflen++] = x[i];
    }
}

static void outch(const char ch) {
    if (buflen >= sizeof buf) flush();
    buf[buflen++] = ch;
    return;
}

static void outnum(long long num) {
    outs(numtostr(0, num));
}


static void outname(const char *d) {
    char ch;
    char ch2;
    unsigned char ch3;
    char buf[4];


    if (!*d) { outch('.'); return ; }

    for (;;) {
        ch = *d++;
        while (ch--) {
            ch2 = *d++;
            if ((ch2 >= 'A') && (ch2 <= 'Z'))
                ch2 += 32;
            if (((ch2 >= 'a') && (ch2 <= 'z')) || ((ch2 >= '0') && (ch2 <= '9')) || (ch2 == '-') || (ch2 == '_')) {
                outch(ch2);
            }
            else {
                ch3 = ch2;
                buf[3] = '0' + (ch3 & 7); ch3 >>= 3;
                buf[2] = '0' + (ch3 & 7); ch3 >>= 3;
                buf[1] = '0' + (ch3 & 7);
                buf[0] = '\\';
                out(buf, sizeof buf);
            }
        }
        if (!*d) return;
        outch('.');
    }
}


static int printable(char ch) {
    if (ch == '.') return 1;
    if ((ch >= 'a') && (ch <= 'z')) return 1;
    if ((ch >= '0') && (ch <= '9')) return 1;
    if ((ch >= 'A') && (ch <= 'Z')) return 1;
    if (ch == '-') return 1;
    return 0;
}

static void process_error(const scanner_t *s) {
    warn_4(WARNING, "unable to parse record ", numtostr(0, s->r_type), "\n");
}

static void process_record(const scanner_t *s) {

    uint32_t block;
    long long i;
    const char *data;
    unsigned char ch;
    char buf[4];

    if (s->r_class != KNOT_CLASS_IN) { process_error(s); return; }

    switch (s->r_type) {
        case KNOT_RRTYPE_SOA:
            if (s->r_data_blocks_count != 3) { process_error(s); return; }
            if (s->r_data_blocks[3] - s->r_data_blocks[2] != 20) { process_error(s); return; }
            outch('Z');
            outname(s->r_owner);
            outch(':');
            outname(s->r_data + s->r_data_blocks[0]);
            outs(".:");
            outname(s->r_data + s->r_data_blocks[1]);
            outch('.');
            data = s->r_data + s->r_data_blocks[2];
            for (i = 0; i < 5; ++i) {
                outch(':');
                outnum(uint32_unpack_big(data + 4 * i));
            }
            break;
        case KNOT_RRTYPE_NS:
            if (s->r_data_blocks_count != 1) { process_error(s); return; }
            if (s->r_data_blocks[1] == 0) { process_error(s); return; }
            outch('&');
            outname(s->r_owner);
            outs("::");
            outname(s->r_data);
            outch('.');
            break;
        case KNOT_RRTYPE_CNAME:
            if (s->r_data_blocks_count != 1) { process_error(s); return; }
            if (s->r_data_blocks[1] == 0) { process_error(s); return; }
            outch('C');
            outname(s->r_owner);
            outch(':');
            outname(s->r_data);
            outch('.');
            break;
        case KNOT_RRTYPE_PTR:
            if (s->r_data_blocks_count != 1) { process_error(s); return; }
            if (s->r_data_blocks[1] == 0) { process_error(s); return; }
            outch('^');
            outname(s->r_owner);
            outch(':');
            outname(s->r_data);
            outch('.');
            break;
        case KNOT_RRTYPE_MX:
            if (s->r_data_blocks_count != 2) { process_error(s); return; }
            if ((s->r_data_blocks[1] - s->r_data_blocks[0]) != 2) { process_error(s); return; }
            outch('@');
            outname(s->r_owner);
            outs("::");
            outname(s->r_data + s->r_data_blocks[1]);
            outs(".:");
            outnum(uint16_unpack_big(s->r_data + s->r_data_blocks[0]));
            break;
        case KNOT_RRTYPE_A:
            if (s->r_data_length != 4) { process_error(s); return; }
            outch('+');
            outname(s->r_owner);
            outch(':');
            outnum(s->r_data[0]); outch('.');
            outnum(s->r_data[1]); outch('.');
            outnum(s->r_data[2]); outch('.');
            outnum(s->r_data[3]);
            break;
        case KNOT_RRTYPE_AAAA:
            if (s->r_data_length != 16) { process_error(s); return; }
        default:
            outch(':');
            outname(s->r_owner);
            outch(':');
            outnum(s->r_type);
            outch(':');
            for (i = 0; i < s->r_data_length; ++i) { 
                if (printable((s->r_data)[i])) {
                    outch((s->r_data)[i]);
                }
                else {
                    ch = (unsigned char)((s->r_data)[i]);
                    buf[3] = '0' + (ch & 7); ch >>= 3;
                    buf[2] = '0' + (ch & 7); ch >>= 3;
                    buf[1] = '0' + (ch & 7);
                    buf[0] = '\\';
                    out(buf, sizeof buf);
                }
            }
    }
    outch(':');
    outnum(s->r_ttl);
    outch('\n');
}


int main(int argc, char **argv) {

    int ret;
    const char *origin;

    if (!argv[0]) die_usage(0);
    if (!argv[1]) die_usage("file missing");

    origin = env_get("ORIGIN");
    if (!origin) origin = ".";

    fl = file_loader_create(argv[1], origin, DEFAULT_CLASS, DEFAULT_TTL, &process_record, &process_error, 0);
    if (!fl) die_fatal("unable to parse file", 0);

    ret = file_loader_process(fl);
    if (ret != KNOT_EOK) die_fatal("unable to parse file:", knot_strerror(ret));

    if (fsyncfd(1) == -1) die_fatal("unable to write output", 0);
    die();
    return 111;
}
