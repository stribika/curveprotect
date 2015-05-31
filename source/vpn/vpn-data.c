/*
 * 20110906
 * Jan Mojzis
 * Public domain.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "open.h"
#include "stralloc.h"
#include "buffer.h"
#include "cdb_make.h"
#include "getln.h"
#include "strerr.h"
#include "byte.h"
#include "ip4.h"
#include "hexparse.h"

#define FATAL "vpn-data: fatal: "

static void die_datatmp(void){strerr_die2sys(111,FATAL,"unable to create data.tmp: ");}
static void die_nomem(void){strerr_die1sys(111,FATAL);}

static buffer b;
static char bspace[1024];

static stralloc line = {0};
static int match = 1;

#define NUMFIELDS 10
static stralloc f[NUMFIELDS];

static struct cdb_make cdb;

static stralloc key = {0};
static stralloc val = {0};

static char ip[4];
static unsigned char pk[32];

int main(int argc, const char **argv, const char **envp){

    int fddata;
    int fdcdb;
    unsigned int i,j,k;
    char ch;

    umask(022);

    fddata = open_read("data");
    if (fddata == -1)
        strerr_die2sys(111,FATAL,"unable to open data: ");

    buffer_init(&b,buffer_unixread,fddata,bspace,sizeof bspace);

    fdcdb = open_trunc("data.tmp");
    if (fdcdb == -1) die_datatmp();
    if (cdb_make_start(&cdb,fdcdb) == -1) die_datatmp();

    while (match){
        if (getln(&b,&line,&match,'\n') == -1)
            strerr_die2sys(111,FATAL,"unable to read line: ");

        while (line.len) {
            ch = line.s[line.len - 1];
            if ((ch != ' ') && (ch != '\t') && (ch != '\n')) break;
            --line.len;
        }
        if (!line.len) continue;
        if (line.s[0] == '#') continue;

        j = 1;
        for (i = 0;i < NUMFIELDS;++i) {
            if (j >= line.len) {
                if (!stralloc_copys(&f[i],"")) die_nomem();
            }
            else {
                k = byte_chr(line.s + j,line.len - j,':');
                if (!stralloc_copyb(&f[i],line.s + j,k)) die_nomem();
                j += k + 1;
            }
        }

        if (f[1].len == 64){
            if (!stralloc_0(&f[1])) die_nomem();
            if (!hexparse(pk, sizeof pk, f[1].s)) break; /* XXX */
            if (!stralloc_copyb(&key, (char *)pk, sizeof pk)) die_nomem();
        }
        else if (f[1].len == 0) {
            if (!stralloc_copys(&key, "")) die_nomem();
        }
        else {
            /* XXX */
            continue;
        }

        switch(line.s[0]){
            case 'i':
                if (!f[1].len) break;
            case 'r':
                if (!stralloc_0(&f[2])) die_nomem();
                if (!stralloc_0(&f[3])) die_nomem();
                if (ip4_scan(f[2].s,ip)){
                    if (!stralloc_copyb(&val,line.s,1)) die_nomem();
                    if (!stralloc_catb(&val,ip,4)) die_nomem();
                    if (ip4_scan(f[3].s,ip)){
                        if (!stralloc_catb(&val,ip,4)) die_nomem();
                        if (cdb_make_add(&cdb,key.s,key.len,val.s,val.len) == -1)
                            die_datatmp();
                    }
                }
                break;
            default:
                break;
        }
    }


    if (cdb_make_finish(&cdb) == -1) die_datatmp();
    if (fsync(fdcdb) == -1) die_datatmp();
    if (close(fdcdb) == -1) die_datatmp(); /* NFS stupidity */
    if (rename("data.tmp","data.cdb") == -1)
        strerr_die2sys(111,FATAL,"unable to move data.tmp to data.cdb: ");

  _exit(0);

}

