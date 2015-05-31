digits = '0123456789bcdfghjklmnpqrstuvwxyz'

def encode(s):
  v = 0
  vbits = 0
  output = []

  for c in s:
    v |= ord(c) << vbits
    vbits += 8

    while vbits >= 5:
      output.append(digits[v & 31])
      v >>= 5
      vbits -= 5

  if vbits:
    output.append(digits[v])

  return ''.join(output)


def decode(s):
  v = 0
  vbits = 0
  output = []

  for c in s.lower():
    try:
      u = digits.index(c)
    except ValueError:
      raise ValueError('Invalid base-32 input')
    v |= u << vbits
    vbits += 5

    if vbits >= 8:
      output.append(chr(v & 255))
      v >>= 8
      vbits -= 8

  if vbits >= 5 or v:
    raise ValueError('Invalid base-32 input')

  return ''.join(output)
