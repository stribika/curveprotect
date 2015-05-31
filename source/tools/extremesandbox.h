#ifndef EXTREMESANDBOX_H
#define EXTREMESANDBOX_H

extern long long extremesandbox_getuid(void);
extern int extremesandbox_checkuid(long long);
extern int extremesandbox_droproot(void);
extern int extremesandbox(int, const char *);

#endif
