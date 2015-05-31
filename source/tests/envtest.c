/*
20130606
Jan Mojzis
Public domain.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "env.h"

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

void test_zero(const char *name) {

    if (env_get(0)) err(name, "env_get accepts empty string");
}

void doit(const char *name, const char *d0, const char *d) {

    char *s;

    s = env_get(d0);

    if (!s) {
        if (d) err(name, "env_get find existing variable");
        return;
    }

    if (!s) err(name, "env_get didn't find existing variable");
    if (strcmp(s, d)) err2(name, s, d);
    return;
}

void test_setunset(const char *name) {

    setenv("x","x",1);
    doit(name, "x", "x");

    setenv("x","y",1);
    doit(name, "x", "y");

    unsetenv("x");
    doit(name, "x", 0);
}


void test_env(const char *name) {

    char *env[10];

    env[0] = "a=1";
    env[1] = "b=b=1";
    env[2] = "c=";
    env[3] = "d";
    env[4] = "=null";
    env[5] = "e==";
    env[6] = 0;

    environ = env;

    doit(name, "a", "1");
    doit(name, "b", "b=1");
    doit(name, "b=b", "1");
    doit(name, "c", "");
    doit(name, "d", 0);
    doit(name, "e", "=");
    doit(name, "", "null");
}

int main() {

    test_zero("evntest: env test");
    test_setunset("evntest: setunset test");
    test_env("evntest: env test");

    _exit(0);
}
