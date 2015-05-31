/*
20120422
Jan Mojzis
Public domain.
Based on public domain libraries from djbdns-1.5 and nacl-20110221
*/

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h> /* rename */
#include <utime.h>
#include "die.h"
#include "warn.h"
#include "e.h"
#include "str.h"
#include "dns.h"
#include "open.h"
#include "dnszones.h"
#include "seconds.h"
#include "loadnum.h"
#include "extremesandbox.h"
#include "env.h"
#include "pathalloc.h"
#include "pathexec.h"
#include "safechown.h"
#include "coe.h"
#include "byte.h"
#include "fastrandommod.h"


#define INFO "dnszonedownload: info: "
#define FATAL "dnszonedownload: fatal: "
#define WARNING "dnszonedownload: warning: "

void die_usage(void) {
    die_1(100, "dnszonedownload: usage: dnszonedownload /cfgdir /vardir\n");
}

void die_fatal(const char *trouble, const char *d, const char *fn) {

    if (d) {
        if (fn) die_9(111,FATAL,trouble," ",d,"/",fn,": ",e_str(errno),"\n");
        die_7(111,FATAL,trouble," ",d,": ",e_str(errno),"\n");
    }
    die_5(111,FATAL,trouble,": ",e_str(errno),"\n");
}

char *cfgdir, *vardir, *tmpdir;
struct dnszones dnszones;

long long getmtime(const char *fn) {

    struct stat st;

    if (stat(fn, &st) == -1) {
        warn_6(WARNING, "unable to stat file ", fn, ": ", e_str(errno), "\n");
        return -1;
    }
    return st.st_mtime;
}

long long getperiod(const char *fn) {

    long long r;

    if (!loadnum(fn, &r)) {
        warn_6(WARNING, "unable to load number from file ", fn, ": ", e_str(errno), "\n");
        r = 3600;
    }
    if (r < 60) r = 60;
    if (r > 604800) r = 604800;
    return r;
}

#if 0
int touch(char *fn, long long period) {

    time_t ut[2];
    long long fd;

    ut[0] = seconds();
    ut[1] = ut[0] + period;
    if (utime(fn, ut) == -1) {
        if (errno == ENOENT) {
            fd = open_write(fn);
            if (fd == -1) return -1;
            if (close(fd) == -1) return -1;
            if (utime(fn, ut) == 0) return 0;
        }
        return -1;
    }
    return 0;
}
#else
int touch(char *fn, long long period) {

    struct utimbuf ut;
    long long fd;

    ut.actime = seconds();
    ut.modtime = ut.actime + period;
    if (utime(fn, &ut) == -1) {
        if (errno == ENOENT) {
            fd = open_write(fn);
            if (fd == -1) return -1;
            if (close(fd) == -1) return -1;
            if (utime(fn, &ut) == 0) return 0;
        }
        return -1;
    }
    return 0;
}
#endif


int isequal(const char *fnx, const char *fny) {

    struct stat stx, sty;
    int fdx = -1, fdy = -1;
    char *mx = MAP_FAILED;
    char *my = MAP_FAILED;
    long long len = -1;
    int ret = 0;

    if (stat(fnx, &stx) != 0) return 0;
    if (stat(fny, &sty) != 0) return 0;
    if (stx.st_size != sty.st_size) return 0;
    len = stx.st_size;

    fdx = open_read(fnx);
    if (fdx == -1) goto cleanup;

    fdy = open_read(fny);
    if (fdy == -1) goto cleanup;

    mx = mmap(0, len, PROT_READ, MAP_SHARED, fdx, 0);
    if (mx == MAP_FAILED) goto cleanup;

    my = mmap(0, len, PROT_READ, MAP_SHARED, fdy, 0);
    if (my == MAP_FAILED) goto cleanup;

    if (byte_isequal(mx, len, my)) ret = 1;

cleanup:

    if (mx != MAP_FAILED) munmap(mx, len);
    if (my != MAP_FAILED) munmap(my, len);
    if (fdx != -1) close(fdx);
    if (fdy != -1) close(fdy);

    return ret;
}

int isfresh(const char *fn) {

    struct stat st;

    if (stat(fn, &st) != 0) return 0;
    if (st.st_mtime + 604800 < seconds()) return 0;
    return 1;
}


long long basefd;

void mychdir2(const char *d0, const char *d1) {
    if (fchdir(basefd) == -1) die_fatal("unable to change directory to base directroy", 0, 0);
    if (chdir(d0) == -1) die_fatal("unable to change directory to", d0, 0);
    if (chdir(d1) == -1) die_fatal("unable to change directory to", d0, d1);
}

