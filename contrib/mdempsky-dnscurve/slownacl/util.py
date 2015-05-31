__all__ = ['xor', '_randombytes', 'randombytes']

devurandom=open('/dev/urandom')

def xor(s, t):
  output = []
  if len(s) != len(t): raise ValueError('Cannot xor strings of unequal length')
  for i in range(len(s)):
    output.append(chr(ord(s[i]) ^ ord(t[i])))
  return ''.join(output)

def _randombytes(n):
  return devurandom.read(n)

def randombytes(n):
  return devurandom.read(n)
