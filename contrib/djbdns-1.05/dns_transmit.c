#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include "socket.h"
#include "alloc.h"
#include "error.h"
#include "byte.h"
#include "uint16.h"
#include "uint64.h"
#include "case.h"
#include "dns.h"
#include "base32.h"

#include "crypto_box.h"

#if crypto_box_PUBLICKEYBYTES != 32
error!
#endif
#if crypto_box_NONCEBYTES != 24
error!
#endif

static void makebasequery(struct dns_transmit *d,char *query)
{
  unsigned int len;

  len = dns_domain_length(d->name);

  byte_copy(query,2,d->id);
  byte_copy(query + 2,10,d->flagrecursive ? "\1\0\0\1\0\0\0\0\0\0" : "\0\0\0\1\0\0\0\0\0\0gcc-bug-workaround");
  byte_copy(query + 12,len,d->name);
  byte_copy(query + 12 + len,2,d->qtype);
  byte_copy(query + 14 + len,2,DNS_C_IN);
  if (d->paddinglen > 0) {
      byte_zero(query + 16 + len,d->paddinglen);
      query[16 + len + d->paddinglen - 1] = 0x80;
  }
}

static void regularquery(struct dns_transmit *d) {

  unsigned int len;

  d->paddinglen = 0;

  len = dns_domain_length(d->name) + d->paddinglen;
  d->querylen = len + 18;

  uint16_pack_big(d->query, d->querylen - 2);
  d->id[0] = dns_random(256);
  d->id[1] = dns_random(256);
  makebasequery(d, d->query + 2);
}

static void streamlinedquery(struct dns_transmit *d) {

  unsigned int len;
  char nonce[24];

  d->paddinglen = (2 + dns_random(2)) * 64 - (len + 16) % 64;

  len = dns_domain_length(d->name) + d->paddinglen;
  d->querylen = len + 86;

  dns_nonce(d->nonce);
  byte_copy(nonce, 12, d->nonce);
  byte_zero(nonce + 12, 12);

  byte_zero(d->query + 38, 32);
  d->id[0] = dns_random(256);
  d->id[1] = dns_random(256);
  makebasequery(d, d->query + 38 + 32);
  crypto_box_afternm((unsigned char *)d->query + 38, (unsigned char *)d->query + 38, len + 48, (unsigned char *)nonce, (unsigned char *) d->keys + 33 * d->curserver + 1);

  uint16_pack_big(d->query, d->querylen - 2);
  byte_copy(d->query + 2, 8, dns_magicq);
  byte_copy(d->query + 10, 32, d->pubkey);
  byte_copy(d->query + 42, 12, nonce);
}

static void txtquery(struct dns_transmit *d) {

  unsigned int len, suffixlen, m;
  char nonce[24];

  d->paddinglen = 0;

  len = dns_domain_length(d->name) + d->paddinglen;
  suffixlen = dns_domain_length(d->suffix);
  m = base32_bytessize(len + 44);
  d->querylen = m + suffixlen + 73;

  dns_nonce(d->nonce);
  byte_copy(nonce, 12, d->nonce);
  byte_zero(nonce + 12, 12);

  byte_zero(d->query, 32);
  d->id[0] = dns_random(256);
  d->id[1] = dns_random(256);
  makebasequery(d, d->query + 32);
  crypto_box_afternm((unsigned char *)d->query, (unsigned char *)d->query, len + 48, (unsigned char *)nonce, (unsigned char *) d->keys + 33 * d->curserver + 1);

  byte_copyr(d->query + d->querylen - len - 32, len + 32, d->query + 16);
  byte_copy(d->query + d->querylen - len - 44, 12, nonce);

  uint16_pack_big(d->query, d->querylen - 2);
  d->query[0] = dns_random(256);
  d->query[1] = dns_random(256);
  byte_copy(d->query + 4, 10, "\0\0\0\1\0\0\0\0\0\0");
  base32_encodebytes(d->query + 14,d->query + d->querylen - len - 44, len + 44);
  base32_encodekey(d->query + 14 + m, d->pubkey);
  byte_copy(d->query + 69 + m, suffixlen, d->suffix);
  byte_copy(d->query + 69 + m + suffixlen, 2, DNS_T_TXT);
  byte_copy(d->query + 69 + m + suffixlen + 2, 2, DNS_C_IN);
}

static void prepquery(struct dns_transmit *d)
{
  const char *key;

  if (!d->keys) {
    regularquery(d);
    return;
  }
  key = d->keys + 33 * d->curserver;
  if (key[0] == 0) {
    regularquery(d);
    return;
  }
  if (key[0] == 1) {
    streamlinedquery(d);
    return;
  }
  txtquery(d);
}

