# derived from public domain code from:
# https://github.com/mdempsky/dnscurve/

import random, os, asyncore, time, re, struct, socket, sys, lib, config
from asyncore import socket_map as map
import base32

try:
    import nacl
except ImportError:
    import slownacl as nacl


class Random(random.Random):

        def __init__(self):
                """
                """

                self.seed(nacl.randombytes(16))
                self.gauss_next = None

        def randombytes(self, l = 0):
                """
                """

                return nacl.randombytes(l)

def poll(timeout = 0.1, map = asyncore.socket_map):

        tm = time.time()
        for fd, obj in list(map.items()):

                if (tm - obj.start) > obj.timeout:
                        obj.handle_timeout()

        if len(map) == 0:
                time.sleep(timeout)
        else:
                asyncore.poll(timeout, map)
        return

def doit(qc = []):

        parallel = 10
        j = 0
        while True:

                for i in range(j, len(qc)):
                        if len(map) <= parallel:
                                (q,c) = qc[i]
                                DNSResolver(query = q,  consumer = c)
                                j += 1

                if not len(map) and j == len(qc):
                        break

                poll()


qtypes = {1: 'A',
          2: 'NS',
          5: 'CNAME',
          6: 'SOA',
          12: 'PTR',
          15: 'MX',
          16: 'TXT',
          28: 'AAAA',
          33: 'SRV',
          255: 'ANY'
          }

qclasses = {1: 'IN'}


errors = {      0: 'Success',
                1: 'Format error',
                2: 'Server failure',
                3: 'Name error',
                4: 'Not implemented',
                5: 'Refused'
}


def dns_name_read(rest, p):
  output = []
  firstcompress = None

  while True:
    b = ord(rest[0])
    if b == 0:
      rest = rest[1:]
      break
    elif b < 64:
      output.append(rest[1:1+b])
      rest = rest[1+b:]
      continue
    elif b >= 192:
      b2 = ord(rest[1])
      pos = 256 * (b - 192) + b2
      if firstcompress is None:
        firstcompress = rest[2:]
      rest = p[pos:]
      continue
    else:
      raise ValueError('Bad DNS name')

  def repl(matchobj):
    return '\\%03o' % ord(matchobj.group(0)[0])
  name = '.'.join([re.sub('[^0-9A-Za-z_-]', repl, l) for l in output])

  if firstcompress is not None:
    return (name, firstcompress)
  return (name, rest)

def dns_query_read(rest, p):
  (name, rest) = dns_name_read(rest, p)
  (qtype, qclass) = struct.unpack('>HH', rest[:4])
  return ((name, qtypes.get(qtype, '??'), qclasses.get(qclass, '??')), rest[4:])

def dns_result_read(rest, p):
  (name, rest) = dns_name_read(rest, p)
  (qtype, qclass, ttl, rdlen) = struct.unpack('>HHIH', rest[:10])
  rest = rest[10:]
  data = rest[:rdlen]
  return ((name, qtypes.get(qtype, '??'), qclasses.get(qclass, '??'), ttl, data), rest[rdlen:])

def dns_pretty_rdata(type, qclass, data, p):
  # Classless record types
  if type == 'NS' or type == 'PTR' or type == 'CNAME':
    (name, rest) = dns_name_read(data, p)
    if len(rest): raise ValueError('Bad DNS record data')
    return name
  if type == 'MX':
    (pref,) = struct.unpack('>H', data[:2])
    (name, rest) = dns_name_read(data[2:], p)
    if len(rest): raise ValueError('Bad DNS record data')
    return '%d\t%s' % (pref, name)
  if type == 'SOA':
    (mname, rest) = dns_name_read(data, p)
    (rname, rest) = dns_name_read(rest, p)
    (serial, refresh, retry, expire, minimum) = struct.unpack('>5I', rest)
    return (mname, rname, serial, refresh, retry, expire, minimum)

  # Internet class record types
  if qclass == 'IN':
    if type == 'A':
      return '%d.%d.%d.%d' % struct.unpack('>4B', data)
    if type == 'AAAA':
      return '%x:%x:%x:%x:%x:%x:%x:%x' % struct.unpack('>8H', data)
    if type == 'SRV':
      (pref, weight, port) = struct.unpack('>HHH', data[:6])
      (name, rest) = dns_name_read(data[6:], None)
      if len(rest): raise ValueError('Bad DNS record data')
      return (pref, weight, port, name)

  res = []
  for c in data:
    if ord(c) >= 33 and ord(c) <= 126 and c != '\\':
      res.append(c)
    else:
      res.append('\\%03o' % (ord(c),))
  return ''.join(res)