void mychdir(const char *d0) {
    if (fchdir(basefd) == -1) die_fatal("unable to change directory to base directroy", 0, 0);
    if (chdir(d0) == -1) die_fatal("unable to change directory to", d0, 0);
}


void prepairedir(const char *d0, const char *d1) {

    struct stat st;
    long long targetuid;

    if (fchdir(basefd) == -1) die_fatal("unable to change directory to base directroy", 0, 0);

    /* check parent directory */
    if (geteuid() == 0) {
        if (stat(d0, &st) == -1) die_fatal("unable to stat directory", d0, 0);
        if (st.st_uid != 0) {errno = EPERM; die_fatal("root user ownership expected in directory", d0, 0);}
        if (st.st_gid != 0) {errno = EPERM; die_fatal("root group ownership expected in directory", d0, 0);}
    }
    if (chdir(d0) == -1) die_fatal("unable to change directory to", d0, 0);

    if (lstat(d1, &st) == -1) {
        if (errno != ENOENT) die_fatal("unable to stat directory", d0, d1);
        if (mkdir(d1, 0700) == -1) die_fatal("unable to create directory", d0, d1);
    }
    else {
        if (!S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            die_fatal("unable to change directory to", d0, d1);
        }
    }

    if (geteuid() == 0) {
        targetuid = extremesandbox_getuid();
        if (targetuid == -1) die_fatal("unable to extremesandbox_getuid", 0, 0);
        if (safechown(d1, targetuid, targetuid, 1) == -1) die_fatal("unable to change permitions in directory", d0, d1);
    }
    if (chdir(d1) == -1) die_fatal("unable to change directory to", d0, d1);
    return;
}

void prepairedir2(const char *d0, const char *d1) {

    if (fchdir(basefd) == -1) die_fatal("unable to change directory to base directroy", 0, 0);

    if (chdir(d0) == -1) die_fatal("unable to change directory to", d0, 0);
    if (geteuid() == 0) {
        if (safechown(d1, 0, 0, 1) == -1) die_fatal("unable to change permitions in directory", d0, d1);
    }

    if (fchdir(basefd) == -1) die_fatal("unable to change directory to base directroy", 0, 0);
}

