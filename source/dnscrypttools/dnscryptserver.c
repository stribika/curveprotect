/*
20130105
Jan Mojzis
Public domain.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "die.h"
#include "timeoutread.h"
#include "timeoutwrite.h"
#include "timeoutconn.h"
#include "e.h"
#include "uint16_unpack_big.h"
#include "uint16_pack_big.h"
#include "byte.h"
#include "env.h"
#include "hexparse.h"
#include "xsocket.h"
#include "strtoip.h"
#include "droproot.h"
#include "str.h"
#include "dns.h"
#include "buffer.h"
#include "dnscrypt.h"
#include "fastrandombytes.h"
#include "fastrandommod.h"
#include "crypto_box_curve25519xsalsa20poly1305.h"
#include "portparse.h"

#define FATAL "dnscryptserver: fatal: "
#define INFO "dnscryptserver: info: "
#define WARNING "dnscryptserver: warning: "

#define PUBLICKEYBYTES crypto_box_curve25519xsalsa20poly1305_PUBLICKEYBYTES
#define SECRETKEYBYTES crypto_box_curve25519xsalsa20poly1305_SECRETKEYBYTES
#define BEFORENMBYTES crypto_box_curve25519xsalsa20poly1305_BEFORENMBYTES
#define NONCEBYTES crypto_box_curve25519xsalsa20poly1305_NONCEBYTES
#define BOXZEROBYTES crypto_box_curve25519xsalsa20poly1305_BOXZEROBYTES
#define ZEROBYTES crypto_box_curve25519xsalsa20poly1305_ZEROBYTES

/* DNScurve header */
#define MAGICLEN 8
#define PKLEN PUBLICKEYBYTES
#define NONCELEN 12

#define USAGE "\
\n\
dnscryptserver: usage:\n\
\n\
 name:\n\
   dnscryptserver - DNSCrypt server\n\
\n\
 syntax:\n\
   dnscryptserver [options]\n\
\n\
 description:\n\
   Dnscryptserver is a simple TCP only server-side DNSCrypt proxy.\n\
   It's using TCP port 443, because the port 443 is not very often blocked on firewalls\n\
   It's a dirty workaround for providers that are transparently\n\
   redirecting the DNS traffic to a local resolver\n\
\n\
 options:\n\
   -h (optional): print this help\n\
\n\
 environment:\n\
   $ROOT (mandatory): runs chrooted in $ROOT directory\n\
   $GID  (mandatory): runs under specified $GID gid\n\
   $UID  (mandatory): runs under specified $UID uid\n\
   $RESOLVERIP   (mandatory): DNS resolvers IPv4 address\n\
   $RESOLVERPORT  (optional): DNS resolvers TCP port (default: 53)\n\
   $PROVIDER     (mandatory): DNSCrypt provider name (DNS zone containing DNSCrypt certificate in TXT record)\n\
   $SECRETKEY    (mandatory): secret-key in hexadecimal form\n\
   $OLDSECRETKEY  (optional): second secret-key in hexadecimal form (for keys rollover)\n\
   $NONCEKEY     (mandatory): encryption key for nonce\n\
   $NONCESTART    (optional): nonce startbits\n\
   $QUERYMAGIC    (optional): (default: q6fnvWj9)\n\
   $RESPONSEMAGIC (optional): (default: r6fnvWj8)\n\
\n\
 notes:\n\
   dnscryptserver is fork-on-request server -> very poor performance\n\
\
"


static void die_usage(void) {
    die_1(100, USAGE);
}

unsigned char sk1[SECRETKEYBYTES];
unsigned char sk2[SECRETKEYBYTES];
unsigned char nk[16];
int flagsk2 = 0;
unsigned char pk[PUBLICKEYBYTES];
unsigned char k[BEFORENMBYTES];
unsigned char n[NONCEBYTES];

void cleanup(void) {

    fastrandombytes(sk1, sizeof sk1);
    fastrandombytes(sk2, sizeof sk2);
    fastrandombytes(k, sizeof k);
}

char *remoteip = "unknown";
char *resolveripstr;
unsigned char resolverip[4];
unsigned char resolverport[2];
char *querymagic = DNSCRYPT_QUERY_MAGIC;
char *responsemagic = DNSCRYPT_RESPONSE_MAGIC;

static unsigned char *zone = 0;
static unsigned char *providerzone = 0;


