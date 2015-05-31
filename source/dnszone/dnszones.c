#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include "direntry.h"
#include "e.h"
#include "byte.h"
#include "str.h"
#include "dns.h"
#include "pathalloc.h"
#include "warn.h"
#include "open.h"
#include "dnszones.h"
#include "numtostr.h"
#include "deepsleep.h"
#include "alloc.h"


static int dnszones_add(struct dnszones *dnszones, char *fqdn, unsigned char **q, const char *cfgdir, const char *vardir) {

    struct dnszone **newzones;
    struct dnszone *z;
    char *x;
    char *fqdn2;

    /* allocate space */
    z = (struct dnszone *)alloc(sizeof(struct dnszone));
    if (!z) return 0;
    byte_zero(z, sizeof(struct dnszone));

    /* DNS name */
    z->q = *q; *q = 0; /* XXX */

    /* fqdn */
    x = pathalloc(fqdn, 0, 0);
    if (!x) return 0;
    z->fqdn = x;

    /* fqdn2 */
    fqdn2 = fqdn;
    if (str_equal(fqdn, "@")) fqdn2 = ".";
    x = pathalloc(fqdn2, 0, 0);
    if (!x) return 0;
    z->fqdn2 = x;

    if (vardir) {

        /* period file */
        x = pathalloc(cfgdir, fqdn, DNSZONES_FILE_PERIOD);
        if (!x) return 0;
        z->periodfn = x;

        /* orig file */
        x = pathalloc(vardir, DNSZONES_DIR_ORIG, fqdn);
        if (!x) return 0;
        z->fn = x;

        /* orig temporary file */
        x = pathalloc(vardir, DNSZONES_DIR_ORIGTMP, fqdn);
        if (!x) return 0;
        z->tmpfn = x;

        /* zone file */
        x = pathalloc(vardir, DNSZONES_DIR_ZONES, fqdn);
        if (!x) return 0;
        z->zonefn = x;

        /* zone temporary file */
        x = pathalloc(vardir, DNSZONES_DIR_ZONESTMP, fqdn);
        if (!x) return 0;
        z->zonetmpfn = x;

        /* signed zone file */
        x = pathalloc(vardir, DNSZONES_DIR_ZONESPOST, fqdn);
        if (!x) return 0;
        z->signfn = x;

        /* signed zone temporary file */
        x = pathalloc(vardir, DNSZONES_DIR_ZONESPOSTTMP, fqdn);
        if (!x) return 0;
        z->signtmpfn = x;
    }

    /* add */
    if (dnszones->zoneslen + 1 > dnszones->zonesalloc) {
        dnszones->zonesalloc = 128 + 2*dnszones->zonesalloc;
        newzones = (struct dnszone **)alloc(dnszones->zonesalloc * sizeof(struct dnszone *));
        if (!newzones) return 0;
        if (dnszones->zones) {
            byte_copy(newzones, dnszones->zoneslen * sizeof(struct dnszone *), dnszones->zones);
            alloc_free(dnszones->zones);
        }
        dnszones->zones = newzones;
    }
    dnszones->zones[dnszones->zoneslen++] = z;
    return 1;
}


int dnszones_namefind(struct dnszones *dnszones,char *fqdn) {

    long long i;

    for(i = 0; i < dnszones->zoneslen; ++i) {
        if (str_equal(fqdn, dnszones->zones[i]->fqdn)) return 1;
    }
    return 0;
}


int dnszones_exist(struct dnszones *dnszones, unsigned char *q) {

    long long i;

    for(i = 0; i < dnszones->zoneslen; ++i) {
        if (dns_domain_equal(q, dnszones->zones[i]->q)) {errno = EEXIST; return 1;}
    }
    return 0;
}

static int dnszones_fileinit2(struct dnszones *dnszones, const char *cfgdir, const char *vardir, const char *warning) {

    direntry *d;
    DIR *dir;
    struct stat st;
    const char *fqdn;
    unsigned char *q = 0;

    dir = opendir(".");
    if (!dir) return 0;

    for (;;) {
        errno = 0;
        d = readdir(dir);
        if (!d) break;
        if (d->d_name[0] == '.') continue;

        fqdn = d->d_name;
        if (str_equal(d->d_name, "@")) fqdn = ".";

        if (stat(d->d_name, &st) == -1) {
            warn_6(warning, "unable to stat ", d->d_name, ": ", e_str(errno), "\n");
            continue;
        }
        if (!S_ISREG(st.st_mode)) continue;

        if (!dns_domain_fromdot(&q, (unsigned char *)fqdn, str_len(fqdn))) {
            warn_6(warning, "unable init zone ", fqdn, ": ", e_str(errno), "\n");
            continue;
        }

        if (dnszones_exist(dnszones, q)) {
            warn_4(warning, "unable to init zone ", fqdn, ": exist\n");
            continue;
        }

        if (!dnszones_add(dnszones, d->d_name, &q, cfgdir, vardir)) {
            warn_6(warning, "unable init zone ", fqdn, ": ", e_str(errno), "\n");
            break;
        }
    }
    if (errno) {closedir(dir); return 0;}
    closedir(dir);
    return 1;
}