def dns_domain_fromdot(host):
        def repl(matchobj):
                return chr(int(matchobj.group(0)[1:], 8))
        return [re.sub(r'\\[0-7]{1,3}', repl, l) for l in host.split('.') if l]

def dnscurve_getpubkey(name):
  for s in name:
    if len(s) == 54 and s[:3].lower() == 'uz5':
      try:
        return base32.decode(s[3:] + '0')
      except ValueError:
        pass
  return None


def dnscurve_encode_streamlined_query(nonce, box, pubkey):
  if len(nonce) != 12:
    raise ValueError('Invalid nonce')
  if len(pubkey) != 32 or ord(pubkey[31]) >= 128:
    raise ValueError('Invalid public key')
  return 'Q6fnvWj8' + pubkey + nonce + box

def dnscurve_decode_streamlined_query(packet):
  if len(packet) < 52 or packet[:8] != 'Q6fnvWj8':
    raise ValueError('Not a streamlined query')
  return (packet[8:20], packet[20:52], packet[52:])

def dnscurve_encode_streamlined_response(nonce, box):
  if len(nonce) != 24:
    raise ValueError('Invalid nonce')
  return 'R6fnvWJ8' + nonce + box

def dnscurve_decode_streamlined_response(packet):
  if len(packet) < 32 or packet[:8] != 'R6fnvWJ8':
    raise ValueError('Not a streamlined response')
  return (packet[8:32], packet[32:])


def dnscurve_encode_txt_query(nonce, box, pubkey, zone, id):
  output = []
  output.append(id)
  output.append('\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00')

  for label in dnscurve_encode_queryname(nonce, box, pubkey, zone):
    output.append(chr(len(label)))
    output.append(label)
  output.append(chr(0))

  output.append('\x00\x10\x00\x01')
  return ''.join(output)

def dnscurve_decode_txt_response(packet):
  (id, f1, f2, nquery, nans, nauth, nadd) = struct.unpack('>HBBHHHH', packet[:12])
  if (f2 & 0x0f or not nans):
    raise Exception("remote server doesn't support TXT DNSCurve")
  rest = packet[12:]
  query, rest = dns_query_read(rest, packet)
  (name, type, qclass, ttl, rdata), rest = dns_result_read(rest, packet)
  if type != 'TXT': raise Exception("remote server doesn't support TXT DNSCurve")
  key, nonce1, box1 = dnscurve_decode_queryname(dns_domain_fromdot(name))
  nonce2, box2 = dnscurve_decode_rdata(parse_txt_rdata(rdata))
  return nonce1 + nonce2, box2

def parse_txt_rdata(rdata):
  strings = []
  i = 0
  while i < len(rdata):
    n = ord(rdata[i])
    i += 1
    strings.append(rdata[i:i + n])
    i += n
  return strings

def dnscurve_encode_queryname(nonce, box, pubkey, zone):
  if len(nonce) != 12:
    raise ValueError('Invalid nonce')
  if len(pubkey) != 32 or ord(pubkey[31]) >= 128:
    raise ValueError('Invalid public key')

  data = base32.encode(nonce + box)
  output = chunk(data, 50)
  output.append('x1a' + base32.encode(pubkey)[:51])
  output.extend(zone)

  return output

