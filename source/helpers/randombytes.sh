#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`randombytes -h 2>&1 | grep '^randombytes: usage:'`" != x"randombytes: usage:" ]; then
  echo "randombytes: help error"; exit 111
fi

#bad/negative input
for i in a '' -1 -01 100000000000000000000000000000000000000; do
  e=`randombytes "${i}" 2>&1 | tail -1`
  if [ x"${e}" != x"randombytes: fatal: unable to parse number" ]; then
    echo "randombytes: negative test for >>${i}<< failed: ${e}"; exit 111
  fi
done

#zero length
for i in 0 +0 -0 000; do
  randombytes "${i}" || { echo "zerobytes: zero length test failed"; exit 111; }
done

#length check
if [ "`randombytes +0100 | wc -c`" -ne 100 ]; then
  echo "randombytes: length check failed"; exit 111
fi

#disk full
if [ -w /dev/full ]; then
  e=`randombytes 10 2>&1 > /dev/full || :`
  if [ x"${e}" != x"randombytes: fatal: unable to write output: out of disk space" ]; then
    echo "${e}"; exit 111
  fi
fi
