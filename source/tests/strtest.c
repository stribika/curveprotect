/*
20130610
Jan Mojzis
Public domain.
*/

#include <stdio.h>
#include <unistd.h>
#include "str.h"

void err(const char *n, const char *e) {
    fprintf(stdout, "%s: failed: ", n);
    fprintf(stdout, "%s\n", e);
    fflush(stdout);
    _exit(111);
}

void test_chr(const char *name) {

    if (str_chr("", '.') != 0) err(name, "str_chr doesn't accept zero length");
    if (str_rchr("", '.') != 0) err(name, "str_rchr doesn't accept zero length");

    if (str_chr(" ", '.') != 1) err(name, "str_chr return's bad position");
    if (str_rchr(" ", '.') != 1) err(name, "str_rchr return's bad position");
    if (str_chr(".aaa", '.') != 0) err(name, "str_chr return's bad position");
    if (str_rchr(".aaa", '.') != 0) err(name, "str_rchr return's bad position");

    if (str_chr("a.a.a", '.') != 1) err(name, "str_chr return's bad position");
    if (str_rchr("a.a.a", '.') != 3) err(name, "str_rchr return's bad position");
    return;
}

void test_len(const char *name) {

    if (str_len("") != 0) err(name, "str_len return's bad length");
    if (str_len("\0ahoj") != 0) err(name, "str_len return's bad length");
    if (str_len("ahoj\0") != 4) err(name, "str_len return's bad length");
}


static int oldstr_diff(register const char *s,register const char *t)
{
  register char x;

  for (;;) {
    x = *s; if (x != *t) break; if (!x) break; ++s; ++t;
    x = *s; if (x != *t) break; if (!x) break; ++s; ++t;
    x = *s; if (x != *t) break; if (!x) break; ++s; ++t;
    x = *s; if (x != *t) break; if (!x) break; ++s; ++t;
  }
  return ((int)(unsigned int)(unsigned char) x)
       - ((int)(unsigned int)(unsigned char) *t);
}


void test_diff(const char *name) {

    if (str_diff("", "") != 0) err(name, "str_diff doesn't handle same data");
    if (str_diff("1234", "1234") != 0) err(name, "str_diff doesn't handle same data");
    if (str_diff("a", "b") != oldstr_diff("a", "b")) err(name, "str_diff doesn't handle different data ab");
    if (str_diff("b", "a") != oldstr_diff("b", "a")) err(name, "str_diff doesn't handle different data ba");
    if (str_diff("aa", "ab") != oldstr_diff("aa", "ab")) err(name, "str_diff doesn't handle different data aa");
    if (str_diff("ab", "aa") != oldstr_diff("ab", "aa")) err(name, "str_diff doesn't handle different data ab");
    if (str_diff("abc", "") != oldstr_diff("abc", "")) err(name, "str_diff doesn't handle different data abc");
    if (str_diff("", "cab") != oldstr_diff("", "cab")) err(name, "str_diff doesn't handle different data cab");
}

int main() {

    test_chr("strtest: str_chr/str_rchr test");
    test_len("strtest: str_len test");
    test_diff("strtest:: str_diff test");

    _exit(0);
}