static int dnszones_dirinit2(struct dnszones *dnszones, const char *cfgdir, const char *vardir, const char *warning) {

    direntry *d;
    DIR *dir;
    struct stat st;
    const char *fqdn;
    unsigned char *q = 0;

    dir = opendir(".");
    if (!dir) return 0;

    for (;;) {
        errno = 0;
        d = readdir(dir);
        if (!d) break;
        if (d->d_name[0] == '.') continue;

        fqdn = d->d_name;
        if (str_equal(d->d_name, "@")) fqdn = ".";

        if (stat(d->d_name, &st) == -1) {
            warn_6(warning, "unable to stat ", d->d_name, ": ", e_str(errno), "\n");
            continue;
        }
        if (!S_ISDIR(st.st_mode)) continue;

        if (!dns_domain_fromdot(&q, (unsigned char *)fqdn, str_len(fqdn))) {
            warn_6(warning, "unable init zone ", fqdn, ": ", e_str(errno), "\n");
            continue;
        }

        if (dnszones_exist(dnszones, q)) {
            warn_4(warning, "unable to init zone ", fqdn, ": exist\n");
            continue;
        }

        if (!dnszones_add(dnszones, d->d_name, &q, cfgdir, vardir)) {
            warn_6(warning, "unable init zone ", fqdn, ": ", e_str(errno), "\n");
            break;
        }

    }
    if (errno) {closedir(dir); return 0;}
    closedir(dir);
    return 1;
}



static int dnszones_init(struct dnszones *dnszones, const char *cfgdir, const char *vardir, const char *warning, int (op)()) {

    int r;
    long long basefd;

    byte_zero(dnszones, sizeof (struct dnszones));

    /* warning */
    dnszones->warning = warning;

    basefd = open_cwd();
    if (basefd == -1) return -1;

    if (chdir(cfgdir) == -1) {
        close(basefd);
        return -1;
    }

    r = op(dnszones, cfgdir, vardir, warning);

    if (fchdir(basefd) == -1) r = -1;
    close(basefd);
    return r;
}

int dnszones_dirinit(struct dnszones *dnszones, const char *cfgdir, const char *vardir, const char *warning) {
    return dnszones_init(dnszones, cfgdir, vardir, warning, dnszones_dirinit2);
}

int dnszones_fileinit(struct dnszones *dnszones, const char *zonedir, const char *warning) {
    return dnszones_init(dnszones, zonedir, 0, warning, dnszones_fileinit2);
}

int okstatus(const char *warning, const char *what, int childstatus, struct dnszone *zone) {

    if (!WIFEXITED(childstatus)) {
        if (zone) {
            warn_7(warning, what, " for zone ", zone->fqdn2, " killed by signal ", numtostr(0, WTERMSIG(childstatus)), "\n");
        }
        else {
            warn_5(warning, what, " killed by signal ", numtostr(0, WTERMSIG(childstatus)), "\n");
        }
        return 0;
    }

    if (WEXITSTATUS(childstatus) != 0) {
        if (zone) {
            warn_7(warning, what, " for zone ", zone->fqdn2, " exited with status ", numtostr(0, WEXITSTATUS(childstatus)), "\n");
        }
        else {
            warn_5(warning, what, " exited with status ", numtostr(0, WEXITSTATUS(childstatus)), "\n");
        }
        return 0;
    }
    return 1;
}