static void die_fatal(const char *trouble, const char *fn) {

    cleanup();

    if (errno) {
        if (fn) die_7(111, FATAL, trouble, " ", fn, ": ", e_str(errno), "\n");
        die_5(111, FATAL, trouble, ": ", e_str(errno), "\n");
    }
    if (fn) die_5(111, FATAL, trouble, " ", fn, "\n");
    die_3(111, FATAL, trouble, "\n");
}

void netread(int fd, unsigned char *buf, long long len) {

    long long r;

    while (len > 0) {
        r = timeoutread(10, fd, (char *)buf, len);
        if (r == 0) { cleanup(); _exit(0); }
        if (r < 0) die_fatal("unable to read from network", 0);
        buf += r; len -= r;
    }
    return;
}

void netwrite(int fd, unsigned char *buf, long long len) {

    long long w;

    while (len > 0) {
        w = timeoutwrite(10, fd, (char *)buf, len);
        if (w == 0) { errno = 0; die_fatal("unable to write to network: connection closed", 0); }
        if (w < 0) die_fatal("unable to write to network", 0);
        buf += w; len -= w;
    }
    return;
}

static void out(const char *x, long long len) {
    if (buffer_put(buffer_2, x, len) == -1) die_fatal("unable to write output", 0);
}

static void outs(const char *x) {
    if (buffer_puts(buffer_2, x) == -1) die_fatal("unable to write output", 0);
}

static void flush(void) {
    if (buffer_flush(buffer_2) == -1) die_fatal("unable to write output", 0);
}

int flagaccess = 1;
long long pathlen = 0;
stralloc path = {0};

static void access_init(void) {

    struct stat st;

    if (!stralloc_copys(&path, "./pk/")) die_fatal("unable to allocate memory", 0);
    pathlen = path.len;
    if (!stralloc_0(&path)) die_fatal("unable to allocate memory", 0);

    if (stat((char *)path.s, &st) == 0) {
	if (S_ISDIR(st.st_mode)) { flagaccess = 1; return; }
	die_fatal("unable to initialize accesslisting in directory", (char *)path.s); 
    }
    if (errno != ENOENT) die_fatal("unable to initialize accesslisting in directory", (char *)path.s);
    flagaccess = 0;
    return;
}

static void access_check(unsigned char *p) {

    long long i;
    char ch;
    struct stat st;

    if (!flagaccess) return;

    path.len = pathlen;

    for (i = 0; i < PUBLICKEYBYTES; ++i) {
	ch = "0123456789abcdef"[15 & (int) (p[i] >> 4)];
	if (!stralloc_append(&path, &ch)) die_fatal("unable to allocate memory", 0);
	ch = "0123456789abcdef"[15 & (int) (p[i] >> 0)];
	if (!stralloc_append(&path, &ch)) die_fatal("unable to allocate memory", 0);
    }
    if (!stralloc_0(&path)) die_fatal("unable to allocate memory", 0); 

    if (stat((char *)path.s, &st) == 0) return;
    errno = 0;
    die_fatal("access not desired for", (char *)path.s + pathlen);
}


static void xlog(char *severity, const unsigned char *q, const unsigned char qtype[2], int flagencrypted, const char *result) {

    char ch;
    unsigned char ch2;
    unsigned char cbuf[4];
    long long i;

    outs(severity);
    outs(remoteip);
    if (flagencrypted)
        outs(": secured: ");
    else
        outs(": regular: ");

    /* type */
    for (i = 0; i < 2; ++i) {
        ch = "0123456789abcdef"[15 & (int) (qtype[i] >> 4)];
        out(&ch, 1);
        ch = "0123456789abcdef"[15 & (int) (qtype[i] >> 0)];
        out(&ch, 1);
    }
    outs(" ");

    /* name */
    if (!*q)
        outs("unknown");
    else
        for (;;) {
            ch = *q++;
            while (ch--) {
                ch2 = (unsigned char)*q++;
                if ((ch2 >= 'A') && (ch2 <= 'Z'))
                    ch2 += 32;
                if (((ch2 >= 'a') && (ch2 <= 'z')) || ((ch2 >= '0') && (ch2 <= '9')) || (ch2 == '-') || (ch2 == '_'))
                    out((char *)&ch2, 1);
                else {
                    cbuf[3] = '0' + (ch2 & 7); ch2 >>= 3;
                    cbuf[2] = '0' + (ch2 & 7); ch2 >>= 3;
                    cbuf[1] = '0' + (ch2 & 7);
                    cbuf[0] = '\\';
                    out((char *)cbuf, 4);
                }
            }
            if (!*q) break;
            outs(".");
        }

    /* result */
    if (result) {
        outs(": ");
        outs(result);
    }
    outs("\n");
    flush();
}



