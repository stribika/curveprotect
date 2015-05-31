#ifndef _DNSZONES_H____
#define _DNSZONES_H____

#define DNSZONES_DIR_ORIG "out"
#define DNSZONES_DIR_ORIGTMP "tmp1"
#define DNSZONES_DIR_ZONES "zones"
#define DNSZONES_DIR_ZONESTMP "tmp2"
#define DNSZONES_DIR_ZONESPOST "postzones"
#define DNSZONES_DIR_ZONESPOSTTMP "tmp3"
#define DNSZONES_DIR_TMP "tmp"
#define DNSZONES_DIR_ROOT "root"
#define DNSZONES_DIR_EMPTY "empty"
#define DNSZONES_FILE_PERIOD "period"

struct dnszone {

    /* cfg */
    char *fqdn;
    char *fqdn2;
    unsigned char *q;
    long long pid;
    long long postpid;
    long long zonepid;
    long long period;
    long long periodmtime;
    char *periodfn;

    /* var */
    char *fn;
    char *tmpfn;
    char *zonefn;
    char *zonetmpfn;
    char *signfn;
    char *signtmpfn;
    int signskip;

    long long deadline;
};

struct dnszones {
    struct dnszone **zones;
    long long zonesalloc;
    long long zoneslen;

    long long datapid;
    int dataflag;

    void (*op1)();
    void (*op2)();
    void (*op3)();
    void (*zoneop1)();
    void (*zoneop2)();
    void (*zoneop3)();
    void (*postop1)();
    void (*postop2)();
    void (*postop3)();
    void (*dataop1)();
    void (*dataop2)();
    void (*dataop3)();

    const char *warning;
};

extern void dnszones_collect_zombies(struct dnszones *);
extern void dnszones_finish(struct dnszones *);
extern int dnszones_namefind(struct dnszones *, char *);
extern int dnszones_exist(struct dnszones *, unsigned char *);

extern int dnszones_dirinit(struct dnszones *, const char *, const char *, const char *);
extern int dnszones_fileinit(struct dnszones *, const char *, const char *);

extern void dnszones_op_register(struct dnszones *dnszones, void (*op1)(), void (*op2)(), void (*op3)());
extern void dnszones_zoneop_register(struct dnszones *dnszones, void (*op1)(), void (*op2)(), void (*op3)());
extern void dnszones_postop_register(struct dnszones *dnszones, void (*op1)(), void (*op2)(), void (*op3)());
extern void dnszones_dataop_register(struct dnszones *dnszones, void (*dataop1)(), void (*dataop2)(), void (*dataop3)());
extern void dnszones_run(struct dnszones *dnszones, struct dnszone *zone);
extern void dnszones_datarun(struct dnszones *dnszones);


#endif
