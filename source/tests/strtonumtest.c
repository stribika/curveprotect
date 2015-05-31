/*
20130606
Jan Mojzis
Public domain.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "strtonum.h"

void err(const char *n, const char *e) {
    fprintf(stdout, "%s: failed: ", n);
    fprintf(stdout, "%s\n", e);
    fflush(stdout);
    _exit(111);
}

char *zeronums[] = {
    "+0",
    "+0+",
    "-0",
    "-0-",
    "0",
    "0.",
    "0.1",
    "00000000000000000000000000000000000000000000000000",
    "00000000000000000000000000000000000000000000000000.",
    0
};

char *oknums[] = {
    "-01",
    "-01.01",
    "-0000000000000000000000000000000000000000000000001",
    "-1a",
    "-1-",
    0
};

char *badnums[] = {
    "+",
    "+a23",
    "-",
    "-a23",
    "",
    "a23",
    "++1",
    "--1",
    0
};

void test_zero(const char *name) {


    long long l, i;
    int r;

    for (i = 0; zeronums[i]; ++i) {
        r = strtonum(&l, zeronums[i]);
        if (!r || l != 0) err(name, "strtonum doesn't accept 0");
    }

}

void test_one(const char *name) {

    long long l, i;
    int r;

    for (i = 0; oknums[i]; ++i) {
        r = strtonum(&l, oknums[i]);
        if (!r || l != -1) err(name, "strtonum doesn't accept -1");
    }
}

void test_badnum(const char *name) {

    long long l, i;
    int r;

    for (i = 0; badnums[i]; ++i) {
        r = strtonum(&l, badnums[i]);
        if (r) err(name, "strtonum accepts bad string");
    }
    r = strtonum(&l, 0);
    if (r) err(name, "strtonum accepts bad string");
}

long long int128max(void) {

    long long x = 9223372036854775807LL;

    x <<= 32;
    x <<= 32;
    x += 9223372036854775807LL;
    x += 9223372036854775807LL;
    x += 1;

    return x;
}

void test_overflow(const char *name) {

    char *llover;
    char *llunder;
    int r;
    long long l;

    switch (sizeof(long long)) {
        case 16:
            llover  = "170141183460469231731687303715884105728";
            llunder = "-170141183460469231731687303715884105729";
            break;
        case 8:
            llover  = "9223372036854775808";
            llunder = "-9223372036854775809";
            break;
        default:
            err(name, "unsupported long long size");
    }

    r = strtonum(&l, llover);
    if (r) err(name, "strtonum accepts too-large number");

    r = strtonum(&l, llunder);
    if (r) err(name, "strtonum accepts too-small number");
}

void test_minmax(const char *name) {

    char *llmax;
    char *llmin;
    long long llmaxnum;
    long long llminnum;
    int r;
    long long l;

    switch (sizeof(long long)) {
        case 16:
            llmax    = "170141183460469231731687303715884105727";
            llmin    = "-170141183460469231731687303715884105728";
            llmaxnum = int128max();
            llminnum = -llmaxnum - 1LL;
            break;
        case 8:
            llmax    = "9223372036854775807";
            llmin    = "-9223372036854775808";
            llmaxnum = 9223372036854775807LL;
            llminnum = -llmaxnum - 1LL;
            break;
        default:
            err(name, "unsupported long long size");
    }

    r = strtonum(&l, llmax);
    if (!r || l != llmaxnum) err(name, "strtonum doesn't accept max number");

    r = strtonum(&l, llmin);
    if (!r || l != llminnum) err(name, "strtonum doesn't accept min number");
}


int main() {

    test_zero("strtonumtest: 0 test");
    test_one("strtonumtest: -1 test");
    test_badnum("strtonumtest: bad number test");
    test_overflow("strtonumtest: overflow test");
    test_minmax("strtonumtest: minmax test");

    _exit(0);
}