static int uncurve(const struct dns_transmit *d,char *buf,unsigned int *lenp)
{
  const char *key;
  char nonce[24];
  unsigned int len;
  char out[16];
  unsigned int pos;
  uint16 datalen;
  unsigned int i;
  unsigned int j;
  char ch;
  unsigned int txtlen;
  unsigned int namelen;

  if (!d->keys) return 0;
  key = d->keys + 33 * d->curserver;
  if (key[0] == 0) return 0;

  len = *lenp;

  if (key[0] == 1) {
    if (len < 48) return 1;
    if (byte_diff(buf,8,dns_magicr)) return 1;
    if (byte_diff(buf + 8,12,d->nonce)) return 1;
    byte_copy(nonce,24,buf + 8);
    byte_zero(buf + 16,16);
    if (crypto_box_open_afternm((unsigned char *) buf + 16,(const unsigned char *) buf + 16,len - 16,(const unsigned char *) nonce,(const unsigned char *) key + 1)) return 1;
    byte_copy(buf,len - 48,buf + 48);
    *lenp = len - 48;
    return 0;
  }

  /* XXX: be more leniant? */

  pos = dns_packet_copy(buf,len,0,out,12); if (!pos) return 1;
  if (byte_diff(out,2,d->query + 2)) return 1;
  if (byte_diff(out + 2,10,"\204\0\0\1\0\1\0\0\0\0")) return 1;

  /* query name might be >255 bytes, so can't use dns_packet_getname */
  namelen = dns_domain_length(d->query + 14);
  if (namelen > len - pos) return 1;
  if (case_diffb(buf + pos,namelen,d->query + 14)) return 1;
  pos += namelen;

  pos = dns_packet_copy(buf,len,pos,out,16); if (!pos) return 1;
  if (byte_diff(out,14,"\0\20\0\1\300\14\0\20\0\1\0\0\0\0")) return 1;
  uint16_unpack_big(out + 14,&datalen);
  if (datalen > len - pos) return 1;

  j = 4;
  txtlen = 0;
  for (i = 0;i < datalen;++i) {
    ch = buf[pos + i];
    if (!txtlen)
      txtlen = (unsigned char) ch;
    else {
      --txtlen;
      buf[j++] = ch;
    }
  }
  if (txtlen) return 1;

  if (j < 32) return 1;
  byte_copy(nonce,12,d->nonce);
  byte_copy(nonce + 12,12,buf + 4);
  byte_zero(buf,16);
  if (crypto_box_open_afternm((unsigned char *) buf,(const unsigned char *) buf,j,(const unsigned char *) nonce,(const unsigned char *) key + 1)) return 1;
  byte_copy(buf,j - 32,buf + 32);
  *lenp = j - 32;
  return 0;
}

static int serverwantstcp(const char *buf,unsigned int len)
{
  char out[12];

  if (!dns_packet_copy(buf,len,0,out,12)) return 1;
  if (out[2] & 2) return 1;
  return 0;
}

static int serverfailed(const char *buf,unsigned int len)
{
  char out[12];
  unsigned int rcode;

  if (!dns_packet_copy(buf,len,0,out,12)) return 1;
  rcode = out[3];
  rcode &= 15;
  if (rcode && (rcode != 3)) { errno = error_again; return 1; }
  return 0;
}

static int irrelevant(const struct dns_transmit *d,const char *buf,unsigned int len)
{
  char out[12];
  char *dn;
  unsigned int pos;

  pos = dns_packet_copy(buf,len,0,out,12); if (!pos) return 1;
  if (byte_diff(out,2,d->id)) return 1;
  if (out[4] != 0) return 1;
  if (out[5] != 1) return 1;

  dn = 0;
  pos = dns_packet_getname(buf,len,pos,&dn); if (!pos) return 1;
  if (!dns_domain_equal(dn,d->name)) { alloc_free(dn); return 1; }
  alloc_free(dn);

  pos = dns_packet_copy(buf,len,pos,out,4); if (!pos) return 1;
  if (byte_diff(out,2,d->qtype)) return 1;
  if (byte_diff(out + 2,2,DNS_C_IN)) return 1;

  return 0;
}

static void packetfree(struct dns_transmit *d)
{
  if (!d->packet) return;
  alloc_free(d->packet);
  d->packet = 0;
}

static void queryfree(struct dns_transmit *d)
{
  if (!d->query) return;
  alloc_free(d->query);
  d->query = 0;
}

