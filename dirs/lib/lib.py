# 20110802
# Jan Mojzis
# Public domain.

import sys, os, traceback, random, resource, time, check, base32
import struct, errno, binascii

from config import basedir, tinydnsip, forwarder_ips

#XXX
curvecp_keydir=".keys/curvecp"
ed25519_keydir=".keys/ed25519"
dnscurve_keydir=".keys/dnscurve"

gpg_keydir=".keys/gnupg"

#XXX
protocols = {
        "http":         {"start" : "clientfirst", "port" : "80",   "selector" : "h",},
        "https":        {"start" : "clientfirst", "port" : "443",  "selector" : "h",},
        "httpproxy":    {"start" : "clientfirst", "port" : "3128", "selector" : "h",},
        "jabber":       {"start" : "clientfirst", "port" : "5222", "selector" : "j",},
        "jabbers":      {"start" : "clientfirst", "port" : "5223", "selector" : "j",},
        "smtp":         {"start" : "serverfirst", "port" : "25",   "selector" : "s",},
        "smtps":        {"start" : "clientfirst", "port" : "465",  "selector" : "s",},
        "pop":          {"start" : "serverfirst", "port" : "110",  "selector" : "p",},
        "pops":         {"start" : "clientfirst", "port" : "995",  "selector" : "p",},
        "imap":         {"start" : "serverfirst", "port" : "143",  "selector" : "i",},
        "imaps":        {"start" : "clientfirst", "port" : "993",  "selector" : "i",},
        "rsync":        {"start" : "serverfirst", "port" : "873",  "selector" : "r",},
        "ssh":          {"start" : "serverfirst", "port" : "22",   "selector" : "",},
        "irc":          {"start" : "clientfirst", "port" : "6667", "selector" : "",},
}


def svc(d = ""):
        """
        """

        try:
                fd = os.open("%s/supervise/ok" % (d), os.O_WRONLY | os.O_NDELAY | os.O_TRUNC | os.O_CREAT)
        except (IOError, OSError),e:
                if e[0] == errno.ENXIO:
                        return "supervise not running"
                return "unable to open supervise/ok: %s" % (e[1])
        else:
                os.close(fd)

        try:
                f = open("%s/supervise/status" % (d), mode='rb')

                status = f.read()
                f.close()
        except (IOError, OSError),e:
                return "unable to open supervise/status: %s" % (e[1])

        normallyup = 0
        if not os.path.exists("%s/down" % (d)):
                normallyup = 1

        (xtm, dummy, spid, paused, want) = struct.unpack('>Q4s4sBs', status)
        tm = xtm - 4611686018427387914
        (pid,) = struct.unpack('<i', spid)

        if (pid > 0):
                #ret = "up (pid %d) %d seconds" % (pid, time.time() - tm)
                ret = "up %d seconds" % (time.time() - tm)
        else:
                ret = "down %d seconds" % (time.time() - tm)

        if pid > 0 and normallyup == 0:
                ret += ", normally down"

        if pid == 0 and normallyup > 0:
                ret += ", normally up"

        if pid > 0 and paused > 0:
                ret += ", paused"

        if pid == 0 and want == 'u':
                ret += ", want up"

        if pid > 0 and want == 'd':
                ret += ", want down"

        return ret


def escape(s = ""):
        """
        """

        s = s.replace("&", "&amp;")
        s = s.replace("<", "&lt;")
        s = s.replace(">", "&gt;")
        s = s.replace('"', "&quot;")
        s = s.replace("'", "&apos;")
        #s = s.replace("/", "&frasl;")
        return s

def openreadcloseescape(fn = ""):
        """
        """

        f = open(fn, "r")
        data = f.read()
        f.close

        data = data.split('\n')[0]
        data = data.split('\r')[0]
        return escape(data)

def openreadallcloseescape(fn = ""):
        """
        """

        f = open(fn, "r")
        data = f.read()
        f.close

        return escape(data)

def openreadclose(fn = ""):
        """
        """

        f = open(fn, "rb")
        data = f.read()
        f.close
        return data

def tryunlink(fn = ""):
        """
        """

        if os.path.exists(fn):
                os.unlink(fn)
        return


