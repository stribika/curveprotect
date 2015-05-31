/*
20130607
Jan Mojzis
Public domain.
*/

#include <stdio.h>
#include <unistd.h>
#include "strtoip.h"

void err(const char *n, const char *e) {
    fprintf(stdout, "%s: failed: unable to parse ", n);
    fprintf(stdout, "%s\n", e);
    fflush(stdout);
    _exit(111);
}

void errbad(const char *n, const char *e) {
    fprintf(stdout, "%s: failed: strtoip accepts malformed ip ", n);
    fprintf(stdout, "%s\n", e);
    fflush(stdout);
    _exit(111);
}

struct vectors {
    const char *ip;
    const char *ipstr;
} testvectors[] = {

    { "\000\000\000\000\000\000\000\000\000\000\377\377\000\000\000\000", "0.0.0.0" },
    { "\000\000\000\000\000\000\000\000\000\000\377\377\000\000\000\000", "::ffff:0.0.0.0" },
    { "\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377", "255.255.255.255" },
    { "\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377", "::ffff:255.255.255.255" },
    { "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377", "ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255" },
    { "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377", "ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff" },
    { "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377", "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:255.255.255.255" },
    { "\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377\377", "FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF" },
    { "\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377", "::255.255.255.255" },
    { 0, 0 }
};

const char *badipstr[] = {

    /* bad character */
    "0.0.0.0.",
    ".0.0.0.0",
    "::.0.0.0.0",
    "\\0.0.0.0",
    "0.0.0\\.0",
    "0.0.0\\.0",
    "::0.0.0.ff",

    /* overflow */
    "0.0.0.256",
    "::0.0.0.256",
    "::0:g:0",
    "::0:10ffff:0",

    /* too may numbers */
    "0.0.0.0.0",
    "::0:0:0:0:0:0:0:0",
    "0:0:0:0:0:0:0:0:0",
    "0:0:0:0:0:0:0:0:0.0.0.0",
    "0:0:0:0:0:0:0:0.0.0.0",
#ifdef TODO
    "::0:0:0:0:0:0:0", /* ????? */
#endif

    /* ip too short */
    "0:0:0:0:0:0:0",
    "0:0:0:0:0:0.0.0.0",

    /* extra ':' */
    "::0::",
    ":0:::0:0",
    "0:0.0.0.0::",

    0
};

char ipstr[1024];

void test_zero(const char *name) {

    long long i, j, k;
    long long max, num, suf;
    char *p;
    unsigned char ip[16];

    for (num = 1; num < 10; ++num) {
        for (max = 0; max < 8; ++max) {
            for (i = 0; i < max; ++i) {
                for (suf = 0; suf < 2; ++suf) {
                    p = ipstr;
                    for (j = 0; j < i; ++j) {
                        for (k = 0; k < num; ++k) *p++ = '0';
                        *p++ = ':';
                    }
                    if (i != j || i == 0 || j == 0) *p++ = ':';
                    if (i == j && j == max - 1) *p++ = ':';
                    for (j = i; j < max - 1; ++j) {
                        *p++ = ':';
                        for (k = 0; k < num; ++k) *p++ = '0';
                    }
                    /* suffix 0.0.0.0 */
                    if (max < 7) {
                        if (suf > 0) {
                            if (!(i == j && j == max - 1)) *p++ = ':';
#ifdef TODO
                            for (k = 0; k < num; ++k)
#endif
                                *p++ = '0'; *p++ = '.';
#ifdef TODO
                            for (k = 0; k < num; ++k)
#endif
                                *p++ = '0'; *p++ = '.';
#ifdef TODO
                            for (k = 0; k < num; ++k)
#endif
                                *p++ = '0'; *p++ = '.';
#ifdef TODO
                            for (k = 0; k < num; ++k)
#endif
                                *p++ = '0';
                        }
                    }
                    *p++ = 0;
#if 1
                    if (!strtoip(ip, ipstr)) err(name, ipstr);
                    for (k = 0; k < 16; ++k) if (ip[k] != 0) err(name, ipstr);
#else
                    printf("%s\n", ipstr);
#endif
                }
            }
        }
    }

}

void test_vectors(const char *name) {

    long long i, j;
    unsigned char ip[16];
    const unsigned char *x;

    for (i = 0;testvectors[i].ipstr; ++i) {
        x = (unsigned char *)testvectors[i].ip;
        if (!strtoip(ip, testvectors[i].ipstr)) err(name, testvectors[i].ipstr);
        for (j = 0; j < 16; ++j) if (ip[j] != x[j]) err(name, testvectors[i].ipstr);
    }
}

void test_bad(const char *name) {

    long long i;
    unsigned char ip[16];

    for (i = 0; badipstr[i]; ++i) {
        if (strtoip(ip, badipstr[i])) errbad(name, badipstr[i]);
    }
}


int main() {

    test_zero("strtoiptest: zero test");
    test_vectors("strtoiptest: vectors test");
    test_bad("strtoiptest: malformed IP test");

    _exit(0);
}
