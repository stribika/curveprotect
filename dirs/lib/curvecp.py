# 20111025
# Jan Mojzis
# Public domain.

import os

try:
        import nacl
except ImportError:
        import slownacl as nacl

#__all__ = ['keygen','makekey','unlinkkey']
__all__ = ['keygen','makekey']

def create(fn, data):
        f = open(fn, "wb")
        f.write(data)
        f.flush()
        os.fsync(f.fileno())
        f.close()

def keygen():
        sk = os.urandom(32)
        pk = nacl.crypto_scalarmult_curve25519_base(sk)
        noncekey = os.urandom(32)
        return (pk, sk, noncekey)

def makekey(pk, sk, noncekey, fn):

        noncecounter = "\0\0\0\0\0\0\0\0"
        lock = "\0"

        os.umask(022)
        os.mkdir(fn,0755)
        os.chdir(fn)
        create("publickey",pk);
        os.mkdir(".expertsonly",0700)
        os.umask(077);
        create(".expertsonly/secretkey",sk);
        create(".expertsonly/lock",lock);
        create(".expertsonly/noncekey", noncekey);
        create(".expertsonly/noncecounter",noncecounter);

#def unlinkkey(fn):
#        os.unlink("%s/.expertsonly/secretkey" % (fn))
#        os.unlink("%s/.expertsonly/lock" % (fn))
#        os.unlink("%s/.expertsonly/noncekey" % (fn))
#        os.unlink("%s/.expertsonly/noncecounter" % (fn))
#        os.unlink("%s/publickey" % (fn))
#        os.rmdir("%s/.expertsonly" % (fn))
#        os.rmdir(fn)
