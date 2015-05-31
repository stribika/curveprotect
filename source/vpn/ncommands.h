#ifndef NCOMMANDS_H
#define NCOMMANDS_H

#include "buffer.h"

struct ncommands {
  const char *verb;
  void (*action)(char *, unsigned int);
  void (*flush)(void);
} ;

extern int ncommands(buffer *,struct ncommands *);

#endif
