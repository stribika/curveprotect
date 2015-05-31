#include <unistd.h>
#include "scan.h"
#include "iopause.h"
#include "ndelay.h"
#include "error.h"
#include "byte.h"
#include "strerr.h"
#include "fmt.h"
#include "uint16.h"
#include "uint32.h"
#include "stralloc.h"
#include "writeall.h"

static char strnum[FMT_ULONG];
static void warn_read(const char *warning, int t){
    if (!warning) return;
    strnum[fmt_ulong(strnum, t)] = 0;
    strerr_warn4(warning,"unable to read fd ",strnum,": ",&strerr_sys);
}

static void warn_packet(const char *warning){
    if (!warning) return;
    strerr_warn2(warning,"unable to parse packet length: dropping packet", 0);
}

static void warn_write(const char *warning, int t){
    if (!warning) return;
    strnum[fmt_ulong(strnum, t)] = 0;
    strerr_warn4(warning,"unable to write to fd ",strnum,": ",&strerr_sys);
}

static void warn_ndelay(const char *warning, int t){
    if (!warning) return;
    strnum[fmt_ulong(strnum, t)] = 0;
    strerr_warn4(warning,"unable to set fd ",strnum," to 'no delay' mode: ",&strerr_sys);
}

stralloc buf0 = {0};
stralloc buf1 = {0};
stralloc *buf;
int flagwrite0 = 1;
int flagwrite1 = 1;
int *flagwrite;
int fdout;
char addrbuf[60];

/* random 'ping' string, first byte must be 0x00 */
unsigned char pingbuf[20] = {
    0x00,0xf6,0x8a,0x8d,
    0x6c,0xed,0x5b,0x98,
    0xa0,0xb4,0xf2,0xd3,
    0x66,0x81,0x76,0x8f,
    0xe5,0xb7,0x43,0x46
};

