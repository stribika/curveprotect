# 20120322
# Jan Mojzis
# Public domain.


import asyncore, socket, time, sys, errno, os
from asyncore import socket_map as map
try:
        import encodings.idna 
except ImportError:
        pass

try:
        import nacl
except ImportError:
        import slownacl as nacl


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


class Consumer:

        def __init__(self):
                self.host_ok = []
                self.host_err = {}
                return

        def handle_success(self, host = "", dummy = ""):
                self.host_ok.append(host)
                return

        def handle_error(self, host = "", err = ""):
                self.host_err[host]=err
                return

        def result(self):
                err = []
                for host in self.host_err:
                        e = self.host_err[host]
                        err.append((host,e))
                return (self.host_ok, err)

        def is_done(self):
                return (len(self.host_ok) > 0)


allzero="\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"

class Hello(asyncore.dispatcher):

        def __init__(self, host = "", ip = "", port = 0, key = "", ext = "", timeout = 5, consumer = None, clientext = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"):
                asyncore.dispatcher.__init__(self)
                self.consumer = consumer
                self.host = host
                self.ip = ip
                self.port = port
                self.key = key
                self.ext = ext
                self.start = time.time()
                self.timeout = timeout
                self.clientext = clientext

                nonce = "CurveCP-client-H%s" % nacl.randombytes(8)

                packet = "QvnQ5XlH"
                packet += self.ext
                packet += self.clientext #XXX
                (self.clientshorttermpk, self.clientshorttermsk) = nacl.crypto_box_curve25519xsalsa20poly1305_keypair()
                packet += self.clientshorttermpk
                packet += allzero
                packet += nonce[16:]
                text = nacl.crypto_box_curve25519xsalsa20poly1305(allzero, nonce, self.key, self.clientshorttermsk)
                packet += text
                self.packet = packet

                try:
                        self.create_socket(socket.AF_INET, socket.SOCK_DGRAM)
                        self.bind(('', 0))
                except socket.error:
                        self.handle_error()

        def handle_write(self):
                if len(self.packet):
                        sent = self.sendto(self.packet, (self.ip, self.port))
                        if (sent != len(self.packet)):
                                self.handle_error()
                        self.packet = ""


        def handle_read(self):
                data, address = self.recvfrom(256)

                if len(data) != 200:
                        self.handle_xerror("bad cookie packet length")

                if data[0:8] != "RL3aNMXK":
                        self.handle_xerror("bad magic")

                if data[8:24] != self.clientext:
                        self.handle_xerror("bad client extension")

                if data[24:40] != self.ext:
                        self.handle_xerror("bad server extension")

                servernonce = "CurveCPK%s" % data[40:56]

                try:
                        text = nacl.crypto_box_curve25519xsalsa20poly1305_open(data[56:], servernonce, self.key, self.clientshorttermsk)
                except:
                        self.handle_xerror("unable to verify packet")

                try:
                        fce = self.consumer.handle_success
                except AttributeError:
                        pass
                else:
                        fce(self.host)

                self.close()

        def writable(self): return len(self.packet)
        def readable(self): return True
        def handle_connect(self): pass


        def handle_xerror(self, text):
                try:
                        fce = self.consumer.handle_error
                except AttributeError:
                        pass
                else:
                        fce(self.host, text)
                self.handle_close()

        def handle_error(self):

                try:
                        fce = self.consumer.handle_error
                        (exc,err,tb) = sys.exc_info()
                except AttributeError:
                        pass
                else:
                        fce(self.host, os.strerror(err.errno).lower())
                self.handle_close()

        def handle_timeout(self):
                try:
                        fce = self.consumer.handle_error
                except AttributeError:
                        pass
                else:
                        fce(self.host, "timeout")
                self.handle_close()

        def handle_close(self):
                self.close()
                self.del_channel(self._map)


        def handle_expt(self): self.handle_error()

def doit(consumer = None):
        while True:
                if consumer:
                        if consumer.is_done():
                                for fd, obj in list(map.items()):
                                        obj.handle_close()
                                break
                if not len(map):
                        break
                poll()



