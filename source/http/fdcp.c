#include <unistd.h>
#include "scan.h"
#include "iopause.h"
#include "ndelay.h"
#include "error.h"
#include "byte.h"
#include "writeall.h"
#include "strerr.h"
#include "fmt.h"
#include "uint16.h"


static char strnum[FMT_ULONG];
static void warn_read(const char *warning, int t){
    if (!warning) return;
    strnum[fmt_ulong(strnum, t)] = 0;
    strerr_warn4(warning,"unable to read fd ",strnum,": ",&strerr_sys);
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

static char buf[1024];

int fdcp(const char *warning, int fdin1, int fdout1, int timeout1, int fdin2, int fdout2,  int timeout2){

    struct taia now;
    struct taia deadline1;
    struct taia deadline2;
    iopause_fd p[2];
    int r, savederrno;
    int ret = -1;

    /* XXX */
    if (write(15,"00000",5) == -1) {};

    if (ndelay_on(fdin1) == -1) {warn_ndelay(warning,fdin1); goto cleanup;}
    if (ndelay_on(fdout1)== -1) {warn_ndelay(warning,fdout1); goto cleanup;}
    if (ndelay_on(fdin2) == -1) {warn_ndelay(warning,fdin2); goto cleanup;}
    if (ndelay_on(fdout2) == -1) {warn_ndelay(warning,fdout2); goto cleanup;}

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

    for(;;){

        p[0].fd = fdin1;
        p[1].fd = fdin2;
        p[0].events = IOPAUSE_READ;
        p[1].events = IOPAUSE_READ;

        taia_now(&now);
        if (taia_less(&deadline1,&deadline2)){
            iopause(p,2,&deadline1,&now);
        }
        else{
            iopause(p,2,&deadline2,&now);
        }

        if (p[0].revents){
            r = read(fdin1, buf, sizeof buf);
            if (r == -1){
                if (errno == error_intr || errno == error_again || errno == error_wouldblock) continue;
                warn_read(warning,fdin1);
                goto cleanup;
            }
            if (r == 0) break;
            if (writeall(fdout1, buf, r) == -1){
                warn_write(warning,fdout1);
                goto cleanup;
            }
            taia_now(&now);
            taia_uint(&deadline1,timeout1);
            taia_add(&deadline1,&now,&deadline1);
            continue;
        }

        if (p[1].revents){
            r = read(fdin2, buf, sizeof buf);
            if (r == -1){
                if (errno == error_intr || errno == error_again || errno == error_wouldblock) continue;
                warn_read(warning,fdin2);
                goto cleanup;
            }
            if (r == 0) break;
            if (writeall(fdout2, buf, r) == -1){
                warn_write(warning,fdout2);
                goto cleanup;
            }
            taia_now(&now);
            taia_uint(&deadline2,timeout2);
            taia_add(&deadline2,&now,&deadline2);
            continue;
        }

        taia_now(&now);
        if (taia_less(&deadline1,&now)) {
            errno = error_timeout;
            warn_read(warning,fdin1);
            goto cleanup;
        }
        if (taia_less(&deadline2,&now)) {
            errno = error_timeout;
            warn_read(warning,fdin2);
            goto cleanup;
        }
    }

    ret = 0;
cleanup:
    savederrno = errno;
    close(fdin1);
    close(fdout1);
    close(fdin2);
    close(fdout2);
    errno = savederrno;
    return ret;
}

