#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`randomtext -h 2>&1 | grep '^randomtext: usage:'`" != x"randomtext: usage:" ]; then
  echo "randomtext: help error"; exit 111
fi

#bad/negative input
for i in a '' -1 -01 100000000000000000000000000000000000000; do
  e=`randombytes "${i}" 2>&1 | tail -1`
  if [ x"${e}" != x"randombytes: fatal: unable to parse number" ]; then
    echo "${e}"; exit 111
  fi
done

#zero length
for i in 0 +0 -0 000; do
  randomtext "${i}" | tr -d '\n'
done

#length check
if [ "`randomtext '+0100' | tr -d '\n' | wc -c`" -ne 100 ]; then
  echo "randomtext: length check failed"; exit 111
fi

#randomtext
dict=abcdefghijklmnopqrstuvwyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789; export dict
mask=_____________________________________________________________; export mask
res=''
for i in `awk 'BEGIN{for(i=1;i<=100;++i)print i;exit}'`; do
  length="${i}"
  res="${res}_"
done
export length
export res

x=`randomtext "${length}" "${dict}" | sed "y/${dict}/${mask}/"`
if [ x"${x}" != x"${res}" ]; then
  echo "randomtext: sed test failed"; exit 111
fi

#disk full
if [ -w /dev/full ]; then
  e=`randomtext 10 2>&1 > /dev/full || :`
  if [ x"${e}" != x"randomtext: fatal: unable to write output: out of disk space" ]; then
    echo "${e}"; exit 111
  fi
fi
