# This is not part of the slownacl library, it's just a unittest to verify a
# few things against the Python wrapping of NaCl.

import slownacl
import nacl
import random

def check_funcs(its, a, b, arglens):
  '''Checks that two functions are a same by calling each with random strings
     where the lengths of the random strings are given in @arglens (list of
     int). The entries of @arglens may also be a range tuple'''

  def r(x):
    if type(x) == int:
      return slownacl._randombytes(x)
    elif type(x) == tuple:
      length = random.randint(*x)
      return slownacl._randombytes(length)

  for i in range(its):
    args = [r(x) for x in arglens]
    if a(*args) != b(*args):
      print
      print
      print 'failed after %d tests. Failing input:' % i
      print args
      print
      print repr(a(*args))
      print repr(b(*args))
      return False

  return True

def check(its, name, arglens):
  print ('Checking %s...' % name),
  if check_funcs(its, getattr(nacl, name), getattr(slownacl, name), arglens):
    print 'ok'
  else:
    print 'FAILED'

if __name__ == '__main__':
  check(1024, 'crypto_hash_sha512', [(1, 100)])
  check(1024, 'crypto_auth_hmacsha512', [(1, 100), 32])
  check(1024, 'crypto_onetimeauth_poly1305', [(1, 100), 32])
  check(256, 'crypto_scalarmult_curve25519_base', [32])
  check(128, 'crypto_stream_salsa20_xor', [(0, 1024), 8, 32])
  check(128, 'crypto_stream_xsalsa20_xor', [(0, 1024), 24, 32])
  check(64, 'crypto_scalarmult_curve25519', [32, 32])
