#20121220
#Jan Mojzis
#Public domain.

#help
if [ x"`bintobase32 -h 2>&1 | grep '^bintobase32: usage:'`" != x"bintobase32: usage:" ]; then
  echo "bintobase32: help error"; exit 111
fi

# zero bytes
bintobase32 < /dev/null | tr -d '\n'

#checksum
dict=abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789
sum=97af5f72ceabeacc7d96c165fbec8af87daa55cc6f793f2f9fa3f3af64da51a2
if [ x"`echo ${dict} | bintobase32 | nacl-sha256`" != x"${sum}" ]; then
  echo "bintobase32: checksum error"; exit 111
fi

#disk full
if [ -w /dev/full ]; then
  e=`echo aa | bintobase32 2>&1 > /dev/full || :`
  if [ x"${e}" != x"bintobase32: fatal: unable to write output: out of disk space" ]; then
    echo "${e}"; exit 111
  fi
fi
