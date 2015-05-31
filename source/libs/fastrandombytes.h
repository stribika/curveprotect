#ifndef _FASTRANDOMBYTES_H____
#define _FASTRANDOMBYTES_H____

extern void fastrandombytes(void *, long long);

#ifdef TEST
extern void fastrandombytes_getkey(unsigned char *);
extern void fastrandombytes_getnonce(unsigned char *);
#endif


#endif
