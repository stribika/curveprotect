#ifndef COMMANDS_H
#define COMMANDS_H

struct commands {
  char *verb;
  void (*action)(char *);
  void (*flush)(void);
} ;

extern int commands(buffer *,struct commands *);

#endif
