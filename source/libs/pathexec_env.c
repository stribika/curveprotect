#include "alloc.h"
#include "pathexec.h"
#include "str.h"
#include "byte.h"
#include "env.h"

static char **plus = 0;
static long long pluslen = 0;
static long long plusalloc = 0;

static int findkey(const char *y, const char *x, long long xlen) {

    long long i;

    for(i = 0; i < xlen; ++i) {
        if (*y++ != x[i]) return 0;
    }
    if (*y == '=') return 1;
    return 0;
}

static int plus_add(char *x) {

    char **newplus;

    if (pluslen + 1 > plusalloc) {
        while (pluslen + 1 > plusalloc)
            plusalloc = 1 + 2 * plusalloc;
        newplus = (char **)alloc(plusalloc * sizeof(char *));
        if (!newplus) return 0;
        if (plus) {
            byte_copy(newplus, pluslen * sizeof(char *), plus);
            alloc_free(plus);
        }
        plus = newplus;
    }
    if (!x) return 1;
    plus[pluslen++] = x;
    return 1;
}

int pathexec_env(char *s, char *t) {

    long long slen = 0, tlen = 0;
    char *x, *p;

    if (!s) return 1;
    slen = str_len(s);
    if (t) tlen = str_len(t);

    x = (char *)alloc(slen + tlen + 2);
    if (!x) return 0;
    p = x;

    byte_copy(p, slen, s); p += slen; *p++ = '=';
    byte_copy(p, tlen, t); p += tlen; *p++ = 0;

    if (!plus_add(x)) { alloc_free(x); return 0; }
    return 1;
}


void pathexec(char **argv) {

    char **e;
    long long elen, i, j, split;

    for (elen = 0; environ[elen]; ++elen);
    elen += pluslen + 1;

    e = (char **)alloc((elen + 1) * sizeof(char *));
    if (!e) return;

    elen = 0;
    for (i = 0; environ[i]; ++i) e[elen++] = environ[i];

    for (i = 0; i < pluslen; ++i) {
        split = str_chr(plus[i],'=');
        for (j = 0; j < elen; ++j) {
            if (findkey(e[j], plus[i], split)) {
                e[j] = e[--elen];
                break;
            }
        }
        e[elen++] = plus[i];
    }
    e[elen] = 0;

    pathexec_run(*argv, argv, e);
    alloc_free(e);
}