def tryrmdir(dr):

        try:
                os.rmdir(dr)
        except OSError, err:
                if not err.errno in [errno.ENOTEMPTY]:
                        raise

def trymkdir(dr):
        try:
                os.mkdir(dr)
        except OSError, err:
                if not err.errno in [errno.EEXIST]:
                        raise



def openwriteclose(fn = "", data = ""):
        """
        """

        f = open(fn, "w")
        f.write(data)
        f.flush()
        f.close()
        return


def openwritecloserename(fn = "", data = "", d=""):
        """
        """

        r = random.randint(0,9999999999)
        p = os.getpid()
        tmp="%s/tmp/%d.%d.tmp" % (d,p,r)
        try:
                f = open(tmp, "w")
                f.write(data)
                f.flush()
                os.fsync(f.fileno())
                f.close()
                os.rename(tmp,fn)
        finally:
                tryunlink(tmp)
        return

def recursiverrm(d = ""):
        """
        """

        if not os.path.exists(d):
                return

        try:
                os.unlink(d)
                return
        except:
                pass

        try:
                os.rmdir(d)
                return
        except:
                pass

        for f in os.listdir(d):
                dd = "%s/%s" % (d,f)
                recursiverrm(dd)

        os.rmdir(d)
        return


def taiconv(s):
        """
        """
        if s[0] == "@": s = s[1:]

        try:
                return long(s[:16], 16) - 4611686018427387904
        except NameError:
                return int(s[:16], 16) - 4611686018427387904


def taifmt(s):
        """
        """
        sec = taiconv(s)
        return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(sec))



def get_tb():
        """
        """

        s = "Traceback (most recent call last):\n"

        t, v, tb = sys.exc_info()
        lines = traceback.extract_tb(tb)

        for line in lines:
                s = "%s  File \"%s\", line %d, in %s\n    %s\n" % (s, line[0], line[1], line[2], line[3])

        s = "%s%s: %s" % (s, t, v)

        return s


def setutime(fn = "", tm = 0.0):
        """
        """

        try:
                x = (long(time.time()), long(tm))
        except NameError:
                x = (int(time.time()), int(tm))
        os.utime(fn, x)
        return

def getmtime(fn = ""):
        """
        """

        st = os.stat(fn)
        return st.st_mtime


def getid(x = ""):
        """
        XXX
        """

        i = int(x)
        if (i > 65535):
                s = struct.pack("<I", int(x))
                return struct.unpack("<i", s)[0]
        return i

def chroot():
        """
        """

        root = os.getenv("ROOT")
        if not root:
                raise ValueError("$ROOT not set")

        #chroot
        os.chdir(root)
        os.chroot(".")

def droproot():
        """
        """

        #set UID, GID 
        gid = os.getenv("GID")
        if not gid:
                raise ValueError("$GID not set")
        uid = os.getenv("UID")
        if not uid:
                raise ValueError("$UID not set")
        os.setgid(getid(gid))
        os.setuid(getid(uid))

def droproot2():
        """
        """

        #set UID, GID 
        gid = os.getenv("USRGID")
        if not gid:
                raise ValueError("$USRGID not set")
        uid = os.getenv("USRUID")
        if not uid:
                raise ValueError("$USRUID not set")
        os.setgid(getid(gid))
        os.setuid(getid(uid))


def limit_resources2(d = ""):

        root = os.getenv("ROOT")
        if not root:
                raise ValueError("$ROOT not set")
        root = "%s/%s" % (root, d)

        #chroot
        os.chdir(root)
        os.chroot(".")

        droproot2()

        #no new processes
        try:
                resource.setrlimit(resource.RLIMIT_NPROC, (0,0))
        except AttributeError:
                pass


def limit_resources(d = ""):
        """
        """


        root = os.getenv("ROOT")
        if not root:
                raise ValueError("$ROOT not set")
        root = "%s/%s" % (root, d)

        #chroot
        os.chdir(root)
        os.chroot(".")

        droproot()

        #no new processes
        try:
                resource.setrlimit(resource.RLIMIT_NPROC, (0,0))
        except AttributeError:
                pass


