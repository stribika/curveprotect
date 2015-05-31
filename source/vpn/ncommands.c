#include "buffer.h"
#include "stralloc.h"
#include "str.h"
#include "byte.h"
#include "ncommands.h"
#include "error.h"

static stralloc arg = {0};

int ncommands(buffer *ss,struct ncommands *c)
{
  char ch;
  char cmd;
  unsigned int len;
  unsigned int i;

  for (;;) {
    if (!stralloc_copys(&arg,"")) return -1;
    len = 0;

    for (;;) {
      if (buffer_get(ss,&ch,1) != 1) return -1;
      if (ch == ':') break;
      len = 10 * len + (ch - '0');
      if (len > 1024 || ch < '0' || ch > '9') {errno = error_proto; return -1;}
    }
    if (len < 1) {errno = error_proto; return -1;}

    for (i = 0; i < len; ++i) {
      if (buffer_get(ss,&ch,1) != 1) return -1;
      if (i > 0){
        if (!stralloc_append(&arg,&ch)) return -1;
      }
      else{
          cmd = ch;
      }
    }

    if (buffer_get(ss,&ch,1) != 1) return -1;
    if (ch != ',') {errno = error_proto; return -1;}

    for (i = 0;c[i].verb;++i) if (byte_equal(c[i].verb,1,&cmd)) break;
    c[i].action(arg.s, arg.len);
    if (c[i].flush) c[i].flush();
  }
}
