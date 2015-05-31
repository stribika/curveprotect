# 20111022
# Jan Mojzis
# Public domain.


import asyncore, socket, time, sys, errno, os
from asyncore import socket_map as map
try:
        import encodings.idna 
except ImportError:
        pass

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



class Ping(asyncore.dispatcher):

        def __init__(self, host = "", port = 53, timeout = 5, consumer = None, ignorerefused = True):
                asyncore.dispatcher.__init__(self)
                self.consumer = consumer
                self.host = host
                self.port = port
                self.start = time.time()
                self.timeout = timeout
                self.ignorerefused = ignorerefused

                try:
                        self.create_socket(socket.AF_INET, socket.SOCK_STREAM)
                        self.connect((self.host, self.port))
                except socket.error:
                        self.handle_error()

        def handle_write(self): self.close()
        def handle_read(self):  self.close()
        def writable(self): return True
        def readable(self): return False

        def handle_connect(self):

                try:
                        self.socket.getpeername()
                except:
                        #sets errno
                        self.socket.recv(1)
                try:
                        fce = self.consumer.handle_success
                except AttributeError:
                        pass
                else:
                        fce(self.host)
                self.handle_close()
                return


        def handle_error(self):

                try:
                        fce = self.consumer.handle_error
                        (exc,err,tb) = sys.exc_info()
                        if exc == socket.error:
                                if err.errno in [errno.ECONNREFUSED]:
                                        if self.ignorerefused:
                                                fce = self.consumer.handle_success
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