def curvecp_keys():
        """
        """

        keys = []
        try:
                for name in os.listdir(curvecp_keydir):
                        keys.append(name)
        except:
                pass
        return keys


def ed25519_keys():
        """
        """

        keys = []
        try:
                for name in os.listdir(ed25519_keydir):
                        keys.append(name)
        except:
                pass
        return keys


def dnscurve_keys():
        """
        """

        keys = []
        try:
                for name in os.listdir(dnscurve_keydir):
                        keys.append(name)
        except:
                pass
        return keys


def urlparse(url = ""):
        """
        """

        #parse protocol
        pos = url.find("://")
        if pos == -1:
                proto = ""
        else:
                proto = url[0:pos]
                url = url[pos+3:]

        #parse path
        pos = url.find("/")
        if pos == -1:
                path = ""
        else:
                path = url[pos+1:]
                url = url[0:pos]

        #parse port
        pos = url.find(":")
        if pos == -1:
                port = 0
        else:
                try:
                        port = int(url[pos+1:])
                except (ValueError):
                        port = 0
                url = url[0:pos]

        host = url
        return (proto,host,port,path)


def parse_key(key = ""):
        """
        """

        keylen = len(key)

        if keylen == 64:
                #hex
                check.hexkey(key)
                return fromhex(key)
        elif keylen == 54:
                #uz5 + base32
                if key[0:3] != "uz5":
                        raise ValueError("first 3 characters must be nym uz5")
                rkey = tohex(base32.decode("%s0" % key[3:]))
                check.hexkey(rkey)
                return fromhex(rkey)
        elif keylen == 51:
                #base32
                rkey = tohex(base32.decode("%s0" % key))
                check.hexkey(rkey)
                return fromhex(rkey)


def parse_ipkey(s = ""):
        """
        ip:hexkey
        ip:uz5+base32key
        ip:base32key
        """

        s = s.split('\n')[0]

        ss = s.split('|')
        if len(ss) != 2:
                raise ValueError("string must be in format ip:key")
        ip  = ss[0]
        key = ss[1].lower()
        keylen = len(key)
        check.ip(ip)

        if keylen == 64:
                #hex
                check.hexkey(key)
                return (ip.strip(), "uz5%s" % (base32.encode(fromhex(key))[:51]))
        elif keylen == 54:
                #uz5 + base32
                if key[0:3] != "uz5":
                        raise ValueError("first 3 characters must be nym uz5")
                rkey = tohex(base32.decode("%s0" % key[3:]))
                check.hexkey(rkey)
                return (ip.strip(), "uz5%s" % (base32.encode(fromhex(rkey))[:51]))
        elif keylen == 51:
                #base32
                rkey = tohex(base32.decode("%s0" % key))
                check.hexkey(rkey)
                return (ip.strip(), "uz5%s" % (base32.encode(fromhex(rkey))[:51]))
        else:
                raise ValueError("key must must have length 64bytes in hex, 51 in base32 or 54 in base32 + nym uz5")


def tohex(data = ""):
        """
        """

        return binascii.hexlify(data)

def fromhex(data = ""):
        """
        """

        return binascii.unhexlify(data)


def print_refresh(r = ""):
        print('<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">')
        print("<html>")
        print("<head>")
        print('<meta http-equiv="refresh" content="0; url=%s">' % (r))
        print("</head>")
        print("</html>")

def print_refresh_success(r = ""):
        print('<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">')
        print("<html>")
        print("<head>")
        #print('<meta http-equiv="refresh" content="0; url=/%s.dynhtml?success">' % (r))
        print('<meta http-equiv="refresh" content="0; url=/%s.dynhtml">' % (r))
        print("</head>")
        print("</html>")

def print_refresh_failed(r = ""):
        print("<html>")
        print('<meta http-equiv="refresh" content="0; url=/%s.dynhtml?failed">' % (r))
        print("<head>")
        print("</head>")
        print("</html>")
        print("<body>")
        print("<pre>")
        #print (get_tb())
        sys.stderr.write(get_tb())
        sys.stderr.write("\n")
        sys.stderr.flush()
        print 
        print("</pre>")
        print("</body>")

