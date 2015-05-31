/*
20131208
Jan Mojzis
Public domain.
*/

#include <stdio.h>
#include <unistd.h>
#include "byte.h"
#include "dns.h"

void err(const char *n, const char *e) {
    fprintf(stdout, "%s: failed: ", n);
    fprintf(stdout, "%s\n", e);
    fflush(stdout);
    _exit(111);
}

unsigned char servers[256];
unsigned char keys[528];

void dns_sortipkeytest(const char *name) {

    long long i, j;

    for (j = 0; j < 10; ++j) {
        byte_zero(servers, sizeof servers);
        byte_zero(keys, sizeof keys);
        i = 10; keys[33*i] = 0; byte_copy(servers + 16*i, 16, "\0\0\0\0\0\0\0\0\0\0\377\377\177\0\0\1"); /* IPv4 regular */
        i = 11; keys[33*i] = 0; byte_copy(servers + 16*i, 16, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1"); /* IPv6 regular */
        i = 12; keys[33*i] = 2; byte_copy(servers + 16*i, 16, "\0\0\0\0\0\0\0\0\0\0\377\377\177\0\0\2"); /* IPv4 TXT */
        i = 13; keys[33*i] = 2; byte_copy(servers + 16*i, 16, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2"); /* IPv6 TXT */
        i = 14; keys[33*i] = 1; byte_copy(servers + 16*i, 16, "\0\0\0\0\0\0\0\0\0\0\377\377\177\0\0\3"); /* IPv4 streamlined */
        i = 15; keys[33*i] = 1; byte_copy(servers + 16*i, 16, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\3"); /* IPv6 streamlined */
        dns_sortipkey(servers, keys, 256);
        i = 0; if (!byte_isequal(servers + 16 * i, 16, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\3")) err(name, "bad position for IPv6 streamlined");
        i = 1; if (!byte_isequal(servers + 16 * i, 16, "\0\0\0\0\0\0\0\0\0\0\377\377\177\0\0\3")) err(name, "bad position for IPv4 streamlined");
        i = 2; if (!byte_isequal(servers + 16 * i, 16, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\2")) err(name, "bad position for IPv6 TXT");
        i = 3; if (!byte_isequal(servers + 16 * i, 16, "\0\0\0\0\0\0\0\0\0\0\377\377\177\0\0\2")) err(name, "bad position for IPv4 TXT ");
        i = 4; if (!byte_isequal(servers + 16 * i, 16, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1")) err(name, "bad position for IPv6 regular");
        i = 5; if (!byte_isequal(servers + 16 * i, 16, "\0\0\0\0\0\0\0\0\0\0\377\377\177\0\0\1")) err(name, "bad position for IPv4 regular ");
    }

    for (j = 0; j < 10; ++j) {
        byte_zero(servers, sizeof servers);
        byte_zero(keys, sizeof keys);
        i = 10; keys[33*i] = 0; byte_copy(servers + 16*i, 16, "\0\0\0\0\0\0\0\0\0\0\377\377\177\0\0\1"); /* IPv4 regular */
        i = 11; keys[33*i] = 0; byte_copy(servers + 16*i, 16, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1"); /* IPv6 regular */
        dns_sortipkey(servers, keys, 256);
        i = 0; if (!byte_isequal(servers + 16 * i, 16, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1")) err(name, "bad position for IPv6 regular");
        i = 1; if (!byte_isequal(servers + 16 * i, 16, "\0\0\0\0\0\0\0\0\0\0\377\377\177\0\0\1")) err(name, "bad position for IPv4 regular ");
    }
}

#if TODO
void dns_sortiptest(const char *name) {

    long long i, j;

    for (j = 0; j < 10; ++j) {
        byte_zero(servers, sizeof servers);
        i = 10; byte_copy(servers + 16*i, 16, "\0\0\0\0\0\0\0\0\0\0\377\377\177\0\0\1"); /* IPv4 regular */
        i = 11; byte_copy(servers + 16*i, 16, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1"); /* IPv6 regular */
        dns_sortip(servers, 256);
        i = 0; if (!byte_isequal(servers + 16 * i, 16, "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\1")) err(name, "bad position for IPv6 regular");
        i = 1; if (!byte_isequal(servers + 16 * i, 16, "\0\0\0\0\0\0\0\0\0\0\377\377\177\0\0\1")) err(name, "bad position for IPv4 regular ");
    }
}
#endif

int main() {

    dns_sortipkeytest("dns_sortipkeytest");
#if TODO
    dns_sortiptest("dns_sortip");
#endif

    _exit(0);
}