def dnscurve_decode_queryname(name):
  output = []

  for s in name:
    if len(s) > 50: break
    output.append(s)

  if len(s) != 54 or s[:3].lower() != 'x1a':
    raise ValueError('Not a DNSCurve query (1)')

  key = base32.decode(s[3:] + '0')
  r = base32.decode(''.join(output))
  if r < 12: raise ValueError('Not a DNSCurve query (2)')

  return (key, r[:12], r[12:])


def dnscurve_encode_rdata(nonce, box):
  if len(nonce) != 8: raise ValueError('Invalid nonce')
  data = nonce + box
  return chunk(data, 255)

def dnscurve_decode_rdata(rdata):
  data = ''.join(rdata)
  if len(data) < 12: raise ValueError('Invalid DNSCurve response')
  return (data[:12], data[12:])


# Split 's' into a list of strings with length no greater than 'n'.
def chunk(s, n):
  return [s[i:i+n] for i in range(0, len(s), n)]


class DNSClass:

        def __init__(self):
                """
                """

                self.randomgen = Random()

                #query
                self.id        = 0
                self.host      = ""
                self.qtype     = ""

                #TCP
                self.tcp       = 0

                #DNS flags
                self.flag_qr   = 0
                self.flag_aa   = 0
                self.flag_tc   = 0
                self.flag_rd   = 0
                self.flag_ra   = 0

                self.rcode     = 0
                self.opcode    = 0
                self.zero      = 0

                #DNS data
                self.question  = []
                self.answers   = []
                self.authority = []
                self.glue      = []
                return

        def packet(self, dnsid = ""):
                """
                """

                output = []
                output.append(dnsid)
                if self.flag_rd:
                        output.append('\x01\x00\x00\x01\x00\x00\x00\x00\x00\x00')
                else:
                        output.append('\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00')

                for label in dns_domain_fromdot(self.host):
                        output.append(chr(len(label)))
                        output.append(label)
                output.append(chr(0))

                try:
                        n = list(qtypes.keys())[list(qtypes.values()).index(self.qtype.upper())]
                except:
                        n = int(self.qtype)

                output.append(struct.pack('>H', n))
                output.append('\x00\x01')

                ret = ''.join(output)
                return ret

        def dnscurve_packet(self, zone = "", mypk = "", key = "", nonce1 = "", dnsid = "", txtid = ""):
                """
                """

                query0 = self.packet(dnsid)
                box = nacl.crypto_box_curve25519xsalsa20poly1305_afternm(query0, nonce1 + 12 * '\0', key)
                if zone:
                        query = dnscurve_encode_txt_query(nonce1, box, mypk, zone, txtid)
                else:
                        query = dnscurve_encode_streamlined_query(nonce1, box, mypk)
                return query

        def print_in_dig_format(self):
                """
                """
                flags = []
                if self.flag_qr:
                        flags.append('response')
                else:
                        flags.append('query')

                if self.opcode:  flags.append('weird-op')
                if self.flag_aa: flags.append('authoritative')
                if self.flag_tc: flags.append('truncated')
                if self.flag_rd: flags.append('recursion-desired')
                if self.flag_ra: flags.append('recursion-avail')
                if self.zero:    flags.append('weird-z')

                errors = {0: 'Success',
                        1: 'Format error',
                        2: 'Server failure',
                        3: 'Name error',
                        4: 'Not implemented',
                        5: 'Refused'}

                status = errors.get(self.rcode, 'Unknown')

                print >>sys.stderr, ';; DNS packet:', ' '.join(flags)
                print >>sys.stderr, ';; Status:', status
                print >>sys.stderr, ';; Id:', self.id
                print >>sys.stderr, ';; QUERY: %d, ANSWER: %d, AUTHORITY: %d, ADDITIONAL: %d' % (len(self.question), len(self.answers), len(self.authority), len(self.glue))
                print >>sys.stderr, ''
                for q in self.question:
                        print >>sys.stderr, ';%s\t\t\t%s\t%s' % q

                for (s, section) in [(self.answers, 'ANSWER'), (self.authority, 'AUTHORITY'), (self.glue, 'ADDITIONAL')]:
                        print >>sys.stderr, ""
                        print >>sys.stderr, ';; %s SECTION' % section
                        for (name, ttl, type, qclass, data) in s:
                                print >>sys.stderr, '%s\t\t%d\t%s\t%s\t%s' % (name, ttl, type, qclass, data)
                print >>sys.stderr, ''
                print >>sys.stderr, ''
                return



