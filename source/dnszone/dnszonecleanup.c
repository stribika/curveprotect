/*
20120426
Jan Mojzis
Public domain.
Based on public domain libraries from djbdns-1.5 and nacl-20110221
*/

#include <unistd.h>
#include "direntry.h"
#include "die.h"
#include "warn.h"
#include "e.h"
#include "open.h"
#include "dnszones.h"
#include "str.h"
#include "saferm.h"

#define INFO "dnszonecleanup: info: "
#define FATAL "dnszonecleanup: fatal: "
#define WARNING "dnszonecleanup: warning: "

void die_usage(void) {
    die_1(100, "dnszonecleanup: usage: dnszonecleanup /cfgdir /vardir\n");
}

void die_fatal(const char *trouble, const char *d, const char *fn) {

    if (d) {
        if (fn) die_9(111,FATAL,trouble," ",d,"/",fn,": ",e_str(errno),"\n");
        die_7(111,FATAL,trouble," ",d,": ",e_str(errno),"\n");
    }
    die_5(111,FATAL,trouble,": ",e_str(errno),"\n");
}

struct dnszones dnszones;


int cleanup_tmp(const char *d0, const char *d1, int dummy) {

    direntry *d;
    DIR *dir;

    dir = opendir(".");
    if (!dir) return 0;

    for (;;) {
        errno = 0;
        d = readdir(dir);
        if (!d) break;
        if (d->d_name[0] == '.') continue;
        if (dnszones_namefind(&dnszones, d->d_name)) continue;
        if (saferm(d->d_name, 1) == 0) continue;
        warn_9(WARNING, "unable to cleanup in directory ", d0, "/", d1, "/",  d->d_name, ": ", e_str(errno));
        warn_1("\n");
    }
    if (errno) {closedir(dir); return 0;}
    closedir(dir);
    return 1;
}

void tryunlink(const char *d0, const char *d1, const char *fn) {

    if (unlink(fn) == -1) {
        warn_9(WARNING, "unable to unlink file ", d0, "/", d1, "/", fn, ": ", e_str(errno));
        warn_1("\n");
    }
    return;
}


int cleanup_zones(const char *d0, const char *d1, int flag) {

    /*
    flag = 0 ... remove all files
    flag = 1 ... remove only files for nonexistent zones
    flag = 2 ... remove all files exclude data, data.cdb and lock
    */

    direntry *d;
    DIR *dir;

    dir = opendir(".");
    if (!dir) return 0;

    for (;;) {
        errno = 0;
        d = readdir(dir);
        if (!d) break;
        if (str_equal(d->d_name, ".")) continue;
        if (str_equal(d->d_name, "..")) continue;
        if (flag == 1 && dnszones_namefind(&dnszones, d->d_name)) continue;
        if (flag == 2 && str_equal(d->d_name, "data")) continue;
        if (flag == 2 && str_equal(d->d_name, "data.cdb")) continue;
        if (flag == 2 && str_equal(d->d_name, "lock")) continue;
        tryunlink(d0, d1, d->d_name);
    }
    if (errno) {closedir(dir); return 0;}
    closedir(dir);
    return 1;
}


long long basefd;

void cleanup(const char *d0, const char *d1, int flag, int op(const char *d0, const char *d1, int flag)) {

    if (fchdir(basefd) == -1){
        warn_4(WARNING, "unable to change directory to base directory: ", e_str(errno), "\n");
        return;
    }

    if (chdir(d0) == -1) {
        warn_6(WARNING, "unable to change directory to ", d0, ": ", e_str(errno), "\n");
        return;
    }
    if (chdir(d1) == -1) {
        warn_8(WARNING, "unable to change directory to ", d0, "/", d1, ": ", e_str(errno), "\n");
        return;
    }

    if (!op(d0, d1, flag)) {
        warn_6(WARNING, "unable to cleanup in directory ", d0, ": ", e_str(errno), "\n");
        return;
    }
    return;
}


int main(int argc, char **argv) {

    const char *cfgdir, *vardir;

    if (!argv[0]) die_usage();
    if (!argv[1]) die_usage();
    if (!argv[2]) die_usage();
    cfgdir = argv[1];
    vardir = argv[2];

    basefd = open_cwd();
    if (basefd == -1) die_fatal("unable to open current working directory", 0, 0);

    /* lock cfg directory and read configuration */
    if (chdir(cfgdir) == -1) die_fatal("unable to change directory to", cfgdir, 0);
    if (open_lock("lock") == -1) die_fatal("unable to lock file", cfgdir, "lock");
    if (!dnszones_dirinit(&dnszones, cfgdir, vardir, WARNING))  die_fatal("unable to read configuration", 0, 0);

    /* cleanup  */
    cleanup(vardir, DNSZONES_DIR_TMP, -1, cleanup_tmp);
    cleanup(vardir, DNSZONES_DIR_ZONESTMP, 0, cleanup_zones);
    cleanup(vardir, DNSZONES_DIR_ZONESPOSTTMP, 0, cleanup_zones);
    cleanup(vardir, DNSZONES_DIR_ORIGTMP, 0, cleanup_zones);
    cleanup(vardir, DNSZONES_DIR_ZONES, 1, cleanup_zones);
    cleanup(vardir, DNSZONES_DIR_ZONESPOST, 1, cleanup_zones);
    cleanup(vardir, DNSZONES_DIR_ORIG, 1, cleanup_zones);
    cleanup(vardir, DNSZONES_DIR_ROOT, 2, cleanup_zones);

    _exit(0);
}


