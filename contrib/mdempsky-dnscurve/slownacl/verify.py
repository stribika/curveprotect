__all__ = ['crypto_verify16', 'crypto_verify32']

import functools

def crypto_verify16(a, b):
  if len(a) != 16 or len(b) != 16:
    raise ValueError('Not 16 bytes')
  return 0 == reduce(lambda x, y: x | y, [ord(a) ^ ord(b) for (a,b) in zip(a,b)])

def crypto_verify32(a, b):
  if len(a) != 32 or len(b) != 32:
    raise ValueError('Not 32 bytes')
  return 0 == reduce(lambda x, y: x | y, [ord(a) ^ ord(b) for (a,b) in zip(a,b)])
