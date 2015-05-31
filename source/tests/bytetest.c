/*
20130608
Jan Mojzis
Public domain.
*/

#include <stdio.h>
#include <unistd.h>
#include "byte.h"

void err(const char *n, const char *e) {
    fprintf(stdout, "%s: failed: ", n);
    fprintf(stdout, "%s\n", e);
    fflush(stdout);
    _exit(111);
}

void test_copy(const char *name) {

    char *x = "abcdefgh";
    char y[8];

    byte_zero(y, 8);
    if (!byte_isequal(y, 8, "\0\0\0\0\0\0\0\0")) err(name, "byte_zero/byte_isequal failure");

    byte_copy(y, 8, x);
    if (!byte_isequal(y, 8, x)) err(name, "byte_copy/byte_isequal failure");

    byte_copyr(y + 1, 7, y);
    if (!byte_isequal(y, 8, "aabcdefg")) err(name, "byte_copyr/byte_isequal failure");

    byte_copy(y, 7, y + 1);
    if (!byte_isequal(y, 8, "abcdefgg")) err(name, "byte_copy/byte_isequal failure");

    byte_zero(y, 8);
    if (!byte_isequal(y, 8, "\0\0\0\0\0\0\0\0")) err(name, "byte_zero/byte_isequal failure");

    return;
}

void test_chr(const char *name) {

    if (byte_chr("", -3, '.') != -3) err(name, "byte_chr doesn't accept negative length");
    if (byte_rchr("", -3, '.') != -3) err(name, "byte_rchr doesn't accept negative length");
    if (byte_chr("", 0, '.') != 0) err(name, "byte_chr doesn't accept zero length");
    if (byte_rchr("", 0, '.') != 0) err(name, "byte_rchr doesn't accept zero length");

    if (byte_chr("", 1, '.') != 1) err(name, "byte_chr return's bad position");
    if (byte_rchr("", 1, '.') != 1) err(name, "byte_rchr return's bad position");
    if (byte_chr(".aaa", 4, '.') != 0) err(name, "byte_chr return's bad position");
    if (byte_rchr(".aaa", 4, '.') != 0) err(name, "byte_rchr return's bad position");

    if (byte_chr("a.a.a", 5, '.') != 1) err(name, "byte_chr return's bad position");
    if (byte_rchr("a.a.a", 5, '.') != 3) err(name, "byte_rchr return's bad position");
    return;
}

static int oldbyte_diff(s,n,t)
register char *s;
register unsigned int n;
register char *t;
{
  for (;;) {
    if (!n) return 0; if (*s != *t) break; ++s; ++t; --n;
    if (!n) return 0; if (*s != *t) break; ++s; ++t; --n;
    if (!n) return 0; if (*s != *t) break; ++s; ++t; --n;
    if (!n) return 0; if (*s != *t) break; ++s; ++t; --n;
  }
  return ((int)(unsigned int)(unsigned char) *s)
       - ((int)(unsigned int)(unsigned char) *t);
}

void test_diff(const char *name) {

    if (byte_diff("1234", 4, "1234") != 0) err(name, "byte_diff doesn't handle same data");
    if (byte_diff("1234", 4, "12345") != 0) err(name, "byte_diff doesn't handle overlap");
    if (byte_diff("12345", 4, "1234") != 0) err(name, "byte_diff doesn't handle overlap");
    if (byte_diff("12345", 0, "4321") != 0) err(name, "byte_diff doesn't handle zero length");
    if (byte_diff("12345", -1, "4321") != 0) err(name, "byte_diff doesn't handle negative length");
    if (byte_diff("a", 1, "b") != oldbyte_diff("a", 1, "b")) err(name, "byte_diff doesn't handle different data");
    if (byte_diff("b", 1, "a") != oldbyte_diff("b", 1, "a")) err(name, "byte_diff doesn't handle different data");
    if (byte_diff("aa", 2, "ab") != oldbyte_diff("aa", 2, "ab")) err(name, "byte_diff doesn't handle different data");
    if (byte_diff("ab", 2, "aa") != oldbyte_diff("ab", 2, "aa")) err(name, "byte_diff doesn't handle different data");
}


int main() {

    test_copy("bytetest: copy test");
    test_chr("bytetest: byte_chr/byte_rchr test");
    test_diff("bytetest: byte_diff test");

    _exit(0);
}