static void socketfree(struct dns_transmit *d)
{
  if (!d->s1) return;
  close(d->s1 - 1);
  d->s1 = 0;
}

void dns_transmit_free(struct dns_transmit *d)
{
  queryfree(d);
  socketfree(d);
  packetfree(d);
}

static int randombind(struct dns_transmit *d)
{
  int j;

  for (j = 0;j < 10;++j)
    if (socket_bind4(d->s1 - 1,d->localip,1025 + dns_random(64510)) == 0)
      return 0;
  if (socket_bind4(d->s1 - 1,d->localip,0) == 0)
    return 0;
  return -1;
}

static const int timeouts[4] = { 1, 3, 11, 45 };

static int thisudp(struct dns_transmit *d)
{
  const char *ip;

  socketfree(d);

  while (d->udploop < 4) {
    for (;d->curserver < 16;++d->curserver) {
      ip = d->servers + 4 * d->curserver;
      if (byte_diff(ip,4,"\0\0\0\0")) {
        prepquery(d);
  
        d->s1 = 1 + socket_udp();
        if (!d->s1) { dns_transmit_free(d); return -1; }
	if (randombind(d) == -1) { dns_transmit_free(d); return -1; }

        if (socket_connect4(d->s1 - 1,ip,53) == 0)
          if (send(d->s1 - 1,d->query + 2,d->querylen - 2,0) == d->querylen - 2) {
            struct taia now;
            taia_now(&now);
            taia_uint(&d->deadline,timeouts[d->udploop]);
            taia_add(&d->deadline,&d->deadline,&now);
            d->tcpstate = 0;
            return 0;
          }
  
        socketfree(d);
      }
    }

    ++d->udploop;
    d->curserver = 0;
  }

  dns_transmit_free(d); return -1;
}

static int firstudp(struct dns_transmit *d)
{
  d->curserver = 0;
  return thisudp(d);
}

static int nextudp(struct dns_transmit *d)
{
  ++d->curserver;
  return thisudp(d);
}

static int thistcp(struct dns_transmit *d)
{
  struct taia now;
  const char *ip;

  socketfree(d);
  packetfree(d);

  for (;d->curserver < 16;++d->curserver) {
    ip = d->servers + 4 * d->curserver;
    if (byte_diff(ip,4,"\0\0\0\0")) {
      prepquery(d);

      d->s1 = 1 + socket_tcp();
      if (!d->s1) { dns_transmit_free(d); return -1; }
      if (randombind(d) == -1) { dns_transmit_free(d); return -1; }
  
      taia_now(&now);
      taia_uint(&d->deadline,10);
      taia_add(&d->deadline,&d->deadline,&now);
      if (socket_connect4(d->s1 - 1,ip,53) == 0) {
        d->pos = 0;
        d->tcpstate = 2;
        return 0;
      }
      if ((errno == error_inprogress) || (errno == error_wouldblock)) {
        d->tcpstate = 1;
        return 0;
      }
  
      socketfree(d);
    }
  }

  dns_transmit_free(d); return -1;
}

static int firsttcp(struct dns_transmit *d)
{
  d->curserver = 0;
  return thistcp(d);
}

static int nexttcp(struct dns_transmit *d)
{
  ++d->curserver;
  return thistcp(d);
}

int dns_transmit_start(struct dns_transmit *d,const char servers[64],int flagrecursive,const char *q,const char qtype[2],const char localip[4])
{
  return dns_transmit_start2(d,servers,flagrecursive,q,qtype,localip,0,0,0);
}

int dns_transmit_start2(struct dns_transmit *d,const char servers[64],int flagrecursive,const char *q,const char qtype[2],const char localip[4],const char keys[528],const char pubkey[crypto_box_PUBLICKEYBYTES],const char *suffix)
{
  unsigned int len;
  unsigned int suffixlen;
  unsigned int m;

  dns_transmit_free(d);
  errno = error_io;

  len = dns_domain_length(q);

  d->paddinglen = 3 * 64 - (len + 16) % 64;

  if (!suffix) suffix = "";
  suffixlen = dns_domain_length(suffix);
  m = base32_bytessize(len + 44 + d->paddinglen);
  d->querylen = m + suffixlen + 73;

  d->query = alloc(d->querylen);
  if (!d->query) return -1;

  d->name = q;
  byte_copy(d->qtype,2,qtype);
  d->servers = servers;
  byte_copy(d->localip,4,localip);
  d->flagrecursive = flagrecursive;
  d->keys = keys;
  d->pubkey = pubkey;
  d->suffix = suffix;

#if 0
  if (!d->keys) {
    uint16_pack_big(d->query,len + 16);
    makebasequery(d,d->query + 2);
    d->name = d->query + 14; /* keeps dns_transmit_start backwards compatible */
  }
#endif

  d->udploop = flagrecursive ? 1 : 0;

  if (len + 16 > 512) return firsttcp(d);
  return firstudp(d);
}

