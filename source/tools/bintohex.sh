#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`bintohex -h 2>&1 | grep '^bintohex: usage:'`" != x"bintohex: usage:" ]; then
  echo "bintohex: help error"; exit 111
fi

# zero bytes
bintohex < /dev/null | tr -d '\n'

#checksum
dict=abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789
sum=9f68160fa3c884b4b4c407a63b50b4b1a7462661aec5d8a79c83f5f743de5ce0
if [ x"`echo ${dict} | bintohex | nacl-sha256`" != x"${sum}" ]; then
  echo "bintohex: checksum error"; exit 111
fi

#disk full
if [ -w /dev/full ]; then
  e=`echo aa | bintohex 2>&1 > /dev/full || :`
  if [ x"${e}" != x"bintohex: fatal: unable to write output: out of disk space" ]; then
    echo "${e}"; exit 111
  fi
fi
