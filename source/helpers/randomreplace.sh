#20130119
#Jan Mojzis
#Public domain.

a=`randomreplace < randomreplace.c | nacl-sha256`
b=`nacl-sha256 < randomreplace.c`

#help
if [ x"`randomreplace -h 2>&1 | grep '^randomreplace: usage:'`" != x"randomreplace: usage:" ]; then
  echo "randomreplace: help error"; exit 111
fi

if [ x"${a}" = x"${b}" ]; then
  echo "randomreplace test failed"; exit 111
fi

a=`randomreplace </dev/null`
if [ x"${a}" != x ]; then
  echo "randomreplace: zero input test failed"; exit 111
fi

#disk full
if [ -w /dev/full ]; then
  e=`echo aa | randomreplace 2>&1 > /dev/full || :`
  if [ x"${e}" != x"randomreplace: fatal: unable to write output: out of disk space" ]; then
    echo "randomreplace test failed: ${e}"; exit 111
  fi
fi