unsigned char bufspace[4096 + 256 + 100]; /* 4096 for data, rest for padding, etc ...*/

void bufread(unsigned char **x, crypto_uint16 *len, int fd, crypto_uint16 offset) {

    netread(fd, bufspace, 2);
    *len = uint16_unpack_big(bufspace);
    if (*len > 4096) { errno = 0; die_fatal("excessively large request", 0); }
    netread(fd, bufspace + offset + 2, *len);
    *x = bufspace + 2;
}

void bufwrite(int fd, crypto_uint16 len) {
    uint16_pack_big(bufspace, len);
    netwrite(fd, bufspace, len + 2);

}

int main(int argc, char **argv) {

    int s, flagencrypted, flagdnscurve;
    int stype;
    char *x;
    unsigned long long xlen;

    long long pos;
    unsigned char header[12];
    unsigned char qtype[2];
    unsigned char qclass[2];

    crypto_uint16 len, paddinglen, offset;
    unsigned char *buf;

    if (argv[0])
        if (argv[1])
            if (str_equal(argv[1], "-h")) die_usage();

    fastrandombytes(header, 1);

    droproot(FATAL);
    errno = 0;

    /* get remoteip */
    x = env_get("TCPREMOTEIP");
    if (x) remoteip = x;

    /* get resolverip */
    resolveripstr = env_get("RESOLVERIP");
    if (!resolveripstr) die_fatal("$RESOLVERIP not set", 0);
    if (!strtoip(resolverip, resolveripstr)) die_fatal("unable to parse ip address from $RESOLVERIP", resolveripstr);

    /* get resolverport */
    x = env_get("RESOLVERPORT");
    if (!x) x = "53";
    if (!portparse(resolverport, x));

    /* get provider */
    x = env_get("PROVIDER");
    if (!x) die_fatal("$PROVIDER not set", 0);
    if (!dns_domain_fromdot(&providerzone, (unsigned char *)x, str_len(x))) die_fatal("unable to parse $PROVIDER", 0); 

    /* get secretkey */
    x = env_get("SECRETKEY");
    if (!x) die_fatal("$SECRETKEY not set", 0);
    if (!hexparse(sk1, sizeof sk1, x)) die_fatal("$SECRETKEY must be exactly 64 hex characters", 0);
    while (*x) { *x = 0; ++x; }

    /* get nonce key, nonce separation */
    x = env_get("NONCEKEY");
    if (!x) die_fatal("$NONCEKEY not set", 0);
    if (!hexparse(nk, sizeof nk, x)) die_fatal("$NONCEKEY must be exactly 32 hex characters", 0);
    dns_nonce_init(env_get("NONCESTART"), nk);
    while (*x) { *x = 0; ++x; }

    /* get oldsecretkey */
    x = env_get("OLDSECRETKEY");
    if (x) {
        flagsk2 = 1;
        if (!hexparse(sk2, sizeof sk2, x)) die_fatal("$OLDSECRETKEY must be exactly 64 hex characters", 0);
        while (*x) { *x = 0; ++x; }
    }

    /* get querymagic */
    x = env_get("QUERYMAGIC");
    if (x && str_len(x) == 8) querymagic = x;

    /* get responsemagic */
    x = env_get("RESPONSEMAGIC");
    if (x && str_len(x) == 8) responsemagic = x;

    /* accesslist */
    access_init();

    for (;;) {
        bufread(&buf, &len, 0, 0);

        flagencrypted = 0;
        flagdnscurve  = 0;
        if (len >= 52 && byte_isequal("Q6fnvWj8", MAGICLEN, buf)) {
            flagencrypted = 1;
            flagdnscurve = 1;
        }
        if (len >= 52 && byte_isequal(querymagic, MAGICLEN, buf)) {
            flagencrypted = 1;
        }

        if (flagencrypted) {
            /* pk */
            x = (char *)buf + MAGICLEN;
            byte_copy(pk, PKLEN, x);
	    access_check(pk);

            /* nonce */
            x = (char *)buf + MAGICLEN + PKLEN;
            byte_zero(n, sizeof n);
            byte_copy(n, NONCELEN, x);

            /* open box */
            x =    (char *)buf + MAGICLEN + PKLEN + NONCELEN - BOXZEROBYTES;
            xlen =         len - MAGICLEN - PKLEN - NONCELEN + BOXZEROBYTES;
            byte_zero(x, BOXZEROBYTES);
            crypto_box_curve25519xsalsa20poly1305_beforenm(k, pk, sk1);
            if (crypto_box_curve25519xsalsa20poly1305_open_afternm((unsigned char *)x, (unsigned char *)x, xlen, n, k) == 0) goto ok;
            if (flagsk2) {
                crypto_box_curve25519xsalsa20poly1305_beforenm(k, pk, sk2);
                if (crypto_box_curve25519xsalsa20poly1305_open_afternm((unsigned char *)x, (unsigned char *)x, xlen, n, k) == 0) goto ok;
            }
            errno = 0;
            die_fatal("unable to open box", 0); 
ok:
            x += ZEROBYTES;
            xlen -= ZEROBYTES;
            byte_copy(buf, xlen, x);
            len = xlen;
        }

	/* parse query */
        errno = 0;
        pos = dns_packet_copy(buf, len, 0, header, 12); if (!pos) die_fatal("truncated request", 0);
        if (header[2] & 254) die_fatal("bogus query", 0);
        if (header[4] || (header[5] != 1)) die_fatal("bogus query", 0);
        pos = dns_packet_getname(buf, len, pos, &zone); if (!pos) die_fatal("truncated request", 0);
        pos = dns_packet_copy(buf, len, pos, qtype,2); if (!pos) die_fatal("truncated request", 0);
        pos = dns_packet_copy(buf, len, pos, qclass, 2); if (!pos) die_fatal("truncated request", 0);
        if (byte_diff(qclass, 2, DNS_C_IN) && byte_diff(qclass, 2, DNS_C_ANY)) die_fatal("bogus query: bad class", 0);

        if (!flagencrypted && !dns_domain_equal(providerzone, zone)) {
            xlog(WARNING, zone, qtype, flagencrypted, "query not allowed in regular mode");
            _exit(111);
        }
        if (!flagencrypted && byte_diff(qtype, 2, DNS_T_TXT)) {
            xlog(WARNING, zone, qtype, flagencrypted, "query not allowed in regular mode");
            _exit(111);
        }

        /* remove padding  */
        len = pos;

	/* connect to resolver + send query */
        stype = xsocket_type(resolverip);
        s = xsocket_tcp(stype);
        if (s == -1) die_fatal("unable to create tcp socket", 0);
        if (timeoutconn(s, resolverip, resolverport, 5) == -1) die_fatal("unable to connect to resolver ip", resolveripstr); 
        bufwrite(s, len);

        if (!flagencrypted) {
            bufread(&buf, &len, s, 0);
            close(s);
        }
        else {
            offset = MAGICLEN + NONCEBYTES - BOXZEROBYTES + ZEROBYTES;
            bufread(&buf, &len, s, offset);
            x = (char *)buf + offset - ZEROBYTES;
            close(s);

            /* padding */
            if (!flagdnscurve) {
                paddinglen  = (2 + fastrandommod(3)) * DNSCRYPT_BLOCK - len % DNSCRYPT_BLOCK;
                byte_zero(x + ZEROBYTES + len, paddinglen);
                x[ZEROBYTES + len] = (char)0x80;
                len += paddinglen;
            }

            byte_zero(x, ZEROBYTES);
            dns_nonce(n + NONCELEN);
            crypto_box_curve25519xsalsa20poly1305_afternm((unsigned char *)x, (unsigned char *)x, len + ZEROBYTES, n, k);

            byte_copy(buf, MAGICLEN, responsemagic);
            if (flagdnscurve) byte_copy(buf,  MAGICLEN, "R6fnvWJ8");
            byte_copy(buf + MAGICLEN, NONCEBYTES, n);
            len += offset;
        }
        bufwrite(1, len);
        xlog(INFO, zone, qtype, flagencrypted, 0);

    }
    return 0;
}
