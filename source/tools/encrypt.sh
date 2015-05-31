#20121219
#Jan Mojzis
#Public domain.

#help
if [ x"`encrypt -h 2>&1 | grep '^encrypt: usage:'`" != x"encrypt: usage:" ]; then
  echo "encrypt test failed: help error"; exit 111
fi

#PASSWORD not set
e=`env - PATH=${PATH} encrypt </dev/null 2>&1 | tail -1 || :`
if [ x"${e}" != "xencrypt: fatal: \$PASSWORD not set" ]; then
  echo "encrypt test failed: ${e}"; exit 111
fi

#encrypt test
#for i in `awk 'BEGIN{for(i=4090;i<4100;++i)print i;exit}'`; do
for i in `awk 'BEGIN{for(i=0;i<20;++i)print i;exit}'`; do
  export i
  PASSWORD=`randomtext "${i}"`; export PASSWORD
  randombytes "${i}" | encrypt 1>/dev/null
done

#disk full
PASSWORD=ahoj; export PASSWORD
if [ -w /dev/full ]; then
  e=`encrypt < /dev/null 2>&1 > /dev/full || :`
  if [ x"${e}" != x"encrypt: fatal: unable to write output: out of disk space" ]; then
    echo "encrypt test failed: ${e}"; exit 111
  fi
fi