void dns_transmit_io(struct dns_transmit *d,iopause_fd *x,struct taia *deadline)
{
  x->fd = d->s1 - 1;

  switch(d->tcpstate) {
    case 0: case 3: case 4: case 5:
      x->events = IOPAUSE_READ;
      break;
    case 1: case 2:
      x->events = IOPAUSE_WRITE;
      break;
  }

  if (taia_less(&d->deadline,deadline))
    *deadline = d->deadline;
}

int dns_transmit_get(struct dns_transmit *d,const iopause_fd *x,const struct taia *when)
{
  char udpbuf[4097];
  unsigned char ch;
  int r;
  int fd;
  unsigned int len;

  errno = error_io;
  fd = d->s1 - 1;

  if (!x->revents) {
    if (taia_less(when,&d->deadline)) return 0;
    errno = error_timeout;
    if (d->tcpstate == 0) return nextudp(d);
    return nexttcp(d);
  }

  if (d->tcpstate == 0) {
/*
have attempted to send UDP query to each server udploop times
have sent query to curserver on UDP socket s
*/
    r = recv(fd,udpbuf,sizeof udpbuf,0);
    if (r <= 0) {
      if (errno == error_connrefused) if (d->udploop == 2) return 0;
      return nextudp(d);
    }
    if (r + 1 > sizeof udpbuf) return 0;

    len = r;
    if (uncurve(d,udpbuf,&len)) return 0;
    if (irrelevant(d,udpbuf,len)) return 0;
    if (serverwantstcp(udpbuf,len)) return firsttcp(d);
    if (serverfailed(udpbuf,len)) {
      if (d->udploop == 2) return 0;
      return nextudp(d);
    }
    socketfree(d);

    d->packetlen = len;
    d->packet = alloc(d->packetlen);
    if (!d->packet) { dns_transmit_free(d); return -1; }
    byte_copy(d->packet,d->packetlen,udpbuf);
    queryfree(d);
    return 1;
  }

  if (d->tcpstate == 1) {
/*
have sent connection attempt to curserver on TCP socket s
pos not defined
*/
    if (!socket_connected(fd)) return nexttcp(d);
    d->pos = 0;
    d->tcpstate = 2;
    return 0;
  }

  if (d->tcpstate == 2) {
/*
have connection to curserver on TCP socket s
have sent pos bytes of query
*/
    r = write(fd,d->query + d->pos,d->querylen - d->pos);
    if (r <= 0) return nexttcp(d);
    d->pos += r;
    if (d->pos == d->querylen) {
      struct taia now;
      taia_now(&now);
      taia_uint(&d->deadline,10);
      taia_add(&d->deadline,&d->deadline,&now);
      d->tcpstate = 3;
    }
    return 0;
  }

  if (d->tcpstate == 3) {
/*
have sent entire query to curserver on TCP socket s
pos not defined
*/
    r = read(fd,&ch,1);
    if (r <= 0) return nexttcp(d);
    d->packetlen = ch;
    d->tcpstate = 4;
    return 0;
  }

  if (d->tcpstate == 4) {
/*
have sent entire query to curserver on TCP socket s
pos not defined
have received one byte of packet length into packetlen
*/
    r = read(fd,&ch,1);
    if (r <= 0) return nexttcp(d);
    d->packetlen <<= 8;
    d->packetlen += ch;
    d->tcpstate = 5;
    d->pos = 0;
    d->packet = alloc(d->packetlen);
    if (!d->packet) { dns_transmit_free(d); return -1; }
    return 0;
  }

  if (d->tcpstate == 5) {
/*
have sent entire query to curserver on TCP socket s
have received entire packet length into packetlen
packet is allocated
have received pos bytes of packet
*/
    r = read(fd,d->packet + d->pos,d->packetlen - d->pos);
    if (r <= 0) return nexttcp(d);
    d->pos += r;
    if (d->pos < d->packetlen) return 0;

    socketfree(d);
    if (uncurve(d,d->packet,&d->packetlen)) return nexttcp(d);
    if (irrelevant(d,d->packet,d->packetlen)) return nexttcp(d);
    if (serverwantstcp(d->packet,d->packetlen)) return nexttcp(d);
    if (serverfailed(d->packet,d->packetlen)) return nexttcp(d);

    queryfree(d);
    return 1;
  }

  return 0;
}
