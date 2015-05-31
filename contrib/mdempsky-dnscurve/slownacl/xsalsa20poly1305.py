from util import xor, _randombytes
from salsa20 import crypto_core_hsalsa20, crypto_stream_xsalsa20
from poly1305 import crypto_onetimeauth_poly1305, crypto_onetimeauth_poly1305_verify
from curve25519 import crypto_scalarmult_curve25519, crypto_scalarmult_curve25519_base

__all__ = ['crypto_secretbox_xsalsa20poly1305', 'crypto_secretbox_xsalsa20poly1305_open', 'box_curve25519xsalsa20poly1305_keypair', 'box_curve25519xsalsa20poly1305', 'box_curve25519xsalsa20poly1305', 'box_curve25519xsalsa20poly1305_open', 'box_curve25519xsalsa20poly1305_beforenm', 'box_curve25519xsalsa20poly1305_afternm', 'box_curve25519xsalsa20poly1305_open_afternm']

def crypto_secretbox_xsalsa20poly1305(m, n, k):
  s = crypto_stream_xsalsa20(32 + len(m), n, k)
  c = xor(m, s[32:])
  a = crypto_onetimeauth_poly1305(c, s[:32])
  return a + c

def crypto_secretbox_xsalsa20poly1305_open(c, n, k):
  if len(c) < 16: raise ValueError('Too short for XSalsa20Poly1305 box')
  s = crypto_stream_xsalsa20(32, n, k)
  if not crypto_onetimeauth_poly1305_verify(c[:16], c[16:], s):
    raise ValueError('Bad authenticator for XSalsa20Poly1305 box')
  s = crypto_stream_xsalsa20(16 + len(c), n, k)
  return xor(c[16:], s[32:])


def crypto_box_curve25519xsalsa20poly1305_keypair():
  sk = _randombytes(32)
  pk = crypto_scalarmult_curve25519_base(sk)
  return (pk, sk)

def crypto_box_curve25519xsalsa20poly1305(m, n, pk, sk):
  return crypto_box_curve25519xsalsa20poly1305_afternm(
      m, n, crypto_box_curve25519xsalsa20poly1305_beforenm(pk, sk))

def crypto_box_curve25519xsalsa20poly1305_open(c, n, pk, sk):
  return crypto_box_curve25519xsalsa20poly1305_open_afternm(
      c, n, crypto_box_curve25519xsalsa20poly1305_beforenm(pk, sk))

def crypto_box_curve25519xsalsa20poly1305_beforenm(pk, sk):
  return crypto_core_hsalsa20('\0' * 16, crypto_scalarmult_curve25519(sk, pk))

def crypto_box_curve25519xsalsa20poly1305_afternm(m, n, k):
  return crypto_secretbox_xsalsa20poly1305(m, n, k)

def crypto_box_curve25519xsalsa20poly1305_open_afternm(c, n, k):
  return crypto_secretbox_xsalsa20poly1305_open(c, n, k)
