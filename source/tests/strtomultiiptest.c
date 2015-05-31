/*
20130607
Jan Mojzis
Public domain.
*/

#include <stdio.h>
#include <unistd.h>
#include "strtomultiip.h"


void err(const char *n, const char *e) {
    fprintf(stdout, "%s: failed: unable to parse ", n);
    fprintf(stdout, "%s\n", e);
    fflush(stdout);
    _exit(111);
}

char ipstr[2 * STRTOMULTIIP_BUFSIZE];
const char *k = "::0.0.0.0,::ffff:ffff:ffff:ffff:ffff:ffff";
unsigned char ip[64];

void test_overflow(const char *name) {

    long long i, j;

    for (i = 0; i < sizeof ipstr; ++i) ipstr[i] = 0;
    for (i = 0; i < sizeof ip; ++i) ip[i] = 0;

    for (i = 0; i < (STRTOMULTIIP_BUFSIZE - 20); ++i) ipstr[i] = '0';
    for (j = 0; k[j]; ++j) ipstr[i + j] = k[j];
    ipstr[i + j] = 0;

    if (strtomultiip(ip, sizeof ip, ipstr) != 16) err(name, ipstr);
    for (i = 0; i < sizeof ip; ++i) if (ip[i] != 0) err(name, ipstr);
}

unsigned char *expectedip = (unsigned char *)"\000\000\000\000\000\000\000\000\000\000\377\377\000\000\000\000\000\000\000\000\000\000\000\000\000\000\377\377\377\377\377\377";

char *multiipstr[] = {

    "0.0.0.0.255.255.255.255",
    "0.0.0.0.255.255.255.255.1.2.3",

    "0.0.0.0,255.255.255.255",
    "0.0.0.0 255.255.255.255",
    "0.0.0.0\n255.255.255.255",
    "0.0.0.0\t255.255.255.255",

    "::ffff:0.0.0.0,255.255.255.255",
    "::ffff:0.0.0.0 255.255.255.255",
    "::ffff:0.0.0.0\n255.255.255.255",
    "::ffff:0.0.0.0\t255.255.255.255",

    "::ffff:0.0.0.0,::ffff:255.255.255.255",
    "::ffff:0.0.0.0 ::ffff:255.255.255.255",
    "::ffff:0.0.0.0\n::ffff:255.255.255.255",
    "::ffff:0.0.0.0\t::ffff:255.255.255.255",

    "::ffff:0:0,::ffff:ffff:ffff",
    "::ffff:0:0 ::ffff:ffff:ffff",
    "::ffff:0:0\n::ffff:ffff:ffff",
    "::ffff:0:0\t::ffff:ffff:ffff",

    "::ffff:0:0,255.255.255.255",
    "::ffff:0:0 255.255.255.255",
    "::ffff:0:0\n255.255.255.255",
    "::ffff:0:0\t255.255.255.255",

    "0.0.0.0,255.255.255.255, \n\t",
    "0.0.0.0 255.255.255.255, \n\t",
    "0.0.0.0\n255.255.255.255, \n\t",
    "0.0.0.0\t255.255.255.255, \n\t",
    "0.0.0.0, \n\t255.255.255.255, \n\t",

    "::ffff:0:0,::ffff:ffff:ffff, \n\t",
    "::ffff:0:0 ::ffff:ffff:ffff, \n\t",
    "::ffff:0:0\n::ffff:ffff:ffff, \n\t",
    "::ffff:0:0\t::ffff:ffff:ffff, \n\t",
    "::ffff:0:0, \n\t::ffff:ffff:ffff, \n\t",

    "::ffff:0:0,::00ffff:00ffff:00ffff",

    "::gg,::ffff:0:0,::ffff:ffff:ffff",
    "::ffff:0:0,::ffff:ffff:ffff,::gg",

    0
};

void test_ip(const char *name) {

    long long i, j;

    for (i = 0; i < sizeof ip; ++i) ip[i] = 0;
    for (i = 0; multiipstr[i]; ++i) {
        if (strtomultiip(ip, sizeof ip, multiipstr[i]) != 32) err(name, multiipstr[i]);
        for (j = 0; j < 32; ++j) if (ip[j] != expectedip[j]) err(name, multiipstr[i]);
    }
}

int main() {

    test_overflow("strtomultiiptest: overflow test");
    test_ip("strtomultiiptest: IP test");
    _exit(0);
}