void dnszones_collect_zombies(struct dnszones *dnszones) {

    long long pid, i;
    int childstatus;

    for(;;) {
        pid = waitpid(-1, &childstatus, WNOHANG);
        if (pid <= 0) break;

        if (dnszones->datapid == pid) {
            dnszones->datapid = 0;
            if (okstatus(dnszones->warning, "data maker",  childstatus, 0)) {
                if (dnszones->dataop2) dnszones->dataop2();
            }
            else {
                if (dnszones->dataop3) dnszones->dataop3();
            }
            continue;
        }

        for(i = 0; i < dnszones->zoneslen; ++i) {
            if (dnszones->zones[i]->pid == pid) {
                dnszones->zones[i]->pid = 0;
                if (okstatus(dnszones->warning, "downloader", childstatus, dnszones->zones[i])) {
                    if (dnszones->op2) dnszones->op2(dnszones->zones[i]);
                    if (dnszones->zoneop1) dnszones->zoneop1(dnszones->zones[i]);
                }
                else {
                    if (dnszones->op3) dnszones->op3(dnszones->zones[i]);
                }
                break;
            }
            if (dnszones->zones[i]->zonepid == pid) {
                dnszones->zones[i]->zonepid = 0;
                if (okstatus(dnszones->warning, "dnszonefilter", childstatus, dnszones->zones[i])) {
                    if (dnszones->zoneop2) dnszones->zoneop2(dnszones->zones[i]);
                    if (dnszones->postop1) dnszones->postop1(dnszones->zones[i]);
                    dnszones->dataflag = 1;
                }
                else {
                    if (dnszones->zoneop3) dnszones->zoneop3(dnszones->zones[i]);
                }
                break;
            }
            if (dnszones->zones[i]->postpid == pid) {
                dnszones->zones[i]->postpid = 0;
                if (okstatus(dnszones->warning, "dnszonesign", childstatus, dnszones->zones[i])) {
                    if (dnszones->postop2) dnszones->postop2(dnszones->zones[i]);
                }
                else {
                    if (dnszones->postop3) dnszones->postop3(dnszones->zones[i]);
                }
                break;
            }
        }
    }
    return;
}

static void dnszones_killall(struct dnszones *dnszones, long long sig) {

    long long i;

    if (dnszones->datapid > 0)
        killpg(dnszones->datapid, sig);

    for(i = 0; i < dnszones->zoneslen; ++i) {
        if (dnszones->zones[i]->pid > 0)
            killpg(dnszones->zones[i]->pid, sig);
        if (dnszones->zones[i]->zonepid > 0)
            killpg(dnszones->zones[i]->zonepid, sig);
        if (dnszones->zones[i]->postpid > 0)
            killpg(dnszones->zones[i]->postpid, sig);
    }
    return;
}


static int dnszones_haschilds(struct dnszones *dnszones) {

    long long i;

    if (dnszones->datapid != 0) return 1;

    for(i = 0; i < dnszones->zoneslen; ++i) {
        if (dnszones->zones[i]->pid != 0) return 1;
        if (dnszones->zones[i]->postpid != 0) return 1;
        if (dnszones->zones[i]->zonepid != 0) return 1;
    }
    return 0;
}


void dnszones_finish(struct dnszones *dnszones) {

    long long i;
    long long s[5] = {SIGTERM, SIGALRM, SIGTERM, SIGTERM, SIGKILL};

    for(i = 0; i < 5; ++i) {
        dnszones_collect_zombies(dnszones);
        if (!dnszones_haschilds(dnszones)) break;
        dnszones_killall(dnszones, s[i]);
        deepsleep(1);
    }
}


void dnszones_op_register(struct dnszones *dnszones, void (*op1)(), void (*op2)(), void (*op3)()) {
    dnszones->op1 = op1;
    dnszones->op2 = op2;
    dnszones->op3 = op3;
}

void dnszones_zoneop_register(struct dnszones *dnszones, void (*op1)(), void (*op2)(), void (*op3)()) {
    dnszones->zoneop1 = op1;
    dnszones->zoneop2 = op2;
    dnszones->zoneop3 = op3;
}

void dnszones_postop_register(struct dnszones *dnszones, void (*op1)(), void (*op2)(), void (*op3)()) {
    dnszones->postop1 = op1;
    dnszones->postop2 = op2;
    dnszones->postop3 = op3;
}

void dnszones_dataop_register(struct dnszones *dnszones, void (*op1)(), void (*op2)(), void (*op3)()) {
    dnszones->dataop1 = op1;
    dnszones->dataop2 = op2;
    dnszones->dataop3 = op3;
    dnszones->dataflag = 1;
}

void dnszones_run(struct dnszones *dnszones, struct dnszone *zone) {
    dnszones->op1(zone);
}

void dnszones_datarun(struct dnszones *dnszones) {
    if (dnszones->dataflag == 0) return;
    if (dnszones->datapid > 0) return;
    dnszones->dataflag = 0;
    if (dnszones->dataop1) dnszones->dataop1();
}

