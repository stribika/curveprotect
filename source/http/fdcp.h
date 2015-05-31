#ifndef _FDCP_H____
#define _FDCP_H____

extern int fdcp(const char *warning, int fdin1, int fdout1, int timeout1, int fdin2, int fdout2,  int timeout2);
extern int fdcp_vpn(const char *warning, int fdin0, int fdout0, int timeout1, int fdin1, int fdout1,  int timeout2, int flagverbose, int flagserver);

#endif /* _FDCP_H____ */
