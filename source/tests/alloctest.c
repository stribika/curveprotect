/*
20121025
Jan Mojzis
Public domain.
*/

#include <stdio.h>
#include <unistd.h>
#include "alloc.h"
#include "fastrandombytes.h"

void err(const char *n, const char *e) {
    fprintf(stdout, "%s: failed: ", n);
    fprintf(stdout, "%s\n", e);
    fflush(stdout);
    _exit(111);
}

#define LOOPS 100

void test0(const char *name) {

    if (!alloc(alloc_getspace()-1)) err(name, "unable to allocate memory");
    if (alloc_getallocated() != 0) err(name, "static buffer too short");
    if (!alloc(1)) err("bypass static buffer", "unable to allocate memory");
    if (alloc_getallocated() == 0) err(name, "static buffer too large");
    alloc_freeall();
    if (alloc_getallocated() != 0) err(name, "alloc_freeall doesn't work");
}

void test1(const char *name) {

    char *a, *b;
    long long i;

    alloc_setlimit(10240);

    for (i = 0; i < LOOPS; ++i) {
        if (!(a = alloc(1024))) err(name, "unable to allocate memory");
        if (!(b = alloc(2048))) err(name, "unable to allocate memory");
        if (alloc(8192)) err(name, "memory limit doesn't work");
        alloc_free(a);
        alloc_free(b);
    }

    if (alloc_getallocated() != 0) err(name, "alloc_free doesn't work");
}

void test2(const char *name) {

    long long i, j;
    unsigned char *x;
    long long l[4] = {1, 10, 100, 1000};

    alloc_setlimit(10240);

    for (i = 0; i < LOOPS; ++i) {
        for(j = 0; j < 4; ++j) {
            x = alloc(l[j] + i);
            if (!x) err(name, "unable to allocate memory");
            fastrandombytes(x, l[j] + i);
            alloc_free(x);
        }
    }
    if (alloc_getallocated() != 0) err(name, "alloc_free doesn't work");
}

void test3(const char *name) {

    unsigned char *x;
    long long i;

    alloc_setlimit(10240);

    for (i = 0; i < LOOPS; ++i) {

        x = alloc(-1);
        if (x) err(name, "allocator does handle negative length");

        x = alloc(0);
        if (!x) err(name, "allocator doesn't handle length=0");
        alloc_free(0);
        alloc_free(x);
    }
    if (alloc_getallocated() != 0) err(name, "alloc_free doesn't work");
}

void test4(const char *name) {

    unsigned char *a, *b, *c;
    long long i;

    alloc_setlimit(1024000);

    for (i = 0; i < LOOPS; ++i) {
        a = alloc(10);
        if (!a) err(name, "unable to allocate memory");
        fastrandombytes(a, 10);
    }
    b = alloc(10); if (!b) err(name, "unable to allocate memory");
    for (i = 0; i < 1000; ++i) {
        a = alloc(10);
        if (!a) err(name, "unable to allocate memory");
        fastrandombytes(a, 10);
    }
    c = alloc(10); if (!c) err(name, "unable to allocate memory");
    for (i = 0; i < 1000; ++i) {
        a = alloc(10);
        if (!a) err(name, "unable to allocate memory");
        fastrandombytes(a, 10);
    }
    alloc_free(c);
    alloc_free(b);
    alloc_freeall();

    if (alloc_getallocated() != 0) err(name, "alloc_freeall doesn't work");
}

void test5(const char *name) {

    unsigned char *x[LOOPS];
    long long i, j;

    for (i = 0; i < LOOPS; ++i) {
        x[i] = alloc(10);
        if (!x[i]) err(name, "unable to allocate memory");
        fastrandombytes(x[i], 10);
    }
    for (i = 0; i < LOOPS; ++i) {
        for (j = 0; j < LOOPS; ++j) {
            alloc_free(x[j]);
        }
    }
}

struct astruct {
    unsigned char *a[LOOPS];
};

void test6(const char *name) {

    struct astruct *s;
    long long i;

    s = alloc(sizeof(struct astruct));
    if (!s) err(name, "unable to allocate memory");

    for (i = 0; i < LOOPS; ++i) {
        s->a[i]= alloc(10);
        if (!s->a[i]) err(name, "unable to allocate memory");
    }
    fastrandombytes((unsigned char *)s, sizeof(struct astruct));
    alloc_freeall();
    if (alloc_getallocated() != 0) err(name, "alloc_freeall doesn't work");
}

int main() {

    test0("alloctest: static buffer test");
    test1("alloctest: memory limit test");
    test2("alloctest: alloc/alloc_free test");
    test3("alloctest: negative/zero alloc test");
    test4("alloctest: alloc_freall test");
    test5("alloctest: double free test");
    test6("alloctest: struct test");

    _exit(0);
}