class DNSConsumer:

        def __init__(self):
                self.queries_ok = []
                self.queries_err = []
                return
          
        def handle_success(self, query = None, response = None, consumer = None):
                #truncated packet
                if response.flag_tc:
                        newquery = query
                        newquery.tcp = 1
                        DNSResolver(query = newquery,  consumer = consumer)
                        return
                #servfail
                if response.rcode not in [0,3]:
                        newquery = query
                        DNSResolver(query = newquery,  consumer = consumer)
                        return

                self.queries_ok.append((query, response))
                return
          
        def handle_error(self, query = None, err = "", consumer = None):
                try:
                        newquery = query
                        DNSResolver(query = newquery,  consumer = consumer)
                except Exception:
                        self.queries_err.append((query, err))
                return

        def print_result(self):
                for (q,r) in self.queries_ok:
                        r.print_in_dig_format()


class DNSQuery(DNSClass):
        """
        """

        def __init__(self):
                """
                """
                DNSClass.__init__(self)

                # ((ip,dnscurvekey,dnscurvezone,timeout),)
                self.transport = []
                self.transport_init = []


class DNSResult(DNSClass):
        """
        """

        def __init__(self):
                """
                """

                DNSClass.__init__(self)
                return 


def dns_create_query(host = "", qtype = "", ips = [], timeouts = [], flagrecursive = 0):
        """
        """

        ret         = DNSQuery()
        ret.host    = host
        ret.qtype   = qtype
        ret.flag_rd = flagrecursive;
        for timeout in timeouts:
                for (ip,key,zone) in ips:
                        ret.transport.append((ip,lib.parse_key(key),zone,timeout))

        ret.transport_init = ret.transport
        return ret

def dns_create_result(response = "", zone = "", key = "", nonce1 = "", dnsid = "", txtid = ""):
        """
        """

        if key:
                if zone:
                        if txtid != response[:2]:
                                raise Exception("Response transaction ID (DNSCurve TXT) didn't match")
                        nonce2, box = dnscurve_decode_txt_response(response)
                else:
                        nonce2, box = dnscurve_decode_streamlined_response(response)
                #XXX - TODO
                if nonce2[:12] != nonce1:
                        raise "Response nonce didn't match"
                #XXX - TODO
                p = nacl.crypto_box_curve25519xsalsa20poly1305_open_afternm(box, nonce2, key)
                #p = nacl.crypto_box_curve25519xsalsa20poly1305_open(box, nonce2, pubkey, mykey)
        else:
                p = response

        if dnsid != p[:2]:
                raise "Response transaction ID (DNS) didn't match"

        ret = DNSResult()

        (ret.id, f1, f2, nquery, nans, nauth, nadd) = struct.unpack('>HBBHHHH', p[:12])

        ret.flag_qr = f1 & 0x80
        ret.opcode  = f1 & 0x78
        ret.flag_aa = f1 & 0x04
        ret.flag_tc = f1 & 0x02
        ret.flag_rd = f1 & 0x01
        ret.flag_ra = f2 & 0x80
        ret.zero    = f2 & 0x70
        ret.rcode   = f2 & 0x0f

        if ret.flag_tc:
                return ret

        rest = p[12:]
        for n in range(nquery):
                (query, rest) = dns_query_read(rest, p)
                ret.question.append(query)

        for (section, count) in [(ret.answers, nans), (ret.authority, nauth),(ret.glue, nadd)]:
                for n in range(count):
                        ((name, type, qclass, ttl, data), rest) = dns_result_read(rest, p)
                        section.append((name, ttl, type, qclass, dns_pretty_rdata(type, qclass, data, p)))
        return ret
                


