/*
version 20130120
Jan Mojzis
Public domain.
*/

/* 1-4 byte - random number */
/* 5-12 byte - TEA encrypted counter */

#include <time.h>
#include <sys/time.h>
#include "uint32.h"
#include "uint64.h"
#include "byte.h"
#include "randombytes.h"
#include "dns.h"

static unsigned char noncekey[16];
static uint64 noncecounter = 0;
static char noncemask[4] = {0xff, 0xff, 0xff, 0xff};
static char noncedata[4] = {0, 0, 0, 0};

void dns_nonce_init(const char *ns, const unsigned char *nk) {

    struct timeval t;
    int i;

    gettimeofday(&t,(struct timezone *) 0);
    noncecounter = t.tv_sec * 1000000000ULL + t.tv_usec * 1000ULL;

    if (!ns) ns = "";

    i = 0;
    while(i < 32) {
        if (!ns[i]) break;
        if (ns[i] != '0' && ns[i] != '1') break;

        noncemask[i/8] = noncemask[i/8] * 2;
        noncedata[i/8] = noncedata[i/8] * 2 +  ns[i] - '0';
        ++i;
    }
    while(i < 32) {
        noncemask[i/8] = noncemask[i/8] * 2 + 1;
        noncedata[i/8] = noncedata[i/8] * 2;
        ++i;
    }

    if (nk) byte_copy(noncekey, sizeof noncekey, nk);
    else randombytes(noncekey, sizeof noncekey);

    return;
}

static void dns_nonce_encrypt(unsigned char *out, uint64 in, const unsigned char *k) {

    int i;
    uint32 v0, v1, k0, k1, k2, k3;
    uint32 sum = 0;
    uint32 delta=0x9e3779b9;

    v0 = in; in >>= 32;
    v1 = in;
    /*uint32_unpack(in + 0, &v0);
    uint32_unpack(in + 4, &v1);*/
    uint32_unpack((char *)(k + 0), &k0);
    uint32_unpack((char *)(k + 4), &k1);
    uint32_unpack((char *)(k + 8), &k2);
    uint32_unpack((char *)(k + 12), &k3);

    for (i = 0; i < 32; i++) {
        sum += delta;
        v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
        v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);  
    } 
    uint32_pack((char *)(out + 0),v0);
    uint32_pack((char *)(out + 4),v1);
    return;
}

void dns_nonce(char n[12]) {

    int x;

    for(x = 0; x < 4; ++x) {
        n[x] = dns_random(256);
        n[x] &= noncemask[x];
        n[x] += noncedata[x];
    }

    dns_nonce_encrypt((unsigned char *)(n + 4), ++noncecounter, noncekey); 
    return;
}
