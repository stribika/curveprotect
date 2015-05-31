/*
20130606
Jan Mojzis
Public domain.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "numtostr.h"

void err(const char *n, const char *e) {
    fprintf(stdout, "%s: failed: ", n);
    fprintf(stdout, "%s\n", e);
    fflush(stdout);
    _exit(111);
}

void err2(const char *n, const char *e1, const char *e2) {
    fprintf(stdout, "%s: failed: ", n);
    fprintf(stdout, "\"%s\" != \"%s\"\n", e1, e2);
    fflush(stdout);
    _exit(111);
}

char buf[NUMTOSTR_LEN + 16];

void test_buf(const char *name) {

    long long i;
    unsigned long long ull = 0;
    --ull;

    for (i = 0; i < sizeof buf; ++i) buf[i] = (char)0xff;
    numtostr(buf + 8, (long long)ull);
    for (i = 0; i < 8; ++i) if (buf[i] != (char)0xff) err(name, "numtostr writes before output");
    for (i = 0; i < 8; ++i) if (buf[i + NUMTOSTR_LEN + 8] != (char)0xff) err(name, "numtostr writes after output");
    if (buf[NUMTOSTR_LEN + 7] != 0) err(name, "numtostr doesn't write '\0' correctly");

    for (i = 0; i < sizeof buf; ++i) buf[i] = (char)0x00;
    numtostr(buf + 8, (long long)ull);
    for (i = 0; i < 8; ++i) if (buf[i] != 0x00) err(name, "numtostr writes before output");
    for (i = 0; i < 8; ++i) if (buf[i + NUMTOSTR_LEN + 8] != 0x00) err(name, "numtostr writes after output");
    if (buf[NUMTOSTR_LEN + 7] != 0) err(name, "numtostr doesn't write '\0' correctly");

    if ((NUMTOSTR_LEN) < sizeof("-170141183460469231731687303715884105728")) err(name, "NUMTOSTR_LEN too small");
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

void test_minmax(const char *name) {

    char *llmax;
    char *llmin;
    long long llmaxnum;
    long long llminnum;
    char *x;


    switch(sizeof(long long)) {
        case 16:
            llmax   = "170141183460469231731687303715884105727";
            llmin   = "-170141183460469231731687303715884105728";
            llmaxnum = int128max();
            llminnum = -llmaxnum - 1LL;
            break;
        case 8:
            llmax   = "9223372036854775807";
            llmin   = "-9223372036854775808";
            llmaxnum = 9223372036854775807LL;
            llminnum = -llmaxnum - 1LL;
            break;
        default:
            err(name, "unsupported long long size");
    }

    x = numtostr(0, llmaxnum);
    if (strcmp(x, llmax)) err2(name, x, llmax);

    x = numtostr(0, llminnum);
    if (strcmp(x, llmin)) err2(name, x, llmin);
}



int main() {

    test_buf("numtostrtest: buffer test");
    test_minmax("numtostrtest: minmax test");

    _exit(0);
}
