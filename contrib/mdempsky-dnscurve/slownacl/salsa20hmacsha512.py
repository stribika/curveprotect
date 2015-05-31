from util import xor, _randombytes
from salsa20 import crypto_stream_salsa20
from sha512 import crypto_hash_sha512, crypto_auth_hmacsha512, crypto_auth_hmacsha512_verify
from curve25519 import crypto_scalarmult_curve25519, crypto_scalarmult_curve25519_base

__all__ = ['crypto_secretbox_salsa20hmacsha512', 'crypto_secretbox_salsa20hmacsha512_open', 'crypto_box_curve25519salsa20hmacsha512_keypair', 'crypto_box_curve25519salsa20hmacsha512', 'crypto_box_curve25519salsa20hmacsha512_open', 'crypto_box_curve25519salsa20hmacsha512_beforenm', 'crypto_box_curve25519salsa20hmacsha512_afternm', 'crypto_box_curve25519salsa20hmacsha512_open_afternm']

def crypto_secretbox_salsa20hmacsha512(m, n, k):
  s = crypto_stream_salsa20(len(m) + 32, n, k)
  c = xor(m, s[32:])
  a = crypto_auth_hmacsha512(c, s[:32])
  return a + c

def crypto_secretbox_salsa20hmacsha512_open(c, n, k):
  if len(c) < 32: raise ValueError('Too short for Salsa20HMACSHA512 box')
  s = crypto_stream_salsa20(32, n, k)
  if not crypto_auth_hmacsha512_verify(c[:32], c[32:], s):
    raise ValueError('Bad authenticator for Salsa20HMACSHA512 box')
  s = crypto_stream_salsa20(len(c), n, k)
  return xor(c[32:], s[32:])


def crypto_box_curve25519salsa20hmacsha512_keypair():
  sk = _randombytes(32)
  pk = crypto_scalarmult_curve25519_base(sk)
  return (pk, sk)

def crypto_box_curve25519salsa20hmacsha512(m, n, pk, sk):
  return crypto_box_curve25519salsa20hmacsha512_afternm(
    m, n, crypto_box_curve25519salsa20hmacsha512_beforenm(pk, sk))

def crypto_box_curve25519salsa20hmacsha512_open(c, n, pk, sk):
  return crypto_box_curve25519salsa20hmacsha512_open_afternm(
    c, n, crypto_box_curve25519salsa20hmacsha512_beforenm(pk, sk))

def crypto_box_curve25519salsa20hmacsha512_beforenm(pk, sk):
    return crypto_hash_sha512(crypto_scalarmult_curve25519(sk, pk))[:32]

def crypto_box_curve25519salsa20hmacsha512_afternm(m, n, k):
  return crypto_secretbox_salsa20hmacsha512(m, n, k)

def crypto_box_curve25519salsa20hmacsha512_open_afternm(c, n, k):
  return crypto_secretbox_salsa20hmacsha512_open(c, n, k)