static void signalreset(void) {

    signal(SIGTERM, SIG_DFL);
    signal(SIGPIPE, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
    signal(SIGHUP, SIG_DFL);

}

static void redirect(long long fd1, int (op)(const char *), const char *fn) {

    long long fd2;

    close(fd1);
    fd2 = op(fn);
    coe_disable(fd2);
    if (fd2 == -1) die_fatal("unable to open file ", fn, 0);
    if (fd2 != fd1) if (dup2(fd2, fd1) == -1) die_fatal("unable to duplicate filedescriptor", 0, 0);
    return;
}

char abstmpdir[1024];
char abszonedir[1024];

void datacmd_finish(void) {
    prepairedir2(vardir, DNSZONES_DIR_ROOT);
    warn_2(INFO, "data.cdb successfully created\n");
    return;
}

void datacmd_failed(void) {
    prepairedir2(vardir, DNSZONES_DIR_ROOT);
    warn_2(WARNING, "data.cdb not created, dnszonedata failed\n");
}


void datacmd_run(void) {

    char *run[3];
    long long pid;

    if (dnszones.datapid > 0) return;

    pid = fork();
    if (pid == -1) die_fatal("unable to fork", 0, 0);
    if (pid > 0) {
        dnszones.datapid = pid;
        return;
    }

    mychdir2(vardir, DNSZONES_DIR_ZONES);
    if (!getcwd(abszonedir, sizeof abszonedir)) die_fatal("unable to getcwd", 0, 0);

    prepairedir(vardir, DNSZONES_DIR_ROOT);
    if (geteuid() == 0) {
        if (extremesandbox_droproot() == -1) die_fatal("unable to extremesandbox_droproot", 0, 0);
    }

    mychdir2(vardir, DNSZONES_DIR_ROOT);

    run[0] = "dnszonedata";
    run[1] = abszonedir;
    run[2] = 0;

    signalreset();
    setpgid(0,0);
    pathexec(run);
    die_fatal("unable to run program", *run, 0);
}



void cmd_run(struct dnszone *zone) {

    char *run[2] = {"./run", 0};
    char *e[1] = {0};
    long long pid;
    char *path;

    if (zone->pid > 0) return;

    pid = fork();
    if (pid == -1) die_fatal("unable to fork", 0, 0);
    if (pid > 0) {
        zone->pid = pid;
        return;
    }

    /* redirect stdout to file */
    redirect(1, open_trunc, zone->tmpfn);

    prepairedir(tmpdir, zone->fqdn);
    if (geteuid() == 0) {
        if (extremesandbox_droproot() == -1) die_fatal("unable to extremesandbox_droproot", 0, 0);
    }
    if (!getcwd(abstmpdir, sizeof abstmpdir)) die_fatal("unable to getcwd", 0, 0); 

    /* back to cfgdir */
    mychdir2(cfgdir, zone->fqdn);

    /* set environment */
    path = env_get("PATH");
    if (path) if (!pathexec_env("PATH", path)) die_fatal("unable to set environment varible $PATH", 0, 0);
    if (!pathexec_env("ZONE", zone->fqdn2)) die_fatal("unable to set environment varible $ZONE", 0, 0);
    if (!pathexec_env("TMPDIR", abstmpdir)) die_fatal("unable to set environment varible $TMPDIR", 0, 0);

    umask(077);
    signalreset();
    setpgid(0,0);
    environ = e; /* cleanenv */
    pathexec(run);
    die_fatal("unable to run program", *run, 0);
}


void zonecmd_run(struct dnszone *zone) {

    char *run[3];
    long long pid;

    if (zone->zonepid > 0) return;

    pid = fork();
    if (pid == -1) die_fatal("unable to fork", 0, 0);
    if (pid > 0) {
        zone->zonepid = pid;
        return;
    }

    /* redirect stdout and stdin */
    redirect(1, open_trunc, zone->zonetmpfn);
    redirect(0, open_read, zone->fn);

    run[0] = "dnszonefilter";
    run[1] = zone->fqdn2;
    run[2] = 0;

    signalreset();
    setpgid(0,0);
    pathexec(run);
    die_fatal("unable to run program", *run, 0);
}


void zonecmd_finish(struct dnszone *zone) {

    zone->signskip = 0;
    if (isequal(zone->zonetmpfn, zone->zonefn)) zone->signskip = 1;

    if (rename(zone->zonetmpfn, zone->zonefn) == -1) {
        warn_8(WARNING, "unable to rename ", zone->zonetmpfn, " to ", zone->zonefn, ": ", e_str(errno), "\n");
        return;
    }

    warn_4(INFO, "zone ", zone->fqdn2, " filtered with dnszonefilter successfully\n");
    return;
}

void zonecmd_failed(struct dnszone *zone) {

    warn_4(WARNING, "zone ", zone->fqdn2, " not filtered, dnszonefilter failed\n");
}


void postcmd_run(struct dnszone *zone) {

    char *run[4];
    long long pid;

    if (zone->postpid > 0) return;

    if (isfresh(zone->signfn) && zone->signskip) {
        warn_4(INFO, "zone ", zone->fqdn2, " not changed - skipping postcmd\n");
        return;
    }

    pid = fork();
    if (pid == -1) die_fatal("unable to fork", 0, 0);
    if (pid > 0) {
        zone->postpid = pid;
        return;
    }

    /* redirect stdout and stdin */
    redirect(1, open_trunc, zone->signtmpfn);
    redirect(0, open_read, zone->zonefn);

    run[0] = "/bin/sh";
    run[1] = "-ec";
    run[2] = env_get("POSTCMD");
    run[3] = 0;

    signalreset();
    setpgid(0,0);
    pathexec(run);
    die_fatal("unable to run program", *run, 0);
}


void postcmd_finish(struct dnszone *zone) {


    if (rename(zone->signtmpfn, zone->signfn) == -1) {
        warn_8(WARNING, "unable to rename ", zone->signtmpfn, " to ", zone->signfn, ": ", e_str(errno), "\n");
        return;
    }

    warn_6(INFO, "zone ", zone->fqdn2, " postprocessed using '", env_get("POSTCMD"), "' successfully\n");
    return;
}

void postcmd_failed(struct dnszone *zone) {

    warn_6(WARNING, "zone ", zone->fqdn2, " not postprocessed, '", env_get("POSTCMD"), "' failed\n");
}


void cmd_failed(struct dnszone *zone) {

    long long now;
    long long period;
    now = seconds();

    prepairedir2(tmpdir, zone->fqdn);

    if (now > zone->deadline) {
        period = zone->period;
        if (period > 600) period = 600;
        zone->deadline = now + period - period/10 + fastrandommod(period/10);
    }

    if (touch(zone->fn, 0) == -1) {
        warn_6(WARNING, "unable to touch file ", zone->fn, ": ", e_str(errno), "\n");
        return;
    }
    warn_4(WARNING, "zone ", zone->fqdn2, " download failed\n");
}

void cmd_finish(struct dnszone *zone) {

    long long now;

    prepairedir2(tmpdir, zone->fqdn);

    now = seconds();

    if (touch(zone->tmpfn, zone->period) == -1) {
        warn_6(WARNING, "unable to touch file ", zone->tmpfn, ": ", e_str(errno), "\n");
        return;
    }

    if (rename(zone->tmpfn, zone->fn) == -1) {
        warn_8(WARNING, "unable to rename ", zone->tmpfn, " to ", zone->fn, ": ", e_str(errno), "\n");
        return;
    }

    warn_4(INFO, "zone ", zone->fqdn2, " downloaded successfully\n"); 
    zone->deadline = now + zone->period - zone->period/10 + fastrandommod(zone->period/10);
    return;
}



int flagexitasap = 0;
void exitasap(int signum) {
    flagexitasap = 1;
}

void dummyfunc(int signum) {
    /* nothing here */
}


void doit(void) {

    long long i, now, timeout, deadline = 0, newperiodmtime;
    struct pollfd p;

    for(i = 0; i < dnszones.zoneslen; ++i) {
        dnszones.zones[i]->deadline = getmtime(dnszones.zones[i]->fn);
        dnszones.zones[i]->periodmtime = getmtime(dnszones.zones[i]->periodfn);
        dnszones.zones[i]->period = getperiod(dnszones.zones[i]->periodfn);
    }

    while(!flagexitasap) {
        dnszones_collect_zombies(&dnszones);
        dnszones_datarun(&dnszones);

        now = seconds();

        for(i = 0; i < dnszones.zoneslen; ++i) {
            newperiodmtime = getmtime(dnszones.zones[i]->periodfn);
            if (newperiodmtime != dnszones.zones[i]->periodmtime) {
                /* force download */
                dnszones.zones[i]->period = getperiod(dnszones.zones[i]->periodfn);
                dnszones.zones[i]->periodmtime = newperiodmtime;
                dnszones.zones[i]->deadline = now;
            }

            if (deadline > dnszones.zones[i]->deadline || !i)
                deadline = dnszones.zones[i]->deadline;
        }

        if (now < deadline) {
            p.events = POLLIN;
            timeout = 1000 * (deadline - now);
            if (timeout > 60000) timeout = 60000;
            if (timeout < 20)    timeout = 20;
            poll(&p, 0, timeout);
            continue;
        }

        for(i = 0; i < dnszones.zoneslen; ++i) {
            if (now >= dnszones.zones[i]->deadline) {
                if (dnszones.zones[i]->pid > 0) {
                    warn_4(INFO, "unable to run downloader for zone ", dnszones.zones[i]->fqdn, ": downloader is running\n");
                    killpg(dnszones.zones[i]->pid, SIGTERM);
                    dnszones.zones[i]->deadline = now + 10;
                    continue;
                }
                dnszones_run(&dnszones, dnszones.zones[i]);
                dnszones.zones[i]->deadline = now + dnszones.zones[i]->period;
            }
        }
    }
}


int main(int argc, char **argv) {

    if (!argv[0]) die_usage();
    if (!argv[1]) die_usage();
    if (!argv[2]) die_usage();
    cfgdir = argv[1];
    vardir = argv[2];

    tmpdir = pathalloc(vardir, DNSZONES_DIR_TMP, 0);
    if (!tmpdir) die_fatal("unable to allocate memory", 0, 0);

    basefd = open_cwd();
    if (basefd == -1) die_fatal("unable to open current working directory", 0, 0);

    /* lock cfg directory */
    mychdir(cfgdir);
    if (open_lock("lock") == -1) die_fatal("unable to lock file", cfgdir, "lock");

    /* read configuration */
    if (fchdir(basefd) == -1) die_fatal("unable to change directory to base directroy", 0, 0);
    if (!dnszones_dirinit(&dnszones, cfgdir, vardir, WARNING)) die_fatal("unable to read configuration", 0, 0);

    /* register callbacks */
    dnszones_op_register(&dnszones, cmd_run, cmd_finish, cmd_failed);
    dnszones_zoneop_register(&dnszones, zonecmd_run, zonecmd_finish, zonecmd_failed);
    if (env_get("POSTCMD")) {
        dnszones_postop_register(&dnszones, postcmd_run, postcmd_finish, postcmd_failed);
    }
    if (env_get("MAKECDB")) {
        dnszones_dataop_register(&dnszones, datacmd_run, datacmd_finish, datacmd_failed);
    }

    signal(SIGPIPE, SIG_IGN);
    signal(SIGTERM, exitasap);
    signal(SIGCHLD, dummyfunc);
    signal(SIGHUP, dummyfunc);

    doit();

    dnszones_finish(&dnszones);

    warn_2(INFO, "finished\n");
    _exit(0);
}
