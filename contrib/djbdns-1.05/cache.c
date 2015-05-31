#include "alloc.h"
#include "byte.h"
#include "uint32.h"
#include "exit.h"
#include "tai.h"
#include "cache.h"

#include "buffer.h"
#include "open.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include "dns.h"

uint64 cache_motion = 0;

/* record cache stats */
/* James Raftery <james@now.ie> 6 Nov. 2003 */
uint64 cache_hit = 0;
uint64 cache_miss = 0;

static char *x = 0;
static uint32 size;
static uint32 hsize;
static uint32 writer;
static uint32 oldest;
static uint32 unused;

/*
100 <= size <= 1000000000.
4 <= hsize <= size/16.
hsize is a power of 2.

hsize <= writer <= oldest <= unused <= size.
If oldest == unused then unused == size.

x is a hash table with the following structure:
x[0...hsize-1]: hsize/4 head links.
x[hsize...writer-1]: consecutive entries, newest entry on the right.
x[writer...oldest-1]: free space for new entries.
x[oldest...unused-1]: consecutive entries, oldest entry on the left.
x[unused...size-1]: unused.

Each hash bucket is a linked list containing the following items:
the head link, the newest entry, the second-newest entry, etc.
Each link is a 4-byte number giving the xor of
the positions of the adjacent items in the list.

Entries are always inserted immediately after the head and removed at the tail.

Each entry contains the following information:
4-byte link; 4-byte keylen; 4-byte datalen; 8-byte expire time; key; data.
*/

#define MAXKEYLEN 1000
#define MAXDATALEN 1000000

static void cache_impossible(void)
{
  _exit(111);
}

static void set4(uint32 pos,uint32 u)
{
  if (pos > size - 4) cache_impossible();
  uint32_pack(x + pos,u);
}

static uint32 get4(uint32 pos)
{
  uint32 result;
  if (pos > size - 4) cache_impossible();
  uint32_unpack(x + pos,&result);
  return result;
}

static unsigned int hash(const char *key,unsigned int keylen)
{
  unsigned int result = 5381;

  while (keylen) {
    result = (result << 5) + result;
    result ^= (unsigned char) *key;
    ++key;
    --keylen;
  }
  result <<= 2;
  result &= hsize - 4;
  return result;
}

char *cache_get(const char *key,unsigned int keylen,unsigned int *datalen,uint32 *ttl,int *flagns)
{
  struct tai expire;
  char expirestr[TAI_PACK];
  struct tai now;
  uint32 pos;
  uint32 prevpos;
  uint32 nextpos;
  uint32 u;
  unsigned int loop;
  double d;
  int dummy;

  if (!flagns) flagns=&dummy;

  if (!x) return 0;
  if (keylen > MAXKEYLEN) return 0;

  prevpos = hash(key,keylen);
  pos = get4(prevpos);
  loop = 0;

  *flagns = 0;

  while (pos) {
    if (get4(pos + 4) == keylen) {
      if (pos + 20 + keylen > size) cache_impossible();
      if (byte_equal(key,keylen,x + pos + 20)) {
        byte_copy(expirestr,TAI_PACK,x + pos + 12);
        if (expirestr[0] & 0x80) *flagns = 1;
        expirestr[0] &= 0x7f;
        tai_unpack(expirestr,&expire);
        tai_now(&now);
        if (tai_less(&expire,&now)) return 0;

        tai_sub(&expire,&expire,&now);
        d = tai_approx(&expire);
        if (d > 604800) d = 604800;
        *ttl = d;

        u = get4(pos + 8);
        if (u > size - pos - 20 - keylen) cache_impossible();
        *datalen = u;

        cache_hit++;
        return x + pos + 20 + keylen;
      }
    }
    nextpos = prevpos ^ get4(pos);
    prevpos = pos;
    pos = nextpos;
    if (++loop > 100) { /* to protect against hash flooding */
      cache_miss++;
      return 0;
    }
  }

  cache_miss++;
  return 0;
}

