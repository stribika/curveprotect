# 20130121
# Jan Mojzis
# Public domain.

import os, lib

try:
        import nacl
except ImportError:
        import slownacl as nacl

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
        noncekey = os.urandom(16)
        return (pk, sk, noncekey)

def makekey(pk, sk, noncekey, fn):

        os.umask(022)
        os.mkdir(fn,0755)
        os.chdir(fn)
        create("PUBLICKEY", lib.tohex(pk));
        os.mkdir(".EXPERTSONLY",0700)
        os.umask(077);
        create(".EXPERTSONLY/SECRETKEY", lib.tohex(sk));
        create(".EXPERTSONLY/NONCEKEY", lib.tohex(noncekey));