int packetlen(const char *warning, const char *data, uint16 *dlen, int *flagping){

    char *addr = addrbuf;
    uint32 count;
    uint32 sum = 0;
    uint16 ret;
    uint16 sum1;
    unsigned int pr;

    *flagping = 0;

    pr = data[0] & 0xf0;
    pr >>= 4;
    if (pr == 0) {
        /* ping */
        if (byte_diff(data, sizeof pingbuf, pingbuf)) return -1;
        *flagping = 1;
        *dlen = sizeof pingbuf;
        return 0;
    }
    /* XXX ipv4 only */
    if (pr != 4) return -1;

    count = 4 * (data[0] & 0x0f);
    if (count > sizeof addrbuf) return -1;

    byte_copy(&sum1, 2, data+10);
    byte_copy(addr, count, data);
    byte_zero(addr+10, 2);

    while(count > 1){
        sum += *(uint16*) addr;
        if(sum & 0x80000000)
            sum = (sum & 0xFFFF) + (sum >> 16);
        count -= 2;
        addr  += 2;
    }

    if(count)
        sum += (uint16) *(unsigned char *) addr;
          
    while(sum>>16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    ret = ~sum;
    if (ret != sum1) return -1;
    uint16_unpack_big(data + 2, dlen);
    return 0;
}


int fdcp_vpn(const char *warning, int fdin0, int fdout0, int timeout1, int fdin1, int fdout1,  int timeout2, int flagverbose, int flagserver){

    struct taia now;
    struct taia deadline1;
    struct taia deadline2;
    struct taia mindeadline;
    struct taia pingdeadline;
    unsigned int pingtimeout = 30;
    struct taia *deadline;
    int timeout;
    iopause_fd p[4];
    int r, savederrno;
    int ret = -1;
    uint16 plen;
    unsigned int i, j;
    int flagping;

    if (!stralloc_copys(&buf0,"")) return -1;
    if (!stralloc_copys(&buf1,"")) return -1;

    if (ndelay_on(fdin0) == -1) {warn_ndelay(warning,fdin0); goto cleanup;}
    if (ndelay_on(fdout0)== -1) {warn_ndelay(warning,fdout0); goto cleanup;}
    if (ndelay_on(fdin1) == -1) {warn_ndelay(warning,fdin1); goto cleanup;}
    if (ndelay_on(fdout1) == -1) {warn_ndelay(warning,fdout1); goto cleanup;}

    if (timeout1 > 86400 || timeout1 < 0){
        timeout1 = 86400;
    }
    if (timeout2 > 86400 || timeout2 < 0){
        timeout2 = 86400;
    }

    taia_now(&now);
    taia_uint(&deadline1,timeout1);
    taia_uint(&deadline2,timeout2);
    taia_add(&deadline1,&now,&deadline1);
    taia_add(&deadline2,&now,&deadline2);
    taia_now(&pingdeadline);

    for(;;){
        p[0].fd = fdin0;
        p[0].events = IOPAUSE_READ;
        p[1].fd = fdin1;
        p[1].events = IOPAUSE_READ;
        byte_zero(&p[2], sizeof (iopause_fd));
        byte_zero(&p[3], sizeof (iopause_fd));

        i = 2;

        if (buf0.len > 0 && flagwrite0) { 
            p[i].fd = fdout0;
            p[i].events = IOPAUSE_WRITE;
            ++i;
        }

        if (buf1.len > 0 && flagwrite1) { 
            p[i].fd = fdout1;
            p[i].events = IOPAUSE_WRITE;
            ++i;
        }

        byte_copy(&mindeadline, sizeof (struct taia), &deadline2);
        if (taia_less(&deadline1,&mindeadline)){
            byte_copy(&mindeadline, sizeof (struct taia), &deadline1);
        }
        if (taia_less(&pingdeadline,&mindeadline)){
            byte_copy(&mindeadline, sizeof (struct taia), &pingdeadline);
        }

        taia_now(&now);
        iopause(p,i,&mindeadline,&now);

        for(j = 2; j < i; ++j){

            if (!p[j].revents) continue;

            if (p[j].fd == fdout0) {
                buf = &buf0;
                *flagwrite = flagwrite0;
                /* TODO: optimalize ping */
            }
            else {
                buf = &buf1;
                *flagwrite = flagwrite1;
                /* TODO: optimalize ping */
            }

            do {
                if (buf->len < 20){ /* minimal ip4 ihl */
                    *flagwrite = 0;
                    break;
                }
                if (packetlen(warning, buf->s, &plen, &flagping) == -1) {
                    warn_packet(warning);
                    buf->len = 0;
                    break;
                }
                if (buf->len < plen){
                    *flagwrite = 0;
                    break;
                }

                if (!flagping) {
                    /* XXX */
                    r = writeall(p[j].fd, buf->s, plen);
                    if (r == -1) {
                        warn_write(warning,p[j].fd);
                        goto cleanup;
                    }
                }
                byte_copy(buf->s, buf->len - plen, buf->s + plen);
                buf->len -= plen;
            } while(0);
        }

        for(j = 0; j < 2; ++j){
            if (!p[j].revents) continue;

            if (p[j].fd == fdin0) {
                buf = &buf0;
                timeout = timeout1;
                deadline = &deadline1;
                flagwrite = &flagwrite0;
                fdout = fdout0;
            }
            else {
                buf = &buf1;
                timeout = timeout2;
                deadline = &deadline2;
                flagwrite = &flagwrite1;
                fdout = fdout1;
            }

            if (!stralloc_readyplus(buf, 1024)) return -1;
            r = read(p[j].fd, buf->s + buf->len, 1024);
            if (r == -1){
                if (errno == error_intr || errno == error_again || errno == error_wouldblock) continue;
                warn_read(warning,p[j].fd);
                goto cleanup;
            }
            if (r == 0) goto cleanup;
            buf->len += r;
            *flagwrite = 1;

            taia_now(&now);
            taia_uint(deadline,timeout);
            taia_add(deadline,&now,deadline);
        }
        
        taia_now(&now);
        if (taia_less(&deadline1,&now)) {
            errno = error_timeout;
            warn_read(warning,fdin0);
            goto cleanup;
        }
        if (taia_less(&deadline2,&now)) {
            errno = error_timeout;
            warn_read(warning,fdin1);
            goto cleanup;
        }

        taia_now(&now);
        if (taia_less(&pingdeadline,&now)) {
            if (flagserver) {
                writeall(fdout1, pingbuf, sizeof pingbuf);
            }
            else {
                writeall(fdout0, pingbuf, sizeof pingbuf);
            }
            taia_uint(&pingdeadline,pingtimeout);
            taia_add(&pingdeadline,&now,&pingdeadline);
        }

    }

    ret = 0;
cleanup:
    savederrno = errno;
    close(fdin0);
    close(fdout0);
    close(fdin1);
    close(fdout1);
    errno = savederrno;
    return ret;
}
