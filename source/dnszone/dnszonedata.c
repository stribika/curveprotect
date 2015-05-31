/*
210120426
Jan Mojzis
Public domain.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h> /* rename */
#include "die.h"
#include "e.h"
#include "open.h"
#include "coe.h"
#include "dnszones.h"
#include "writeall.h"
#include "str.h"
#include "byte.h"
#include "dns.h"
#include "pathexec.h"
#include "env.h"
#include "buffer.h"

#define FATAL "dnszonedata: fatal: "
#define WARNING "dnszonedata: warning: "

void die_usage(void) {
    die_1(100, "dnszonedata: usage: dnszonedata /zonedir\n");
}

void die_fatal(const char *trouble, const char *d, const char *fn) {

    if (d) {
        if (fn) die_9(111,FATAL,trouble," ",d,"/",fn,": ",e_str(errno),"\n");
        die_7(111,FATAL,trouble," ",d,": ",e_str(errno),"\n");
    }
    die_5(111,FATAL,trouble,": ",e_str(errno),"\n");
}

buffer bin, bout;
const char tmpfn[] = "data.temp";
const char datafn[] = "data";

void outs(const char *buf) {
    if (buffer_puts(&bout, buf) == -1) die_fatal("unable to write to file", tmpfn, 0);
}


struct dnszones dnszones;
char *q;

char outspace[1024];
char inspace[1024];

int main(int argc, char **argv) {

    const char *zonedir;
    long long i, fd;
    long long basefd;
    long long tmpfd;
    char *run[2];
    int lockfd;

    if (!argv[0]) die_usage();
    if (!argv[1]) die_usage();
    zonedir = argv[1];

    lockfd = open_lock("lock");
    if (lockfd == -1) die_fatal("unable to lock file", "lock", 0);
    coe_disable(lockfd);

    basefd = open_cwd();
    if (basefd == -1) die_fatal("unable to open current working directory", 0, 0);

    tmpfd = open_trunc(tmpfn);
    if (tmpfd == -1) die_fatal("unable to open file", tmpfn, 0);
    buffer_init(&bout, buffer_unixwrite, tmpfd, outspace, sizeof outspace);

    if (!dnszones_fileinit(&dnszones, zonedir, WARNING))  die_fatal("unable to read configuration", 0, 0);

    if (chdir(zonedir) == -1) die_fatal("unable to change directory to", zonedir, 0);

    for(i = 0; i < dnszones.zoneslen; ++i) {
        fd = open_read(dnszones.zones[i]->fqdn);
        if (fd == -1) die_fatal("unable to open file", dnszones.zones[i]->fn, 0);

        buffer_init(&bin, buffer_unixread, fd, inspace, sizeof inspace);

        switch(buffer_copy(&bout, &bin)) {
            case -2:
                die_fatal("unable to read file", dnszones.zones[i]->fn, 0);
            case -3:
                die_fatal("unable to write to file", tmpfn, 0);
        }
        close(fd);
    }

    if (fchdir(basefd) == -1) die_fatal("unable to change directory to base directroy", 0, 0);
    close(basefd);

    if (buffer_flush(&bout) == -1) die_fatal("unable to write to file", tmpfn, 0);
    if (fsync(tmpfd) == -1) die_fatal("unable to write to file", tmpfn, 0);
    if (close(tmpfd) == -1) die_fatal("unable to write to file", tmpfn, 0);
    if (rename(tmpfn, datafn) == -1) die_fatal("unable rename data.temp to data", 0, 0);

    run[0] = "tinydns-data";
    run[1] = 0;

    pathexec(run);
    die_fatal("unable to run program", *run, 0);
    return 111;
}