class DNSResolver(asyncore.dispatcher):

        def __init__(self, query = None, consumer = None):
                """
                """

                asyncore.dispatcher.__init__(self)

                self.consumer  = consumer
                self.query     = query

                if not len(self.query.transport):
                        raise Exception("XXX")
                (self.ip, self.remotepk, self.zone, self.timeout) = self.query.transport[0]
                self.query.transport = self.query.transport[1:]
        
                self.dnsid = nacl.randombytes(2)
                #dnscurve
                self.nonce = ""
                self.mypk  = ""
                self.txtid = ""
                self.key   = ""
                if self.remotepk:
                        self.nonce = nacl.randombytes(12)
                        mysk       = nacl.randombytes(32)
                        self.mypk  = nacl.crypto_scalarmult_curve25519_base(mysk)
                        self.key   = nacl.crypto_box_curve25519xsalsa20poly1305_beforenm(self.remotepk, mysk)
                        if self.zone:
                                self.txtid = nacl.randombytes(2)

                self.start     = time.time()
                self.connected = False
                self.response  = ""

                if self.remotepk:
                        self.request = self.query.dnscurve_packet(self.zone, self.mypk, self.key, self.nonce, self.dnsid, self.txtid)
                else:
                        self.request = self.query.packet(self.dnsid)

                if self.query.tcp:
                        self.request = "%s%s" % (struct.pack('>H', len(self.request)), self.request)

                try:
                        self.bindconnect()
                except socket.error:
                        self.handle_error()

        def bindconnect(self):

                try:
                        socket.inet_pton(socket.AF_INET6, self.ip)
                except:
                        family = socket.AF_INET
                        ipsend = "0.0.0.0"
                else:
                        family = socket.AF_INET6
                        ipsend = "::"


                if self.query.tcp:
                        self.create_socket(family, socket.SOCK_STREAM)
                else:
                        self.create_socket(family, socket.SOCK_DGRAM)

                for i in range(10):
                        try:
                                self.bind((ipsend,self.randomgen.randint(1025, 65535)))
                        except:
                                pass
                        else:
                                break

                self.connect((self.ip, 53))

        def handle_connect(self):
                """
                connection succeeded
                """

                try:
                        self.socket.getpeername()
                except socket.error:
                        self.socket.recv(1)
                self.connected = True
                return

        def handle_timeout(self):
                """
                timeout
                """

                try:
                        fce = self.consumer.handle_error
                except AttributeError:
                        pass
                else:
                        fce(self.query, "timeout", self.consumer)
                self.handle_close()

        def handle_error(self):

                (exc,err,tb) = sys.exc_info()
                if exc == socket.error:
                        text = os.strerror(err.errno).lower()
                else:
                        text = str(err).lower()
                try:
                        fce = self.consumer.handle_error
                except AttributeError:
                        pass
                else:
                        fce(self.query, text, self.consumer)
                self.handle_close()


        def handle_success(self):

                try:
                        fce = self.consumer.handle_success
                except AttributeError:
                        pass
                else:
                        fce(self.query, dns_create_result(self.response, self.zone, self.key, self.nonce, self.dnsid, self.txtid), self.consumer)
                self.handle_close()


        def handle_close(self):
                """
                """
                try:
                        self.close()
                except:
                        pass
                self.del_channel(self._map)

        def handle_expt(self):
                """
                """

                self.handle_error()

        def writable(self):
                """
                """

                return (not self.connected) or len(self.request)

        def readable(self):
                """
                """

                return not len(self.request)

        def handle_write(self):
                """
                """

                sent = self.send(self.request)
                self.request = self.request[sent:]

                return

        def handle_read(self):
                """
                """

                data = self.recv(4096)
                if not data:
                        raise Exception("Remote host closed connection")

                if self.query.tcp:
                        self.response = "%s%s" % (self.response, data)
                        if len(self.response) >= 2:
                                (ln,) = struct.unpack('>H', self.response[0:2])
                                if len(self.response) - 2 == ln:
                                        self.response = self.response[2:]
                                        self.handle_success()
                else:
                        self.response = data
                        self.handle_success()

                return


