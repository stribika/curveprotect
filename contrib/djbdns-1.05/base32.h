#ifndef _BASE32_H____
#define _BASE32_H____

extern unsigned int base32_decode(char *out,const char *in,unsigned int len,int mode);

extern unsigned int base32_bytessize(unsigned int len);
extern void base32_encodebytes(char *out,const char *in,unsigned int len);
extern void base32_encodekey(char *out,const char *key);

#endif /* _BASE32_H____ */
