#include <unistd.h>
#include "writeall.h"
#include "debug.h"

void debug_9(int e
  ,const char *s0
  ,const char *s1
  ,const char *s2
  ,const char *s3
  ,const char *s4
  ,const char *s5
  ,const char *s6
  ,const char *s7
  ,const char *s8
)
{
  const char *s[9];
  const char *x;
  char buf[256];
  long long buflen = 0;
  long long i;

  if (e < 3) return;

  s[0] = s0;
  s[1] = s1;
  s[2] = s2;
  s[3] = s3;
  s[4] = s4;
  s[5] = s5;
  s[6] = s6;
  s[7] = s7;
  s[8] = s8;
  for (i = 0;i < 9;++i) {
    x = s[i];
    if (!x) continue;
    while (*x) {
      if (buflen == sizeof buf) { writeall(2,buf,buflen); buflen = 0; }
      buf[buflen++] = *x++;
    }
  }
  writeall(2,buf,buflen);
}

void debug_exec(int e, const char *message1, const char *message2, char **s) {

    long long i;
    char buf[256];
    long long buflen = 0;
    const char *x;

    if (e < 3) return;

    for (i = 0; message1 && message1[i]; ++i) {
        if (buflen == sizeof buf) { writeall(2, buf, buflen); buflen = 0; }
        buf[buflen++] = message1[i];
    }

    for (i = 0; message2 && message2[i]; ++i) {
        if (buflen == sizeof buf) { writeall(2, buf, buflen); buflen = 0; }
        buf[buflen++] = message2[i];
    }

    for (i = 0; s[i]; ++i) {
        x = s[i];
        while (*x) {
            if (buflen == sizeof buf) { writeall(2, buf, buflen); buflen = 0; }
            buf[buflen++] = *x++;
        }
        if (buflen == sizeof buf) { writeall(2, buf, buflen); buflen = 0; }
        if (s[i + 1]) buf[buflen++] = ' ';
        else buf[buflen++] = '\n';
    }
    writeall(2, buf, buflen);
}