def dnstxt(zone = ""):
        """
        """

        ret = []

        dc = DNSConsumer()
        q = dns_create_query(zone,"txt",[(config.dnscacheip,"","")], [3,11], 1)
        qc = [(q,dc)]
        doit(qc)

        if len(dc.queries_err) != 0:
                #XXX hide errors
                return []
        (q,r) = dc.queries_ok[0]
        if len(r.answers) == 0:
                #XXX hide errors
                return []
        for a in r.answers:
                ret.append(a[4])
        return ret

def _getdnskey(name = ""):
        """
        """

        for n in name.split("."):
                if len(n) != 54:
                        continue
                if n[0:3] != "uz5":
                        continue
                nn = "%s0" % (n[3:])
                try:
                        return lib.tohex(base32.decode(nn))
                except:
                        pass
        return ""


def _getcurvecpkey(name = ""):
        """
        """

	flagkey = False
	key = None

        for n in name.split("."):
		if flagkey == True:
			if len(n) != 32:
				flagkey = False
				key = None
			else:
                                try:
                                        key = lib.tohex(lib.fromhex(key))
                                except:
                                        flagkey = False
                                        key = None
                                else:
				        return (key, n)
                if len(n) != 54:
			flagkey = False
                        continue
                if n[0:3] != "uz7":
			flagkey = False
                        continue
                nn = "%s0" % (n[3:])
                try:
                        key = lib.tohex(base32.decode(nn))
                except:
			flagkey = False
		else:
			flagkey = True
        return ("", "")


def dnsns(zone = ""):
        """
        """

        ret = []

        dc1 = DNSConsumer()
        q = dns_create_query(zone,"ns",[(config.dnscacheip,"","")], [3,11], 1)
        qc = [(q,dc1)]
        doit(qc)

        if len(dc1.queries_err) != 0:
                #XXX hide errors
                return []
        (q,r) = dc1.queries_ok[0]
        if len(r.answers) == 0:
                #XXX hide errors
                return []

        dc2 = DNSConsumer()
        qc2 = []
        for a in r.answers:
                q2 = dns_create_query(a[4],"a",[(config.dnscacheip,"","")], [3,11], 1)
                qc2.append((q2,dc2))
        doit(qc2)

        for (q,r) in dc2.queries_ok:
                if len(r.answers) == 0:
                        #XXX hide errors
                        continue
                for a in r.answers:
                        k = _getdnskey(a[0])
                        ret.append((a[4], k))
        return ret


#XXX - key parsed only from CNAME
#XXX - no service selector check
#XXX - first key wins
def dnsa(zone = ""):

        ret = []

	(k,e) = _getcurvecpkey(zone)
	aa = []

        dc = DNSConsumer()
        q = dns_create_query(zone,"a",[(config.dnscacheip,"","")], [3,11], 1)
        qc = [(q,dc)]
        doit(qc)

        if len(dc.queries_err) != 0:
                #XXX hide errors
                return []
        (q,r) = dc.queries_ok[0]
        if len(r.answers) == 0:
                #XXX hide errors
                return []
        for a in r.answers:
		if a[2] == "A":
			aa.append(a[4])
			continue
		if a[2] == "CNAME":
			if len(k) != 64 or len(e) != 32:
				(k,e) = _getcurvecpkey(a[4])
	for a in aa:
		ret.append((a,k,e))
        return ret
