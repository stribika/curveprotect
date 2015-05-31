#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`zerobytes -h 2>&1 | grep '^zerobytes: usage:'`" != x"zerobytes: usage:" ]; then
  echo "zerobytes: help error"; exit 111
fi

#bad/negative input
for i in a '' -1 -01 100000000000000000000000000000000000000; do
  e=`zerobytes "${i}" 2>&1 | tail -1`
  if [ x"${e}" != x"zerobytes: fatal: unable to parse number" ]; then
    echo "zerobytes: negative test for >>${i}<< failed: ${e}"; exit 111
  fi
done

#zero length
for i in 0 +0 -0 000; do
  zerobytes "${i}" || { echo "zerobytes: zero length test failed"; exit 111; }
done

#length check
if [ "`zerobytes +0100 | wc -c`" -ne 100 ]; then
  echo "zerobytes: length check failed"; exit 111
fi

#disk full
if [ -w /dev/full ]; then
  e=`zerobytes 10 2>&1 > /dev/full || :`
  if [ x"${e}" != x"zerobytes: fatal: unable to write output: out of disk space" ]; then
    echo "${e}"; exit 111
  fi
fi
