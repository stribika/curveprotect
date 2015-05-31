#ifndef _STATUSMESSAGE_H____
#define _STATUSMESSAGE_H____

extern int statusmessage_write(int, long long, char, unsigned char *, unsigned char *);
extern int statusmessage_read(int, long long *, int *, unsigned char *, unsigned char *);

#endif