void cache_set(const char *key,unsigned int keylen,const char *data,unsigned int datalen,uint32 ttl,int flagns)
{
  struct tai now;
  struct tai expire;
  char *expirestr;
  unsigned int entrylen;
  unsigned int keyhash;
  uint32 pos;

  if (!x) return;
  if (keylen > MAXKEYLEN) return;
  if (datalen > MAXDATALEN) return;

  if (!ttl) return;
  if (ttl > 604800) ttl = 604800;

  entrylen = keylen + datalen + 20;

  while (writer + entrylen > oldest) {
    if (oldest == unused) {
      if (writer <= hsize) return;
      unused = writer;
      oldest = hsize;
      writer = hsize;
    }

    pos = get4(oldest);
    set4(pos,get4(pos) ^ oldest);
  
    oldest += get4(oldest + 4) + get4(oldest + 8) + 20;
    if (oldest > unused) cache_impossible();
    if (oldest == unused) {
      unused = size;
      oldest = size;
    }
  }

  keyhash = hash(key,keylen);

  tai_now(&now);
  tai_uint(&expire,ttl);
  tai_add(&expire,&expire,&now);

  pos = get4(keyhash);
  if (pos)
    set4(pos,get4(pos) ^ keyhash ^ writer);
  set4(writer,pos ^ keyhash);
  set4(writer + 4,keylen);
  set4(writer + 8,datalen);
  expirestr = x + writer + 12;
  tai_pack(expirestr,&expire);
  expirestr[0] &= 0x7f;
  if (flagns)
    expirestr[0] += 0x80;
  byte_copy(x + writer + 20,keylen,key);
  byte_copy(x + writer + 20 + keylen,datalen,data);

  set4(keyhash,writer);
  writer += entrylen;
  cache_motion += entrylen;
}

int cache_init(unsigned int cachesize)
{
  if (x) {
    alloc_free(x);
    x = 0;
  }

  if (cachesize > 1000000000) cachesize = 1000000000;
  if (cachesize < 100) cachesize = 100;
  size = cachesize;

  hsize = 4;
  while (hsize <= (size >> 5)) hsize <<= 1;

  x = alloc(size);
  if (!x) return 0;
  byte_zero(x,size);

  writer = hsize;
  oldest = size;
  unused = size;

  return 1;
}


static const char fn[]="dump/data";
static const char fntmp[]="dump/data.tmp";

char bspace[8096];
buffer b;

void cache_clean(int sig)
{
  unlink(fn);
  _exit(0);
}


int cache_dump(int sig)
{
  uint32 pos;
  unsigned int len;
  int r;
  int fd;

  fd = open_trunc(fntmp);
  if (fd == -1) return -1;

  buffer_init(&b,buffer_unixwrite,fd,bspace,sizeof bspace);

  pos = oldest;
  while (pos < unused) {
    len = get4(pos + 4) + get4(pos + 8) + 16;
    if (byte_diff(x + pos + 20, 2, DNS_T_AXFR)){
        if (byte_diff(x + pos + 20, 2, DNS_T_ANY)){
            r = buffer_put(&b, x + pos + 4, len);
            if (r == -1) {close(fd);return -1;}
        }
    }
    pos += 4 + len;
  }
  pos = hsize;
  while (pos < writer) {
    len = get4(pos + 4) + get4(pos + 8) + 16;
    if (byte_diff(x + pos + 20, 2, DNS_T_AXFR)){
        if (byte_diff(x + pos + 20, 2, DNS_T_ANY)){
            r = buffer_put(&b, x + pos + 4, len);
            if (r == -1) {close(fd);return -1;}
        }
    }
    pos += 4 + len;
  }
  if (buffer_flush(&b) == -1) {close(fd);return -1;}
  if (fsync(fd) == -1) {close(fd);return -1;}
  if (close(fd) == -1) return -1;
  if (chmod(fntmp, 0600) == -1) return -1;
  if (rename(fntmp,fn) == -1) return -1;
  return 0;
}

int cache_load(void)
{
  char *p, *xx;
  uint32 pos;
  unsigned int len;
  uint32 keylen;
  uint32 datalen;
  struct tai now;
  struct tai expire;
  int nb;
  struct stat st;
  int fd;
  int flagns = 0;
  char expirestr[TAI_PACK];

  fd = open_read(fn);
  if (fd == -1) return -1;
  
  if (fstat(fd,&st) == -1) {close(fd); return -1;}
  xx = mmap(0,st.st_size,PROT_READ,MAP_SHARED,fd,0);
  if (xx == MAP_FAILED) {close(fd); return -1;}
  len = st.st_size;
  p   = xx;

  tai_now(&now);
  pos = 0;
  nb = 0;
  while (pos + 16 <= len) {
    uint32_unpack(p + pos, &keylen);
    uint32_unpack(p + pos + 4, &datalen);
    byte_copy(expirestr,TAI_PACK,p + pos + 8);
    if (expirestr[0] & 0x80) flagns = 1;
    expirestr[0] &= 0x7f;
    tai_unpack(expirestr, &expire);
    pos += 16;
    if (pos + keylen + datalen > len) break; /* missing data */
    if (!tai_less(&expire,&now)) {
      tai_sub(&expire,&expire,&now);
      cache_set(p + pos, keylen, p + pos + keylen, datalen, (unsigned int)expire.x, flagns);
    } 
    pos += keylen + datalen;
    nb++;
  }
  munmap(xx, st.st_size);
  close(fd);
  return 0;
}



